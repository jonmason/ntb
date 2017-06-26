/*********************************************************************
 * Project Name : NVP1918C
 *
 * Copyright 2011 by NEXTCHIP Co., Ltd.
 * All rights reserved.
 *
 *********************************************************************/
#ifndef _COAX_NVP6124_H_
#define _COAX_NVP6124_H_
//#include "mytype.h"

//FPGA Define
#define PEL_D0	0xA0
#define PEL_D1	0xA1
#define PEL_D2	0xA2
#define PEL_D3	0xA3
#define PEL_OUT	0x8F

#define PEL_CTEN	0x0C
#define PEL_TXST1	0x07
#define EVEN_LINE	0x2F

#define COAX_CH_SEL	0x8B
#define COAX_BAUD	0x00
#define COAX_RBAUD	0x01
#define BL_TXST1	0x03
#define BL_RXST1	0x05
#define BL_HSP01	0x0D
#define BL_HSP02	0x0E
#define PACKET_MODE	0x0B
#define TX_START	0x09
#define TX_BYTE_LEN	0x0A

#define AHD2_PEL_D0	0x20
#define AHD2_FHD_D0	0x10
#define AHD2_PEL_OUT	0x0C
#define AHD2_PEL_BAUD	0x02
#define AHD2_PEL_LINE	0x07
#define AHD2_PEL_SYNC	0x0D
#define AHD2_PEL_EVEN	0x2F
#define AHD2_FHD_BAUD	0x00
#define AHD2_FHD_LINE	0x03
#define AHD2_FHD_LINES	0x05
#define AHD2_FHD_BYTE	0x0A
#define AHD2_FHD_MODE	0x0B
#define AHD2_FHD_OUT	0x09

#define SAM_D0	0x10
#define SAM_OUT	0x09

#define ACP_CAM_STAT	0x55
#define ACP_REG_WR		0x60
#define ACP_REG_RD		0x61
#define ACP_MODE_ID		0x60

#define ACP_RX_D0		0x78

extern void nvp6124_pelco_coax_mode( void );
extern unsigned char nvp6124_pelco_command(unsigned char pel_ch, unsigned char command);
extern unsigned char samsung_coax_command(unsigned char command);
extern void init_acp(void);
extern void init_acp_camera_status(void);
extern void init_acp_reg_wr(void);
extern void init_acp_reg_rd(void);
extern void acp_reg_rx_clear(void);
extern void set_acp_reg_wr(unsigned char bank, unsigned char addr, unsigned char data);
extern void get_acp_reg_rd(unsigned char bank, unsigned char addr);
extern void acp_read(nvp6124_input_videofmt *pvideoacp);

#endif
