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
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include <dt-bindings/soc/nxs_tid.h>
#include <linux/soc/nexell/nxs_function.h>
#include <linux/soc/nexell/nxs_dev.h>

int nxs_set_control(const struct nxs_dev *pthis, int type,
		    const struct nxs_control *pparam)
{
	int i;
	const struct nxs_dev_service *pservice;

	for (i = 0; i < NXS_MAX_SERVICES; i++) {
		pservice = &pthis->dev_services[i];
		if (pservice->type == type)
			return pservice->set_control(pthis, pparam);
	}

	return 0;
}

int nxs_get_control(const struct nxs_dev *pthis, int type,
		    struct nxs_control *pparam)
{
	int i;
	const struct nxs_dev_service *pservice;

	for (i = 0; i < NXS_MAX_SERVICES; i++) {
		pservice = &pthis->dev_services[i];
		if (pservice->type == type)
			return pservice->get_control(pthis, pparam);
	}

	return 0;
}

int nxs_dev_parse_dt(struct platform_device *pdev, struct nxs_dev *pthis)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;

	if (of_property_read_u32(np, "module", &pthis->dev_inst_index)) {
		dev_err(dev, "failed to get module\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "version", &pthis->dev_ver)) {
		dev_err(dev, "failed to get version\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "function", &pthis->dev_function)) {
		dev_err(dev, "failed to get function\n");
		return -EINVAL;
	}

	if (of_property_read_u16(np, "tid", &pthis->tid)) {
		dev_err(dev, "failed to get tid\n");
		return -EINVAL;
	}

	if (of_property_read_u16(np, "tid2", &pthis->tid2))
		pthis->tid2 = NXS_TID_DEFAULT;

	if (of_property_read_u32(np, "max_refcount", &pthis->max_refcount))
		pthis->max_refcount = 1;

	if (of_property_read_u32(np, "can_multitap_follow",
				 &pthis->can_multitap_follow))
		pthis->can_multitap_follow = 0;

	ret = platform_get_irq(pdev, 0);
	if (ret >= 0)
		pthis->irq = ret;
	else
		pthis->irq = -1;

	atomic_set(&pthis->refcount, 0);
	atomic_set(&pthis->connect_count, 0);

	INIT_LIST_HEAD(&pthis->list);
	INIT_LIST_HEAD(&pthis->func_list);
	INIT_LIST_HEAD(&pthis->sibling_list);
	INIT_LIST_HEAD(&pthis->disp_list);

	INIT_LIST_HEAD(&pthis->irq_callback);
	spin_lock_init(&pthis->irq_lock);

	return 0;
}

int nxs_dev_register_irq_callback(struct nxs_dev *pthis, u32 type,
				  struct nxs_irq_callback *callback)
{
	unsigned long flags;

	spin_lock_irqsave(&pthis->irq_lock, flags);
	list_add_tail(&callback->list, &pthis->irq_callback);
	spin_unlock_irqrestore(&pthis->irq_lock, flags);

	return 0;
}

int nxs_dev_unregister_irq_callback(struct nxs_dev *pthis, u32 type,
				    struct nxs_irq_callback *callback)
{
	unsigned long flags;

	spin_lock_irqsave(&pthis->irq_lock, flags);
	list_del_init(&callback->list);
	spin_unlock_irqrestore(&pthis->irq_lock, flags);

	return 0;
}
