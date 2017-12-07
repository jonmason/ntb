/*
 * Copyright (C) 2017  Nexell Co., Ltd.
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
#include <linux/err.h>
#include <linux/nexell_ion.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/dma-mapping.h>
#include "../ion_priv.h"

struct nexell_ion_type_id_table {
	const char *name;
	enum ion_heap_type type;
};

static struct nexell_ion_type_id_table type_id_table[] = {
	{"ion_system_contig", ION_HEAP_TYPE_SYSTEM_CONTIG},
	{"ion_system", ION_HEAP_TYPE_SYSTEM},
	{"ion_carveout", ION_HEAP_TYPE_CARVEOUT},
	{"ion_chunk", ION_HEAP_TYPE_CHUNK},
	{"ion_dma", ION_HEAP_TYPE_DMA},
};

#define NEXELL_ION_HEAP_NUM	16
#define NEXELL_ION_NAME_LEN	16

static struct ion_device *nexell_ion_device;
static struct ion_heap *nexell_ion_heap[NEXELL_ION_HEAP_NUM];
static struct ion_platform_data nexell_ion_platform_data;
static struct ion_platform_heap nexell_ion_platform_heap[NEXELL_ION_HEAP_NUM];

struct ion_device *nexell_get_ion_device(void)
{
	return nexell_ion_device;
}
EXPORT_SYMBOL_GPL(nexell_get_ion_device);

static u64 _dmamask = DMA_BIT_MASK(32);
static struct platform_device ion_dma_device = {
	.name = "ion-dma-device",
	.id = -1,
	.dev = {
		.dma_mask = &_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	}
};

static int get_type_by_name(const char *name, enum ion_heap_type *type)
{
	int i, n;

	n = sizeof(type_id_table)/sizeof(type_id_table[0]);
	for (i = 0; i < n; i++) {
		if (strncmp(name, type_id_table[i].name, NEXELL_ION_NAME_LEN))
			continue;

		*type = type_id_table[i].type;
		return 0;
	}
	return -1;
}

static int nexell_ion_setup_platform_data(struct platform_device *pdev)
{
	struct device_node *node, *np;
	const char *type_name;
	enum ion_heap_type type;
	int ret;
	int index = 0;

	node = pdev->dev.of_node;
	for_each_child_of_node(node, np) {
		ret = of_property_read_string(np, "heap-type", &type_name);
		if (ret < 0) {
			dev_err(&pdev->dev, "%s: failed to get heap-type\n",
				__func__);
			continue;
		}

		ret = get_type_by_name(type_name, &type);
		if (ret < 0) {
			dev_err(&pdev->dev, "%s: failed to get type by %s\n",
				__func__, type_name);
			continue;
		}

		nexell_ion_platform_heap[index].name = type_name;
		nexell_ion_platform_heap[index].id = type;
		nexell_ion_platform_heap[index].type = type;
		if (type == ION_HEAP_TYPE_DMA) {
			nexell_ion_platform_heap[index].priv =
				(void *)&ion_dma_device.dev;
			arch_setup_dma_ops(&ion_dma_device.dev, 0, 0, NULL,
					   false);
		}

		index++;
	}

	if (index > 0) {
		nexell_ion_platform_data.nr = index;
		nexell_ion_platform_data.heaps = nexell_ion_platform_heap;
		return 0;
	}

	return -ENOENT;
}

static void destroy_all_heap(void)
{
	int i;

	for (i = 0; i < NEXELL_ION_HEAP_NUM; i++) {
		if (!nexell_ion_heap[i])
			continue;
		ion_heap_destroy(nexell_ion_heap[i]);
		nexell_ion_heap[i] = NULL;
	}
}

static int nexell_ion_probe(struct platform_device *pdev)
{
	int i, err;
	struct ion_heap *heap;
	struct ion_platform_heap *heap_data;

	err = nexell_ion_setup_platform_data(pdev);
	if (err) {
		dev_err(&pdev->dev, "%s: failed to setup ion platform data\n",
			__func__);
		return err;
	}

	nexell_ion_device = ion_device_create(NULL);
	if (IS_ERR_OR_NULL(nexell_ion_device))
		return PTR_ERR(nexell_ion_device);

	for (i = 0; i < nexell_ion_platform_data.nr; i++) {
		heap_data = &nexell_ion_platform_data.heaps[i];
		heap = ion_heap_create(heap_data);
		if (IS_ERR_OR_NULL(heap)) {
			err = PTR_ERR(heap);
			goto out;
		}

		ion_device_add_heap(nexell_ion_device, heap);
		nexell_ion_heap[i] = heap;
	}

	platform_set_drvdata(pdev, nexell_ion_device);
	return 0;

out:
	destroy_all_heap();
	return err;
}

static int nexell_ion_remove(struct platform_device *pdev)
{
	ion_device_destroy(nexell_ion_device);
	nexell_ion_device = NULL;
	destroy_all_heap();
	return 0;
}

static struct of_device_id nexell_ion_match_table[] = {
	{.compatible = "nexell,ion"},
	{},
};

static struct platform_driver nexell_ion_driver = {
	.probe = nexell_ion_probe,
	.remove = nexell_ion_remove,
	.driver = {
		.name = "ion",
		.of_match_table = nexell_ion_match_table,
	},
};

#ifdef CONFIG_ION_INIT_LEVEL_UP
static int __init init_nexell_ion(void)
{
	return platform_driver_register(&nexell_ion_driver);
}
fs_initcall(init_nexell_ion);
#else
module_platform_driver(nexell_ion_driver);
#endif
