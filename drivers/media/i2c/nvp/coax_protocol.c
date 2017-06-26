/**************************************************************************
	Header file include
**************************************************************************/
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
#include "common.h"
#include "video.h"
#include "coax_protocol.h"

unsigned char g_coax_ch=0;
extern unsigned int nvp6124_cnt;
extern unsigned int nvp6124_mode;
extern unsigned char ch_mode_status[16];


int nvp_i2c_write(unsigned int slave, u8 addr, u8 data);
extern int nvp_i2c_write(unsigned int slave, u8 addr, u8 data);

unsigned char nvp_i2c_read(unsigned char slave, unsigned char addr);
extern unsigned char nvp_i2c_read(unsigned char slave, unsigned char addr);

void Write_I2CV6_1( unsigned char SlaveAddr, unsigned char StartAddr,  unsigned char *InitPtr, unsigned char Value, unsigned char Length)
{
	unsigned char i=0;
	if(Length==1)
		nvp_i2c_write(SlaveAddr, StartAddr, Value);
	else
	{
		for(i=0;i<Length;i++)
			nvp_i2c_write(SlaveAddr, (StartAddr+i), *(InitPtr+i));
	}
}
unsigned char Read_I2CV6_1( unsigned char SlaveAddr, unsigned char StartAddr)
{
	unsigned char ret;
	ret = 	nvp_i2c_read(SlaveAddr, StartAddr);
	return ret;
}
/*************************************************************************
	SAMSUNG PROTOCOL Regiter set value Definition
*************************************************************************/
								 //  0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0A 0x0B 0x0C 0x0D 0x0E 0x0F
unsigned char nvp6124_SAM_SET_Buf[] =		 	{0xAA,0x1C,0x18,0xFF,0xAA,0x3C,0xFF,0xFF };
unsigned char nvp6124_SAM_OSD_Buf[] =		 	{0xAA,0x1C,0x17,0x01,0xAA,0x3C,0xFF,0xFF };
unsigned char nvp6124_SAM_UP_Buf[] =		 	{0xAA,0x1C,0x03,0xFF,0xAA,0x3C,0xFF,0xFF };
unsigned char nvp6124_SAM_DOWN_Buf[] =		 	{0xAA,0x1C,0x09,0xFF,0xAA,0x3C,0xFF,0xFF };
unsigned char nvp6124_SAM_LEFT_Buf[] = 	 		{0xAA,0x1C,0x05,0xFF,0xAA,0x3C,0xFF,0xFF };
unsigned char nvp6124_SAM_RIGHT_Buf[] = 	 	{0xAA,0x1C,0x07,0xFF,0xAA,0x3C,0xFF,0xFF };
unsigned char nvp6124_SAM_PTZ_UP_Buf[] = 	 	{0xAA,0x1B,0x00,0x04,0xAA,0x3B,0x00,0x1F };
unsigned char nvp6124_SAM_PTZ_DOWN_Buf[] =  	{0xAA,0x1B,0x00,0x08,0xAA,0x3B,0x00,0x1F };
unsigned char nvp6124_SAM_PTZ_LEFT_Buf[] =  	{0xAA,0x1B,0x00,0x01,0xAA,0x3B,0x1F,0x00 };
unsigned char nvp6124_SAM_PTZ_RIGHT_Buf[] = 	{0xAA,0x1B,0x00,0x02,0xAA,0x3B,0x1F,0x00 };
unsigned char nvp6124_SAM_IRIS_OPEN_Buf[] = 	{0xAA,0x1B,0x08,0x00,0xAA,0x3B,0x00,0x00 };
unsigned char nvp6124_SAM_IRIS_CLOSE_Buf[]= 	{0xAA,0x1B,0x10,0x00,0xAA,0x3B,0x00,0x00 };
unsigned char nvp6124_SAM_FOCUS_NEAR_Buf[]= 	{0xAA,0x1B,0x02,0x00,0xAA,0x3B,0x00,0x00 };
unsigned char nvp6124_SAM_FOCUS_FAR_Buf[] = 	{0xAA,0x1B,0x01,0x00,0xAA,0x3B,0x00,0x00 };
unsigned char nvp6124_SAM_ZOOM_WIDE_Buf[] = 	{0xAA,0x1B,0x40,0x00,0xAA,0x3B,0x00,0x00 };
unsigned char nvp6124_SAM_ZOOM_TELE_Buf[] = 	{0xAA,0x1B,0x20,0x00,0xAA,0x3B,0x00,0x00 };
unsigned char nvp6124_SAM_SCAN_SR_Buf[] =	 	{0xAA,0x1C,0x13,0x01,0xAA,0x3C,0x01,0x00 };
unsigned char nvp6124_SAM_SCAN_ST_Buf[] =	 	{0xAA,0x1C,0x13,0x00,0xAA,0x3C,0x01,0x00 };
unsigned char nvp6124_SAM_PRESET1_Buf[] =	 	{0xAA,0x1C,0x19,0x01,0xAA,0x3C,0x00,0x00 };
unsigned char nvp6124_SAM_PRESET2_Buf[] =	 	{0xAA,0x1C,0x19,0x02,0xAA,0x3C,0x00,0x00 };
unsigned char nvp6124_SAM_PRESET3_Buf[] =	 	{0xAA,0x1C,0x19,0x03,0xAA,0x3C,0x00,0x00 };
unsigned char nvp6124_SAM_PATTERN_1SR_Buf[] =	{0xAA,0x1C,0x1B,0x01,0xAA,0x3C,0x01,0x00 };
unsigned char nvp6124_SAM_PATTERN_1ST_Buf[] =	{0xAA,0x1C,0x1B,0x01,0xAA,0x3C,0x00,0x00 };
unsigned char nvp6124_SAM_PATTERN_2SR_Buf[] =	{0xAA,0x1C,0x1B,0x02,0xAA,0x3C,0x01,0x00 };
unsigned char nvp6124_SAM_PATTERN_2ST_Buf[] =	{0xAA,0x1C,0x1B,0x02,0xAA,0x3C,0x00,0x00 };
unsigned char nvp6124_SAM_PATTERN_3SR_Buf[] =	{0xAA,0x1C,0x1B,0x03,0xAA,0x3C,0x01,0x00 };
unsigned char nvp6124_SAM_PATTERN_3ST_Buf[] =	{0xAA,0x1C,0x1B,0x03,0xAA,0x3C,0x00,0x00 };
unsigned char nvp6124_SAM_2ND_CMD_Buf[]     =	{0xAA,0x1B,0x00,0x00,0xAA,0x3B,0x00,0x00 };

/*************************************************************************
	Global Variables Definition
*************************************************************************/
//extern void UTIL_DelayUS(WORD wUS);


/**************************************************************************
	Function prototype
**************************************************************************/
void nvp6124_pelco_coax_mode(void);
void ahd2_pelco_shout(unsigned char pel_ch);
void nvp6124_samsung_coax_mode(void);
void init_acp(void);
void init_acp_camera_status(void);
void init_acp_reg_wr(void);
void init_acp_reg_rd(void);
void acp_reg_rx_clear(void);
void set_acp_reg_wr(unsigned char bank, unsigned char addr, unsigned char data);
void get_acp_reg_rd(unsigned char bank, unsigned char addr);
void acp_read(nvp6124_input_videofmt *pvideoacp);
unsigned char nvp6124_pelco_command(unsigned char pel_ch, unsigned char command);
unsigned char samsung_coax_command(unsigned char command);

/*************************************************************************
	Initial NVP1104A
**************************************************************************/
extern int chip_id[4];

void nvp6124_pelco_coax_mode(void)
{
	int i, j, ch;
	Write_I2CV6_1( NVP6124, 0xFF, 0,BANK1,1 );
	if(chip_id[0] == NVP6124_R0_ID)
	{
		Write_I2CV6_1( NVP6124, 0xBD, 0,0xD0,1 );
		Write_I2CV6_1( NVP6124, 0xBE, 0,0xDD,1 );
		Write_I2CV6_1( NVP6124, 0xBF, 0,0x0D,1 );
	}
	else if(chip_id[0] == NVP6114A_R0_ID)
	{
		Write_I2CV6_1( NVP6124, 0xBC, 0,0xDD,1 );
		Write_I2CV6_1( NVP6124, 0xBD, 0,0xED,1 );
	}

	for(i=0;i<4;i++)
	{
		Write_I2CV6_1( NVP6124, 0xFF, 0,BANK5+i,1 );

		if(ch_mode_status[i] == NVP6124_VI_720P_2530 || ch_mode_status[i] == NVP6124_VI_720P_5060)
		{
			Write_I2CV6_1( NVP6124, 0x7C, 0,0x11,1 );
		}
		else if(ch_mode_status[i] == NVP6124_VI_1080P_2530)
		{
			//Write_I2CV6_1( NVP6124, 0x7C, 0,0x01,1 );
			Write_I2CV6_1( NVP6124, 0x7C, 0,0x11,1 );
		}
		else if(ch_mode_status[i] == NVP6124_VI_SD)
		{
			Write_I2CV6_1( NVP6124, 0x7C, 0,0x11,1 );
		}
		else
			printk("nvp6124_pelco_coax_mode ch[%d] error\n", i);
	}

	for(i=0;i<(nvp6124_cnt*4);i++)
	{
		if(ch_mode_status[i] == NVP6124_VI_SD)
		{
			//for(i=0;i<2;i++)
			{
				Write_I2CV6_1( NVP6124, 0xFF, 0,BANK3+(i/2),1 );
				//for(j=0;j<2;j++)
				{

					Write_I2CV6_1( NVP6124, AHD2_PEL_BAUD+((i%2)*0x80), 0,nvp6124_mode%2? 0x1B:0x1B,1 );
					Write_I2CV6_1( NVP6124, AHD2_PEL_LINE+((i%2)*0x80), 0,nvp6124_mode%2? 0x0E:0x0E,1 );
					Write_I2CV6_1( NVP6124, PACKET_MODE+((i%2)*0x80), 0,0x06,1 );
					Write_I2CV6_1( NVP6124, AHD2_PEL_SYNC+((i%2)*0x80), 0,nvp6124_mode%2? 0xa0:0xd4,1 );
					Write_I2CV6_1( NVP6124, AHD2_PEL_SYNC+1+((i%2)*0x80), 0,nvp6124_mode%2?0x05:0x05,1 );
					Write_I2CV6_1( NVP6124, AHD2_PEL_EVEN+((i%2)*0x80), 0,0x01,1 );
				}
			}

			printk("\r\n960H COAXIAL PROTOCOL IS SETTING....");
		}
		else if(ch_mode_status[i] == NVP6124_VI_720P_2530 || ch_mode_status[i] == NVP6124_VI_720P_5060)
		{
			//for(i=0;i<2;i++)
			{
				Write_I2CV6_1( NVP6124, 0xFF, 0,BANK3+(i/2),1 );
				//for(j=0;j<2;j++)
				{
					Write_I2CV6_1( NVP6124, AHD2_PEL_BAUD+((i%2)*0x80), 0,0x0D,1 );
					Write_I2CV6_1( NVP6124, AHD2_PEL_LINE+((i%2)*0x80), 0,nvp6124_mode%2? 0x0D:0x0E,1 );
					Write_I2CV6_1( NVP6124, PACKET_MODE+((i%2)*0x80), 0,0x06,1 );
					Write_I2CV6_1( NVP6124, AHD2_PEL_SYNC+((i%2)*0x80), 0,nvp6124_mode%2? 0x40:0x80,1 );
					Write_I2CV6_1( NVP6124, AHD2_PEL_SYNC+1+((i%2)*0x80), 0,nvp6124_mode%2?0x05:0x00,1 );
					Write_I2CV6_1( NVP6124, AHD2_PEL_EVEN+((i%2)*0x80), 0,0x00,1 );
				}
			}

			printk("\r\n720P COAXIAL PROTOCOL IS SETTING....");
		}
		else if(ch_mode_status[i] == NVP6124_VI_1080P_2530)
		{
			//for(i=0;i<2;i++)
			{
				Write_I2CV6_1( NVP6124, 0xFF, 0,BANK3+(i/2),1 );
				//for(j=0;j<2;j++)
				{
					Write_I2CV6_1( NVP6124, AHD2_FHD_BAUD+((i%2)*0x80), 0,0x0E,1 );
					Write_I2CV6_1( NVP6124, AHD2_FHD_LINE+((i%2)*0x80), 0,nvp6124_mode%2? 0x0E:0x0E,1 );
					Write_I2CV6_1( NVP6124, AHD2_PEL_SYNC+((i%2)*0x80), 0,nvp6124_mode%2? 0x14:0x14,1 );
					Write_I2CV6_1( NVP6124, AHD2_FHD_LINES+((i%2)*0x80), 0,0x03,1 );
					Write_I2CV6_1( NVP6124, AHD2_FHD_BYTE+((i%2)*0x80), 0,0x03,1 );
					Write_I2CV6_1( NVP6124, AHD2_FHD_MODE+((i%2)*0x80), 0,0x10,1 );
					Write_I2CV6_1( NVP6124, ((i%2)*0x80)+0x0E, 0,0x00,1 );  //10.25
					Write_I2CV6_1( NVP6124, AHD2_PEL_EVEN+((i%2)*0x80), 0,0x00,1 );
				}
			}

			printk("\r\n1080P COAXIAL PROTOCOL IS SETTING....");
		}
		else
		{
			nvp6124_samsung_coax_mode();
		}
	}
}

void init_acp(void)
{
 	//Decoder TX Setting
	Write_I2CV6_1( NVP6124, 0xFF, 0,BANK1,1 );
	Write_I2CV6_1( NVP6124, 0xBD, 0,0xD0,1 );
	Write_I2CV6_1( NVP6124, 0xFF, 0,BANK5,1 );
	Write_I2CV6_1( NVP6124, 0x7C, 0,0x11,1 );
	Write_I2CV6_1( NVP6124, 0xFF, 0,BANK3,1 );
	Write_I2CV6_1( NVP6124, AHD2_FHD_BAUD, 0,0x0E,1 );
	Write_I2CV6_1( NVP6124, AHD2_FHD_LINE, 0,0x0E,1 );
	Write_I2CV6_1( NVP6124, AHD2_PEL_SYNC, 0,0x14,1 );
	Write_I2CV6_1( NVP6124, AHD2_PEL_SYNC+1, 0,0x00,1 );  //10.25
	Write_I2CV6_1( NVP6124, AHD2_FHD_LINES, 0,0x07,1 );
	Write_I2CV6_1( NVP6124, AHD2_FHD_BYTE, 0,0x03,1 );
	Write_I2CV6_1( NVP6124, AHD2_FHD_MODE, 0,0x10,1 );
	Write_I2CV6_1( NVP6124, AHD2_PEL_EVEN, 0,0x00,1 );

 	//Decoder RX Setting
	Write_I2CV6_1( NVP6124, 0xFF, 0,BANK5,1 );
	Write_I2CV6_1( NVP6124, 0x30, 0,0x00,1 );
	Write_I2CV6_1( NVP6124, 0x31, 0,0x01,1 );
	Write_I2CV6_1( NVP6124, 0x32, 0,0x64,1 );
	Write_I2CV6_1( NVP6124, 0x7C, 0,0x11,1 );
	Write_I2CV6_1( NVP6124, 0x7D, 0,0x80,1 );

	Write_I2CV6_1( NVP6124, 0xFF, 0,BANK3,1 );
	//Write_I2CV6_1( NVP6124, 0x03, 0,0x0E,1 );
	//Write_I2CV6_1( NVP6124, 0x10, 0,0x48,1 );
	//Write_I2CV6_1( NVP6124, 0x05, 0,0x04,1 );


	//Write_I2CV6_1( NVP6124, 0x62, 0,0x0E,1 );	// Decoder TX DATA Check
	Write_I2CV6_1( NVP6124, 0x62, 0,0x06,1 );	// Camera TX DATA Check
	Write_I2CV6_1( NVP6124, 0x63, 0,0x01,1 );
	Write_I2CV6_1( NVP6124, 0x66, 0,0x80,1 );
	Write_I2CV6_1( NVP6124, 0x67, 0,0x01,1 );
	Write_I2CV6_1( NVP6124, 0x68, 0,0x70,1 );	// RX size
	printk("\n\rACP DATA READ TEST!!!!\n\r");
}

void init_acp_camera_status(void)
{
	//A-CP ID Camera info mode
	Write_I2CV6_1( NVP6124, 0xFF, 0,BANK3,1 );
	Write_I2CV6_1( NVP6124, AHD2_FHD_LINES, 0,0x07,1 );
	Write_I2CV6_1( NVP6124, ACP_MODE_ID, 0,ACP_CAM_STAT,1 );
	Write_I2CV6_1( NVP6124, AHD2_FHD_D0, 0,ACP_RX_D0,1 );
	Write_I2CV6_1( NVP6124, AHD2_FHD_D0+1, 0,ACP_RX_D0+1,1 );
	Write_I2CV6_1( NVP6124, AHD2_FHD_D0+2, 0,ACP_RX_D0+2,1 );
	Write_I2CV6_1( NVP6124, AHD2_FHD_D0+3, 0,ACP_RX_D0+3,1 );
	Write_I2CV6_1( NVP6124, AHD2_FHD_OUT, 0,0x08,1 );
	//Write_I2CV6_1( NVP6124, AHD2_FHD_OUT, 0,0x00,1 );
	msleep( 25 );
	msleep( 25 );
	msleep( 25 );
	msleep( 25 );
	printk("ACP_Camera_status_mode_set\n");
}


void init_acp_reg_wr(void)
{
	//A-CP ID Register write mode
	Write_I2CV6_1( NVP6124, 0xFF, 0,BANK3,1 );
	Write_I2CV6_1( NVP6124, AHD2_FHD_LINES, 0,0x03,1 );
	Write_I2CV6_1( NVP6124, ACP_MODE_ID, 0,ACP_REG_WR,1 );
	printk("ACP_register_write_mode_set\n");
}

void init_acp_reg_rd(void)
{
	//A-CP ID Register read mode
	Write_I2CV6_1( NVP6124, 0xFF, 0,BANK3,1 );
	Write_I2CV6_1( NVP6124, AHD2_FHD_LINES, 0,0x03,1 );
	Write_I2CV6_1( NVP6124, ACP_MODE_ID, 0,ACP_REG_RD,1 );
	printk("ACP_register_read_mode_set\n");
}

void set_acp_reg_wr(unsigned char bank, unsigned char addr, unsigned char data)
{
	//A-CP ID Register write mode
	Write_I2CV6_1( NVP6124, 0xFF, 0,BANK3,1 );
	Write_I2CV6_1( NVP6124, AHD2_FHD_D0, 0,ACP_REG_WR,1 );
	Write_I2CV6_1( NVP6124, AHD2_FHD_D0+1, 0,bank,1 );
	Write_I2CV6_1( NVP6124, AHD2_FHD_D0+2, 0,addr,1 );
	Write_I2CV6_1( NVP6124, AHD2_FHD_D0+3, 0,data,1 );
	Write_I2CV6_1( NVP6124, AHD2_FHD_OUT, 0,0x08,1 );
	msleep( 25 );
	msleep( 25 );
	msleep( 25 );
	msleep( 25 );
	printk("set_ACP_register_write\n");
}

void get_acp_reg_rd(unsigned char bank, unsigned char addr)
{
	//A-CP ID Register read mode
	Write_I2CV6_1( NVP6124, 0xFF, 0,BANK3,1 );
	Write_I2CV6_1( NVP6124, AHD2_FHD_D0, 0,ACP_REG_RD,1 );
	Write_I2CV6_1( NVP6124, AHD2_FHD_D0+1, 0,bank,1 );
	Write_I2CV6_1( NVP6124, AHD2_FHD_D0+2, 0,addr,1 );
	Write_I2CV6_1( NVP6124, AHD2_FHD_D0+3, 0,0x00,1 );//Dummy
	Write_I2CV6_1( NVP6124, AHD2_FHD_OUT, 0,0x08,1 );
	msleep( 25 );
	msleep( 25 );
	msleep( 25 );
	msleep( 25 );
	printk("get_register_read\n");
}


void acp_read(nvp6124_input_videofmt *pvideoacp)
{
	unsigned long buf[16];

	Write_I2CV6_1(NVP6124, 0xFF, 0,0x00,1);
	printk("ACP_RX_DONE = 0x%x\n",Read_I2CV6_1(0x60, 0xc7));
	Write_I2CV6_1(NVP6124, 0xFF, 0,0x03,1);

	// A-CP Camera stuts mode
	if(Read_I2CV6_1(NVP6124, ACP_RX_D0)==ACP_CAM_STAT)
	{
		int i;
		for(i=0;i<8;i++)
		{
			buf[i] = Read_I2CV6_1(NVP6124,ACP_RX_D0+i);
			//pvideoacp->getacpdata[i] = buf[i];
			pvideoacp->getacpdata[i] = buf[i];
			printk("ACP_DATA_0x%d = 0x%x\n",i,buf[i]);
			Write_I2CV6_1(NVP6124, AHD2_FHD_D0+i, 0,buf[i],1); // Decoder compare with Encoder value
		}
		Write_I2CV6_1( NVP6124, AHD2_FHD_OUT, 0,0x08,1 ); // Decoder compare with Encoder value
		printk("ACP_Camera_stuts_mode_read_ok\n");
	}

	// A-CP Register Ctrl Write mode
	else if(Read_I2CV6_1(NVP6124, ACP_RX_D0)==ACP_REG_WR)
	{
		int i;
		for(i=0;i<4;i++)
		{
			buf[i] = Read_I2CV6_1(NVP6124,ACP_RX_D0+i);
			pvideoacp->getacpdata[i] = buf[i];
			printk("ACP_Write_0x%d = 0x%x\n",i,buf[i]);
		}

		printk("ACP_BANK_0x%x\n",Read_I2CV6_1(NVP6124,ACP_RX_D0+1));
		printk("ACP_ADDR_0x%x\n",Read_I2CV6_1(NVP6124,ACP_RX_D0+2));
		printk("ACP_DATA_0x%x\n",Read_I2CV6_1(NVP6124,ACP_RX_D0+3));
		Write_I2CV6_1( NVP6124, AHD2_FHD_OUT, 0,0x00,1 );
		printk("ACP_regiter_write_mode_ok\n");

	}
	// A-CP Register Ctrl Read mode
	else if(Read_I2CV6_1(NVP6124, ACP_RX_D0)==ACP_REG_RD)
	{
		int i;
		for(i=0;i<4;i++)
		{
			buf[i] = Read_I2CV6_1(NVP6124,ACP_RX_D0+i);
			pvideoacp->getacpdata[i] = buf[i];
			printk("ACP_Read_0x%d = 0x%x\n",i,buf[i]);
		}
		printk("ACP_BANK_0x%x\n",Read_I2CV6_1(NVP6124,ACP_RX_D0+1));
		printk("ACP_ADDR_0x%x\n",Read_I2CV6_1(NVP6124,ACP_RX_D0+2));
		printk("ACP_DATA_0x%x\n",Read_I2CV6_1(NVP6124,ACP_RX_D0+3));
		Write_I2CV6_1( NVP6124, AHD2_FHD_OUT, 0,0x00,1 );
		printk("ACP_regiter_read_mode_ok\n");
	}

	// A-CP Read error
	#if 1
	else
	{
		int i;
		for(i=0;i<8;i++)
		{
			pvideoacp->getacpdata[i] = 0x00;
			printk("ACP_DATA_0x%d = 0x%x\n",i,pvideoacp->getacpdata[i]);
		}
		printk("ACP_RX_Error!!!!\n");
		Write_I2CV6_1( NVP6124, AHD2_FHD_OUT, 0,0x00,1 );
	}
	#endif
	acp_reg_rx_clear();
}

void acp_reg_rx_clear(void)
{
	// A-CP read done & clear
	Write_I2CV6_1(NVP6124, 0x3a, 0,0x01,1);
	Write_I2CV6_1(NVP6124, 0xFF, 0,0x00,1);
	if(Read_I2CV6_1(NVP6124, 0xc7)==0x00)
	{
		printk("ACP_RX_DONE clear!!!!\n");
	}
	else
	{
		if(Read_I2CV6_1(NVP6124, 0xc7)==0x00)
		{
			printk("ACP_RX_DONE clear!!!!\n");
		}
		else
		{
			printk("ACP_RX_DONE not clear!!!!\n");
		}
	}
	Write_I2CV6_1(NVP6124, 0xFF, 0,0x03,1);
	Write_I2CV6_1(NVP6124, 0x3a, 0,0x00,1);
}

unsigned char nvp6124_pelco_command(unsigned char pel_ch, unsigned char command)
{
	int i;
	unsigned char str[4];

	msleep( 20 );

	if(ch_mode_status[pel_ch] == NVP6124_VI_1080P_2530)
	{
		switch(command)
		{
			case  PELCO_CMD_RESET :
				str[0] = 0x00;str[1] = 0x00;str[2] = 0x00;str[3] = 0x00;
			break;
			case  PELCO_CMD_SET:
				str[0] = 0x02;str[1] = 0x00;str[2] = 0x00;str[3] = 0x00;
			break;
			case  PELCO_CMD_UP:
				str[0] = 0x00;str[1] = 0x08;str[2] = 0x00;str[3] = 0x32;
			break;
			case  PELCO_CMD_DOWN:
				str[0] = 0x00;str[1] = 0x10;str[2] = 0x00;str[3] = 0x32;
			break;
			case  PELCO_CMD_LEFT:
				str[0] = 0x00;str[1] = 0x04;str[2] = 0x32;str[3] = 0x00;
			break;
			case  PELCO_CMD_RIGHT:
				str[0] = 0x00;str[1] = 0x02;str[2] = 0x32;str[3] = 0x00;
			break;
			case  PELCO_CMD_OSD:
				str[0] = 0x00;str[1] = 0x03;str[2] = 0x00;str[3] = 0x3F;
			break;
			case  PELCO_CMD_IRIS_OPEN:
				str[0] = 0x02;str[1] = 0x00;str[2] = 0x00;str[3] = 0x00;
			break;
			case  PELCO_CMD_IRIS_CLOSE:
				str[0] = 0x04;str[1] = 0x00;str[2] = 0x00;str[3] = 0x00;
			break;
			case  PELCO_CMD_FOCUS_NEAR:
				str[0] = 0x01;str[1] = 0x00;str[2] = 0x00;str[3] = 0x00;
			break;
			case  PELCO_CMD_FOCUS_FAR:
				str[0] = 0x00;str[1] = 0x80;str[2] = 0x00;str[3] = 0x00;
			break;
			case  PELCO_CMD_ZOOM_WIDE:
				str[0] = 0x00;str[1] = 0x40;str[2] = 0x00;str[3] = 0x00;
			break;
			case  PELCO_CMD_ZOOM_TELE:
				str[0] = 0x00;str[1] = 0x20;str[2] = 0x00;str[3] = 0x00;
			break;

			default : return 0; //unexpected command
		}
	}
	else if(ch_mode_status[pel_ch] == NVP6124_VI_720P_2530)
	{
		switch(command)
		{
			case  PELCO_CMD_RESET :
				str[0] = 0x00;str[1] = 0x00;str[2] = 0x00;str[3] = 0x00;
			break;
			case  PELCO_CMD_SET:
				str[0] = 0x40;str[1] = 0x00;str[2] = 0x00;str[3] = 0x00;
			break;
			case  PELCO_CMD_UP:
				str[0] = 0x00;str[1] = 0x10;str[2] = 0x10;str[3] = 0x4C;
			break;
			case  PELCO_CMD_DOWN:
				str[0] = 0x00;str[1] = 0x08;str[2] = 0x08;str[3] = 0x4C;
			break;
			case  PELCO_CMD_LEFT:
				str[0] = 0x00;str[1] = 0x20;str[2] = 0x20;str[3] = 0x00;
			break;
			case  PELCO_CMD_RIGHT:
				str[0] = 0x00;str[1] = 0x40;str[2] = 0x40;str[3] = 0x00;
			break;
			case  PELCO_CMD_OSD:
				str[0] = 0x00;str[1] = 0xC0;str[2] = 0xC0;str[3] = 0xFA;
			break;
			case  PELCO_CMD_IRIS_OPEN:
				str[0] = 0x40;str[1] = 0x00;str[2] = 0x00;str[3] = 0x00;
			break;
			case  PELCO_CMD_IRIS_CLOSE:
				str[0] = 0x20;str[1] = 0x00;str[2] = 0x00;str[3] = 0x00;
			break;
			case  PELCO_CMD_FOCUS_NEAR:
				str[0] = 0x80;str[1] = 0x00;str[2] = 0x00;str[3] = 0x00;
			break;
			case  PELCO_CMD_FOCUS_FAR:
				str[0] = 0x00;str[1] = 0x01;str[2] = 0x01;str[3] = 0x00;
			break;
			case  PELCO_CMD_ZOOM_WIDE:
				str[0] = 0x00;str[1] = 0x02;str[2] = 0x02;str[3] = 0x00;
			break;
			case  PELCO_CMD_ZOOM_TELE:
				str[0] = 0x00;str[1] = 0x04;str[2] = 0x04;str[3] = 0x00;
			break;
			case  PELCO_CMD_SCAN_SR:
				str[0] = 0x00;str[1] = 0xE0;str[2] = 0xE0;str[3] = 0x46;
			break;
			case  PELCO_CMD_SCAN_ST:
				str[0] = 0x00;str[1] = 0xE0;str[2] = 0xE0;str[3] = 0x00;
			break;
			case  PELCO_CMD_PRESET1:
				str[0] = 0x00;str[1] = 0xE0;str[2] = 0xE0;str[3] = 0x80;
			break;
			case  PELCO_CMD_PRESET2:
				str[0] = 0x00;str[1] = 0xE0;str[2] = 0xE0;str[3] = 0x40;
			break;
			case  PELCO_CMD_PRESET3:
				str[0] = 0x00;str[1] = 0xE0;str[2] = 0xE0;str[3] = 0xC0;
			break;
			case  PELCO_CMD_PTN1_SR:
				str[0] = 0x00;str[1] = 0xF8;str[2] = 0xF8;str[3] = 0x01;
			break;
			case  PELCO_CMD_PTN1_ST:
				str[0] = 0x00;str[1] = 0x84;str[2] = 0x84;str[3] = 0x01;
			break;
			case  PELCO_CMD_PTN2_SR:
				str[0] = 0x00;str[1] = 0xF8;str[2] = 0xF8;str[3] = 0x02;
			break;
			case  PELCO_CMD_PTN2_ST:
				str[0] = 0x00;str[1] = 0x84;str[2] = 0x84;str[3] = 0x02;
			break;
			case  PELCO_CMD_PTN3_SR:
				str[0] = 0x00;str[1] = 0xF8;str[2] = 0xF8;str[3] = 0x03;
			break;
			case  PELCO_CMD_PTN3_ST:
				str[0] = 0x00;str[1] = 0x84;str[2] = 0x84;str[3] = 0x03;
			break;
			case  PELCO_CMD_RUN:
				str[0] = 0x00;str[1] = 0xC4;str[2] = 0xC4;str[3] = 0x00;
			break;

			default : return 0; //unexpected command
		}
	}
	else
	{
		switch(command)
		{
			case  PELCO_CMD_RESET :
				str[0] = 0x00;str[1] = 0x00;str[2] = 0x00;str[3] = 0x00;
			break;
			case  PELCO_CMD_SET:
				str[0] = 0x40;str[1] = 0x00;str[2] = 0x00;str[3] = 0x00;
			break;
			case  PELCO_CMD_UP:
				str[0] = 0x00;str[1] = 0x10;str[2] = 0x00;str[3] = 0x4C;
			break;
			case  PELCO_CMD_DOWN:
				str[0] = 0x00;str[1] = 0x08;str[2] = 0x00;str[3] = 0x4C;
			break;
			case  PELCO_CMD_LEFT:
				str[0] = 0x00;str[1] = 0x20;str[2] = 0x4C;str[3] = 0x00;
			break;
			case  PELCO_CMD_RIGHT:
				str[0] = 0x00;str[1] = 0x40;str[2] = 0x4C;str[3] = 0x00;
			break;
			case  PELCO_CMD_OSD:
				str[0] = 0x00;str[1] = 0xC0;str[2] = 0x00;str[3] = 0xFA;
			break;
			case  PELCO_CMD_IRIS_OPEN:
				str[0] = 0x40;str[1] = 0x00;str[2] = 0x00;str[3] = 0x00;
			break;
			case  PELCO_CMD_IRIS_CLOSE:
				str[0] = 0x20;str[1] = 0x00;str[2] = 0x00;str[3] = 0x00;
			break;
			case  PELCO_CMD_FOCUS_NEAR:
				str[0] = 0x80;str[1] = 0x00;str[2] = 0x00;str[3] = 0x00;
			break;
			case  PELCO_CMD_FOCUS_FAR:
				str[0] = 0x00;str[1] = 0x01;str[2] = 0x00;str[3] = 0x00;
			break;
			case  PELCO_CMD_ZOOM_WIDE:
				str[0] = 0x00;str[1] = 0x02;str[2] = 0x00;str[3] = 0x00;
			break;
			case  PELCO_CMD_ZOOM_TELE:
				str[0] = 0x00;str[1] = 0x04;str[2] = 0x00;str[3] = 0x00;
			break;
			case  PELCO_CMD_SCAN_SR:
				str[0] = 0x00;str[1] = 0xE0;str[2] = 0x00;str[3] = 0x46;
			break;
			case  PELCO_CMD_SCAN_ST:
				str[0] = 0x00;str[1] = 0xE0;str[2] = 0x00;str[3] = 0x00;
			break;
			case  PELCO_CMD_PRESET1:
				str[0] = 0x00;str[1] = 0xE0;str[2] = 0x00;str[3] = 0x80;
			break;
			case  PELCO_CMD_PRESET2:
				str[0] = 0x00;str[1] = 0xE0;str[2] = 0x00;str[3] = 0x40;
			break;
			case  PELCO_CMD_PRESET3:
				str[0] = 0x00;str[1] = 0xE0;str[2] = 0x00;str[3] = 0xC0;
			break;
			case  PELCO_CMD_PTN1_SR:
				str[0] = 0x00;str[1] = 0xF8;str[2] = 0x00;str[3] = 0x01;
			break;
			case  PELCO_CMD_PTN1_ST:
				str[0] = 0x00;str[1] = 0x84;str[2] = 0x00;str[3] = 0x01;
			break;
			case  PELCO_CMD_PTN2_SR:
				str[0] = 0x00;str[1] = 0xF8;str[2] = 0x00;str[3] = 0x02;
			break;
			case  PELCO_CMD_PTN2_ST:
				str[0] = 0x00;str[1] = 0x84;str[2] = 0x00;str[3] = 0x02;
			break;
			case  PELCO_CMD_PTN3_SR:
				str[0] = 0x00;str[1] = 0xF8;str[2] = 0x00;str[3] = 0x03;
			break;
			case  PELCO_CMD_PTN3_ST:
				str[0] = 0x00;str[1] = 0x84;str[2] = 0x00;str[3] = 0x03;
			break;
			case  PELCO_CMD_RUN:
				str[0] = 0x00;str[1] = 0xC4;str[2] = 0x00;str[3] = 0x00;
			break;

			default : return 0; //unexpected command
		}
	}


	//PELCO CH SELECT
	//if(pel_ch<2)
	{
		Write_I2CV6_1( NVP6124, 0xFF, 0,BANK3+(pel_ch/2),1 );
	}
	//else
	{
		//Write_I2CV6_1( NVP6124, 0xFF, 0,BANK4,1 );
	}

	if(ch_mode_status[pel_ch] == NVP6124_VI_SD)
	{
		for(i=0;i<4;i++)
		{
			Write_I2CV6_1( NVP6124, AHD2_PEL_D0+i+(((pel_ch)%2)*0x80), 0,0x00,1 );
		}

		ahd2_pelco_shout(pel_ch);


		for(i=0;i<4;i++)
		{
			Write_I2CV6_1( NVP6124, AHD2_PEL_D0+i+(((pel_ch)%2)*0x80), 0,str[i],1 );
		}
		ahd2_pelco_shout(pel_ch);
		printk("\r\nPelco protocl shout!");
	}

	else if(ch_mode_status[pel_ch] == NVP6124_VI_720P_2530|| ch_mode_status[pel_ch] == NVP6124_VI_720P_5060)
	{
		for(i=0;i<4;i++)
		{
			Write_I2CV6_1( NVP6124, AHD2_PEL_D0+i+(((pel_ch)%2)*0x80), 0,0x00,1 );
		}

		ahd2_pelco_shout(pel_ch);


		for(i=0;i<4;i++)
		{
			Write_I2CV6_1( NVP6124, AHD2_PEL_D0+i+(((pel_ch)%2)*0x80), 0,str[i],1 );
		}
		ahd2_pelco_shout(pel_ch);
		printk("\r\n720p Coaxial protocl shout!");
	}

	else if(ch_mode_status[pel_ch] == NVP6124_VI_1080P_2530)
	{
		for(i=0;i<4;i++)
		{
			Write_I2CV6_1( NVP6124, AHD2_FHD_D0+i+(((pel_ch)%2)*0x80), 0,0x00,1 );
		}

		ahd2_pelco_shout(pel_ch);

		for(i=0;i<4;i++)
		{
			Write_I2CV6_1( NVP6124, AHD2_FHD_D0+i+(((pel_ch)%2)*0x80), 0,str[i],1 );
		}

		ahd2_pelco_shout(pel_ch);
		printk("\r\n1080p Coaxial protocl shout!");
	}
	return 1;
}

void ahd2_pelco_shout(unsigned char pel_ch)
{
	Write_I2CV6_1( NVP6124, 0xFF, 0,BANK3+(pel_ch/2),1 );
		if(ch_mode_status[pel_ch] == NVP6124_VI_SD)
		{
			Write_I2CV6_1( NVP6124, AHD2_PEL_OUT+(((pel_ch)%2)*0x80), 0,0x01,1 );
			msleep( 30 );
			Write_I2CV6_1( NVP6124, AHD2_PEL_OUT+(((pel_ch)%2)*0x80), 0,0x00,1 );
			msleep( 60 );
		}
		else if(ch_mode_status[pel_ch] == NVP6124_VI_720P_2530|| ch_mode_status[pel_ch] == NVP6124_VI_720P_5060)
		{
			Write_I2CV6_1( NVP6124, AHD2_PEL_OUT+(((pel_ch)%2)*0x80), 0,0x01,1 );
			msleep( 30 );
			Write_I2CV6_1( NVP6124, AHD2_PEL_OUT+(((pel_ch)%2)*0x80), 0,0x00,1 );
			msleep( 60 );
		}

		else //if(g_hsize == AHD2_1080P)
		{
			Write_I2CV6_1( NVP6124, AHD2_FHD_OUT+(((pel_ch)%2)*0x80), 0,0x08,1 );
			msleep( 25 );
			Write_I2CV6_1( NVP6124, AHD2_FHD_OUT+(((pel_ch)%2)*0x80), 0,0x00,1 );
			msleep( 25 );
		}
}

void nvp6124_samsung_coax_mode(void)
{
	//Write_I2CV6_1( NVP6124, 0xFF, 0,BANK1,1 );
	//Write_I2CV6_1( NVP6124, 0xC8, 0,0x00,1 );
	//Write_I2CV6_1( NVP6124, 0xC9, 0,0x00,1 );
	//Write_I2CV6_1( NVP6124, 0xCC, 0,0x00,1 );
	//Write_I2CV6_1( NVP6124, 0xCD, 0,0x00,1 );
	//Write_I2CV6_1( NVP6124, 0xCE, 0,0x00,1 );
	//Write_I2CV6_1( NVP6124, 0xCF, 0,0x00,1 );

	Write_I2CV6_1( NVP6124, 0xFF, 0,BANK1,1 );
	Write_I2CV6_1( NVP6124, 0xBC, 0,0xDD,1 );
	Write_I2CV6_1( NVP6124, 0xBD, 0,0xDD,1 );

	Write_I2CV6_1( NVP6124, 0xFF, 0,BANK3,1 );

	Write_I2CV6_1( NVP6124, COAX_BAUD, 0,0x37,1 );
	Write_I2CV6_1( NVP6124, BL_HSP01, 0,0x46,1 );
	//Write_I2CV6_1( NVP6124, TX_START, 0,0x00,1 );
	Write_I2CV6_1( NVP6124, TX_BYTE_LEN, 0,0x08,1 );			// 2Line mode
	Write_I2CV6_1( NVP6124, PACKET_MODE, 0,0x00,1 );

	// Video format select
	Write_I2CV6_1( NVP6124, BL_TXST1, NULL, nvp6124_mode%2? 0x06 : 0x0A,1);	// TX Line
	Write_I2CV6_1( NVP6124, BL_RXST1, NULL, nvp6124_mode%2? 0x08 : 0x07,1);	// RX Line
	Write_I2CV6_1( NVP6124, COAX_BAUD, NULL, nvp6124_mode%2? 0x23 : 0x37,1);	// TX Duty rate
	Write_I2CV6_1( NVP6124, COAX_RBAUD, NULL, nvp6124_mode%2? 0x23 : 0x37,1);	// TX Duty rate

	Write_I2CV6_1( NVP6124, EVEN_LINE, 0,0x01,1 );

	printk("test CCVC SETTING FINISHED\r\n");
}

unsigned char samsung_coax_command(unsigned char command)
{
	switch(command)
	{
		case SS_CMD_SET: 		Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_SET_Buf,0,  8); break;
		case SS_CMD_UP : 		Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_OSD_Buf,0,  8); break;
		case SS_CMD_DOWN: 		Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_UP_Buf,0,  8); break;
		case SS_CMD_LEFT: 		Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_DOWN_Buf,0,  8); break;
		case SS_CMD_RIGHT: 		Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_LEFT_Buf,0,  8); break;
		case SS_CMD_OSD: 		Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_RIGHT_Buf,0,  8); break;
		case SS_CMD_PTZ_UP: 	Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_PTZ_UP_Buf,0,  8); break;
		case SS_CMD_PTZ_DOWN: 	Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_PTZ_DOWN_Buf,0,  8); break;
		case SS_CMD_PTZ_LEFT: 	Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_PTZ_LEFT_Buf,0, 8); break;
		case SS_CMD_PTZ_RIGHT: 	Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_PTZ_RIGHT_Buf,0,  8); break;
		case SS_CMD_IRIS_OPEN: 	Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_IRIS_OPEN_Buf,0, 8); break;
		case SS_CMD_IRIS_CLOSE: Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_IRIS_CLOSE_Buf,0, 8); break;
		case SS_CMD_FOCUS_NEAR: Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_FOCUS_NEAR_Buf,0, 8); break;
		case SS_CMD_FOCUS_FAR: 	Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_FOCUS_FAR_Buf,0, 8); break;
		case SS_CMD_ZOOM_WIDE: 	Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_ZOOM_WIDE_Buf,0, 8); break;
		case SS_CMD_ZOOM_TELE: 	Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_ZOOM_TELE_Buf,0, 8); break;
		case SS_CMD_SCAN_SR: 	Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_SCAN_SR_Buf,0, 8); break;
		case SS_CMD_SCAN_ST: 	Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_SCAN_ST_Buf,0, 8); break;
		case SS_CMD_PRESET1: 	Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_PRESET1_Buf,0, 8); break;
		case SS_CMD_PRESET2: 	Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_PRESET2_Buf,0, 8); break;
		case SS_CMD_PRESET3: 	Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_PRESET3_Buf,0, 8); break;
		case SS_CMD_PTN1_SR: 	Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_PATTERN_1SR_Buf,0, 8); break;
		case SS_CMD_PTN1_ST: 	Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_PATTERN_1ST_Buf,0, 8); break;
		case SS_CMD_PTN2_SR: 	Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_PATTERN_2SR_Buf,0, 8); break;
		case SS_CMD_PTN2_ST: 	Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_PATTERN_2ST_Buf,0, 8); break;
		case SS_CMD_PTN3_SR: 	Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_PATTERN_3SR_Buf,0, 8); break;
		case SS_CMD_PTN3_ST: 	Write_I2CV6_1( NVP6124, SAM_D0,  nvp6124_SAM_PATTERN_3ST_Buf,0, 8); break;
		case 255: 				nvp6124_samsung_coax_mode(); break;
		default			   :    return 0; // unexpected command
	}
	//Write_I2CV6_1( NVP6124, SAM_D0+8,  SAM_2ND_CMD_Buf,0, 8);


	//Write_I2CV6_1( NVP6124, SAM_D0, SAM_SET_Buf,0, 8 );
	//Write_I2CV6_1( NVP6124, SAM_D0+8,  SAM_2ND_CMD_Buf,0, 8);
	Write_I2CV6_1( NVP6124, 0x30, 0,0x00,1 );
	msleep( 20 );

	printk("CCVC is SHOUT!! Command is %d\r\n",command);
	Write_I2CV6_1( NVP6124, SAM_OUT, 0,0x01,1 );
	Write_I2CV6_1( NVP6124, SAM_OUT, 0,0x00,1 );
	msleep( 20 );

	//return 1;
}

