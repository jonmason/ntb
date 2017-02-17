/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Hyunseok, Jung <hsjung@nexell.co.kr>
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/gpio.h>

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#ifdef CONFIG_RESET_CONTROLLER
#include <linux/reset.h>
#endif

#ifdef CONFIG_ARM_S5Pxx18_DEVFREQ
#include <linux/cpufreq.h>
#include <linux/pm_qos.h>
#include <linux/soc/nexell/cpufreq.h>
#endif

#define	I2C_CLOCK_RATE		(100000)	/* 100kHz */
#define	WAIT_ACK_TIME		(500)		/* wait 50 msec */

struct nx_i2c_register {
	unsigned int ICCR;	/* 0x00 : I2C Control Register */
	unsigned int ICSR;      /* 0x04 : I2C Status Register */
	unsigned int IAR;       /* 0x08 : I2C Address Register */
	unsigned int IDSR;      /* 0x0C : I2C Data Register */
	unsigned int STOPCON;   /* 0x10 : I2C Stop Control Register */
};

#define CLKSRC_CNT	2
#define CLKSRC_DIV16	16
#define CLKSRC_DIV256	256
#define CLKSCALE_MIN	2
#define CLKSCALE_MAX	16

/*
 *	local data and macro
 */
struct nx_i2c_hw {
	int port;
	int irqno;
	int scl_io;
	int sda_io;
	int clksrc;
	int clkscale;
	/* Register */
	void __iomem *base_addr;
};

#define	I2C_TRANS_RUN		(1<<0)
#define	I2C_TRANS_DONE		(1<<1)
#define	I2C_TRANS_ERR		(1<<2)

struct nx_i2c_param {
	struct nx_i2c_hw hw;
	spinlock_t lock;
	wait_queue_head_t wait_q;
	unsigned int condition;
	unsigned int rate;
	int	no_stop;
	u8	pre_data;
	int	request_ack;
	int	timeout;

	/* i2c trans data */
	struct i2c_adapter adapter;
	struct i2c_msg *msg;
	struct clk *clk;
	int	trans_count;
	int	trans_mode;
	int running;
	unsigned int trans_status;
	struct pinctrl *pctrl;
	struct pinctrl_state	*pins_sda_dft;
	struct pinctrl_state	*pins_scl_dft;
	struct pinctrl_state	*pins_sda_gpio;
	struct pinctrl_state	*pins_scl_gpio;
	/* test */
	int irq_count;
	int thd_count;
	int sda_delay;
	int retry_delay;
	struct device *dev;

#ifdef CONFIG_ARM_S5Pxx18_DEVFREQ
	struct notifier_block	freq_transition;
#endif
	unsigned long		clk_in;
};

/*
 * I2C control macro
 */
#define I2C_TXRXMODE_SLAVE_RX	0	/* Slave Receive Mode */
#define I2C_TXRXMODE_SLAVE_TX	1	/* Slave Transmit Mode */
#define I2C_TXRXMODE_MASTER_RX	2	/* Master Receive Mode */
#define I2C_TXRXMODE_MASTER_TX	3	/* Master Transmit Mode */

#define I2C_ICCR_OFFS		0x00
#define I2C_ICSR_OFFS		0x04
#define I2C_IDSR_OFFS		0x0C
#define I2C_STOP_OFFS		0x10

#define ICCR_IRQ_CLR_POS	8
#define ICCR_ACK_ENB_POS	7
#define ICCR_IRQ_ENB_POS	5
#define ICCR_IRQ_PND_POS	4
#define ICCR_CLK_SRC_POS	6
#define ICCR_CLK_VAL_POS	0

#define ICSR_MOD_SEL_POS	6
#define ICSR_SIG_GEN_POS	5
#define ICSR_BUS_BUSY_POS	5
#define ICSR_OUT_ENB_POS	4
#define ICSR_ARI_STA_POS	3 /* Arbitration */
#define ICSR_ACK_REV_POS	0 /* ACK */

#define STOP_ACK_GEM_POS	2
#define STOP_DAT_REL_POS	1  /* only slave transmode */
#define STOP_CLK_REL_POS	0  /* only master transmode */

#define _SETDATA(p, d)		(((struct nx_i2c_register *)p)->IDSR = d)
#define _GETDATA(p)		(((struct nx_i2c_register *)p)->IDSR)
#define _BUSOFF(p)		(((struct nx_i2c_register *)p)->ICSR & \
				 ~(1<<ICSR_OUT_ENB_POS))
#define _ACKSTAT(p)		(((struct nx_i2c_register *)p)->ICSR & \
				 (1<<ICSR_ACK_REV_POS))
#define _ARBITSTAT(p)		(((struct nx_i2c_register *)p)->ICSR & \
				 (1<<ICSR_ARI_STA_POS))
#define _INTSTAT(p)		(((struct nx_i2c_register *)p)->ICCR & \
				 (1<<ICCR_IRQ_PND_POS))

static inline void nx_i2c_start_dev(struct nx_i2c_param *par)
{
	void __iomem *base = par->hw.base_addr;
	unsigned int ICSR = 0, ICCR = 0;

	ICSR = readl(base+I2C_ICSR_OFFS);
	ICSR = (1<<ICSR_OUT_ENB_POS);
	writel(ICSR, (base+I2C_ICSR_OFFS));

	writel(par->pre_data, (base+I2C_IDSR_OFFS));

	ICCR = readl(base+I2C_ICCR_OFFS);
	ICCR &= ~(1<<ICCR_ACK_ENB_POS);
	ICCR |= (1<<ICCR_IRQ_ENB_POS);
	writel(ICCR, (base+I2C_ICCR_OFFS));
	ICSR = (par->trans_mode << ICSR_MOD_SEL_POS) |
		(1<<ICSR_SIG_GEN_POS) | (1<<ICSR_OUT_ENB_POS);
	writel(ICSR, (base+I2C_ICSR_OFFS));
}

static inline void nx_i2c_trans_dev(void __iomem *base, unsigned int ack,
				    int stop)
{
	unsigned int ICCR = 0, STOP = 0;

	ICCR = readl(base+I2C_ICCR_OFFS);
	ICCR &= ~(1<<ICCR_ACK_ENB_POS);
	ICCR |= ack << ICCR_ACK_ENB_POS;

	writel(ICCR, (base+I2C_ICCR_OFFS));
	if (stop) {
		STOP = readl(base+I2C_STOP_OFFS);
		STOP |= 1<<STOP_ACK_GEM_POS;
		writel(STOP, base+I2C_STOP_OFFS);
	}

	ICCR = readl((base+I2C_ICCR_OFFS));
	ICCR &= (~(1<<ICCR_IRQ_PND_POS) | (~(1<<ICCR_IRQ_PND_POS)));
	ICCR |= 1<<ICCR_IRQ_CLR_POS;
	ICCR |= 1<<ICCR_IRQ_ENB_POS;
	writel(ICCR, (base+I2C_ICCR_OFFS));
}

static int nx_i2c_stop_scl(struct nx_i2c_param *par)
{
	void __iomem *base = par->hw.base_addr;
	unsigned int ICSR = 0, ICCR = 0, STOP = 0;
	int gpio = par->hw.scl_io;
	unsigned long start;
	int timeout = 5, ret = 0;

	STOP = (1<<STOP_CLK_REL_POS);
	writel(STOP, (base+I2C_STOP_OFFS));
	ICSR = readl(base+I2C_ICSR_OFFS);
	ICSR &= ~(1<<ICSR_OUT_ENB_POS);
	ICSR = par->trans_mode << ICSR_MOD_SEL_POS;
	writel(ICSR, (base+I2C_ICSR_OFFS));
	ICCR = (1<<ICCR_IRQ_CLR_POS);
	writel(ICCR, (base+I2C_ICCR_OFFS));

	gpio_direction_input(gpio);
	if (!gpio_get_value(gpio)) {
		pinctrl_select_state(par->pctrl, par->pins_scl_gpio);
		gpio_direction_output(gpio, 1);
		start = jiffies;
		while (!gpio_get_value(gpio)) {
			if (time_after(jiffies, start + timeout)) {
				if (gpio_get_value(gpio))
					break;
				ret = -ETIMEDOUT;
				goto _stop_timeout;
			}
			cpu_relax();
		}
	}

_stop_timeout:
	pinctrl_select_state(par->pctrl, par->pins_scl_dft);
	return ret;
}

static inline void nx_i2c_stop_dev(struct nx_i2c_param *par, int nostop,
				   int read)
{
	void __iomem *base = par->hw.base_addr;
	unsigned int ICSR = 0, ICCR = 0;
	int delay = par->sda_delay;
	unsigned long flags;

	spin_lock_irqsave(&par->lock, flags);
	if (!nostop) {
		pinctrl_select_state(par->pctrl, par->pins_sda_gpio);
		gpio_direction_output(par->hw.sda_io, 0); /* SDA LOW */
		udelay(delay);
		nx_i2c_stop_scl(par);
		udelay(delay);
		gpio_set_value(par->hw.sda_io, 1);	/* STOP Signal Gen */
		pinctrl_select_state(par->pctrl, par->pins_sda_dft);
	} else {
		pinctrl_select_state(par->pctrl, par->pins_sda_gpio);
		gpio_direction_output(par->hw.sda_io, 1); /* SDA HIGH */
		udelay(delay);
		pinctrl_select_state(par->pctrl, par->pins_scl_gpio);
		gpio_direction_output(par->hw.scl_io, 1); /* SCL HIGH */
		ICSR = par->trans_mode << ICSR_MOD_SEL_POS;
		writel(ICSR, (base+I2C_ICSR_OFFS));

		ICCR = (1<<ICCR_IRQ_CLR_POS);
		writel(ICCR, (base+I2C_ICCR_OFFS));
		pinctrl_select_state(par->pctrl, par->pins_sda_dft);
		pinctrl_select_state(par->pctrl, par->pins_scl_dft);
	}
	spin_unlock_irqrestore(&par->lock, flags);
}

static inline void nx_i2c_wait_dev(struct nx_i2c_param *par, int *wait)
{
	void __iomem *base = par->hw.base_addr;
	unsigned int ICSR = 0;

	do {
		ICSR = readl(base+I2C_ICSR_OFFS);
		if (!(ICSR & (1<<ICSR_BUS_BUSY_POS)) &&
		    !(ICSR & (1<<ICSR_ARI_STA_POS)))
			break;
		mdelay(1);
	} while ((*wait)-- > 0);
}

static inline void nx_i2c_bus_off(struct nx_i2c_param *par)
{
	void __iomem *base = par->hw.base_addr;
	unsigned int ICSR = 0;

	ICSR &= ~(1<<ICSR_OUT_ENB_POS);
	writel(ICSR, (base+I2C_ICSR_OFFS));
}

/*
 *	Hardware I2C
 */
static inline void nx_i2c_set_clock(struct nx_i2c_param *par, int enable)
{
	int cksrc = (par->hw.clksrc == CLKSRC_DIV16) ?  0 : 1;
	int ckscl = par->hw.clkscale;
	void __iomem *base = par->hw.base_addr;
	unsigned int ICCR = 0, ICSR = 0;

	dev_dbg(par->dev, "%s: i2c.%d, src:%d, scale:%d, %s\n",
		__func__,  par->hw.port, cksrc, ckscl, enable?"on":"off");

	if (enable) {
		ICCR = readl(base+I2C_ICCR_OFFS);
		ICCR &= ~(0x0f | 1 << ICCR_CLK_SRC_POS);
		ICCR |= ((cksrc << ICCR_CLK_SRC_POS) | (ckscl-1));
		writel(ICCR, (base+I2C_ICCR_OFFS));
	} else {
		ICSR = readl(base+I2C_ICSR_OFFS);
		ICSR &= ~(1<<ICSR_OUT_ENB_POS);
		writel(ICSR, (base+I2C_ICSR_OFFS));
	}
}

static inline int nx_i2c_wait_busy(struct nx_i2c_param *par)
{
	void __iomem *base = par->hw.base_addr;
	int wait = 500;
	int ret = 0;

	dev_dbg(par->dev, "%s(i2c.%d, nostop:%d)\n", __func__,
		par->hw.port, par->no_stop);

	/* busy status check*/
	nx_i2c_wait_dev(par, &wait);
	if (0 == wait) {
		dev_err(par->dev, "Fail, i2c.%d is busy, arbitration %s ...\n",
			par->hw.port, _ARBITSTAT(base)?"busy":"free");
		ret = -1;
	}
	return ret;
}

static irqreturn_t nx_i2c_irq_thread(int irqno, void *dev_id)
{
	struct nx_i2c_param *par = dev_id;
	struct i2c_msg *msg = par->msg;
	void __iomem *base = par->hw.base_addr;
	u16 flags = par->msg->flags;
	int len = msg->len;
	int cnt = par->trans_count;

	par->thd_count++;

	/* Arbitration Check. */
	if ((_ARBITSTAT((void *)base) != 0)) {
		dev_err(par->dev,
			"Fail, arbit i2c.%d  addr [0x%02x] data[0x%02x],",
			par->hw.port, (msg->addr<<1), par->pre_data);
		dev_err(par->dev, "trans[%2d:%2d]\n", cnt, len);
		par->trans_status = I2C_TRANS_ERR;
		goto __irq_end;
	}

	if (par->request_ack && _ACKSTAT((void *)base)) {
		dev_dbg(par->dev,
			"Fail, noack i2c.%d addr [0x%02x] data[0x%02x],",
			par->hw.port, (msg->addr<<1), par->pre_data);
		dev_dbg(par->dev, "trans[%2d:%2d]\n", cnt, len);
		par->trans_status = I2C_TRANS_ERR;
		goto __irq_end;
	}

	if (I2C_TRANS_RUN == par->trans_status) {
		if (flags & I2C_M_RD) {
			int ack  = (len == cnt + 1) ? 0 : 1;
			int last = (len == cnt + 1) ? 1 : 0;

			par->request_ack = 0;
			if (0 == cnt) {
				/* first: address */
				nx_i2c_trans_dev(base, ack, 0);
				par->trans_count += 1;
				return IRQ_HANDLED;
			}

			msg->buf[cnt - 1] = _GETDATA((void *)base);

			if (len == par->trans_count) {
				par->trans_status = I2C_TRANS_DONE;
				goto __irq_end;
			} else {
				nx_i2c_trans_dev(base, ack, last);
				par->trans_count += 1;
				return IRQ_HANDLED;
			}
		} else {
			par->pre_data = msg->buf[cnt];
			par->request_ack =
				(msg->flags & I2C_M_IGNORE_NAK) ? 0 : 1;
			if (len == 0)
				par->trans_status = I2C_TRANS_DONE;
			else if (len == ++par->trans_count)
				par->trans_status = I2C_TRANS_DONE;

			_SETDATA((void *)base, msg->buf[cnt]);
			nx_i2c_trans_dev(base, 0,
				      par->trans_status == I2C_TRANS_DONE ?
				      1 : 0);

			return IRQ_HANDLED;
		}
	}

__irq_end:
	nx_i2c_stop_dev(par, par->no_stop, (flags & I2C_M_RD ? 0 : 1));

	par->condition = 1;
	wake_up(&par->wait_q);

	return IRQ_HANDLED;
}

static irqreturn_t nx_i2c_irq_handler(int irqno, void *dev_id)
{
	struct nx_i2c_param *par = dev_id;
	unsigned int ICCR = 0;
	void __iomem *base = par->hw.base_addr;
	unsigned long flags;

	spin_lock_irqsave(&par->lock, flags);

	par->irq_count++;

	if (!par->running) {
		dev_dbg(par->dev, "** I2C.%d IRQ NO RUN **\n", par->hw.port);
		spin_unlock_irqrestore(&par->lock, flags);
		return IRQ_HANDLED;
	}

	ICCR = readl((base+I2C_ICCR_OFFS));
	ICCR &= (~(1 << ICCR_IRQ_PND_POS) | (~(1 << ICCR_IRQ_ENB_POS)));
	ICCR |= 1<<ICCR_IRQ_CLR_POS;

	writel(ICCR, (base+I2C_ICCR_OFFS));

	spin_unlock_irqrestore(&par->lock, flags);

	return IRQ_WAKE_THREAD;
}

static int nx_i2c_trans_done(struct nx_i2c_param *par)
{
	void __iomem *base = par->hw.base_addr;
	struct i2c_msg *msg = par->msg;
	int wait, timeout, ret = 0;

	par->condition = 0;

	if (msg->len)
		wait = msg->len * msecs_to_jiffies(par->timeout);
	else
		wait = msecs_to_jiffies(par->timeout);
	timeout = wait_event_timeout(par->wait_q, par->condition, wait);

	if (par->condition) { /* done or error */
		if (I2C_TRANS_ERR == par->trans_status)
			ret = -1;
	} else {
		dev_dbg(par->dev,
			"Fail, i2c.%d %s [0x%02x] cond(%d) pend(%s) arbit(%s)",
			par->hw.port, (msg->flags&I2C_M_RD)?"R":"W",
		       (par->msg->addr<<1), par->condition,
		       _INTSTAT(base)?"yes":"no",
		       _ARBITSTAT(base)?"busy":"free");
		dev_dbg(par->dev, "tran(%d:%d,%d:%d) wait(%dms)\n",
			par->trans_count, msg->len, par->irq_count,
			par->thd_count, par->timeout);
		ret = -1;
	}

	if (0 > ret)
		nx_i2c_stop_dev(par, 0, 0);

	return ret;
}

static inline int nx_i2c_trans_data(struct nx_i2c_param *par,
				    struct i2c_msg *msg)
{
	u32 mode;
	u8  addr;

	if (msg->flags & I2C_M_TEN) {
		dev_err(par->dev, "Fail, i2c.%d not support ten bit",
			par->hw.port);
		dev_err(par->dev, "addr:0x%02x, flags:0x%x\n",
			(msg->addr<<1), msg->flags);
		return -1;
	}
	if (msg->flags & I2C_M_RD) {
		addr =  msg->addr << 1 | 1;
		mode = I2C_TXRXMODE_MASTER_RX;
	} else {
		addr = msg->addr << 1;
		mode = I2C_TXRXMODE_MASTER_TX;
	}

	dev_dbg(par->dev, "%s(i2c.%d, addr:0x%02x, %s)\n",
		__func__, par->hw.port, addr, msg->flags&I2C_M_RD?"R":"W");

	/* clear irq condition */
	par->msg = msg;
	par->condition = 0;
	par->pre_data = addr;
	par->request_ack = (msg->flags & I2C_M_IGNORE_NAK) ? 0 : 1;
	par->trans_count = 0;
	par->trans_mode = mode;
	par->trans_status = I2C_TRANS_RUN;

	nx_i2c_start_dev(par);

	/* wait for end transfer */
	return nx_i2c_trans_done(par);
}

static int nx_i2c_transfer(struct nx_i2c_param *par, struct i2c_msg *msg,
			   int num)
{
	int ret = -EAGAIN;

	dev_dbg(par->dev, "\n%s(flags:0x%x, %c)\n",
		__func__, msg->flags, msg->flags&I2C_M_RD?'R':'W');

	nx_i2c_set_clock(par, 1);

	if (0 > nx_i2c_wait_busy(par))
		goto err_i2c;

	if (0 > nx_i2c_trans_data(par, msg))
		goto err_i2c;

	ret = msg->len;

err_i2c:
	dev_dbg(par->dev, "%s : Err  ", __func__);
	if (ret != msg->len)
		msg->flags &= ~I2C_M_NOSTART;

	nx_i2c_wait_busy(par);
	nx_i2c_set_clock(par, 0);

	return ret;
}

static int nx_i2c_algo_xfer(struct i2c_adapter *adapter, struct i2c_msg *msgs,
			    int num)
{
	struct nx_i2c_param *par  = adapter->algo_data;
	struct i2c_msg  *tmsg = msgs;
	int i = adapter->retries;
	int j = num;
	int ret = -EAGAIN;
	int len = 0;
	int delay = par->retry_delay;
	int  (*transfer_i2c)(struct nx_i2c_param *, struct i2c_msg *, int);

	transfer_i2c = nx_i2c_transfer;
	par->running = 1;
	par->irq_count = 0;
	par->thd_count = 0;

	dev_dbg(par->dev, "\n %s(msg num:%d)\n", __func__, num);

	for ( ; j > 0; j--, tmsg++) {
		par->no_stop = (1 == j ? 0 : 1);
		len = tmsg->len;

		/* transfer */
		for (i = adapter->retries; i > 0; i--) {
			ret = transfer_i2c(par, tmsg, num);
			if (ret == len)
				break;
			udelay(delay);
			dev_dbg(par->dev, "i2c.%d addr 0x%02x (try:%d)\n",
				par->hw.port, tmsg->addr<<1,
				adapter->retries-i+1);
		}

		/* Error */
		if (ret != len)
			break;
	}

	par->running = 0;

	if (ret == len)
		return num;

	dev_dbg(par->dev,
		"Error: i2c.%d, addr:%02x, trans len:%d(%d), try:%d\n",
		par->hw.port, (msgs->addr<<1), ret, len, adapter->retries);

	return ret;
}

static u32 nx_i2c_algo_fn(struct i2c_adapter *adapter)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static struct i2c_algorithm nx_i2c_algo = {
	.master_xfer	= nx_i2c_algo_xfer,
	.functionality	= nx_i2c_algo_fn,
};

static void nx_i2c_set_clk_param(struct nx_i2c_param *par, unsigned long rate)
{
	unsigned int t_src = 0, t_div = 0;
	unsigned long calc_clk, t_clk = 0;
	unsigned long real_clk = 0, get_real_clk = 0, req_rate = 0;
	unsigned int i = 0, src = 0;
	int div = 0;

	req_rate = par->rate;
	t_clk = rate/CLKSRC_DIV16/CLKSCALE_MIN;

	for (i = 0; i < CLKSRC_CNT; i++) {
		src = (i == 0) ? CLKSRC_DIV16 : CLKSRC_DIV256;
		for (div = CLKSCALE_MIN ; div < CLKSCALE_MAX; div++) {
			get_real_clk = rate/src/div;
			if (get_real_clk > req_rate)
				calc_clk = get_real_clk - req_rate;
			else
				calc_clk = req_rate - get_real_clk;

			if (calc_clk < t_clk) {
				t_clk = calc_clk;
				t_div = div;
				t_src = src;
				real_clk = get_real_clk;
			} else if (calc_clk == 0) {
				t_div = div;
				t_src = src;
				real_clk = get_real_clk;
				break;
			} else
				break;
		}
		if (calc_clk == 0)
			break;
	}

	par->hw.clksrc = t_src;
	par->hw.clkscale = t_div;
	par->clk_in = rate;

	dev_dbg(par->dev, "%s.%d: %8ld hz [pclk=%ld, clk = %3d,", "nexell-i2c",
		par->hw.port, real_clk, rate, par->hw.clksrc);
	dev_dbg(par->dev, "scale=%2d, timeout=%4d ms]\n", par->hw.clkscale-1,
		par->timeout);
}

static int nx_i2c_set_param(struct nx_i2c_param *par,
			    struct platform_device *pdev)
{
	unsigned long rate;
	int ret;
	struct clk *clk;
#ifdef CONFIG_RESET_CONTROLLER
	struct reset_control *rst;
#endif

	/* set par hardware */
	par->hw.port = of_alias_get_id(pdev->dev.of_node, "i2c");
	par->hw.irqno = platform_get_irq(pdev, 0);
	par->hw.base_addr = of_iomap(pdev->dev.of_node, 0);
	par->hw.sda_io = of_get_gpio(pdev->dev.of_node, 0);
	par->hw.scl_io = of_get_gpio(pdev->dev.of_node, 1);
	par->no_stop = 0;
	par->timeout = WAIT_ACK_TIME;

	of_property_read_u32(pdev->dev.of_node, "sda-delay", &par->sda_delay);
	if (!par->sda_delay)
		par->sda_delay = 1;

	of_property_read_u32(pdev->dev.of_node, "retry-delay",
			     &par->retry_delay);
	if (!par->retry_delay)
		par->retry_delay = 0;

	of_property_read_u32(pdev->dev.of_node, "retry-cnt",
			     &par->adapter.retries);
	if (!par->adapter.retries)
		par->adapter.retries = 3;

	of_property_read_u32(pdev->dev.of_node, "rate", &par->rate);
	if (!par->rate)
		par->rate = I2C_CLOCK_RATE;

	clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(clk)) {
		ret = PTR_ERR(clk);
		return ret;
	}

	rate = clk_get_rate(clk);
	clk_prepare_enable(clk);
	par->clk = clk;

	nx_i2c_set_clk_param(par, rate);

#ifdef CONFIG_RESET_CONTROLLER
	rst = devm_reset_control_get(&pdev->dev, "i2c-reset");

	if (!IS_ERR(rst)) {
		if (reset_control_status(rst))
			reset_control_reset(rst);
	}
#endif
	/* init par resource */
	spin_lock_init(&par->lock);
	init_waitqueue_head(&par->wait_q);


	ret = devm_request_threaded_irq(par->dev, par->hw.irqno,
					nx_i2c_irq_handler, nx_i2c_irq_thread,
					IRQF_SHARED, "nexell-i2c", par);
	if (ret)
		dev_err(par->dev, "Fail, i2c.%d request irq %d ...\n",
			par->hw.port, par->hw.irqno);

	nx_i2c_bus_off(par);

	return ret;
}

#ifdef CONFIG_ARM_S5Pxx18_DEVFREQ

#define freq_to_i2c(_n) container_of(_n, struct nx_i2c_param, freq_transition)

static int nx_i2c_freq_transition(struct notifier_block *nb, unsigned long val,
				  void *data)
{
	unsigned long freq;
	struct nx_i2c_param *i2c = freq_to_i2c(nb);

	if (val == 0 || val == PM_QOS_DEFAULT_VALUE)
		val = NX_BUS_CLK_LOW_KHZ;

	/* change KHz to MHz and bus to pclk*/
	freq = (val * 1000) / 2;

	dev_dbg(i2c->dev, "[nx-devfreq] freq %lu, clk_in %lu\n",
		 freq, i2c->clk_in);

	if (freq != i2c->clk_in) {
		i2c_lock_adapter(&i2c->adapter);
		nx_i2c_set_clk_param(i2c, freq);
		i2c_unlock_adapter(&i2c->adapter);
	}

	return 0;
}

static inline int nx_i2c_register_freq_notifier(struct nx_i2c_param *i2c)
{
	i2c->freq_transition.notifier_call = nx_i2c_freq_transition;

	return nx_bus_add_notifier(&i2c->freq_transition);
}

static inline void nx_i2c_unregister_freq_notifier(struct nx_i2c_param *i2c)
{
	nx_bus_remove_notifier(&i2c->freq_transition);
	pm_qos_remove_notifier(PM_QOS_BUS_THROUGHPUT, &i2c->freq_transition);
}
#else
static inline int nx_i2c_register_freq_notifier(struct nx_i2c_param *i2c)
{
	return 0;
}

static inline void nx_i2c_unregister_freq_notifier(struct nx_i2c_param *i2c)
{
}
#endif

static int nx_i2c_probe(struct platform_device *pdev)
{
	struct nx_i2c_param *par = NULL;
	int ret = 0;

	dev_dbg(&pdev->dev, "%s name:%s, id:%d\n", __func__,
		pdev->name, pdev->id);

	/* allocate nx_i2c_param data */
	par = devm_kzalloc(&pdev->dev, sizeof(struct nx_i2c_param), GFP_KERNEL);
	if (!par)
		return -ENOMEM;

	par->dev = &pdev->dev;

	/* init par data struct */
	ret = nx_i2c_set_param(par, pdev);
	if (0 > ret)
		return ret;

	/* init par adapter */
	strlcpy(par->adapter.name, "nexell-i2c", I2C_NAME_SIZE);

	par->adapter.owner	= THIS_MODULE;
	par->adapter.nr		= par->hw.port;
	par->adapter.class	= I2C_CLASS_HWMON | I2C_CLASS_SPD;
	par->adapter.algo	= &nx_i2c_algo;
	par->adapter.algo_data	= par;
	par->adapter.dev.parent	= &pdev->dev;
	par->adapter.dev.of_node = pdev->dev.of_node;

	devm_gpio_request(&pdev->dev, par->hw.sda_io, NULL);
	devm_gpio_request(&pdev->dev, par->hw.scl_io, NULL);

	par->pctrl = devm_pinctrl_get(&pdev->dev);

	par->pins_sda_dft = pinctrl_lookup_state(par->pctrl, "sda_dft");
	par->pins_scl_dft = pinctrl_lookup_state(par->pctrl, "scl_dft");
	par->pins_sda_gpio = pinctrl_lookup_state(par->pctrl, "sda_gpio");
	par->pins_scl_gpio = pinctrl_lookup_state(par->pctrl, "scl_gpio");

	pinctrl_select_state(par->pctrl, par->pins_sda_dft);
	pinctrl_select_state(par->pctrl, par->pins_scl_dft);

	ret = nx_i2c_register_freq_notifier(par);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to register freq notifier for i2c %d\n",
			par->hw.port);
		return ret;
	}

	ret = i2c_add_numbered_adapter(&par->adapter);
	if (ret) {
		dev_err(&pdev->dev, "Fail, i2c.%d add to adapter ...\n",
			par->hw.port);
		return ret;
	}

	/* set driver data */
	platform_set_drvdata(pdev, par);
	return ret;
}

static int nx_i2c_remove(struct platform_device *pdev)
{
	struct nx_i2c_param *par = platform_get_drvdata(pdev);
#ifdef CONFIG_RESET_CONTROLLER
	struct reset_control *rst;

	rst = devm_reset_control_get(&pdev->dev, "i2c-reset");
	reset_control_assert(rst);
#endif
	clk_disable_unprepare(par->clk);

	i2c_del_adapter(&par->adapter);

	platform_set_drvdata(pdev, NULL);
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int nx_i2c_suspend_noirq(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct nx_i2c_param *par = platform_get_drvdata(pdev);
#ifdef CONFIG_RESET_CONTROLLER
	struct reset_control *rst;

	rst = devm_reset_control_get(&pdev->dev, "i2c-reset");
	reset_control_assert(rst);
#endif
	clk_disable_unprepare(par->clk);
	dev_dbg(&pdev->dev, "%s\n", __func__);
	return 0;
}

static int nx_i2c_resume_noirq(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct nx_i2c_param *par = platform_get_drvdata(pdev);
#ifdef CONFIG_RESET_CONTROLLER
	struct reset_control *rst;
#endif

	dev_dbg(&pdev->dev, "%s\n", __func__);
	pinctrl_select_state(par->pctrl, par->pins_sda_dft);
	pinctrl_select_state(par->pctrl, par->pins_scl_dft);
#ifdef CONFIG_RESET_CONTROLLER
	rst = devm_reset_control_get(&pdev->dev, "i2c-reset");

	if (!IS_ERR(rst)) {
		if (reset_control_status(rst))
			reset_control_reset(rst);
	}
#endif
	clk_prepare_enable(par->clk);
	mdelay(1);

	nx_i2c_bus_off(par);
	return 0;
}
#endif

#ifdef CONFIG_OF
static const struct of_device_id nx_i2c_match[] = {
	{ .compatible = "nexell,s5p4418-i2c" },
	{},
};
MODULE_DEVICE_TABLE(of, nx_i2c_match);
#else
#define nx_i2c_match NULL
#endif

#ifdef CONFIG_PM
static const struct dev_pm_ops nx_i2c_dev_pm_ops = {
#ifdef CONFIG_PM_SLEEP
	.suspend_noirq = nx_i2c_suspend_noirq,
	.resume_noirq = nx_i2c_resume_noirq,
	.freeze_noirq = nx_i2c_suspend_noirq,
	.thaw_noirq = nx_i2c_resume_noirq,
	.poweroff_noirq = nx_i2c_suspend_noirq,
	.restore_noirq = nx_i2c_resume_noirq,
#endif
};

#define NX_DEV_PM_OPS (&nx_i2c_dev_pm_ops)
#else
#define NX_DEV_PM_OPS NULL
#endif

static struct platform_driver i2c_plat_driver = {
	.probe	 = nx_i2c_probe,
	.remove	 = nx_i2c_remove,
	.driver	 = {
		.name	 = "nexell-i2c",
		.owner	 = THIS_MODULE,
		.pm	 = NX_DEV_PM_OPS,
		.of_match_table = of_match_ptr(nx_i2c_match),
	},
};

static int __init nx_i2c_init(void)
{
	return platform_driver_register(&i2c_plat_driver);
}

static void __exit nx_i2c_exit(void)
{
	platform_driver_unregister(&i2c_plat_driver);
}

subsys_initcall(nx_i2c_init);
module_exit(nx_i2c_exit);

MODULE_DESCRIPTION("I2C driver for the Nexell");
MODULE_AUTHOR("Hyunseok Jung, <hsjung@nexell.co.kr>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform: nexell i2c");
