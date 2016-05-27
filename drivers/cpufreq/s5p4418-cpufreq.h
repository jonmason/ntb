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

#ifndef __S5P4418_ASV_H__
#define __S5P4418_ASV_H__

extern void nxp_cpu_id_ecid(u32 ecid[4]);
extern void nxp_cpu_id_string(u32 string[12]);

#define	CPU_ID_S5P4418		(0xE4418000)
#define	VOLTAGE_STEP_UV		(12500)	/* 12.5 mV */
#define	ASV_DEFAULT_LEVEL	(0)		/* for 4330 */

#define ASV_TABLE_COND(id)	(id == CPU_ID_S5P4418)


#define	FREQ_MAX_FREQ_KHZ	(1200*1000)
#define	FREQ_ARRAY_SIZE		(9)
#define	UV(v)				(v*1000)

struct asv_tb_info {
	int ids;
	int ro;
	long mhz[FREQ_ARRAY_SIZE];
	long uv[FREQ_ARRAY_SIZE];
};

#define	ASB_FREQ_MHZ {	\
	[0] = 1200,	\
	[1] = 1100,	\
	[2] = 1000,	\
	[3] =  900,	\
	[4] =  800,	\
	[5] =  700,	\
	[6] =  600,	\
	[7] =  500,	\
	[8] =  400,	\
	}

static struct asv_tb_info asv_tables[] = {
	[0] = {	.ids = 10, .ro = 110,
			.mhz = ASB_FREQ_MHZ,
			.uv  = { UV(1250), UV(1200), UV(1175),
				UV(1150), UV(1125), UV(1100),
				UV(1075), UV(1075), UV(1075) },
	},
	[1] = {	.ids = 15, .ro = 130,
			.mhz = ASB_FREQ_MHZ,
			.uv  = { UV(1200), UV(1150), UV(1125),
				UV(1100), UV(1075), UV(1050),
				UV(1025), UV(1025), UV(1025) },
	},
	[2] = {	.ids = 20, .ro = 140,
			.mhz = ASB_FREQ_MHZ,
			.uv  = { UV(1150), UV(1100), UV(1075),
				UV(1050), UV(1025), UV(1000),
				UV(1000), UV(1000), UV(1000) },
	},
	[3] = {	.ids = 50, .ro = 170,
			.mhz = ASB_FREQ_MHZ,
			.uv  = { UV(1100), UV(1050), UV(1025),
				UV(1000), UV(1000), UV(1000),
				UV(1000), UV(1000), UV(1000) },
	},
	[4] = {	.ids = 50, .ro = 170,
			.mhz = ASB_FREQ_MHZ,
			.uv  = { UV(1050), UV(1000), UV(1000),
				UV(1000), UV(1000), UV(1000),
				UV(1000), UV(1000), UV(1000) },
	},
};
#define	ASV_ARRAY_SIZE	ARRAY_SIZE(asv_tables)

struct asv_param {
	int level;
	int ids, ro;
	int flag, group, shift;
};

static struct asv_tb_info *pasv_table;
static struct asv_param	asv_param = { 0, };

static inline unsigned int mtol(unsigned int data, int bits)
{
	unsigned int result = 0;
	unsigned int mask = 1;
	int i = 0;

	for (i = 0; i < bits ; i++) {
		if (data&(1<<i))
			result |= mask<<(bits-i-1);
	}
	return result;
}

static int s5p4418_asv_setup_table(unsigned long (*freq_tables)[2])
{
	unsigned int ecid[4] = { 0, };
	unsigned int string[12] = { 0, };
	int i, ids = 0, ro = 0;
	int idslv, rolv, asvlv;

	nxp_cpu_id_string(string);
	nxp_cpu_id_ecid(ecid);

	if (string[0] != CPU_ID_S5P4418) {
		asvlv = ASV_DEFAULT_LEVEL;
		goto asv_find;
	}

	/* Use Fusing Flags */
	if ((ecid[2] & (1<<0))) {
		int gs = mtol((ecid[2]>>1) & 0x07, 3);
		int ag = mtol((ecid[2]>>4) & 0x0F, 4);

		asv_param.level = (ag - gs);
		if (asv_param.level < 0)
			asv_param.level = 0;

		asv_param.flag = 1;
		asv_param.group = ag;
		asv_param.shift = gs;
		pasv_table = &asv_tables[asv_param.level];
		pr_info("DVFS: ASV[%d] IDS(%dmA) Ro(%d),\n",
			asv_param.level, pasv_table->ids, pasv_table->ro);
		pr_info("fusing shift(%d), group(%d)", gs, ag);
		goto asv_done;
	}

	/* Use IDS/Ro */
	ids = mtol((ecid[1]>>16) & 0xFF, 8);
	ro  = mtol((ecid[1]>>24) & 0xFF, 8);

	/* find IDS Level */
	for (i = 0; (ASV_ARRAY_SIZE-1) > i; i++) {
		pasv_table = &asv_tables[i];
		if (pasv_table->ids >= ids)
			break;
	}
	idslv = ASV_ARRAY_SIZE != i ? i : (ASV_ARRAY_SIZE-1);

	/* find RO Level */
	for (i = 0; (ASV_ARRAY_SIZE-1) > i; i++) {
		pasv_table = &asv_tables[i];
		if (pasv_table->ro >= ro)
			break;
	}
	rolv = ASV_ARRAY_SIZE != i ? i : (ASV_ARRAY_SIZE-1);

	/* find Lowest ASV Level */
	asvlv = idslv > rolv ? rolv : idslv;

asv_find:
	pasv_table = &asv_tables[asvlv];
	asv_param.level = asvlv;
	asv_param.ids = ids;
	asv_param.ro  = ro;
	pr_info("DVFS: ASV[%d] IDS(%dmA,%3d) Ro(%d,%3d)\n", asv_param.level,
	       pasv_table->ids, ids, pasv_table->ro, ro);

asv_done:
	for (i = 0; FREQ_ARRAY_SIZE > i; i++) {
		freq_tables[i][0] = pasv_table->mhz[i] * 1000;	/* frequency */
		freq_tables[i][1] = pasv_table->uv[i];		/* voltage */
	}

	return FREQ_ARRAY_SIZE;
}

static long s5p4418_asv_get_voltage(long freqkhz)
{
	long uv = 0;
	int i = 0;

	if (NULL == pasv_table)
		return -EINVAL;

	for (i = 0; FREQ_ARRAY_SIZE > i; i++) {
		if (freqkhz == (pasv_table->mhz[i]*1000)) {
			uv = pasv_table->uv[i];
			break;
		}
	}

	if (0 == uv) {
		pr_info("FAIL: %ldkhz is not exist on the ASV TABLEs !!!\n",
		       freqkhz);
		return -EINVAL;
	}

	return uv;
}

static int s5p4418_asv_modify_vol_table(unsigned long (*freq_tables)[2],
					int table_size,	long value,
					bool down, bool percent)
{
	long step_vol = VOLTAGE_STEP_UV;
	long uv, dv, new;
	int i = 0, n = 0;

	if (NULL == freq_tables || NULL == pasv_table || (0 > value))
		return -EINVAL;

	/* initialzie */
	for (i = 0; table_size > i; i++) {
		for (n = 0; FREQ_ARRAY_SIZE > n; n++) {
			if (freq_tables[i][0] == (pasv_table->mhz[n]*1000)) {
				freq_tables[i][1] = pasv_table->uv[n];
				break;
			}
		}
	}
	pr_info("DVFS:%s%ld%s\n", down?"-":"+", value, percent?"%":"mV");

	/* new voltage */
	for (i = 0; table_size > i; i++) {
		int al = 0;

		uv = freq_tables[i][1];
		dv = percent ? ((uv/100) * value) : (value*1000);
		new = down ? uv - dv : uv + dv;

		if ((new % step_vol)) {
			new = (new / step_vol) * step_vol;
			al = 1;
			if (down)
				new += step_vol;	/* Upper */
		}

		pr_info("%7ldkhz, %7ld (%s%ld) align %ld (%s) -> %7ld\n",
			freq_tables[i][0], freq_tables[i][1],
			down?"-":"+", dv, step_vol, al?"X":"O", new);

		freq_tables[i][1] = new;
	}
	return 0;
}

static long s5p4418_asv_get_vol_margin(long uv, long value,
				       bool down, bool percent)
{
	long step_vol = VOLTAGE_STEP_UV;
	long dv = percent ? ((uv/100) * value) : (value*1000);
	long new = down ? uv - dv : uv + dv;
	int al = 0;

	if (NULL == pasv_table)
		return -EINVAL;

	if ((new % step_vol)) {
		new = (new / step_vol) * step_vol;
		al = 1;
		if (down)
			new += step_vol;	/* Upper */
	}
	return new;
}

static int s5p4418_asv_current_label(char *buf)
{
	char *s = buf;

	if (NULL == pasv_table)
		return -EINVAL;

	if (s && pasv_table) {
		if (!asv_param.flag) {
			s += sprintf(s, "%d:%dmA,%d\n", asv_param.level,
				     asv_param.ids, asv_param.ro);
		} else {
			s += sprintf(s, "%d:G%d,S%d\n", asv_param.level,
				     asv_param.group, asv_param.shift);
		}
	}
	return (s - buf);
}

static struct cpufreq_asv_ops asv_ops = {
	.setup_table = s5p4418_asv_setup_table,
	.get_voltage = s5p4418_asv_get_voltage,
	.modify_vol_table = s5p4418_asv_modify_vol_table,
	.current_label = s5p4418_asv_current_label,
	.get_vol_margin = s5p4418_asv_get_vol_margin,
};

#endif


