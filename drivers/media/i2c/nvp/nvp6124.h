#ifndef _NVP6124_H_
#define _NVP6124_H_

#include <linux/ioctl.h>

//#define ONLY_720H

#define NVP6124	0x60
#define NVP6124_R0_ID 	0x84
#define NVP6114A_R0_ID 	0x85
#define NVP6124B_R0_ID  0x86

//FIXME HI3520 Register
#define VIU_CH_CTRL					0x08
#define VIU_ANC0_START				0x9c
#define VIU_ANC0_SIZE				0xa0
#define VIU_ANC1_START				0xa4
#define VIU_ANC1_SIZE				0xa8
#define VIU_BLANK_DATA_ADDR			0xac

#define VDEC_NTSC	        1
#define VDEC_PAL	        2
#define VDEC_AUTO	        3
#define VDEC_INVALID_MODE	4
#define	VIDEO_NTSC_960_4CH	6
#define	VIDEO_PAL_960_4CH	7
#define	VIDEO_NTSC_960_720P	8
#define	VIDEO_PAL_960_720P	9
#define	VIDEO_NTSC_720P_960	10
#define	VIDEO_PAL_720P_960	11
#define	VIDEO_NTSC_720P_2CH	36
#define	VIDEO_PAL_720P_2CH	37

#define NVP1114A_IOC_MAGIC            't'
#define NVP1114ACMD_READ_REG		    0x01
#define NVP1114ACMD_WRITE_REG	        0x02

#define NVP1114A_GET_VIDEO_LOSS			0x04
#define NVP1114A_SET_IMAGE_ADJUST	    0x05
#define NVP1114A_GET_IMAGE_ADJUST	    0x06
#define NVP1114A_SET_VIDEO_NORM	    	0x07
#define NVP1114A_GET_VIDEO_NORM	    	0x08
#define NVP1114A_SET_WORK_MODE	   		0x09

#define NVP1114A_SET_SAMPLE_RATE	    0x0a
#define NVP1114A_SET_AUDIO_PLAYBACK   	0x0b
#define NVP1114A_SET_AUDIO_DA_MUTE    	0x0c
#define NVP1114A_SET_AUDIO_DA_VOLUME  	0x0d

#define NVP1114A_SET_BRIGHTNESS	    	0x0e
#define NVP1114A_SET_CONTRAST   		0x0f
#define NVP1114A_SET_HUE    			0x10
#define NVP1114A_SET_SATURATION  		0x11
#define NVP1114A_SET_SHARPNESS  		0x12
#define NVP1114A_SET_AUDIO_MUTE	        0x13

#define IOC_AUDIO_SET_LIVE_CH			0x14
#define IOC_AUDIO_SET_PB_CH				0x15
#define NVP1108_GET_MOTION				0x16
#define NVP1108_SET_MOTION				0x17
#define NVP1108_SET_MOTION_SENS			0x18
#define NVP1108_SET_MOTION_EN			0x19
#define NVP1108_SET_MOTION_TRACE		0x1A


//Hwarang
#define IOC_VDEC_SET_VIDEO_MODE			0x07
#define IOC_VDEC_GET_INPUT_VIDEO_FMT	0x08
#define IOC_VDEC_GET_VIDEO_LOSS     	0x09
#define IOC_VDEC_SET_SYNC		     	0x0A
#define IOC_VDEC_SET_EQUALIZER			0x0B
#define IOC_VDEC_SET_BRIGHTNESS	    	0x0e
#define IOC_VDEC_SET_CONTRAST   		0x0f
#define IOC_VDEC_SET_HUE    			0x10
#define IOC_VDEC_SET_SATURATION  		0x11
#define IOC_VDEC_SET_SHARPNESS  		0x12
#define IOC_VDEC_PTZ_CHANNEL_SEL		0x20
#define IOC_VDEC_PTZ_PELCO_INIT			0x21
#define IOC_VDEC_PTZ_PELCO_RESET		0x22
#define IOC_VDEC_PTZ_PELCO_SET			0x23
#define IOC_VDEC_PTZ_PELCO_UP			0x24
#define IOC_VDEC_PTZ_PELCO_DOWN			0x25
#define IOC_VDEC_PTZ_PELCO_LEFT			0x26
#define IOC_VDEC_PTZ_PELCO_RIGHT		0x27
#define IOC_VDEC_PTZ_PELCO_OSD			0x28
#define IOC_VDEC_PTZ_PELCO_IRIS_OPEN	0x29
#define IOC_VDEC_PTZ_PELCO_IRIS_CLOSE	0x2a
#define IOC_VDEC_PTZ_PELCO_FOCUS_NEAR	0x2b
#define IOC_VDEC_PTZ_PELCO_FOCUS_FAR	0x2c
#define IOC_VDEC_PTZ_PELCO_ZOOM_WIDE	0x2d
#define IOC_VDEC_PTZ_PELCO_ZOOM_TELE	0x2e

#define IOC_VDEC_INIT_MOTION			0x40
#define IOC_VDEC_ENABLE_MOTION			0x41
#define IOC_VDEC_DISABLE_MOTION			0x42
#define IOC_VDEC_SET_MOTION_AREA		0x43
#define IOC_VDEC_GET_MOTION_INFO		0x44
#define IOC_VDEC_SET_MOTION_DISPLAY		0x45
#define IOC_VDEC_SET_MOTION_SENS		0x46

#define IOC_VDEC_ENABLE_LOW_RES			0x50
#define IOC_VDEC_DISABLE_LOW_RES		0x51

#define IOC_VDEC_ENABLE_BW				0x60
#define IOC_VDEC_DISABLE_BW				0x61
#define IOC_VDEC_READ_BALCK_COUNT		0x62
#define IOC_VDEC_READ_WHITE_COUNT		0x63
#define IOC_VDEC_4CH_VIDEO_SEQUENCE		0x64


typedef struct _nvp6124_video_mode
{
    unsigned int chip;
    unsigned int mode;
	unsigned char vformat[16];
	unsigned char chmode[16];
}nvp6124_video_mode;

typedef struct _nvp6124_input_videofmt
{
    //unsigned int ch;
    unsigned int inputvideofmt[16];
	unsigned int getvideofmt[16];
	unsigned int geteqstage[16];
	unsigned int getacpdata[16];
}nvp6124_input_videofmt;

typedef struct _nvp6124_video_loss
{
    unsigned int chip;
    unsigned int ch;
    unsigned int is_lost;
}nvp6124_video_loss;

typedef struct _nvp6124_video_adjust
{
    unsigned int ch;
	unsigned int value;
}nvp6124_video_adjust;

typedef struct _NVP1114A_audio_output
{
	unsigned char PlaybackOrLoop;   /*0:playback; 1:loop*/
	unsigned char channel;          /*[0, 15]*/
	unsigned char reserve[6];
} nvp1114a_audio_output;

typedef struct _nvp6124_clock_mode
{
    unsigned int chip;
    unsigned int mode;
}nvp6124_clock_mode;


typedef struct _nvp6124_motion_area
{
    unsigned char ch;
    int m_info[12];
}nvp6124_motion_area;

typedef struct _nvp6124_audio_playback
{
    unsigned int chip;
    unsigned int ch;
}nvp6124_audio_playback;

typedef struct _nvp6124_audio_da_mute
{
    unsigned int chip;
}nvp6124_audio_da_mute;

typedef struct _nvp6124_audio_da_volume
{
    unsigned int chip;
    unsigned int volume;
}nvp6124_audio_da_volume;


#endif
