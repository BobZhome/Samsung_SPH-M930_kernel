/*
 * Copyright (c) 2008 QUALCOMM USA, INC.
 * Author: Haibo Jeff Zhong <hzhong@qualcomm.com>
 * 
 * All source code in this file is licensed under the following license
 * except where indicated.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can find it at http://www.fsf.org
 *
 */

//M4MO BY PGH

#ifndef M5MO_H
#define M5MO_H



#include <mach/board.h> //PGH
//#include "pgh_debug.h" 

#define	CAMERA_DEBUG	

#if 0
//unuse standby pin #define CAM_STB						85
#define CAM_RST						17
#define CAM_ON						76//REV02
//#define CAM_ON						22//REV01
#define CAM_FLASH_EN				23
#define CAM_FLASH_SET				31
#endif
#include <linux/mfd/pmic8058.h>

#define	CAM_TYPE_8M	0
#define	CAM_TYPE_VGA	1

#if (CONFIG_BOARD_REVISION >= 0x01)
#define	CAM_8M_RST			PM8058_GPIO_PM_TO_SYS(25)	// must sub 1 - PM_GPIO_26
#define	CAM_VGA_RST			PM8058_GPIO_PM_TO_SYS(29)	//PM_GPIO_30
#else
#define	CAM_8M_RST			0
#define	CAM_VGA_RST		31
#endif
#define	CAM_MEGA_EN			1	// STEALTH REV03 below
#define	CAM_MEGA_EN_REV04	74	// STEALTH REV04 
#if (CONFIG_BOARD_REVISION >= 0x01)
#define	CAM_VGA_EN		PM8058_GPIO_PM_TO_SYS(24)	// PM_GPIO_25
#else
#define	CAM_VGA_EN		2
#endif
#define	CAM_PMIC_STBY			3	// PMIC EN (CAM_IO_EN)

#define	CAM_SENSOR_A_EN		PM8058_GPIO_PM_TO_SYS(39)	// STEALTH HW REV04 PM8058 GPIO 40
#define	CAM_SENSOR_A_EN_ALTER		PM8058_GPIO_PM_TO_SYS(12)	// STEALTH beyond HW REV07 PM8058 GPIO 13		

#define	ON		1
#define	OFF		0
#define LOW							0
#define HIGH							1
#define M5MO_IMG_I2C_ADDR			0x3F


#define	PREVIEW_NORMAL		0
#define	PREVIEW_WIDE			1
#define	PREVIEW_MAX			2


#define QVGA 0
#define HVGA 1
#define VGA 2

#define PREVIEW_SIZE HVGA


#define FLASH_CMD           200

#define FLASH_CAMERA       0
#define FLASH_MOVIE        1  

#define FLASH_CMD_ON      1
#define FLASH_CMD_OFF     0

#define RESET_FOR_FW            202

#define MODE_CMD			203

#define MODE_CAMERA			0
#define MODE_CAMCORDER		1
#define MODE_FW				2

#define	SCREEN_SIZE_WIDTH			800
#define	SCREEN_SIZE_HEIGHT		480

/* Interrupt Factor */
#define M5MO_INT_SOUND			(1 << 7)
#define M5MO_INT_LENS_INIT		(1 << 6)
#define M5MO_INT_FD			(1 << 5)
#define M5MO_INT_FRAME_SYNC	(1 << 4)
#define M5MO_INT_CAPTURE		(1 << 3)
#define M5MO_INT_ZOOM			(1 << 2)
#define M5MO_INT_AF			(1 << 1)
#define M5MO_INT_MODE			(1 << 0)

/* FPS Capabilities */
#define M5MO_5_FPS				5
#define M5MO_15_FPS			15
#define M5MO_30_FPS			30
#define M5MO_60_FPS			60
/*
#define M5MO_5_FPS				0x1
#define M5MO_7_FPS				0x2
#define M5MO_10_FPS			0x3
#define M5MO_12_FPS			0x4
#define M5MO_15_FPS			0x5
#define M5MO_24_FPS			0x6
#define M5MO_30_FPS			0x7
#define M5MO_60_FPS			0x8
#define M5MO_120_FPS			0x9
#define M5MO_AUTO_FPS			0xA
*/
/* M5MO Sensor Mode */
#define M5MO_SYSINIT_MODE		0x0
#define M5MO_PARMSET_MODE	0x1
#define M5MO_MONITOR_MODE	0x2
#define M5MO_STILLCAP_MODE	0x3

/* M5MO Preview Size */
#define M5MO_SUB_QCIF_SIZE	0x1	
#define M5MO_QQVGA_SIZE		0x3		
#define M5MO_144_176_SIZE		0x4
#define M5MO_QCIF_SIZE			0x5
#define M5MO_176_176_SIZE		0x6
#define M5MO_LQVGA_SIZE		0x8		
#define M5MO_QVGA_SIZE			0x9		
#define M5MO_LWQVGA_SIZE		0xC		
#define M5MO_WQVGA_SIZE		0xD		
#define M5MO_CIF_SIZE			0xE		
#define M5MO_480_360_SIZE		0x13		
#define M5MO_QHD_SIZE			0x15		
#define M5MO_VGA_SIZE			0x17		
#define M5MO_NTSC_SIZE			0x18		
#define M5MO_WVGA_SIZE		0x1A		
#define M5MO_SVGA_SIZE			0x1F		// 1600x1200
#define M5MO_HD_SIZE			0x21		
#define M5MO_FULL_HD_SIZE		0x25
#define M5MO_8M_SIZE			0x29
#define M5MO_QVGA_SL60_SIZE	0x30		
#define M5MO_QVGA_SL120_SIZE	0x31		

/* M5MO Capture Size */
#define M5MO_SHOT_QVGA_SIZE		0x2
#define M5MO_SHOT_WQVGA_SIZE		0x4
#define M5MO_SHOT_480_360_SIZE	0x7
#define M5MO_SHOT_QHD_SIZE		0x8
#define M5MO_SHOT_VGA_SIZE		0x9
#define M5MO_SHOT_WVGA_SIZE		0xA
#define M5MO_SHOT_HD_SIZE			0x10
#define M5MO_SHOT_1M_SIZE			0x14
#define M5MO_SHOT_2M_SIZE			0x17	//1600x1200
#define M5MO_SHOT_FULL_HD_SIZE	0x19
#define M5MO_SHOT_3M_1_SIZE		0x1A
#define M5MO_SHOT_3M_2_SIZE		0x1B
#define M5MO_SHOT_4M_1_SIZE		0x1C
#define M5MO_SHOT_4M_2_SIZE		0x1D	//2560x1536
#define M5MO_SHOT_5M_SIZE			0x1F	//2560x1920
#define M5MO_SHOT_6M_1_SIZE		0x21
#define M5MO_SHOT_6M_2_SIZE		0x22	//3264x1960
#define M5MO_SHOT_8M_SIZE			0x25
#define M5MO_SHOT_W1_5_SIZE			0x2B	// 1600x960 added

/* M5MO Thumbnail Size */
#define M5MO_THUMB_QVGA_SIZE		0x1
#define M5MO_THUMB_400_225_SIZE	0x2
#define M5MO_THUMB_WQVGA_SIZE	0x3
#define M5MO_THUMB_VGA_SIZE		0x4
#define M5MO_THUMB_WVGA_SIZE		0x5

#define AF_SUCCEED	2
#define AF_FAILED	1
#define AF_NONE		0

/* M5MO Auto Focus Mode */
#define M5MO_AF_STOP  					0
#define M5MO_AF_INIT_NORMAL 			1
#define M5MO_AF_INIT_MACRO  			2
#define M5MO_AF_START 					3
#define M5MO_AF_RELEASE				4
#define M5MO_AF_INIT_TOUCH			5
//#define M5MO_AF_INIT_TOUCH_MACRO 	6

#define M5MO_AF_MODE_MACRO  			1
#define M5MO_AF_MODE_NORMAL 			4	// == auto
#define M5MO_AF_MODE_CAF				2
#define M5MO_AF_MODE_FACEDETECT  	3
#define M5MO_AF_MODE_TOUCHAF			5
#define M5MO_AF_MODE_NIGHTSHOT		6


#define M5MO_AF_CONTINUOUS_OFF		0
#define M5MO_AF_CONTINUOUS_ON		1

#define M5MO_AF_STATUS_MOVING		0
#define M5MO_AF_STATUS_SUCCESS		1
#define M5MO_AF_STATUS_FAILED			2

/* Face Detection */
#define M5MO_FACE_DETECTION_OFF		0
#define M5MO_FACE_DETECTION_ON		1

/* Image Effect */
#define M5MO_EFFECT_OFF				0
#define M5MO_EFFECT_MONO				1
#define M5MO_EFFECT_NEGATIVE			2
#define M5MO_EFFECT_SOLARIZATION3	3
#define M5MO_EFFECT_SEPIA				4
#define M5MO_EFFECT_AQUA			   	8

#define M5MO_EFFECT_RED				4
#define M5MO_EFFECT_GREEN				5
#define M5MO_EFFECT_BLUE				6
#define M5MO_EFFECT_PINK				7
#define M5MO_EFFECT_YELLOW			8
#define M5MO_EFFECT_PURPLE			9
#define M5MO_EFFECT_ANTIQUE			10

//#define M5MO_EFFECT_SOLARIZATION1		12
//#define M5MO_EFFECT_SOLARIZATION2		13

//#define M5MO_EFFECT_SOLARIZATION4		15
#define M5MO_EFFECT_EMBOSS			16
#define M5MO_EFFECT_OUTLINE			17

/* ISO */
#define M5MO_ISO_AUTO					0
#define M5MO_ISO_50					1
#define M5MO_ISO_100					2
#define M5MO_ISO_200					3
#define M5MO_ISO_400					4
#define M5MO_ISO_800					5
#define M5MO_ISO_1600					6
#define M5MO_ISO_3200					7

/* EV */
#define M5MO_EV_MINUS_4				0
#define M5MO_EV_MINUS_3				1
#define M5MO_EV_MINUS_2				2
#define M5MO_EV_MINUS_1				3
#define M5MO_EV_DEFAULT				4
#define M5MO_EV_PLUS_1					5
#define M5MO_EV_PLUS_2					6
#define M5MO_EV_PLUS_3					7
#define M5MO_EV_PLUS_4					8

/* Exposure mode */
/* Saturation*/
#define M5MO_METERING_AVERAGE		0
#define M5MO_METERING_CENTER			1
#define M5MO_METERING_SPOT			2

/* Saturation*/
#define M5MO_SATURATION_MINUS_2		0
#define M5MO_SATURATION_MINUS_1		1
#define M5MO_SATURATION_DEFAULT		2
#define M5MO_SATURATION_PLUS_1		3
#define M5MO_SATURATION_PLUS_2		4

/* Sharpness */
#define M5MO_SHARPNESS_MINUS_2		0
#define M5MO_SHARPNESS_MINUS_1		1
#define M5MO_SHARPNESS_DEFAULT		2
#define M5MO_SHARPNESS_PLUS_1		3
#define M5MO_SHARPNESS_PLUS_2		4

/* Contrast */
#define M5MO_CONTRAST_MINUS_2		0
#define M5MO_CONTRAST_MINUS_1		1
#define M5MO_CONTRAST_DEFAULT		2
#define M5MO_CONTRAST_PLUS_1			3
#define M5MO_CONTRAST_PLUS_2			4

/* White Balance */
#define M5MO_WB_AUTO					1
#define M5MO_WB_INCANDESCENT			3
#define M5MO_WB_FLUORESCENT			4
#define M5MO_WB_DAYLIGHT				5
#define M5MO_WB_CLOUDY				6
#define M5MO_WB_SHADE					8
#define M5MO_WB_HORIZON				7
#define M5MO_WB_LED					9

/* Flash Setting */
#define M5MO_FLASH_CAPTURE_OFF		0
#define M5MO_FLASH_CAPTURE_AUTO		1
#define M5MO_FLASH_CAPTURE_ON		2
#define M5MO_FLASH_MOVIE_ON			3


/* Scene Mode */ 
#define M5MO_SCENE_NORMAL				0
#define M5MO_SCENE_LANDSCAPE			1
#define M5MO_SCENE_BEACH_SNOW		2
#define M5MO_SCENE_SUNSET				4
#define M5MO_SCENE_NIGHT				5
#define M5MO_SCENE_PORTRAIT			6
#define M5MO_SCENE_BACKLIGHT			7
#define M5MO_SCENE_SPORTS				8
#define M5MO_SCENE_FLOWERS			10
#define M5MO_SCENE_CANDLELIGHT		11
#define M5MO_SCENE_FIREWORK			12
#define M5MO_SCENE_PARTY_INDOOR		13


#define M5MO_SCENE_DUSK_DAWN			20
#define M5MO_SCENE_FALL_COLOR			21
#define M5MO_SCENE_TEXT				22
#define M5MO_SCENE_SHOWWINDOW		23

/* JPEG Quality */ 
#define M5MO_JPEG_SUPERFINE			1
#define M5MO_JPEG_FINE					2
#define M5MO_JPEG_NORMAL				3
#define M5MO_JPEG_ECONOMY				4

/* WDR */
#define M5MO_WDR_OFF					0
#define M5MO_WDR_ON					1

/* Image Stabilization */
#define M5MO_ISC_STILL_OFF				1
#define M5MO_ISC_STILL_ON				2
#define M5MO_ISC_STILL_AUTO			3 /* Not Used */
#define M5MO_ISC_MOVIE_ON				4 /* Not Used */

#define M5MO_I2C_VERIFY_RETRY			200

#define M5MO_FW_PATH_SDCARD			"/sdcard/RS_M5LS.bin"
#define M5MO_FW_PATH_IMAGE			"/system/firmware/RS_M5LS.bin"
#define M5MO_FW_DUMP_PATH		"/tmp/cam_fw.bin"
#define M5MO_FLASH_BASE_ADDR	0x10000000
#define M5MO_INT_RAM_BASE_ADDR	0x68000000
#define M5MO_VERSION_INFO_SIZE	21
#define M5MO_VERSION_INFO_ADDR	0x1016FF00 //0x16fee0 // offset in binary file 

#define M5MO_CATEGORY_FLASH		0x0F    /* F/W update */
/* M5MO_CATEGORY_FLASH */
#define M5MO_FLASH_ADDR			0x00
#define M5MO_FLASH_BYTE			0x04
#define M5MO_FLASH_ERASE		0x06
#define M5MO_FLASH_WR			0x07
#define M5MO_FLASH_RAM_CLEAR	0x08
#define M5MO_FLASH_CAM_START	0x12
#define M5MO_FLASH_SEL			0x13

/* DTP */
#define M5MO_DTP_OFF		0
#define M5MO_DTP_ON		1
#define M5MO_DTP_OFF_ACK		2
#define M5MO_DTP_ON_ACK		3

struct m5mo_picture_size
{
	int width;
	int height;
	int reg_val;
};
struct m5mo_preview_size
{
	int width;
	int height;
};
struct m5mo_userset {
	unsigned int focus_mode;
	unsigned int focus_status;
	unsigned int continuous_af;

	unsigned int	metering;
	unsigned int	exposure;
	unsigned int		wb;
	unsigned int		iso;
	int	contrast;
	int	saturation;
	int	sharpness;
	int	ev;

	unsigned int zoom;
	unsigned int effect;	/* Color FX (AKA Color tone) */
	unsigned int scenemode;
	unsigned int detectmode;
	unsigned int antishake;
	
	unsigned int stabilize;	/* IS */

	unsigned int strobe;
	unsigned int jpeg_quality;
	//unsigned int preview_size;
	struct m5mo_preview_size preview_size;
	unsigned int preview_size_idx;
	unsigned int capture_size;
	unsigned int thumbnail_size;
	unsigned int fps;

	unsigned int started;
};

struct m5mo_ctrl_t {
	int8_t  opened;
	struct  msm_camera_sensor_info 	*sensordata;

	struct m5mo_userset settings;
	
};

struct m5mo_exif_data {
	u32	ep_numerator;
	u32	ep_denominator;
	u32	tv_numerator;
	u32	tv_denominator;
	u32	av_numerator;
	u32	av_denominator;
	u32	bv_numerator;
	u32	bv_denominator;
	u32	ebv_numerator;
	u32	ebv_denominator;
	u32	iso;
	u32	flash;
	
};

/* gtuo.park */
int32_t m4mo_i2c_write_8bit_external(unsigned char category, unsigned char byte, unsigned char value);

int cam_pmic_onoff(int onoff);
static int m5mo_mem_write(u16 len, u32 addr, u8 *val);
static int m5mo_mem_read(u16 len, u32 addr, u8 *val);


#endif /* M5MO_H */

