/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Bon-gyu, KOO <freestyle@nexell.co.kr>
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

#include <linux/platform_device.h>
#include <linux/stmmac.h>
#include <linux/clk.h>
#include <linux/phy.h>
#include <linux/of_net.h>
#include <linux/reset.h>
#include <linux/ethtool.h>

#include "stmmac_platform.h"
#include "stmmac.h"

struct nexell_priv_data {
	int clk_enabled;
	struct clk *tx_clk;
	int wolopts;
};

static void nexell_get_wol(struct net_device *ndev, struct ethtool_wolinfo *wol)
{
	struct stmmac_priv *stpriv = netdev_priv(ndev);
	wol->supported = WAKE_MAGIC;
	wol->wolopts = stpriv->wolopts;
}

static int nexell_set_wol(struct net_device *ndev, struct ethtool_wolinfo *wol)
{
	struct stmmac_priv *stpriv = netdev_priv(ndev);
	u32 support = WAKE_MAGIC;
	int err;

	if (wol->wolopts & ~support)
		return -EOPNOTSUPP;

	err = phy_ethtool_set_wol(stpriv->phydev, wol);
	if (err < 0) {
		dev_err(stpriv->device, "The PHY does not support set_wol\n");
		return -EOPNOTSUPP;
	}

	spin_lock_irq(&stpriv->lock);
	stpriv->wolopts |= wol->wolopts;
	spin_unlock_irq(&stpriv->lock);

	return 0;
}

static void *nexell_gmac_setup(struct platform_device *pdev)
{
	struct nexell_priv_data *gmac;
	struct device *dev = &pdev->dev;

	gmac = devm_kzalloc(dev, sizeof(*gmac), GFP_KERNEL);
	if (!gmac)
		return ERR_PTR(-ENOMEM);

	gmac->tx_clk = devm_clk_get(dev, "nexell_gmac_tx");
	if (IS_ERR(gmac->tx_clk)) {
		dev_err(dev, "could not get tx clock\n");
		return gmac->tx_clk;
	}

	return gmac;
}

#define GMAC_GMII_RGMII_RATE	125000000

static int nexell_gmac_init(struct platform_device *pdev, void *priv)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct stmmac_priv *stpriv = NULL;
	struct nexell_priv_data *gmac = priv;

	if (ndev)
		stpriv = netdev_priv(ndev);

	if (stpriv && stpriv->stmmac_rst)
		reset_control_deassert(stpriv->stmmac_rst);

	clk_set_rate(gmac->tx_clk, GMAC_GMII_RGMII_RATE);
	clk_prepare_enable(gmac->tx_clk);
	gmac->clk_enabled = 1;
	gmac->wolopts = 0;

	return 0;
}

static void nexell_gmac_exit(struct platform_device *pdev, void *priv)
{
	struct nexell_priv_data *gmac = priv;

	if (gmac->clk_enabled) {
		clk_disable(gmac->tx_clk);
		gmac->clk_enabled = 0;
	}
	clk_unprepare(gmac->tx_clk);
}

static int nexell_gmac_probe(struct platform_device *pdev)
{
	struct plat_stmmacenet_data *plat_dat;
	struct stmmac_resources stmmac_res;
	int ret;

	ret = stmmac_get_platform_resources(pdev, &stmmac_res);
	if (ret)
		return ret;

	plat_dat = stmmac_probe_config_dt(pdev, &stmmac_res.mac);
	if (IS_ERR(plat_dat))
		return PTR_ERR(plat_dat);

	plat_dat->has_gmac = true;
	plat_dat->init = nexell_gmac_init;
	plat_dat->exit = nexell_gmac_exit;
	plat_dat->set_wol = nexell_set_wol;
	plat_dat->get_wol = nexell_get_wol;

	plat_dat->bsp_priv = nexell_gmac_setup(pdev);

	if (IS_ERR(plat_dat->bsp_priv))
		return PTR_ERR(plat_dat->bsp_priv);

	ret = nexell_gmac_init(pdev, plat_dat->bsp_priv);
	if (ret)
		return ret;

	return stmmac_dvr_probe(&pdev->dev, plat_dat, &stmmac_res);
}

static const struct of_device_id nexell_dwmac_match[] = {
	{ .compatible = "nexell,s5p6818-gmac", },
	{ }
};
MODULE_DEVICE_TABLE(of, nexell_dwmac_match);

static struct platform_driver nexell_dwmac_driver = {
	.probe  = nexell_gmac_probe,
	.remove = stmmac_pltfr_remove,
	.driver = {
		.name           = "nexell-dwmac",
		.pm		= &stmmac_pltfr_pm_ops,
		.of_match_table = nexell_dwmac_match,
	},
};
module_platform_driver(nexell_dwmac_driver);

MODULE_AUTHOR("Bon-gyu, KOO <freestyle@nexell.co.kr>");
MODULE_DESCRIPTION("Nexell DWMAC specific glue layer");
MODULE_LICENSE("GPL");
