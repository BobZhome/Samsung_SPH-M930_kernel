/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#ifdef CONFIG_MACH_CHIEF
#include "s5k5cc.h"
#else
#include "s5k5cc_vital2.h"
#endif

#include <mach/vreg.h>
#include <mach/camera.h>
#ifdef CONFIG_KEYBOARD_ADP5587
#include <linux/i2c/adp5587.h>
#endif
#include <linux/mfd/pmic8058.h>

#define SENSOR_DEBUG 0
#undef CONFIG_LOAD_FILE
//#define CONFIG_LOAD_FILE
#define I2C_BURST_MODE //dha23 100325 카메라 기동 시간 줄이기 위해 I2C Burst mode 사용.

#if defined(CONFIG_MACH_CHIEF) || defined(CONFIG_MACH_VITAL2) || defined (CONFIG_MACH_ROOKIE2) || \
	defined(CONFIG_MACH_PREVAIL2)
extern struct class *sec_class;
struct device *flash_dev;
#endif

////////////////////////////////////////
#ifdef CONFIG_MACH_PREVAIL2
#define CAM_RESET 130
#define CAM_STANDBY 131
#define CAM_EN_2 132
#define CAM_EN 3
#define CAM_I2C_SCL 30
#define CAM_I2C_SDA 17
#define CAM_VT_nSTBY 2
#define CAM_VT_RST 175
#define CAM_MCLK 15	
#else
#if 1
#ifdef CONFIG_MACH_CHIEF
#define CAM_RESET ((system_rev>=6)?161:75)
#define CAM_STANDBY ((system_rev>=6)?132:74)
#else //vital2
#define CAM_RESET 130
#define CAM_STANDBY 131
#define CAM_EN_2 132
#endif
#define CAM_EN 3
#define CAM_I2C_SCL 30
#define CAM_I2C_SDA 17
#define CAM_VT_nSTBY 2
#define CAM_VT_RST 175
#define CAM_MCLK 15	
#endif
#endif
#define PMIC_GPIO_CAM_FLASH_SET	PM8058_GPIO(27)
#define PMIC_GPIO_CAM_FLASH_EN	PM8058_GPIO(28)

#define PCAM_CONNECT_CHECK		0
#define PCAM_VT_MODE        		1
#define PCAM_EXPOSURE_TIME		2
#define PCAM_ISO_SPEED			3
#define PCAM_FIXED_FRAME		4
#define PCAM_AUTO_FRAME			5
#define PCAM_NIGHT_SHOT			6
#define PCAM_FLASH_OFF			7
#define PCAM_MOVIE_FLASH_ON		8
#define PCAM_PREVIEW_FPS		9
#define PCAM_GET_FLASH			10
#define PCAM_LOW_TEMP			11

#define MOVIEMODE_FLASH 	17
#define FLASHMODE_FLASH 	18
#define FLASHMODE_AUTO	2
#define FLASHMODE_ON	3
#define FLASHMODE_OFF	1

struct s5k5cc_work {
	struct work_struct work;
};

static struct  s5k5cc_work *s5k5cc_sensorw;
static struct  i2c_client *s5k5cc_client;

struct s5k5cc_ctrl {
	const struct msm_camera_sensor_info *sensordata;
};


static int  sceneNight = 0;
static int  fireWorks = 0;
static int  AELock = 0;
static int  AWBLock = 0;

static int s5k5cc_set_flash(int lux_val);
static struct s5k5cc_ctrl *s5k5cc_ctrl;

static DECLARE_WAIT_QUEUE_HEAD(s5k5cc_wait_queue);
DECLARE_MUTEX(s5k5cc_sem);


#ifdef CONFIG_LOAD_FILE
static int s5k5cc_regs_table_write(char *name);
#endif
static int cam_hw_init(void);


static int previous_scene_mode = -1;
static int previous_WB_mode = 0;

static int16_t s5k5cc_effect = CAMERA_EFFECT_OFF;

typedef enum{
    af_stop = 0,
    af_running,
    af_status_max
};

static int af_status = af_status_max;

typedef enum{
    af_position_auto = 0,
    af_position_infinity,
    af_position_macro,
    af_position_max
};
static unsigned short AFPosition[af_position_max] = {0xFF, 0xFF, 0x48, 0xFF };       // auto, infinity, macro, default
static unsigned short DummyAFPosition[af_position_max] = {0xFE, 0xFE, 0x50, 0xFE};     // auto, infinity, macro, default
static unsigned short set_AF_postion = 0xFF;
static unsigned short set_Dummy_AF_position = 0xFE;
static unsigned short lux_value = 0;
static int preview_start = 0;
static int flash_mode = 1;
static int af_mode = 0;
static int flash_set_on_af = 0;
static int flash_set_on_snap = 0;
static int preflash_on = 0;
static int exif_flash_status = 0;
static int vtmode = 0;
static int low_temp = 0;

/*=============================================================
	EXTERNAL DECLARATIONS
==============================================================*/
extern struct s5k5cc_reg s5k5cc_regs;


/*=============================================================*/

static int s5k5cc_sensor_read(unsigned short subaddr, unsigned short *data)
{
	int ret;
	unsigned char buf[2];
	struct i2c_msg msg = { s5k5cc_client->addr, 0, 2, buf };
	
	buf[0] = (subaddr >> 8);
	buf[1] = (subaddr & 0xFF);

	ret = i2c_transfer(s5k5cc_client->adapter, &msg, 1) == 1 ? 0 : -EIO;
	if (ret == -EIO) 
        {   
              printk("[S5K5CC] s5k5cc_sensor_read fail : %d \n", ret);                 
		goto error;
        }

	msg.flags = I2C_M_RD;
	
	ret = i2c_transfer(s5k5cc_client->adapter, &msg, 1) == 1 ? 0 : -EIO;
	if (ret == -EIO) 
        {   
              printk("[S5K5CC] s5k5cc_sensor_read fail : %d \n", ret);                 
		goto error;
        }

	*data = ((buf[0] << 8) | buf[1]);

error:
	return ret;
}

int s5k5cc_sensor_write(unsigned short subaddr, unsigned short val)
{
	unsigned char buf[4];
	struct i2c_msg msg = { s5k5cc_client->addr, 0, 4, buf };
#if 0
	printk("[PGH] on write func s5k5cc_client->addr : %x\n", subaddr);
	printk("[PGH] on write func  s5k5cc_client->adapter->nr : %x\n", val);
#endif
	buf[0] = (subaddr >> 8);
	buf[1] = (subaddr & 0xFF);
	buf[2] = (val >> 8);
	buf[3] = (val & 0xFF);

        if(i2c_transfer(s5k5cc_client->adapter, &msg, 1) == 1)
        {
            return 0;
        }
        else
        {
            printk("[S5K5CC] s5k5cc_sensor_write fail \n");        
            return -EIO;
        }
//	return i2c_transfer(s5k5cc_client->adapter, &msg, 1) == 1 ? 0 : -EIO;
}

int s5k5cc_sensor_write_list(struct samsung_short_t *list,char *name)
{
#ifndef CONFIG_LOAD_FILE 
	int i;
#endif
       int ret;
	ret = 0;

#ifdef CONFIG_LOAD_FILE 
	printk("[S5K5CC]%s (%s)\n", __func__, name);
	s5k5cc_regs_table_write(name);	
#else
	printk("[S5K5CC]%s (%s)\n", __func__, name);
	for (i = 0; list[i].subaddr != 0xffff; i++)
	{
	    if(s5k5cc_sensor_write(list[i].subaddr, list[i].value) < 0)
            {
                printk("[S5K5CC] s5k5cc_sensor_write_list fail : %d, %x, %x \n", i, list[i].value, list[i].subaddr);
                return -1;
             }
	}
#endif
	return ret;
}

void sensor_rough_control(void __user *arg)      
{
	ioctl_pcam_info_8bit		ctrl_info;

	int Exptime;
	int Expmax;
	unsigned short read_1, read_2, read_3;	

//	printk("[S5K5CC] sensor_rough_control\n");

	if(copy_from_user((void *)&ctrl_info, (const void *)arg, sizeof(ctrl_info)))
	{
		printk("<=PCAM=> %s fail copy_from_user!\n", __func__);
	}
//	printk("<=PCAM=> TEST %d %d %d %d %d \n", ctrl_info.mode, ctrl_info.address, ctrl_info.value_1, ctrl_info.value_2, ctrl_info.value_3);


	switch(ctrl_info.mode)
	{
		case PCAM_CONNECT_CHECK:
                    printk("[S5K5CC] PCAM_CONNECT_CHECK\n");   
                    int rc = 0;
                    rc = s5k5cc_sensor_write(0x002C, 0x7000);
                    if(rc < 0) //check sensor connection
                    {
                       printk("[S5K5CC] Connect error\n");                       
                       ctrl_info.value_1 = 1;
                    }
                    break;

		case PCAM_VT_MODE:
                     printk("[S5K5CC] PCAM_VT_MODE : %d\n", ctrl_info.address);   
                     vtmode = ctrl_info.address;
			break;

		case PCAM_EXPOSURE_TIME:
                    s5k5cc_sensor_write(0xFCFC, 0xD000);
                    s5k5cc_sensor_write(0x002C, 0x7000);
                    s5k5cc_sensor_write(0x002E, 0x2A14);
                    s5k5cc_sensor_read(0x0F12, &ctrl_info.value_1); // LSB
                    s5k5cc_sensor_read(0x0F12, &ctrl_info.value_2); // MSB
                    printk("[S5K5CC] PCAM_EXPOSURE_TIME : LSB(%x), MSB(%x)\n]",ctrl_info.value_1,ctrl_info.value_2);
                    break;

		case PCAM_ISO_SPEED:
                    printk("[S5K5CC] PCAM_ISO_SPEED\n");            
                    s5k5cc_sensor_write(0xFCFC, 0xD000);
                    s5k5cc_sensor_write(0x002C, 0x7000);
                    s5k5cc_sensor_write(0x002E, 0x2A18);
                    s5k5cc_sensor_read(0x0F12, &ctrl_info.value_1);
                    break;

		case PCAM_FIXED_FRAME:
                    printk("[S5K5CC] PCAM_FIXED_FRAME\n");   
#if defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || defined(CONFIG_MACH_PREVAIL2)
                    msleep(50);
#endif
                    s5k5cc_sensor_write_list(s5k5cc_30_fps,"s5k5cc_30_fps");
#if defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || defined(CONFIG_MACH_PREVAIL2)
                    msleep(150);
#endif
                    break;

		case PCAM_AUTO_FRAME:
                    printk("[S5K5CC] PCAM_AUTO_FRAME\n");
#if defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || defined(CONFIG_MACH_PREVAIL2)
                    msleep(50);
#endif
                    s5k5cc_sensor_write_list(s5k5cc_fps_nonfix,"s5k5cc_fps_nonfix");
#if defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || defined(CONFIG_MACH_PREVAIL2)
                    msleep(150);
#endif
                    break;

		case PCAM_NIGHT_SHOT:
                    printk("[S5K5CC] PCAM_NIGHT_SHOT\n");            
                    break;

		case PCAM_FLASH_OFF:
                    printk("[S5K5CC] PCAM_FLASH_OFF\n");      
                    s5k5cc_set_flash(0);
                    if(flash_set_on_snap)
                    {
                        s5k5cc_sensor_write_list(S5K5CC_Flash_End_EVT1,"S5K5CC_Flash_End_EVT1");
                    	   flash_set_on_snap = 0;
                    }
                    break;

		case PCAM_MOVIE_FLASH_ON:
                    printk("[S5K5CC] PCAM_MOVIE_FLASH_ON\n");      
                    s5k5cc_set_flash(MOVIEMODE_FLASH);
                    break;

		case PCAM_PREVIEW_FPS:
                    if(vtmode == 2||vtmode == 3)     
                    {
                        printk("[S5K5CC] PCAM_PREVIEW_FPS : %d\n", ctrl_info.address);  
                        if(ctrl_info.address == 15)
                            s5k5cc_sensor_write_list(s5k5cc_fps_15fix,"s5k5cc_fps_15fix");
                    }
                    break;

		case PCAM_GET_FLASH:
                    printk("[S5K5CC] PCAM_GET_FLASH : %d\n",exif_flash_status);  
                    ctrl_info.value_1 = exif_flash_status;
                    break;

		case PCAM_LOW_TEMP:
                    printk("[S5K5CC] PCAM_LOW_TEMP\n");  
                    low_temp = 1;
                    break;

		default :
			printk("<=PCAM=> Unexpected mode on sensor_rough_control!!!\n");
			break;
	}

	if(copy_to_user((void *)arg, (const void *)&ctrl_info, sizeof(ctrl_info)))
	{
		printk("<=PCAM=> %s fail on copy_to_user!\n", __func__);
	}
	
}

#ifdef I2C_BURST_MODE //dha23 100325
#define BURST_MODE_SET			1
#define BURST_MODE_END			2
#define NORMAL_MODE_SET			3
#define MAX_INDEX				1000
static int s5k5cc_sensor_burst_write_list(struct samsung_short_t *list,char *name)
{
	__u8 temp_buf[MAX_INDEX];
	int index_overflow = 1;
	int new_addr_start = 0;
	int burst_mode = NORMAL_MODE_SET;
	unsigned short pre_subaddr = 0;
	struct i2c_msg msg = { s5k5cc_client->addr, 0, 4, temp_buf };
	int i=0, ret=0;
	unsigned int index = 0;

        memset(temp_buf, 0x00 ,1000);
	
	printk("s5k5cc_sensor_burst_write_list( %s ) \n", name); 
//	printk("s5k5cc_sensor_i2c_address(0x%02x) \n", s5k5cc_client->addr);     
#ifdef CONFIG_LOAD_FILE 
	s5k5cc_regs_table_write(name);	
#else
	for (i = 0; list[i].subaddr != 0xffff; i++)
	{
		if(list[i].subaddr == 0xdddd)
		{
		    msleep(list[i].value);
                    printk("delay 0x%04x, value 0x%04x\n", list[i].subaddr, list[i].value);
		}	
		else
		{					
			if( list[i].subaddr == list[i+1].subaddr )
			{
				burst_mode = BURST_MODE_SET;
				if((list[i].subaddr != pre_subaddr) || (index_overflow == 1))
				{
					new_addr_start = 1;
					index_overflow = 0;
				}
			}
			else
			{
				if(burst_mode == BURST_MODE_SET)
				{
					burst_mode = BURST_MODE_END;
					if(index_overflow == 1)
					{
						new_addr_start = 1;
						index_overflow = 0;
					}
				}
				else
				{
					burst_mode = NORMAL_MODE_SET;
				}
			}

			if((burst_mode == BURST_MODE_SET) || (burst_mode == BURST_MODE_END))
			{
				if(new_addr_start == 1)
				{
					index = 0;
					//memset(temp_buf, 0x00 ,1000);
					index_overflow = 0;

					temp_buf[index] = (list[i].subaddr >> 8);
					temp_buf[++index] = (list[i].subaddr & 0xFF);

					new_addr_start = 0;
				}
				
				temp_buf[++index] = (list[i].value >> 8);
				temp_buf[++index] = (list[i].value & 0xFF);
				
				if(burst_mode == BURST_MODE_END)
				{
					msg.len = ++index;

					ret = i2c_transfer(s5k5cc_client->adapter, &msg, 1) == 1 ? 0 : -EIO;
					if( ret < 0)
					{
						printk("i2c_transfer fail ! \n");
						return -1;
					}
				}
				else if( index >= MAX_INDEX-1 )
				{
					index_overflow = 1;
					msg.len = ++index;
					
					ret = i2c_transfer(s5k5cc_client->adapter, &msg, 1) == 1 ? 0 : -EIO;
					if( ret < 0)
					{
						printk("I2C_transfer Fail ! \n");
						return -1;
					}
				}
				
			}
			else
			{
				//memset(temp_buf, 0x00 ,4);
			
				temp_buf[0] = (list[i].subaddr >> 8);
				temp_buf[1] = (list[i].subaddr & 0xFF);
				temp_buf[2] = (list[i].value >> 8);
				temp_buf[3] = (list[i].value & 0xFF);

				msg.len = 4;
				ret = i2c_transfer(s5k5cc_client->adapter, &msg, 1) == 1 ? 0 : -EIO;
				if( ret < 0)
				{
					printk("I2C_transfer Fail ! \n");
					return -1;
				}
			}
		}
		
		pre_subaddr = list[i].subaddr;
	}
#endif
	return ret;
}
#endif


static long s5k5cc_set_effect(int mode, int effect)
{
	long rc = 0;

	switch (effect) {
        	case CAMERA_EFFECT_OFF:
                	s5k5cc_sensor_write_list(s5k5cc_effect_off,"s5k5cc_effect_off");
      			break;

        	case CAMERA_EFFECT_MONO:
                	s5k5cc_sensor_write_list(s5k5cc_effect_gray,"s5k5cc_effect_gray");
        		break;

        	case CAMERA_EFFECT_NEGATIVE:
                	s5k5cc_sensor_write_list(s5k5cc_effect_negative,"s5k5cc_effect_negative");
        		break;

        	case CAMERA_EFFECT_SOLARIZE:
                	s5k5cc_sensor_write_list(s5k5cc_effect_aqua,"s5k5cc_effect_aqua");
        		break;

        	case CAMERA_EFFECT_SEPIA:
                	s5k5cc_sensor_write_list(s5k5cc_effect_sepia,"s5k5cc_effect_sepia");
        		break;

        	case CAMERA_EFFECT_AQUA:
                	s5k5cc_sensor_write_list(s5k5cc_effect_aqua,"s5k5cc_effect_aqua");
        		break;

         	case CAMERA_EFFECT_WHITEBOARD:
                	s5k5cc_sensor_write_list(s5k5cc_effect_sketch,"s5k5cc_effect_sketch");
        		break; 

        	default:
                	printk("[PGH] default .effect\n");
                	s5k5cc_sensor_write_list(s5k5cc_effect_off,"s5k5cc_effect_off");
			//return -EINVAL;
			return 0;

	}
	s5k5cc_effect = effect;

	return rc;
}

static long s5k5cc_set_brightness(int mode, int brightness)
{
	long rc = 0;

	switch (brightness) {
		case CAMERA_BRIGTHNESS_0:
			s5k5cc_sensor_write_list(s5k5cc_br_minus3, "s5k5cc_br_minus3");
			break;

		case CAMERA_BRIGTHNESS_1:
			s5k5cc_sensor_write_list(s5k5cc_br_minus2, "s5k5cc_br_minus2");
			break;

		case CAMERA_BRIGTHNESS_2:
			s5k5cc_sensor_write_list(s5k5cc_br_minus1, "s5k5cc_br_minus1");
			break;

		case CAMERA_BRIGTHNESS_3:
			s5k5cc_sensor_write_list(s5k5cc_br_zero, "s5k5cc_br_zero");
			break;

		case CAMERA_BRIGTHNESS_4:
			s5k5cc_sensor_write_list(s5k5cc_br_plus1, "s5k5cc_br_plus1");
			break;

		case CAMERA_BRIGTHNESS_5:
			s5k5cc_sensor_write_list(s5k5cc_br_plus2, "s5k5cc_br_plus2");
 			break;

		case CAMERA_BRIGTHNESS_6:
			s5k5cc_sensor_write_list(s5k5cc_br_plus3, "s5k5cc_br_plus3");
 			break;

		default:
#ifndef PRODUCT_SHIP
			printk("<=PCAM=> unexpected brightness %s/%d\n", __func__, __LINE__);
#endif
			//return -EINVAL;
			return 0;
 	}
	return rc;
}

static long s5k5cc_set_whitebalance(int mode, int wb)
{
	long rc = 0;

	switch (wb) {
		case CAMERA_WB_AUTO:
                        previous_WB_mode = wb;
			s5k5cc_sensor_write_list(s5k5cc_wb_auto, "s5k5cc_wb_auto");
			break;

		case CAMERA_WB_INCANDESCENT:
                        previous_WB_mode = wb;            
			s5k5cc_sensor_write_list(s5k5cc_wb_tungsten, "s5k5cc_wb_tungsten");
			break;

		case CAMERA_WB_FLUORESCENT:
                        previous_WB_mode = wb;            
			s5k5cc_sensor_write_list(s5k5cc_wb_fluorescent, "s5k5cc_wb_fluorescent");
			break;

		case CAMERA_WB_DAYLIGHT:
                        previous_WB_mode = wb;            
			s5k5cc_sensor_write_list(s5k5cc_wb_sunny, "s5k5cc_wb_sunny");
			break;

		case CAMERA_WB_CLOUDY_DAYLIGHT:
                        previous_WB_mode = wb;            
			s5k5cc_sensor_write_list(s5k5cc_wb_cloudy, "s5k5cc_wb_cloudy");
			break;

		default:
#ifndef PRODUCT_SHIP
			printk("<=PCAM=> unexpected WB mode %s/%d\n", __func__, __LINE__);
#endif
			//return -EINVAL;
			return 0;
 	}
	return rc;
}


static long s5k5cc_set_exposure_value(int mode, int exposure)
{
	long rc = 0;

	switch (exposure) {

		case CAMERA_EXPOSURE_NEGATIVE_2:
			s5k5cc_sensor_write_list(S5K5CCGX_EV_N_2, "S5K5CCGX_EV_N_2");
			break;

		case CAMERA_EXPOSURE_NEGATIVE_1:
			s5k5cc_sensor_write_list(S5K5CCGX_EV_N_1, "S5K5CCGX_EV_N_1");
			break;

		case CAMERA_EXPOSURE_0:
			s5k5cc_sensor_write_list(S5K5CCGX_EV_0, "S5K5CCGX_EV_0");
			break;

		case CAMERA_EXPOSURE_POSITIVE_1:
			s5k5cc_sensor_write_list(S5K5CCGX_EV_P_1, "S5K5CCGX_EV_P_1");
			break;

		case CAMERA_EXPOSURE_POSITIVE_2:
			s5k5cc_sensor_write_list(S5K5CCGX_EV_P_2, "S5K5CCGX_EV_P_2");
			break;

		default:
#ifndef PRODUCT_SHIP
			printk("<=PCAM=> unexpected Exposure Value %s/%d\n", __func__, __LINE__);
#endif
			//return -EINVAL;
			return 0;
	}
	return rc;
}

static long s5k5cc_set_auto_exposure(int mode, int metering)
{
	long rc = 0;

	switch (metering) {

		case CAMERA_AEC_CENTER_WEIGHTED:
#ifdef I2C_BURST_MODE //dha23 100325	
			s5k5cc_sensor_burst_write_list(s5k5cc_measure_brightness_center, "s5k5cc_measure_brightness_center");
#else            
			s5k5cc_sensor_write_list(s5k5cc_measure_brightness_center, "s5k5cc_measure_brightness_center");
#endif
			break;

		case CAMERA_AEC_SPOT_METERING:
#ifdef I2C_BURST_MODE //dha23 100325	
			s5k5cc_sensor_burst_write_list(s5k5cc_measure_brightness_spot, "s5k5cc_measure_brightness_spot");
#else            
			s5k5cc_sensor_write_list(s5k5cc_measure_brightness_spot, "s5k5cc_measure_brightness_spot");
#endif
			break;

		case CAMERA_AEC_FRAME_AVERAGE:
#ifdef I2C_BURST_MODE //dha23 100325	
			s5k5cc_sensor_burst_write_list(s5k5cc_measure_brightness_default, "s5k5cc_measure_brightness_default");
#else            
			s5k5cc_sensor_write_list(s5k5cc_measure_brightness_default, "s5k5cc_measure_brightness_default");
#endif
			break;

		default:
#ifndef PRODUCT_SHIP
			printk("<=PCAM=> unexpected Auto exposure %s/%d\n", __func__, __LINE__);
#endif
			//return -EINVAL;
			return 0;
 	}
	return rc;
}

static long s5k5cc_set_scene_mode(int mode, int scene)
{
	long rc = 0;
	if(scene == 5){
		sceneNight = 1;
	}
	else sceneNight = 0;

        if(scene == 12){
                fireWorks= 1;
        }
        else fireWorks= 0;
    

	switch (scene) {
		case CAMERA_SCENE_AUTO:	//0
			s5k5cc_sensor_write_list(S5K5CCGX_CAM_SCENE_OFF, "S5K5CCGX_CAM_SCENE_OFF");
			break;

		case CAMERA_SCENE_PORTRAIT:	//6
			s5k5cc_sensor_write_list(S5K5CCGX_CAM_SCENE_OFF, "S5K5CCGX_CAM_SCENE_OFF");
			s5k5cc_sensor_write_list(S5K5CCGX_CAM_SCENE_PORTRAIT, "S5K5CCGX_CAM_SCENE_PORTRAIT");
			break;

		case CAMERA_SCENE_LANDSCAPE:	//1	
			s5k5cc_sensor_write_list(S5K5CCGX_CAM_SCENE_OFF, "S5K5CCGX_CAM_SCENE_OFF");
			s5k5cc_sensor_write_list(S5K5CCGX_CAM_SCENE_LANDSCAPE, "S5K5CCGX_CAM_SCENE_LANDSCAPE");
			break;

		case CAMERA_SCENE_NIGHT:	//5
			s5k5cc_sensor_write_list(S5K5CCGX_CAM_SCENE_OFF, "S5K5CCGX_CAM_SCENE_OFF");
			s5k5cc_sensor_write_list(S5K5CCGX_CAM_SCENE_NIGHT, "S5K5CCGX_CAM_SCENE_NIGHT");
			break;

		case CAMERA_SCENE_BEACH:	//3
			s5k5cc_sensor_write_list(S5K5CCGX_CAM_SCENE_OFF, "S5K5CCGX_CAM_SCENE_OFF");
			s5k5cc_sensor_write_list(S5K5CCGX_CAM_SCENE_BEACH, "S5K5CCGX_CAM_SCENE_BEACH");
			break;

		case CAMERA_SCENE_SUNSET:	//4	
			s5k5cc_sensor_write_list(S5K5CCGX_CAM_SCENE_OFF, "S5K5CCGX_CAM_SCENE_OFF");
			s5k5cc_sensor_write_list(S5K5CCGX_CAM_SCENE_SUNSET, "S5K5CCGX_CAM_SCENE_SUNSET");
			break;

		case CAMERA_SCENE_FIREWORKS:	//12
			s5k5cc_sensor_write_list(S5K5CCGX_CAM_SCENE_OFF, "S5K5CCGX_CAM_SCENE_OFF");
			s5k5cc_sensor_write_list(S5K5CCGX_CAM_SCENE_FIRE, "S5K5CCGX_CAM_SCENE_FIRE");
			break;

		case CAMERA_SCENE_SPORTS:	//8
			s5k5cc_sensor_write_list(S5K5CCGX_CAM_SCENE_OFF, "S5K5CCGX_CAM_SCENE_OFF");
			s5k5cc_sensor_write_list(S5K5CCGX_CAM_SCENE_SPORTS, "S5K5CCGX_CAM_SCENE_SPORTS");
			break;

		case CAMERA_SCENE_PARTY:	//13
			s5k5cc_sensor_write_list(S5K5CCGX_CAM_SCENE_OFF, "S5K5CCGX_CAM_SCENE_OFF");
			s5k5cc_sensor_write_list(S5K5CCGX_CAM_SCENE_PARTY, "S5K5CCGX_CAM_SCENE_PARTY");
			break;

		case CAMERA_SCENE_CANDLE:		//11
			s5k5cc_sensor_write_list(S5K5CCGX_CAM_SCENE_OFF, "S5K5CCGX_CAM_SCENE_OFF");
			s5k5cc_sensor_write_list(S5K5CCGX_CAM_SCENE_CANDLE, "S5K5CCGX_CAM_SCENE_CANDLE");
			break;

		case CAMERA_SCENE_AGAINST_LIGHT:	//7
			s5k5cc_sensor_write_list(S5K5CCGX_CAM_SCENE_OFF, "S5K5CCGX_CAM_SCENE_OFF");
			s5k5cc_sensor_write_list(S5K5CCGX_CAM_SCENE_BACKLIGHT, "S5K5CCGX_CAM_SCENE_BACKLIGHT");
			break;

		case CAMERA_SCENE_DAWN:		//17
			s5k5cc_sensor_write_list(S5K5CCGX_CAM_SCENE_OFF, "S5K5CCGX_CAM_SCENE_OFF");
			s5k5cc_sensor_write_list(S5K5CCGX_CAM_SCENE_DAWN, "S5K5CCGX_CAM_SCENE_DAWN");
			break;

		case CAMERA_SCENE_TEXT:		//19
			s5k5cc_sensor_write_list(S5K5CCGX_CAM_SCENE_OFF, "S5K5CCGX_CAM_SCENE_OFF");
			s5k5cc_sensor_write_list(S5K5CCGX_CAM_SCENE_TEXT, "S5K5CCGX_CAM_SCENE_TEXT");
			break;


		default:
#ifndef PRODUCT_SHIP
			printk("<=PCAM=> unexpected scene %s/%d\n", __func__, __LINE__);
#endif
			//return -EINVAL;
			return 0;
 	}
	return rc;
}

static long s5k5cc_set_ISO(int mode, int iso)
{
	long rc = 0;

	switch (iso) {
		case CAMERA_ISOValue_AUTO:
			s5k5cc_sensor_write_list(s5k5cc_iso_auto, "s5k5cc_iso_auto");			
			break;

		case CAMERA_ISOValue_100:
			s5k5cc_sensor_write_list(s5k5cc_iso100, "s5k5cc_iso100");			
			break;

		case CAMERA_ISOValue_200:
			s5k5cc_sensor_write_list(s5k5cc_iso200, "s5k5cc_iso200");			
			break;

		case CAMERA_ISOValue_400:
			s5k5cc_sensor_write_list(s5k5cc_iso400, "s5k5cc_iso400");			
			break;

		default:
#ifndef PRODUCT_SHIP
			printk("<=PCAM=> unexpected ISO value %s/%d\n", __func__, __LINE__);
#endif
			//return -EINVAL;
			return 0;
 	}
	return rc;
}

static int s5k5cc_set_flash(int lux_val)
{
	int i = 0;
	int err = 0;
	int curt = 5;	//current = 100%
	printk("%s, flash set is %d\n", __func__, lux_val);

        if(low_temp == 1 && lux_val == FLASHMODE_FLASH)
            lux_val = MOVIEMODE_FLASH;
        
        if(flash_mode != FLASH_MODE_OFF)
        {
        	if (lux_val == MOVIEMODE_FLASH)
        	{
        		printk("[ASWOOGI] FLASH MOVIE MODE\n");
        		//movie mode
        		gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_CAM_FLASH_EN), 0);		//FLEN : LOW

			for (i = curt; i > 1; i--)	//Data
        		{
        			//gpio on
        			gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_CAM_FLASH_SET), 1);
        			udelay(1);
        			//gpio off
        			gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_CAM_FLASH_SET), 0);
        			udelay(1);
        		}
        			gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_CAM_FLASH_SET), 1);	
        			msleep(2);
        	}
        	else if (lux_val == FLASHMODE_FLASH)
        	{
			//flash mode
        		printk("[ASWOOGI] FLASH ON : %d\n", lux_val);
        		gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_CAM_FLASH_EN), 1);
       		gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_CAM_FLASH_SET), 0);
        	}
		else
        	{
        		printk("[ASWOOGI] FLASH OFF\n");
        		//flash off
        		gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_CAM_FLASH_EN), 0);		
        		gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_CAM_FLASH_SET), 0);
        	}
        }

        return err;
}

static long s5k5cc_set_flash_mode(int mode, int flash)
{
	long rc = 0;
	int temp = 0;

	printk("FLASH MODE : %d \n", flash);

	switch (flash) {
		case FLASH_MODE_OFF:
                        if(flash_mode == FLASH_TORCH_MODE_ON)
                            s5k5cc_set_flash(0);
                        
			flash_mode = flash;
			break;

		case FLASH_MODE_AUTO:
			flash_mode = flash;
			break;

		case FLASH_MODE_ON:
			flash_mode = flash;
			break;

		case FLASH_TORCH_MODE_ON: //for 3rd party flash app
			flash_mode = flash;            
                        s5k5cc_set_flash(MOVIEMODE_FLASH);
			break;

		default:
#ifndef PRODUCT_SHIP
			printk("unexpected Flash value %s/%d\n", __func__, __LINE__);
#endif
			//return -EINVAL;
			return 0;
 	}
	return rc;
}

static void s5k5cc_sensor_reset_af_position(void)
{
    printk("RESET AF POSITION\n");
}

static int s5k5cc_sensor_set_ae_lock(void)
{
    if(vtmode == 3)
        return 0;
    
    printk("SET AE LOCK \n"); 
    AELock = 1;
    s5k5cc_sensor_write(0xFCFC, 0xD000);
    s5k5cc_sensor_write(0x0028, 0x7000);
    s5k5cc_sensor_write(0x002A, 0x2A5A);
    s5k5cc_sensor_write(0x0F12, 0x0000);  //Mon_AAIO_bAE
}

static int s5k5cc_sensor_set_awb_lock(void)
{
    if(vtmode == 3)
        return 0;

    printk("SET AWB LOCK \n"); 
    AWBLock = 1;
    s5k5cc_sensor_write(0xFCFC, 0xD000);
    s5k5cc_sensor_write(0x0028, 0x7000);
    s5k5cc_sensor_write(0x002A, 0x11D6);
    s5k5cc_sensor_write(0x0F12, 0xFFFF);  //awbb_WpFilterMinThr
}


static void s5k5cc_sensor_set_awb_unlock(void)
{
    if(AWBLock == 0)
        return;
    
    printk("SET AWB UNLOCK \n"); 
    AWBLock = 0;
    s5k5cc_sensor_write(0xFCFC, 0xD000);        
    s5k5cc_sensor_write(0x0028, 0x7000); 
    s5k5cc_sensor_write(0x002A, 0x11D6); 
    s5k5cc_sensor_write(0x0F12, 0x001E);  //awbb_WpFilterMinThr
}
static void s5k5cc_sensor_set_ae_unlock(void)
{
    if(AELock == 0)
        return;

    printk("SET AE UNLOCK \n"); 
    AELock = 0;
    s5k5cc_sensor_write(0xFCFC, 0xD000);        
    s5k5cc_sensor_write(0x0028, 0x7000); 
    s5k5cc_sensor_write(0x002A, 0x2A5A); 
    s5k5cc_sensor_write(0x0F12, 0x0001); //MON_AAIO_bAE
}
static int s5k5cc_sensor_get_lux_value(void) 
{
	int msb = 0;
	int lsb = 0;
	int cur_lux = 0;

	s5k5cc_sensor_write(0xFCFC, 0xD000);	
	s5k5cc_sensor_write(0x002C, 0x7000);	
	s5k5cc_sensor_write(0x002E, 0x2A3C);
	s5k5cc_sensor_read(0x0F12, (unsigned short*)&lsb);
	s5k5cc_sensor_read(0x0F12, (unsigned short*)&msb);
	cur_lux = (msb<<16) | lsb;

	printk("[S5K5CC] s5k5cc_sensor_get_lux_value : %x\n", cur_lux);

	return cur_lux;
}


static int s5k5cc_sensor_af_control(int type)
{
    unsigned short read_value;	
    int err, count;
    int ret = 0;
    int size = 0;
    int i = 0;
    unsigned short light = 0;
    int lux = 0;

    printk("[CAM-SENSOR] s5k5cc_sensor_af_control : %d, preview_start : %d\n", type, preview_start); 

    switch (type)
    {
        case AF_MODE_START:                                          // AF start
		printk("AF_MODE_START\n");
		af_status = af_running;

		s5k5cc_sensor_set_ae_lock();		// lock AE
		if(flash_set_on_af == 0)
        		s5k5cc_sensor_set_awb_lock();		// lock AWB
        		
		s5k5cc_sensor_write_list(s5k5cc_af_on,"s5k5cc_af_on");

                if(sceneNight == 1 || fireWorks == 1)
        		msleep(600);                                                // delay 2frames before af status check
                else
	        	msleep(260);                                                // delay 2frames before af status check

                //1st AF search
                for(count=0; count <25; count++)
                {
                    //Check AF Result
                    err=s5k5cc_sensor_write(0xFCFC, 0xD000);
                    err=s5k5cc_sensor_write(0x002C, 0x7000);
                    err=s5k5cc_sensor_write(0x002E, 0x2D12);
                    err=s5k5cc_sensor_read(0x0F12, &read_value);

                    if(err < 0)
                    {
                        printk("CAM 3M 1st AF Status fail \n"); 			
                        return -EIO;
                    }
//                    printk("CAM 3M 1st AF Status Value = %x \n", read_value); 

                    if(read_value == 0x0001)
                    {
                        if(sceneNight == 1)
                            msleep(200);                                                
                        else if(fireWorks == 1)
                            msleep(300);                    
                        else
                            msleep(67);
                        continue;                                       //progress
                    }
                    else
                    {
                        break;
                    }
                }            

		if (read_value == 0x0002) 
		{
			ret = 1;		
			printk("CAM 3M 1st AF_Single Mode SUCCESS. \r\n");
        		msleep(130);                                                // delay 1frames before af status check
        		
                        //2nd AF search
                        for(count=0; count <15; count++)
                        {
                            //Check AF Result
                            err=s5k5cc_sensor_write(0xFCFC, 0xD000);
                            err=s5k5cc_sensor_write(0x002C, 0x7000);
                            err=s5k5cc_sensor_write(0x002E, 0x1F2F);
                            err=s5k5cc_sensor_read(0x0F12, &read_value);

                            if(err < 0)
                            {
                                printk("CAM 3M 2nd AF Status fail \n"); 			
                                return -EIO;
                            }
//                            printk("CAM 3M 2nd AF Status Value = %x \n", read_value); 

                            if(read_value < 256)
                            {
                                printk("CAM 3M 2nd AF Status success = %x \n", read_value);                         
                                break;
                            }
                            else
                            {
                                msleep(67);
                                continue;                                       //progress
                            }
                        }            
		}
		else if(read_value == 0x0004) 
		{
	                ret = 0;
	                printk("CAM 3M AF_Single Mode Fail.==> Cancel \n");
		}
		else 
		{
	                ret = 0;
	                printk("CAM 3M AF_Single Mode Fail.==> FAIL \n");
		}

		if(flash_set_on_af)
		{
			s5k5cc_sensor_write_list(S5K5CC_AE_SPEEDNORMAL,"S5K5CC_AE_SPEEDNORMAL");						
			s5k5cc_sensor_write_list(S5K5CC_Pre_Flash_End_EVT1,"S5K5CC_Pre_Flash_End_EVT1");
                     flash_set_on_af = 0;
		}
        	s5k5cc_set_flash(0);	//flash	
		printk("CAM:3M AF_SINGLE SET \r\n");	
            break;

        case AF_MODE_PREFLASH: //pre flash
		printk("AF_MODE_PREFLASH\n"); 
                preflash_on = 0;

		lux = s5k5cc_sensor_get_lux_value();		//read rux
		
		if ((lux < 0x0020) || (flash_mode == FLASH_MODE_ON))
		{
		    if(flash_mode != FLASH_MODE_OFF)
		    {
                        preflash_on = 1;
                        flash_set_on_af = 1;
                        s5k5cc_sensor_write_list(S5K5CC_AE_SPEEDUP,"S5K5CC_AE_SPEEDUP");						
                        s5k5cc_sensor_write_list(S5K5CC_Pre_Flash_Start_EVT1,"S5K5CC_Pre_Flash_Start_EVT1");						
                        s5k5cc_set_flash(MOVIEMODE_FLASH);	//flash	

                        for(count=0; count <40; count++)
                        {
                            //Check AE stable check
                            err=s5k5cc_sensor_write(0xFCFC, 0xD000);
                            err=s5k5cc_sensor_write(0x002C, 0x7000);
                            err=s5k5cc_sensor_write(0x002E, 0x1E3C);
                            err=s5k5cc_sensor_read(0x0F12, &read_value);

                            if(err < 0)
                            {
                                printk("CAM 3M AE Status fail \n"); 			
                                return -EIO;
                            }
//                            printk("CAM 3M AE Status Value = %x \n", read_value); 

                            if(read_value == 0x0001) //AE stable
                            {
                                break;
                            }
                            else
                            {
                                msleep(10);
                                continue;                                       //progress
                            }
                        }
		    }
		}
            break;


        case AF_MODE_NORMAL: // release //infinity
		printk("[CAM-SENSOR] Focus Mode -> release\n"); 
		af_status = af_stop;

#if defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || defined(CONFIG_MACH_PREVAIL2)
                if(af_mode == 1)
        		s5k5cc_sensor_write_list(s5k5cc_af_macro_off,"s5k5cc_af_macro_off");
                else
#endif
		s5k5cc_sensor_write_list(s5k5cc_af_off,"s5k5cc_af_off");
		// AW & AWB UNLOCK
		s5k5cc_sensor_set_ae_unlock();
		s5k5cc_sensor_set_awb_unlock();        
            break;

        case AF_MODE_INFINITY: // infinity
		if(af_status == af_running || preview_start == 0)
			break;
#ifdef I2C_BURST_MODE 
        	s5k5cc_sensor_burst_write_list(s5k5cc_af_infinity,"s5k5cc_af_infinity");
#else            
		s5k5cc_sensor_write_list(s5k5cc_af_infinity,"s5k5cc_af_infinity");
#endif
            break;

        case AF_MODE_MACRO: // macro
		if(af_status == af_running|| preview_start == 0)
			break;
                af_mode = 1;
#ifdef I2C_BURST_MODE 
        	s5k5cc_sensor_burst_write_list(s5k5cc_af_macro,"s5k5cc_af_macro");
#else            
		s5k5cc_sensor_write_list(s5k5cc_af_macro,"s5k5cc_af_macro");						
#endif
            break;
        
        
        case AF_MODE_AUTO: // auto
		if(af_status == af_running || preview_start == 0)
			break;
                af_mode = 0;        
#ifdef I2C_BURST_MODE 
        	s5k5cc_sensor_burst_write_list(s5k5cc_af_auto,"s5k5cc_af_auto");
#else            
		s5k5cc_sensor_write_list(s5k5cc_af_auto,"s5k5cc_af_auto");
#endif
            break;
                    
        default:
            break;
    }

    return ret;
}


static long s5k5cc_set_sensor_mode(int mode)
{
	int lux = 0;
        int cnt, vsync_value;

//	printk("[PGH] Sensor Mode \n");
	switch (mode) {
	case SENSOR_PREVIEW_MODE:
                printk("[PGH] Preview \n");
                af_status = af_stop;  
                preview_start = 1;
                low_temp = 0;
                
//                if(sceneNight == 1 || fireWorks == 1)
//                {
        		for(cnt=0; cnt<200; cnt++)
        		{
        			vsync_value = gpio_get_value(14);

        			if(vsync_value)
                                {         
        				printk(" on preview,  end cnt:%d vsync_value:%d\n", cnt, vsync_value);			                        
        				break;
                                }
        			else
        			{
//        				printk(" on preview,  wait cnt:%d vsync_value:%d\n", cnt, vsync_value);			
        				msleep(1);
        			}
        	
        		}
//                }
                
#ifdef I2C_BURST_MODE
		s5k5cc_sensor_burst_write_list(s5k5cc_preview,"s5k5cc_preview"); // preview start
#else            
		s5k5cc_sensor_write_list(s5k5cc_preview,"s5k5cc_preview"); // preview start
#endif
		mdelay(100);		
		break;

	case SENSOR_SNAPSHOT_MODE:
		printk("[PGH] Capture \n");		

		lux = s5k5cc_sensor_get_lux_value();		//read rux

		if (((lux < 0x0020) || (flash_mode == FLASH_MODE_ON) || preflash_on == 1) && (flash_mode != FLASH_MODE_OFF))//flash on
		{
                    preflash_on = 0;
                    flash_set_on_snap = 1;
                    s5k5cc_sensor_write_list(S5K5CC_Flash_Start_EVT1,"S5K5CC_Flash_Start_EVT1");
#if defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || defined(CONFIG_MACH_PREVAIL2)                        
                    if(af_mode == 1)
                        s5k5cc_set_flash(MOVIEMODE_FLASH);
                    else
#endif                            
                    s5k5cc_set_flash(FLASHMODE_FLASH);
                    exif_flash_status = 1;
		}
		else
		{
                        exif_flash_status = 0;		
			s5k5cc_sensor_set_ae_lock();		// lock AE
		}

		for(cnt=0; cnt<700; cnt++)
		{
			vsync_value = gpio_get_value(14);

			if(vsync_value)
                        {         
				printk(" on snapshot,  end cnt:%d vsync_value:%d\n", cnt, vsync_value);			                        
				break;
                        }
			else
			{
//				printk(" on snapshot,  wait cnt:%d vsync_value:%d\n", cnt, vsync_value);			
				msleep(3);
			}
	
		}

        if (lux > 0xFFFE) 		// highlight snapshot
        {
//                printk("Snapshot : highlight snapshot\n");
                    
#ifdef I2C_BURST_MODE 
                s5k5cc_sensor_burst_write_list(s5k5cc_snapshot_normal_high,"s5k5cc_snapshot_normal_high");
	        s5k5cc_sensor_set_ae_unlock();
		s5k5cc_sensor_set_awb_unlock();                    
#else            
                s5k5cc_sensor_write_list(s5k5cc_snapshot_normal_high,"s5k5cc_snapshot_normal_high");
	        s5k5cc_sensor_set_ae_unlock();
		s5k5cc_sensor_set_awb_unlock();                    
#endif
        }
        else if (lux > 0x0020) 	// Normal snapshot
        {	
//                printk("Snapshot : Normal snapshot\n");
                    
#ifdef I2C_BURST_MODE 
               s5k5cc_sensor_burst_write_list(s5k5cc_capture_normal_normal,"s5k5cc_capture_normal_normal");
	        s5k5cc_sensor_set_ae_unlock();
		s5k5cc_sensor_set_awb_unlock();                    
#else            
                s5k5cc_sensor_write_list(s5k5cc_capture_normal_normal,"s5k5cc_capture_normal_normal");
                s5k5cc_sensor_set_ae_unlock();
		s5k5cc_sensor_set_awb_unlock();                        
#endif
        }                
        else    //lowlight snapshot
        {
//                printk("Snapshot : lowlight snapshot\n");
#ifdef I2C_BURST_MODE
                if(sceneNight == 1 || fireWorks == 1)
                {
                        s5k5cc_sensor_burst_write_list(s5k5cc_snapshot_nightmode,"s5k5cc_snapshot_nightmode");
                }
                else
                {
                        s5k5cc_sensor_burst_write_list(s5k5cc_snapshot_normal_low,"s5k5cc_snapshot_normal_low");
                }

                s5k5cc_sensor_set_ae_unlock();      // aw unlock
		s5k5cc_sensor_set_awb_unlock();                        
#else            
                if(sceneNight == 1 || fireWorks == 1)
                {
                        s5k5cc_sensor_burst_write_list(s5k5cc_snapshot_nightmode,"s5k5cc_snapshot_nightmode");
                }
                else
                {
                        s5k5cc_sensor_write_list(s5k5cc_snapshot_normal_low,"s5k5cc_snapshot_normal_low");
                }
                s5k5cc_sensor_set_ae_unlock();      // aw unlock
		s5k5cc_sensor_set_awb_unlock();                                
#endif
                }              

		break;

	case SENSOR_RAW_SNAPSHOT_MODE:
		printk("[PGH}-> Capture RAW \n");		
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

int s5k5cc_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
	int rc = 0;

	unsigned short	id = 0; //PGH FOR TEST

	printk("%s/%d \n", __func__, __LINE__);
    
	s5k5cc_sensor_write(0x002C, 0x7000);
	s5k5cc_sensor_write(0x002E, 0x0152);
	s5k5cc_sensor_read(0x0F12, &id);
	printk("[PGH] SENSOR FW => id 0x%04x \n", id);

	s5k5cc_sensor_write(0x002C, 0xD000);
	s5k5cc_sensor_write(0x002E, 0x0040);
	s5k5cc_sensor_read(0x0F12, &id);
	printk("[PGH] SENSOR FW => id 0x%04x \n", id);

#ifdef I2C_BURST_MODE //dha23 100325
	s5k5cc_sensor_burst_write_list(s5k5cc_init0,"s5k5cc_init0");
#else            
	s5k5cc_sensor_write_list(s5k5cc_init0,"s5k5cc_init0");
#endif
	msleep(10);	

#ifdef I2C_BURST_MODE //dha23 100325	
	s5k5cc_sensor_burst_write_list(s5k5cc_init1,"s5k5cc_init1");
#else            
	s5k5cc_sensor_write_list(s5k5cc_init1,"s5k5cc_init1");
#endif

	return rc;

}

#ifdef CONFIG_MACH_CHIEF
static int cam_hw_init()
{
	int rc = 0;
       	printk("cam_hw_init : hw rev %d \n", system_rev);
        
        if(system_rev>=6)
        {
        	printk("<=PCAM=> ++++++++++++++++++++++++++test driver++++++++++++++++++++++++++++++++++++ \n");

        	gpio_tlmm_config(GPIO_CFG(CAM_RESET, 0,GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE); //CAM_RESET
        	gpio_tlmm_config(GPIO_CFG(CAM_STANDBY, 0,GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE); //CAM_STANDBY
        	gpio_tlmm_config(GPIO_CFG(CAM_EN, 0,GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE); //CAM_EN
        	gpio_tlmm_config(GPIO_CFG(CAM_VT_RST, 0,GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE); //CAM_VT_RST
        	gpio_tlmm_config(GPIO_CFG(CAM_VT_nSTBY, 0,GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE); //CAM_VT_nSTBY
        	
        	struct vreg* vreg_L8;
        	vreg_L8 = vreg_get(NULL, "gp7"); //vga power
                if(system_rev>=8) //SF sensor
                {
                	vreg_set_level(vreg_L8, 1800);
                }
                else //LSI sensor
                {
                	vreg_set_level(vreg_L8, 1500);
                }
        	struct vreg* vreg_L16;
        	vreg_L16 = vreg_get(NULL, "gp10"); //af power
        	vreg_set_level(vreg_L16, 3000);
        	
        	gpio_set_value(CAM_EN, 1); //EN -> UP
               	vreg_enable(vreg_L8);                
        	vreg_enable(vreg_L16);
        	mdelay(1);

        	gpio_set_value(CAM_VT_nSTBY, 1); //VT_nSTBY -> UP
        	mdelay(1);

        	/* Input MCLK = 24MHz */
        	gpio_tlmm_config(GPIO_CFG(CAM_MCLK, 1,GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE); //CAM_MCLK            
        	msm_camio_clk_rate_set(24000000);	//MCLK
        	msm_camio_camif_pad_reg_reset();
        	mdelay(1);

        	gpio_set_value(CAM_VT_RST, 1); //VT_RST -> UP
        	mdelay(1);

        	gpio_set_value(CAM_VT_nSTBY, 0); //VT_nSTBY -> DOWN
        	mdelay(15);

        	gpio_set_value(CAM_STANDBY, 1); //STBY -> UP
        	mdelay(1);

        	gpio_set_value(CAM_RESET, 1); //REST -> UP
        	mdelay(10);

        }
        else
        {
        	struct vreg* vreg_L16;
        	vreg_L16 = vreg_get(NULL, "gp10"); 
        	vreg_set_level(vreg_L16, 3000);
        	vreg_enable(vreg_L16);
        	gpio_set_value(CAM_EN, 1); //STBY -> UP

        	/* Input MCLK = 24MHz */
        	msm_camio_clk_rate_set(24000000);
        	msm_camio_camif_pad_reg_reset();
        	udelay(1);
        	gpio_tlmm_config(GPIO_CFG(CAM_MCLK, 1,GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA), GPIO_CFG_ENABLE); //CAM_MCLK : yjlee

        	gpio_set_value(CAM_STANDBY, 1); //STBY -> UP
        	gpio_set_value_cansleep(CAM_RESET, 1); //RESET UP

        	msleep(30);
        }
            
	return rc;

}
#else
static int cam_hw_init()
{
	int rc = 0;
       	printk("cam_hw_init : hw rev %d \n", system_rev);
    
	struct vreg* vreg_L16;
	struct vreg* vreg_L8;    

	printk("<=PCAM=> ++++++++++++++++++++++++++test driver++++++++++++++++++++++++++++++++++++ \n");

	gpio_tlmm_config(GPIO_CFG(CAM_RESET, 0,GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE); //CAM_RESET
	gpio_tlmm_config(GPIO_CFG(CAM_STANDBY, 0,GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE); //CAM_STANDBY
	gpio_tlmm_config(GPIO_CFG(CAM_EN, 0,GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE); //CAM_EN
        if(system_rev>=6)
        {
        	gpio_tlmm_config(GPIO_CFG(CAM_EN_2, 0,GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE); //CAM_EN_2	
        }
	gpio_tlmm_config(GPIO_CFG(CAM_VT_RST, 0,GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE); //CAM_VT_RST
	gpio_tlmm_config(GPIO_CFG(CAM_VT_nSTBY, 0,GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE); //CAM_VT_nSTBY

	vreg_L16 = vreg_get(NULL, "gp10"); 
	vreg_set_level(vreg_L16, 3000);
	vreg_L8 = vreg_get(NULL, "gp7");
	vreg_set_level(vreg_L8, 1800);

	gpio_set_value(CAM_EN, 1); //EN -> UP
       	udelay(80);	
	vreg_enable(vreg_L8);
	
        if(system_rev>=6)
        {
        	udelay(150);
        	gpio_set_value(CAM_EN_2, 1); //EN_2 -> UP            
        }
        
	vreg_enable(vreg_L16);
	mdelay(1);

	gpio_set_value(CAM_VT_nSTBY, 1); //VT_nSTBY -> UP
	mdelay(1);

	/* Input MCLK = 24MHz */
       	gpio_tlmm_config(GPIO_CFG(CAM_MCLK, 1,GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE); //CAM_MCLK                
	msm_camio_clk_rate_set(24000000);	//MCLK
	msm_camio_camif_pad_reg_reset();
	mdelay(1);

	gpio_set_value(CAM_VT_RST, 1); //VT_RST -> UP
	mdelay(5);

	gpio_set_value(CAM_VT_nSTBY, 0); //VT_nSTBY -> DOWN
	mdelay(10);

	gpio_set_value(CAM_STANDBY, 1); //STBY -> UP
	mdelay(1);

	gpio_set_value(CAM_RESET, 1); //REST -> UP
	mdelay(55);

	return rc;

}
#endif

#ifdef CONFIG_LOAD_FILE

#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include <asm/uaccess.h>

static char *s5k5cc_regs_table = NULL;

static int s5k5cc_regs_table_size;

int s5k5cc_regs_table_init(void)
{
	struct file *filp;
	char *dp;
	long l;
	loff_t pos;
	int ret;
	mm_segment_t fs = get_fs();

	printk("%s %d\n", __func__, __LINE__);

	set_fs(get_ds());
#if 0
	//filp = filp_open("/data/camera/s5k5cc.h", O_RDONLY, 0);
	filp = filp_open("/data/s5k5cc.h", O_RDONLY, 0);
#else
	filp = filp_open("/mnt/sdcard/s5k5cc.h", O_RDONLY, 0);
#endif
	if (IS_ERR(filp)) {
		printk("file open error %d\n", PTR_ERR(filp));
		return -1;
	}
	l = filp->f_path.dentry->d_inode->i_size;	
	printk("l = %ld\n", l);
	dp = kmalloc(l, GFP_KERNEL);
	if (dp == NULL) {
		printk("Out of Memory\n");
		filp_close(filp, current->files);
	}
	pos = 0;
	memset(dp, 0, l);
	ret = vfs_read(filp, (char __user *)dp, l, &pos);
	if (ret != l) {
		printk("Failed to read file ret = %d\n", ret);
		kfree(dp);
		filp_close(filp, current->files);
		return -1;
	}

	filp_close(filp, current->files);
	
	set_fs(fs);

	s5k5cc_regs_table = dp;
	
	s5k5cc_regs_table_size = l;

	*((s5k5cc_regs_table + s5k5cc_regs_table_size) - 1) = '\0';

//	printk("s5k5cc_regs_table 0x%04x, %ld\n", dp, l);
	return 0;
}

void s5k5cc_regs_table_exit(void)
{
	printk("%s %d\n", __func__, __LINE__);
	if (s5k5cc_regs_table) {
		kfree(s5k5cc_regs_table);
		s5k5cc_regs_table = NULL;
	}	
}

static int s5k5cc_regs_table_write(char *name)
{
	char *start, *end, *reg;	
	unsigned short addr, value;
	char reg_buf[7], data_buf[7];

	*(reg_buf + 6) = '\0';
	*(data_buf + 6) = '\0';

	start = strstr(s5k5cc_regs_table, name);
	
	end = strstr(start, "};");

	while (1) {	
		/* Find Address */	
		reg = strstr(start,"{ 0x");		
		if (reg)
			start = (reg + 16);
		if ((reg == NULL) || (reg > end))
			break;
		/* Write Value to Address */	
		if (reg != NULL) {
			memcpy(reg_buf, (reg + 2), 6);	
			memcpy(data_buf, (reg + 10), 6);	
			addr = (unsigned short)simple_strtoul(reg_buf, NULL, 16); 
			value = (unsigned short)simple_strtoul(data_buf, NULL, 16); 
//			printk("addr 0x%04x, value 0x%04x\n", addr, value);

			if (addr == 0xdddd)
			{
		    	msleep(value);
				printk("delay 0x%04x, value 0x%04x\n", addr, value);
			}	
			else if (addr == 0xffff){
					}
			else
				s5k5cc_sensor_write(addr, value);
		}
	}

	return 0;
}

#endif



int s5k5cc_sensor_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;

	printk("[PGH]  %s/%d \n", __func__, __LINE__);	

#ifdef CONFIG_LOAD_FILE
	if(0 > s5k5cc_regs_table_init()) {
		CDBG("s5k5cc_sensor_init file open failed!\n");
		rc = -1;
		goto init_fail;
	}
#endif

	s5k5cc_ctrl = kzalloc(sizeof(struct s5k5cc_ctrl), GFP_KERNEL);
	if (!s5k5cc_ctrl) {
		CDBG("s5k5cc_init alloc mem failed!\n");
		rc = -ENOMEM;
		goto init_done;
	}

	if (data)
		s5k5cc_ctrl->sensordata = data;

	rc = cam_hw_init();
	if (rc < 0) 
	{
		printk("<=PCAM=> cam_hw_init failed!\n");
		goto init_fail;
	}

	rc = s5k5cc_sensor_init_probe(data);
	if (rc < 0) {
		CDBG("s5k5cc_sensor_init init probe failed!\n");
		goto init_fail;
	}

init_done:
	return rc;

init_fail:
	kfree(s5k5cc_ctrl);
	return rc;
}

int s5k5cc_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&s5k5cc_wait_queue);
	return 0;
}

int s5k5cc_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cfg_data;
	long   rc = 0;

	if (copy_from_user(&cfg_data,
			(void *)argp,
			sizeof(struct sensor_cfg_data)))
		return -EFAULT;

	/* down(&s5k5cc_sem); */

	printk("s5k5cc_ioctl, cfgtype = %d, mode = %d\n", 
		cfg_data.cfgtype, cfg_data.mode);

		switch (cfg_data.cfgtype) {
		case CFG_SET_MODE:
			rc = s5k5cc_set_sensor_mode(
						cfg_data.mode);
			break;

		case CFG_SET_EFFECT:
			rc = s5k5cc_set_effect(cfg_data.mode,
						cfg_data.cfg.effect);
			break;

		case CFG_SET_BRIGHTNESS:
			rc = s5k5cc_set_brightness(cfg_data.mode,
						cfg_data.cfg.brightness);
			break;

		case CFG_SET_WB:
			rc = s5k5cc_set_whitebalance(cfg_data.mode,
						cfg_data.cfg.wb);
			break;

		case CFG_SET_ISO:
			rc = s5k5cc_set_ISO(cfg_data.mode,
						cfg_data.cfg.iso);
			break;

		case CFG_SET_EXPOSURE_MODE:
			rc = s5k5cc_set_auto_exposure(cfg_data.mode,
						cfg_data.cfg.metering);
			break;

		case CFG_SET_EXPOSURE_VALUE:
			rc = s5k5cc_set_exposure_value(cfg_data.mode,
						cfg_data.cfg.ev);
			break;
		
		case CFG_SET_FLASH:
			rc = s5k5cc_set_flash_mode(cfg_data.mode,
						cfg_data.cfg.flash);
			break;

       	case CFG_MOVE_FOCUS:
        		rc = s5k5cc_sensor_af_control(cfg_data.cfg.focus.steps);
                     if(cfg_data.cfg.focus.steps == AF_MODE_START)
                     {
                           cfg_data.rs = rc;
                     	if (copy_to_user((void *)argp,&cfg_data,sizeof(struct sensor_cfg_data)))
                      	    return -EFAULT;       
                      }
        		break;

       	case CFG_SET_DEFAULT_FOCUS:
			rc = s5k5cc_sensor_af_control(cfg_data.cfg.focus.steps);
        		break;

       	case CFG_GET_AF_MAX_STEPS:
//        		cfg_data.max_steps = MT9T013_TOTAL_STEPS_NEAR_TO_FAR;
        		if (copy_to_user((void *)argp,	&cfg_data,sizeof(struct sensor_cfg_data)))
        		    rc = -EFAULT;
        		break;
        	
		case CFG_SET_SCENE_MODE:
			rc = s5k5cc_set_scene_mode(cfg_data.mode,cfg_data.cfg.scene);
			break;

		case CFG_SET_DATALINE_CHECK:
                    if(cfg_data.cfg.dataline)
                    {         
                        printk("[ASWOOGI] CFG_SET_DATALINE_CHECK ON\n");	                        
                        s5k5cc_sensor_write(0xFCFC, 0xD000);
                        s5k5cc_sensor_write(0x0028, 0xD000);
                        s5k5cc_sensor_write(0x002A, 0xB054);
                        s5k5cc_sensor_write(0x0F12, 0x0001);
                    }
                    else
                    {         
                        printk("[ASWOOGI] CFG_SET_DATALINE_CHECK OFF \n");	                                                
                        s5k5cc_sensor_write(0xFCFC, 0xD000);
                        s5k5cc_sensor_write(0x0028, 0xD000);
                        s5k5cc_sensor_write(0x002A, 0xB054);
                        s5k5cc_sensor_write(0x0F12, 0x0000);
                    }                            
			break;

		default:
			printk("[PGH] undefined cfgtype %s/%d \n", __func__, __LINE__, cfg_data.cfgtype);	
//			rc = -EINVAL;
                        rc = 0;
			break;
		}

	/* up(&s5k5cc_sem); */

	return rc;
}

int s5k5cc_sensor_release(void)
{
	int rc = 0;
        preview_start = 0;

        s5k5cc_set_flash(0); //baiksh temp fix
	/* down(&s5k5cc_sem); */

	gpio_set_value(CAM_RESET, 0); //REST -> DOWN
	mdelay(1);

	gpio_set_value(CAM_VT_RST, 0); //REST -> DOWN
	mdelay(1);

	gpio_set_value(CAM_STANDBY, 0); //STBY -> DOWN
	mdelay(1);

	gpio_set_value(CAM_VT_nSTBY, 0); //STBY -> DOWN
	mdelay(1);
    
	vreg_disable(vreg_get(NULL, "gp10"));
#ifdef CONFIG_MACH_CHIEF
	vreg_disable(vreg_get(NULL, "gp7"));    
#endif
	mdelay(1);
#if defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || defined(CONFIG_MACH_PREVAIL2)
        if(system_rev>=6)
        {
        	gpio_set_value(CAM_EN_2, 0);
        	udelay(20);    
        }
	vreg_disable(vreg_get(NULL, "gp7"));        
#endif        
	gpio_set_value(CAM_EN, 0);
	mdelay(1);

	kfree(s5k5cc_ctrl);
	/* up(&s5k5cc_sem); */

	return rc;
}

int s5k5cc_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	printk("[PGH] %s/%d \n", __func__, __LINE__);	

	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		goto probe_failure;
	}

	s5k5cc_sensorw =
		kzalloc(sizeof(struct s5k5cc_work), GFP_KERNEL);

	if (!s5k5cc_sensorw) {
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, s5k5cc_sensorw);
	s5k5cc_init_client(client);
	s5k5cc_client = client;
	printk("[PGH] s5k5cc_probe succeeded!  %s/%d \n", __func__, __LINE__);	
	CDBG("s5k5cc_probe succeeded!\n");
	return 0;

probe_failure:
	kfree(s5k5cc_sensorw);
	s5k5cc_sensorw = NULL;
	CDBG("s5k5cc_probe failed!\n");
	return rc;
}

static const struct i2c_device_id s5k5cc_i2c_id[] = {
	{ "s5k5ccgx", 0},
	{ },
};

static struct i2c_driver s5k5cc_i2c_driver = {
	.id_table = s5k5cc_i2c_id,
	.probe  = s5k5cc_i2c_probe,
	.remove = __exit_p(s5k5cc_i2c_remove),
	.driver = {
	.name = "s5k5ccgx",
	},
};

#if defined(CONFIG_MACH_CHIEF) || defined(CONFIG_MACH_VITAL2) || defined (CONFIG_MACH_ROOKIE2) || defined(CONFIG_MACH_PREVAIL2)
/* for sysfs control (/sys/class/sec/flash/flash_switch) */
/* turn on : echo 1 > /sys/class/sec/flash/flash_switch */
/* turn off : echo 0 > /sys/class/sec/flash/flash_switch */

static ssize_t s5k5cc_flash_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	if (strncmp(buf, "0", 1) == 0)
	{
	    printk("[PGH] turn off s5k5cc flash.\n");
	    flash_mode = 1;        
	    s5k5cc_set_flash(0);
	}
	else if (strncmp(buf, "1", 1) == 0)
	{
	    printk("[PGH] turn on s5k5cc flash.\n");
	    //s5k5cc_set_flash(FLASHMODE_FLASH);
	    flash_mode = 1;
	    s5k5cc_set_flash(MOVIEMODE_FLASH);
	}

	return size;
}


static DEVICE_ATTR(flash_switch, S_IRUGO | S_IWUSR, NULL, s5k5cc_flash_store);
#endif

int s5k5cc_sensor_probe(const struct msm_camera_sensor_info *info,
				struct msm_sensor_ctrl *s)
{
	printk("[PGH]  %s/%d \n", __func__, __LINE__);	
	int rc = i2c_add_driver(&s5k5cc_i2c_driver);
	if (rc < 0 || s5k5cc_client == NULL) {
		rc = -ENOTSUPP;
		goto probe_done;
	}
	printk("[PGH] %s/%d \n", __func__, __LINE__);	

	printk("[PGH] s5k5cc_client->addr : %x\n", s5k5cc_client->addr);
	printk("[PGH] s5k5cc_client->adapter->nr : %d\n", s5k5cc_client->adapter->nr);


#if defined(CONFIG_MACH_CHIEF) || defined(CONFIG_MACH_VITAL2) || defined (CONFIG_MACH_ROOKIE2) || defined(CONFIG_MACH_PREVAIL2)
	flash_dev = device_create(sec_class, NULL, 0, NULL, "flash");

	if (IS_ERR(flash_dev))
		pr_err("Failed to create device(flash)!\n");
	if (device_create_file(flash_dev, &dev_attr_flash_switch) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_flash_switch.attr.name);

#endif

	s->s_init = s5k5cc_sensor_init;
	s->s_release = s5k5cc_sensor_release;
	s->s_config  = s5k5cc_sensor_config;
	s->s_camera_type = BACK_CAMERA_2D;        
    	s->s_mount_angle = 0;

probe_done:
	CDBG("%s %s:%d\n", __FILE__, __func__, __LINE__);
	return rc;
}

int __s5k5cc_probe(struct platform_device *pdev)
{
	printk("[PGH]  %s/%d \n", __func__, __LINE__);	
	return msm_camera_drv_start(pdev, s5k5cc_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __s5k5cc_probe,
	.driver = {
		.name = "msm_camera_s5k5ccgx",
		.owner = THIS_MODULE,
	},
};

int __init s5k5cc_init(void)
{
	printk("[PGH]  %s/%d \n", __func__, __LINE__);
	return platform_driver_register(&msm_camera_driver);
}

module_init(s5k5cc_init);
