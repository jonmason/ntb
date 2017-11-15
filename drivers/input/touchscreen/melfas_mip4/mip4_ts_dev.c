/*
 * MELFAS MIP4 Touchscreen
 *
 * Copyright (C) 2000-2017 MELFAS Inc.
 *
 *
 * mip4_ts_dev.c : Development functions (Optional)
 *
 * Version : 2017.07.14
 */

#include "mip4_ts.h"

#if USE_DEV
/*
* Dev node output to user
*/
static ssize_t mip4_ts_dev_fs_read(struct file *fp, char *rbuf,
		size_t cnt, loff_t *fpos)
{
	struct mip4_ts_info *info = fp->private_data;
	int ret = 0;

	ret = copy_to_user(rbuf, info->dev_fs_buf, cnt);

	return ret;
}

/*
* Dev node input from user
*/
static ssize_t mip4_ts_dev_fs_write(struct file *fp, const char *wbuf,
		size_t cnt, loff_t *fpos)
{
	struct mip4_ts_info *info = fp->private_data;
	u8 *buf;
	int ret = 0;
	int cmd = 0;

	buf = kzalloc(cnt + 1, GFP_KERNEL);

	if ((buf == NULL) || copy_from_user(buf, wbuf, cnt)) {
		dev_err(&info->client->dev, "%s [ERROR] copy_from_user\n", __func__);
		ret = -EIO;
		goto exit;
	}

	cmd = buf[cnt - 1];

	if (cmd == 1) {
		if (mip4_ts_i2c_read(info, buf, (cnt - 2),
				info->dev_fs_buf, buf[cnt - 2])) {
			dev_err(&info->client->dev,
				"%s [ERROR] mip4_ts_i2c_read\n", __func__);
		}
	} else if (cmd == 2) {
		if (mip4_ts_i2c_write(info, buf, (cnt - 1))) {
			dev_err(&info->client->dev,
				"%s [ERROR] mip4_ts_i2c_write\n", __func__);
		}
	} else {
		goto exit;
	}

exit:
	kfree(buf);

	return ret;
}

/*
* Open dev node
*/
static int mip4_ts_dev_fs_open(struct inode *node, struct file *fp)
{
	struct mip4_ts_info *info = container_of(node->i_cdev,
		struct mip4_ts_info, cdev);


	fp->private_data = info;
	info->dev_fs_buf = kzalloc(1024 * 4, GFP_KERNEL);

	return 0;
}

/*
* Close dev node
*/
static int mip4_ts_dev_fs_release(struct inode *node, struct file *fp)
{
	struct mip4_ts_info *info = fp->private_data;

	kfree(info->dev_fs_buf);

	return 0;
}

/*
* Dev node info
*/
static const struct file_operations mip4_ts_dev_fops = {
	.owner = THIS_MODULE,
	.open = mip4_ts_dev_fs_open,
	.release = mip4_ts_dev_fs_release,
	.read = mip4_ts_dev_fs_read,
	.write = mip4_ts_dev_fs_write,
};

/*
* Create dev node
*/
int mip4_ts_dev_create(struct mip4_ts_info *info)
{
	struct i2c_client *client = info->client;
	int ret = 0;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	if (alloc_chrdev_region(&info->mip4_ts_dev, 0, 1, MIP4_TS_DEVICE_NAME)) {
		dev_err(&client->dev, "%s [ERROR] alloc_chrdev_region\n", __func__);
		ret = -ENOMEM;
		goto error;
	}

	cdev_init(&info->cdev, &mip4_ts_dev_fops);
	info->cdev.owner = THIS_MODULE;

	if (cdev_add(&info->cdev, info->mip4_ts_dev, 1)) {
		dev_err(&client->dev, "%s [ERROR] cdev_add\n", __func__);
		ret = -EIO;
		goto error;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return 0;
}
#endif /* USE_DEV */

#if (USE_SYS || USE_CMD)
/*
* Process table data
*/
static int mip4_ts_proc_table_data(struct mip4_ts_info *info,
		u8 data_type_size, u8 data_type_sign, u8 buf_addr_h,
		u8 buf_addr_l, u8 row_num, u8 col_num, u8 buf_col_num, u8 rotate)
{
	char data[16];
	int i_col, i_row, i_x, i_y;
	int max_x = 0;
	int max_y = 0;
	int sValue = 0;
	unsigned int uValue = 0;
	int value = 0;
	int size = 0;
	u8 wbuf[8];
	u8 rbuf[512];
	unsigned int buf_addr;
	int offset;

	memset(data, 0, 10);

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	/* set axis */
	if (rotate == 0) {
		max_x = col_num;
		max_y = row_num;
	} else if (rotate == 1) {
		max_x = row_num;
		max_y = col_num;
	} else {
		dev_err(&info->client->dev,
			"%s [ERROR] rotate[%d]\n", __func__, rotate);
		goto error;
	}

	/* get table data */
	for (i_row = 0; i_row < row_num; i_row++) {
		/* get line data */
		offset = buf_col_num * data_type_size;
		size = col_num * data_type_size;
		buf_addr = (buf_addr_h << 8) | buf_addr_l | (offset * i_row);

		wbuf[0] = (buf_addr >> 8) & 0xFF;
		wbuf[1] = buf_addr & 0xFF;
		if (mip4_ts_i2c_read(info, wbuf, 2, rbuf, size)) {
			dev_err(&info->client->dev,
				"%s [ERROR] Read data buffer\n", __func__);
			goto error;
		}

		/* save data */
		for (i_col = 0; i_col < col_num; i_col++) {
			if (data_type_sign == 0) {
				/* unsigned */
				switch (data_type_size) {
				case 1:
					uValue = (u8)rbuf[i_col];
					break;
				case 2:
					uValue = (u16)(rbuf[data_type_size * i_col]
							| (rbuf[data_type_size * i_col + 1] << 8));
					break;
				case 4:
					uValue = (u32)(rbuf[data_type_size * i_col]
							| (rbuf[data_type_size * i_col + 1] << 8)
							| (rbuf[data_type_size * i_col + 2] << 16)
							| (rbuf[data_type_size * i_col + 3] << 24));
					break;
				default:
					dev_err(&info->client->dev,
							"%s [ERROR] data_type_size[%d]\n",
							__func__, data_type_size);
					goto error;
				}
				value = (int)uValue;
			} else {
				/* signed */
				switch (data_type_size) {
				case 1:
					sValue = (s8)rbuf[i_col];
					break;
				case 2:
					sValue = (s16)(rbuf[data_type_size * i_col]
							| (rbuf[data_type_size * i_col + 1] << 8));
					break;
				case 4:
					sValue = (s32)(rbuf[data_type_size * i_col]
							| (rbuf[data_type_size * i_col + 1] << 8)
							| (rbuf[data_type_size * i_col + 2] << 16)
							| (rbuf[data_type_size * i_col + 3] << 24));
					break;
				default:
					dev_err(&info->client->dev,
							"%s [ERROR] data_type_size[%d]\n",
							__func__, data_type_size);
					goto error;
				}
				value = (int)sValue;
			}

			switch (rotate) {
			case 0:
				info->image_buf[i_row * col_num + i_col] = value;
				break;
			case 1:
				info->image_buf[i_col * row_num + (row_num - 1 - i_row)]=value;
				break;
			default:
				dev_err(&info->client->dev,
						"%s [ERROR] rotate[%d]\n", __func__, rotate);
				goto error;
			}
		}
	}

	/* print table header */
	printk("    ");
	snprintf(data, sizeof(data), "    ");
	strlcat(info->print_buf, data, PAGE_SIZE);
	memset(data, 0, 10);

	switch (data_type_size) {
	case 1:
		for (i_x = 0; i_x < max_x; i_x++) {
			printk("[%2d]", i_x);
			snprintf(data, sizeof(data), "[%2d]", i_x);
			strlcat(info->print_buf, data, PAGE_SIZE);
			memset(data, 0, 10);
		}
		break;
	case 2:
		for (i_x = 0; i_x < max_x; i_x++) {
			printk("[%4d]", i_x);
			snprintf(data, sizeof(data), "[%4d]", i_x);
			strlcat(info->print_buf, data, PAGE_SIZE);
			memset(data, 0, 10);
		}
		break;
	case 4:
		for (i_x = 0; i_x < max_x; i_x++) {
			printk("[%5d]", i_x);
			snprintf(data, sizeof(data), "[%5d]", i_x);
			strlcat(info->print_buf, data, PAGE_SIZE);
			memset(data, 0, 10);
		}
		break;
	default:
		dev_err(&info->client->dev,
				"%s [ERROR] data_type_size[%d]\n", __func__, data_type_size);
		goto error;
	}

	printk("\n");
	snprintf(data, sizeof(data), "\n");
	strlcat(info->print_buf, data, PAGE_SIZE);
	memset(data, 0, 10);

	/* print table */
	for (i_y = 0; i_y < max_y; i_y++) {
		/* print line header */
		printk("[%2d]", i_y);
		snprintf(data, sizeof(data), "[%2d]", i_y);
		strlcat(info->print_buf, data, PAGE_SIZE);
		memset(data, 0, 10);

		/* print line */
		for (i_x = 0; i_x < max_x; i_x++) {
			switch (data_type_size) {
			case 1:
				printk(" %3d", info->image_buf[i_y * max_x + i_x]);
				snprintf(data, sizeof(data), " %3d",
						info->image_buf[i_y * max_x + i_x]);
				break;
			case 2:
				printk(" %5d", info->image_buf[i_y * max_x + i_x]);
				snprintf(data, sizeof(data), " %5d",
						info->image_buf[i_y * max_x + i_x]);
				break;
			case 4:
				printk(" %6d", info->image_buf[i_y * max_x + i_x]);
				snprintf(data, sizeof(data), " %6d",
						info->image_buf[i_y * max_x + i_x]);
				break;
			default:
				dev_err(&info->client->dev,
						"%s [ERROR] data_type_size[%d]\n",
						__func__, data_type_size);
				goto error;
			}

			strlcat(info->print_buf, data, PAGE_SIZE);
			memset(data, 0, 10);
		}

		printk("\n");
		snprintf(data, sizeof(data), "\n");
		strlcat(info->print_buf, data, PAGE_SIZE);
		memset(data, 0, 10);
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return 1;
}

/*
* Process vector data
*/
static int mip4_ts_proc_vector_data(struct mip4_ts_info *info,
		u8 data_type_size, u8 data_type_sign,
		u8 buf_addr_h, u8 buf_addr_l, u8 key_num, u8 vector_num,
		u16 *vector_id, u16 *vector_elem_num, int table_size)
{
	char data[16];
	int i, i_line, i_vector, i_elem;
	int sValue = 0;
	unsigned int uValue = 0;
	int value = 0;
	int size = 0;
	u8 wbuf[8];
	u8 rbuf[512];
	unsigned int buf_addr;
	int key_exist = 0;
	int total_len = 0;
	int elem_len = 0;
	int vector_total_len = 0;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	memset(data, 0, 10);

	for (i = 0; i < vector_num; i++) {
		vector_total_len += vector_elem_num[i];
	}
	total_len = key_num + vector_total_len;

	for (i = 0; i < vector_num; i++) {
		dev_dbg(&info->client->dev, "%s - vector_elem_num(%d)[%d]\n",
				__func__, i, vector_elem_num[i]);
	}
	dev_dbg(&info->client->dev, "%s - key_num[%d] total_len[%d]\n",
				__func__, key_num, total_len);

	if (key_num > 0) {
		key_exist = 1;
	} else {
		key_exist = 0;
	}

	/* get line data */
	size = (key_num + vector_total_len) * data_type_size;
	buf_addr = (buf_addr_h << 8) | buf_addr_l;

	wbuf[0] = (buf_addr >> 8) & 0xFF;
	wbuf[1] = buf_addr & 0xFF;
	if (mip4_ts_i2c_read(info, wbuf, 2, rbuf, size)) {
		dev_err(&info->client->dev, "%s [ERROR] Read data buffer\n", __func__);
		goto error;
	}

	/* save data */
	for (i = 0; i < total_len; i++) {
		if (data_type_sign == 0) {
			/* unsigned */
			switch (data_type_size) {
			case 1:
				uValue = (u8)rbuf[i];
				break;
			case 2:
				uValue = (u16)(rbuf[data_type_size * i]
						| (rbuf[data_type_size * i + 1] << 8));
				break;
			case 4:
				uValue = (u32)(rbuf[data_type_size * i]
						| (rbuf[data_type_size * i + 1] << 8)
						| (rbuf[data_type_size * i + 2] << 16)
						| (rbuf[data_type_size * i + 3] << 24));
				break;
			default:
				dev_err(&info->client->dev,
					"%s [ERROR] data_type_size[%d]\n",
					__func__, data_type_size);
				goto error;
			}
			value = (int)uValue;
		} else {
			/* signed */
			switch (data_type_size) {
			case 1:
				sValue = (s8)rbuf[i];
				break;
			case 2:
				sValue = (s16)(rbuf[data_type_size * i]
						| (rbuf[data_type_size * i + 1] << 8));
				break;
			case 4:
				sValue = (s32)(rbuf[data_type_size * i]
						| (rbuf[data_type_size * i + 1] << 8)
						| (rbuf[data_type_size * i + 2] << 16)
						| (rbuf[data_type_size * i + 3] << 24));
				break;
			default:
				dev_err(&info->client->dev,
					"%s [ERROR] data_type_size[%d]\n",
						__func__, data_type_size);
				goto error;
			}
			value = (int)sValue;
		}

		info->image_buf[table_size + i] = value;
	}

	/* print header */
	i_vector = 0;
	i_elem = 0;
	for (i_line = 0; i_line < (key_exist + vector_num); i_line++) {
		if ((i_line == 0) && (key_exist == 1)) {
			elem_len = key_num;
			printk("[Key]");
			snprintf(data, sizeof(data), "[Key]");
		} else {
			elem_len = vector_elem_num[i_vector];
			if (elem_len <= 0) {
				continue;
			}
			switch (vector_id[i_vector]) {
			case 1:
				printk("[Screen Rx]");
				snprintf(data, sizeof(data), "[Screen Rx]");
				break;
			case 2:
				printk("[Screen Tx]");
				snprintf(data, sizeof(data), "[Screen Tx]");
				break;
			case 3:
				printk("[Key Rx]");
				snprintf(data, sizeof(data), "[Key Rx]");
				break;
			case 4:
				printk("[Key Tx]");
				snprintf(data, sizeof(data), "[Key Tx]");
				break;
			case 5:
				printk("[Pressure]");
				snprintf(data, sizeof(data), "[Pressure]");
				break;
			default:
				printk("[Vector %d]", i_vector);
				snprintf(data, sizeof(data), "[Vector %d]", i_vector);
				break;
			}
			i_vector++;
		}
		strlcat(info->print_buf, data, PAGE_SIZE);
		memset(data, 0, 10);

		/* print line */
		for (i = i_elem; i < (i_elem + elem_len); i++) {
			switch (data_type_size) {
			case 1:
				printk(" %3d", info->image_buf[table_size + i]);
				snprintf(data, sizeof(data), " %3d",
					info->image_buf[table_size + i]);
				break;
			case 2:
				printk(" %5d", info->image_buf[table_size + i]);
				snprintf(data, sizeof(data), " %5d",
					info->image_buf[table_size + i]);
				break;
			case 4:
				printk(" %6d", info->image_buf[table_size + i]);
				snprintf(data, sizeof(data), " %6d",
					info->image_buf[table_size + i]);
				break;
			default:
				dev_err(&info->client->dev, "%s [ERROR] data_type_size[%d]\n",
					__func__, data_type_size);
				goto error;
			}

			strlcat(info->print_buf, data, PAGE_SIZE);
			memset(data, 0, 10);
		}

		printk("\n");
		snprintf(data, sizeof(data), "\n");
		strlcat(info->print_buf, data, PAGE_SIZE);
		memset(data, 0, 10);

		i_elem += elem_len;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return 1;
}

/*
* Run test
*/
int mip4_ts_run_test(struct mip4_ts_info *info, u8 test_type)
{
	int busy_cnt = 100;
	int wait_cnt = 0;
	int wait_num = 200;
	u8 wbuf[8];
	u8 rbuf[512];
	u8 row_num;
	u8 col_num;
	u8 buffer_col_num;
	u8 rotate;
	u8 key_num;
	u8 data_type;
	u8 data_type_size;
	u8 data_type_sign;
	u8 vector_num = 0;
	u16 vector_id[16];
	u16 vector_elem_num[16];
	u8 buf_addr_h;
	u8 buf_addr_l;
	u16 buf_addr;
	int table_size;
	int i;
	int ret = 0;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);
	dev_dbg(&info->client->dev, "%s - test_type[%d]\n", __func__, test_type);

	while (busy_cnt--) {
		if (info->test_busy == false) {
			break;
		}
		usleep_range(10 * 1000, 10 * 1000);
	}
	mutex_lock(&info->lock);
	disable_irq(info->irq);
	info->test_busy = true;
	mutex_unlock(&info->lock);

	memset(info->print_buf, 0, PAGE_SIZE);

	/* disable touch event */
	wbuf[0] = MIP4_R0_CTRL;
	wbuf[1] = MIP4_R1_CTRL_EVENT_TRIGGER_TYPE;
	wbuf[2] = MIP4_CTRL_TRIGGER_NONE;
	if (mip4_ts_i2c_write(info, wbuf, 3)) {
		dev_err(&info->client->dev, "%s [ERROR] Disable event\n", __func__);
		goto error;
	}

	/* check test type */
	switch (test_type) {
	case MIP4_TEST_TYPE_CM:
		dev_dbg(&info->client->dev, "=== Cm Test ===\n");
		snprintf(info->print_buf, PAGE_SIZE, "\n=== Cm Test ===\n\n");
		break;
	case MIP4_TEST_TYPE_CM_ABS:
		dev_dbg(&info->client->dev, "=== Cm Abs Test ===\n");
		snprintf(info->print_buf, PAGE_SIZE, "\n=== Cm Abs Test ===\n\n");
		break;
	case MIP4_TEST_TYPE_CM_JITTER:
		dev_dbg(&info->client->dev, "=== Cm Jitter Test ===\n");
		snprintf(info->print_buf, PAGE_SIZE, "\n=== Cm Jitter Test ===\n\n");
		break;
	case MIP4_TEST_TYPE_CM_DIFF_HOR:
		dev_dbg(&info->client->dev, "=== Cm Diff (Horizontal) Test ===\n");
		snprintf(info->print_buf, PAGE_SIZE,
			"\n=== Cm Diff (Horizontal) Test ===\n\n");
		break;
	case MIP4_TEST_TYPE_CM_DIFF_VER:
		dev_dbg(&info->client->dev, "=== Cm Diff (Vertical) Test ===\n");
		snprintf(info->print_buf, PAGE_SIZE,
			"\n=== Cm Diff (Vertical) Test ===\n\n");
		break;
	case MIP4_TEST_TYPE_CP:
		dev_dbg(&info->client->dev, "=== Cp Test ===\n");
		snprintf(info->print_buf, PAGE_SIZE,
			"\n=== Cp Test ===\n\n");
		break;
	case MIP4_TEST_TYPE_CP_LOW:
		dev_dbg(&info->client->dev, "=== Cp (Low Freq) Test ===\n");
		snprintf(info->print_buf, PAGE_SIZE,
			"\n=== Cp (Low Freq) Test ===\n\n");
		break;
	case MIP4_TEST_TYPE_CP_HIGH:
		dev_dbg(&info->client->dev, "=== Cp (High Freq) Test ===\n");
		snprintf(info->print_buf, PAGE_SIZE,
			"\n=== Cp (High Freq) Test ===\n\n");
		break;
	case MIP4_TEST_TYPE_CP_JITTER:
		dev_dbg(&info->client->dev, "=== Cp Jitter Test ===\n");
		snprintf(info->print_buf, PAGE_SIZE, "\n=== Cp Jitter Test ===\n\n");
		break;
	case MIP4_TEST_TYPE_CP_DIFF_HOR:
		dev_dbg(&info->client->dev, "=== Cp Diff (Horizontal) Test ===\n");
		snprintf(info->print_buf, PAGE_SIZE,
			"\n=== Cp Diff (Horizontal) Test ===\n\n");
		break;
	case MIP4_TEST_TYPE_CP_DIFF_VER:
		dev_dbg(&info->client->dev, "=== Cp Diff (Vertical) Test ===\n");
		snprintf(info->print_buf, PAGE_SIZE,
			"\n=== Cp Diff (Vertical) Test ===\n\n");
		break;
	case MIP4_TEST_TYPE_CP_SHORT:
		dev_dbg(&info->client->dev, "=== Cp Short Test ===\n");
		snprintf(info->print_buf, PAGE_SIZE, "\n=== Cp Short Test ===\n\n");
		break;
	case MIP4_TEST_TYPE_CP_LPM:
		dev_dbg(&info->client->dev, "=== Cp (LPM) Test ===\n");
		snprintf(info->print_buf, PAGE_SIZE, "\n=== Cp (LPM) Test ===\n\n");
		break;
	case MIP4_TEST_TYPE_SHORT:
		dev_dbg(&info->client->dev, "=== Short Test ===\n");
		snprintf(info->print_buf, PAGE_SIZE, "\n=== Short Test ===\n\n");
		break;
	default:
		dev_err(&info->client->dev, "%s [ERROR] Unknown test type\n", __func__);
		snprintf(info->print_buf, PAGE_SIZE, "\nERROR : Unknown test type\n\n");
		goto error;
	}

	/* set test mode */
	wbuf[0] = MIP4_R0_CTRL;
	wbuf[1] = MIP4_R1_CTRL_MODE;
	wbuf[2] = MIP4_CTRL_MODE_TEST;
	if (mip4_ts_i2c_write(info, wbuf, 3)) {
		dev_err(&info->client->dev, "%s [ERROR] Write test mode\n", __func__);
		goto error;
	}

	/* wait ready status */
	wait_cnt = wait_num;
	while (wait_cnt--) {
		if (mip4_ts_get_ready_status(info) == MIP4_CTRL_STATUS_READY) {
			break;
		}
		usleep_range(5 * 1000, 10 * 1000);

		dev_dbg(&info->client->dev, "%s - wait[%d]\n", __func__, wait_cnt);
	}

	if (wait_cnt <= 0) {
		dev_err(&info->client->dev, "%s [ERROR] Wait timeout\n", __func__);
		goto error;
	}

	dev_dbg(&info->client->dev, "%s - set control mode\n", __func__);

	/* set test type */
	wbuf[0] = MIP4_R0_TEST;
	wbuf[1] = MIP4_R1_TEST_TYPE;
	wbuf[2] = test_type;
	if (mip4_ts_i2c_write(info, wbuf, 3)) {
		dev_err(&info->client->dev, "%s [ERROR] Write test type\n", __func__);
		goto error;
	}

	dev_dbg(&info->client->dev, "%s - set test type\n", __func__);

	/* wait ready status */
	wait_cnt = wait_num;
	while (wait_cnt--) {
		if (mip4_ts_get_ready_status(info) == MIP4_CTRL_STATUS_READY) {
			break;
		}
		usleep_range(5 * 1000, 10 * 1000);

		dev_dbg(&info->client->dev, "%s - wait[%d]\n", __func__, wait_cnt);
	}

	if (wait_cnt <= 0) {
		dev_err(&info->client->dev, "%s [ERROR] Wait timeout\n", __func__);
		goto error;
	}

	dev_dbg(&info->client->dev, "%s - ready\n", __func__);

	/* data format */
	wbuf[0] = MIP4_R0_TEST;
	wbuf[1] = MIP4_R1_TEST_DATA_FORMAT;
	if (mip4_ts_i2c_read(info, wbuf, 2, rbuf, 7)) {
		dev_err(&info->client->dev, "%s [ERROR] Read data format\n", __func__);
		goto error;
	}
	row_num = rbuf[0];
	col_num = rbuf[1];
	buffer_col_num = rbuf[2];
	rotate = rbuf[3];
	key_num = rbuf[4];
	data_type = rbuf[5];
	data_type_sign = (data_type & 0x80) >> 7;
	data_type_size = data_type & 0x7F;
	vector_num = rbuf[6];

	dev_dbg(&info->client->dev,
	"%s - row_num[%d] col_num[%d] buffer_col_num[%d] rotate[%d] key_num[%d]\n",
	__func__, row_num, col_num, buffer_col_num, rotate, key_num);
	dev_dbg(&info->client->dev,
   	"%s - data_type[0x%02X] data_type_sign[%d] data_type_size[%d]\n",
   	__func__, data_type, data_type_sign, data_type_size);
	dev_dbg(&info->client->dev, "%s - vector_num[%d]\n", __func__, vector_num);

	if (vector_num > 0) {
		wbuf[0] = MIP4_R0_TEST;
		wbuf[1] = MIP4_R1_TEST_VECTOR_INFO;
		if (mip4_ts_i2c_read(info, wbuf, 2, rbuf, (vector_num * 4))) {
			dev_err(&info->client->dev,
				"%s [ERROR] Read vector info\n", __func__);
			goto error;
		}
		for (i = 0; i < vector_num; i++) {
			vector_id[i] = rbuf[i * 4 + 0] | (rbuf[i * 4 + 1] << 8);
			vector_elem_num[i] = rbuf[i * 4 + 2] | (rbuf[i * 4 + 3] << 8);
			dev_dbg(&info->client->dev,
				"%s - vector[%d] : id[%d] elem_num[%d]\n", __func__,
				i, vector_id[i], vector_elem_num[i]);
		}
	}

	/* get buf addr */
	wbuf[0] = MIP4_R0_TEST;
	wbuf[1] = MIP4_R1_TEST_BUF_ADDR;
	if (mip4_ts_i2c_read(info, wbuf, 2, rbuf, 2)) {
		dev_err(&info->client->dev, "%s [ERROR] Read buf addr\n", __func__);
		goto error;
	}

	buf_addr_l = rbuf[0];
	buf_addr_h = rbuf[1];
	buf_addr = (buf_addr_h << 8) | buf_addr_l;
	dev_dbg(&info->client->dev,
		"%s - table buf_addr[0x%02X 0x%02X][0x%04X]\n",
			__func__, buf_addr_h, buf_addr_l, buf_addr);

	/* print data */
	table_size = row_num * col_num;
	if (table_size > 0) {
		if (mip4_ts_proc_table_data(info, data_type_size, data_type_sign,
			buf_addr_h, buf_addr_l, row_num, col_num, buffer_col_num, rotate)) {
			dev_err(&info->client->dev,
				"%s [ERROR] mip4_ts_proc_table_data\n", __func__);
			goto error;
		}
	}
	if ((key_num > 0) || (vector_num > 0)) {
		if (table_size > 0) {
			buf_addr += (row_num * buffer_col_num * data_type_size);
		}

		buf_addr_l = buf_addr & 0xFF;
		buf_addr_h = (buf_addr >> 8) & 0xFF;
		dev_dbg(&info->client->dev,
			"%s - vector buf_addr[0x%02X 0x%02X][0x%04X]\n",
			__func__, buf_addr_h, buf_addr_l, buf_addr);

		if (mip4_ts_proc_vector_data(info, data_type_size,
			data_type_sign, buf_addr_h, buf_addr_l, key_num,
			vector_num, vector_id, vector_elem_num, table_size)) {
			dev_err(&info->client->dev,
				"%s [ERROR] mip4_ts_proc_vector_data\n", __func__);
			goto error;
		}
	}

	/* set normal mode */
	wbuf[0] = MIP4_R0_CTRL;
	wbuf[1] = MIP4_R1_CTRL_MODE;
	wbuf[2] = MIP4_CTRL_MODE_NORMAL;
	if (mip4_ts_i2c_write(info, wbuf, 3)) {
		dev_err(&info->client->dev, "%s [ERROR] mip4_ts_i2c_write\n", __func__);
		goto error;
	}

	/* wait ready status */
	wait_cnt = wait_num;
	while (wait_cnt--) {
		if (mip4_ts_get_ready_status(info) == MIP4_CTRL_STATUS_READY) {
			break;
		}
		usleep_range(5 * 1000, 10 * 1000);

		dev_dbg(&info->client->dev, "%s - wait[%d]\n", __func__, wait_cnt);
	}

	if (wait_cnt <= 0) {
		dev_err(&info->client->dev, "%s [ERROR] Wait timeout\n", __func__);
		goto error;
	}

	dev_dbg(&info->client->dev, "%s - set normal mode\n", __func__);

	goto exit;

error:
	ret = 1;

exit:
	mip4_ts_restart(info);

	/* enable touch event */
	wbuf[0] = MIP4_R0_CTRL;
	wbuf[1] = MIP4_R1_CTRL_EVENT_TRIGGER_TYPE;
	wbuf[2] = MIP4_CTRL_TRIGGER_INTR;
	if (mip4_ts_i2c_write(info, wbuf, 3)) {
		dev_err(&info->client->dev, "%s [ERROR] Enable event\n", __func__);
		ret = 1;
	}

	mutex_lock(&info->lock);
	info->test_busy = false;
	enable_irq(info->irq);
	mutex_unlock(&info->lock);

	if (!ret) {
		dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	} else {
		dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	}

	return ret;
}

/*
* Read image data
*/
int mip4_ts_get_image(struct mip4_ts_info *info, u8 image_type)
{
	int busy_cnt = 500;
	int wait_cnt = 0;
	int wait_num = 200;
	u8 wbuf[8];
	u8 rbuf[512];
	u8 row_num;
	u8 col_num;
	u8 buffer_col_num;
	u8 rotate;
	u8 key_num;
	u8 data_type;
	u8 data_type_size;
	u8 data_type_sign;
	u8 vector_num = 0;
	u16 vector_id[16];
	u16 vector_elem_num[16];
	u8 buf_addr_h;
	u8 buf_addr_l;
	u16 buf_addr;
	int table_size;
	int i;
	int ret = 0;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);
	dev_dbg(&info->client->dev, "%s - image_type[%d]\n", __func__, image_type);

	while (busy_cnt--) {
		if (info->test_busy == false) {
			break;
		}
		dev_dbg(&info->client->dev, "%s - busy_cnt[%d]\n", __func__, busy_cnt);
		usleep_range(1 * 1000, 1 * 1000);
	}

	mutex_lock(&info->lock);
	disable_irq(info->irq);
	info->test_busy = true;
	mutex_unlock(&info->lock);

	memset(info->print_buf, 0, PAGE_SIZE);

	/* disable touch event */
	wbuf[0] = MIP4_R0_CTRL;
	wbuf[1] = MIP4_R1_CTRL_EVENT_TRIGGER_TYPE;
	wbuf[2] = MIP4_CTRL_TRIGGER_NONE;
	if (mip4_ts_i2c_write(info, wbuf, 3)) {
		dev_err(&info->client->dev, "%s [ERROR] Disable event\n", __func__);
		goto error;
	}

	/* check image type */
	switch (image_type) {
	case MIP4_IMG_TYPE_INTENSITY:
		dev_dbg(&info->client->dev, "=== Intensity Image ===\n");
		snprintf(info->print_buf, PAGE_SIZE, "\n=== Intensity Image ===\n\n");
		break;
	case MIP4_IMG_TYPE_RAWDATA:
		dev_dbg(&info->client->dev, "=== Rawdata Image ===\n");
		snprintf(info->print_buf, PAGE_SIZE, "\n=== Rawdata Image ===\n\n");
		break;
	case MIP4_IMG_TYPE_GESTURE:
		dev_dbg(&info->client->dev, "=== Gesture Image ===\n");
		snprintf(info->print_buf, PAGE_SIZE, "\n=== Gesture Image ===\n\n");
		break;
	case MIP4_IMG_TYPE_HSELF_RAWDATA:
		dev_dbg(&info->client->dev, "=== Self Rawdata Image ===\n");
		snprintf(info->print_buf, PAGE_SIZE,
			"\n=== Self Rawdata Image ===\n\n");
		break;
	case MIP4_IMG_TYPE_HSELF_INTENSITY:
		dev_dbg(&info->client->dev, "=== Self Intensity Image ===\n");
		snprintf(info->print_buf, PAGE_SIZE,
			"\n=== Self Intensity Image ===\n\n");
		break;
	default:
		dev_err(&info->client->dev,
			"%s [ERROR] Unknown image type\n", __func__);
		snprintf(info->print_buf, PAGE_SIZE,
			"\nERROR : Unknown image type\n\n");
		goto error;
	}

	/* set image type */
	wbuf[0] = MIP4_R0_IMAGE;
	wbuf[1] = MIP4_R1_IMAGE_TYPE;
	wbuf[2] = image_type;
	if (mip4_ts_i2c_write(info, wbuf, 3)) {
		dev_err(&info->client->dev, "%s [ERROR] Write image type\n", __func__);
		goto error;
	}

	dev_dbg(&info->client->dev, "%s - set image type\n", __func__);

	/* wait ready status */
	wait_cnt = wait_num;
	while (wait_cnt--) {
		if (mip4_ts_get_ready_status(info) == MIP4_CTRL_STATUS_READY) {
			break;
		}
		usleep_range(5 * 1000, 10 * 1000);

		dev_dbg(&info->client->dev, "%s - wait[%d]\n", __func__, wait_cnt);
	}

	if (wait_cnt <= 0) {
		dev_err(&info->client->dev, "%s [ERROR] Wait timeout\n", __func__);
		goto error;
	}

	dev_dbg(&info->client->dev, "%s - ready\n", __func__);

	/* data format */
	wbuf[0] = MIP4_R0_IMAGE;
	wbuf[1] = MIP4_R1_IMAGE_DATA_FORMAT;
	if (mip4_ts_i2c_read(info, wbuf, 2, rbuf, 7)) {
		dev_err(&info->client->dev, "%s [ERROR] Read data format\n", __func__);
		goto error;
	}
	row_num = rbuf[0];
	col_num = rbuf[1];
	buffer_col_num = rbuf[2];
	rotate = rbuf[3];
	key_num = rbuf[4];
	data_type = rbuf[5];
	data_type_sign = (data_type & 0x80) >> 7;
	data_type_size = data_type & 0x7F;
	vector_num = rbuf[6];

	dev_dbg(&info->client->dev,
	"%s - row_num[%d] col_num[%d] buffer_col_num[%d] rotate[%d] key_num[%d]\n",
	__func__, row_num, col_num, buffer_col_num, rotate, key_num);
	dev_dbg(&info->client->dev,
	"%s - data_type[0x%02X] data_type_sign[%d] data_type_size[%d]\n",
	__func__, data_type, data_type_sign, data_type_size);
	dev_dbg(&info->client->dev, "%s - vector_num[%d]\n", __func__, vector_num);

	if (vector_num > 0) {
		wbuf[0] = MIP4_R0_IMAGE;
		wbuf[1] = MIP4_R1_IMAGE_VECTOR_INFO;
		if (mip4_ts_i2c_read(info, wbuf, 2, rbuf, (vector_num * 4))) {
			dev_err(&info->client->dev, "%s [ERROR] Read vector info\n",
				__func__);
			goto error;
		}
		for (i = 0; i < vector_num; i++) {
			vector_id[i] = rbuf[i * 4 + 0] | (rbuf[i * 4 + 1] << 8);
			vector_elem_num[i] = rbuf[i * 4 + 2] | (rbuf[i * 4 + 3] << 8);
			dev_dbg(&info->client->dev,
				"%s - vector[%d] : id[%d] elem_num[%d]\n", __func__,
				i, vector_id[i], vector_elem_num[i]);
		}
	}

	/* get buf addr */
	wbuf[0] = MIP4_R0_IMAGE;
	wbuf[1] = MIP4_R1_IMAGE_BUF_ADDR;
	if (mip4_ts_i2c_read(info, wbuf, 2, rbuf, 2)) {
		dev_err(&info->client->dev, "%s [ERROR] Read buf addr\n", __func__);
		goto error;
	}

	buf_addr_l = rbuf[0];
	buf_addr_h = rbuf[1];
	buf_addr = (buf_addr_h << 8) | buf_addr_l;
	dev_dbg(&info->client->dev,
		"%s - table buf_addr[0x%02X 0x%02X][0x%04X]\n", __func__,
			buf_addr_h, buf_addr_l, buf_addr);

	/* print data */
	table_size = row_num * col_num;
	if (table_size > 0) {
		if (mip4_ts_proc_table_data(info, data_type_size,
			data_type_sign, buf_addr_h, buf_addr_l, row_num,
			col_num, buffer_col_num, rotate)) {
			dev_err(&info->client->dev,
				"%s [ERROR] mip4_ts_proc_table_data\n", __func__);
			goto error;
		}
	}
	if ((key_num > 0) || (vector_num > 0)) {
		if (table_size > 0) {
			buf_addr += (row_num * buffer_col_num * data_type_size);
		}

		buf_addr_l = buf_addr & 0xFF;
		buf_addr_h = (buf_addr >> 8) & 0xFF;
		dev_dbg(&info->client->dev,
			"%s - vector buf_addr[0x%02X 0x%02X][0x%04X]\n",
			__func__, buf_addr_h, buf_addr_l, buf_addr);

		if (mip4_ts_proc_vector_data(info, data_type_size,
			data_type_sign, buf_addr_h, buf_addr_l, key_num,
			vector_num, vector_id, vector_elem_num, table_size)) {
			dev_err(&info->client->dev,
				"%s [ERROR] mip4_ts_proc_vector_data\n", __func__);
			goto error;
		}
	}

	/* clear image type */
	wbuf[0] = MIP4_R0_IMAGE;
	wbuf[1] = MIP4_R1_IMAGE_TYPE;
	wbuf[2] = MIP4_IMG_TYPE_NONE;
	if (mip4_ts_i2c_write(info, wbuf, 3)) {
		dev_err(&info->client->dev, "%s [ERROR] Clear image type\n", __func__);
		goto error;
	}

	/* post process */
	if (image_type == MIP4_IMG_TYPE_GESTURE) {
		memcpy(info->debug_buf, info->print_buf, PAGE_SIZE);
	}

	goto exit;

error:
	ret = 1;

exit:
	/* enable touch event */
	wbuf[0] = MIP4_R0_CTRL;
	wbuf[1] = MIP4_R1_CTRL_EVENT_TRIGGER_TYPE;
	wbuf[2] = MIP4_CTRL_TRIGGER_INTR;
	if (mip4_ts_i2c_write(info, wbuf, 3)) {
		dev_err(&info->client->dev, "%s [ERROR] Enable event\n", __func__);
		ret = 1;
	}

	mutex_lock(&info->lock);
	info->test_busy = false;
	enable_irq(info->irq);
	mutex_unlock(&info->lock);

	if (!ret) {
		dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	} else {
		dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	}

	return ret;
}
#endif /* (USE_SYS || USE_CMD) */

#if USE_SYS
/*
* Print chip firmware version
*/
static ssize_t mip4_ts_sys_fw_version(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mip4_ts_info *info = dev_get_drvdata(dev);
	u8 data[255];
	int ret;
	u8 rbuf[16];

	if (mip4_ts_get_fw_version(info, rbuf)) {
		dev_err(&info->client->dev,
			"%s [ERROR] mip4_ts_get_fw_version\n", __func__);

		snprintf(data, sizeof(data), "F/W Version : ERROR\n");
		goto error;
	}

	dev_info(&info->client->dev,
		"%s - F/W Version : %02X.%02X/%02X.%02X/%02X.%02X/%02X.%02X\n",
		__func__, rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4], rbuf[5],
		rbuf[6], rbuf[7]);
	snprintf(data, sizeof(data),
		"F/W Version : %02X.%02X/%02X.%02X/%02X.%02X/%02X.%02X\n",
		rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4], rbuf[5], rbuf[6], rbuf[7]);

error:
	ret = snprintf(buf, 255, "%s\n", data);
	return ret;
}

#if (CHIP_MODEL != CHIP_NONE)
/*
* Update firmware from external storage
*/
static ssize_t mip4_ts_sys_fw_update_from_storage(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mip4_ts_info *info = i2c_get_clientdata(client);
	int result = 0;
	u8 data[255];
	int ret = 0;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	/* Update firmware */
	ret = mip4_ts_fw_update_from_storage(info, info->fw_path_ext, true);

	switch (ret) {
	case fw_err_none:
		snprintf(data, sizeof(data), "F/W update success.\n");
		break;
	case fw_err_uptodate:
		snprintf(data, sizeof(data), "F/W is already up-to-date.\n");
		break;
	case fw_err_download:
		snprintf(data, sizeof(data), "F/W update failed : Download error\n");
		break;
	case fw_err_file_type:
		snprintf(data, sizeof(data), "F/W update failed : File type error\n");
		break;
	case fw_err_file_open:
		snprintf(data, sizeof(data),
			"F/W update failed : File open error[%s]\n", info->fw_path_ext);
		break;
	case fw_err_file_read:
		snprintf(data, sizeof(data), "F/W update failed : File read error\n");
		break;
	default:
		snprintf(data, sizeof(data), "F/W update failed.\n");
		break;
	}

	/* Re-config driver */
	mip4_ts_config(info);
	mip4_ts_config_input(info);

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);

	result = snprintf(buf, 255, "%s\n", data);
	return result;
}

/*
* Set firmware path
*/
static ssize_t mip4_ts_sys_fw_path_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mip4_ts_info *info = dev_get_drvdata(dev);
	char *path;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	if (count <= 1) {
		dev_err(&info->client->dev,
			"%s [ERROR] Wrong value [%s]\n", __func__, buf);
		goto error;
	}

	path = kzalloc(count - 1, GFP_KERNEL);
	memcpy(path, buf, count - 1);

	info->fw_path_ext = kstrdup(path, GFP_KERNEL);

	dev_dbg(&info->client->dev,
			"%s - Path : %s\n", __func__, info->fw_path_ext);

	kfree(path);

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return count;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return count;
}

/*
* Print firmware path
*/
static ssize_t mip4_ts_sys_fw_path_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mip4_ts_info *info = dev_get_drvdata(dev);
	u8 data[255];
	int ret;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	snprintf(data, sizeof(data), "Path : %s\n", info->fw_path_ext);

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);

	ret = snprintf(buf, 255, "%s\n", data);
	return ret;
}

/*
* Print bin(file) firmware version
*/
static ssize_t mip4_ts_sys_bin_version(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct mip4_ts_info *info = dev_get_drvdata(dev);
	u8 data[255];
	int ret;
	u8 rbuf[16];

	if (mip4_ts_get_fw_version_from_bin(info, rbuf)) {
		dev_err(&info->client->dev,
			"%s [ERROR] mip4_ts_get_fw_version_from_bin\n", __func__);
		snprintf(data, sizeof(data), "BIN Version : ERROR\n");
		goto error;
	}

	dev_info(&info->client->dev,
		"%s - BIN Version : %02X.%02X/%02X.%02X/%02X.%02X/%02X.%02X\n",
		__func__, rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4],
		rbuf[5], rbuf[6], rbuf[7]);
	snprintf(data, sizeof(data),
		"BIN Version : %02X.%02X/%02X.%02X/%02X.%02X/%02X.%02X\n",
		rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4], rbuf[5], rbuf[6], rbuf[7]);

error:
	ret = snprintf(buf, 255, "%s\n", data);
	return ret;
}
#endif	/* CHIP_MODEL */

#if USE_FLASH_CAL_TABLE
/*
* Flash calibration table
*/
static ssize_t mip4_ts_sys_flash_cal_table(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mip4_ts_info *info = dev_get_drvdata(dev);
	u8 data[255];
	int ret;

	if (mip4_ts_flash_cal_table(info)) {
		dev_err(&info->client->dev,
			"%s [ERROR] mip4_ts_flash_cal_table\n", __func__);
		snprintf(data, sizeof(data), "ERROR");
		goto error;
	}
	snprintf(data, sizeof(data), "DONE");

error:
	ret = snprintf(buf, 255, "%s\n", data);
	return ret;
}
#endif /* USE_FLASH_CAL_TABLE */

/*
* Print channel info
*/
static ssize_t mip4_ts_sys_info(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mip4_ts_info *info = dev_get_drvdata(dev);
	u8 data[255];
	int ret;
	u8 rbuf[16];
	u8 wbuf[8];
	unsigned int res_x, res_y;
	unsigned int len_x, len_y;
	unsigned int ppm_x, ppm_y;

	memset(info->print_buf, 0, PAGE_SIZE);
	snprintf(info->print_buf, PAGE_SIZE, "\n");

	snprintf(info->print_buf, PAGE_SIZE,
			"Device Name : %s\n", MIP4_TS_DEVICE_NAME);

	snprintf(data, sizeof(data), "Driver Chip Name : %s\n", CHIP_NAME);
	strlcat(info->print_buf, data, PAGE_SIZE);

	wbuf[0] = MIP4_R0_INFO;
	wbuf[1] = MIP4_R1_INFO_IC_NAME;
	mip4_ts_i2c_read(info, wbuf, 2, rbuf, 4);
	snprintf(data, sizeof(data), "Chip Model Code : %.*s\n", 4, rbuf);
	strlcat(info->print_buf, data, PAGE_SIZE);

	wbuf[0] = MIP4_R0_INFO;
	wbuf[1] = MIP4_R1_INFO_PRODUCT_NAME;
	mip4_ts_i2c_read(info, wbuf, 2, rbuf, 16);
	snprintf(data, sizeof(data), "Product Name : %.*s\n", 16, rbuf);
	strlcat(info->print_buf, data, PAGE_SIZE);

	mip4_ts_get_fw_version(info, rbuf);
	snprintf(data, sizeof(data),
		"F/W Version : %02X.%02X/%02X.%02X/%02X.%02X/%02X.%02X\n",
		rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4], rbuf[5], rbuf[6], rbuf[7]);
	strlcat(info->print_buf, data, PAGE_SIZE);

	wbuf[0] = MIP4_R0_INFO;
	wbuf[1] = MIP4_R1_INFO_RESOLUTION_X;
	mip4_ts_i2c_read(info, wbuf, 2, rbuf, 14);
	res_x = (rbuf[0]) | (rbuf[1] << 8);
	res_y = (rbuf[2]) | (rbuf[3] << 8);
	snprintf(data, sizeof(data), "Resolution : X[%d] Y[%d]\n", res_x, res_y);
	strlcat(info->print_buf, data, PAGE_SIZE);

	len_x = (rbuf[8]) | (rbuf[9] << 8);
	len_y = (rbuf[10]) | (rbuf[11] << 8);
	snprintf(data, sizeof(data), "Panel size (dmm) : X[%d] Y[%d]\n",
			len_x, len_y);
	strlcat(info->print_buf, data, PAGE_SIZE);

	ppm_x = rbuf[12];
	ppm_y = rbuf[13];
	snprintf(data, sizeof(data), "Pixels per mm : X[%d] Y[%d]\n", ppm_x, ppm_y);
	strlcat(info->print_buf, data, PAGE_SIZE);

	snprintf(data, sizeof(data), "Node Num : X[%d] Y[%d] Key[%d]\n",
		rbuf[4], rbuf[5], rbuf[6]);
	strlcat(info->print_buf, data, PAGE_SIZE);

	ret = snprintf(buf, PAGE_SIZE, "%s\n", info->print_buf);
	return ret;
}

#ifdef DEBUG
/*
* Power on
*/
static ssize_t mip4_ts_sys_power_on(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mip4_ts_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	u8 data[255];
	int ret;

	mip4_ts_power_on(info);

	dev_info(&client->dev, "%s", __func__);

	snprintf(data, sizeof(data), "Power : On\n");

	ret = snprintf(buf, 225, "%s\n", data);
	return ret;

}

/*
* Power off
*/
static ssize_t mip4_ts_sys_power_off(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mip4_ts_info *info = dev_get_drvdata(dev);
	u8 data[255];
	int ret;

	mip4_ts_power_off(info);

	dev_info(&info->client->dev, "%s", __func__);

	snprintf(data, sizeof(data), "Power : Off\n");

	ret = snprintf(buf, 225, "%s\n", data);
	return ret;
}

/*
* Restart chip
*/
static ssize_t mip4_ts_sys_restart(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mip4_ts_info *info = dev_get_drvdata(dev);
	u8 data[255];
	int ret;

	dev_info(&info->client->dev, "%s", __func__);

	if (info->irq_enabled) {
		disable_irq(info->irq);
		info->irq_enabled = false;
	}

	mip4_ts_clear_input(info);

	mip4_ts_restart(info);

	if (!info->irq_enabled) {
		enable_irq(info->irq);
		info->irq_enabled = true;
	}

	snprintf(data, sizeof(data), "Restart\n");

	ret = snprintf(buf, 255, "%s\n", data);
	return ret;
}
#endif /* DEBUG */

/*
* Sysfs set mode
*/
static ssize_t mip4_ts_sys_mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mip4_ts_info *info = dev_get_drvdata(dev);
	u8 wbuf[8];
	u8 value = 0;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	wbuf[0] = MIP4_R0_CTRL;

	if (!strcmp(attr->attr.name, "mode_glove")) {
		wbuf[1] = MIP4_R1_CTRL_HIGH_SENS_MODE;
	} else if (!strcmp(attr->attr.name, "mode_charger")) {
		wbuf[1] = MIP4_R1_CTRL_CHARGER_MODE;
	} else if (!strcmp(attr->attr.name, "mode_cover_window")) {
		wbuf[1] = MIP4_R1_CTRL_WINDOW_MODE;
	} else if (!strcmp(attr->attr.name, "mode_palm_rejection")) {
		wbuf[1] = MIP4_R1_CTRL_PALM_REJECTION;
	} else if (!strcmp(attr->attr.name, "mode_edge_correction")) {
		wbuf[1] = MIP4_R1_CTRL_EDGE_CORRECTION;
	} else if (!strcmp(attr->attr.name, "mode_proximity_sensing")) {
		wbuf[1] = MIP4_R1_CTRL_PROXIMITY_SENSING;
	} else if (!strcmp(attr->attr.name, "mode_gesture_debug")) {
		wbuf[1] = MIP4_R1_CTRL_GESTURE_DEBUG;
	} else if (!strcmp(attr->attr.name, "mode_power")) {
		wbuf[1] = MIP4_R1_CTRL_POWER_STATE;
	} else {
		dev_err(&info->client->dev, "%s [ERROR] Unknown mode[%s]\n", __func__,
				attr->attr.name);
		goto error;
	}

	if (buf[0] == 48) {
		value = 0;
	} else if (buf[0] == 49) {
		value = 1;
	} else {
		dev_err(&info->client->dev, "%s [ERROR] Wrong value[%c]\n",
			__func__, buf[0]);
		goto error;
	}
	wbuf[2] = value;

	if (mip4_ts_i2c_write(info, wbuf, 3)) {
		dev_err(&info->client->dev, "%s [ERROR] mip4_ts_i2c_write\n", __func__);
		goto error;
	} else {
		dev_info(&info->client->dev, "%s - addr[0x%02X%02X] value[%d]\n",
			__func__, wbuf[0], wbuf[1], value);
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return count;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return count;
}

/*
* Sysfs print mode
*/
static ssize_t mip4_ts_sys_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mip4_ts_info *info = dev_get_drvdata(dev);
	u8 data[255];
	int ret;
	u8 wbuf[8];
	u8 rbuf[4];

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	wbuf[0] = MIP4_R0_CTRL;

	if (!strcmp(attr->attr.name, "mode_glove")) {
		wbuf[1] = MIP4_R1_CTRL_HIGH_SENS_MODE;
	} else if (!strcmp(attr->attr.name, "mode_charger")) {
		wbuf[1] = MIP4_R1_CTRL_CHARGER_MODE;
	} else if (!strcmp(attr->attr.name, "mode_cover_window")) {
		wbuf[1] = MIP4_R1_CTRL_WINDOW_MODE;
	} else if (!strcmp(attr->attr.name, "mode_palm_rejection")) {
		wbuf[1] = MIP4_R1_CTRL_PALM_REJECTION;
	} else if (!strcmp(attr->attr.name, "mode_edge_correction")) {
		wbuf[1] = MIP4_R1_CTRL_EDGE_CORRECTION;
	} else if (!strcmp(attr->attr.name, "mode_proximity_sensing")) {
		wbuf[1] = MIP4_R1_CTRL_PROXIMITY_SENSING;
	} else if (!strcmp(attr->attr.name, "mode_gesture_debug")) {
		wbuf[1] = MIP4_R1_CTRL_GESTURE_DEBUG;
	} else if (!strcmp(attr->attr.name, "mode_power")) {
		wbuf[1] = MIP4_R1_CTRL_POWER_STATE;
	} else {
		dev_err(&info->client->dev, "%s [ERROR] Unknown mode[%s]\n", __func__,
			attr->attr.name);
		snprintf(data, sizeof(data), "%s : Unknown Mode\n", attr->attr.name);
		goto exit;
	}

	if (mip4_ts_i2c_read(info, wbuf, 2, rbuf, 1)) {
		dev_err(&info->client->dev, "%s [ERROR] mip4_ts_i2c_read\n", __func__);
		snprintf(data, sizeof(data), "%s : ERROR\n", attr->attr.name);
	} else {
		dev_info(&info->client->dev, "%s - addr[0x%02X%02X] value[%d]\n",
			__func__, wbuf[0], wbuf[1], rbuf[0]);
		snprintf(data, sizeof(data), "%s : %d\n", attr->attr.name, rbuf[0]);
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);

exit:
	ret = snprintf(buf, 255, "%s\n", data);
	return ret;
}

/*
* Sysfs print image
*/
static ssize_t mip4_ts_sys_image(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mip4_ts_info *info = dev_get_drvdata(dev);
	int ret;
	u8 type;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	if (!strcmp(attr->attr.name, "image_intensity")) {
		type = MIP4_IMG_TYPE_INTENSITY;
	} else if (!strcmp(attr->attr.name, "image_rawdata")) {
		type = MIP4_IMG_TYPE_RAWDATA;
	} else if (!strcmp(attr->attr.name, "image_gesture")) {
		type = MIP4_IMG_TYPE_GESTURE;
	} else if (!strcmp(attr->attr.name, "image_self_rawdata")) {
		type = MIP4_IMG_TYPE_HSELF_RAWDATA;
	} else if (!strcmp(attr->attr.name, "image_self_intensity")) {
		type = MIP4_IMG_TYPE_HSELF_INTENSITY;
	} else {
		dev_err(&info->client->dev, "%s [ERROR] Unknown image[%s]\n",
			__func__, attr->attr.name);
		ret = snprintf(buf, PAGE_SIZE, "%s\n", "ERROR : Unknown image type");
		goto error;
	}

	if (mip4_ts_get_image(info, type)) {
		dev_err(&info->client->dev, "%s [ERROR] mip4_ts_get_image\n", __func__);
		ret = snprintf(buf, PAGE_SIZE, "%s\n", "ERROR");
		goto error;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);

	ret = snprintf(buf, PAGE_SIZE, "%s\n", info->print_buf);
	return ret;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return ret;
}

/*
* Sysfs print debug data
*/
static ssize_t mip4_ts_sys_debug(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mip4_ts_info *info = dev_get_drvdata(dev);
	int ret;
	u8 type;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	if (!strcmp(attr->attr.name, "debug_gesture")) {
		type = MIP4_IMG_TYPE_GESTURE;
	} else {
		dev_err(&info->client->dev, "%s [ERROR] Unknown image[%s]\n",
			__func__, attr->attr.name);
		ret = snprintf(buf, PAGE_SIZE, "%s\n", "ERROR : Unknown image type");
		goto error;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);

	ret = snprintf(buf, PAGE_SIZE, "%s\n", info->debug_buf);
	return ret;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return ret;
}

/*
* Sysfs run test
*/
static ssize_t mip4_ts_sys_test(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mip4_ts_info *info = dev_get_drvdata(dev);
	int ret;
	u8 test_type;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	if (!strcmp(attr->attr.name, "test_cm")) {
		test_type = MIP4_TEST_TYPE_CM;
	} else if (!strcmp(attr->attr.name, "test_cm_abs")) {
		test_type = MIP4_TEST_TYPE_CM_ABS;
	} else if (!strcmp(attr->attr.name, "test_cm_jitter")) {
		test_type = MIP4_TEST_TYPE_CM_JITTER;
	} else if (!strcmp(attr->attr.name, "test_cm_diff_horizontal")) {
		test_type = MIP4_TEST_TYPE_CM_DIFF_HOR;
	} else if (!strcmp(attr->attr.name, "test_cm_diff_vertical")) {
		test_type = MIP4_TEST_TYPE_CM_DIFF_VER;
	} else if (!strcmp(attr->attr.name, "test_cp")) {
		test_type = MIP4_TEST_TYPE_CP;
	} else if (!strcmp(attr->attr.name, "test_cp_low")) {
		test_type = MIP4_TEST_TYPE_CP_LOW;
	} else if (!strcmp(attr->attr.name, "test_cp_high")) {
		test_type = MIP4_TEST_TYPE_CP_HIGH;
	} else if (!strcmp(attr->attr.name, "test_cp_jitter")) {
		test_type = MIP4_TEST_TYPE_CP_JITTER;
	} else if (!strcmp(attr->attr.name, "test_cp_diff_horizontal")) {
		test_type = MIP4_TEST_TYPE_CP_DIFF_HOR;
	} else if (!strcmp(attr->attr.name, "test_cp_diff_vertical")) {
		test_type = MIP4_TEST_TYPE_CP_DIFF_VER;
	} else if (!strcmp(attr->attr.name, "test_cp_short")) {
		test_type = MIP4_TEST_TYPE_CP_SHORT;
	} else if (!strcmp(attr->attr.name, "test_cp_lpm")) {
		test_type = MIP4_TEST_TYPE_CP_LPM;
	} else if (!strcmp(attr->attr.name, "test_short")) {
		test_type = MIP4_TEST_TYPE_SHORT;
	} else {
		dev_err(&info->client->dev, "%s [ERROR] Unknown test[%s]\n",
			__func__, attr->attr.name);
		ret = snprintf(buf, PAGE_SIZE, "%s\n", "ERROR : Unknown test type");
		goto error;
	}

	if (mip4_ts_run_test(info, test_type)) {
		dev_err(&info->client->dev, "%s [ERROR] mip4_ts_run_test\n", __func__);
		ret = snprintf(buf, PAGE_SIZE, "%s\n", "ERROR");
		goto error;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);

	ret = snprintf(buf, PAGE_SIZE, "%s\n", info->print_buf);
	return ret;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return ret;
}

/*
* Sysfs run short test
*/
static ssize_t mip4_ts_sys_test_short(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mip4_ts_info *info = dev_get_drvdata(dev);
	int ret;
	u8 test_type = MIP4_TEST_TYPE_SHORT;
	int ix, iy, idx, ch;
	int cnt = 0;
	char data[10];
	int size = info->node_x + info->node_y + info->node_key;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	if (mip4_ts_run_test(info, test_type)) {
		dev_err(&info->client->dev, "%s [ERROR] mip4_ts_run_test\n", __func__);
		ret = snprintf(buf, PAGE_SIZE, "%s\n", "ERROR");
		goto error;
	}

	memset(info->print_buf, 0, PAGE_SIZE);
	strlcat(info->print_buf, "\n=== Short Test ===\n\n", PAGE_SIZE);

	for (iy = 0; iy < size; iy++) {
		for (ix = (iy + 1); ix < size; ix++) {
			idx = iy * size + ix;

			if (info->image_buf[idx] != 255) {
				cnt++;

				strlcat(info->print_buf, "Short : ", PAGE_SIZE);

				if (ix < info->node_x) {
					strlcat(info->print_buf, "X", PAGE_SIZE);
					ch = ix;
				} else if (ix < (info->node_x + info->node_y)) {
					strlcat(info->print_buf, "Y", PAGE_SIZE);
					ch = ix - info->node_x;
				} else {
					strlcat(info->print_buf, "Key", PAGE_SIZE);
					ch = ix - (info->node_x + info->node_y);
				}
				memset(data, 0, sizeof(data));
				snprintf(data, sizeof(data), "[%d]", ch);
				strlcat(info->print_buf, data, PAGE_SIZE);

				strlcat(info->print_buf, "-", PAGE_SIZE);

				if (iy < info->node_x) {
					strlcat(info->print_buf, "X", PAGE_SIZE);
					ch = iy;
				} else if (iy < (info->node_x + info->node_y)) {
					strlcat(info->print_buf, "Y", PAGE_SIZE);
					ch = iy - info->node_x;
				} else {
					strlcat(info->print_buf, "Key", PAGE_SIZE);
					ch = iy - (info->node_x + info->node_y);
				}
				memset(data, 0, sizeof(data));
				snprintf(data, sizeof(data), "[%d]", ch);
				strlcat(info->print_buf, data, PAGE_SIZE);

				memset(data, 0, sizeof(data));
				snprintf(data, sizeof(data), " (%d)\n", info->image_buf[idx]);
				strlcat(info->print_buf, data, PAGE_SIZE);
			}
		}
	}

	if (cnt == 0) {
		strlcat(info->print_buf, "Result : Pass\n", PAGE_SIZE);
	} else {
		strlcat(info->print_buf, "\n", PAGE_SIZE);
		strlcat(info->print_buf, "Result : Fail\n", PAGE_SIZE);
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);

	ret = snprintf(buf, PAGE_SIZE, "%s\n", info->print_buf);
	return ret;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return ret;
}

/*
* Print wake-up gesture code
*/
#if USE_WAKEUP_GESTURE
static ssize_t mip4_ts_sys_wakeup_gesture(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mip4_ts_info *info = dev_get_drvdata(dev);
	u8 data[255];
	int ret;

	dev_dbg(&info->client->dev, "%s", __func__);

	snprintf(data, sizeof(data), "gesture:%d\n", info->wakeup_gesture_code);

	ret = snprintf(buf, 255, "%s\n", data);
	return ret;
}
#endif /* USE_WAKEUP_GESTURE */

/*
* Sysfs functions
*/
static DEVICE_ATTR(fw_version, S_IRUGO, mip4_ts_sys_fw_version, NULL);
#if (CHIP_MODEL != CHIP_NONE)
static DEVICE_ATTR(fw_update_storage, S_IRUGO,
			mip4_ts_sys_fw_update_from_storage, NULL);
static DEVICE_ATTR(fw_path, (S_IRUGO | S_IWUSR | S_IWGRP),
			mip4_ts_sys_fw_path_show, mip4_ts_sys_fw_path_store);
static DEVICE_ATTR(bin_version, S_IRUGO, mip4_ts_sys_bin_version, NULL);
#endif /* CHIP_MODEL */
#if USE_FLASH_CAL_TABLE
static DEVICE_ATTR(flash_cal_table, S_IRUGO, mip4_ts_sys_flash_cal_table, NULL);
#endif /* USE_FLASH_CAL_TABLE */
static DEVICE_ATTR(info, S_IRUGO, mip4_ts_sys_info, NULL);
#ifdef DEBUG
static DEVICE_ATTR(power_on, S_IRUGO, mip4_ts_sys_power_on, NULL);
static DEVICE_ATTR(power_off, S_IRUGO, mip4_ts_sys_power_off, NULL);
static DEVICE_ATTR(restart, S_IRUGO, mip4_ts_sys_restart, NULL);
#endif /* DEBUG */
static DEVICE_ATTR(mode_glove, (S_IRUGO | S_IWUSR | S_IWGRP),
			mip4_ts_sys_mode_show, mip4_ts_sys_mode_store);
static DEVICE_ATTR(mode_charger, (S_IRUGO | S_IWUSR | S_IWGRP),
			mip4_ts_sys_mode_show, mip4_ts_sys_mode_store);
static DEVICE_ATTR(mode_cover_window, (S_IRUGO | S_IWUSR | S_IWGRP),
			mip4_ts_sys_mode_show, mip4_ts_sys_mode_store);
static DEVICE_ATTR(mode_palm_rejection, (S_IRUGO | S_IWUSR | S_IWGRP),
			mip4_ts_sys_mode_show, mip4_ts_sys_mode_store);
static DEVICE_ATTR(mode_edge_correction, (S_IRUGO | S_IWUSR | S_IWGRP),
			mip4_ts_sys_mode_show, mip4_ts_sys_mode_store);
static DEVICE_ATTR(mode_proximity_sensing, (S_IRUGO | S_IWUSR | S_IWGRP),
			mip4_ts_sys_mode_show, mip4_ts_sys_mode_store);
static DEVICE_ATTR(mode_power, (S_IRUGO | S_IWUSR | S_IWGRP),
			mip4_ts_sys_mode_show, mip4_ts_sys_mode_store);

#ifdef DEBUG
static DEVICE_ATTR(image_intensity, S_IRUGO, mip4_ts_sys_image, NULL);
static DEVICE_ATTR(image_rawdata, S_IRUGO, mip4_ts_sys_image, NULL);
static DEVICE_ATTR(image_debug, S_IRUGO, mip4_ts_sys_debug, NULL);
static DEVICE_ATTR(image_self_rawdata, S_IRUGO, mip4_ts_sys_image, NULL);
static DEVICE_ATTR(image_self_intensity, S_IRUGO, mip4_ts_sys_image, NULL);
static DEVICE_ATTR(test_cm, S_IRUGO, mip4_ts_sys_test, NULL);
static DEVICE_ATTR(test_cm_abs, S_IRUGO, mip4_ts_sys_test, NULL);
static DEVICE_ATTR(test_cm_jitter, S_IRUGO, mip4_ts_sys_test, NULL);
static DEVICE_ATTR(test_cm_diff_horizontal, S_IRUGO, mip4_ts_sys_test, NULL);
static DEVICE_ATTR(test_cm_diff_vertical, S_IRUGO, mip4_ts_sys_test, NULL);
static DEVICE_ATTR(test_cp, S_IRUGO, mip4_ts_sys_test, NULL);
static DEVICE_ATTR(test_cp_low, S_IRUGO, mip4_ts_sys_test, NULL);
static DEVICE_ATTR(test_cp_high, S_IRUGO, mip4_ts_sys_test, NULL);
static DEVICE_ATTR(test_cp_jitter, S_IRUGO, mip4_ts_sys_test, NULL);
static DEVICE_ATTR(test_cp_diff_horizontal, S_IRUGO, mip4_ts_sys_test, NULL);
static DEVICE_ATTR(test_cp_diff_vertical, S_IRUGO, mip4_ts_sys_test, NULL);
static DEVICE_ATTR(test_cp_short, S_IRUGO, mip4_ts_sys_test, NULL);
static DEVICE_ATTR(test_cp_lpm, S_IRUGO, mip4_ts_sys_test, NULL);
static DEVICE_ATTR(test_short, S_IRUGO, mip4_ts_sys_test_short, NULL);
#endif /* DEBUG */
#if USE_WAKEUP_GESTURE
static DEVICE_ATTR(wakeup_gesture, S_IRUGO, mip4_ts_sys_wakeup_gesture, NULL);
#ifdef DEBUG
static DEVICE_ATTR(debug_gesture, S_IRUGO, mip4_ts_sys_debug, NULL);
static DEVICE_ATTR(mode_gesture_debug, (S_IRUGO | S_IWUSR | S_IWGRP),
			mip4_ts_sys_mode_show, mip4_ts_sys_mode_store);
#endif /* DEBUG */
#endif /* USE_WAKEUP_GESTURE */

/*
* Sysfs attr list info
*/
static struct attribute *mip4_ts_sys_attr[] = {
	&dev_attr_fw_version.attr,
#if (CHIP_MODEL != CHIP_NONE)
	&dev_attr_fw_update_storage.attr,
	&dev_attr_fw_path.attr,
	&dev_attr_bin_version.attr,
#endif /* CHIP_MODEL */
#if USE_FLASH_CAL_TABLE
	&dev_attr_flash_cal_table.attr,
#endif /* USE_FLASH_CAL_TABLE */
	&dev_attr_info.attr,
#ifdef DEBUG
	&dev_attr_power_on.attr,
	&dev_attr_power_off.attr,
	&dev_attr_restart.attr,
#endif /* DEBUG */
	&dev_attr_mode_glove.attr,
	&dev_attr_mode_charger.attr,
	&dev_attr_mode_cover_window.attr,
	&dev_attr_mode_palm_rejection.attr,
	&dev_attr_mode_edge_correction.attr,
	&dev_attr_mode_proximity_sensing.attr,
	&dev_attr_mode_power.attr,
#ifdef DEBUG
	&dev_attr_image_intensity.attr,
	&dev_attr_image_rawdata.attr,
	&dev_attr_image_debug.attr,
	&dev_attr_image_self_rawdata.attr,
	&dev_attr_image_self_intensity.attr,
	&dev_attr_test_cm.attr,
	&dev_attr_test_cm_abs.attr,
	&dev_attr_test_cm_jitter.attr,
	&dev_attr_test_cm_diff_horizontal.attr,
	&dev_attr_test_cm_diff_vertical.attr,
	&dev_attr_test_cp.attr,
	&dev_attr_test_cp_low.attr,
	&dev_attr_test_cp_high.attr,
	&dev_attr_test_cp_jitter.attr,
	&dev_attr_test_cp_diff_horizontal.attr,
	&dev_attr_test_cp_diff_vertical.attr,
	&dev_attr_test_cp_short.attr,
	&dev_attr_test_cp_lpm.attr,
	&dev_attr_test_short.attr,
#endif /* DEBUG */
#if USE_WAKEUP_GESTURE
	&dev_attr_wakeup_gesture.attr,
#ifdef DEBUG
	&dev_attr_debug_gesture.attr,
	&dev_attr_mode_gesture_debug.attr,
#endif /* DEBUG */
#endif /* USE_WAKEUP_GESTURE */
	NULL,
};

/*
* Sysfs attr group info
*/
static const struct attribute_group mip4_ts_sys_attr_group = {
	.attrs = mip4_ts_sys_attr,
};

/*
* Create sysfs test functions
*/
int mip4_ts_sysfs_create(struct mip4_ts_info *info)
{
	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	if (sysfs_create_group(&info->client->dev.kobj, &mip4_ts_sys_attr_group)) {
		dev_err(&info->client->dev,
			"%s [ERROR] sysfs_create_group\n", __func__);
		return -EAGAIN;
	}

	info->print_buf = devm_kzalloc(&info->client->dev,
		sizeof(u8) * PAGE_SIZE, GFP_KERNEL);
	info->debug_buf = devm_kzalloc(&info->client->dev,
		sizeof(u8) * PAGE_SIZE, GFP_KERNEL);
	info->image_buf = devm_kzalloc(&info->client->dev,
		sizeof(int) * PAGE_SIZE, GFP_KERNEL);

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;
}

/*
* Remove sysfs test functions
*/
void mip4_ts_sysfs_remove(struct mip4_ts_info *info)
{
	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	sysfs_remove_group(&info->client->dev.kobj, &mip4_ts_sys_attr_group);

	devm_kfree(&info->client->dev, info->print_buf);
	devm_kfree(&info->client->dev, info->debug_buf);
	devm_kfree(&info->client->dev, info->image_buf);

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
}
#endif /* USE_SYS */

