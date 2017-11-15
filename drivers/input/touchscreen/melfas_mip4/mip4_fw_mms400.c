/*
 * MELFAS MIP4 Touchscreen
 *
 * Copyright (C) 2016 MELFAS Inc.
 *
 *
 * mip4_fw_mms400.c : Firmware update functions for MMS427/438/438S/449/458
 *
 * Version : 2016.12.09
 */

#include "mip4_ts.h"

/* Firmware Info */
#if (CHIP_MODEL == CHIP_MMS427)
#define FW_CHIP_CODE	"M4H2"
#endif
#if (CHIP_MODEL == CHIP_MMS438)
#define FW_CHIP_CODE	"M4H0"
#endif
#if (CHIP_MODEL == CHIP_MMS438S)
#define FW_CHIP_CODE	"M4HS"
#endif
#if (CHIP_MODEL == CHIP_MMS449)
#define FW_CHIP_CODE	"M4HP"
#endif
#if (CHIP_MODEL == CHIP_MMS458)
#define FW_CHIP_CODE	"M4HP"
#endif

#define FW_TYPE_TAIL

/* ISC Info */
#define ISC_PAGE_SIZE			128

/* ISC Command */
#define ISC_CMD_ERASE_MASS		{0xFB, 0x4A, 0x00, 0x15, 0x00, 0x00}
#define ISC_CMD_READ_PAGE		{0xFB, 0x4A, 0x00, 0xC2, 0x00, 0x00}
#define ISC_CMD_WRITE_PAGE		{0xFB, 0x4A, 0x00, 0xA5, 0x00, 0x00}
#define ISC_CMD_READ_STATUS	{0xFB, 0x4A, 0x36, 0xC2, 0x00, 0x00}
#define ISC_CMD_EXIT				{0xFB, 0x4A, 0x00, 0x66, 0x00, 0x00}

/* ISC Status */
#define ISC_STATUS_BUSY			0x96
#define ISC_STATUS_DONE			0xAD

/*
* Firmware binary tail info
*/
struct melfas_bin_tail {
	u8 tail_mark[4];
	char chip_name[4];
	u32 bin_start_addr;
	u32 bin_length;

	u16 ver_boot;
	u16 ver_core;
	u16 ver_app;
	u16 ver_param;
	u8 boot_start;
	u8 boot_end;
	u8 core_start;
	u8 core_end;
	u8 app_start;
	u8 app_end;
	u8 param_start;
	u8 param_end;

	u8 checksum_type;
	u8 hw_category;
	u16 param_id;
	u32 param_length;
	u32 build_date;
	u32 build_time;

	u32 reserved1;
	u32 reserved2;
	u16 reserved3;
	u16 tail_size;
	u32 crc;
} __attribute__ ((packed));

#define MELFAS_BIN_TAIL_MARK	{0x4D, 0x42, 0x54, 0x01}	/* M B T 0x01 */
#define MELFAS_BIN_TAIL_SIZE		64

/*
* Read ISC status
*/
static int isc_read_status(struct mip4_ts_info *info)
{
	struct i2c_client *client = info->client;
	u8 cmd[6] = ISC_CMD_READ_STATUS;
	u8 result = 0;
	int cnt = 200;
	int ret = 0;
	struct i2c_msg msg[2] = {
		{
			.addr = client->addr,
			.flags = 0,
			.buf = cmd,
			.len = 6,
		}, {
			.addr = client->addr,
			.flags = I2C_M_RD,
			.buf = &result,
			.len = 1,
		},
	};

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	do {
		if (i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg))
				!= ARRAY_SIZE(msg)) {
			dev_err(&info->client->dev, "%s [ERROR] i2c_transfer\n", __func__);
			ret = -1;
			goto ERROR;
		}

		if (result == ISC_STATUS_DONE) {
			ret = 0;
			break;
		} else if (result == ISC_STATUS_BUSY) {
			ret = -1;
			usleep_range(50, 200);
		} else {
			dev_err(&info->client->dev, "%s [ERROR] wrong value [0x%02X]\n",
				   	__func__, result);
			ret = -1;
			usleep_range(50, 200);
		}
	} while (--cnt);

	if (!cnt) {
		dev_err(&info->client->dev, "%s [ERROR] count overflow - cnt [%d] \
				status [0x%02X]\n", __func__, cnt, result);
		goto ERROR;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return ret;

ERROR:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return ret;
}

/*
* Command : Erase Mass
*/
static int isc_erase_mass(struct mip4_ts_info *info)
{
	u8 write_buf[6] = ISC_CMD_ERASE_MASS;
	struct i2c_msg msg[1] = {
		{
			.addr = info->client->addr,
			.flags = 0,
			.buf = write_buf,
			.len = 6,
		},
	};

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	if (i2c_transfer(info->client->adapter, msg, ARRAY_SIZE(msg))
			!= ARRAY_SIZE(msg)) {
		dev_err(&info->client->dev, "%s [ERROR] i2c_transfer\n", __func__);
		goto ERROR;
	}

	if (isc_read_status(info) != 0) {
		goto ERROR;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

ERROR:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return -1;
}

/*
* Command : Read Page
*/
static int isc_read_page(struct mip4_ts_info *info, int offset, u8 *data)
{
	u8 write_buf[6] = ISC_CMD_READ_PAGE;
	struct i2c_msg msg[2] = {
		{
			.addr = info->client->addr,
			.flags = 0,
			.buf = write_buf,
			.len = 6,
		}, {
			.addr = info->client->addr,
			.flags = I2C_M_RD,
			.buf = data,
			.len = ISC_PAGE_SIZE,
		},
	};

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	write_buf[4] = (u8)((offset >> 8) & 0xFF);
	write_buf[5] = (u8)(offset & 0xFF);
	if (i2c_transfer(info->client->adapter, msg, ARRAY_SIZE(msg))
			!= ARRAY_SIZE(msg)) {
		dev_err(&info->client->dev, "%s [ERROR] i2c_transfer\n", __func__);
		goto ERROR;
	}

	dev_dbg(&info->client->dev, "%s [DONE] - Offset [0x%04X]\n",
			__func__, offset);
	return 0;

ERROR:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return -1;
}

/*
* Command : Write Page
*/
static int isc_write_page(struct mip4_ts_info *info, int offset,
		const u8 *data, int length)
{
	u8 write_buf[6 + ISC_PAGE_SIZE] = ISC_CMD_WRITE_PAGE;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	if (length > ISC_PAGE_SIZE) {
		dev_err(&info->client->dev, "%s [ERROR] page length overflow\n",
				__func__);
		goto ERROR;
	}

	write_buf[4] = (u8)((offset >> 8) & 0xFF);
	write_buf[5] = (u8)(offset & 0xFF);

	memcpy(&write_buf[6], data, length);

	if (i2c_master_send(info->client, write_buf, (length + 6))
			!= (length + 6)) {
		dev_err(&info->client->dev, "%s [ERROR] i2c_master_send\n", __func__);
		goto ERROR;
	}

	if (isc_read_status(info) != 0) {
		goto ERROR;
	}

	dev_dbg(&info->client->dev, "%s [DONE] - Offset[0x%04X] Length[%d]\n",
		   	__func__, offset, length);
	return 0;

ERROR:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return -1;
}

/*
* Command : Exit ISC
*/
static int isc_exit(struct mip4_ts_info *info)
{
	u8 write_buf[6] = ISC_CMD_EXIT;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	if (i2c_master_send(info->client, write_buf, 6 ) != 6) {
		dev_err(&info->client->dev, "%s [ERROR] i2c_master_send\n", __func__);
		goto ERROR;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

ERROR:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return -1;
}

/*
* Flash chip firmware (main function)
*/
int mip4_ts_flash_fw(struct mip4_ts_info *info, const u8 *fw_data,
		size_t fw_size, bool force, bool section)
{
	struct i2c_client *client = info->client;
	struct melfas_bin_tail *bin_info;
	int ret = 0;
	int retry = 3;
	u8 rbuf[ISC_PAGE_SIZE];
	int offset = 0;
	int offset_start = 0;
	int bin_size = 0;
	u8 *bin_data;
	u16 tail_size = 0;
	u8 tail_mark[4] = MELFAS_BIN_TAIL_MARK;
	u16 ver_chip[FW_MAX_SECT_NUM];

	dev_dbg(&client->dev, "%s [START]\n", __func__);

	/* Check tail size */
	tail_size = (fw_data[fw_size - 5] << 8) | fw_data[fw_size - 6];
	if (tail_size != MELFAS_BIN_TAIL_SIZE) {
		dev_err(&client->dev, "%s [ERROR] wrong tail size [%d]\n",
				__func__, tail_size);
		ret = fw_err_file_type;
		goto ERROR_FILE;
	}

	/* Check bin format */
	if (memcmp(&fw_data[fw_size - tail_size], tail_mark, 4)) {
		dev_err(&client->dev, "%s [ERROR] wrong tail mark\n", __func__);
		ret = fw_err_file_type;
		goto ERROR_FILE;
	}

	/* Read bin info */
	bin_info = (struct melfas_bin_tail *)&fw_data[fw_size - tail_size];
	dev_dbg(&client->dev, "%s - bin_info : bin_len[%d] hw_cat[0x%2X] \
			date[%4X] time[%4X] tail_size[%d]\n", __func__,
			bin_info->bin_length, bin_info->hw_category, bin_info->build_date,
			bin_info->build_time, bin_info->tail_size);

#if FW_UPDATE_DEBUG
	print_hex_dump(KERN_ERR, MIP4_TS_DEVICE_NAME " Bin Info : ",
			DUMP_PREFIX_OFFSET, 16, 1, bin_info, tail_size, false);
#endif

	/* Check chip code */
	if (memcmp(bin_info->chip_name, FW_CHIP_CODE, 4)) {
		dev_err(&client->dev, "%s [ERROR] F/W file is not for %s\n",
			   	__func__, CHIP_NAME);
		ret = fw_err_file_type;
		goto ERROR_FILE;
	}

	/* Check F/W version */
	dev_info(&client->dev, "%s - F/W file version [0x%04X 0x%04X \
			0x%04X 0x%04X]\n", __func__, bin_info->ver_boot,
			bin_info->ver_core, bin_info->ver_app, bin_info->ver_param);

	if (force == true) {
		/* Force update */
		dev_info(&client->dev, "%s - Skip chip firmware version check\n",
			   	__func__);
	} else {
		/* Read firmware version from chip */
		while (retry--) {
			if (!mip4_ts_get_fw_version_u16(info, ver_chip)) {
				break;
			} else {
				mip4_ts_reset(info);
			}
		}

		if (retry < 0) {
			dev_err(&client->dev, "%s [ERROR] Unknown chip firmware version\n",
				__func__);
		} else {
			dev_info(&client->dev, "%s - Chip firmware version [0x%04X 0x%04X \
					0x%04X 0x%04X]\n", __func__, ver_chip[0], ver_chip[1],
					ver_chip[2], ver_chip[3]);

			if ((ver_chip[0] == bin_info->ver_boot) &&
					(ver_chip[1] == bin_info->ver_core) &&
					(ver_chip[2] == bin_info->ver_app) &&
					(ver_chip[3] == bin_info->ver_param)) {
				dev_info(&client->dev, "%s - Chip firmware is already \
						up-to-date\n", __func__);
				ret = fw_err_uptodate;
				goto UPTODATE;
			}
		}
	}

	dev_dbg(&client->dev, "%s - Start offset[0x%04X]\n", __func__,
			offset_start);

	/* Read bin data */
	bin_size = bin_info->bin_length;
	bin_data = kzalloc(sizeof(u8) * (bin_size), GFP_KERNEL);
	memcpy(bin_data, fw_data, bin_size);

	/* Erase */
	dev_dbg(&client->dev, "%s - Erase\n", __func__);
	ret = isc_erase_mass(info);
	if (ret != 0) {
		dev_err(&client->dev,"%s [ERROR] isc_erase_mass\n", __func__);
		ret = fw_err_download;
		goto ERROR_UPDATE;
	}

	/* Download & Verify */
	dev_dbg(&client->dev, "%s - Download & Verify\n", __func__);
	offset = bin_size - ISC_PAGE_SIZE;
	while (offset >= offset_start) {
		/* Write page */
		if (isc_write_page(info, offset, &bin_data[offset], ISC_PAGE_SIZE)) {
			dev_err(&client->dev, "%s [ERROR] isc_write_page : \
					offset[0x%04X]\n", __func__, offset);
			ret = fw_err_download;
			goto ERROR_UPDATE;
		}
		dev_dbg(&client->dev, "%s - isc_write_page : offset[0x%04X]\n",
			   	__func__, offset);

		/* Verify page */
		if (isc_read_page(info, offset, rbuf)) {
			dev_err(&client->dev, "%s [ERROR] isc_read_page : offset[0x%04X]\n",
				   	__func__, offset);
			ret = fw_err_download;
			goto ERROR_UPDATE;
		}
		dev_dbg(&client->dev, "%s - isc_read_page : offset[0x%04X]\n", __func__,
			   offset);

#if FW_UPDATE_DEBUG
		print_hex_dump(KERN_ERR, MIP4_TS_DEVICE_NAME " F/W File : ",
			DUMP_PREFIX_OFFSET, 16, 1, &bin_data[offset], ISC_PAGE_SIZE, false);
		print_hex_dump(KERN_ERR, MIP4_TS_DEVICE_NAME " F/W Chip : ",
			DUMP_PREFIX_OFFSET, 16, 1, rbuf, ISC_PAGE_SIZE, false);
#endif

		if (memcmp(rbuf, &bin_data[offset], ISC_PAGE_SIZE)) {
			dev_err(&client->dev, "%s [ERROR] Verify failed : offset[0x%04X]\n",
				   __func__, offset);
			ret = fw_err_download;
			goto ERROR_UPDATE;
		}

		/* Next offset */
		offset -= ISC_PAGE_SIZE;
	}

	/* Exit ISC mode */
	dev_dbg(&client->dev, "%s - Exit\n", __func__);
	isc_exit(info);

	/* Reset chip */
	mip4_ts_reset(info);

	/* Check chip firmware version */
	if (mip4_ts_get_fw_version_u16(info, ver_chip)) {
		dev_err(&client->dev, "%s [ERROR] Unknown chip firmware version\n",
				__func__);
		ret = fw_err_download;
		goto ERROR_UPDATE;
	} else {
		if ((ver_chip[0] == bin_info->ver_boot) &&
				(ver_chip[1] == bin_info->ver_core) &&
				(ver_chip[2] == bin_info->ver_app) &&
				(ver_chip[3] == bin_info->ver_param)) {
			dev_dbg(&client->dev, "%s - Version check OK\n", __func__);
		} else {
			dev_err(&client->dev, "%s [ERROR] Version mismatch after flash. \
					Chip[0x%04X 0x%04X 0x%04X 0x%04X] \
					File[0x%04X 0x%04X 0x%04X 0x%04X]\n", __func__,
					ver_chip[0], ver_chip[1], ver_chip[2], ver_chip[3],
					bin_info->ver_boot, bin_info->ver_core, bin_info->ver_app,
					bin_info->ver_param);
			ret = fw_err_download;
			goto ERROR_UPDATE;
		}
	}

	kfree(bin_data);

UPTODATE:
	dev_dbg(&client->dev, "%s [DONE]\n", __func__);
	goto EXIT;

ERROR_UPDATE:
	kfree(bin_data);

ERROR_FILE:
	dev_err(&client->dev, "%s [ERROR]\n", __func__);

EXIT:
	return ret;
}

/*
* Get version of F/W bin file
*/
int mip4_ts_bin_fw_version(struct mip4_ts_info *info, const u8 *fw_data,
		size_t fw_size, u8 *ver_buf)
{
	struct melfas_bin_tail *bin_info;
	u16 tail_size = 0;
	u8 tail_mark[4] = MELFAS_BIN_TAIL_MARK;

	dev_dbg(&info->client->dev,"%s [START]\n", __func__);

	/* Check tail size */
	tail_size = (fw_data[fw_size - 5] << 8) | fw_data[fw_size - 6];
	if (tail_size != MELFAS_BIN_TAIL_SIZE) {
		dev_err(&info->client->dev, "%s [ERROR] wrong tail size [%d]\n",
				__func__, tail_size);
		goto ERROR;
	}

	/* Check bin format */
	if (memcmp(&fw_data[fw_size - tail_size], tail_mark, 4)) {
		dev_err(&info->client->dev, "%s [ERROR] wrong tail mark\n", __func__);
		goto ERROR;
	}

	/* Read bin info */
	bin_info = (struct melfas_bin_tail *)&fw_data[fw_size - tail_size];

	/* F/W version */
	ver_buf[0] = (bin_info->ver_boot >> 8) & 0xFF;
	ver_buf[1] = (bin_info->ver_boot) & 0xFF;
	ver_buf[2] = (bin_info->ver_core >> 8) & 0xFF;
	ver_buf[3] = (bin_info->ver_core) & 0xFF;
	ver_buf[4] = (bin_info->ver_app >> 8) & 0xFF;
	ver_buf[5] = (bin_info->ver_app) & 0xFF;
	ver_buf[6] = (bin_info->ver_param >> 8) & 0xFF;
	ver_buf[7] = (bin_info->ver_param) & 0xFF;

	dev_dbg(&info->client->dev,"%s [DONE]\n", __func__);
	return 0;

ERROR:
	dev_err(&info->client->dev,"%s [ERROR]\n", __func__);
	return 1;
}

