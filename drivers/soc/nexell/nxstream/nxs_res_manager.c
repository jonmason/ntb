/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Sungwoo, Park <swpark@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>

#include <linux/nxs_ioctl.h>

#include <linux/soc/nexell/nxs_function.h>
#include <linux/soc/nexell/nxs_dev.h>
#include <linux/soc/nexell/nxs_res_manager.h>

struct nxs_res_manager {
	struct device *dev;

	struct mutex lock;
	struct list_head dev_list;
	struct list_head used_list;

	struct nxs_function_builder *builder;
};

static struct nxs_res_manager *res_manager;

int register_nxs_dev(struct nxs_dev *nxs_dev)
{
	if (!res_manager)
		BUG();

	mutex_lock(&res_manager->lock);
	list_add_tail(&nxs_dev->list, &res_manager->dev_list);
	mutex_unlock(&res_manager->lock);

	return 0;
}
EXPORT_SYMBOL_GPL(register_nxs_dev);

void unregister_nxs_dev(struct nxs_dev *nxs_dev)
{
	struct nxs_dev *entry;

	mutex_lock(&res_manager->lock);
	list_for_each_entry(entry, &res_manager->dev_list, list) {
		if (entry == nxs_dev) {
			list_del_init(&entry->list);
			break;
		}
	}
	mutex_unlock(&res_manager->lock);
}
EXPORT_SYMBOL_GPL(unregister_nxs_dev);

struct nxs_function *request_nxs_function(const char *name,
					  struct nxs_function_request *req,
					  bool use_builder)
{
	if (use_builder) {
		if (res_manager->builder)
			return res_manager->builder->build(res_manager->builder,
							   name, req);
		return NULL;
	} else
		return nxs_function_build(req);
}
EXPORT_SYMBOL_GPL(request_nxs_function);

void remove_nxs_function(struct nxs_function *f, bool use_builder)
{
	if (use_builder) {
		if (res_manager->builder)
			res_manager->builder->free(res_manager->builder, f);
	} else
		nxs_function_destroy(f);
}
EXPORT_SYMBOL_GPL(remove_nxs_function);

void mark_multitap_follow(struct list_head *head)
{
	struct nxs_function_elem *elem;

	list_for_each_entry(elem, head, list)
		elem->multitap_follow = true;
}
EXPORT_SYMBOL_GPL(mark_multitap_follow);

struct nxs_dev *get_nxs_dev(u32 function, u32 index, u32 user,
			    bool multitap_follow)
{
	struct nxs_dev *entry;
	bool found = false;

	mutex_lock(&res_manager->lock);

	list_for_each_entry(entry, &res_manager->dev_list, list) {
		if (entry->dev_function == function &&
		    (index == NXS_FUNCTION_ANY ||
		     index == entry->dev_inst_index)) {
			u32 refcount;

			/* user check */
			if (entry->user != NXS_FUNCTION_USER_NONE &&
			    entry->user != user) {
				continue;
			}

			refcount = atomic_read(&entry->refcount);
			if (refcount < entry->max_refcount) {
				found = true;
				entry->multitap_connected = multitap_follow;
				if (multitap_follow)
					atomic_inc(&entry->connect_count);
				atomic_inc(&entry->refcount);
				break;
			} else {
				if (entry->multitap_connected &&
				    multitap_follow &&
				    (refcount - 1) < entry->max_refcount) {
					found = true;
					atomic_inc(&entry->connect_count);
					break;
				}
				dev_dbg(res_manager->dev,
				  "[%s:%d] refcount(%d) over max_refcount(%d)\n",
				  nxs_function_to_str(entry->dev_function),
				  entry->dev_inst_index,
				  refcount, entry->max_refcount);
			}
		}
	}

	if (found)
		entry->user = user;
	else
		entry = NULL;

	mutex_unlock(&res_manager->lock);

	return entry;
}
EXPORT_SYMBOL_GPL(get_nxs_dev);

void put_nxs_dev(struct nxs_dev *nxs_dev)
{
	struct nxs_dev *entry = NULL;

	mutex_lock(&res_manager->lock);

	list_for_each_entry(entry, &res_manager->dev_list, list) {
		if (entry == nxs_dev) {
			u32 refcount = atomic_read(&entry->refcount);

			if (refcount == 0) {
				dev_err(res_manager->dev,
					"%s: invalid call because refcount is zero\n",
					__func__);
			}

			if (entry->multitap_connected) {
				u32 connect_count =
					atomic_read(&entry->connect_count);

				if (connect_count > 0)
					atomic_dec(&entry->connect_count);

				if (atomic_read(&entry->connect_count) == 0) {
					entry->multitap_connected = false;
					atomic_dec(&entry->refcount);
				}
			} else {
				atomic_dec(&entry->refcount);
			}

			if (atomic_read(&entry->refcount) == 0)
			    entry->user = NXS_FUNCTION_USER_NONE;

			break;
		}
	}

	mutex_unlock(&res_manager->lock);
}
EXPORT_SYMBOL_GPL(put_nxs_dev);

int register_nxs_function_builder(struct nxs_function_builder *builder)
{
	if (res_manager->builder) {
		dev_err(res_manager->dev,
			"%s: Already registered other builder\n", __func__);
		return -EBUSY;
	}

	dev_info(res_manager->dev, "register builder %p\n", builder);
	res_manager->builder = builder;

	return 0;
}
EXPORT_SYMBOL_GPL(register_nxs_function_builder);

void unregister_nxs_function_builder(struct nxs_function_builder *builder)
{
	if (res_manager->builder && res_manager->builder == builder) {
		dev_info(res_manager->dev, "unregister builder %p\n", builder);
		res_manager->builder = NULL;
	}
}
EXPORT_SYMBOL_GPL(unregister_nxs_function_builder);

static int nxs_res_open(struct inode *inode, struct file *file)
{
	file->private_data = res_manager;
	return 0;
}

static int nxs_res_release(struct inode *inode, struct file *file)
{
	return 0;
}

static int get_function_index(struct nxs_function *f, int func)
{
	struct nxs_dev *nxs_dev;

	list_for_each_entry(nxs_dev, &f->dev_list, list) {
		if (nxs_dev->dev_function == func)
			return nxs_dev->dev_inst_index;
	}

	return -ENOENT;
}

static struct nxs_function_request *
make_function_request(struct nxs_res_manager *manager,
		      struct nxs_request_function *req)
{
	int i;
	struct nxs_function_request *func_req;

	func_req = kzalloc(sizeof(*func_req), GFP_KERNEL);
	if (!func_req) {
		WARN_ON(1);
		return NULL;
	}
	INIT_LIST_HEAD(&func_req->head);

	func_req->flags = req->flags;
	memcpy(&func_req->option, &req->option, sizeof(func_req->option));

	for (i = 0; i < req->count * 2; i += 2) {
		struct nxs_function_elem *elem;

		elem = kzalloc(sizeof(*elem), GFP_KERNEL);
		if (!elem) {
			WARN_ON(1);
			goto error_out;
		}

		elem->function = req->array[i];
		elem->index = req->array[i+1];
		elem->user = NXS_FUNCTION_USER_APP;
		list_add_tail(&elem->list, &func_req->head);
		func_req->nums_of_function++;
	}

	return func_req;

error_out:
	nxs_free_function_request(func_req);
	return NULL;
}

static int handle_request_function(struct nxs_res_manager *manager,
				   unsigned long arg)
{
	struct nxs_request_function req;
	struct nxs_function_request *func_req = NULL;
	struct nxs_function *f;

	if (copy_from_user(&req, (void __user *)arg, sizeof(req)))
		return -EFAULT;

	func_req = make_function_request(manager, &req);
	if (!func_req) {
		dev_err(manager->dev,
			"%s: failed to make_function_request for %s\n",
			__func__, req.name);
		return -EINVAL;
	}

	f = request_nxs_function(req.name, func_req, true);
	if (!f) {
		dev_err(manager->dev, "%s: failed to request_nxs_function\n",
			__func__);
		nxs_free_function_request(func_req);
		return -ENOENT;
	}

	req.handle = f->id;
	if (copy_to_user((void __user *)arg, &req, sizeof(req))) {
		remove_nxs_function(f, true);
		return -EFAULT;
	}

	return 0;
}

static int handle_remove_function(struct nxs_res_manager *manager,
				  unsigned long arg)
{
	struct nxs_remove_function remove;

	if (copy_from_user(&remove, (void __user *)arg, sizeof(remove)))
		return -EFAULT;

	if (manager->builder->get) {
		struct nxs_function *f;

		f = manager->builder->get(manager->builder, remove.handle);
		if (!f) {
			dev_err(manager->dev, "can't get function for handle %d\n",
				remove.handle);
			return -ENOENT;
		}

		return manager->builder->free(manager->builder, f);
	}

	return -EINVAL;
}

static int handle_query_function(struct nxs_res_manager *manager,
				 unsigned long arg)
{
	struct nxs_query_function query;
	int ret;

	if (copy_from_user(&query, (void __user *)arg, sizeof(query)))
		return -EFAULT;

	if (manager->builder->query) {
		ret = manager->builder->query(manager->builder, &query);
		if (ret < 0)
			return ret;
		if (copy_to_user((void __user *)arg, &query, sizeof(query)))
			return -EFAULT;

		return ret;
	}

	dev_err(manager->dev, "%s: no query callback\n", __func__);

	return -EINVAL;
}

static long nxs_res_ioctl(struct file *file, unsigned int req,
			  unsigned long arg)
{
	long ret = 0;
	struct nxs_res_manager *manager;

	manager = file->private_data;

	switch (req) {
	case NXS_REQUEST_FUNCTION:
		ret = handle_request_function(manager, arg);
		break;

	case NXS_REMOVE_FUNCTION:
		ret = handle_remove_function(manager, arg);
		break;

	case NXS_QUERY_FUNCTION:
		ret = handle_query_function(manager, arg);
		break;

	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static const struct file_operations nxs_res_fops = {
	.owner		= THIS_MODULE,
	.open		= nxs_res_open,
	.release	= nxs_res_release,
	.unlocked_ioctl	= nxs_res_ioctl,
};

static struct miscdevice nxs_res_miscdev = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= "nxs_res",
	.fops		= &nxs_res_fops,
};

static int nxs_res_manager_probe(struct platform_device *pdev)
{
	int ret = 0;

	if (res_manager) {
		pr_err("%s: already probed\n", __func__);
		return -EBUSY;
	}

	res_manager = devm_kzalloc(&pdev->dev, sizeof(*res_manager),
				   GFP_KERNEL);
	if (!res_manager)
		return -ENOMEM;

	dev_info(&pdev->dev, "%s Entered\n", __func__);

	INIT_LIST_HEAD(&res_manager->dev_list);
	mutex_init(&res_manager->lock);

	res_manager->dev = &pdev->dev;

	ret = misc_register(&nxs_res_miscdev);
	if (ret)
		dev_err(&pdev->dev, "%s: failed to misc_register\n", __func__);

	return ret;
}

static int nxs_res_manager_remove(struct platform_device *pdev)
{
	misc_deregister(&nxs_res_miscdev);
	res_manager = NULL;
	return 0;
}

static const struct of_device_id nxs_res_manager_match[] = {
	{ .compatible = "nexell,nxs_resource_manager-1.0", },
	{},
};

static struct platform_driver nxs_res_manager_driver = {
	.probe	= nxs_res_manager_probe,
	.remove	= nxs_res_manager_remove,
	.driver	= {
		.name = "nxs-res-manager",
		.of_match_table = of_match_ptr(nxs_res_manager_match),
	},
};

static int __init nxs_res_manager_init(void)
{
	return platform_driver_register(&nxs_res_manager_driver);
}
/* subsys_initcall(nxs_res_manager_init); */
fs_initcall(nxs_res_manager_init);

static void __exit nxs_res_manager_exit(void)
{
	platform_driver_unregister(&nxs_res_manager_driver);
}
module_exit(nxs_res_manager_exit);

MODULE_DESCRIPTION("Nexell Stream resource manager driver");
MODULE_AUTHOR("Sungwoo Park, <swpark@nexell.co.kr>");
MODULE_LICENSE("GPL");
