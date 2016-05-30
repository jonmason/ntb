/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Seonghee, Kim <kshblue@nexell.co.kr>
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

#ifndef _VIDEODEV2_NXP_MEDIA_H
#define _VIDEODEV2_NXP_MEDIA_H

/*
 * F O R M A T S
 */


/* compressed formats */
#define V4L2_PIX_FMT_DIV3			v4l2_fourcc('D', 'I', 'V', '3')
#define V4L2_PIX_FMT_DIV4			v4l2_fourcc('D', 'I', 'V', '4')
#define V4L2_PIX_FMT_DIV5			v4l2_fourcc('D', 'I', 'V', '5')
#define V4L2_PIX_FMT_DIV6			v4l2_fourcc('D', 'I', 'V', '6')
#define V4L2_PIX_FMT_DIVX			v4l2_fourcc('D', 'I', 'V', 'X')
#define V4L2_PIX_FMT_RV8			v4l2_fourcc('R', 'V', '3', '0')
#define V4L2_PIX_FMT_RV9			v4l2_fourcc('R', 'V', '4', '0')
#define V4L2_PIX_FMT_WMV9			v4l2_fourcc('W', 'M', 'V', '3')
#define V4L2_PIX_FMT_WVC1			v4l2_fourcc('W', 'V', 'C', '1')
#define V4L2_PIX_FMT_FLV1			v4l2_fourcc('F', 'L', 'V', '1')
#define V4L2_PIX_FMT_THEORA			v4l2_fourcc('T', 'H', 'E', 'O')

/* two non contiguous planes - one Y, one Cr + Cb interleaved  */
/* 24  Y/CbCr 4:4:4 */
#define V4L2_PIX_FMT_NV24M	v4l2_fourcc('N', 'M', '2', '4')
/* 24  Y/CrCb 4:4:4 */
#define V4L2_PIX_FMT_NV42M	v4l2_fourcc('N', 'M', '4', '2')

/* three non contiguous planes - Y, Cb, Cr */
/* 16  YUV422 planar */
#define V4L2_PIX_FMT_YUV422M	v4l2_fourcc('Y', 'M', '1', '6')
/* 24  YUV444 planar */
#define V4L2_PIX_FMT_YUV444M	v4l2_fourcc('Y', 'M', '2', '4')



/*
 * C O N T R O L S
 */


/* Video Codec */
#define V4L2_CID_NXP_VPU_BASE			(V4L2_CTRL_CLASS_MPEG | 0x3000)

#define V4L2_CID_MPEG_VIDEO_FPS_NUM		(V4L2_CID_NXP_VPU_BASE + 0x1)
#define V4L2_CID_MPEG_VIDEO_FPS_DEN		(V4L2_CID_NXP_VPU_BASE + 0x2)
#define V4L2_CID_MPEG_VIDEO_SEARCH_RANGE	(V4L2_CID_NXP_VPU_BASE + 0x4)
#define V4L2_CID_MPEG_VIDEO_H264_AUD_INSERT	(V4L2_CID_NXP_VPU_BASE + 0x5)
#define V4L2_CID_MPEG_VIDEO_RC_DELAY		(V4L2_CID_NXP_VPU_BASE + 0x6)
#define V4L2_CID_MPEG_VIDEO_RC_GAMMA_FACTOR	(V4L2_CID_NXP_VPU_BASE + 0x7)
#define V4L2_CID_MPEG_VIDEO_FRAME_SKIP_MODE	(V4L2_CID_NXP_VPU_BASE + 0x8)
#define V4L2_CID_MPEG_VIDEO_FORCE_I_FRAME	(V4L2_CID_NXP_VPU_BASE + 0x9)
#define V4L2_CID_MPEG_VIDEO_FORCE_SKIP_FRAME	(V4L2_CID_NXP_VPU_BASE + 0xA)

#define V4L2_CID_MPEG_VIDEO_H263_PROFILE	(V4L2_CID_NXP_VPU_BASE + 0xB)
enum v4l2_mpeg_video_h263_profile {
	V4L2_MPEG_VIDEO_H263_PROFILE_P0	= 0,
	V4L2_MPEG_VIDEO_H263_PROFILE_P3	= 1,
};

#define V4L2_CID_MPEG_VIDEO_THUMBNAIL_MODE	(V4L2_CID_NXP_VPU_BASE + 0xC)

#endif
