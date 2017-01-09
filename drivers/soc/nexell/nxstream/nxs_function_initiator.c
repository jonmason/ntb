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
#include <linux/spinlock.h>
#include <linux/platform_device.h>

#include <linux/soc/nexell/nxs_function.h>
#include <linux/soc/nexell/nxs_res_manager.h>

static struct nxs_function_request *
make_nxs_function_request(struct device *dev, struct device_node *np, int index)
{
	int count;
	char node_name[16] = {0, };
	struct nxs_function_request *req = NULL;

	sprintf(node_name, "function%d", index);

	count = of_property_count_elems_of_size(np, node_name, 4);
	if (count > 0) {
		int i;
		u32 *array;

		dev_info(dev, "%s: entry count %d\n", __func__, count/2);
		req = kzalloc(sizeof(*req), GFP_KERNEL);
		if (!req) {
			WARN_ON(1);
			return NULL;
		}
		INIT_LIST_HEAD(&req->head);

		array = kcalloc(count, sizeof(u32), GFP_KERNEL);
		if (!array) {
			WARN_ON(1);
			kfree(req);
			return NULL;
		}

		of_property_read_u32_array(np, node_name, array, count);

		for (i = 0; i < count; i += 2) {
			struct nxs_function_elem *elem;

			elem = kzalloc(sizeof(*elem), GFP_KERNEL);
			if (!elem) {
				WARN_ON(1);
				kfree(array);
				goto fail_return;
			}

			elem->function = array[i];
			elem->index = array[i + 1];
			elem->user = NXS_FUNCTION_USER_KERNEL;

			if (elem->function == NXS_FUNCTION_MULTITAP)
				mark_multitap_follow(&req->head);

			dev_info(dev, "function %s index %d added to request %p\n",
				 nxs_function_to_str(elem->function),
				 elem->index, req);
			list_add_tail(&elem->list, &req->head);
			req->nums_of_function++;
		}

		kfree(array);
	}

	return req;

fail_return:
	nxs_free_function_request(req);
	return NULL;
}

static int nxs_function_initiator_probe(struct platform_device *pdev)
{
	int i;
	u32 num_functions;
	struct nxs_function_request *req;
	struct device_node *np = pdev->dev.of_node;

	dev_info(&pdev->dev, "%s entered\n", __func__);

	if (of_property_read_u32(np, "num_functions", &num_functions)) {
		dev_err(&pdev->dev, "failed to get num_functions\n");
		return -ENOENT;
	}

	if (num_functions <= 0) {
		dev_err(&pdev->dev, "num_functions is not valid\n");
		return -EINVAL;
	}

	for (i = 0; i < num_functions; i++) {
		req = make_nxs_function_request(&pdev->dev, np, i);
		if (req) {
			char name[32] = {0, };
			sprintf(name, "nxs_function%d", i);
			request_nxs_function(name, req, true);
		}
	}

	return 0;
}

static int nxs_function_initiator_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id nxs_function_initiator_match[] = {
	{ .compatible = "nexell,nxs_function_initiator-1.0", },
	{},
};

static struct platform_driver nxs_function_initiator_driver = {
	.probe = nxs_function_initiator_probe,
	.remove = nxs_function_initiator_remove,
	.driver = {
		.name = "nxs-function-initiator",
		.of_match_table = of_match_ptr(nxs_function_initiator_match),
	},
};

static int __init nxs_function_initiator_init(void)
{
	return platform_driver_register(&nxs_function_initiator_driver);
}
late_initcall(nxs_function_initiator_init);

static void __exit nxs_function_initiator_exit(void)
{
	platform_driver_unregister(&nxs_function_initiator_driver);
}
module_exit(nxs_function_initiator_exit);
