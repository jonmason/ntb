/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Youngbok, Park <ybpark@nexell.co.kr>
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
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/mmc/host.h>
#include <linux/mmc/dw_mmc.h>
#include <linux/mmc/mmc.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/reset.h>

#include "dw_mmc.h"
#include "dw_mmc-pltfm.h"
#include "dw_mmc-nexell.h"

struct dw_mci_nexell_priv_data {
	struct	reset_control *rst;
	u32	clkdly;
};

static int dw_mci_nexell_priv_init(struct dw_mci *host)
{
	struct dw_mci_nexell_priv_data *priv = host->priv;

	if (!IS_ERR(priv->rst)) {
		if (reset_control_status(priv->rst))
			reset_control_reset(priv->rst);
	}

	mci_writel(host, CLKCTRL, priv->clkdly);

	return 0;
}

static int dw_mci_nexell_setup_clock(struct dw_mci *host)
{
	host->bus_hz /= NX_SDMMC_CLK_DIV;
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int dw_mci_nexell_suspend(struct device *dev)
{
	struct dw_mci *host = dev_get_drvdata(dev);

	clk_disable_unprepare(host->biu_clk);
	clk_disable_unprepare(host->ciu_clk);

	return dw_mci_suspend(host);
}

static int dw_mci_nexell_resume(struct device *dev)
{
	struct dw_mci *host = dev_get_drvdata(dev);

	clk_prepare_enable(host->biu_clk);
	clk_prepare_enable(host->ciu_clk);

	dw_mci_nexell_priv_init(host);
	return dw_mci_resume(host);
}

#else
#define dw_mci_nexell_suspend		NULL
#define dw_mci_nexell_resume		NULL
#endif /* CONFIG_PM_SLEEP */

static void dw_mci_nexell_prepare_command(struct dw_mci *host, u32 *cmdr)
{
	*cmdr |= SDMMC_CMD_USE_HOLD_REG;
}

static int dw_mci_nexell_parse_dt(struct dw_mci *host)
{
	struct dw_mci_nexell_priv_data *priv;
	struct device_node *np = host->dev->of_node;
	int drive_delay, drive_shift, sample_delay, sample_shift;

	priv = devm_kzalloc(host->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	if (of_property_read_u32(np, "nexell,drive_dly", &drive_delay))
		drive_delay = 0;

	if (of_property_read_u32(np, "nexell,drive_shift", &drive_shift))
		drive_shift = 2;

	if (of_property_read_u32(np, "nexell,sample_dly", &sample_delay))
		sample_delay = 0;

	if (of_property_read_u32(np, "nexell,sample_shift", &sample_shift))
		sample_shift = 1;

	priv->clkdly = NX_MMC_CLK_DELAY(drive_delay, drive_shift,
					sample_delay, sample_shift);

	priv->rst = devm_reset_control_get(host->dev, "dw_mmc-reset");

	host->priv = priv;
	return 0;
}

/* Common capabilities of s5pxx18 SoC */
static unsigned long nexell_dwmmc_caps[4] = {
	MMC_CAP_CMD23,
	MMC_CAP_CMD23,
	MMC_CAP_CMD23,
	MMC_CAP_CMD23,
};

static const struct dw_mci_drv_data nexell_drv_data = {
	.caps			= nexell_dwmmc_caps,
	.init			= dw_mci_nexell_priv_init,
	.setup_clock		= dw_mci_nexell_setup_clock,
	.prepare_command	= dw_mci_nexell_prepare_command,
	.parse_dt		= dw_mci_nexell_parse_dt,
};

static const struct of_device_id dw_mci_nexell_match[] = {
	{ .compatible = "nexell,s5p6818-dw-mshc",
			.data = &nexell_drv_data, },
	{},
};
MODULE_DEVICE_TABLE(of, dw_mci_nexell_match);

static int dw_mci_nexell_probe(struct platform_device *pdev)
{
	const struct dw_mci_drv_data *drv_data;
	const struct of_device_id *match;

	match = of_match_node(dw_mci_nexell_match, pdev->dev.of_node);
	drv_data = match->data;
	return dw_mci_pltfm_register(pdev, drv_data);
}

static const struct dev_pm_ops dw_mci_nexell_pmops = {
	SET_SYSTEM_SLEEP_PM_OPS(dw_mci_nexell_suspend, dw_mci_nexell_resume)
};

static struct platform_driver dw_mci_nexell_pltfm_driver = {
	.probe		= dw_mci_nexell_probe,
	.remove		= dw_mci_pltfm_remove,
	.driver		= {
		.name		= "dwmmc_nexell",
		.of_match_table	= dw_mci_nexell_match,
		.pm		= &dw_mci_nexell_pmops,
	},
};

module_platform_driver(dw_mci_nexell_pltfm_driver);

MODULE_DESCRIPTION("Nexell Specific DW-MSHC Driver Extension");
MODULE_AUTHOR("Youngbok Park <ybpart@nexell.co.kr");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:dwmmc-nexell");
