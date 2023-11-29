// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2020, ITE TECH. INC. All rights reserved.
 */
#include <linux/bits.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/extcon.h>
#include <linux/fs.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <crypto/hash.h>
#include <crypto/sha2.h>
#include <linux/of_graph.h>

#include <linux/videodev2.h>
#include <linux/workqueue.h>
#include <linux/v4l2-dv-timings.h>
#include <media/v4l2-dv-timings.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-mediabus.h>

static int debug;
module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "debug level (0-3)");

#define delay1ms(x) msleep(x)

#define DPCD_VERSION 0x12
#define DPCD_LANE_CNT 4

//0x06:1.65G
//0x0A:2.7G
//0x14:5.4G
#define DPCD_BITRATE 0x14

// 0 : receptacle, long cable
// 1 : genernal TYPE-C cable
// 2 : short TYPE-C cable
#define DEFAULT_RS_LEVEL 1
#define MAX_RS_LEVEL 2

#define MIPI_DATA_LANES 4
#define MBUS_FMT_CODE MEDIA_BUS_FMT_UYVY8_2X8

//#define USE_ANALOG_CDR
#define USE_XTL_CK_SRC

#define MIPI_I2C_ADR 0xAC
#define RAM_I2C_ADR 0xB2


#define DP_REG_INT_STS_07 0x07
#define DP_REG_INT_STS_08 0x08
#define DP_REG_INT_STS_09 0x09
#define DP_REG_INT_STS_0A 0x0A
#define DP_REG_INT_STS_0B 0x0B
#define DP_REG_INT_STS_0C 0x0C
#define DP_REG_INT_STS_0D 0x0D
#define DP_REG_INT_STS_0E 0x0E
#define DP_REG_INT_STS_0F 0x0F
#define DP_REG_INT_STS_2B 0x2B
#define DP_REG_INT_STS_2C 0x2C
#define DP_REG_INT_STS_2D 0x2D

#define DP_REG_INT_MASK_07 0xD1
#define DP_REG_INT_MASK_08 0xD2
#define DP_REG_INT_MASK_09 0xD3
#define DP_REG_INT_MASK_0A 0xD4
#define DP_REG_INT_MASK_0B 0xD5
#define DP_REG_INT_MASK_0C 0xD6
#define DP_REG_INT_MASK_0D 0xD7
#define DP_REG_INT_MASK_0E 0xD8
#define DP_REG_INT_MASK_0F 0xD9
#define DP_REG_INT_MASK_10 0xDA
#define DP_REG_INT_MASK_11 0xDB
#define DP_REG_INT_MASK_2D 0xDC
#define DP_REG_INT_MASK_2C 0xDD
#define DP_REG_INT_MASK_2B 0xDE

enum
{
	AUD_LPCM = 0x00,
	AUD_HBR  = 0x01,
	AUD_3DLPCM  = 0x02,
	AUD_OneBit  = 0x03,
	AUD_DST  = 0x04,

};

enum
{
	AUD32K  = 0x03,
	AUD64K  = 0x0B,
	AUD128K = 0x2B,
	AUD256K = 0x1B,
	AUD512K = 0x3B,

	AUD44K  = 0x00,
	AUD88K  = 0x08,
	AUD176K = 0x0C,
	AUD352K = 0x0D,
	AUD705K = 0x2D,

	AUD48K  = 0x02,
	AUD96K  = 0x0A,
	AUD192K = 0x0E,
	AUD384K = 0x05,
	AUD768K = 0x09,
};

enum
{
	VSC_COLOR_RGB = 0x00,
	VSC_COLOR_YUV444 = 0x01,
	VSC_COLOR_YUV422 = 0x02,
	VSC_COLOR_YUV420 = 0x03,
	VSC_COLOR_YONLY,
	VSC_COLOR_RAW,
	VSC_COLOR_RESERVE
};


enum
{
	COLOR_RGB = 0x00,
	COLOR_YUV422 = 0x01,
	COLOR_YUV444 = 0x02,
	COLOR_YUV420 = 0x03,
	COLOR_RESERVE
};

enum
{
	DP_COLOR_6BIT = 0x00,
	DP_COLOR_8BIT = 0x01,
	DP_COLOR_10BIT = 0x02,
	DP_COLOR_12BIT = 0x03,
	DP_COLOR_16BIT = 0x04,

};

enum
{
	COLORIMETRY_BT601 = 0x00,
	COLORIMETRY_BT709 = 0x01,
	COLORIMETRY_xvYCC601 = 0x02,
	COLORIMETRY_xvYCC709 = 0x03,
	COLORIMETRY_sYCC601 = 0x04,
	COLORIMETRY_aYCC601 = 0x05,
	COLORIMETRY_BT2020YcCbcCrc = 0x06,
	COLORIMETRY_BT2020YCbCr = 0x07,
	COLORIMETRY_RESERVE
};

enum
{
	COLORIMETRY_sRGB = 0x00,
	COLORIMETRY_fixRGB = 0x01,
	COLORIMETRY_scRGB = 0x02,
	COLORIMETRY_aRGB = 0x03,
	COLORIMETRY_DCIP3 = 0x04,
	COLORIMETRY_CUSTOM = 0x05,
	COLORIMETRY_BT2020RGB = 0x06

};


enum
{
	CSI_RGB10b = 0x25,
	CSI_RGB888 = 0x24,
	CSI_RGB666 = 0x23,
	CSI_RGB565 = 0x22,
	CSI_RGB555 = 0x21,
	CSI_RGB444 = 0x20,
	CSI_YCbCr4208b = 0x1A,
	CSI_YCbCr4228b = 0x1E,
	CSI_YCbCr42210b = 0x1F,
	CSI_YCbCr42212b = 0x30
};


enum
{
	CSC_BYPASS =  0x00,
	CSC_RGB2YUV = 0x02,
	CSC_YUV2RGB = 0x03,

};

enum
{
	AUDIO_OFF = 0x00,
	AUDIO_I2S = 0x01,
	AUDIO_SPDIF = 0x02,
};

struct csi_config
{
	u8 lane;
	u8 type;
	u8 reg23_p2m;
	u8 reg25_p2mdly;
	u8 regb0_div[3];
};

struct csi_bus_para
{
	struct csi_config cfg;
	u8 swap_pn;
	u8 swap_lan;
	u8 pclk_inv;
	u8 mclk_inv;
	u8 lpx_num;
};

struct mipi_bus
 {
	u8 lane_cnt;
	u8 data_type;
	u32 mbus_fmt_code;
	struct csi_bus_para bus;
 };

struct color_format
{
	unsigned char color_mode;
	unsigned char color_depth;
	unsigned char color_cea_range;
	unsigned char color_colorietry;
	unsigned char color_ex_colorietry;
	unsigned char content_type;

};

struct dprx_data {
	
	u8 support_hdcp;
	u8 support_hdcp2;
	
	u8 video_stable;
	u8 audio_stable;
	u8 audio_i2s;
	u8 link_train_done;
	u8 issue_hpd_irq;
	u8 rs_level;

	u8 audio_fs_force;
	u8 guess_fs;
};


struct dpcd_config
{
	u8 version;
	u8 lane;
	u8 bitrate;

};

struct it6510 {
	struct i2c_client *dp_i2c;
	struct i2c_client *mipi_i2c;
	struct i2c_client *edid_i2c;
	struct regmap *dp_regmap;
	struct regmap *mipi_regmap;

	struct regmap *edid_regmap;

	struct v4l2_fwnode_bus_mipi_csi2 bus;
	struct v4l2_subdev sd;
	struct v4l2_ctrl_handler hdl;
	struct media_pad pad;
	struct v4l2_dv_timings timings;

	struct dpcd_config dpcd;

	u8 audio_wait;
	struct delayed_work delayed_audio;

	struct mutex lock;
	struct dprx_data dprx;
	struct mipi_bus csi;
	struct color_format color_fmt;

	struct delayed_work delayed_link_check;

	wait_queue_head_t wq;

	u8 power_up;
	u8 edid_data[256];


	u8 attr_enable_stream;
	u8 attr_dp_reg_bank;

	bool streaming;
};

static inline struct it6510 *sd_to_it6510(struct v4l2_subdev *sd)
{
	return container_of(sd, struct it6510, sd);
}

#define IT6510_DP_MIN_WIDTH		640
#define IT6510_DP_MAX_WIDTH		1920
#define IT6510_DP_MIN_HEIGHT		480
#define IT6510_DP_MAX_HEIGHT		1200

/* V4L2_DV_BT_CEA_720X480P59 - 27 MHz */
#define IT6510_DP_MIN_PIXELCLOCK	27000000
/* V4L2_DV_BT_DMT_1600X1200P60 */
#define IT6510_DP_MAX_PIXELCLOCK	162000000

static const struct v4l2_dv_timings_cap it6510_timings_cap = {
	.type = V4L2_DV_BT_656_1120,
	/* keep this initialization for compatibility with GCC < 4.4.6 */
	.reserved = { 0 },

	V4L2_INIT_BT_TIMINGS(IT6510_DP_MIN_WIDTH, IT6510_DP_MAX_WIDTH,
				IT6510_DP_MIN_HEIGHT, IT6510_DP_MAX_HEIGHT,
				IT6510_DP_MIN_PIXELCLOCK, IT6510_DP_MAX_PIXELCLOCK,
				V4L2_DV_BT_STD_CEA861 | V4L2_DV_BT_STD_DMT |
				V4L2_DV_BT_STD_GTF | V4L2_DV_BT_STD_CVT,
				V4L2_DV_BT_CAP_PROGRESSIVE |
				V4L2_DV_BT_CAP_REDUCED_BLANKING)
};




#define dprx_msg_err(...) dev_err(&it6510->dp_i2c->dev, ## __VA_ARGS__)
#define mipi_msg_err(...) dev_err(&it6510->mipi_i2c->dev, ## __VA_ARGS__)
#define dprx_msg_info(...) dev_info(&it6510->dp_i2c->dev, ## __VA_ARGS__)
#define mipi_msg_info(...) dev_info(&it6510->mipi_i2c->dev, ## __VA_ARGS__)
#define dprx_msg_dbg(...) dev_dbg(&it6510->dp_i2c->dev, ## __VA_ARGS__)
#define mipi_msg_dbg(...) dev_dbg(&it6510->mipi_i2c->dev, ## __VA_ARGS__)

static void dprx_link_status_change(struct it6510 *it6510, u8 status);
static void dprx_config_color_transfer(struct it6510 *it6510, struct color_format* video_in);
static void dprx_get_color_from_misc(struct it6510 *it6510, struct color_format *video_para);
static u16 dprx_get_pclk(struct it6510 *it6510);
static void dprx_get_bt_timing(struct it6510 *it6510);

static const u8 default_edid[] = 
{
	#if 0
	0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,
	0x10,0xAC,0xEF,0x40,0x4D,0x33,0x37,0x32,
	0x1C,0x1A,0x01,0x04,0x80,0x3E,0x22,0x78,
	0xEA,0x08,0xA5,0xA2,0x57,0x4F,0xA2,0x28,
	0x0F,0x50,0x54,0xA5,0x4B,0x00,0xD1,0xC0,
	0xA9,0x40,0x81,0x80,0x81,0x00,0x71,0x4F,
	0xE1,0x00,0x01,0x01,0x01,0x01,0x08,0xE8,
	0x00,0x30,0xF2,0x70,0x5A,0x80,0xB0,0x58,
	0x8A,0x00,0x6D,0x55,0x21,0x00,0x00,0x1E,
	0x00,0x00,0x00,0xFF,0x00,0x47,0x44,0x34,
	0x35,0x50,0x36,0x37,0x47,0x32,0x37,0x33,
	0x4D,0x0A,0x00,0x00,0x00,0xFC,0x00,0x44,
	0x45,0x4C,0x4C,0x20,0x53,0x32,0x38,0x31,
	0x37,0x51,0x0A,0x20,0x00,0x00,0x00,0xFD,
	0x00,0x1D,0x4B,0x1F,0x8C,0x3C,0x00,0x0A,
	0x20,0x20,0x20,0x20,0x20,0x20,0x01,0xE5,
	
	0x02,0x03,0x3B,0xF1,0x55,0x61,0x60,0x5F,
	0x5E,0x5D,0x10,0x1F,0x20,0x05,0x14,0x04,
	0x13,0x12,0x11,0x03,0x02,0x16,0x15,0x07,
	0x06,0x01,0x23,0x09,0x07,0x07,0x83,0x01,
	0x00,0x00,0x6D,0x03,0x0C,0x00,0x10,0x00,
	0x00,0x3C,0x20,0x00,0x60,0x01,0x02,0x03,
	0x67,0xD8,0x5D,0xC4,0x01,0x78,0xC0,0x03,
	0xE2,0x0F,0x03,0x04,0x74,0x00,0x30,0xF2,
	0x70,0x5A,0x80,0xB0,0x58,0x8A,0x00,0x6D,
	0x55,0x21,0x00,0x00,0x1E,0x4D,0x6C,0x80,
	0xA0,0x70,0x70,0x3E,0x80,0x30,0x20,0x3A,
	0x00,0x6D,0x55,0x21,0x00,0x00,0x1A,0x56,
	0x5E,0x00,0xA0,0xA0,0xA0,0x29,0x50,0x30,
	0x20,0x35,0x00,0x6D,0x55,0x21,0x00,0x00,
	0x1A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1F
	#else
	
	0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,
	0x10,0xAC,0xEE,0x40,0x4D,0x33,0x37,0x32,
	0x1C,0x1A,0x01,0x04,0xB5,0x3E,0x22,0x78,
	0x3A,0x08,0xA5,0xA2,0x57,0x4F,0xA2,0x28,
	0x0F,0x50,0x54,0xA5,0x4B,0x00,0xD1,0xC0,
	0xA9,0x40,0x81,0x80,0x81,0x00,0x71,0x4F,
	0x01,0x01,0x01,0x01,0x01,0x01,0x4D,0xD0,
	0x00,0xA0,0xF0,0x70,0x3E,0x80,0x30,0x20,
	0x35,0x00,0x6D,0x55,0x21,0x00,0x00,0x1A,
	0x00,0x00,0x00,0xFF,0x00,0x47,0x44,0x34,
	0x35,0x50,0x36,0x37,0x47,0x32,0x37,0x33,
	0x4D,0x0A,0x00,0x00,0x00,0xFC,0x00,0x44,
	0x45,0x4C,0x4C,0x20,0x53,0x32,0x38,0x31,
	0x37,0x51,0x0A,0x20,0x00,0x00,0x00,0xFD,
	0x00,0x1D,0x4B,0x1F,0x8C,0x36,0x01,0x0A,
	0x20,0x20,0x20,0x20,0x20,0x20,0x01,0xD7,
	0x02,0x03,0x1D,0xF1,0x50,0x10,0x1F,0x20,
	0x05,0x14,0x04,0x13,0x12,0x11,0x03,0x02,
	0x16,0x15,0x07,0x06,0x01,0x23,0x09,0x07,
	0x07,0x83,0x01,0x00,0x00,0xA3,0x66,0x00,
	0xA0,0xF0,0x70,0x1F,0x80,0x30,0x20,0x35,
	0x00,0x6D,0x55,0x21,0x00,0x00,0x1A,0x56,
	0x5E,0x00,0xA0,0xA0,0xA0,0x29,0x50,0x30,
	0x20,0x35,0x00,0x6D,0x55,0x21,0x00,0x00,
	0x1A,0x4D,0x6C,0x80,0xA0,0x70,0x70,0x3E,
	0x80,0x30,0x20,0x3A,0x00,0x6D,0x55,0x21,
	0x00,0x00,0x1A,0x11,0x44,0x00,0xA0,0x80,
	0x00,0x25,0x50,0x30,0x20,0x36,0x00,0x6D,
	0x55,0x21,0x00,0x00,0x1A,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xDB,
#endif

};

static const struct csi_config it6510_csi_bus_cfg[] =
{
	{4, CSI_RGB10b, 0x8F, 0x03, {0xEE, 0xEE, 0xEE}},
	{4, CSI_RGB888, 0x23, 0x03, {0x65, 0x22, 0x22}},
	{4, CSI_RGB666, 0x89, 0x03, {0xE8, 0xE8, 0xE8}},
	{4, CSI_RGB565, 0x22, 0x02, {0x63, 0x21, 0x00}},
	{4, CSI_RGB555, 0x22, 0x02, {0x63, 0x21, 0x00}},
	{4, CSI_RGB444, 0x22, 0x02, {0x63, 0x21, 0x00}},
	{4, CSI_YCbCr4208b, 0x23, 0x03, {0x65, 0x22, 0x22}},
	{4, CSI_YCbCr4228b, 0x22, 0x02, {0x63, 0x21, 0x00}},
	{4, CSI_YCbCr42210b, 0x45, 0x03, {0x64, 0x64, 0x64}},
	{4, CSI_YCbCr42212b, 0x23, 0x03, {0x65, 0x22, 0x22}},

	{2, CSI_RGB10b, 0x4F, 0x04, {0x6E, 0x6E, 0x6E}},
	{2, CSI_RGB888, 0x13, 0x04, {0x6B, 0x25, 0x25}},
	{2, CSI_RGB666, 0x49, 0x04, {0x68, 0x68, 0x68}},
	{2, CSI_RGB565, 0x12, 0x03, {0x67, 0x23, 0x01}},
	{2, CSI_RGB555, 0x12, 0x03, {0x67, 0x23, 0x01}},
	{2, CSI_RGB444, 0x12, 0x03, {0x67, 0x23, 0x01}},
	{2, CSI_YCbCr4208b, 0x13, 0x04, {0x6B, 0x25, 0x25}},
	{2, CSI_YCbCr4228b, 0x12, 0x03, {0x67, 0x23, 0x01}},
	{2, CSI_YCbCr42210b, 0x25, 0x04, {0x69, 0x24, 0x25}},
	{2, CSI_YCbCr42212b, 0x13, 0x04, {0x6B, 0x25, 0x25}},

	{1, CSI_RGB10b, 0x2F, 0x0B, {0x7D, 0x2E, 0x2E}},
	{1, CSI_RGB888, 0x03, 0x0B, {0x77, 0x2B, 0x05}},
	{1, CSI_RGB666, 0x29, 0x0B, {0x71, 0x28, 0x28}},
	{1, CSI_RGB565, 0x02, 0x0B, {0x6F, 0x27, 0x03}},
	{1, CSI_RGB555, 0x02, 0x0B, {0x6F, 0x27, 0x03}},
	{1, CSI_RGB444, 0x02, 0x0B, {0x6F, 0x27, 0x03}},
	{1, CSI_YCbCr4208b, 0x03, 0x0B, {0x77, 0x2B, 0x05}},
	{1, CSI_YCbCr4228b, 0x02, 0x0B, {0x6F, 0x27, 0x03}},
	{1, CSI_YCbCr42210b, 0x15, 0x0B, {0x73, 0x29, 0x04}},
	{1, CSI_YCbCr42212b,  0x03, 0x0B, {0x77, 0x2B, 0x05}},
	{0,0,0,0,{0,0,0}},
};




/*bCSCMtx_RGB2YUV_ITU601_16_235*/   
static const u8 cscmtx_rgb2yuv_itu601_16_235[] = 
{
	0x00, 0x80, 0x10, 0xB2, 0x04, 0x65,
	0x02, 0xE9, 0x00, 0x93, 0x3C, 0x18,
	0x04, 0x55, 0x3F, 0x49, 0x3D, 0x9F,
	0x3E, 0x18, 0x04, 0x00
};

/*bCSCMtx_RGB2YUV_ITU601_00_255*/   
static const u8 cscmtx_rgb2yuv_itu601_0_255[] = 
{
	0x10, 0x80, 0x10, 0x09, 0x04, 0x0E,
	0x02, 0xC9, 0x00, 0x0F, 0x3D, 0x84,
	0x03, 0x6D, 0x3F, 0xAB, 0x3D, 0xD1,
	0x3E, 0x84, 0x03, 0x00
};


static const u8 cscmtx_rgb2yuv_itu709_16_235[] = 
{	
	0x00, 0x80, 0x10, 0xB8, 0x05, 0xB4,
	0x01, 0x94, 0x00, 0x4A, 0x3C, 0x17,
	0x04, 0x9F, 0x3F, 0xD9, 0x3C, 0x10,
	0x3F, 0x17, 0x04, 0x00
};

static const u8 cscmtx_rgb2yuv_itu709_0_255[] = 
{	0x10, 0x80, 0x10, 0xEA, 0x04, 0x77, 
	0x01, 0x7F, 0x00, 0xD0, 0x3C, 0x83, 
	0x03, 0xAD, 0x3F, 0x4B, 0x3D, 0x32, 
	0x3F, 0x83, 0x03, 0x00
};

static const u8 cscmtx_yuv2rgb_itu709_16_235[] = 
{
	0x00, 0x00, 0x00, 0x00, 0x08, 0x55,
	0x3C, 0x88, 0x3E, 0x00, 0x08, 0x51,
	0x0C, 0x00, 0x00, 0x00, 0x08, 0x00,
	0x00, 0x84, 0x0E, 0x00
};



u8 it6510_read(struct regmap *regmap, u8 reg)
{	
	int val;
	regmap_read(regmap, reg, &val);
	return (u8)val;
}

int it6510_write(struct regmap *regmap, u8 reg, u8 value)
{
	return regmap_write(regmap, reg, value);
}

u8 dprx_read(struct regmap *regmap, u8 reg)
{
	return it6510_read(regmap, reg);
}

int dprx_write(struct regmap *regmap, u8 reg, u8 value)
{
	return regmap_write(regmap, reg, value);
}


int dprxset(struct regmap *regmap, u8 reg, u8 mask, u8 value)
{
	return regmap_update_bits(regmap, reg, mask, value);
}


u8 mipi_read(struct regmap *regmap, u8 reg)
{
	return it6510_read(regmap, reg);
}

int mipi_write(struct regmap *regmap, u8 reg, u8 value)
{
	return regmap_write(regmap, reg, value);
}

int mipiset(struct regmap *regmap, u8 reg, u8 mask, u8 value)
{
	return regmap_update_bits(regmap, reg, mask, value);
}


void dprxchgbank(struct regmap *regmap, unsigned char bank)
{
	dprxset(regmap, 0x05, 0x0F, bank);
}


static void dprx_init_dpcd(struct it6510 *it6510)
{
	struct dpcd_config *dpcd = &it6510->dpcd;

	dpcd->version = DPCD_VERSION;
	dpcd->lane = DPCD_LANE_CNT;
	dpcd->bitrate = DPCD_BITRATE;

}


static void mipi_init_csi_bus_para(struct it6510 *it6510)
{
	struct csi_bus_para *csi_para = &it6510->csi.bus;

	csi_para->swap_pn = false;
	csi_para->swap_lan = false;
	csi_para->pclk_inv = false;
	csi_para->mclk_inv = true;
	csi_para->lpx_num = 3;

}

static void mipi_csi_initial(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;
	struct regmap *mipi = it6510->mipi_regmap;
	struct csi_bus_para *csi_para = &it6510->csi.bus;
	u8 lan_num = it6510->csi.lane_cnt - 1;
 
	dprxchgbank(dp, 3);
	dprxset(dp, 0x55, 0x07, 0x05);
	dprxchgbank(dp, 1);
	dprxset(dp, 0x97, 0x1f, 0x14);
	dprxchgbank(dp, 0);

	mipiset(mipi, 0x05, 0x09, 0x09);
	mipi_write(mipi, 0x05, 0x36);

	mipiset(mipi, 0x21, 0x30, lan_num << 4);
	mipiset(mipi, 0x2A, 0x3c, 0x08); //FS sync to Vsync
	mipiset(mipi, 0x28, 0x0c, csi_para->swap_pn << 3 |
				csi_para->swap_lan << 2);
	mipiset(mipi, 0x10, 0x06, csi_para->pclk_inv << 2 |
				csi_para->mclk_inv << 1);
	mipiset(mipi, 0x8c, 0x40, 0x00);
	mipiset(mipi, 0x47, 0xf0, 0x10);
	mipiset(mipi, 0x44, 0x04, 0x04);
	mipiset(mipi, 0x45, 0x0f, csi_para->lpx_num);
	mipiset(mipi, 0x3c, 0x20, 0x20);
	mipiset(mipi, 0x6b, 0x01, 0x00);
	mipiset(mipi, 0x28, 0x20, 0x00);
	mipiset(mipi, 0x10, 0x80, 0x00);
	mipiset(mipi, 0xc1, 0x03, 0x03);

	mipiset(mipi, 0xa8, 0x01, 0x00);
	//patternGen
	//mipiset(mipi, 0xa8, 0x01, 0x01);
	//mipiset(mipi, 0xa9, 0xFF, 0x70);


	mipi_msg_info("%s\n", __func__);
	mipi_msg_dbg("csi lan_num %d\n", it6510->csi.lane_cnt);
	mipi_msg_dbg("csi swap_pn %d\n", csi_para->swap_pn);
	mipi_msg_dbg("csi swap_lan  %d\n", csi_para->swap_lan);
	mipi_msg_dbg("csi pclk_inv %d\n", csi_para->pclk_inv);
	mipi_msg_dbg("csi mclk_inv %d\n", csi_para->mclk_inv);
	mipi_msg_dbg("csi lpx_num %d\n", csi_para->lpx_num);

}

static int csi_get_bus_para(struct it6510 *it6510)
{
	struct mipi_bus *csi = &it6510->csi;
	int i;

	for (i = 0; it6510_csi_bus_cfg[i].lane; i++) {
		if(it6510_csi_bus_cfg[i].lane == csi->lane_cnt &&
			it6510_csi_bus_cfg[i].type == csi->data_type){
			
			csi->bus.cfg = it6510_csi_bus_cfg[i];
			mipi_msg_dbg("csi_get_bus_para = %d \n", i);
			return 0;
		}
	}
	
	mipi_msg_err("csi_get_bus_para() err \n");
	return -1;
}

__maybe_unused static void mipitx_setup_patternGen(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;
	struct regmap *mipi = it6510->mipi_regmap;

	u16 htotal, hdes, hdew, hfph, hsyncw, hdee, hre;
	u16 vtotal, vdes, vdew, vfph, vsyncw, vdee, vre;
	u16 vsyncpol, hsyncpol, interlace;
	
	dprxchgbank(dp, 6);

	htotal = dprx_read(dp, 0x5F) << 8;
	htotal += dprx_read(dp, 0x5E);
	
	hdes = dprx_read(dp, 0x61) << 8;
	hdes += dprx_read(dp, 0x60);

	hdew = dprx_read(dp, 0x63) << 8;
	hdew += dprx_read(dp, 0x62);
	
	hsyncw = dprx_read(dp, 0x6B) & 0x7F << 8;
	hsyncw += dprx_read(dp, 0x6A);

	hfph = htotal - hdew - hdes;
    hdee = hdes + hdew;
    hre = hfph + hsyncw;

	hsyncpol = dprx_read(dp, 0x6B) & 0x80 ? 1 : 0;

	vtotal = dprx_read(dp, 0x65) << 8;
	vtotal += dprx_read(dp, 0x64);
	
	vdes = dprx_read(dp, 0x67) << 8;
	vdes += dprx_read(dp, 0x66);
	
	vdew = dprx_read(dp, 0x69) << 8;
	vdew += dprx_read(dp, 0x68);
	
	vsyncw = dprx_read(dp, 0x6D) & 0x7F << 8;
	vsyncw += dprx_read(dp, 0x6C);
	
	vfph = vtotal - vdew - vdes;
    vdee = vdes + vdew;
    vre = vfph + vsyncw;


	vsyncpol = dprx_read(dp, 0x6D) & 0x80 ? 1 : 0;

	interlace = dprx_read(dp, 0x70) & 0x04 ? 1 : 0;

	dprxchgbank(dp, 0);

	mipi_msg_info("mipitx_setup_patternGen()!\n");
	mipi_msg_dbg("htotal = %d\n", htotal);
	mipi_msg_dbg("hdew = %d\n", hdew);
	mipi_msg_dbg("hdes = %d\n", hdes);
	mipi_msg_dbg("hfph = %d\n", hfph);
	mipi_msg_dbg("hsyncw = %d\n", hsyncw);
	mipi_msg_dbg("hsyncpol = %d\n",hsyncpol);
	mipi_msg_dbg("vtotal = %d\n", vtotal);
	mipi_msg_dbg("vdes = %d\n", vdes);
	mipi_msg_dbg("vdew = %d\n", vdew);
	mipi_msg_dbg("vfph = %d\n", vfph);
	mipi_msg_dbg("vsyncw = %d\n", vsyncw);
	mipi_msg_dbg("vsyncpol = %d\n", vsyncpol);
	mipi_msg_dbg("interlaced mode = %d\n", interlace);

    mipiset(mipi,0x90, 0x02, hsyncpol<<1);//hpol
    mipiset(mipi,0x90, 0x04, vsyncpol<<2);//vpol

    mipiset(mipi,0x90, 0xf0, (htotal&0x0F)<<4);//htotal[3:0]
    mipiset(mipi,0x91, 0xff, htotal>>4);//htotal[11:4]
    mipiset(mipi,0x8f, 0x01, htotal>>12);

    
    mipiset(mipi,0x92, 0xff, hdes&0xFF);//hdes[7:0] 
    mipiset(mipi,0x94, 0x0f, hdes>>8); //reg_94[3:0]=>hdes[11:8]
    mipiset(mipi,0x8f, 0x02, hdes>>11); //(HDES>>12)<<1

    mipiset(mipi,0x93, 0xff, hdee&0xFF);//hdee[7:0]
    mipiset(mipi,0x94, 0xf0, hdee>>4);//reg_94[7:4]=>hdee[11:8]
    mipiset(mipi,0x8f, 0x04, hdee>>10);//(HDES>>12)<<2

    mipiset(mipi,0x95, 0xff, hfph);//hrs[7:0]
    mipiset(mipi,0x97, 0x0f, hfph>>8);
    mipiset(mipi,0x8f, 0x08, hfph>>9); //(HDES>>12)<<3

    mipiset(mipi,0x96, 0xff, hre);//hre[7:0]
    mipiset(mipi,0x97, 0xf0, hre>>8);
    mipiset(mipi,0x8f, 0x10, hre>>8); //(HRE>>12)<<4

    mipiset(mipi,0x98, 0xff, vtotal&0xFF);//vtotal
    mipiset(mipi,0x99, 0x0f, vtotal>>8);

    mipiset(mipi,0x9a, 0xff, vdes&0xFF);//vdes
    mipiset(mipi,0x9c, 0x0f, vdes>>8);

    mipiset(mipi,0x9b, 0xff, vdee);//vdee
    mipiset(mipi,0x9c, 0xf0, vdee>>4);

    mipiset(mipi,0xa0, 0xff, vfph);//vrs
    mipiset(mipi,0xa1, 0x0f, vfph>>8);
    mipiset(mipi,0xa1, 0xf0, vre<<4);//vre
    mipiset(mipi,0xa6, 0x0f, vre>>4);

    //mipiset(mipi,0x9d, 0xff, 0x00);//vdes2
    //mipiset(mipi,0x9f, 0x0f, 0x00);
    //mipiset(mipi,0x9e, 0xff, 0x00);//vdee2
    //mipiset(mipi,0x9f, 0xf0, 0x00);
    //mipiset(mipi,0xa2, 0xff, 0x00);//vrs2
    //mipiset(mipi,0xa3, 0x0f, 0x00);
    //mipiset(mipi,0xa3, 0xf0, 0x00);//vre2
    //mipiset(mipi,0xa6, 0xf0, 0x00);

    mipiset(mipi,0xa5, 0xff, 0x00);

    mipiset(mipi,0xaf, 0xff, 0x00);//Hinc
    mipiset(mipi,0xa7, 0xff, 0x00);//Vinc
    mipiset(mipi,0xa9, 0x3f, 0x00);//sel color option
    mipiset(mipi,0xaa, 0xff, 0x00);//int R/Cr
    mipiset(mipi,0xab, 0xff, 0x32);//int G/Y
    mipiset(mipi,0xac, 0xff, 0x00);//int B/Cb
    mipiset(mipi,0xa8, 0x01, 0x01);//RegPGEN

}


static void mipitx_setup_csi(struct it6510 *it6510)
{
	struct regmap *mipi = it6510->mipi_regmap;
	struct csi_config *csi_cfg = &it6510->csi.bus.cfg;
	struct v4l2_bt_timings *bt = &it6510->timings.bt;

	u16 pixelclock_mhz, mipiclock_mhz, hs_clock_mhz;
	u16 mipiclock_ns, mipiclock_10ns, mipi_ui;

	u16 reg_hs_prepare_zero, reg_hs_trail;

	u8 mpdiv_sel, mpll_div, mpre_div;
	u8 p2m_dly, p2m_time;
	u8 regb0;

	u8 en_fsfrnum = false;

	//debug only
	//mipitx_setup_patternGen(it6510);

	//no u64 divid provid in lib
	//pixelclock_mhz = bt->pixelclock / 1000000;
	//pixelclock_mhz = bt->pixelclock >> 6;
	//pixelclock_mhz /= 15625;
	
	pixelclock_mhz = dprx_get_pclk(it6510);

	mipi_msg_dbg("pixelclock_mhz = %u\n", pixelclock_mhz);
	
	if (pixelclock_mhz <= 0)
		return;

	if (csi_get_bus_para(it6510) != 0)
		return;

	mipi_msg_info("it6510_csi_update_bus_para\n");
	mipi_msg_dbg("{%X, %X, %X, %X, %X, %X ,%X}\n",
		csi_cfg->lane,
		csi_cfg->type,
		csi_cfg->reg23_p2m,
		csi_cfg->reg25_p2mdly,
		csi_cfg->regb0_div[0],
		csi_cfg->regb0_div[1],
		csi_cfg->regb0_div[2]);

	//mipi_msg_dbg("pixelclock_mhz = %u\n", pixelclock_mhz);


	if (pixelclock_mhz > 200)
		mpdiv_sel = 0;  //regb0 = csi_cfg.regb0_div[0];
	else if (pixelclock_mhz > 100)
		mpdiv_sel = 1;	//regb0 = csi_cfg.regb0_div[1];
	else
		mpdiv_sel = 2;  //regb0 = csi_cfg.regb0_div[2];

	mpre_div = (csi_cfg->regb0_div[mpdiv_sel] >> 5) + 1;
	mpll_div = (csi_cfg->regb0_div[mpdiv_sel] & 0x1F) + 1;

	p2m_dly = csi_cfg->reg25_p2mdly;
	p2m_time = csi_cfg->reg23_p2m & 0x0F;

	en_fsfrnum = bt->interlaced == V4L2_DV_INTERLACED ? true : false;

	//todo:
	//Use For pll divider overwrite
	//mpre_div = over_write_mpredivider();
	//mpll_div = over_write_mdivider();

	mipi_msg_dbg("mpdiv_sel %d, mpre_div %d mpll_div %d\n",
			mpdiv_sel, mpre_div, mpll_div);

	if (pixelclock_mhz < 10 * mpre_div ||
		pixelclock_mhz > 100 * mpre_div) {

		mipi_msg_err("MPTX PHY setting wrong !!!\n");
		mipi_msg_err("Need to reset parameter for TXPHY !!!\n");
	}




	#if 1
	//calculate m clock unit use eq 1
	mipiclock_10ns = (20000 * mpre_div) / (pixelclock_mhz * mpll_div);
	
	//link clock info
	mipiclock_mhz =  10000 / mipiclock_10ns;
	hs_clock_mhz = 2 * mipiclock_mhz;
	
	//get m unit with rounding
	mipiclock_ns = mipiclock_10ns / 10;
	
	//Disable Rounding for 4K30 297/296 issue
	//if (mipiclock_10ns % 10 > 4)
	//	mipiclock_ns++;

	//get ui with rounding
	mipi_ui = mipiclock_ns / 4;
	if (mipi_ui % 4 > 2)       
		mipi_ui++;	
	
	#else
	//calculate m clock unit use eq2
	// time with 10, get more significant digit

	hs_clock_mhz = 8 * (pixelclock_mhz * mpll_div) / (2 * mpre_div);
	mipiclock_mhz = hs_clock_mhz / 2;
	mipiclock_10ns = 10000 / mipiclock_mhz;
	mipi_ui = 1 / mipiclock_mhz;
	
	#endif
		
	mipi_msg_dbg("mipiclock_ns %d, mipi_ui %d, mipiclock_mhz %d pixelclock_mhz %d\n",
			mipiclock_ns, mipi_ui, mipiclock_mhz, pixelclock_mhz);

	#if 0
	reg_hs_prepare_zero = (285 - ((3 * mipiclock_ns) >> 1)) / mipiclock_ns;
	reg_hs_trail = ((14 * mipiclock_ns) - 20) / mipiclock_ns;
	reg_hs_prepare_zero += 9;
	reg_hs_trail += 4;
	#endif


	//HS_prepeare_zero > 145ns + 10UI
	reg_hs_prepare_zero = ((285 + (mipi_ui * 10)) / mipiclock_ns) - 3;
	reg_hs_prepare_zero = reg_hs_prepare_zero > 9 ? reg_hs_prepare_zero : 9;
	
	//HS_trail > max(8UI, 60ns + 4UI) > 64ns (UI = 1), 8UI (UI>15)
	//HS_trail < Teot < 105ns + 12UI
	reg_hs_trail = 4 + ((5 + (12 * mipi_ui)) / mipiclock_ns);
	
	regb0 = ((mpre_div - 1) << 5) | ((mpll_div - 1) & 0x1F);
	
	mipi_msg_dbg("en_fsfrnum = %d !!!\n", en_fsfrnum);
	mipi_msg_dbg("reg_hs_prepare_zero %X(%d)\n", reg_hs_prepare_zero, reg_hs_prepare_zero);
	mipi_msg_dbg("reg_hs_trail %X(%d)\n", reg_hs_trail, reg_hs_trail);
	
	mipi_msg_dbg("reg23 = %X !!!\n", csi_cfg->reg23_p2m);
	mipi_msg_dbg("reg25 = %X !!!\n", csi_cfg->reg25_p2mdly);
	mipi_msg_dbg("regb0 = %X !!!\n", regb0);
	
	mipi_write(mipi, 0x1F, reg_hs_trail);
	mipi_write(mipi, 0x20, csi_cfg->type);
	mipiset(mipi, 0x21, 0x30, (csi_cfg->lane - 1) << 4);
	mipi_write(mipi, 0x22, reg_hs_prepare_zero);
	mipi_write(mipi, 0x23, csi_cfg->reg23_p2m);
	mipi_write(mipi, 0x24, 0x20);
	mipi_write(mipi, 0x25, csi_cfg->reg25_p2mdly);
	mipiset(mipi, 0x26, 0x20, (en_fsfrnum << 5));
	mipi_write(mipi, 0x27, 0x02);
	mipi_write(mipi, 0xb0, regb0);
	
	mipi_write(mipi, 0x05, 0x00);
}


static void mipitx_disable_csi(struct it6510 *it6510)
{
	struct regmap *mipi = it6510->mipi_regmap;

	mipi_write(mipi, 0x05, 0x36);
}


static void mipitx_irq(struct it6510 *it6510)
{
	struct regmap *mipi = it6510->mipi_regmap;
	u8 reg09h;
	u8 reg0ah;
	u8 reg0bh;

	reg09h = mipi_read(mipi, 0x09);
	reg0ah = mipi_read(mipi, 0x0A);
	reg0bh = mipi_read(mipi, 0x0B);

	// mipi_msg_dbg("reg09h = %x\n", reg09h);
	// mipi_msg_dbg("reg0ah = %x\n", reg0ah);
	// mipi_msg_dbg("reg0bh = %x\n", reg0bh);

	if(reg0bh & 0x10) {
		mipi_msg_dbg("mipi tx VidStb Change ...\n");
		mipi_msg_dbg("mipi video status = %d", reg09h & 0x40 );
		mipi_write(mipi, 0x0B, 0x10);
	}

	if (reg0ah & 0x70) {

		if(reg0ah & 0x20)
			mipi_msg_err("Mipi Byte mismatch Err!!! \n");


		if(reg0ah & 0x40)
			mipi_msg_err("mipi P2M FIFO Err!!! \n");

		mipi_write(mipi, 0x0A, reg0ah & 0x70);
	}
}


static void dprx_get_edid_ram(struct it6510 *it6510, u8 *buf)
{

	struct regmap *dp = it6510->dp_regmap;
	struct regmap *edid = it6510->edid_regmap;

	dprxchgbank(dp, 2);
	dprxset(dp, 0x24, 0x03, 0x00);
	dprxchgbank(dp, 0);


	regmap_bulk_read(edid, 0, buf, 256);

	dprxchgbank(dp, 2);
	dprxset(dp, 0x24, 0x03, 0x03);
	dprxchgbank(dp, 0);
}

static void dprx_init_edid_ram(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;
	struct regmap *edid = it6510->edid_regmap;

	dprxchgbank(dp, 2);
	dprxset(dp, 0x24, 0x03, 0x00);
	dprxchgbank(dp, 0);


	regmap_bulk_write(edid, 0, it6510->edid_data, 256);

	dprxchgbank(dp, 2);
	dprxset(dp, 0x24, 0x03, 0x03);
	dprxchgbank(dp, 0);
}


static int it6510_read_ids(struct it6510 *it6510)
{
	struct device *dev = &it6510->dp_i2c->dev;
	int ret;
	u8 dp_id[5];

	ret = regmap_bulk_read(it6510->dp_regmap, 0, dp_id, 5);

	if (ret) {
		dev_err(dev, "dp regmap_bulk_read failed %d\n", ret);
		return ret;
	}

	dev_info(dev, "dp:%02X%02X%02X%02X%02X\n",
			dp_id[0],dp_id[1],dp_id[2],dp_id[3],dp_id[4]);

	return 0;
}


static void it6510_debug_caof(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;

	u8 reg734,reg735,reg736,reg737;
	u8 reg738,reg739,reg73a,reg73b;
	u8 reg73c,reg73d,reg73e,reg73f;
	u8 reg770,reg771;

	u8 l0_caof_a, l0_caof_b, l0_caof_c, l0_caof_d;
	u8 l1_caof_a, l1_caof_b, l1_caof_c, l1_caof_d;
	u8 l2_caof_a, l2_caof_b, l2_caof_c, l2_caof_d;
	u8 l3_caof_a, l3_caof_b, l3_caof_c, l3_caof_d;
	u8 reg717;

	reg734 = dprx_read(dp, 0x34);
	reg735 = dprx_read(dp, 0x35);
	reg736 = dprx_read(dp, 0x36);
	reg737 = dprx_read(dp, 0x37);
	reg738 = dprx_read(dp, 0x38);
	reg739 = dprx_read(dp, 0x39);
	reg73a = dprx_read(dp, 0x3A);
	reg73b = dprx_read(dp, 0x3B);
	reg73c = dprx_read(dp, 0x3C);
	reg73d = dprx_read(dp, 0x3D);
	reg73e = dprx_read(dp, 0x3E);
	reg73f = dprx_read(dp, 0x3F);
	reg770 = dprx_read(dp, 0x70);
	reg771 = dprx_read(dp, 0x71);
	reg717 = dprx_read(dp, 0x17);

	l0_caof_a = reg734 & 0x1f;
	l0_caof_b = reg735 & 0x1f;
	l0_caof_c = ((reg735 & 0xe0) >> 5) + ((reg736 & 0x03) << 3);
	l0_caof_d = ((reg736&0x7c)>>2);

	l1_caof_a = reg737 & 0x1f;
	l1_caof_b = reg738 & 0x1f;
	l1_caof_c = ((reg738 & 0xe0) >> 5) + ((reg739 & 0x03) << 3);
	l1_caof_d = ((reg739&0x7c)>>2);

	l2_caof_a = reg73a & 0x1f;
	l2_caof_b = reg73b & 0x1f;
	l2_caof_c = ((reg73b & 0xe0) >> 5)+((reg73c & 0x03) << 3);
	l2_caof_d = ((reg73c & 0x7c) >> 2);

	l3_caof_a = reg73d & 0x1f;
	l3_caof_b = reg73e & 0x1f;
	l3_caof_c = ((reg73e & 0xe0) >>5) + ((reg73f & 0x03) << 3);
	l3_caof_d = ((reg73f & 0x7c) >> 2);

	//DPRX_DEBUG_CAOF(("CAOF FLAG   = %02X \n", reg11));
	dprx_msg_dbg("CAOF STATUS = %02X   %X\n", reg771, reg770);
	dprx_msg_dbg("\n");
	dprx_msg_dbg("ENCAOF/ENSOF =%02X \n", reg717);
	dprx_msg_dbg("ENCAOF/ENSOF =%02X \n", reg717);
	dprx_msg_dbg("ENCAOF/ENSOF =%02X \n", reg717);
	dprx_msg_dbg("\n");
	dprx_msg_dbg("L0_CAOF_A   = %02X\n", l0_caof_a);
	dprx_msg_dbg("L0_CAOF_B   = %02X\n", l0_caof_b);
	dprx_msg_dbg("L0_CAOF_C   = %02X\n", l0_caof_c);
	dprx_msg_dbg("L0_CAOF_D   = %02X\n", l0_caof_d);
	dprx_msg_dbg("L1_CAOF_A   = %02X\n", l1_caof_a);
	dprx_msg_dbg("L1_CAOF_B   = %02X\n", l1_caof_b);
	dprx_msg_dbg("L1_CAOF_C   = %02X\n", l1_caof_c);
	dprx_msg_dbg("L1_CAOF_D   = %02X\n", l1_caof_d);
	dprx_msg_dbg("L2_CAOF_A   = %02X\n", l2_caof_a);
	dprx_msg_dbg("L2_CAOF_B   = %02X\n", l2_caof_b);
	dprx_msg_dbg("L2_CAOF_C   = %02X\n", l2_caof_c);
	dprx_msg_dbg("L2_CAOF_D   = %02X\n", l2_caof_d);
	dprx_msg_dbg("L3_CAOF_A   = %02X\n", l3_caof_a);
	dprx_msg_dbg("L3_CAOF_B   = %02X\n", l3_caof_b);
	dprx_msg_dbg("L3_CAOF_C   = %02X\n", l3_caof_c);
	dprx_msg_dbg("L3_CAOF_D   = %02X\n", l3_caof_d);

}

static int dprx_caof(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;

	u8 reg11;
	u8 reg770,reg771;
	u8 caof_done;

	u16 wait_cnt;
	u16 retry_cnt;

	dprxchgbank(dp, 7);
	dprxset(dp, 0x31, 0x40, 0x40);
	dprxchgbank(dp, 1);

	dprx_write(dp, 0xC1 ,0x0F);
	dprx_write(dp, 0xC0 ,0x04);
	dprx_write(dp, 0x87 ,0x00);

	dprxchgbank(dp, 0);
	dprxset(dp, 0x11, 0xc0, 0xc0);
	reg11 = dprx_read(dp, 0x11);
	
	dprx_msg_info("CAOF FLAG INIT  = %02X\n", reg11);

	dprxchgbank(dp, 7);

	dprxset(dp, 0x17, 0x03, 0x00);
	dprxset(dp, 0x34, 0x20, 0x00);
	dprxset(dp, 0x33, 0xF0, 0x70);
	dprxset(dp, 0x31, 0x28, 0x20);
	dprxset(dp, 0x31, 0x20, 0x00);
	reg770 = dprx_read(dp, 0x70);
	reg771 = dprx_read(dp, 0x71);

	dprx_msg_dbg("CAOF STATUS = %02X %02X\n", reg771, reg770);

	delay1ms(1);
	dprxset(dp, 0x31, 0x08, 0x08);
	dprxset(dp, 0x60, 0x80, 0x00);
	dprxset(dp, 0x12, 0x80, 0x80);
	dprxchgbank(dp, 0);
	reg11 = (dprx_read(dp, 0x11)&0x40);

	dprx_msg_dbg("reg11 = %x \n", reg11);

	wait_cnt = 0;
	retry_cnt = 0;
	caof_done = false;

	do {
		dprx_msg_info("Wait for CAOF done\n");
		dprx_msg_dbg("wait_cnt=%d, retry_cnt=%d \n",
				wait_cnt, retry_cnt);

		delay1ms(1);
		reg11 = dprx_read(dp, 0x11)&0x40;
		wait_cnt++;
		
		if (wait_cnt == 3 ) {
			dprxset(dp, 0x11, 0xc0, 0xc0);
			dprxchgbank(dp, 7);
			dprxset(dp, 0x31, 0x28, 0x00);
			dprxset(dp, 0x31, 0x28, 0x20);
			delay1ms(1);
			dprxset(dp, 0x31, 0x28, 0x00);
			dprxset(dp, 0x31, 0x28, 0x08);
			dprxchgbank(dp, 0);
			wait_cnt = 0;
			retry_cnt++;
		}

		if (reg11 == 0x40){
			caof_done = true;
			dprx_msg_info("CAOF Finish !! \n");
		}

	}while ((reg11 != 0x40) && (retry_cnt <= 3));

	reg11 = dprx_read(dp, 0x11);

	dprxchgbank(dp, 7);

	if (caof_done == false) {
		dprx_msg_err("CAOF FAIL !!\n");
		dprxset(dp, 0x31, 0x28, 0x00);
		dprxset(dp, 0x31, 0x28, 0x20);
		delay1ms(1);
		dprxset(dp, 0x31, 0x28, 0x00);
	}
	it6510_debug_caof(it6510);

	dprxset(dp, 0x31, 0x28, 0x00);
	dprxset(dp, 0x12, 0x80, 0x00);
	
	dprxchgbank(dp, 1);
	dprx_write(dp, 0xC1,0x00);
	dprx_write(dp, 0xC0,0x00);
	dprx_write(dp, 0x94, 0x04);
	dprxset(dp, 0x8F, 0x40, 0x00);

	dprxchgbank(dp, 0);

	return caof_done;
}


static void dprx_update_rs(struct it6510 *it6510, u8 level) 
{
	struct regmap *dp = it6510->dp_regmap;
	u8 t_rs[3] = {0x1F, 0x3F, 0x7F};

	
	it6510->dprx.rs_level = level;
	
	dprxchgbank(dp, 0);
	//HBR2RS_L
	dprx_write(dp, 0x71, 0x7F);
	dprx_write(dp, 0x72, 0x7F);
	dprx_write(dp, 0x73, 0x7F);
	dprx_write(dp, 0x74, 0x7F);
	//HBR2RS_H
	dprx_write(dp, 0x75, t_rs[level]);
	dprx_write(dp, 0x76, t_rs[level]);
	dprx_write(dp, 0x77, t_rs[level]);
	dprx_write(dp, 0x78, t_rs[level]);
	//HBR2RS_P
	dprx_write(dp, 0x79, t_rs[level]);
	dprx_write(dp, 0x7A, t_rs[level]);
	dprx_write(dp, 0x7B, t_rs[level]);
	dprx_write(dp, 0x7C, t_rs[level]);

}
static void dprx_reset_reg(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;

	dprxchgbank(dp, 0);

	dprx_write(dp, 0x12, 0x10);
	dprx_write(dp, 0x12, 0x00);
	dprx_write(dp, 0x12, 0xEF);
	dprx_write(dp, 0x13, 0x04);
	dprx_write(dp, 0x12, 0x00);
	dprx_write(dp, 0x13, 0x00);

	dprx_write(dp, 0x15, 0x78);
	dprx_write(dp, 0x17, 0x9F);
	dprx_write(dp, 0x18, 0x40);
	dprx_write(dp, 0x1B, 0xFF);

	dprx_write(dp, 0x41, 0x81);

	dprx_write(dp, 0x4A, 0xF0);
	dprx_write(dp, 0x51, 0x40);
	dprx_write(dp, 0x52, 0x01);
	dprx_write(dp, 0x57, 0x76);

	dprx_write(dp, 0x5A, 0x3D);//RS opt
	
	//RBR_HBR_RS_H
	dprx_write(dp, 0x5F, 0x3F);
	dprx_write(dp, 0x60, 0x3F);
	dprx_write(dp, 0x61, 0x3F);
	dprx_write(dp, 0x62, 0x3F);
	//RBR_HBR RS_P
	dprx_write(dp, 0x63, 0x07);
	dprx_write(dp, 0x64, 0x07);
	dprx_write(dp, 0x65, 0x07);
	dprx_write(dp, 0x66, 0x07);

	dprx_write(dp, 0x69, 0xA6);
	dprx_write(dp, 0x6A, 0xBD);
	dprx_write(dp, 0x6B, 0x71);
	dprx_write(dp, 0x6D, 0xC1);
	dprx_write(dp, 0x6E, 0xC8);
	dprx_write(dp, 0x6F, 0x2E);
	dprx_write(dp, 0x70, 0xFF);

	//HBR2RS_L
	dprx_write(dp, 0x71, 0x7F);
	dprx_write(dp, 0x72, 0x7F);
	dprx_write(dp, 0x73, 0x7F);
	dprx_write(dp, 0x74, 0x7F);
	//HBR2RS_H
	dprx_write(dp, 0x75, 0x1F);
	dprx_write(dp, 0x76, 0x1F);
	dprx_write(dp, 0x77, 0x1F);
	dprx_write(dp, 0x78, 0x1F);
	//HBR2RS_P
	dprx_write(dp, 0x79, 0x1F);
	dprx_write(dp, 0x7A, 0x1F);
	dprx_write(dp, 0x7B, 0x1F);
	dprx_write(dp, 0x7C, 0x1F);

	//CS
	dprx_write(dp, 0x7F, 0x12);
	dprx_write(dp, 0x80, 0x12);
	
	dprx_write(dp, 0xD0, 0x01);
	// HPD
	dprx_write(dp, 0xF0, 0x1F);
	dprx_write(dp, 0xF2, (0x7A >> 1));
	dprx_write(dp, 0xF3, 0xF6);
	
	//SHA sub device for HDCP
	//dprx_write(dp, 0xFA, 0x97);

	dprxchgbank(dp, 1);

	dprx_write(dp, 0x76, 0xF0);
	dprx_write(dp, 0x7B, 0xF0);

	dprx_write(dp, 0x8A, 0x04);
	dprx_write(dp, 0x8B, 0xB0);
	dprx_write(dp, 0x8C, 0x00);
	dprx_write(dp, 0x8E, 0x0F);
	dprx_write(dp, 0x8F, 0x00);


	dprx_write(dp, 0x95, 0x42);

	//dprx_write(dp, 0x96, AUX_CFG);
	//dprx_write(dp, 0xA0, 0xFF);
	//dprx_write(dp, 0xA1, 0x67);
	dprx_write(dp, 0xA3, 0x86);
	dprx_write(dp, 0xA4, 0x06);//HBR2 KI/KP
	//dprxset(0xA4, 0x0F, (6));


	dprx_write(dp, 0xC0, 0xFF);
	dprx_write(dp, 0xC1, 0x0F);
	//delay1ms(1);
	dprx_write(dp, 0xC0, 0x00);
	dprx_write(dp, 0xC1, 0x00);
	
	// PHYDIGTOP
	dprx_write(dp, 0xC6, 0x40);
	dprx_write(dp, 0xF8, 0x0D);
	dprx_write(dp, 0xF9, 0xA0);

	dprxchgbank(dp, 2);

	//Aux Control
	dprx_write(dp, 0x1C, 0x9B);

	dprx_write(dp, 0x22, (0x3D >> 1));
	dprx_write(dp, 0x23, 0xFC);
	dprx_write(dp, 0x24, 0x28);
	dprx_write(dp, 0x25, RAM_I2C_ADR |0x01);
	// PHYTOP
	dprxchgbank(dp, 3);

	dprx_write(dp, 0x10, 0x00);
	dprx_write(dp, 0x44, 0x01);
	dprx_write(dp, 0x9A, MIPI_I2C_ADR | 0x01);
	dprx_write(dp, 0xD0, 0x8A);
	dprx_write(dp, 0xD1, 0xE7);
	
	dprxchgbank(dp, 5);
	//HDCP reset off
	dprx_write(dp, 0x10, 0x00);
	dprx_write(dp, 0x50, 0x11);
	dprx_write(dp, 0x52, 0x20);
	dprx_write(dp, 0x64, 0x70);
	//todo: add to enable hdcp api
	dprx_write(dp, 0x68, 0x00);// RxHP2Ver 
	
	// LinkTop
	dprxchgbank(dp, 6);
	//dprx_write(dp, 0x17, 0x70);// RegVPLLGain Gain
	//audio
	//dprx_write(dp, 0x2B, 0x44);
	dprx_write(dp, 0x2C, 0x01);
	dprx_write(dp, 0x30, 0x30);
	dprx_write(dp, 0x31, 0x20);
	dprx_write(dp, 0x33, 0x00);
	dprx_write(dp, 0x34, 0xC5);//Enable NLPCM I2S
	dprx_write(dp, 0x35, 0xC0);//RegVidEnLCT
	dprx_write(dp, 0xE0, 0x47);  //auto LS clock reset
	dprxchgbank(dp, 7);
	dprx_write(dp, 0x81, 0x7C);//Auto DS;
	dprxchgbank(dp, 0);
	
    //Add 20211118
    dprxchgbank(dp, 1);
    dprxset(dp, 0x9A, 0x01, 0x01);
    dprxchgbank(dp, 0);
}


static void dprx_power_down_lc(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;


	dprxchgbank(dp, 1);

	#ifdef USE_ANALOG_CDR
	dprxset(dp, 0x87, 0x0F, 0x0F);
	dprxset(dp, 0xC3, 0xF1, 0x01); //[0]: PWDIPLL
	dprxchgbank(dp, 7);
	dprxset(dp, 0x29, 0xF0, 0xF0);
	dprxchgbank(dp, 1);
	#else
	dprxset(dp, 0x87, 0x0F, 0x00);
	dprxset(dp, 0xC3, 0xF1, 0xF0); //[7:4]: PWDCPLL
	dprxchgbank(dp, 7);
	dprxset(dp, 0x29, 0xF0, 0x00);
	dprxchgbank(dp, 1);
	#endif

	#ifdef USE_XTL_CK_SRC
	dprx_write(dp, 0x92, 0x2F);
	dprxset(dp, 0x92, 0x07, 0x00);//power down LC
	dprxset(dp, 0x93, 0x10, 0x10);//power down regulator
	#else
	dprx_write(dp, 0x92,0x3F);
	#endif

	dprxchgbank(dp, 0);
}
	


static void dprx_reset_dpcd(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;
	struct dpcd_config *dpcd = &it6510->dpcd;
	u8 en_excaps, en_dpcdv14, dpcd2210;

	if (dpcd->version == 0x14) {
		en_excaps = 0x01;
		en_dpcdv14 = 0x01;
		dpcd2210 = 0x70;
	} 
	else {
		en_excaps = 0x00;
		en_dpcdv14 = 0x00;
		dpcd2210 = 0x00;
	}		
		

	dprxchgbank(dp, 2);
	//DPCD
	dprx_write(dp, 0x10, dpcd->version);
	dprx_write(dp, 0x11, dpcd->bitrate);
	dprx_write(dp, 0x12, 0x60 | dpcd->lane);
	dprx_write(dp, 0x13, 0x21);
	dprx_write(dp, 0x14, 0x10);

	dprx_write(dp, 0x29, 0x21 | (en_excaps << 7));
	dprx_write(dp, 0x2D, 0x01);

	// dprx_write(dp, 0x4B, 0x00);
	// dprx_write(dp, 0x4C, 0x00);
	// dprx_write(dp, 0x4D, 0x00);
	// dprx_write(dp, 0x4E, 0x00);

	//DPCD Extend Caps
	dprx_write(dp, 0x67, 0x14);

	dprx_write(dp, 0x68, 0x00);
	dprx_write(dp, 0x6A, (en_excaps << 7) | dpcd2210);

	dprx_write(dp, 0x6C, 0xC0);

	dprx_write(dp, 0x71, 0x14);
	dprx_write(dp, 0x72, 0x78);

	dprxchgbank(dp, 0);
}



static  void dprx_config_hdcp(struct it6510 *it6510)
{
	struct dprx_data *dprx = &it6510->dprx;
	struct regmap *dp = it6510->dp_regmap;

	u8 hdcp_rx_cap[3] = {0x00,0x00,0x00};

	dprx_msg_info("dprx_config_hdcp(%X,%X)\n",dprx->support_hdcp,dprx->support_hdcp2);
	
	dprxchgbank(dp,5);

	if ((dprx->support_hdcp == true) || (dprx->support_hdcp2 == true)) {
		
		if (dprx->support_hdcp == true) {
			dprxset(dp, 0x10, 0x11, 0x01);
			hdcp_rx_cap[0] = 0x00;
			hdcp_rx_cap[1] = 0x00;
			hdcp_rx_cap[2] = 0x00;
		}
		if (dprx->support_hdcp2 == true) {
			dprxset(dp, 0x50, 0x49, 0x41);
			hdcp_rx_cap[0] = 0x02;
			hdcp_rx_cap[1] = 0x00;
			hdcp_rx_cap[2] = 0x02;
		}
		dprx_write(dp, 0x61, 0x00);
		dprx_write(dp, 0x62, 0x00);

		dprxset(dp, 0x10, 0x08, 0x08);

		dprx_msg_dbg("FWRxCap=%X%X%X\n",hdcp_rx_cap[0],hdcp_rx_cap[1],hdcp_rx_cap[2]);

		dprx_write(dp, 0x57, 0x04);
		dprx_write(dp, 0x58, hdcp_rx_cap[0]);
		dprx_write(dp, 0x59, hdcp_rx_cap[1]);
		dprx_write(dp, 0x5A, hdcp_rx_cap[2]);
		dprx_write(dp, 0x5B, 0x00);

		//FW force enter HDCP2
		if (dprx->support_hdcp2 == true) {
			dprxset(dp, 0x50, 0x80, 0x80);
			dprxset(dp, 0x50, 0x80, 0x00);
		}
		else {
			dprxset(dp, 0x50, 0x41, 0x00);
		}
	}
	else {
		dprxset(dp, 0x10, 0x09, 0x00);
		dprxset(dp, 0x50, 0x41, 0x00);
		dprx_write(dp, 0x57, 0x04);
		dprx_write(dp, 0x58, hdcp_rx_cap[0]);
		dprx_write(dp, 0x59, hdcp_rx_cap[1]);
		dprx_write(dp, 0x5A, hdcp_rx_cap[2]);
		dprx_write(dp, 0x5B, 0x00);
	}

	dprxchgbank(dp, 0);

	//dprx.en_hdcp14_hw_irq = true;
	dprxset(dp, 0xF3, 0x80, 0x80);
}

static u8 dprx_get_hdcp2_stream_type(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;
	u8 hdcp_stream_type = 0;

	dprxchgbank(dp, 5);
	hdcp_stream_type = dprx_read(dp, 0x6B);
	dprxchgbank(dp, 0);
	
	return hdcp_stream_type;
}

static int dprx_get_hdcp_status(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;
	int hdcp_detect = 0;
	u8 hdpc_status= false, hdcp2_status = false;

	dprxchgbank(dp, 6);
	hdpc_status = dprx_read(dp, 0x70) & 0x20 ? true : false;
	dprxchgbank(dp, 5);
	hdcp2_status = (dprx_read(dp, 0x67) & 0x0B) == 0x0B ? true : false;
	dprxchgbank(dp, 0);
		
	dprx_msg_dbg("dprx_get_hdcp_status %X, %X\n", hdpc_status, hdcp2_status);
	
	if (hdcp2_status == true)
		hdcp_detect = 2;
	else if (hdpc_status == true)
		hdcp_detect = 1;

	return hdcp_detect;
	
}


static void dprx_aud_fiforst(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;
	
	dprxset(dp, 0x12, 0x04, 0x04);
	dprxchgbank(dp, 1);
	dprxset(dp, 0xC0, 0x02, 0x02);
	dprxset(dp, 0xC0, 0x02, 0x00);
	dprxchgbank(dp, 0);
	dprxset(dp, 0x12, 0x04, 0x00);
	
	delay1ms(2);
	dprx_write(dp, 0x07, 0x0C);
}



static void dprx_reset_audio(struct it6510 *it6510)
{
	struct dprx_data *dprx = &it6510->dprx;

	dprx->audio_stable = false;
	dprx_aud_fiforst(it6510);
}

static void dprx_set_ext_audio(struct it6510 *it6510,
					u8 i2s_enable)
{
	struct regmap *dp = it6510->dp_regmap;
	struct dprx_data *dprx = &it6510->dprx;
	
	if (dprx->audio_i2s != AUDIO_OFF && i2s_enable == true) {
		if (dprx->audio_i2s == AUDIO_SPDIF)
			dprxset(dp, 0x28, 0x10, 0x00);
		else
			dprxset(dp, 0x28, 0x0F, 0x00);
	}
	else {
		dprxset(dp, 0x28, 0x1F, 0x1F);  
	}
}

static void dprx_aud_mute_clr(struct it6510 *it6510,
					unsigned char oe)
{
	struct regmap *dp = it6510->dp_regmap;
	
	dprxchgbank(dp, 6);
	dprxset(dp, 0x2C, 0x02, 0x02);
	dprxset(dp, 0x2C, 0x02, 0x00);
	dprxchgbank(dp, 0);

	dprx_write(dp, 0x08, 0x08);

	dprx_set_ext_audio(it6510, oe);
}




unsigned char dprx_guess_fs(unsigned int fs)
{

    if (fs < 400 ) {return AUD32K; }
    if (fs < 450 ) {return AUD44K; }
    if (fs < 600 ) {return AUD48K; }
    if (fs < 750 ) {return AUD64K; }
    if (fs < 900 ) {return AUD88K; }
    if (fs < 110 ) {return AUD96K; }
    if (fs < 150 ) {return AUD128K;}
    if (fs < 185 ) {return AUD176K;}
    if (fs < 220 ) {return AUD192K;}
    if (fs < 300 ) {return AUD256K;}
    if (fs < 375 ) {return AUD352K;}
    if (fs < 480 ) {return AUD384K;}
    if (fs < 650 ) {return AUD512K;}
    if (fs < 730 ) {return AUD705K;}

    return AUD768K;
}

unsigned long dprx_get_ls_clock(unsigned char bit_rate)
{

	//link rate / 512
    if (bit_rate == 0x06 ) return 3125;
    if (bit_rate == 0x0A ) return 5273;
    if (bit_rate == 0x14 ) return 10546;
	
	return 0;
}


static unsigned char dprx_is_audio_fs_error(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;
	struct dprx_data *dprx = &it6510->dprx;

    unsigned long M_aud,N_aud,LS_rck;
    unsigned char uc, fs;

    dprxchgbank(dp, 6);
	uc = dprx_read(dp, 0x70) & 0x10;
    if (uc) {
		dprxchgbank(dp, 0);
        return false;
	}

    dprxchgbank(dp, 1);
	uc = dprx_read(dp, 0x10);
	dprxchgbank(dp, 0);
	
	LS_rck = dprx_get_ls_clock(uc);

	if (LS_rck != 0 ) {

		dprxchgbank(dp, 6);

		//dprx_msg_dbg("audio M=%X%X%X,  \n", dprx_read(dp,0x59), dprx_read(dp,0x58), dprx_read(dp,0x57) );
		//dprx_msg_dbg("audio N=%X%X%X,  \n", dprx_read(dp,0x5C), dprx_read(dp,0x5B), dprx_read(dp,0x5A) );

		M_aud = (unsigned long) dprx_read(dp, 0x59);
		M_aud <<= 8;
		M_aud |= (unsigned long) dprx_read(dp, 0x58);
		M_aud <<= 8;
		M_aud |= (unsigned long) dprx_read(dp, 0x57);

		N_aud = (unsigned long) dprx_read(dp, 0x5C);
		N_aud <<= 8;
		N_aud |= (unsigned long) dprx_read(dp, 0x5B);
		N_aud <<= 8;
		N_aud |= (unsigned long) dprx_read(dp, 0x5A);

		fs = (dprx_read(dp, 0x71)&0x3F);  //[5:0] Fs, [6] "1:LPCM,0:NLPCM"

		dprxchgbank(dp, 0);

		dprx_msg_dbg("audio M=%lu, N=%lu, fs=%X \n", M_aud,  N_aud, fs);

		LS_rck *= M_aud;
		LS_rck /= N_aud;

		dprx->guess_fs = dprx_guess_fs((unsigned int)LS_rck);

		if (dprx->guess_fs != uc) {
			dprx_msg_dbg("audio M=%lu, N=%lu, LS_rck=%lu \n", M_aud,  N_aud, LS_rck);
			dprx_msg_dbg("fs guess = %X (%X) \n",dprx->guess_fs, fs);
			return true;
		}
	}

    return false;
}


static void dprx_audio_force_fs(struct it6510 *it6510, unsigned char ena)
{
	struct regmap *dp = it6510->dp_regmap;
	struct dprx_data *dprx = &it6510->dprx;

    dprxchgbank(dp, 6);

    //dprx_msg_info("dprx_audio_force_fs(%X,%X)\n", ena, dprx->guess_fs);

	dprxset(dp, 0x35, 0x3F, dprx->guess_fs);

    if (ena == true) {
        dprxset(dp, 0x2B, 0x10, 0x10);
    } else {
        dprxset(dp, 0x2B, 0x10, 0x00);
    }
	dprxchgbank(dp, 0);

	dprx->audio_fs_force = ena;
    dprx_aud_fiforst(it6510);
}


static bool dprx_check_hbr_audio(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;
	//struct dprx_data *dprx = &it6510->dprx;
	u8 coding_type;

	dprxchgbank(dp, 4);
	coding_type = dprx_read(dp, 0x71) >> 3;
	dprxchgbank(dp, 0);

	dprx_msg_info("Audio codinfg type %x\n", coding_type);
	return coding_type == AUD_HBR ? true : false;

}


static void dprx_audio_check_fs(struct it6510 *it6510)
{
	//struct regmap *dp = it6510->dp_regmap;
	struct dprx_data *dprx = &it6510->dprx;

	dprx_check_hbr_audio(it6510);
	
	if (dprx_is_audio_fs_error(it6510)) {
		dprx_audio_force_fs(it6510, true);
		
		if (dprx->audio_stable == true)
			dprx_reset_audio(it6510);
	
	} else {

		if (dprx->audio_fs_force == true)
			dprx_audio_force_fs(it6510, false);
	}

}

static void dprx_audio_check(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;
	struct dprx_data *dprx = &it6510->dprx;

	u8 audstable;
	u8 ls_audio_mute = true;

	dprxchgbank(dp, 6);
	ls_audio_mute = dprx_read(dp, 0x70) & 0x10 ? true : false;
	dprxchgbank(dp, 0);

	audstable = dprx_read(dp, 0x22) & 0x02 ? true : false;

	dprx_msg_dbg("LSAudioMute=%X, audstable=%X,\n",
			ls_audio_mute, audstable);
	
	if (ls_audio_mute == true || audstable == false) {
		dprx_reset_audio(it6510);

		if (ls_audio_mute == true)
			dprx_audio_force_fs(it6510, false);
		else
			dprx_audio_check_fs(it6510);


	}
	else {
		
		if (dprx->audio_stable == false) {
			dprx->audio_stable = true;
			dprx_aud_mute_clr(it6510, true);
		}
	}
}

static void dprx_delayed_audio_work(struct work_struct *work)
{
    struct it6510 *it6510 = container_of(work, struct it6510,
                        delayed_audio.work);

	if (it6510->power_up != true)
		return;
		
    mutex_lock(&it6510->lock);
    it6510->audio_wait = false;
    dprx_msg_info("dprx_delayed_audio_work...\n");
    dprx_audio_check(it6510);
    mutex_unlock(&it6510->lock);
}

static void dprx_irq_audio_change(struct it6510 *it6510)
{
	//struct regmap *dp = it6510->dp_regmap;
	struct dprx_data *dprx = &it6510->dprx;

	//u8 audstable;
	//u8 ls_audio_mute = true;

    if (it6510->audio_wait == true) {
	dprx_msg_dbg("it6510->audio_wait=%X...\n", it6510->audio_wait);
	return;
    }

    it6510->audio_wait = true;
    if (dprx->audio_stable == true)
        dprx_reset_audio(it6510);

    queue_delayed_work(system_wq, &it6510->delayed_audio,
                                msecs_to_jiffies(180));

                            
}




static void dprx_enable_audio(struct it6510 *it6510, u8 enable)
{
	struct regmap *dp = it6510->dp_regmap;
	//struct dprx_data *dprx = &it6510->dprx;
	
	dprx_msg_dbg("dprx_enable_audio(%X)\n", enable);

	if (enable == true) {
		dprx_write(dp, 0x07, 0x6C);
		dprx_write(dp, 0x08, 0xC0);
		dprx_write(dp, 0x2C, 0xBC);
		dprxset(dp, 0xD1, 0x6C, 0x6C);
		dprxset(dp, 0xD2, 0xC8, 0xC8);
		dprxset(dp, 0xDD, 0xBC, 0xBC);

	}
	else {
		dprxset(dp, 0xD1, 0x6C, 0x00);
		dprxset(dp, 0xD2, 0xC8, 0x00);
		dprxset(dp, 0xDD, 0xBC, 0x00);
		dprx_write(dp, 0x07, 0x6C);
		dprx_write(dp, 0x08, 0xC0);
		dprx_write(dp, 0x2C, 0xBC);
	}

	dprx_reset_audio(it6510);
	dprx_audio_force_fs(it6510, false);

    it6510->audio_wait = false;
    cancel_delayed_work(&it6510->delayed_audio);
}


static u8 dprx_get_audio_status(struct it6510 *it6510)
{
	struct dprx_data *dprx = &it6510->dprx;

	dprx_audio_check(it6510);


	return dprx->audio_stable;

}

int dprx_audio_code_to_sample_rate(struct it6510 *it6510)
{
	u8 fs;
	int sampl_rate = 0;
	struct regmap *dp = it6510->dp_regmap;

	dprxchgbank(dp, 6);
	fs = dprx_read(dp, 0x71)&0x3F;
	dprxchgbank(dp, 0);

	switch (fs) {
	case AUD32K :
		sampl_rate = 32000;
		break;
	case AUD64K :
		sampl_rate = 64000;
		break;
	case AUD128K :
		sampl_rate = 128000;
		break;
	case AUD256K :
		sampl_rate = 256000;
		break;
	case AUD512K :
		sampl_rate = 512000;
		break;
	case AUD44K :
		sampl_rate = 44100;
		break;
	case AUD88K :
		sampl_rate = 88200;
		break;
	case AUD176K :
		sampl_rate = 176400;
		break;
	case AUD352K :
		sampl_rate = 352800;
		break;
	case AUD705K :
		sampl_rate = 705600;
		break;
	case AUD48K :
		sampl_rate = 48000;
		break;
	case AUD96K :
		sampl_rate = 96000;
		break;
	case AUD192K :
		sampl_rate = 192000;
		break;
	case AUD384K :
		sampl_rate = 384000;
		break;
	case AUD768K :
		sampl_rate = 768000;
		break;
	}


	return sampl_rate;
}

int dprx_get_audio_sampling_rate(struct it6510 *it6510)
{

	int sampl_rate = 0;
	
	sampl_rate = dprx_audio_code_to_sample_rate(it6510);

	return sampl_rate;
}

//Get IEC60958 audio enconde Type
u8 dprx_get_audio_enconde_type(struct it6510 *it6510)
{
	u8 code_typte;
	struct regmap *dp = it6510->dp_regmap;

	dprxchgbank(dp, 6);
	//[6] "1:LPCM,0:NLPCM"
	code_typte = (dprx_read(dp, 0x71) & 0x40) >> 6;
	dprxchgbank(dp, 0);

	return code_typte;
}

/**
 * [dprx_get_audio_coding_type description]
 * @param  it6510 
 * @return        [description]
  *               AUD_LPCM = 0x00,
 *                AUD_HBR  = 0x01,
 *                AUD_3DLPCM  = 0x02,
 *                AUD_OneBit  = 0x03,
 *                AUD_DST  = 0x04,
 */
u8 dprx_get_audio_coding_type(struct it6510 *it6510)
{
	u8 audio_pkt_type;
	struct regmap *dp = it6510->dp_regmap;
	
	dprxchgbank(dp, 4);
	audio_pkt_type = (dprx_read(dp, 0x71) & 0x78) >> 3;
	dprxchgbank(dp, 0);

	return audio_pkt_type;

}


static void dprx_vid_fiforst(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;

	dprxset(dp, 0x12, 0x02, 0x02);
	dprxchgbank(dp, 1);
	dprxset(dp, 0xC0, 0x01, 0x01);
	dprxset(dp, 0xC0, 0x01, 0x00);
	dprxchgbank(dp, 0);
	dprxset(dp, 0x12, 0x02, 0x00);

	dprx_write(dp, 0x07, 0x03);
}

static void dprx_reset_video(struct it6510 *it6510)
{
	//struct regmap *dp = it6510->dp_regmap;
	struct dprx_data *dprx = &it6510->dprx;
	
	dprx_msg_dbg("DPRX Video Reset !!\n");

	dprx_vid_fiforst(it6510);

	//disable overwrite video format parameters in MSA
	//dprxchgbank(dp, 6);
	//dprxset(dp, 0x17, 0x10, 0x00);
	//dprxset(dp, 0x39, 0x20, 0x00); //disable over write Mvid
	//dprxchgbank(dp, 0);

	dprx->video_stable = false;
}

static void dprx_video_check(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;
	struct dprx_data *dprx = &it6510->dprx;
	struct color_format video_in;

	u8 vidstable;
	u8 ls_no_vidstream = false;

	dprxchgbank(dp, 6);
	ls_no_vidstream = dprx_read(dp, 0x70) & 0x08 ? true : false;
	dprxchgbank(dp, 0);

	vidstable = dprx_read(dp, 0x22) & 0x01 ? true : false;

	dprx_msg_dbg("LSNoVidstream = %X, vidstable = %X\n",
				ls_no_vidstream, vidstable);

	if (vidstable != true) {
		if ((ls_no_vidstream == false) ||
			(dprx->video_stable == true)) {
			dprx_reset_video(it6510);
		}
	}
	else {

		if (dprx->video_stable != true) {
			dprx->video_stable = true;
			dprx_get_color_from_misc(it6510, &video_in);
			dprx_config_color_transfer(it6510, &video_in);
			dprx_get_bt_timing(it6510);
		}
	}
}


static void dprx_enable_video(struct it6510 *it6510, u8 enable)
{
	struct regmap *dp = it6510->dp_regmap;
	//struct dprx_data *dprx = &it6510->dprx;

	dprx_msg_dbg("dprx_enable_video(%X)\n", enable);
	
	if (enable == true) {
		dprx_write(dp, 0x07, 0x13);
		dprx_write(dp, 0x08, 0x30);
		dprx_write(dp, 0x09, 0x10);
		dprx_write(dp, 0x0C, 0x21);
		dprx_write(dp, 0x0E, 0x30);
		dprx_write(dp, 0x2C, 0x40);

		dprxset(dp, 0xD1, 0x13, 0x13);
		dprxset(dp, 0xD2, 0x30, 0x30);
		dprxset(dp, 0xD3, 0x10, 0x00);
		dprxset(dp, 0xD6, 0x21, 0x20);
		dprxset(dp, 0xD8, 0x30, 0x30);
		dprxset(dp, 0xDD, 0x40, 0x40);

		//dprx_set_new_pkt_irq(0x02,0x02);

	}
	else {
		dprxset(dp, 0xD1, 0x13, 0x00);
		dprxset(dp, 0xD2, 0x30, 0x00);
		dprxset(dp, 0xD3, 0x10, 0x00);
		dprxset(dp, 0xD6, 0x21, 0x00);
		dprxset(dp, 0xD8, 0x30, 0x30);
		dprxset(dp, 0xDD, 0x40, 0x00);

		dprx_write(dp, 0x07, 0x13);
		dprx_write(dp, 0x08, 0x30);
		dprx_write(dp, 0x09, 0x10);
		dprx_write(dp, 0x0C, 0x21);
		dprx_write(dp, 0x0E, 0x30);
		dprx_write(dp, 0x2C, 0x40);
		
		//dprx_set_new_pkt_irq(0x02,0x00);
	}

	dprx_reset_video(it6510);
	//Packet status reset
	//dprx_detect_hdr_pkt(enable);
	//dprx_detect_vsc_pkt(enable);
	//dprx_detect_vsif_pkt(enable);
}

static void dprx_get_color_from_misc(struct it6510 *it6510,
				struct color_format *video_para)
{
	struct regmap *dp = it6510->dp_regmap;

	u8 uc;
	u8 color, profile;

	dprxchgbank(dp, 6);
	uc = dprx_read(dp, 0x6E);
	dprxchgbank(dp, 0);

	//DPRX_DEBUG_VIDEO(("Reg_66E = %X",uc));
	
	color = uc >> 1 & 0x03;
	profile =  uc >> 3 & 0x03;

	video_para->color_depth = uc >> 5 & 0x07;
	video_para->color_mode = color;
	video_para->content_type = 0;

	//DPRX_DEBUG_VIDEO(("dprx_get_color_from_misc()\n"));
	//DPRX_DEBUG_VIDEO(("color = %X",color));
	//DPRX_DEBUG_VIDEO(("profile = %X",profile));
	

	switch (color) {
	case COLOR_RGB:  //RGB
		switch (profile) {
		case 0x01:
			video_para->color_cea_range = 1;
			video_para->color_colorietry = COLORIMETRY_sRGB;
			break;
		// case 0x02:
		//     video_para->color_cea_range = 0;
		//     video_para->color_colorietry = COLORIMETRY_scRGB;
		//     break;
		// case 0x03:
		//     video_para->color_cea_range = 0;
		//     video_para->color_colorietry = COLORIMETRY_aRGB;
		//     break;
		// case 0x00:
		default:
			video_para->color_cea_range = 0;
			video_para->color_colorietry = COLORIMETRY_sRGB;
			break;
		}
		break;
	case COLOR_YUV422: //YUV422
	case COLOR_YUV444: //YUV444
		
		video_para->color_cea_range = true;

		if ((profile&0x02) == 0x02)
			video_para->color_colorietry = COLORIMETRY_BT709;
		else
			video_para->color_colorietry = COLORIMETRY_BT601;

		if ((profile&0x01) == 0x00)
			video_para->color_cea_range = true;

		break;
	case COLOR_YUV420:
		video_para->color_cea_range = true;
		if ((profile&0x2) == 0x02)
			video_para->color_colorietry = COLORIMETRY_BT709;
		else
			video_para->color_colorietry = COLORIMETRY_BT601;
		break;

	default:
		video_para->color_mode = 0x00;
		video_para->color_cea_range = 0;
		video_para->color_colorietry = COLORIMETRY_sRGB;
		break;
	}
}


static void dprx_config_color_transfer(struct it6510 *it6510,
					struct color_format* video_in)
{
	struct regmap *dp = it6510->dp_regmap;
	struct color_format *video_out = &it6510->color_fmt;

	video_out->color_cea_range = video_in->color_cea_range;
	video_out->color_colorietry = video_in->color_colorietry ;
	//video_out->color_depth = video_in->color_depth;
	
	dprxchgbank(dp, 3);

	if (video_in->color_mode == COLOR_RGB &&
		video_out->color_mode != COLOR_RGB) {
		
		dprxset(dp, 0x10, 0xC0, CSC_RGB2YUV << 6);
		regmap_bulk_write(dp, 0x12, (u8*)cscmtx_rgb2yuv_itu709_16_235,
					sizeof(cscmtx_rgb2yuv_itu709_16_235));
	}
	else if (video_in->color_mode != COLOR_RGB &&
			video_out->color_mode == COLOR_RGB) {
		
		dprxset(dp, 0x10, 0xC0, CSC_YUV2RGB << 6);
		regmap_bulk_write(dp, 0x12, (u8*)cscmtx_yuv2rgb_itu709_16_235,
					sizeof(cscmtx_yuv2rgb_itu709_16_235));
	}

	
	dprx_write(dp, 0x11,
		(video_out->color_mode << 6) |
		((video_out->color_depth - 1) << 4));

	dprx_write(dp, 0x27, 0x02);
	dprx_write(dp, 0x28, 0x00);
	dprxset(dp, 0x29, 0x40, 0x40);
	

	//if (debug == 3) {
		dprx_msg_dbg("%s\n",  __func__);
		dprx_msg_dbg(" in color mode = %X\n", video_in->color_mode);
		dprx_msg_dbg("out color mode = %X\n", video_out->color_mode);
		dprx_msg_dbg("reg_310 = %X\n", dprx_read(dp, 0x10));
		dprx_msg_dbg("reg_311 = %X\n", dprx_read(dp, 0x11));
		dprx_msg_dbg("reg_327 = %X\n", dprx_read(dp, 0x27));
		dprx_msg_dbg("reg_328 = %X\n", dprx_read(dp, 0x28));
	//}


	dprxchgbank(dp, 0);



	//dprx_vid_fiforst(it6510);
}

static void dprx_irq_data_status_change(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;
	struct dprx_data *dprx = &it6510->dprx;

	u8 reg07,reg08,reg09,reg0c;
	u8 reg2c, reg2d;
	//u8 audio_change = false;
	
	dprxchgbank(dp, 0);

	reg07 = dprx_read(dp, 0x07);
	reg08 = dprx_read(dp, 0x08);
	reg09 = dprx_read(dp, 0x09);
	reg0c = dprx_read(dp, 0x0C);
	reg2c = dprx_read(dp, 0x2C);
	reg2d = dprx_read(dp, 0x2D);
	
	if (reg07)
		dprx_write(dp, 0x07, reg07);
	if (reg08)
		dprx_write(dp, 0x08, reg08);
	if (reg09)
		dprx_write(dp, 0x09, reg09);
	if (reg0c)
		dprx_write(dp, 0x0C, reg0c);
	if (reg2c&0xFC)
		dprx_write(dp, 0x2C, reg2c&0xFC);
	if (reg2d)
		dprx_write(dp, 0x2D, reg2d);

	// DPRX_DEBUG_PRINT(("reg07=%X\n", reg07));
	// DPRX_DEBUG_PRINT(("reg08=%X\n", reg08));
	// DPRX_DEBUG_PRINT(("reg09=%X\n", reg09));
	// DPRX_DEBUG_PRINT(("reg0c=%X\n", reg0c));
	// DPRX_DEBUG_PRINT(("reg2c=%X\n", reg2c));

	//if (reg07&0xF0) {
	//     DPRX_DEBUG_PRINT(("MVid/MAud/NAud/VBID Majority Error Interrupt (%X)...\n",
	//     								(reg07&0xF0)>>4));
	//     show_link_attribute();
	//}
	
	if (reg08 & 0x07) {
		dprx_msg_err("AUX Receive Error Interrupt(%X) ...\n",
							reg08 & 0x07);
	}

	//
	///audio irq
	//
	if (reg08 & 0xC8 || reg2c & 0xBC || reg07 & 0x0C){


		if (reg07 & 0x0C) {
			dprx_msg_err("Audio FIFO Error Interrupt (%X)...\n",
							reg07 & 0x0C);
		}

		//DPRX_DEBUG_AUDIO(("DPRX Audio Irq:\n"));
		if (reg2c & 0xBC) {
			dprx_msg_dbg("Audio status change Interrupt(%X) ...\n",
								reg2c & 0xBC);

		}
		if (reg08 & 0xC0) {
			dprx_msg_dbg("Audio Mute Start/end (%X/%X)\n",
					(reg08 & 0x40) >> 6, reg08 & 0x80 >> 7);
		}

		//dprx_audio_check(it6510);
        dprx_irq_audio_change(it6510);

		if (reg08 & 0x08) {
			dprx_msg_dbg("Auto Audio Mute\n");
			if(dprx->audio_stable == true)
				dprx_aud_mute_clr(it6510, true);
		}
	}

	// if (reg0c&0x02) {
	// 	DPRX_DEBUG_AUDIO(("New Audio InfoFrame Interrupt ...\n"));
	// }
	// if (reg09&0x20) {
	// 	DPRX_DEBUG_PRINT(("No Audio InfoFrame Interrupt ...\n"));
	// }


	//
	///video irq
	//

	if (reg2c & 0x40 || reg08 & 0x30 || reg0c & 0x20 || reg07 & 0x03) {

	if (reg07 & 0x03) {
		dprx_msg_err("video FIFO Error Interrupt (%X)...\n",
						reg07 & 0x03);
	}	

		if (reg2c & 0x40) {
			dprx_msg_dbg("video status change Interrupt ...\n");
		}

		if (reg08 & 0x10) {
			dprx_msg_dbg("Video Mute Start Interrupt ...\n");
		}        
	
		if (reg08 & 0x20) {
			dprx_msg_dbg("Video Mute End Interrupt ...\n");
		}

		if (reg0c & 0x20) {
			dprx_msg_dbg("New Video Format Interrupt ...\n");

		}

		dprx_video_check(it6510);
	}

	if (reg0c & 0x10) {
		//DPRX_DEBUG_PRINT(("New GenPkt InfoFrame Interrupt ...\n"));
		//dprx_genrecord_pkt_update(true);
	}



	if (reg0c & 0x40) {
		dprx_msg_dbg("New VSC Interrupt ...\n");
		//dprx_vsc_pkt_update();

	}

	if (reg0c & 0x80) {
		dprx_msg_dbg("Gen2Pkt Interrupt ...\n");
		//dprx_gen2record_pkt_update(true);
		//dprx_hdr_pkt_update(true);
	}


	if (reg2d & 0x20) {
		dprx_msg_dbg("NO GEN record !\n");
		//dprx_irq_no_genpkt_record();

	}

	if (reg2d & 0x40) {
		dprx_msg_dbg("NO GEN2 record !\n");
		//dprx_irq_no_gen2pkt_record();
	}
}


static u8 dprx_link_symbole_lock_status(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;
	//struct dprx_data *dprx = &it6510->dprx;	
	
	u8 link_status, dpcd101, mask;
	
	link_status = dprx_read(dp, 0x90);
	dprxchgbank(dp, 1);
	dpcd101 = dprx_read(dp, 0x11);
	dprx_msg_info("dprx_link_symbole_lock_status.\n");
	dprx_msg_dbg("link_status=%X\n", link_status);
	dprx_msg_dbg("dpcd101=%x\n",dpcd101);
	dprx_msg_dbg("dpcd202=%x\n",dprx_read(dp, 0x1A));
	dprx_msg_dbg("dpcd203=%x\n",dprx_read(dp, 0x1B));
	dprx_msg_dbg("dpcd204=%x\n",dprx_read(dp, 0x1C));
	dprxchgbank(dp, 0);
	
	if ((link_status & 0x84) == 0x04) {
		switch (dpcd101 & 0x1F) {
			case 0x01:
				mask = 0x08;
				break;
			case 0x02:
				mask = 0x18;
				break;
			case 0x04:
				mask = 0x78;
				break;
			default:
				mask = 0x00;
				break;
		}
		if ((link_status & mask) == 0)
			return true;
	}
	
	return false;
}

static void dprx_reset_link_error_counter(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;
	dprxchgbank(dp, 0);
	dprxset(dp, 0x41, 0x10, 0x10);
	dprxset(dp, 0x41, 0x10, 0x00);
}

static void dprx_trigger_sareq(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;
	u8 dpcd100, dpcd101;
	
	dprx_msg_info("dprx_trigger_sareq...\n");
	
	dprxchgbank(dp, 1);
	dpcd100 = dprx_read(dp, 0x10);
	dpcd101 = dprx_read(dp, 0x11);
	dprxchgbank(dp, 0);
	
	dprx_write(dp, 0x10, 0xF8);
	dprx_write(dp, 0x10, 0xF8);
	
	dprxset(dp, 0x56, 0x08, 0x08);
	
	dprx_write(dp, 0x5A, 0x3C);
	
	dprx_write(dp, 0x75, 0xFF);
	dprx_write(dp, 0x76, 0xFF);
	dprx_write(dp, 0x77, 0xFF);
	dprx_write(dp, 0x78, 0xFF);

	dprxchgbank(dp, 7);
	dprxset(dp, 0x31, 0x01, 0x00);
	
	dprxchgbank(dp, 1);
	dprx_write(dp, 0x28, dpcd100);
	dprx_write(dp, 0x29, dpcd101);
	
	dprx_write(dp, 0x8D, 0x9B);
	dprxset(dp, 0xA0, 0x04, 0x00);
	dprx_write(dp, 0xA2, 0x38);
	dprx_write(dp, 0xA3, 0x86);
	
	dprxchgbank(dp, 7);
	dprxset(dp, 0x18, 0x80, 0x00);
	dprxset(dp, 0xF0, 0x80, 0x80);
	dprxset(dp, 0x60, 0x60, 0x00);
	dprxset(dp, 0x61, 0x01, 0x00);
	dprx_write(dp, 0x62, 0xFC);
	dprx_write(dp, 0x63, 0x41);
	
	dprxset(dp, 0x2F, 0x04, 0x04);
	
	dprx_write(dp, 0x30, 0x20);
	dprx_write(dp, 0x40, 0xC2);
	dprx_write(dp, 0x41, 0xC6);
	dprx_write(dp, 0x42, 0xCA);
	dprx_write(dp, 0x43, 0xD2);
	dprx_write(dp, 0x48, 0x00);
	dprx_write(dp, 0x4F, 0x04);
	
	dprx_write(dp, 0x4B, 0xB0);
	//dprxset(dp, 0x4A, 0xC0, 0x00);
	//dprxset(dp, 0x4A, 0x30, 0x00);
	dprxset(dp, 0x4A, 0xF0, 0x00);
	dprxset(dp, 0x4D, 0x03, 0x03);
	dprx_write(dp, 0x4C, 0x2F);
	dprxset(dp, 0x31, 0x02, 0x00);
	dprx_write(dp, 0x32, 0x40);
	
	dprxset(dp, 0x31, 0x01, 0x01);

	dprxchgbank(dp, 1);
	dprx_write(dp, 0x75, 0xF0);
	dprxchgbank(dp, 0);

}


#define DP_LINK_ERROR_BOUND 0x0006
static u8 dprx_link_eq_process(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;
	//struct dprx_data *dprx = &it6510->dprx;
	u8 lane_set, i, link_error_status[4], link_error;
	u16 link_error_count[4];

	dprxchgbank(dp, 1);
	lane_set = dprx_read(dp, 0x11) & 0x1F;
	
	if ((lane_set > 4 ) || (lane_set == 3))
		lane_set = 4;
	
	
	link_error = false;
	
	for (i = 0; i < 4; i++) {
		link_error_status[i] = false;
		link_error_count[i] = 0;
	}


	for (i = 0; i < lane_set; i++) {
		link_error_status[i] = false;
		link_error_count[i] = dprx_read(dp, 0x21 + (i*2));
		link_error_count[i] <<= 8;
		link_error_count[i] |= dprx_read(dp, 0x20 + (i*2));
		
		if (link_error_count[i] & 0x8000) {
			if ((link_error_count[i] & 0x7FFF) > DP_LINK_ERROR_BOUND) {
				link_error_status[i] = true;
				link_error = true;
			}
			
		}
		
		dprx_msg_dbg("L[%X ]link_error_status = %x link_error_count= %x\n",
					i,link_error_status[i], link_error_count[i]);
	}
	
	dprxchgbank(dp, 0);
	
	if (link_error == true) {
		if (it6510->dprx.rs_level < MAX_RS_LEVEL) {
			dprx_update_rs(it6510, it6510->dprx.rs_level + 1 );
			return false;
		}
		else {
			dprx_trigger_sareq(it6510);
		}
	}
	
	return true;
}


static void dprx_delayed_link_work(struct work_struct *work)
{
    struct it6510 *it6510 = container_of(work, struct it6510,
                        delayed_link_check.work);

	struct dprx_data *dprx = &it6510->dprx;
	u8 ret;

	if (it6510->power_up != true)
		return;
		
    mutex_lock(&it6510->lock);
    dprx_msg_info("dprx_delayed_link_work...\n");
 
start_link_error_check:
    if (dprx->link_train_done == true) {
		if (dprx_link_symbole_lock_status(it6510) == true) {
			
			
			dprx_reset_link_error_counter(it6510);
			
			mutex_unlock(&it6510->lock);
			wait_event_interruptible_timeout(it6510->wq, 
								(dprx->link_train_done != true),
								msecs_to_jiffies(100));
								
			if (dprx->link_train_done != true)
				return;
				
			mutex_lock(&it6510->lock);
				 
			ret = dprx_link_eq_process(it6510);
			
			if (ret == false) {
				dprx_msg_err("check link again\n");
				goto start_link_error_check;
			}
		}
	}
        
    mutex_unlock(&it6510->lock);
}



static void dprx_issue_hpd_irq(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;
	struct dprx_data *dprx = &it6510->dprx;
	
	dprx->issue_hpd_irq = false;

	dprx_write(dp, 0x1F, 0x40);
	dprx_msg_dbg("DPRX issue HPD_IRQ!\n");

}
static void dprx_irq_check_symble_unlcok(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;
	struct dprx_data *dprx = &it6510->dprx;
	
	u8 reg0a;
	u8 symunlkno;
	u8 link_sts, unalign;
	u8 lnkstupd;


	dprxchgbank(dp, 0);

	reg0a = dprx_read(dp, 0x0A);
	dprx_write(dp, 0x0A, reg0a & 0xF4);

	dprxchgbank(dp, 1);
	lnkstupd = dprx_read(dp, 0x1C) & 0x80 ? true : false;
	dprxchgbank(dp, 0);

	link_sts = dprx_read(dp, 0x90); //& 0x80;

	symunlkno = (reg0a & 0xF0) >> 4;
	symunlkno &= (link_sts >> 3) & 0x0F;

	link_sts &= 0x80; //b[7] unlign status

	if (lnkstupd == true && dprx->link_train_done == true) {
		unalign = false;
	
		if ((reg0a & 0x04) && link_sts) {
			dprx_msg_dbg("Symbol UnAlign !!\n");
			unalign = true;
		}

		if (symunlkno || unalign == true) {

			dprx->issue_hpd_irq = true;
			dprx_link_status_change(it6510, false);
			
			dprx_msg_dbg("Symbol Unlock status : ");
			dprx_msg_dbg("[%c][%c][%c][%c]\n",
				(symunlkno&0x01)?'1':'0',
				(symunlkno&0x02)?'1':'0',
				(symunlkno&0x04)?'1':'0',
				(symunlkno&0x08)?'1':'0');
		}
	}
	
	if (dprx->issue_hpd_irq == true)
		dprx_issue_hpd_irq(it6510);

}


static void dprx_irq_training_status_change(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;
	struct dprx_data *dprx = &it6510->dprx;
	
	u8 reg0a, reg2c, reg90;

	dprxchgbank(dp, 0);

	reg0a = dprx_read(dp, 0x0A);
	dprx_write(dp, 0x0A, reg0a & 0x0B);
	
	reg2c = dprx_read(dp, 0x2C);
	dprx_write(dp, 0x2C, reg2c & 0x01);

	if (reg0a & 0x0B) {
			
		reg90 = dprx_read(dp, 0x90);
		dprx_msg_dbg("reg0A = %X, reg90 = %X\n",reg0a ,reg90);

		dprx->link_train_done = reg90 & 0x04 ? true : false;

		dprx_link_status_change(it6510,
				dprx->link_train_done);
	
		dprx_msg_dbg("dprx->link_train_done = %X\n",
					dprx->link_train_done);
	}

	if (reg0a & 0x01) {
		dprx_msg_dbg("Link Training Start Interrupt!!\n");
	}

	if (reg0a & 0x02) {
		dprx_msg_dbg("Link Training Done Interrupt\n");
	}

	if (reg2c & 0x01) {
		dprx_msg_err("HBR2 Training Fail Interrupt !\n");
	}
	if (reg0a & 0x08) {
		dprx_msg_err("Link Training Fail Interrupt\n");
	}

	dprx_irq_check_symble_unlcok(it6510);

}


static void dprx_irq(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;
	struct dprx_data *dprx = &it6510->dprx;
	
	u8 reg0b, reg0f;
	u8 reg2e, reg2f, reg_tmp;

	dprxchgbank(dp, 0);
	reg2e = dprx_read(dp, 0x2E);
	reg2f = dprx_read(dp, 0x2F);

	dprx->issue_hpd_irq = false;

	if ((reg2e == 0x00) && (reg2f == 0x00))
		return;

	dprx_msg_dbg("Reg2E=%X, Reg2F=%X\n", reg2e, reg2f);

	if (reg2e&0x10) {

		reg0b = dprx_read(dp, 0x0B);

		dprx_msg_dbg("Reg0B=%X\n", reg0b);

		dprx_write(dp, 0x0B, reg0b & 0xFC);

		if (reg0b & 0x03) {
			dprx_write(dp, 0x0B, reg0b & 0x03);
		}

		if (reg0b & 0xF1) {
			dprx_msg_dbg("dprx hdcp1.4 status irq = %x \n", reg0b&0xF1);
			dprx_write(dp, 0x0B, reg0b&0xF1);
			if (reg0b & 0x80) {
				dprx_msg_dbg("HDCP 1.4 Authentication Done Interrupt ...\n");
				dprx_get_hdcp_status(it6510);
			}
		}
	}

	if (reg2e & 0x08)
		dprx_irq_training_status_change(it6510);

	if (reg2e & 0x27 || reg2f & 0x18)
		dprx_irq_data_status_change(it6510);

	if (reg2f & 0x01) {

		reg0f = dprx_read(dp, 0x0F);
		dprx_write(dp, 0x0F, reg0f);
		//DPRX_DEBUG_INTERRUPT(("reg0f=%x\n", reg0f));
		if (reg0f & 0x10) {
		//DPRX_DEBUG_INTERRUPT(("I2C Bus Hang Interrupt ...\n"));
			dprxchgbank(dp, 2);
			dprxset(dp, 0x1C, 0x40, 0x40);  //master sel
			dprxset(dp, 0x1E, 0x02, 0x02);
			dprxset(dp, 0x1E, 0x02, 0x00);
			dprxset(dp, 0x1C, 0x40, 0x00);
			dprxchgbank(dp, 0);
			dprx_write(dp, 0x0F, 0x10);
		}
	
		if (reg0f & 0xC0) {
			dprx_msg_dbg("dprx hdcp2.3 status irq = %x \n", reg0f&0xC0);
			if (reg0f & 0x80) {
				dprx_msg_dbg("HDCP 2.3 Authentication Done Interrupt\n");
				dprx_get_hdcp_status(it6510);
				dprx_msg_dbg("HDCP2 stream Type = %X\n",
						dprx_get_hdcp2_stream_type(it6510));
			}
		}

	}

	if (reg2e & 0x40) {
		reg_tmp = dprx_read(dp, 0x0D);
		dprx_write(dp, 0x0D, reg_tmp);
	}

	if (reg2e & 0x80) {
		reg_tmp = dprx_read(dp, 0x0E);
		dprx_write(dp, 0x0E, reg_tmp);
	}


	if (reg2f & 0x02) {
		reg_tmp = dprx_read(dp, 0x10);
		dprx_write(dp, 0x10,reg_tmp);

		if (reg_tmp & 0x01)	
			dprx_msg_dbg("HDCP 2.3 encription status chage\n");

		if (reg_tmp & 0x02)	
			dprx_msg_dbg("HDCP 2.3 update status irq\n");
			
	}

	if (reg2f & 0x04) {
		reg_tmp = dprx_read(dp, 0x11);
		dprx_write(dp, 0x11,reg_tmp);
	}


	if ( dprx->issue_hpd_irq == true)
		dprx_issue_hpd_irq(it6510);
}

static void dprx_enable_link_irq(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;

	dprxset(dp, DP_REG_INT_MASK_07, 0xFF, 0x00);
	dprxset(dp, DP_REG_INT_MASK_08, 0xFF, 0x00);
	dprxset(dp, DP_REG_INT_MASK_09, 0xFF, 0x00);
	dprxset(dp, DP_REG_INT_MASK_0A, 0xFF, 0xFF);
	dprxset(dp, DP_REG_INT_MASK_0B, 0xFF, 0xFF);
	dprxset(dp, DP_REG_INT_MASK_0C, 0xFF, 0x00);
	dprxset(dp, DP_REG_INT_MASK_0D, 0xFF, 0x00);
	dprxset(dp, DP_REG_INT_MASK_0E, 0xFF, 0x00);
	dprxset(dp, DP_REG_INT_MASK_0F, 0xFF, 0xCD);
	dprxset(dp, DP_REG_INT_MASK_10, 0xFF, 0x03);
	dprxset(dp, DP_REG_INT_MASK_11, 0xFF, 0x00);
	dprxset(dp, DP_REG_INT_MASK_2D, 0xFF, 0x01);
	dprxset(dp, DP_REG_INT_MASK_2C, 0xFF, 0x00);
	dprxset(dp, DP_REG_INT_MASK_2B, 0xFF, 0x00);

}



static void dprx_disable_link_irq(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;

	dprx_write(dp, DP_REG_INT_MASK_07, 0x00);
	dprx_write(dp, DP_REG_INT_MASK_08, 0x00);
	dprx_write(dp, DP_REG_INT_MASK_09, 0x00);
	dprx_write(dp, DP_REG_INT_MASK_0A, 0x00);
	dprx_write(dp, DP_REG_INT_MASK_0B, 0x00); 
	dprx_write(dp, DP_REG_INT_MASK_0C, 0x00);
	dprx_write(dp, DP_REG_INT_MASK_0D, 0x00);
	dprx_write(dp, DP_REG_INT_MASK_0E, 0x00);
	dprx_write(dp, DP_REG_INT_MASK_0F, 0x00);
	dprx_write(dp, DP_REG_INT_MASK_10, 0x00);
	dprx_write(dp, DP_REG_INT_MASK_11, 0x00);
	dprx_write(dp, DP_REG_INT_MASK_2D, 0x00);
	dprx_write(dp, DP_REG_INT_MASK_2C, 0x00);
	dprx_write(dp, DP_REG_INT_MASK_2B, 0x00);

}
static void dprx_irq_clear_all(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;
	//clear all
	dprx_write(dp, DP_REG_INT_STS_07, 0xFF);
	dprx_write(dp, DP_REG_INT_STS_08, 0xFF);
	dprx_write(dp, DP_REG_INT_STS_09, 0xFF);
	dprx_write(dp, DP_REG_INT_STS_0A, 0xFF);
	dprx_write(dp, DP_REG_INT_STS_0B, 0xFF);
	dprx_write(dp, DP_REG_INT_STS_0C, 0xFF);
	dprx_write(dp, DP_REG_INT_STS_0D, 0xFF);
	dprx_write(dp, DP_REG_INT_STS_0E, 0xFF);
	dprx_write(dp, DP_REG_INT_STS_0F, 0xFF);
	dprx_write(dp, DP_REG_INT_STS_2B, 0xFF);
	dprx_write(dp, DP_REG_INT_STS_2C, 0xFF);
	dprx_write(dp, DP_REG_INT_STS_2D, 0xFF);

}



static void dprx_show_link_info(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;
	u8 dpcd100, dpcd101, dpcd102, dpcd103, dpcd104; 
	u8 dpcd105, dpcd106, dpcd107, dpcd108;
	u8 dpcd200, dpcd202, dpcd203, dpcd204;
	u8 dpcd205, dpcd206, dpcd207;

	dprxchgbank(dp, 1);
	dpcd100 = dprx_read(dp, 0x10);
	dpcd101 = dprx_read(dp, 0x11);
	dpcd102 = dprx_read(dp, 0x12);
	dpcd103 = dprx_read(dp, 0x13);
	dpcd104 = dprx_read(dp, 0x14);
	dpcd105 = dprx_read(dp, 0x15);
	dpcd106 = dprx_read(dp, 0x16);
	dpcd107 = dprx_read(dp, 0x17);
	dpcd108 = dprx_read(dp, 0x18);
	dpcd200 = dprx_read(dp, 0x19);
	dpcd202 = dprx_read(dp, 0x1A);
	dpcd203 = dprx_read(dp, 0x1B);
	dpcd204 = dprx_read(dp, 0x1C);
	dpcd205 = dprx_read(dp, 0x1D);
	dpcd206 = dprx_read(dp, 0x1E);
	dpcd207 = dprx_read(dp, 0x1F);
	dprxchgbank(dp, 0);
	dprx_msg_dbg("Link Configuration: \n");
	dprx_msg_dbg("LINK_BW_SET: %u.%u Gbps\n",(((u16)(dpcd100)*27)/100),
						((((u16)(dpcd100)*27)%100)/10));
	dprx_msg_dbg("LANE_COUNT_SET: %d Lanes\n",dpcd101&0x1F);
	dprx_msg_dbg("Enhanced Framing Mode = %X\n", ((dpcd101&0x80)>>7));
	dprx_msg_dbg("Scrambling = %X\n", ((dpcd102&0x20)>>5) );
	dprx_msg_dbg("DPCD status\n");
	dprx_msg_dbg("Lan:        [3][2][1][0]");
	dprx_msg_dbg("Voltage Swing\n");
	dprx_msg_dbg("Req:        [%X][%X][%X][%X]\n", (dpcd207&0x30)>>4,
						(dpcd207&0x03),
						(dpcd206&0x30)>>4,
						(dpcd206&0x03));
	dprx_msg_dbg("Set:        [%X][%X][%X][%X]\n", (dpcd106&0x03),
						(dpcd105&0x03),
						(dpcd104&0x03),
						(dpcd103&0x03));
	dprx_msg_dbg("MaxReached: [%X][%X][%X][%X]\n", (dpcd106&0x04)>>2,
						(dpcd105&0x04)>>2,
						(dpcd104&0x04)>>2,
						(dpcd103&0x04)>>2);
	dprx_msg_dbg("CR Done:    [%X][%X][%X][%X]\n", (dpcd203&0x10)>>4,
						(dpcd203&0x01),
						(dpcd202&0x10)>>4,
						(dpcd202&0x01));
	dprx_msg_dbg("==\n");
	dprx_msg_dbg("Pre-Emphasis");
	dprx_msg_dbg("Req:        [%X][%X][%X][%X]\n", (dpcd207&0xC0)>>6,
						(dpcd207&0x0C)>>2,
						(dpcd206&0xC0)>>6,
						(dpcd206&0x0C)>>2);
	dprx_msg_dbg("Set:        [%X][%X][%X][%X]\n", (dpcd106&0x18)>>3,
						(dpcd105&0x18)>>3,
						(dpcd104&0x18)>>3,
						(dpcd103&0x18)>>3);
	dprx_msg_dbg("MaxReached: [%X][%X][%X][%X]\n", (dpcd106&0x20)>>5,
						(dpcd105&0x20)>>5,
						(dpcd104&0x20)>>5,
						(dpcd103&0x20)>>5);
	dprx_msg_dbg("EQ Done:    [%X][%X][%X][%X]\n", (dpcd203&0x20)>>5,
						(dpcd203&0x02)>>1,
						(dpcd202&0x20)>>5,
						(dpcd202&0x02)>>1);
	dprx_msg_dbg("==\n");
	dprx_msg_dbg("Symbol Lock [%X][%X][%X][%X]\n", (dpcd203&0x40)>>6,
						(dpcd203&0x04)>>2,
						(dpcd202&0x40)>>6,
						(dpcd202&0x04)>>2);
	dprx_msg_dbg("Inter-Lane Align = %X\n",dpcd204&0x01);
	dprx_msg_dbg("Spread spectrum modulation freq = %X\n",dpcd107&0x01);
	dprx_msg_dbg("Spreding amplitude = %X\n",(dpcd107&0x10)>>4 );
	dprx_msg_dbg("ANSI 8B10B: %X\n", dpcd108&0x01);
	dprx_msg_dbg("CP_READY %X\n",(dpcd200&0x40)>>6);
	dprx_msg_dbg("Link Status Update = %X\n", (dpcd204&0x80)>>7);
	dprx_msg_dbg("\n");
}


static void dprx_link_status_change(struct it6510 *it6510, u8 status)
{
	struct regmap *dp = it6510->dp_regmap;
	struct dprx_data *dprx = &it6510->dprx;
	
	dprx->link_train_done = status;

	if (dprx->link_train_done == true) {
		dprxset(dp, 0x12, 0x80, 0x80);
		dprxset(dp, 0x12, 0x80, 0x00);
		dprx_show_link_info(it6510);
		
		queue_delayed_work(system_wq, &it6510->delayed_link_check,
                                msecs_to_jiffies(500));
	}
	else {
		cancel_delayed_work(&it6510->delayed_link_check);
	}

	dprx_enable_video(it6510, status);
	dprx_enable_audio(it6510, status);


}


static void dprx_set_hpd(struct it6510 *it6510, u8 hpd)
{
	struct regmap *dp = it6510->dp_regmap;
	//struct dprx_data *dprx = &it6510->dprx;

	dprx_msg_info("dprx_set_hpd(%X)...\n",hpd);

	//reset to default RS
	dprx_update_rs(it6510, DEFAULT_RS_LEVEL); 
	
	if (hpd == true) {
		dprxset(dp, 0xF3, 0x40, 0x00);
		dprxset(dp, 0xF0, 0xA0, 0x20);
	}
	else {
		dprxset(dp, 0xF3, 0x40, 0x40);
		dprxset(dp, 0xF0, 0xA0, 0x00);		
	}
}

static void dprx_trun_on_hpd(struct it6510 *it6510)
{
	dprx_msg_info("%s\n",  __func__);

	dprx_irq_clear_all(it6510);
	dprx_enable_link_irq(it6510);
	
	dprx_link_status_change(it6510, false);


	dprx_set_hpd(it6510, true);

}

static void dprx_trun_off_hpd(struct it6510 *it6510)
{
	dprx_msg_info("%s\n",  __func__);
	
	dprx_set_hpd(it6510, false);

	dprx_link_status_change(it6510, false);
	
	dprx_disable_link_irq(it6510);
	dprx_irq_clear_all(it6510);
}


static u16 dprx_get_input_status(struct it6510 *it6510)
{
	struct dprx_data *dprx = &it6510->dprx;

	dprx_msg_info("%s\n",  __func__);
	return dprx->video_stable;

}

static u16 dprx_get_pclk(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;

	u16 tmp;
	u16 pclk;

	//dprx_msg_info("%s\n",  __func__);
	dprxchgbank(dp, 0);

	dprxset(dp, 0x24, 0x08, 0x00);

	tmp = (u16)dprx_read(dp, 0x21);
	tmp <<= 8;
	tmp += (u16)dprx_read(dp, 0x20);

	tmp *= 2; //13.5M rclk
	pclk = 0x000D8000/tmp;

	dprxset(dp, 0x24, 0x08, 0x08);

	dprx_msg_dbg("%s %d to  %d\n", __func__, tmp, pclk);

	return pclk;
}


static void it6510_initial(struct it6510 *it6510)
{

	dprx_reset_reg(it6510);
	dprx_caof(it6510);
	dprx_power_down_lc(it6510);
	dprx_reset_dpcd(it6510);
	mipi_csi_initial(it6510);
		
}


static void it6510_enable_dprx(struct it6510 *it6510, unsigned char enable)
{
	mutex_lock(&it6510->lock);
	
	if (enable) {

		dprx_config_hdcp(it6510);
		dprx_init_edid_ram(it6510);
		dprx_trun_on_hpd(it6510);
	}
	else {
		dprx_trun_off_hpd(it6510);
	}
		
	mutex_unlock(&it6510->lock);
}

static void it6510_update_edid_data(struct it6510 *it6510)
{
	mutex_lock(&it6510->lock);
	dprx_init_edid_ram(it6510);
	mutex_unlock(&it6510->lock);	
}

int it6510_get_hdcp_status(struct it6510 *it6510)
{
	int hdcp_status;

	mutex_lock(&it6510->lock);

	hdcp_status = dprx_get_hdcp_status(it6510);

	mutex_unlock(&it6510->lock);
	
	return hdcp_status;
}



static irqreturn_t it6510_intp_threaded_handler(int unused, void *data)
{
	struct it6510 *it6510 = data;

	if (it6510->power_up != true)
		return IRQ_HANDLED;
	
	mutex_lock(&it6510->lock);

	//dprx_msg_info("%s  00\n",  __func__);
	dprx_irq(it6510);
	mipitx_irq(it6510);
	//dprx_msg_info("%s  11\n",  __func__);

	mutex_unlock(&it6510->lock);

	return IRQ_HANDLED;
}


static void it6510_enable_csi(struct it6510 *it6510, u8 enable)
{
	if(enable)
		mipitx_setup_csi(it6510);
	else
		mipitx_disable_csi(it6510);

}


static void it6510_enable_stream(struct v4l2_subdev *sd, bool enable)
{	
	struct it6510 *it6510 = sd_to_it6510(sd);

	dprx_msg_info("%s(%d)\n",  __func__, enable);
	mutex_lock(&it6510->lock);

	it6510_enable_csi(it6510, enable);

	it6510->streaming = !!enable;

	mutex_unlock(&it6510->lock);
}

static void dprx_get_bt_timing(struct it6510 *it6510)
{
	struct regmap *dp = it6510->dp_regmap;
	//struct dprx_data *dprx = &it6510->dprx;

	struct v4l2_bt_timings *bt = &it6510->timings.bt;

	u16 htotal, hdes, hdew, hfph, hsyncw;
	u16 vtotal, vdes, vdew, vfph, vsyncw;
	u16 vsyncpol, hsyncpol, interlace;
	
	dprxchgbank(dp, 6);

	htotal = dprx_read(dp, 0x5F) << 8;
	htotal += dprx_read(dp, 0x5E);
	
	hdes = dprx_read(dp, 0x61) << 8;
	hdes += dprx_read(dp, 0x60);

	hdew = dprx_read(dp, 0x63) << 8;
	hdew += dprx_read(dp, 0x62);
	
	hsyncw = dprx_read(dp, 0x6B) & 0x7F << 8;
	hsyncw += dprx_read(dp, 0x6A);

	hfph = htotal - hdew - hdes;

	hsyncpol = dprx_read(dp, 0x6B) & 0x80 ? 1 : 0;

	vtotal = dprx_read(dp, 0x65) << 8;
	vtotal += dprx_read(dp, 0x64);
	
	vdes = dprx_read(dp, 0x67) << 8;
	vdes += dprx_read(dp, 0x66);
	
	vdew = dprx_read(dp, 0x69) << 8;
	vdew += dprx_read(dp, 0x68);
	
	vsyncw = dprx_read(dp, 0x6D) & 0x7F << 8;
	vsyncw += dprx_read(dp, 0x6C);
	
	vfph = vtotal - vdew - vdes;

	vsyncpol = dprx_read(dp, 0x6D) & 0x80 ? 1 : 0;

	interlace = dprx_read(dp, 0x70) & 0x04 ? 1 : 0;

	dprxchgbank(dp, 0);


	dprx_msg_dbg("htotal = %d\n", htotal);
	dprx_msg_dbg("hdew = %d\n", hdew);
	dprx_msg_dbg("hdes = %d\n", hdes);
	dprx_msg_dbg("hfph = %d\n", hfph);
	dprx_msg_dbg("hsyncw = %d\n", hsyncw);
	dprx_msg_dbg("hsyncpol = %d\n",hsyncpol);
	dprx_msg_dbg("vtotal = %d\n", vtotal);
	dprx_msg_dbg("vdes = %d\n", vdes);
	dprx_msg_dbg("vdew = %d\n", vdew);
	dprx_msg_dbg("vfph = %d\n", vfph);
	dprx_msg_dbg("vsyncw = %d\n", vsyncw);
	dprx_msg_dbg("vsyncpol = %d\n", vsyncpol);
	dprx_msg_dbg("interlaced mode = %d\n", interlace);


	it6510->timings.type = V4L2_DV_BT_656_1120;

	bt->pixelclock = dprx_get_pclk(it6510) * 1000000; //Mhz to hz
	bt->interlaced = interlace ? V4L2_DV_INTERLACED : V4L2_DV_PROGRESSIVE;
	
	bt->width = hdew;
	bt->hsync = hsyncw;
	bt->hfrontporch = hfph;
	bt->hbackporch = hdes - hsyncw;

	bt->height = vdew;
	bt->vsync = vsyncw;
	bt->vfrontporch = vfph;
	bt->vbackporch = vdes - vsyncw;

	bt->polarities = hsyncpol ? V4L2_DV_HSYNC_POS_POL : 0 | 
			 vsyncpol ? V4L2_DV_VSYNC_POS_POL : 0 ;

	if (bt->interlaced == V4L2_DV_INTERLACED) {
		bt->il_vfrontporch = bt->vfrontporch;
		bt->il_vsync = bt->vsync + 1;
		bt->il_vbackporch = bt->vbackporch;
		bt->height *= 2;
	}


}

__maybe_unused static u8 it6510_get_audio_status(struct it6510 *it6510)
{
	u8 audio_status;
	mutex_lock(&it6510->lock);
	audio_status = dprx_get_audio_status(it6510);
	mutex_unlock(&it6510->lock);	

	return audio_status;
}

__maybe_unused static  int it6510_get_audio_sample_rate(struct it6510 *it6510)
{
	int sampl_rate = 0;

	mutex_lock(&it6510->lock);

	if (dprx_get_audio_status(it6510) == false)
		return sampl_rate;
	
	sampl_rate = dprx_get_audio_sampling_rate(it6510);
	mutex_unlock(&it6510->lock);

	return sampl_rate; 

}

static void it6510_get_bt_timing(struct it6510 *it6510,
					struct v4l2_dv_timings *timings)
{
	mutex_lock(&it6510->lock);
	dprx_get_bt_timing(it6510);
	*timings = it6510->timings;
	mutex_unlock(&it6510->lock);

}


static int it6510_dp_get_dv_timings(struct v4l2_subdev *sd,
					struct v4l2_dv_timings *timings)
{
	struct it6510 *it6510 = sd_to_it6510(sd);
	//struct dprx_data *dprx = &it6510->dprx;
	
	//u64 pixelclock;

	if (!timings)
		return -EINVAL;

	memset(timings, 0, sizeof(struct v4l2_dv_timings));

	if (!dprx_get_input_status(it6510))
		return -ENOLINK;

	it6510_get_bt_timing(it6510, timings);

	dprx_msg_dbg("pixelclock = %lld \n", timings->bt.pixelclock);

	if (timings->bt.pixelclock < 0){
		return -ENODATA;
	}

	return 0;
}

static void it6510_set_mipi_color_fmt(struct it6510 *it6510, u32 fmt_code)
{

        it6510->csi.mbus_fmt_code = fmt_code;

        //output color 
        switch (fmt_code) {
        default:
        case MEDIA_BUS_FMT_RGB888_1X24:
                it6510->csi.data_type = CSI_RGB888;
                it6510->color_fmt.color_mode = COLOR_RGB;
                it6510->color_fmt.color_depth = DP_COLOR_8BIT;
                break;
        case MEDIA_BUS_FMT_UYVY8_2X8:
                it6510->csi.data_type = CSI_YCbCr4228b;
                it6510->color_fmt.color_mode = COLOR_YUV422;
                it6510->color_fmt.color_depth = DP_COLOR_8BIT;

                break;
        }
        
        return;
}

static void it6510_update_colorspace(struct it6510 *it6510)
{
	//struct mipi_bus *csi = &it6510->csi;
	struct color_format video_in;

	dprx_msg_info("%s\n",  __func__);

	if (!dprx_get_input_status(it6510))
		return ;

	mutex_lock(&it6510->lock);

	dprx_get_color_from_misc(it6510, &video_in);

	dprx_config_color_transfer(it6510, &video_in);

//	mipitx_setup_csi(it6510);

	mutex_unlock(&it6510->lock);
}

static void it6510_set_power(struct it6510 *it6510, int power_up)
{
	mutex_lock(&it6510->lock);
	
	dprx_msg_info("%s %d\n",  __func__, power_up);
	
	if (power_up == true) {
		it6510_initial(it6510);
	}
	
	it6510->power_up =  power_up;
	
	mutex_unlock(&it6510->lock);
}

/* --------------- VIDEO OPS --------------- */

static int it6510_query_dv_timings(struct v4l2_subdev *sd,
		struct v4l2_dv_timings *timings)
{
	int ret;

	ret = it6510_dp_get_dv_timings(sd, timings);
	if (ret)
		return ret;

	if(debug) 
		v4l2_print_dv_timings(sd->name, "it6510_query_dv_timings: ",
					timings, true);

	if (!v4l2_valid_dv_timings(timings,
				&it6510_timings_cap, NULL, NULL)) {
		v4l2_dbg(1, debug, sd, "%s: timings out of range\n", __func__);
		return -ERANGE;
	}

	return 0;
}

static int it6510_g_input_status(struct v4l2_subdev *sd, u32 *status)
{
	struct it6510 *it6510 = sd_to_it6510(sd);
	*status = dprx_get_input_status(it6510) == true ? 0 : V4L2_IN_ST_NO_SIGNAL;
	dprx_msg_info("%s\n",  __func__);
	v4l2_dbg(1, debug, sd, "%s: status = 0x%x\n", __func__, *status);

	return 0;
}

static int it6510_g_dv_timings(struct v4l2_subdev *sd,
				 struct v4l2_dv_timings *timings)
{
	struct it6510 *it6510 = sd_to_it6510(sd);

	*timings = it6510->timings;

	return 0;
}

static int it6510_s_dv_timings(struct v4l2_subdev *sd,
				 struct v4l2_dv_timings *timings)
{
	struct it6510 *it6510 = sd_to_it6510(sd);

	if (!timings)
		return -EINVAL;

	if (debug)
		v4l2_print_dv_timings(sd->name, "it6510_s_dv_timings: ",
				timings, false);

	if (v4l2_match_dv_timings(&it6510->timings, timings, 0, false)) {
		v4l2_dbg(1, debug, sd, "%s: no change\n", __func__);
		return 0;
	}

	if (!v4l2_valid_dv_timings(timings, &it6510_timings_cap,
							NULL, NULL)) {
		
		v4l2_dbg(1, debug, sd, "%s: timings out of range\n", __func__);
		
		return -ERANGE;
	}

	//it6510 may not support set device timing
	//it6510->timings = *timings;

	return 0;
}


static int it6510_s_stream(struct v4l2_subdev *sd, int enable)
{
	v4l2_dbg(1, debug, sd, "%s: enable = 0x%x\n", __func__, enable);

	it6510_enable_stream(sd, enable);
	return 0;
}


static const struct v4l2_subdev_video_ops it6510_video_ops = {
	.g_input_status = it6510_g_input_status,
	.query_dv_timings = it6510_query_dv_timings,
	.s_dv_timings = it6510_s_dv_timings,
	.g_dv_timings = it6510_g_dv_timings,
	.s_stream = it6510_s_stream,
};


/* --------------- PAD OPS --------------- */



static int it6510_enum_mbus_code(struct v4l2_subdev *sd,
		struct v4l2_subdev_state *sd_state,
		struct v4l2_subdev_mbus_code_enum *code)
{
	switch (code->index) {
	case 0:
		code->code = MEDIA_BUS_FMT_RGB888_1X24;
		break;
	case 1:
		code->code = MEDIA_BUS_FMT_UYVY8_2X8;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}


static u32 it6510_g_colorspace(struct it6510 *it6510)
{
	switch (it6510->csi.mbus_fmt_code) {
	case MEDIA_BUS_FMT_RGB888_1X24:
		return V4L2_COLORSPACE_SRGB;
	case MEDIA_BUS_FMT_UYVY8_2X8:
		return V4L2_COLORSPACE_REC709;
	default:
		return 0;
	}
}

static int it6510_get_fmt(struct v4l2_subdev *sd,
		struct v4l2_subdev_state *sd_state,
		struct v4l2_subdev_format *format)
{
	struct it6510 *it6510 = sd_to_it6510(sd);

	if (format->pad != 0)
		return -EINVAL;

	//it6510_get_bt_timing(it6510);

	format->format.code = it6510->csi.mbus_fmt_code;
	format->format.width = it6510->timings.bt.width;
	format->format.height = it6510->timings.bt.height;
	format->format.field = V4L2_FIELD_NONE;

	format->format.colorspace = it6510_g_colorspace(it6510);

	return 0;
}

static int it6510_set_fmt(struct v4l2_subdev *sd,
		struct v4l2_subdev_state *sd_state,
		struct v4l2_subdev_format *format)
{
	struct it6510 *it6510 = sd_to_it6510(sd);

	u32 code = format->format.code; /* is overwritten by get_fmt */
	int ret;

	if (it6510->streaming)
		return -EBUSY;

	ret = it6510_get_fmt(sd, sd_state, format);
	format->format.code = code;

	if (ret)
		return ret;

	switch (code) {
	case MEDIA_BUS_FMT_RGB888_1X24:
	case MEDIA_BUS_FMT_UYVY8_2X8:
		break;
	default:
		return -EINVAL;
	}

	if (format->which == V4L2_SUBDEV_FORMAT_TRY)
		return 0;

	it6510_s_stream(sd, false);
	it6510_set_mipi_color_fmt(it6510, code);
        it6510_update_colorspace(it6510);

	return 0;
}


static int it6510_enum_dv_timings(struct v4l2_subdev *sd,
				    struct v4l2_enum_dv_timings *timings)
{
	if (timings->pad != 0)
		return -EINVAL;

	return v4l2_enum_dv_timings_cap(timings,
			&it6510_timings_cap, NULL, NULL);
}



static int it6510_dv_timings_cap(struct v4l2_subdev *sd,
		struct v4l2_dv_timings_cap *cap)
{
	if (cap->pad != 0)
		return -EINVAL;

	*cap = it6510_timings_cap;

	return 0;
}



static int it6510_get_edid(struct v4l2_subdev *sd,
		struct v4l2_subdev_edid *edid)
{
	struct it6510 *it6510 = sd_to_it6510(sd);
	int i;

	memset(edid->reserved, 0, sizeof(edid->reserved));

	if (edid->pad != 0)
		return -EINVAL;

	if (edid->start_block == 0 && edid->blocks == 0) {
		return 0;
	}

	//support only 2 block edid
	if (edid->start_block > 1  || edid->start_block + edid->blocks > 1)
		return -ENODATA;

	for (i = 0; i < edid->blocks; i++)
		memcpy(edid->edid, &it6510->edid_data[i], 128);

	return 0;
}




static int it6510_set_edid(struct v4l2_subdev *sd,
		struct v4l2_subdev_edid *edid)
{
	struct it6510 *it6510 = sd_to_it6510(sd);
	int i;
		
	memset(edid->reserved, 0, sizeof(edid->reserved));


	if (edid->pad != 0)
		return -EINVAL;

	if (edid->start_block != 0)
		return -EINVAL;

	if (edid->blocks > 2) {
		edid->blocks = 2;
		return -E2BIG;
	}

	if (edid->blocks == 0) {
		return 0;
	}

	for (i = 0; i < edid->blocks; i++)
		memcpy(&it6510->edid_data[i], edid, sizeof(it6510->edid_data));

	it6510_update_edid_data(it6510);

	return 0;
}


static int it6510_get_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
			     struct v4l2_mbus_config *cfg)
{
	struct it6510 *it6510 = sd_to_it6510(sd);

	cfg->type = V4L2_MBUS_CSI2_DPHY;


	/* Support for non-continuous CSI-2 clock is missing in the driver */
	cfg->flags = V4L2_MBUS_CSI2_CONTINUOUS_CLOCK;

	switch (it6510->csi.lane_cnt) {
	case 1:
		cfg->flags |= V4L2_MBUS_CSI2_1_LANE;
		break;
	case 2:
		cfg->flags |= V4L2_MBUS_CSI2_2_LANE;
		break;
	case 3:
		cfg->flags |= V4L2_MBUS_CSI2_3_LANE;
		break;
	case 4:
		cfg->flags |= V4L2_MBUS_CSI2_4_LANE;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}



static const struct v4l2_subdev_pad_ops it6510_pad_ops = {
	.enum_mbus_code = it6510_enum_mbus_code,
	.set_fmt = it6510_set_fmt,
	.get_fmt = it6510_get_fmt,
	.get_edid = it6510_get_edid,
	.set_edid = it6510_set_edid,
	.enum_dv_timings = it6510_enum_dv_timings,
	.dv_timings_cap = it6510_dv_timings_cap,
	.get_mbus_config = it6510_get_mbus_config,
};


#if 0
static int it6510_v4l2_set_power(struct v4l2_subdev *sd, int on)
{
	struct it6510 *it6510 = sd_to_it6510(sd);
	
	v4l2_dbg(1, debug, sd, "%s: enable = 0x%x\n", __func__, on);
	it6510_set_power(it6510, on);
	
	return 0;
}


static const struct v4l2_subdev_core_ops it6510_core_ops = {
	.s_power = &it6510_v4l2_set_power,

};
#endif

static const struct v4l2_subdev_ops it6510_ops = {
	//.core = &it6510_core_ops,
	.video = &it6510_video_ops,
	.pad = &it6510_pad_ops,
};




static const struct regmap_range it6510_dp_volatile_ranges[] = {
	{ .range_min = 0, .range_max = 0xff },
};

static const struct regmap_access_table it6510_dp_volatile_table = {
	.yes_ranges = it6510_dp_volatile_ranges,
	.n_yes_ranges = ARRAY_SIZE(it6510_dp_volatile_ranges),
};

static const struct regmap_config it6510_dp_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.volatile_table = &it6510_dp_volatile_table,
	.cache_type = REGCACHE_NONE,
};

static const struct regmap_range it6510_mipi_volatile_ranges[] = {
	{ .range_min = 0, .range_max = 0xff },
};

static const struct regmap_access_table it6510_mipi_volatile_table = {
	.yes_ranges = it6510_mipi_volatile_ranges,
	.n_yes_ranges = ARRAY_SIZE(it6510_mipi_volatile_ranges),
};

static const struct regmap_config it6510_mipi_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.volatile_table = &it6510_mipi_volatile_table,
	.cache_type = REGCACHE_NONE,
};


static const struct regmap_range it6510_edid_volatile_ranges[] = {
	{ .range_min = 0, .range_max = 0xff },
};

static const struct regmap_access_table it6510_edid_volatile_table = {
	.yes_ranges = it6510_edid_volatile_ranges,
	.n_yes_ranges = ARRAY_SIZE(it6510_edid_volatile_ranges),
};

static const struct regmap_config it6510_edid_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.volatile_table = &it6510_edid_volatile_table,
	.cache_type = REGCACHE_NONE,
};


static ssize_t attr_buffer_put(char *buf, char *reg_buf)
{
	int i = 0;
	char *str = buf, *end = buf + PAGE_SIZE;

	str += scnprintf(str, end - str,
	"     0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07");
	str += scnprintf(str, end - str,
	" 0x08 0x09 0x0A 0x0B 0x0C 0x0D 0x0E 0x0F\n");
	str += scnprintf(str, end - str,
	"=============================================");
	str += scnprintf(str, end - str,
	"=======================================");

	for (i = 0; i < 256; i++) {
		if ( i % 16 == 0)
			str += scnprintf(str, end - str,
					"\n[%02X] ", i & 0xF0);
		str += scnprintf(str, end - str, "0x%02X ",
					reg_buf[i]);
	}
	str += scnprintf(str, end - str, "\n");

	return end - str;
}
static ssize_t edid_ram_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct it6510 *it6510 = sd_to_it6510(sd);
	//struct regmap *edid = it6510->edid_regmap;

	u8 reg_buf[256];

	dev_info(dev, "%s(%x)\n", __func__, it6510->attr_dp_reg_bank);

	mutex_lock(&it6510->lock);
	dprx_get_edid_ram(it6510, reg_buf);
	mutex_unlock(&it6510->lock);


	return attr_buffer_put(buf, reg_buf);
}


static ssize_t dp_reg_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct it6510 *it6510 = sd_to_it6510(sd);
	int reg_bank;

	if (kstrtoint(buf, 10, &reg_bank) < 0)
		return -EINVAL;

	it6510->attr_dp_reg_bank = (u8) reg_bank;

	dev_info(dev, "dp_reg_store() %d, %x\n",
			reg_bank, it6510->attr_dp_reg_bank);

	return count;
}


static ssize_t dp_reg_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct it6510 *it6510 = sd_to_it6510(sd);
	struct regmap *dp = it6510->dp_regmap;
	int i;
	u8 reg_buf[256];

	dev_info(dev, "%s(%x)\n", __func__, it6510->attr_dp_reg_bank);

	mutex_lock(&it6510->lock);
	dprxchgbank(dp, it6510->attr_dp_reg_bank);
	for (i=0; i<256; i++)
		reg_buf[i] = dprx_read(dp, i);
	//regmap_bulk_read(dp, 0, reg_buf, 256);
	dprxchgbank(dp, 0);
	mutex_unlock(&it6510->lock);

	return  attr_buffer_put(buf, reg_buf);
}


static ssize_t mipi_reg_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct it6510 *it6510 = sd_to_it6510(sd);
	struct regmap *mipi = it6510->mipi_regmap;
	int i;
	u8 reg_buf[256];

	mutex_lock(&it6510->lock);
	for (i=0; i<256; i++)
		reg_buf[i] = mipi_read(mipi, i);
	//regmap_bulk_read(mipi, 0, reg_buf, 256);
	mutex_unlock(&it6510->lock);

	return  attr_buffer_put(buf, reg_buf);
}


static ssize_t mipi_reg_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t size)
{


	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct it6510 *it6510 = sd_to_it6510(sd);
	struct regmap *mipi = it6510->mipi_regmap;
	unsigned int addr,val;

    if( sscanf(buf,"%X %X ", &addr, &val) ){
    
        dev_info(dev, "addr= %2.2X \n", addr);
        dev_info(dev, "val = %2.2X \n", val);

        if((( addr<=0xFF)&&(0x00<=addr))&&(( val<=0xFF)&&(0x00<=val))){
            regmap_write(mipi, addr, val);   
        }
    }
    else{
            dev_err(dev, "it6682_fwrite_mhl_reg , error[%s]\n", buf);
    }
    
    return size;
}


static ssize_t enable_stream_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct it6510 *it6510 = sd_to_it6510(sd);

	return scnprintf(buf, PAGE_SIZE, "%d\n", it6510->attr_enable_stream);
}

static ssize_t enable_stream_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct it6510 *it6510 = sd_to_it6510(sd);
	int en_stream;

	if (kstrtoint(buf, 10, &en_stream) < 0)
		return -EINVAL;
	dev_info(dev, "enable_stream_store() %d\n", en_stream);

	it6510->attr_enable_stream = en_stream;

	if (en_stream)
		it6510_s_stream(sd, true);
	else
		it6510_s_stream(sd, false);

	return count;
}


static ssize_t mipi_link_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct it6510 *it6510 = sd_to_it6510(sd);

	return scnprintf(buf, PAGE_SIZE, "%d\n", it6510->csi.lane_cnt);
}

static ssize_t mipi_link_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct it6510 *it6510 = sd_to_it6510(sd);
	int en_link;

	if (kstrtoint(buf, 10, &en_link) < 0)
		return -EINVAL;
	dev_info(dev, "mipi_link_store() %d\n", en_link);
	
	if ((en_link == 1) || (en_link == 2) || (en_link == 4)) {
		it6510->csi.lane_cnt = en_link;
		if (it6510->attr_enable_stream)
			it6510_s_stream(sd, true);
	}
	
	return count;
}



static ssize_t power_up_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct it6510 *it6510 = sd_to_it6510(sd);

	return scnprintf(buf, PAGE_SIZE, "%d\n",(int)it6510->power_up);
}

static ssize_t power_up_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct v4l2_subdev *sd = dev_get_drvdata(dev);
	struct it6510 *it6510 = sd_to_it6510(sd);
	int power_up;

	if (kstrtoint(buf, 10, &power_up) < 0)
		return -EINVAL;
		
	dev_info(dev, "power_up_store() %d\n", power_up);
	
	if (power_up == 0)
		power_up = false;
	else
		power_up = true;

	it6510_set_power(it6510, power_up);

	if (power_up == true)
		it6510_enable_dprx(it6510, true);
	
	return count;
}


//file op debug
//
static DEVICE_ATTR_RW(mipi_link);
static DEVICE_ATTR_RW(enable_stream);
static DEVICE_ATTR_RW(mipi_reg);
static DEVICE_ATTR_RO(edid_ram);
static DEVICE_ATTR_RW(dp_reg);
static DEVICE_ATTR_RW(power_up);

static const struct attribute *it6510_attrs[] = {
	&dev_attr_power_up.attr,
	&dev_attr_enable_stream.attr,
	&dev_attr_mipi_link.attr,
	&dev_attr_dp_reg.attr,
	&dev_attr_mipi_reg.attr,
	&dev_attr_edid_ram.attr,
	NULL,
};

#ifdef CONFIG_OF
static int it6510_parst_dt(struct it6510 *it6510)
{
	struct device *dev = &it6510->dp_i2c->dev;
	struct v4l2_fwnode_endpoint endpoint = {.bus_type = V4L2_MBUS_CSI2_DPHY};
	struct device_node *ep;
	u8 tmp;
	int ret = -EINVAL;

	//read device para
	if (device_property_read_u8(dev, "dp-lanes", &tmp) == 0)
		it6510->dpcd.lane = tmp;

	if (device_property_read_u8(dev, "dp-bitrate", &tmp) == 0)
		it6510->dpcd.bitrate = tmp;


	//read end piont 
	ep = of_graph_get_next_endpoint(dev->of_node, NULL);
	if (!ep) {
		dev_err(dev, "missing endpoint node\n");
		return -EINVAL;
	}

	ret = v4l2_fwnode_endpoint_alloc_parse(of_fwnode_handle(ep) ,&endpoint);
	if (ret) {
		dev_err(dev, "failed to parse endpoint\n");
		goto put_node;
	}

	if (endpoint.bus_type != V4L2_MBUS_CSI2_DPHY ||
	    endpoint.bus.mipi_csi2.num_data_lanes == 0 ||
	    endpoint.nr_of_link_frequencies == 0) {
		dev_err(dev, "missing CSI-2 properties in endpoint\n");
		goto free_endpoint;
	}

	if (endpoint.bus.mipi_csi2.num_data_lanes > 4) {
		dev_err(dev, "invalid number of lanes\n");
		goto free_endpoint;
	}


	it6510->csi.lane_cnt = endpoint.bus.mipi_csi2.num_data_lanes;

	dev_dbg(dev, "Display Port lanes = %d\n", it6510->dpcd.lane);
	dev_dbg(dev, "Display Port bitRate = %d.%dG\n",
							(it6510->dpcd.bitrate*27)/100,
							(it6510->dpcd.bitrate*27)%100);
	dev_dbg(dev, "csi lanes = %d\n", it6510->csi.lane_cnt);

	ret = 0;

free_endpoint:
	v4l2_fwnode_endpoint_free(&endpoint);
put_node:
	of_node_put(ep);
	return ret;
}
#endif

static void it6510_init_para(struct it6510 *it6510)
{
	dprx_init_dpcd(it6510);
	mipi_init_csi_bus_para(it6510);
	memcpy(it6510->edid_data, default_edid, sizeof(it6510->edid_data));
	
	it6510->dprx.rs_level = DEFAULT_RS_LEVEL;
	it6510->dprx.audio_i2s = AUDIO_I2S;
	
	it6510->dprx.support_hdcp = false;
	it6510->dprx.support_hdcp2 = false;

	it6510->csi.lane_cnt = MIPI_DATA_LANES;
	it6510_set_mipi_color_fmt(it6510, MBUS_FMT_CODE);

}

static int  it6510_initial_clients(struct it6510 *it6510)
{

	struct i2c_client *client = it6510->dp_i2c;

	it6510->mipi_i2c = i2c_new_dummy_device(client->adapter,
					MIPI_I2C_ADR >> 1);

	if (IS_ERR(it6510->mipi_i2c))
		return PTR_ERR(it6510->mipi_i2c);

	it6510->mipi_regmap = devm_regmap_init_i2c(it6510->mipi_i2c,
					&it6510_mipi_regmap_config);

	if (IS_ERR(it6510->mipi_regmap))
		return PTR_ERR(it6510->mipi_regmap);


	it6510->edid_i2c = i2c_new_dummy_device(client->adapter,
					RAM_I2C_ADR >> 1);

	if (IS_ERR(it6510->edid_i2c))
		return PTR_ERR(it6510->edid_i2c);

	it6510->edid_regmap = devm_regmap_init_i2c(it6510->edid_i2c,
					&it6510_edid_regmap_config);
	
	if (IS_ERR(it6510->edid_regmap))
		return PTR_ERR(it6510->edid_regmap);

	return 0;
}

static void it6510_unregister_clients(struct it6510 *it6510)
{
	i2c_unregister_device(it6510->mipi_i2c);
	i2c_unregister_device(it6510->edid_i2c);	
}

static int it6510_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	//struct device_node *np = dev->of_node;
	struct it6510 *it6510;
	struct v4l2_subdev *sd;
	int err = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -EIO;

	dev_info(dev, "it6510_probe()\n");

	it6510 = devm_kzalloc(dev, sizeof(*it6510), GFP_KERNEL);
	if (!it6510)
		return -ENOMEM;

	it6510->dp_i2c = client;
	i2c_set_clientdata(client, it6510);

	it6510->dp_regmap = devm_regmap_init_i2c(client,
					&it6510_dp_regmap_config);

	if (IS_ERR(it6510->dp_i2c))
		return PTR_ERR(it6510->dp_i2c);
		
	if (it6510_read_ids(it6510) != 0)
		return -EIO;

	it6510_init_para(it6510);

	#ifdef CONFIG_OF
	it6510_parst_dt(it6510);
	#endif

	mutex_init(&it6510->lock);

	err = it6510_initial_clients(it6510);
	
	if (err)
		goto err_free_mutex;

	if (!client->irq) {
		dev_err(dev, "Failed to get CABLE_DET and INTP IRQ");
		err = -ENODEV;
		goto err_unregister_clients;
	}


	init_waitqueue_head(&it6510->wq);
    INIT_DELAYED_WORK(&it6510->delayed_audio, dprx_delayed_audio_work);    
    INIT_DELAYED_WORK(&it6510->delayed_link_check, dprx_delayed_link_work);

	it6510_initial(it6510);
	
	err = devm_request_threaded_irq(&client->dev, client->irq, NULL,
					it6510_intp_threaded_handler,
					IRQF_TRIGGER_LOW | IRQF_ONESHOT,
					"it6510-intp", it6510);
	if (err) {
		dev_err(dev, "Failed to request INTP threaded IRQ: %d",
			      err);
		goto err_unregister_clients;
	}

	err = sysfs_create_files(&client->dev.kobj, it6510_attrs);
	if (err) {
		goto err_unregister_clients;
	}


	sd = &it6510->sd;
	v4l2_i2c_subdev_init(sd, client, &it6510_ops);
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

	/* control handlers */
	v4l2_ctrl_handler_init(&it6510->hdl, 0);
	sd->ctrl_handler = &it6510->hdl;


	it6510->pad.flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.function = MEDIA_ENT_F_VID_IF_BRIDGE;
	err = media_entity_pads_init(&sd->entity, 1, &it6510->pad);
	dev_info(dev, "media_entity_pads_init() %d\n",err);
	if (err < 0)
		goto err_hdl;

	sd->dev = &client->dev;
	err = v4l2_async_register_subdev(sd);
	dev_info(dev, "v4l2_async_register_subdev() %d\n",err);
	if (err < 0)
		goto err_hdl;

	it6510->power_up = true;
	it6510_enable_dprx(it6510, true);

	return err;

err_hdl:
	v4l2_ctrl_handler_free(&it6510->hdl);
	media_entity_cleanup(&sd->entity);
err_unregister_clients:
	it6510_unregister_clients(it6510);

err_free_mutex:
	mutex_destroy(&it6510->lock);

	return err;
}

static int it6510_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct it6510 *it6510 = sd_to_it6510(sd);

	cancel_delayed_work(&it6510->delayed_audio);
	cancel_delayed_work(&it6510->delayed_link_check);
	
	sysfs_remove_files(&client->dev.kobj, it6510_attrs);
	v4l2_async_unregister_subdev(sd);
	v4l2_device_unregister_subdev(sd);

	v4l2_ctrl_handler_free(&it6510->hdl);
	
	it6510_unregister_clients(it6510);
	media_entity_cleanup(&sd->entity);
	mutex_destroy(&it6510->lock);


	return 0;
}

static const struct of_device_id it6510_dt_ids[] = {
	{ .compatible = "ite,it6510", },
	{ }
};
MODULE_DEVICE_TABLE(of, it6510_dt_ids);

static const struct i2c_device_id it6510_i2c_ids[] = {
	{ "it6510", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, it6510_i2c_ids);

static struct i2c_driver it6510_driver = {
	.probe = it6510_probe,
	.remove = it6510_remove,
	.driver = {
		.name = "it6510",
		.of_match_table = it6510_dt_ids,
	},
	.id_table = it6510_i2c_ids,
};

module_i2c_driver(it6510_driver);

MODULE_AUTHOR("Hermes Wu <hermes.wu@ite.com.tw>");
MODULE_DESCRIPTION("IT6510 DisplayPort to MIPI CSI driver");
MODULE_LICENSE("GPL v2");
