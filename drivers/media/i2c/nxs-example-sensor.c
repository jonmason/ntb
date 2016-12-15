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

#include <linux/of.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/v4l2-mediabus.h>
#include <media/v4l2-device.h>
#include <media/v4l2-of.h>
#include <media/v4l2-subdev.h>

#define NXS_EXAMPLE_SENSOR_NAME		"nxs-example-sensor"

struct nxs_sensor {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct v4l2_of_endpoint bus_cfg;
	struct device *dev;
};

/* -----------------------------------------------------------------------------
 * V4L2 subdev video operations
 */
static int nxs_sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	pr_info("%s: enable --> %d\n", __func__, enable);
	return 0;
}

static const struct v4l2_subdev_video_ops nxs_sensor_video_ops = {
	.s_stream = nxs_sensor_s_stream,
};

static const struct v4l2_subdev_ops nxs_sensor_ops = {
	.video = &nxs_sensor_video_ops,
};

static int parse_dt(struct nxs_sensor *sensor)
{
	int ret = 0;
	struct v4l2_of_endpoint *bus_cfg = NULL;
	struct device_node *ep = NULL;
	struct device *dev = sensor->dev;
	struct device_node *np;

	np = dev->of_node;
	if (!np) {
		dev_err(dev, "%s: no of_node\n", __func__);
		ret = -EINVAL;
		goto out_err;
	}

	ep = of_graph_get_next_endpoint(np, NULL);
	if (!ep) {
		dev_err(dev, "%s: failed to get enpoint node\n", __func__);
		ret = -ENOENT;
		goto out_err;
	}

	bus_cfg = v4l2_of_alloc_parse_endpoint(ep);
	if (IS_ERR(bus_cfg)) {
		dev_err(dev, "%s: failed to v4l2_of_alloc_parse_endpoint\n",
			__func__);
		ret = PTR_ERR(bus_cfg);
		goto out_err;
	}

	memcpy(&sensor->bus_cfg, bus_cfg, sizeof(*bus_cfg));

out_err:
	if (bus_cfg)
		v4l2_of_free_endpoint(bus_cfg);
	if (ep)
		of_node_put(ep);

	return ret;
}

static int nxs_sensor_probe(struct i2c_client *client,
			    const struct i2c_device_id *devid)
{
	struct nxs_sensor *sensor;
	int ret;

	dev_info(&client->dev, "%s entered\n", __func__);

	sensor = devm_kzalloc(&client->dev, sizeof(*sensor), GFP_KERNEL);
	if (!sensor)
		return -ENOMEM;

	sensor->dev = &client->dev;

	ret = parse_dt(sensor);
	if (ret)
		return ret;

	v4l2_i2c_subdev_init(&sensor->sd, client, &nxs_sensor_ops);
	sensor->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	sensor->pad.flags = MEDIA_PAD_FL_SOURCE;
	ret = media_entity_init(&sensor->sd.entity, 1, &sensor->pad, 0);
	if (ret)
		return ret;

	ret = v4l2_async_register_subdev(&sensor->sd);
	if (ret)
		media_entity_cleanup(&sensor->sd.entity);

	dev_info(&client->dev, "%s exit: ret %d\n", __func__, ret);

	return ret;
}

static int nxs_sensor_remove(struct i2c_client *client)
{
	return 0;
}

static const struct of_device_id nxs_sensor_of_table[] = {
	{ .compatible = "nxs,example_sensor" },
	{ },
};
MODULE_DEVICE_TABLE(of, nxs_sensor_of_table);

static const struct i2c_device_id nxs_sensor_id_table[] = {
	{ NXS_EXAMPLE_SENSOR_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, nxs_sensor_id_table);

static struct i2c_driver nxs_sensor_i2c_driver = {
	.driver = {
		.of_match_table = nxs_sensor_of_table,
		.name = NXS_EXAMPLE_SENSOR_NAME,
	},
	.probe = nxs_sensor_probe,
	.remove = nxs_sensor_remove,
	.id_table = nxs_sensor_id_table,
};

module_i2c_driver(nxs_sensor_i2c_driver);

MODULE_AUTHOR("Sungwoo Park <swpark@nexell.co.kr>");
MODULE_DESCRIPTION("Nexell Stream Bus example camera sensor driver");
MODULE_LICENSE("GPL");
