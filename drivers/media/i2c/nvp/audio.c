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

int nvp_i2c_write(unsigned int slave, u8 addr, u8 data);
extern int nvp_i2c_write(unsigned int slave, u8 addr, u8 data);

unsigned char nvp_i2c_read(unsigned char slave, unsigned char addr);
extern unsigned char nvp_i2c_read(unsigned char slave, unsigned char addr);

/*
param:
dec[4]= {0x60,0x62,0x64,0x66};
ch_num: audio channel number
samplerate: 0[8k], 1[16k]
bits: 0[16bits], 1[8bits]
*/
void audio_init(unsigned char dec, unsigned char ch_num, unsigned char samplerate, unsigned char bits)
{
	unsigned char smode;
	nvp_i2c_write(dec, 0xff, 0x01);
	if( (dec == 0x60) || (dec == 0x64))
	{
		nvp_i2c_write(dec, 0x06, 0x1a);
		nvp_i2c_write(dec, 0x07, 0x80|(samplerate<<3)|(bits<<2));	//Rec I2C 8K 16bit : master
		if(8 == ch_num)
		{
			nvp_i2c_write(dec, 0x06, 0x1b);
			nvp_i2c_write(dec, 0x08, 0x02);
			nvp_i2c_write(dec, 0x0f, 0x54);    //set I2S right sequence
			nvp_i2c_write(dec, 0x10, 0x76);
		}
		else if(4 == ch_num)
		{
			nvp_i2c_write(dec, 0x06, 0x1b);
			nvp_i2c_write(dec, 0x08, 0x01);
			nvp_i2c_write(dec, 0x0f, 0x32);   //set I2S right sequence
		}

		nvp_i2c_write(dec, 0x13, 0x80|(samplerate<<3)|(bits<<2));	// PB I2C 8k 16bit : master
//		nvp_i2c_write(dec, 0x13, 0x00|(samplerate<<3)|(bits<<2));	// PB I2C 8k 16bit : slave
		nvp_i2c_write(dec, 0x23, 0x10);  // Audio playback out
//		nvp_i2c_write(dec, 0x23, 0x18);  // Audio mix out
	}
	else
	{
		nvp_i2c_write(dec, 0x06, 0x19);
		nvp_i2c_write(dec, 0x07, 0x00|(samplerate<<3)|(bits<<2));	//Rec I2C 16K 16bit : slave
		nvp_i2c_write(dec, 0x13, 0x00|(samplerate<<3)|(bits<<2));	// PB I2C 8k 16bit : slave
	}
	nvp_i2c_write(dec, 0x01, 0x0f);  // ch1 Audio input gain init
	nvp_i2c_write(dec, 0x02, 0x0f);
	nvp_i2c_write(dec, 0x03, 0x0f);
	nvp_i2c_write(dec, 0x04, 0x0f);
	nvp_i2c_write(dec, 0x05, 0x0f); //mic gain
	nvp_i2c_write(dec, 0x40, 0x0f);  //ch5
	nvp_i2c_write(dec, 0x41, 0x0f);
	nvp_i2c_write(dec, 0x42, 0x0f);
	nvp_i2c_write(dec, 0x43, 0x0f);
	nvp_i2c_write(dec, 0x22, 0x03);  //aogain

	nvp_i2c_write(dec, 0x24, 0x14); //set mic_1's data to i2s_sp left channel
	nvp_i2c_write(dec, 0x25, 0x15); //set mic_2's data to i2s_sp right channel

	smode = nvp_i2c_read(dec, 0x13);
	if(smode&0x80)
		nvp_i2c_write(dec, 0xd5, 0x00);
	else
		nvp_i2c_write(dec, 0xd5, 0x01);
	//printk("nvp6124 audio init\r\n");
}
