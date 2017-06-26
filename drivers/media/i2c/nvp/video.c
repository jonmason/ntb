#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/io.h>
//#include <asm/system.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/string.h>
#include <linux/list.h>
#include <asm/delay.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <linux/moduleparam.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>

//#include "gpio_i2c.h"
#include "nvp6124.h"
#include "video.h"
#include "nvp6124_reg.h"
#include "nvp6124B_reg.h"

/*for 144MHz data,72M clock sampling*/
//#define SAMPLING_CLK_72M

#ifdef ONLY_720H
#define VDEC_108M
#endif

#ifndef VDEC_108M
#define VDEC_144M
#endif
#define NTSC   0
#define PAL    1
#define SET_ALL_CH          0xff

/*nvp6124 1080P ɫ???Ƽ?????*/
#define BRI_CENTER_VAL_NTSC 0xF4
#define BRI_CENTER_VAL_PAL  0xF4
#define CON_CENTER_VAL_NTSC 0x90
#define CON_CENTER_VAL_PAL  0x90
#define SAT_CENTER_VAL_NTSC 0x80
#define SAT_CENTER_VAL_PAL  0x80
#define HUE_CENTER_VAL_NTSC 0x00
#define HUE_CENTER_VAL_PAL  0x00


/*nvp6124 720P ɫ???Ƽ?????*/
#define BRI_CENTER_VAL_NTSC_720P 0x08
#define BRI_CENTER_VAL_PAL_720P  0x08
#define CON_CENTER_VAL_NTSC_720P 0x88
#define CON_CENTER_VAL_PAL_720P  0x88
#define SAT_CENTER_VAL_NTSC_720P 0x90
#define SAT_CENTER_VAL_PAL_720P  0x90
#define HUE_CENTER_VAL_NTSC_720P 0xFD
#define HUE_CENTER_VAL_PAL_720P  0x00

/*nvp6124 960H ɫ???Ƽ?????*/
#define BRI_CENTER_VAL_NTSC_960H 0xF8
#define BRI_CENTER_VAL_PAL_960H  0xF5
#define CON_CENTER_VAL_NTSC_960H 0x80
#define CON_CENTER_VAL_PAL_960H  0x79
#define SAT_CENTER_VAL_NTSC_960H 0xA0
#define SAT_CENTER_VAL_PAL_960H  0x80
#define HUE_CENTER_VAL_NTSC_960H 0x01
#define HUE_CENTER_VAL_PAL_960H  0x00

unsigned int nvp6124_con_tbl[2]  = {CON_CENTER_VAL_NTSC, CON_CENTER_VAL_PAL};
unsigned int nvp6124_hue_tbl[2]  = {HUE_CENTER_VAL_NTSC, HUE_CENTER_VAL_PAL};
unsigned int nvp6124_sat_tbl[2]  = {SAT_CENTER_VAL_NTSC, SAT_CENTER_VAL_PAL};
unsigned int nvp6124_bri_tbl[2]  = {BRI_CENTER_VAL_NTSC, BRI_CENTER_VAL_PAL};

unsigned int nvp6124_con_tbl_720P[2]  = {CON_CENTER_VAL_NTSC_720P, CON_CENTER_VAL_PAL_720P};
unsigned int nvp6124_hue_tbl_720P[2]  = {HUE_CENTER_VAL_NTSC_720P, HUE_CENTER_VAL_PAL_720P};
unsigned int nvp6124_sat_tbl_720P[2]  = {SAT_CENTER_VAL_NTSC_720P, SAT_CENTER_VAL_PAL_720P};
unsigned int nvp6124_bri_tbl_720P[2]  = {BRI_CENTER_VAL_NTSC_720P, BRI_CENTER_VAL_PAL_720P};


unsigned int nvp6124_con_tbl_960H[2]  = {CON_CENTER_VAL_NTSC_960H, CON_CENTER_VAL_PAL_960H};
unsigned int nvp6124_hue_tbl_960H[2]  = {HUE_CENTER_VAL_NTSC_960H, HUE_CENTER_VAL_PAL_960H};
unsigned int nvp6124_sat_tbl_960H[2]  = {SAT_CENTER_VAL_NTSC_960H, SAT_CENTER_VAL_PAL_960H};
unsigned int nvp6124_bri_tbl_960H[2]  = {BRI_CENTER_VAL_NTSC_960H, BRI_CENTER_VAL_PAL_960H};

unsigned char nvp6124_motion_sens_tbl[8]= {0xe0,0xc8,0xa0,0x98,0x78,0x68,0x50,0x48};
unsigned char ch_mode_status[16]={0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
unsigned char ch_mode_status_tmp[16]={0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};

extern unsigned int nvp6124_cnt;
extern int chip_id[4];
extern unsigned int nvp6124_mode;
extern int g_soc_chiptype;
extern void audio_init(unsigned char dec, unsigned char ch_num, unsigned char samplerate, unsigned char bits);


void nvp6124_ntsc_common_init(void);
void nvp6124_pal_common_init(void);
void nvp6124_write_table(unsigned char chip_addr, unsigned char addr, unsigned char *tbl_ptr, unsigned char tbl_cnt);

int nvp_i2c_write(unsigned int slave, u8 addr, u8 data);
extern int nvp_i2c_write(unsigned int slave, u8 addr, u8 data);

unsigned char nvp_i2c_read(unsigned char slave, unsigned char addr);
extern unsigned char nvp_i2c_read(unsigned char slave, unsigned char addr);

extern unsigned int nvp6124_mode;
extern unsigned int nvp6124_slave_addr[2];
void nvp6124B_outport_1mux_chseq(void);
void NVP6124_AfeReset(void)
{
	nvp_i2c_write(nvp6124_slave_addr[0], 0xFF, 0x00);
	nvp_i2c_write(nvp6124_slave_addr[1], 0xFF, 0x00);
	nvp_i2c_write(nvp6124_slave_addr[0], 0x02, nvp_i2c_read(nvp6124_slave_addr[0], 0x02)|0x0F);
	nvp_i2c_write(nvp6124_slave_addr[1], 0x02, nvp_i2c_read(nvp6124_slave_addr[1], 0x02)|0x0F);
	nvp_i2c_write(nvp6124_slave_addr[0], 0x02, nvp_i2c_read(nvp6124_slave_addr[0], 0x02)&0xF0);
	nvp_i2c_write(nvp6124_slave_addr[1], 0x02, nvp_i2c_read(nvp6124_slave_addr[1], 0x02)&0xF0);
	printk("NVP6124_AfeReset done\n");
}
EXPORT_SYMBOL(NVP6124_AfeReset);
void nvp6124_datareverse(void)
{
/*
BANK1 0xD2[5:2],ÿ??bit????һ??bt656??????˳????1Ϊ??????0Ϊ??????
*/
	int i = 0;
	for(i=0;i<nvp6124_cnt;i++)
	{
		nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x01);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xD2, 0x3C);
	}
	printk("nvp6124 data reversed\n");
}

void nvp6124B_system_init(void)
{
    int i = 0;
    for(i=0;i<nvp6124_cnt;i++)
    {
        nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x01);
        nvp_i2c_write(nvp6124_slave_addr[i], 0x82, 0x12);
        nvp_i2c_write(nvp6124_slave_addr[i], 0x83, 0x2C);
        nvp_i2c_write(nvp6124_slave_addr[i], 0x3e, 0x10);
        nvp_i2c_write(nvp6124_slave_addr[i], 0x80, 0x60);
        nvp_i2c_write(nvp6124_slave_addr[i], 0x80, 0x61);
        msleep(100);
        nvp_i2c_write(nvp6124_slave_addr[i], 0x80, 0x40);
        nvp_i2c_write(nvp6124_slave_addr[i], 0x81, 0x02);
        nvp_i2c_write(nvp6124_slave_addr[i], 0x97, 0x00);
        msleep(10);
        nvp_i2c_write(nvp6124_slave_addr[i], 0x80, 0x60);
        nvp_i2c_write(nvp6124_slave_addr[i], 0x81, 0x00);
        msleep(10);
        nvp_i2c_write(nvp6124_slave_addr[i], 0x97, 0x0F);
        nvp_i2c_write(nvp6124_slave_addr[i], 0x38, 0x18);
        nvp_i2c_write(nvp6124_slave_addr[i], 0x38, 0x08);
        msleep(10);
        //printk("nvp6124B_system_init~~~~~~~~~~~~~~~~\n");
        msleep(100);
        nvp_i2c_write( nvp6124_slave_addr[i], 0xCA, 0xAE);
    }
    if(chip_id[0] == NVP6124_R0_ID)
        nvp6124_outport_1mux_chseq();
    else if(chip_id[0] == NVP6124B_R0_ID)
        nvp6124B_outport_1mux_chseq();
    else if(chip_id[0] == NVP6114A_R0_ID)
        nvp6114a_outport_1mux_chseq();
}

void nvp6124_system_init(void)
{
    int i = 0;
	for(i=0;i<nvp6124_cnt;i++)
	{
		nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x01);
		nvp_i2c_write(nvp6124_slave_addr[i], 0x82, 0x14);
		nvp_i2c_write(nvp6124_slave_addr[i], 0x83, 0x2C);
		nvp_i2c_write(nvp6124_slave_addr[i], 0x3e, 0x10);
		nvp_i2c_write(nvp6124_slave_addr[i], 0x80, 0x60);
		nvp_i2c_write(nvp6124_slave_addr[i], 0x80, 0x61);
		msleep(100);
		nvp_i2c_write(nvp6124_slave_addr[i], 0x80, 0x40);
		nvp_i2c_write(nvp6124_slave_addr[i], 0x81, 0x02);
		nvp_i2c_write(nvp6124_slave_addr[i], 0x97, 0x00);
		msleep(10);
		nvp_i2c_write(nvp6124_slave_addr[i], 0x80, 0x60);
		nvp_i2c_write(nvp6124_slave_addr[i], 0x81, 0x00);
		msleep(10);
		nvp_i2c_write(nvp6124_slave_addr[i], 0x97, 0x0F);
		nvp_i2c_write(nvp6124_slave_addr[i], 0x38, 0x18);
		nvp_i2c_write(nvp6124_slave_addr[i], 0x38, 0x08);
		msleep(10);
		//printk("nvp6124_system_init~~~~~~~~~~~~~~~~\n");
		msleep(100);
		nvp_i2c_write( nvp6124_slave_addr[i], 0xCA, 0xFF);
	}
	if(chip_id[0] == NVP6124_R0_ID)
		nvp6124_outport_1mux_chseq();
	else if(chip_id[0] == NVP6124B_R0_ID)
		nvp6124B_outport_1mux_chseq();
	else if(chip_id[0] == NVP6114A_R0_ID)
		nvp6114a_outport_1mux_chseq();
}

void software_reset(void)
{
	  int i = 0;
	for(i=0;i<nvp6124_cnt;i++)
	{
	    nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x01);
	    nvp_i2c_write(nvp6124_slave_addr[i], 0x80, 0x40);
	    nvp_i2c_write(nvp6124_slave_addr[i], 0x81, 0x02);
	    nvp_i2c_write(nvp6124_slave_addr[i], 0x80, 0x61);
	    nvp_i2c_write(nvp6124_slave_addr[i], 0x81, 0x00);
	    msleep(100);
	    nvp_i2c_write(nvp6124_slave_addr[i], 0x80, 0x60);
	    nvp_i2c_write(nvp6124_slave_addr[i], 0x81, 0x00);
	    msleep(100);
	}
	printk("\n\r nvp6124 software reset!!!");
}
void nvp6124B_ntsc_common_init(void)
{
    int i = 0;
    //nvp6124B_system_init();

    for(i=0;i<nvp6124_cnt;i++)
    {
        nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x00);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124B_B0_30P_Buf,254);
        nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x01);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124B_B1_30P_Buf,254);
        nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x02);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124B_B2_30P_Buf,254);
        nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x03);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124B_B3_30P_Buf,254);
        nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x04);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124B_B4_30P_Buf,254);
        nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x05);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124B_B5_30P_Buf,254);
        nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x06);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124B_B6_30P_Buf,254);
        nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x07);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124B_B7_30P_Buf,254);
        nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x08);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124B_B8_30P_Buf,254);
        nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x09);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124B_B9_30P_Buf,254);
        nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x0A);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124B_BA_30P_Buf,254);
        nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x0B);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124B_BB_30P_Buf,254);
    }

    nvp6124B_system_init();
}

void nvp6124_ntsc_common_init(void)
{
	int i = 0;
	for(i=0;i<nvp6124_cnt;i++)
	{
		nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x00);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B0_30P_Buf,254);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x01);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B1_30P_Buf,254);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x02);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B2_30P_Buf,254);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x03);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B3_30P_Buf,254);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x04);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B4_30P_Buf,254);
		//nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x05);
        //nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B5_30P_Buf,254);
		//nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x06);
        //nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B6_30P_Buf,254);
		//nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x07);
        //nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B7_30P_Buf,254);
		//nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x08);
        //nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B8_30P_Buf,254);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x09);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B9_30P_Buf,254);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x0A);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_BA_30P_Buf,254);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x0B);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_BB_30P_Buf,254);
	}

	nvp6124_system_init();
}

void nvp6124_pal_common_init(void)
{
	int i = 0;
	for(i=0;i<nvp6124_cnt;i++)
	{
		nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x00);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B0_25P_Buf,254);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x01);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B1_25P_Buf,254);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x02);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B2_25P_Buf,254);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x03);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B3_25P_Buf,254);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x04);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B4_25P_Buf,254);
		//nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x05);
        //nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B5_25P_Buf,254);
		//nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x06);
        //nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B6_25P_Buf,254);
		//nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x07);
        //nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B7_25P_Buf,254);
		//nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x08);
        //nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B8_25P_Buf,254);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x09);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B9_25P_Buf,254);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x0A);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_BA_25P_Buf,254);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x0B);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_BB_25P_Buf,254);
	}

	nvp6124_system_init();
}
void mpp2clk(unsigned char clktype)
{
	int i=0;
	for(i=0;i<nvp6124_cnt;i++)
	{
		nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x01);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xD4, 0x0F);  //mpp1,2,3,4 port clock func enables
		nvp_i2c_write(nvp6124_slave_addr[i], 0xB4, 0x66);  //clock&delay setting
		nvp_i2c_write(nvp6124_slave_addr[i], 0xB5, 0x66);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xB6, 0x66);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xB7, 0x66);
	}
}
/*??Ƶ??????ʽ???⺯??*/
void video_fmt_det(nvp6124_input_videofmt *pvideofmt)
{
	int i;
	unsigned char tmp;
	for(i=0; i<nvp6124_cnt*4; i++)
	{
		nvp_i2c_write(nvp6124_slave_addr[i/4], 0xFF, 0x00);
		pvideofmt->getvideofmt[i] = nvp_i2c_read(nvp6124_slave_addr[i/4], 0xD0+i%4);
		if(pvideofmt->getvideofmt[i] == 0x40 || pvideofmt->getvideofmt[i] == 0x10)
		{
			nvp_i2c_write(nvp6124_slave_addr[i/4], 0xFF, 0x00);
			nvp_i2c_write(nvp6124_slave_addr[i/4], 0x23+4*(i%4), 0x41);
			nvp_i2c_write(nvp6124_slave_addr[i/4], 0xFF, 0x05+i%4);
			nvp_i2c_write(nvp6124_slave_addr[i/4], 0x47, 0xee);
		}
		else
		{
			nvp_i2c_write(nvp6124_slave_addr[i/4], 0xFF, 0x00);
			nvp_i2c_write(nvp6124_slave_addr[i/4], 0x23+4*(i%4), 0x43);
			nvp_i2c_write(nvp6124_slave_addr[i/4], 0xFF, 0x05+i%4);
			nvp_i2c_write(nvp6124_slave_addr[i/4], 0x47, 0x04);
		}

		if(0x01 == pvideofmt->getvideofmt[i] || 0x02 == pvideofmt->getvideofmt[i])
		{
			nvp_i2c_write(nvp6124_slave_addr[i/4], 0xFF, 0x00);
			tmp = nvp_i2c_read(nvp6124_slave_addr[i/4], 0xE8+i%4);
			if(0x6A == tmp || 0x6E == tmp)
			{
				nvp_i2c_write(nvp6124_slave_addr[i/4], 0x21+4*(i%4), nvp6124_mode%2==PAL?0x22:0xA2);
				printk("CH[%d]nextchip comet input\n\n", i);
			}
			else
			{
				nvp_i2c_write(nvp6124_slave_addr[i/4], 0x21+4*(i%4), nvp6124_mode%2==PAL?0x02:0x82);
			}
		}
	}
}

unsigned int nvp6124_getvideoloss(void)
{
	unsigned int vloss=0, i;
	for(i=0;i<nvp6124_cnt;i++)
	{
		nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x00);
		vloss|=(nvp_i2c_read(nvp6124_slave_addr[i], 0xB8)&0x0F)<<(4*i);
	}
	return vloss;
}

//#define _EQ_ADJ_COLOR_
extern unsigned int vloss;
volatile unsigned char stage_update[16];

unsigned char ANALOG_EQ_1080P[8]  = {0x13,0x03,0x53,0x73,0x73,0x73,0x73,0x73};
unsigned char DIGITAL_EQ_1080P[8] = {0x00,0x00,0x00,0x00,0x8B,0x8F,0x8F,0x8F};
#ifdef _EQ_ADJ_COLOR_
unsigned char BRI_EQ_1080P[8]    = {0xF4,0xF4,0xF4,0xF4,0xF8,0xF8,0xF8,0xF8};
unsigned char CON_EQ_1080P[8]    = {0x90,0x90,0x90,0x90,0x90,0x90,0x80,0x80};
unsigned char SAT_EQ_1080P[8]    = {0x80,0x80,0x80,0x78,0x78,0x78,0x78,0x78};
unsigned char BRI_EQ_720P[9]    = {0xF4,0xF4,0xF4,0xF4,0xF8,0xF8,0xF8,0xF8,0xF8};
unsigned char CON_EQ_720P[9]    = {0x90,0x90,0x90,0x90,0x88,0x88,0x84,0x90,0x90};
unsigned char SAT_EQ_720P[9]    = {0x84,0x84,0x84,0x80,0x80,0x80,0x80,0x84,0x84};
#endif
unsigned char SHARP_EQ_1080P[8]  = {0x90,0x90,0x99,0x99,0x99,0x99,0x99,0x90};
unsigned char PEAK_EQ_1080P[8]   = {0x00,0x10,0x00,0x00,0x00,0x00,0x50,0x00};
unsigned char CTI_EQ_1080P[8]    = {0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A};
unsigned char C_LOCK_EQ_1080P[8] = {0x92,0x92,0x92,0x92,0x92,0xA2,0xA2,0xA2};
unsigned char UGAIN_EQ_1080P[8]  = {0x00,0x00,0x00,0x00,0x10,0x10,0x20,0x00};
unsigned char VGAIN_EQ_1080P[8]  = {0x00,0x00,0x00,0x00,0x10,0x10,0x20,0x00};


unsigned char SHARP_EQ_720P[9]   =  {0x90,0x90,0x99,0x99,0x99,0x99,0x99,0x90,0x90};
unsigned char PEAK_EQ_720P[9]    =  {0x00,0x20,0x10,0x10,0x00,0x00,0x40,0x20,0x20};
unsigned char CTI_EQ_720P[9]     =  {0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A};
unsigned char C_LOCK_EQ_720P[9]  =  {0x92,0x92,0x92,0x92,0x92,0x92,0xA2,0x92,0xA2};
unsigned char UGAIN_EQ_720P[9]   =  {0x30,0x30,0x30,0x30,0x30,0x30,0x40,0x30,0x30};
unsigned char VGAIN_EQ_720P[9]   =  {0x30,0x30,0x30,0x30,0x30,0x30,0x40,0x30,0x30};
unsigned char ANALOG_EQ_720P[9]  =  {0x13,0x03,0x53,0x73,0x73,0x73,0x73,0x03,0x13};
unsigned char DIGITAL_EQ_720P[9] =  {0x00,0x00,0x00,0x00,0x88,0x8F,0x8F,0x00,0x00};

unsigned char eq_stage[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#ifdef _EQ_ADJ_COLOR_
void nvp6124_brightness_eq(unsigned int ch,  unsigned int stage)
{
	nvp_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
    if(ch_mode_status[ch] == NVP6124_VI_720P_2530)
		nvp_i2c_write(nvp6124_slave_addr[ch/4],(0x0c+(ch%4)),BRI_EQ_720P[stage]);
	else if(ch_mode_status[ch] == NVP6124_VI_1080P_2530)
		nvp_i2c_write(nvp6124_slave_addr[ch/4],(0x0c+(ch%4)),BRI_EQ_1080P[stage]);
}

void nvp6124_contrast_eq(unsigned int ch,  unsigned int stage)
{
	nvp_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
    if(ch_mode_status[ch] == NVP6124_VI_720P_2530)
		nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x10+(ch%4)),CON_EQ_720P[stage]);
	else if(ch_mode_status[ch] == NVP6124_VI_1080P_2530)
		nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x10+(ch%4)),CON_EQ_1080P[stage]);
}

void nvp6124_saturation_eq(unsigned int ch,  unsigned int stage)
{
	nvp_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
    if(ch_mode_status[ch] == NVP6124_VI_720P_2530)
		nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x3C+(ch%4)),SAT_EQ_720P[stage]);
	else if(ch_mode_status[ch] == NVP6124_VI_1080P_2530)
		nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x3C+(ch%4)),SAT_EQ_1080P[stage]);
}
#endif
void nvp6124_c_filter_eq(unsigned int ch,  unsigned int stage)
{
	nvp_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
    if(ch_mode_status[ch] == NVP6124_VI_720P_2530)
		nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x21+4*(ch%4)),C_LOCK_EQ_720P[stage]);
	else if(ch_mode_status[ch] == NVP6124_VI_1080P_2530)
		nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x21+4*(ch%4)),C_LOCK_EQ_1080P[stage]);
}


void nvp6124_sharpness_eq(unsigned int ch,  unsigned int stage)
{
	nvp_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
    if(ch_mode_status[ch] == NVP6124_VI_720P_2530)
		nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x14+(ch%4)),SHARP_EQ_720P[stage]);
	else if(ch_mode_status[ch] == NVP6124_VI_1080P_2530)
		nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x14+(ch%4)),SHARP_EQ_1080P[stage]);
}

void nvp6124_peaking_eq(unsigned int ch,  unsigned int stage)
{
	nvp_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
    if(ch_mode_status[ch] == NVP6124_VI_720P_2530)
		nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x18+(ch%4)),PEAK_EQ_720P[stage]);
	else if(ch_mode_status[ch] == NVP6124_VI_1080P_2530)
		nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x18+(ch%4)),PEAK_EQ_1080P[stage]);
}

void nvp6124_ctigain_eq(unsigned int ch,  unsigned int stage)
{
	nvp_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
    if(ch_mode_status[ch] == NVP6124_VI_720P_2530)
		nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x38+(ch%4)),CTI_EQ_720P[stage]);
	else if(ch_mode_status[ch] == NVP6124_VI_1080P_2530)
		nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x38+(ch%4)),CTI_EQ_1080P[stage]);
}

void nvp6124_ugain_eq(unsigned int ch,  unsigned int stage)
{
	nvp_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
    if(ch_mode_status[ch] == NVP6124_VI_720P_2530)
		nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x44+(ch%4)),UGAIN_EQ_720P[stage]);
	else if(ch_mode_status[ch] == NVP6124_VI_1080P_2530)
		nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x44+(ch%4)),UGAIN_EQ_1080P[stage]);
}

void nvp6124_vgain_eq(unsigned int ch,  unsigned int stage)
{
	nvp_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
    if(ch_mode_status[ch] == NVP6124_VI_720P_2530)
		nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x48+(ch%4)),VGAIN_EQ_720P[stage]);
	else if(ch_mode_status[ch] == NVP6124_VI_1080P_2530)
		nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x48+(ch%4)),VGAIN_EQ_1080P[stage]);
}
unsigned int get_ceq_stage(unsigned char resol, unsigned int acc_gain)
{
	unsigned char c_eq = 0;

	if(resol == NVP6124_VI_1080P_2530)
	{
		if	   (acc_gain >= 0x000 && acc_gain < 0x052 )   c_eq = 1;
		else if(acc_gain >= 0x052 && acc_gain < 0x089 )   c_eq = 2;
		else if(acc_gain >= 0x089 && acc_gain < 0x113 )   c_eq = 3;
		else if(acc_gain >= 0x113 && acc_gain < 0x25F )   c_eq = 4;
		else if(acc_gain >= 0x25F && acc_gain < 0x700 )   c_eq = 5;
		else if(acc_gain >= 0x700 && acc_gain < 0x7FF )   c_eq = 6;
		else											  c_eq = 7;
	}
	else if(resol == NVP6124_VI_720P_2530)
	{
		if	   (acc_gain >= 0x000 && acc_gain < 0x055 )  c_eq = 1;
		else if(acc_gain >= 0x055 && acc_gain < 0x082 )  c_eq = 2;
		else if(acc_gain >= 0x082 && acc_gain < 0x0D8 )  c_eq = 3;
		else if(acc_gain >= 0x0D8 && acc_gain < 0x18F )  c_eq = 4;
		else if(acc_gain >= 0x18F && acc_gain < 0x700 )  c_eq = 5;
		else if(acc_gain >= 0x700 && acc_gain < 0x7FF )  c_eq = 6;
		else											 c_eq = 7;
	}

	return c_eq;
}

unsigned int get_yeq_stage(unsigned char resol, unsigned int y_minus_slp)
{
	unsigned char y_eq = 0;

	if(resol == NVP6124_VI_1080P_2530)
	{
		if     (y_minus_slp == 0x000)						    y_eq = 0;
		else if(y_minus_slp >  0x000 && y_minus_slp < 0x0E7)    y_eq = 1;
		else if(y_minus_slp >= 0x0E7 && y_minus_slp < 0x11A)    y_eq = 2;
		else if(y_minus_slp >= 0x11A && y_minus_slp < 0x151)    y_eq = 3;
		else if(y_minus_slp >= 0x151 && y_minus_slp < 0x181)    y_eq = 4;
		else if(y_minus_slp >= 0x181 && y_minus_slp < 0x200)    y_eq = 5;
		else													y_eq = 6;
	}
	else if(resol == NVP6124_VI_720P_2530)
	{
		if     (y_minus_slp == 0x000)                           y_eq = 0;
		else if(y_minus_slp >  0x000 && y_minus_slp < 0x104)    y_eq = 1;
		else if(y_minus_slp >= 0x104 && y_minus_slp < 0x125)    y_eq = 2;
		else if(y_minus_slp >= 0x125 && y_minus_slp < 0x14C)    y_eq = 3;
		else if(y_minus_slp >= 0x14C && y_minus_slp < 0x16F)    y_eq = 4;
		else if(y_minus_slp >= 0x16F && y_minus_slp < 0x185)    y_eq = 5;
		else													y_eq = 6;
	}

	return y_eq;

}

unsigned int is_bypass_mode(unsigned int c_stage, unsigned int c_status, unsigned int agc_val, unsigned int bw_on, unsigned int y_ref2_sts)
{

	if(((agc_val < 0x20)  && (y_ref2_sts >= 0x176))			||
	   ((agc_val >= 0x20) && (y_ref2_sts >= 0x1B0)))
			return 1;

	return 0;
}

volatile unsigned char c_same_cnt[16],y_same_cnt[16];
volatile unsigned char check_c_stage[16], check_c_stage_back[16];
volatile unsigned char check_y_stage[16], check_y_stage_back[16];
volatile unsigned char vidmode_back[16];
volatile unsigned char video_on[16];
void nvp6124_set_equalizer(void)
{
	unsigned char ch;
	unsigned char vidmode[16];
	unsigned char agc_lock;
	unsigned int  agc_val[16];
	unsigned int  acc_gain_sts[16];
	unsigned int  y_ref_status[16];
	unsigned int  y_ref2_status[16];
	unsigned char bw_on[16];
	unsigned char bypass_flag = 0;


	for(ch=0;ch<nvp6124_cnt*4;ch++)
	{
		nvp_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x09);
		nvp_i2c_write(nvp6124_slave_addr[ch/4], 0x61, ch);
		nvp_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
		agc_val[ch] = nvp_i2c_read(nvp6124_slave_addr[ch/4],0xF7);
		nvp_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
		vidmode[ch] = nvp_i2c_read(nvp6124_slave_addr[ch/4],0xD0+ch%4);

		agc_lock = nvp_i2c_read(nvp6124_slave_addr[ch/4],0xEC);
		bw_on[ch] = (nvp_i2c_read(nvp6124_slave_addr[ch/4],0xF3) >> (ch%4))&0x01;

		nvp_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x05 + ch%4);
		acc_gain_sts[ch] = nvp_i2c_read(nvp6124_slave_addr[ch/4],0xE2)&0x07;
		acc_gain_sts[ch] <<= 8;
		acc_gain_sts[ch] |= nvp_i2c_read(nvp6124_slave_addr[ch/4],0xE3);
		y_ref_status[ch] = nvp_i2c_read(nvp6124_slave_addr[ch/4],0xEA)&0x07;
		y_ref_status[ch] <<= 8;
		y_ref_status[ch] |= nvp_i2c_read(nvp6124_slave_addr[ch/4],0xEB);

		y_ref2_status[ch] = nvp_i2c_read(nvp6124_slave_addr[ch/4],0xE8)&0x07;
		y_ref2_status[ch] <<= 8;
		y_ref2_status[ch] |= nvp_i2c_read(nvp6124_slave_addr[ch/4],0xE9);

		if(vidmode[ch] >= 4)
		{
			if((((vloss>>ch)&0x01) == 0x00) && (((agc_lock>>(ch%4))&0x01) == 0x01))
				video_on[ch] = 1;
			else
				video_on[ch] = 0;
		}
		else
		{
			video_on[ch] = 0;
		}

		if((ch_mode_status[ch] == NVP6124_VI_1080P_2530) || (ch_mode_status[ch] == NVP6124_VI_720P_2530))
		{
			if(video_on[ch])
			{
				check_c_stage[ch] = get_ceq_stage(ch_mode_status[ch], acc_gain_sts[ch]);
				check_y_stage[ch] = get_yeq_stage(ch_mode_status[ch], y_ref_status[ch]);

				if(ch_mode_status[ch] == NVP6124_VI_720P_2530)
				{
					printk("y_ref_status = %x\n",y_ref_status[ch]);

					if(is_bypass_mode(check_c_stage[ch], acc_gain_sts[ch],agc_val[ch], bw_on[ch],y_ref2_status[ch]))
					{
						bypass_flag = 1;
						check_y_stage[ch] = 7;
					}
					else
					{
						bypass_flag = 0;
					}


					if(y_same_cnt[ch] != 0xFF)
					{
						if(check_c_stage[ch] == check_c_stage_back[ch])
						{
							c_same_cnt[ch]++;
							if(c_same_cnt[ch] > 200 ) c_same_cnt[ch] = 5;
						}
						else
						{
							c_same_cnt[ch] = 0;
						}

						if(check_y_stage[ch] == check_y_stage_back[ch])
						{
							y_same_cnt[ch]++;
							if(y_same_cnt[ch] > 200 ) y_same_cnt[ch] = 5;

						}
						else
						{
							if(abs(check_y_stage_back[ch] - check_y_stage[ch]) > 50)
							{
								if(y_same_cnt[ch] > 1)
								{
									bypass_flag = 1;
									check_y_stage[ch] = 7;
								}
							}
							y_same_cnt[ch] = 0;
						}

						if(bypass_flag == 0)
						{
							if(c_same_cnt[ch] != y_same_cnt[ch])
							{
								c_same_cnt[ch] = 0;
								y_same_cnt[ch] = 0;
							}
						}
					}

					if(bypass_flag)// && (y_same_cnt[ch]==4))
						stage_update[ch] = 1;
					else if((c_same_cnt[ch]==4) && (y_same_cnt[ch]==4))
						stage_update[ch] = 1;
					else
						stage_update[ch] = 0;
					printk("720p : %d %d %d %d\n",check_c_stage[ch],
							                      check_y_stage[ch],
												  c_same_cnt[ch],
												  y_same_cnt[ch]);
				}
				else
				{
					if(y_same_cnt[ch] != 0xFF)
					{
						if(check_y_stage[ch] == check_y_stage_back[ch])
						{
							y_same_cnt[ch]++;
							if(y_same_cnt[ch] > 200 ) y_same_cnt[ch] = 5;
						}
						else
						{
							y_same_cnt[ch]=0;
							if(abs(check_y_stage_back[ch] - check_y_stage[ch]) > 50)
							{
								if(y_same_cnt[ch] > 1)
								{
									bypass_flag = 1;
									check_y_stage[ch] = 7;
								}
							}
						}
					}

					if((y_same_cnt[ch]==4))
						stage_update[ch] = 1;
					else
						stage_update[ch] = 0;
					printk("y_same_cnt = %d check_y_stage = %x\n",y_same_cnt[ch],check_y_stage[ch]);
				}
			}
			else
			{
				if(vidmode_back[ch] >= 0x04)
					stage_update[ch] = 1;
				else
					stage_update[ch] = 0;

				c_same_cnt[ch]=0;
				y_same_cnt[ch]=0;
				check_c_stage[ch]=0;
				check_c_stage_back[ch]=0xFF;
				check_y_stage[ch]=0;
				check_y_stage_back[ch]=0xFF;
				eq_stage[ch] = 0;
		//		printk("Loss stage_update = %d\n",stage_update[ch]);
			}

			if(stage_update[ch])
			{
				if(video_on[ch]) y_same_cnt[ch] = 0xFF;
				eq_stage[ch] = check_y_stage[ch];

				if(ch_mode_status[ch] == NVP6124_VI_720P_2530)
				{
					nvp_i2c_write(nvp6124_slave_addr[ch/4], 0xFF,0x05+ch%4);
					nvp_i2c_write(nvp6124_slave_addr[ch/4], 0x58, ANALOG_EQ_720P[eq_stage[ch]]);
					nvp_i2c_write(nvp6124_slave_addr[ch/4], 0xFF,((ch%4)<2)?0x0A:0x0B);
					nvp_i2c_write(nvp6124_slave_addr[ch/4], (ch%2==0)?0x3B:0xBB, DIGITAL_EQ_720P[eq_stage[ch]]);
				}
				else
				{
					nvp_i2c_write(nvp6124_slave_addr[ch/4], 0xFF,0x05+ch%4);
					nvp_i2c_write(nvp6124_slave_addr[ch/4], 0x58, ANALOG_EQ_1080P[eq_stage[ch]]);
					nvp_i2c_write(nvp6124_slave_addr[ch/4], 0xFF,((ch%4)<2)?0x0A:0x0B);
					nvp_i2c_write(nvp6124_slave_addr[ch/4], (ch%2==0)?0x3B:0xBB, DIGITAL_EQ_1080P[eq_stage[ch]]);
				}
				#ifdef _EQ_ADJ_COLOR_
				nvp6124_brightness_eq(ch, eq_stage[ch]);
				nvp6124_contrast_eq(ch, eq_stage[ch]);
				nvp6124_saturation_eq(ch, eq_stage[ch]);
				#endif
				nvp6124_sharpness_eq(ch, eq_stage[ch]);
				nvp6124_peaking_eq(ch, eq_stage[ch]);
				nvp6124_ctigain_eq(ch, eq_stage[ch]);
				nvp6124_c_filter_eq(ch, eq_stage[ch]);
				nvp6124_ugain_eq(ch, eq_stage[ch]);
				nvp6124_vgain_eq(ch, eq_stage[ch]);
				printk("Stage update : eq_stage = %d\n",eq_stage[ch]);
			}

			check_c_stage_back[ch] = check_c_stage[ch];
			check_y_stage_back[ch] = check_y_stage[ch];
			vidmode_back[ch] = vidmode[ch];
		}
	}

}

/*
nvp6124 has 4 BT656 output ports.
nvp6114a only has 2, so ch_seq[2]&ch_seq[3] are invalid in nvp6114a.
*/
//unsigned char ch_seq[4]={2,1,0,3};
unsigned char ch_seq[4]={1,0,0xFF,0xFF};

void nvp6124B_outport_1mux_chseq(void)
{
    int i = 0;

    for(i=0;i<nvp6124_cnt;i++)
    {
        nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x01);
        nvp_i2c_write(nvp6124_slave_addr[i], 0xC0, ch_seq[0]<<4|ch_seq[1]);
        nvp_i2c_write(nvp6124_slave_addr[i], 0xC1, ch_seq[2]<<4|ch_seq[3]);
        nvp_i2c_write(nvp6124_slave_addr[i], 0xC2, ch_seq[0]<<4|ch_seq[1]);
        nvp_i2c_write(nvp6124_slave_addr[i], 0xC3, ch_seq[2]<<4|ch_seq[3]);
        nvp_i2c_write(nvp6124_slave_addr[i], 0xC8, 0x00);
        nvp_i2c_write(nvp6124_slave_addr[i], 0xC9, 0x00);
        nvp_i2c_write(nvp6124_slave_addr[i], 0xCA, 0xFF);
    }
}

void nvp6124_outport_1mux_chseq(void)
{
	int i = 0;

	for(i=0;i<nvp6124_cnt;i++)
	{
		nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x01);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xC0, ch_seq[0]<<4|ch_seq[0]);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xC1, ch_seq[0]<<4|ch_seq[0]);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xC2, ch_seq[1]<<4|ch_seq[1]);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xC3, ch_seq[1]<<4|ch_seq[1]);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xC4, ch_seq[2]<<4|ch_seq[2]);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xC5, ch_seq[2]<<4|ch_seq[2]);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xC6, ch_seq[3]<<4|ch_seq[3]);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xC7, ch_seq[3]<<4|ch_seq[3]);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xC8, 0x00);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xC9, 0x00);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xCA, 0xFF);
	}
}
void nvp6124B_outport_1mux(unsigned char vformat, unsigned char port1_mode, unsigned char port2_mode )
{
    int ch, i;
	unsigned char tmp=0;
	unsigned char p1_num,p2_num,port1_vimode,port2_vimode;
	nvp6124_video_mode vmode;


	p1_num = port1_mode>>0x04;
	p2_num = port2_mode>>0x04;

	port1_vimode = port1_mode&0x0F;
	port2_vimode = port2_mode&0x0F;

	for(ch=0;ch<nvp6124_cnt*4;ch++)
  	{
  		vmode.vformat[0] = vformat;
		if(ch%4 < 2)
			vmode.chmode[ch] = port1_vimode;
		else
			vmode.chmode[ch] = port2_vimode;
	}
	//nvp6124_each_mode_setting(&vmode);


	for(i=0;i<nvp6124_cnt;i++)
	{
		//nvp_i2c_write( nvp6124_slave_addr[i], 0xFF, 0x01);
		//nvp_i2c_write( nvp6124_slave_addr[i], 0xC0+p1_num*2, 0x10);
		//nvp_i2c_write( nvp6124_slave_addr[i], 0xC1+p1_num*2, 0x10);
		//nvp_i2c_write( nvp6124_slave_addr[i], 0xC0+p2_num*2, 0x32);
		//nvp_i2c_write( nvp6124_slave_addr[i], 0xC1+p2_num*2, 0x32);

        switch (port1_vimode) {
            case NVP6124_VI_1080P_2530:
                tmp = 1<<4;
                break;
            case NVP6124_VI_720P_2530:
                tmp = 0x00;
                break;
            default:
                tmp = 2<<4;
        }
		//nvp_i2c_write( nvp6124_slave_addr[i], 0xC8, tmp);
		//nvp_i2c_write( nvp6124_slave_addr[i], 0xC9, tmp);

		//nvp_i2c_write( nvp6124_slave_addr[i], 0xCA, 0xFF);

        tmp = 0;
        switch (port1_vimode) {
            case NVP6124_VI_1080P_2530:
                tmp = 0x66;
                break;
            case NVP6124_VI_720P_2530:
                tmp = 0x04;
                break;
            case NVP6124_VI_SD:
                tmp = 0x46;
                break;
            default:
                tmp = 0x66;
        }

        nvp_i2c_write( nvp6124_slave_addr[i], 0xCD+p1_num*2, tmp);
        nvp_i2c_write( nvp6124_slave_addr[i], 0xCD+p2_num*2, tmp);
  	}

}

/*
vformat:0->NTSC, 1->PAL
portx_mode:
??4bitѡ??port?ӿ?[7:4]->0~3:port0~3;
??4bitѡ??viģʽ[3:0]-> (NVP6124_VI_SD,NVP6124_VI_720P_2530, NVP6124_VI_1080P_2530)
*/
void nvp6124_outport_2mux(unsigned char vformat, unsigned char port1_mode, unsigned char port2_mode )
{
	int ch, i, tmp=0;
	unsigned char p1_num,p2_num,port1_vimode,port2_vimode;
	nvp6124_video_mode vmode;

	p1_num = port1_mode>>0x04;
	p2_num = port2_mode>>0x04;
	port1_vimode = port1_mode&0x0F;
	port2_vimode = port2_mode&0x0F;
	for(ch=0;ch<nvp6124_cnt*4;ch++)
  	{
  		vmode.vformat[0] = vformat;
		if(ch%4 < 2)
			vmode.chmode[ch] = port1_vimode;
		else
			vmode.chmode[ch] = port2_vimode;
	}
	nvp6124_each_mode_setting(&vmode);
	for(i=0;i<nvp6124_cnt;i++)
	{
		nvp_i2c_write( nvp6124_slave_addr[i], 0xFF, 0x01);
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC0+p1_num*2, 0x10);
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC1+p1_num*2, 0x10);
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC0+p2_num*2, 0x32);
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC1+p2_num*2, 0x32);
		tmp = (port2_vimode==NVP6124_VI_1080P_2530?1:2)<<(p2_num*4)|((port1_vimode==NVP6124_VI_1080P_2530?1:2)<<(p1_num*4));
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC8, tmp&0xFF);
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC9, (tmp>>8)&0xFF);

		tmp=((1<<p1_num)|(1<<p2_num));
		nvp_i2c_write( nvp6124_slave_addr[i], 0xCA, (tmp<<4|tmp));    //?򿪶˿?[3:0]??ʱ??[7:4],??Ӳ?????ء?

		if(port1_vimode == NVP6124_VI_SD)
			nvp_i2c_write( nvp6124_slave_addr[i], 0xCC+p1_num, 0x46);    //ʱ??Ƶ??????
		else
			nvp_i2c_write( nvp6124_slave_addr[i], 0xCC+p1_num, 0x66);
		if(port2_vimode == NVP6124_VI_SD)
			nvp_i2c_write( nvp6124_slave_addr[i], 0xCC+p2_num, 0x46);    //ʱ??Ƶ??????
		else
			nvp_i2c_write( nvp6124_slave_addr[i], 0xCC+p2_num, 0x66);
  	}
	//printk("nvp6124_outport_2mux setting\n");
}

/*
vformat:0->NTSC, 1->PAL
port1_mode:
??4bitѡ??port?ӿ?[7:4]->0~3:port0~3;
??4bitѡ??viģʽ[3:0]-> (NVP6124_VI_SD,NVP6124_VI_720P_2530)
*/
void nvp6124_outport_4mux(unsigned char vformat, unsigned char port1_mode )
{
	int ch, i, tmp=0;
	unsigned char p1_num,port1_vimode;
	nvp6124_video_mode vmode;

	p1_num = port1_mode>>0x04;
	port1_vimode = port1_mode&0x0F;
	for(ch=0;ch<nvp6124_cnt*4;ch++)
  	{
  		vmode.vformat[0] = vformat;
		vmode.chmode[ch] = port1_vimode;
	}
	nvp6124_each_mode_setting(&vmode);
	for(i=0;i<nvp6124_cnt;i++)
	{
		nvp_i2c_write( nvp6124_slave_addr[i], 0xFF, 0x00);
		nvp_i2c_write( nvp6124_slave_addr[i], 0x55, 0x10);
		nvp_i2c_write( nvp6124_slave_addr[i], 0x56, 0x32);  //reset channel id
		nvp_i2c_write( nvp6124_slave_addr[i], 0xFF, 0x01);
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC0+p1_num*2, 0x10);
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC1+p1_num*2, 0x32);
		tmp = ((NVP6124_VI_720P_2530?3:8)<<(p1_num*4));
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC8, tmp&0xFF);
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC9, (tmp>>8)&0xFF);

		tmp=(1<<p1_num);
		nvp_i2c_write( nvp6124_slave_addr[i], 0xCA, (tmp<<4|tmp));    //?򿪶˿?[3:0]??ʱ??[7:4],??Ӳ?????ء?

		nvp_i2c_write( nvp6124_slave_addr[i], 0xCC+p1_num, 0x66);    //ʱ??Ƶ??????
  	}
}

void nvp6114a_outport_1mux_chseq(void)
{
	int i = 0;
	ch_seq[0] = 1;   // port A outputs channel0's video data
	ch_seq[1] = 0;   // port B outputs channel1's video data
	ch_seq[2] = 0xFF;
	ch_seq[3] = 0xFF;
	for(i=0;i<nvp6124_cnt;i++)
	{
		nvp_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x01);
		//nvp_i2c_write(nvp6124_slave_addr[i], 0xC0, ch_seq[0]<<4|ch_seq[0]);
		//nvp_i2c_write(nvp6124_slave_addr[i], 0xC1, ch_seq[0]<<4|ch_seq[0]);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xC2, ch_seq[0]<<4|ch_seq[0]);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xC3, ch_seq[0]<<4|ch_seq[0]);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xC4, ch_seq[1]<<4|ch_seq[1]);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xC5, ch_seq[1]<<4|ch_seq[1]);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xC6, ch_seq[1]<<4|ch_seq[1]);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xC7, ch_seq[1]<<4|ch_seq[1]);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xC8, 0x00);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xC9, 0x00);
		nvp_i2c_write(nvp6124_slave_addr[i], 0xCA, 0xFF);
	}
}

/*
vformat:0->NTSC, 1->PAL
portx_mode:
??4bitѡ??port?ӿ?[7:4]->0~1:port0~1;
??4bitѡ??viģʽ[3:0]-> (NVP6124_VI_SD,NVP6124_VI_720P_2530, NVP6124_VI_1080P_2530)
*/
void nvp6114a_outport_1mux(unsigned char vformat, unsigned char port1_mode, unsigned char port2_mode )
{
    int ch, i;
	unsigned char tmp=0;
	unsigned char p1_num,p2_num,port1_vimode,port2_vimode;
	nvp6124_video_mode vmode;

	p1_num = port1_mode>>0x04;
	p2_num = port2_mode>>0x04;

	port1_vimode = port1_mode&0x0F;
	port2_vimode = port2_mode&0x0F;

	for(ch=0;ch<nvp6124_cnt*4;ch++)
  	{
  		vmode.vformat[0] = vformat;
		if(ch%4 < 2)
			vmode.chmode[ch] = port1_vimode;
		else
			vmode.chmode[ch] = port2_vimode;
	}
	nvp6124_each_mode_setting(&vmode);

	for(i=0;i<nvp6124_cnt;i++)
	{
		nvp_i2c_write( nvp6124_slave_addr[i], 0xFF, 0x01);
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC2+p1_num*2, 0x10);
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC3+p1_num*2, 0x10);
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC2+p2_num*2, 0x32);
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC3+p2_num*2, 0x32);
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC6, nvp_i2c_read( nvp6124_slave_addr[i], 0xC4));
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC7, nvp_i2c_read( nvp6124_slave_addr[i], 0xC5));

        switch (port1_vimode) {
            case NVP6124_VI_1080P_2530:
                tmp = 1<<4;
                break;
            case NVP6124_VI_720P_2530:
                tmp = 0x00;
                break;
            default:
                tmp = 2<<4;
        }
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC8, tmp);

		nvp_i2c_write( nvp6124_slave_addr[i], 0xCA, 0xFF);

        tmp = 0;
        switch (port1_vimode) {
            case NVP6124_VI_1080P_2530:
                tmp = 0x66;
                break;
            case NVP6124_VI_720P_2530:
                tmp = 0x45;
                break;
            case NVP6124_VI_SD:
                tmp = 0x46;
                break;
            default:
                tmp = 0x66;
        }

        nvp_i2c_write( nvp6124_slave_addr[i], 0xCD+p1_num*2, tmp);
        nvp_i2c_write( nvp6124_slave_addr[i], 0xCD+p2_num*2, tmp);
  	}
}

/*
vformat:0->NTSC, 1->PAL
portx_mode:
??4bitѡ??port?ӿ?[7:4]->0~1:port0~1;
??4bitѡ??viģʽ[3:0]-> (NVP6124_VI_SD,NVP6124_VI_720P_2530, NVP6124_VI_1080P_2530)
*/
void nvp6114a_outport_2mux(unsigned char vformat, unsigned char port1_mode, unsigned char port2_mode )
{
int ch, i;
	unsigned char tmp=0;
	unsigned char p1_num,p2_num,port1_vimode,port2_vimode;
	nvp6124_video_mode vmode;

	p1_num = port1_mode>>0x04;
	p2_num = port2_mode>>0x04;

	port1_vimode = port1_mode&0x0F;
	port2_vimode = port2_mode&0x0F;

	for(ch=0;ch<nvp6124_cnt*4;ch++)
  	{
  		vmode.vformat[0] = vformat;
		if(ch%4 < 2)
			vmode.chmode[ch] = port1_vimode;
		else
			vmode.chmode[ch] = port2_vimode;
	}
	nvp6124_each_mode_setting(&vmode);

	for(i=0;i<nvp6124_cnt;i++)
	{
		nvp_i2c_write( nvp6124_slave_addr[i], 0xFF, 0x01);
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC2+p1_num*2, 0x10);
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC3+p1_num*2, 0x10);
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC2+p2_num*2, 0x32);
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC3+p2_num*2, 0x32);
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC6, nvp_i2c_read( nvp6124_slave_addr[i], 0xC4));
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC7, nvp_i2c_read( nvp6124_slave_addr[i], 0xC5));
		tmp = ((port1_vimode==NVP6124_VI_1080P_2530?1:2)<<4);
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC8, tmp);
		tmp = ((port2_vimode==NVP6124_VI_1080P_2530?1:2)<<4)|((port2_vimode==NVP6124_VI_1080P_2530?1:2));
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC9, tmp);

		//tmp=((1<<p1_num)|(1<<p2_num));
		nvp_i2c_write( nvp6124_slave_addr[i], 0xCA, 0xFF);    //?򿪶˿?[3:0]??ʱ??[7:4],??Ӳ?????ء?

		if(port1_vimode == NVP6124_VI_SD)
			nvp_i2c_write( nvp6124_slave_addr[i], 0xCD+p1_num*2, 0x46);    //ʱ??Ƶ??????
		else
			nvp_i2c_write( nvp6124_slave_addr[i], 0xCD+p1_num*2, 0x66);
		if(port2_vimode == NVP6124_VI_SD)
			nvp_i2c_write( nvp6124_slave_addr[i], 0xCD+p2_num*2, 0x46);    //ʱ??Ƶ??????
		else
			nvp_i2c_write( nvp6124_slave_addr[i], 0xCD+p2_num*2, 0x66);
  	}
}


/*
vformat:0->NTSC, 1->PAL
port1_mode:
??4bitѡ??port?ӿ?[7:4]->0~1:port0~1;
??4bitѡ??viģʽ[3:0]-> (NVP6124_VI_SD,NVP6124_VI_720P_2530)
*/
void nvp6114a_outport_4mux(unsigned char vformat, unsigned char port1_mode)
{
	int ch, i;
	unsigned char tmp=0;
	unsigned char p1_num,port1_vimode;
	nvp6124_video_mode vmode;

	p1_num = port1_mode>>0x04;
	port1_vimode = port1_mode&0x0F;
	for(ch=0;ch<nvp6124_cnt*4;ch++)
  	{
  		vmode.vformat[0] = vformat;
		vmode.chmode[ch] = port1_vimode;
	}
	nvp6124_each_mode_setting(&vmode);
	for(i=0;i<nvp6124_cnt;i++)
	{
		nvp_i2c_write( nvp6124_slave_addr[i], 0xFF, 0x00);
		nvp_i2c_write( nvp6124_slave_addr[i], 0x55, 0x10);
		nvp_i2c_write( nvp6124_slave_addr[i], 0x56, 0x32);
		nvp_i2c_write( nvp6124_slave_addr[i], 0xFF, 0x01);
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC2+p1_num*2, 0x10);
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC3+p1_num*2, 0x32);

		nvp_i2c_write( nvp6124_slave_addr[i], 0xC6, nvp_i2c_read( nvp6124_slave_addr[i], 0xC4));
		nvp_i2c_write( nvp6124_slave_addr[i], 0xC7, nvp_i2c_read( nvp6124_slave_addr[i], 0xC5));
		tmp = ((port1_vimode==NVP6124_VI_720P_2530?3:8)<<4)|((port1_vimode==NVP6124_VI_720P_2530?3:8));
		if(p1_num == 0)
		{
			nvp_i2c_write( nvp6124_slave_addr[i], 0xC8, tmp);
			nvp_i2c_write( nvp6124_slave_addr[i], 0xCA, 0x22);
		}
		else
		{
			nvp_i2c_write( nvp6124_slave_addr[i], 0xC9, tmp);
			nvp_i2c_write( nvp6124_slave_addr[i], 0xCA, 0x8C);
		}
		nvp_i2c_write( nvp6124_slave_addr[i], 0xCD+p1_num*2, 0x66);    //ʱ??Ƶ??????
  	}
}


void nvp6124_each_mode_setting(nvp6124_video_mode *pvmode )
{
	int i;
	unsigned char tmp;
	unsigned char ch, chmode[16];
	unsigned char pn_value_sd_nt_comet[4] = {0x4D,0x0E,0x88,0x6C};
	unsigned char pn_value_720p_30[4] = 	{0xEE,0x00,0xE5,0x4E};
	unsigned char pn_value_720p_60[4] = 	{0x78,0x6E,0x7C,0x27};
	unsigned char pn_value_fhd_nt[4] = 		{0x2C,0xF0,0xCA,0x52};
	unsigned char pn_value_sd_pal_comet[4] = {0x75,0x35,0xB4,0x6C};
	unsigned char pn_value_720p_25[4] = 	{0x46,0x08,0x10,0x4F};
	unsigned char pn_value_720p_50[4] = 	{0xCD,0xBD,0x7F,0x27};
	unsigned char pn_value_fhd_pal[4] = 	{0xC8,0x7D,0xC3,0x52};
	unsigned char vformat = pvmode->vformat[0];



  	for(ch=0;ch<(nvp6124_cnt*4);ch++)
  	{
		chmode[ch] = pvmode->chmode[ch];
  	}
	for(i=0;i<nvp6124_cnt;i++)
  	{
		nvp_i2c_write( nvp6124_slave_addr[i], 0xFF, 0);
		nvp_i2c_write( nvp6124_slave_addr[i], 0x80, 0x0f);
		nvp_i2c_write( nvp6124_slave_addr[i], 0xFF, 1);
		nvp_i2c_write( nvp6124_slave_addr[i], 0x93, 0x80);
  	}
	//printk("\n\nchmode[0] %d chmode[1] %d chmode[2] %d chmode[3] %d \n\n", chmode[0],chmode[1],chmode[2],chmode[3]);

	for(ch=0;ch<(nvp6124_cnt*4);ch++)
	{
		//if((chmode[ch] != ch_mode_status[ch]) && (chmode[ch] < NVP6124_VI_BUTT))
		if(chmode[ch] < NVP6124_VI_BUTT)
		{
			switch(chmode[ch])
			{
				case NVP6124_VI_SD:
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xFF, 0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x08+ch%4, vformat==PAL?0xDD:0xA0);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x0c+ch%4, nvp6124_bri_tbl_960H[vformat%2]);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x10+ch%4, nvp6124_con_tbl_960H[vformat%2]);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x14+ch%4, vformat==PAL?0x80:0x80);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x18+ch%4, vformat==PAL?0x18:0x18);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x21+4*(ch%4), vformat==PAL?0x02:0x82);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x23+4*(ch%4), 0x43);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x30+ch%4, vformat==PAL?0x12:0x11);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x3c+ch%4, nvp6124_sat_tbl_960H[vformat%2]);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x40+ch%4, nvp6124_hue_tbl_960H[vformat%2]);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x44+ch%4, vformat==PAL?0x00:0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x48+ch%4, vformat==PAL?0x00:0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x4c+ch%4, vformat==PAL?0x04:0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x50+ch%4, vformat==PAL?0x04:0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x58+ch%4, vformat==PAL?0x80:0x90);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x5c+ch%4, vformat==PAL?0x1e:0x1e);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x64+ch%4, vformat==PAL?0x0d:0x08);
	 				nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x81+ch%4, vformat==PAL?0x00:0x00);
	 				nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x85+ch%4, vformat==PAL?0x11:0x11);
	 				nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x89+ch%4, vformat==PAL?0x10:0x00);
	 				nvp_i2c_write( nvp6124_slave_addr[ch/4], ch%4+0x8E, vformat==PAL?0x08:0x07);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x93+ch%4, 0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x98+ch%4, vformat==PAL?0x07:0x04);
	 				nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xa0+ch%4, vformat==PAL?0x00:0x10);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xa4+ch%4, vformat==PAL?0x00:0x01);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xFF, 0x01);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x88+ch%4, vformat==PAL?0x7e:0x7e);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x8c+ch%4, vformat==PAL?0x26:0x26);
					if(chip_id[0] == NVP6124_R0_ID)
						nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xcc+ch_seq[ch%4], 0x36);
					else if(chip_id[0] == NVP6124B_R0_ID)
						nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xcc+ch_seq[ch%4], 0x36);
					else if(chip_id[0] == NVP6114A_R0_ID && ch_seq[ch]!=0xFF)
						nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xcd+(ch_seq[ch%4]%2)*2, 0x36);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xd7, nvp_i2c_read( nvp6124_slave_addr[ch/4], 0xd7)&(~(1<<(ch%4))));
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xFF, 0x02);  //motion
					tmp = nvp_i2c_read( nvp6124_slave_addr[ch/4], 0x16+(ch%4)/2);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x16+(ch%4)/2, (tmp&(ch%2==0?0xF0:0x0F))|(0x00<<((ch%2)*4)));
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xFF, 0x05+ch%4);
					nvp6124_write_table( nvp6124_slave_addr[ch/4], 0x00, vformat==PAL?NVP6124_B5_PAL_Buf:NVP6124_B5_NTSC_Buf, 254 );
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x06,0x40);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x25,vformat==PAL?0xCA:0xDA);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xFF, 0x09);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x40+(ch%4), 0x60);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x44, nvp_i2c_read( nvp6124_slave_addr[ch/4], 0x44)&(~(1<<(ch%4))));
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x50+4*(ch%4),vformat==PAL?pn_value_sd_pal_comet[0]:pn_value_sd_nt_comet[0]);	//ch%41 960H
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x51+4*(ch%4),vformat==PAL?pn_value_sd_pal_comet[1]:pn_value_sd_nt_comet[1]);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x52+4*(ch%4),vformat==PAL?pn_value_sd_pal_comet[2]:pn_value_sd_nt_comet[2]);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x53+4*(ch%4),vformat==PAL?pn_value_sd_pal_comet[3]:pn_value_sd_nt_comet[3]);
					//printk("ch %d setted to SD %s\n", ch, vformat==PAL?"PAL":"NTSC");
				break;
				case NVP6124_VI_720P_2530:
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xFF, 0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x08+ch%4,vformat==PAL?0x60:0x60);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x0c+ch%4,nvp6124_bri_tbl_720P[vformat%2]);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x10+ch%4,nvp6124_con_tbl_720P[vformat%2]);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x14+ch%4,vformat==PAL?0x90:0x90);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x18+ch%4,vformat==PAL?0x30:0x30);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x21+4*(ch%4), 0x92);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x22+4*(ch%4), 0x0A);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x23+4*(ch%4), 0x43);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x30+ch%4,vformat==PAL?0x12:0x12);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x3c+ch%4,nvp6124_sat_tbl_720P[vformat%2]);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x40+ch%4,nvp6124_hue_tbl_720P[vformat%2]);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x44+ch%4,vformat==PAL?0x30:0x30);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x48+ch%4,vformat==PAL?0x30:0x30);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x4c+ch%4,vformat==PAL?0x04:0x04);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x50+ch%4,vformat==PAL?0x04:0x04);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x58+ch%4,vformat==PAL?0x80:0x90);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x5c+ch%4,vformat==PAL?0x9e:0x9e);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x64+ch%4,vformat==PAL?0xb1:0xb2);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x81+ch%4,vformat==PAL?0x07:0x06);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x85+ch%4,vformat==PAL?0x00:0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x89+ch%4,vformat==PAL?0x10:0x10);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], ch%4+0x8E,vformat==PAL?0x0d:0x0d);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x93+ch%4,0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x98+ch%4,vformat==PAL?0x07:0x04);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xa0+ch%4,vformat==PAL?0x00:0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xa4+ch%4,vformat==PAL?0x00:0x01);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xFF,0x01);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x88+ch%4,vformat==PAL?0x5C:0x5C);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x8c+ch%4,vformat==PAL?0x40:0x40);
					//nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xcc+ch_seq[ch%4],vformat==PAL?0x46:0x46);
					if(chip_id[0] == NVP6124_R0_ID)
						nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xcc+ch_seq[ch%4], 0x46);
					else if(chip_id[0] == NVP6124B_R0_ID)
						nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xcc+ch_seq[ch%4], 0x00);
					else if(chip_id[0] == NVP6114A_R0_ID && ch_seq[ch]!=0xFF)
						nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xcd+(ch_seq[ch%4]%2)*2, 0x46);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xd7,nvp_i2c_read( nvp6124_slave_addr[ch/4], 0xd7)|(1<<(ch%4)));
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xFF, 0x02);  //motion
					tmp = nvp_i2c_read( nvp6124_slave_addr[ch/4], 0x16+(ch%4)/2);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x16+(ch%4)/2, (tmp&(ch%2==0?0xF0:0x0F))|(0x05<<((ch%2)*4)));
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xFF, 0x05+ch%4);
					nvp6124_write_table( nvp6124_slave_addr[ch/4], 0x00, vformat==PAL?NVP6124_B5_25P_Buf:NVP6124_B5_30P_Buf,254 );
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x01,0x0D);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x06,0x40);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x2B,0x78);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x59,0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x58,0x13);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xC0,0x16);
					//eq_init for 720P_2530
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xD8,0x0C);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xD9,0x0E);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xDA,0x12);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xDB,0x14);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xDC,0x1C);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xDD,0x2C);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xDE,0x34);

					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xFF, 0x09);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x40+(ch%4),0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x44, nvp_i2c_read( nvp6124_slave_addr[ch/4], 0x44)|(1<<(ch%4)));
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x50+4*(ch%4),vformat==PAL?pn_value_720p_25[0]:pn_value_720p_30[0]);	//ch%41 960H
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x51+4*(ch%4),vformat==PAL?pn_value_720p_25[1]:pn_value_720p_30[1]);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x52+4*(ch%4),vformat==PAL?pn_value_720p_25[2]:pn_value_720p_30[2]);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x53+4*(ch%4),vformat==PAL?pn_value_720p_25[3]:pn_value_720p_30[3]);
					//printk("ch %d setted to 720P %s\n", ch, vformat==PAL?"PAL":"NTSC");
				break;
				case NVP6124_VI_720P_5060:
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xFF, 0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x08+ch%4,vformat==PAL?0x60:0x60);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x0c+ch%4,nvp6124_bri_tbl_720P[vformat%2]);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x10+ch%4,nvp6124_con_tbl_720P[vformat%2]);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x14+ch%4,vformat==PAL?0x90:0x90);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x18+ch%4,vformat==PAL?0x00:0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x21+4*(ch%4), 0x92);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x22+4*(ch%4), 0x0A);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x23+4*(ch%4), vformat==PAL?0x43:0x43);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x30+ch%4,vformat==PAL?0x12:0x12);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x3c+ch%4,nvp6124_sat_tbl_720P[vformat%2]);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x40+ch%4,nvp6124_hue_tbl_720P[vformat%2]);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x44+ch%4,vformat==PAL?0x30:0x30);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x48+ch%4,vformat==PAL?0x30:0x30);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x4c+ch%4,vformat==PAL?0x04:0x04);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x50+ch%4,vformat==PAL?0x04:0x04);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x58+ch%4,vformat==PAL?0xc0:0xb0);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x5c+ch%4,vformat==PAL?0x9e:0x9e);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x64+ch%4,vformat==PAL?0xb1:0xb2);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x81+ch%4,vformat==PAL?0x05:0x04);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x85+ch%4,vformat==PAL?0x00:0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x89+ch%4,vformat==PAL?0x10:0x10);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], ch%4+0x8E,vformat==PAL?0x0b:0x09);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x93+ch%4,0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x98+ch%4,vformat==PAL?0x07:0x04);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xa0+ch%4,vformat==PAL?0x00:0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xa4+ch%4,vformat==PAL?0x00:0x01);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xFF, 0x01);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x88+ch%4,vformat==PAL?0x4d:0x4d);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x8c+ch%4,vformat==PAL?0x84:0x84);
					//nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xcc+ch_seq[ch%4],vformat==PAL?0x66:0x66);
					if(chip_id[0] == NVP6124_R0_ID)
						nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xcc+ch_seq[ch%4], 0x66);
					else if(chip_id[0] == NVP6124B_R0_ID)
						nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xcc+ch_seq[ch%4], 0x66);
					else if(chip_id[0] == NVP6114A_R0_ID && ch_seq[ch]!=0xFF)
						nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xcd+(ch_seq[ch%4]%2)*2, 0x66);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xd7, nvp_i2c_read( nvp6124_slave_addr[ch/4], 0xd7)|(1<<(ch%4)));
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xFF, 0x02);  //motion
					tmp = nvp_i2c_read( nvp6124_slave_addr[ch/4], 0x16+(ch%4)/2);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x16+(ch%4)/2, (tmp&(ch%2==0?0xF0:0x0F))|(0x05<<((ch%2)*4)));
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xFF, 0x05+ch%4);
					nvp6124_write_table( nvp6124_slave_addr[ch/4], 0x00, vformat==PAL?NVP6124_B5_25P_Buf:NVP6124_B5_30P_Buf,254 );
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x01,0x0C);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x06,0x40);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x2B,0x78);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x59,0x01);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x24,vformat==PAL?0x2A:0x1A);  //sync changed
					//nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x47,vformat==PAL?0x04:0xEE);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x50,vformat==PAL?0x84:0x86);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xBB,vformat==PAL?0x00:0xE4);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xFF,0x09);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x40+(ch%4), 0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x44, nvp_i2c_read( nvp6124_slave_addr[ch/4], 0x44)|(1<<(ch%4)));
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x50+4*(ch%4),vformat==PAL?pn_value_720p_50[0]:pn_value_720p_60[0]);	//ch%41 960H
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x51+4*(ch%4),vformat==PAL?pn_value_720p_50[1]:pn_value_720p_60[1]);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x52+4*(ch%4),vformat==PAL?pn_value_720p_50[2]:pn_value_720p_60[2]);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x53+4*(ch%4),vformat==PAL?pn_value_720p_50[3]:pn_value_720p_60[3]);
					//printk("ch %d setted to 720P@RT %s\n", ch, vformat==PAL?"PAL":"NTSC");
				break;
				case NVP6124_VI_1080P_2530:
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xFF, 0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x08+ch%4,vformat==PAL?0x60:0x60);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x0c+ch%4,nvp6124_bri_tbl[vformat%2]);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x10+ch%4,nvp6124_con_tbl[vformat%2]);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x14+ch%4,vformat==PAL?0x90:0x90);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x18+ch%4,vformat==PAL?0x00:0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x21+4*(ch%4), 0x92);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x22+4*(ch%4), 0x0A);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x23+4*(ch%4), vformat==PAL?0x43:0x43);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x30+ch%4,vformat==PAL?0x12:0x12);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x3c+ch%4,nvp6124_sat_tbl[vformat%2]);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x40+ch%4,nvp6124_hue_tbl[vformat%2]);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x44+ch%4,vformat==PAL?0x00:0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x48+ch%4,vformat==PAL?0x00:0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x4c+ch%4,vformat==PAL?0x00:0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x50+ch%4,vformat==PAL?0x00:0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x58+ch%4,vformat==PAL?0x6a:0x49);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x5c+ch%4,vformat==PAL?0x9e:0x9e);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x64+ch%4,vformat==PAL?0xbf:0x8d);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x81+ch%4,vformat==PAL?0x03:0x02);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x85+ch%4,vformat==PAL?0x00:0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x89+ch%4,vformat==PAL?0x10:0x10);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], ch%4+0x8E,vformat==PAL?0x0a:0x09);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x93+ch%4,0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x98+ch%4,vformat==PAL?0x07:0x04);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xa0+ch%4,vformat==PAL?0x00:0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xa4+ch%4,vformat==PAL?0x00:0x01);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xFF, 0x01);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x88+ch%4,vformat==PAL?0x4c:0x4c);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x8c+ch%4,vformat==PAL?0x84:0x84);
					//nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xcc+ch_seq[ch%4],vformat==PAL?0x66:0x66);
					if(chip_id[0] == NVP6124_R0_ID)
						nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xcc+ch_seq[ch%4], 0x66);
					else if(chip_id[0] == NVP6114A_R0_ID && ch_seq[ch]!=0xFF)
						nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xcd+(ch_seq[ch%4]%2)*2, 0x66);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xd7, nvp_i2c_read( nvp6124_slave_addr[ch/4], 0xd7)|(1<<(ch%4)));
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xFF, 0x02);  //motion
					tmp = nvp_i2c_read( nvp6124_slave_addr[ch/4], 0x16+(ch%4)/2);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x16+(ch%4)/2, (tmp&(ch%2==0?0xF0:0x0F))|(0x05<<((ch%2)*4)));
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xFF, 0x05+ch%4);
					nvp6124_write_table( nvp6124_slave_addr[ch/4], 0x00, vformat==PAL?NVP6124_B5_25P_Buf:NVP6124_B5_30P_Buf,254 );
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x01,0x0C);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x06,0x40);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x2A,0x72);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x2B,0xA8);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x58,0x13);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x59,0x01);
					//eq_init for 1080P_2530
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xD8,0x10);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xD9,0x1F);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xDA,0x2B);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xDB,0x7F);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xDC,0xFF);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xDD,0xFF);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xDE,0xFF);

					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x24,vformat==PAL?0x2A:0x1A);  //sync changed
					//nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x47,vformat==PAL?0x04:0xEE);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x50,vformat==PAL?0x84:0x86);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xBB,vformat==PAL?0x00:0xE4);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0xFF, 0x09);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x40+(ch%4), 0x00);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x44,nvp_i2c_read( nvp6124_slave_addr[ch/4], 0x44)|(1<<(ch%4)));
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x50+4*(ch%4),vformat==PAL?pn_value_fhd_pal[0]:pn_value_fhd_nt[0]);	//ch%41 960H
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x51+4*(ch%4),vformat==PAL?pn_value_fhd_pal[1]:pn_value_fhd_nt[1]);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x52+4*(ch%4),vformat==PAL?pn_value_fhd_pal[2]:pn_value_fhd_nt[2]);
					nvp_i2c_write( nvp6124_slave_addr[ch/4], 0x53+4*(ch%4),vformat==PAL?pn_value_fhd_pal[3]:pn_value_fhd_nt[3]);
					//printk("ch %d setted to 1080P %s\n", ch, vformat==PAL?"PAL":"NTSC");
				break;
				default:
					printk("ch%d wrong mode detected!!!\n", ch);
					break;
			}

			ch_mode_status[ch] = chmode[ch];
		}
	}
}



void nvp6124_video_set_contrast(unsigned int ch, unsigned int value, unsigned int v_format)
{
	nvp_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
	if(value >= 100)
	{
		if(ch_mode_status[ch] == NVP6124_VI_SD)
			nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x10+(ch%4)),(nvp6124_con_tbl_960H[v_format]+value-100));
		else if(ch_mode_status[ch] == NVP6124_VI_1080P_2530)
			nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x10+(ch%4)),(nvp6124_con_tbl[v_format]+value-100));
		else
			nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x10+(ch%4)),(nvp6124_con_tbl_720P[v_format]+value-100));
	}
	else
	{
		if(ch_mode_status[ch] == NVP6124_VI_SD)
			nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x10+(ch%4)),(nvp6124_con_tbl_960H[v_format]+(0xff-(98-value))));
		else if(ch_mode_status[ch] == NVP6124_VI_1080P_2530)
			nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x10+(ch%4)),(nvp6124_con_tbl[v_format]+(0xff-(98-value))));
		else
			nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x10+(ch%4)),(nvp6124_con_tbl_720P[v_format]+(0xff-(98-value))));
	}
}

void nvp6124_video_set_brightness(unsigned int ch, unsigned int value, unsigned int v_format)
{
	nvp_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
	if(value >= 100)
	{
		if(ch_mode_status[ch] == NVP6124_VI_SD)
			nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x0c+(ch%4)),(nvp6124_bri_tbl_960H[v_format]+value-100));
		else if(ch_mode_status[ch] == NVP6124_VI_1080P_2530)
			nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x0c+(ch%4)),(nvp6124_bri_tbl[v_format]+value-100));
		else
			nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x0c+(ch%4)),(nvp6124_bri_tbl_720P[v_format]+value-100));
	}
	else
	{
		if(ch_mode_status[ch] == NVP6124_VI_SD)
			nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x0c+(ch%4)),(nvp6124_bri_tbl_960H[v_format]+(0xff-(98-value))));
		else if(ch_mode_status[ch] == NVP6124_VI_1080P_2530)
			nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x0c+(ch%4)),(nvp6124_bri_tbl[v_format]+(0xff-(98-value))));
		else
			nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x0c+(ch%4)),(nvp6124_bri_tbl_720P[v_format]+(0xff-(98-value))));
	}
}

void nvp6124_video_set_saturation(unsigned int ch, unsigned int value, unsigned int v_format)
{
	nvp_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
	if(ch_mode_status[ch] == NVP6124_VI_SD)
		nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x3c+(ch%4)),(nvp6124_sat_tbl_960H[v_format]+value-100));
	else if(ch_mode_status[ch] == NVP6124_VI_1080P_2530)
		nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x3c+(ch%4)),(nvp6124_sat_tbl[v_format]+value-100));
	else
		nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x3c+(ch%4)),(nvp6124_sat_tbl_720P[v_format]+value-100));
}

void nvp6124_video_set_hue(unsigned int ch, unsigned int value, unsigned int v_format)
{
	nvp_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
	if(ch_mode_status[ch] == NVP6124_VI_SD)
		nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x40+(ch%4)), (nvp6124_hue_tbl_960H[v_format]+value-100));
	else if(ch_mode_status[ch] == NVP6124_VI_1080P_2530)
		nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x40+(ch%4)), (nvp6124_hue_tbl[v_format]+value-100));
	else
		nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x40+(ch%4)), (nvp6124_hue_tbl_720P[v_format]+value-100));
}

void nvp6124_video_set_sharpness(unsigned int ch, unsigned int value)
{
	nvp_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
	nvp_i2c_write(nvp6124_slave_addr[ch/4], (0x14+(ch%4)), (0x90+value-100));
}

void nvp6124_write_table(unsigned char chip_addr, unsigned char addr, unsigned char *tbl_ptr, unsigned char tbl_cnt)
{
	unsigned char i = 0;

	for(i = 0; i < tbl_cnt; i ++)
	{
		nvp_i2c_write(chip_addr, (addr + i), *(tbl_ptr + i));
	}
}

