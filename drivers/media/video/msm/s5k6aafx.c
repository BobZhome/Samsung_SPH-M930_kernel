/*
  SEC S5K6AAFX
 */
/***************************************************************
CAMERA DRIVER FOR 5M CAM(FUJITSU M4Mo) by PGH
ver 0.1 : only preview (base on universal)
****************************************************************/

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include "s5k6aafx.h"

#include <mach/vreg.h>//bestiq
#include <mach/camera.h>//bestiq
#ifdef CONFIG_KEYBOARD_ADP5587
#include <linux/i2c/adp5587.h>
#endif

//#include <asm/gpio.h> //PGH
//#include <linux/clk.h>
//#include <linux/io.h>
//#include <mach/board.h>

#define S5K6AAFX_DEBUG
#ifdef S5K6AAFX_DEBUG
#define CAM_DEBUG(fmt, arg...)	\
		do{\
		printk("\033[[s5k6aafx] %s:%d: " fmt "\033[0m\n", __FUNCTION__, __LINE__, ##arg);}\
		while(0)
#else
#define CAM_DEBUG(fmt, arg...)	
#endif

////////////////////////////////////////
#if 1
#ifdef CONFIG_MACH_CHIEF
#define CAM_RESET ((system_rev>=6)?161:75)
#define CAM_STANDBY ((system_rev>=6)?132:74)
#else //vital2
#define CAM_RESET 75
#define CAM_STANDBY 74
#endif
#define CAM_EN 3
#define CAM_I2C_SCL 176
#define CAM_I2C_SDA 174
#define CAM_VT_nSTBY 2		//yjlee : add
#define CAM_VT_RST 175		//yjlee : add
#define CAM_MCLK 15			//yjlee : add
#endif


#define CAMERA_MODE			0
#define CAMCORDER_MODE		1


/*	Read setting file from SDcard
	- There must be no "0x" string in comment. If there is, it cause some problem.
*/

struct s5k6aafx_work_t {
	struct work_struct work;
};

static struct  s5k6aafx_work_t *s5k6aafx_sensorw;
static struct  i2c_client *s5k6aafx_client;

struct s5k6aafx_ctrl_t {
	int8_t  opened;
	struct  msm_camera_sensor_info 	*sensordata;
	int dtp_mode;
	int app_mode;	// camera or camcorder
};

static struct s5k6aafx_ctrl_t *s5k6aafx_ctrl;
static DECLARE_WAIT_QUEUE_HEAD(s5k6aafx_wait_queue);
DECLARE_MUTEX(s5k6aafx_sem);

static int s5k6aafx_preview_start = 0;

static int s5k6aafx_hw_init();


//i2c_11111///////////////////////////////////////////////////////////////////////////////////////

static inline int s5k6aafx_read(struct i2c_client *client, 
	unsigned short subaddr, unsigned short *data)
{
	unsigned char buf[2];
	int err = 0;
	struct i2c_msg msg = {client->addr, 0, 2, buf};

	*(unsigned short *)buf = cpu_to_be16(subaddr);

//	printk("\n\n\n%X %X\n\n\n", buf[0], buf[1]);

	err = i2c_transfer(client->adapter, &msg, 1);
	if (unlikely(err < 0))
		printk("%s: %d register read fail\n", __func__, __LINE__);	

	msg.flags = I2C_M_RD;

	err = i2c_transfer(client->adapter, &msg, 1);
	if (unlikely(err < 0))
		printk("%s: %d register read fail\n", __func__, __LINE__);	

//	printk("\n\n\n%X %X\n\n\n", buf[0], buf[1]);

	*data = ((buf[0] << 8) | buf[1]);
		
	return err;
}

static inline int s5k6aafx_write(struct i2c_client *client,
		unsigned long packet)
{
	unsigned char buf[4];

	int err = 0;
	int retry_count = 5;

	struct i2c_msg msg = 
	{
		.addr	= client->addr,
		.flags	= 0,
		.buf	= buf,
		.len	= 4,
	};

//	printk("%s: %d Start 1 \n", __func__, __LINE__);	

	if (!client->adapter)
	{
	  dev_err(&client->dev, "%s: can't search i2c client adapter\n", __func__);
	  return -EIO;
	}

	while(retry_count--)
	{		
		*(unsigned long *)buf = cpu_to_be32(packet);
		err = i2c_transfer(client->adapter, &msg, 1);
		if (likely(err == 1)) {
//			printk("%s: %d Start 2 \n", __func__, __LINE__);	
			break;
		}
		mdelay(10);
//	    printk("%s: %d Start 3 \n", __func__, __LINE__);	
	}

	if (unlikely(err < 0)) 
	{
		dev_err(&client->dev, "%s: 0x%08x write failed\n", __func__, (unsigned int)packet);
		return err;
	}
	
//	printk("%s: %d Start 4 %d \n", __func__, __LINE__,err);	
	return (err != 1) ? -1 : 0;
}

/* program multiple registers */
static int s5k6aafx_write_regs(
		unsigned long *packet, unsigned int num)
{
	struct i2c_client *client = s5k6aafx_client;
	int ret = -EAGAIN;	/* FIXME */
	unsigned long temp;
	char delay = 0;

	while (num--) 
	{
		temp = *packet++;
		
/*
		if ((temp & S5K6AAFX_DELAY) == S5K6AAFX_DELAY) 
		{
			if (temp & 0x1) 
			{
				dev_info(&client->dev, "delay for 100msec\n");
				msleep(100);
				continue;
			} 
			else 
			{
				dev_info(&client->dev, "delay for 10msec\n");
				msleep(10);
				continue;
			}	
		}
*/

#if 1
		if ((temp & S5K6AAFX_DELAY) == S5K6AAFX_DELAY) 
		{                                                    
			delay = temp & 0xFFFF;                                                                              
			//printk("func(%s):line(%d):delay(0x%x):delay(%d)\n",__func__,__LINE__,delay,delay);       
			msleep(delay);                                                                                      
			continue;                                                                                           
		}
#endif
		ret = s5k6aafx_write(client, temp);

		/* In error circumstances */
		/* Give second shot */
		if (unlikely(ret)) 
		{
			dev_info(&client->dev,
				"s5k6aafx i2c retry one more time\n");
			ret = s5k6aafx_write(client, temp);

			/* Give it one more shot */
			if (unlikely(ret)) 
			{
				dev_info(&client->dev,
					"s5k6aafx i2c retry twice\n");
				ret = s5k6aafx_write(client, temp);
				break;
			}
		}
	}

	dev_info(&client->dev, "S5K6AAFX register programming ends up\n");
	if( ret < 0)
		return -EIO;	
	
	return ret;	/* FIXME */
}

static int s5k6aafx_set_capture_start()
{
	struct i2c_client *client = s5k6aafx_client;
	
	int err = -EINVAL;
	unsigned short lvalue = 0;

	s5k6aafx_write(client, 0x002C7000);
	s5k6aafx_write(client, 0x002E1AAA); //read light value
	
	s5k6aafx_read(client, 0x0F12, &lvalue);

	printk("%s : light value is %x\n", __func__, lvalue);

	/* set initial regster value */
	err = s5k6aafx_write_regs(s5k6aafx_capture,
		sizeof(s5k6aafx_capture) / sizeof(s5k6aafx_capture[0]));
	if (lvalue < 0x40)
	{
		printk("\n----- low light -----\n\n");
		mdelay(100);//add 100 ms delay for capture sequence
	}
	else
	{
		printk("\n----- normal light -----\n\n");
		mdelay(50);
	}

	if (unlikely(err)) 
	{
		printk("%s: failed to make capture\n", __func__);
		return err;
	}

	return err;
}

static int s5k6aafx_set_preview_start()
{
//	struct i2c_client *client = s5k6aafx_client;

	int err = -EINVAL;

	printk("%s: set_Preview_start\n", __func__);

#if 0
	if(state->check_dataline)		// Output Test Pattern from VGA sensor
	{
	     printk(" pattern on setting~~~~~~~~~~~\n");
	     err = s5k6aafx_write_regs(sd, s5k6aafx_pattern_on, sizeof(s5k6aafx_pattern_on) / sizeof(s5k6aafx_pattern_on[0]));
          //  mdelay(200);
	}
	else
	{
#endif

	/* set initial regster value */
        err = s5k6aafx_write_regs(s5k6aafx_preview,
			sizeof(s5k6aafx_preview) / sizeof(s5k6aafx_preview[0]));
        if (unlikely(err)) {
                printk("%s: failed to make preview\n", __func__);
                return err;
            }
//	}

//	mdelay(200); // add 200 ms for displaying preview

	return err;
}

static int s5k6aafx_set_preview_stop()
{
	int err = 0;

	return err;
}

#if defined(CONFIG_TARGET_LOCALE_LTN)
//latin_cam VT CAM Antibanding
static int s5k6aafx_set_60hz_antibanding()
{
//	struct i2c_client *client = s5k6aafx_client;
	int err = -EINVAL;

	unsigned long s5k6aafx_antibanding60hz[] = {
	0xFCFCD000,
	0x00287000, 
	// Anti-Flicker //
	// End user init script
	0x002A0400,
	0x0F12005F,  //REG_TC_DBG_AutoAlgEnBits //Auto Anti-Flicker is enabled bit[5] = 1.
	0x002A03DC,
	0x0F120002,  //02 REG_SF_USER_FlickerQuant //Set flicker quantization(0: no AFC, 1: 50Hz, 2: 60 Hz)
	0x0F120001, 
	};

	err = s5k6aafx_write_regs(s5k6aafx_antibanding60hz,
				       	sizeof(s5k6aafx_antibanding60hz) / sizeof(s5k6aafx_antibanding60hz[0]));
	printk("%s:  setting 60hz antibanding \n", __func__);
	if (unlikely(err)) 
	{
		printk("%s: failed to set 60hz antibanding \n", __func__);
		return err;
	}	

	return 0;
}
//hmin84.park - 100706
#endif

static int s5k6aafx_set_brightness(int value)
{
//	struct i2c_client *client = s5k6aafx_client;
	int err = -EINVAL;

	switch (value)
	{
		//case EV_MINUS_4:
		case 0 :
			err = s5k6aafx_write_regs(s5k6aafx_bright_m4, \
				sizeof(s5k6aafx_bright_m4) / sizeof(s5k6aafx_bright_m4[0]));
			break;
//		case EV_MINUS_3:
		case 1 :
			err = s5k6aafx_write_regs(s5k6aafx_bright_m3, \
				sizeof(s5k6aafx_bright_m3) / sizeof(s5k6aafx_bright_m3[0]));

			break;
//		case EV_MINUS_2:
		case 2 :
			err = s5k6aafx_write_regs(s5k6aafx_bright_m2, \
				sizeof(s5k6aafx_bright_m2) / sizeof(s5k6aafx_bright_m2[0]));
			break;
//		case EV_MINUS_1:
		case 3 :
			err = s5k6aafx_write_regs(s5k6aafx_bright_m1, \
				sizeof(s5k6aafx_bright_m1) / sizeof(s5k6aafx_bright_m1[0]));
			break;
//		case EV_DEFAULT:
		case 4 :
			err = s5k6aafx_write_regs(s5k6aafx_bright_default, \
				sizeof(s5k6aafx_bright_default) / sizeof(s5k6aafx_bright_default[0]));
			break;
//		case EV_PLUS_1:
		case 5:
			err = s5k6aafx_write_regs(s5k6aafx_bright_p1, \
				sizeof(s5k6aafx_bright_p1) / sizeof(s5k6aafx_bright_p1[0]));
			break;
//		case EV_PLUS_2:
		case 6 :
			err = s5k6aafx_write_regs(s5k6aafx_bright_p2, \
				sizeof(s5k6aafx_bright_p2) / sizeof(s5k6aafx_bright_p2[0]));
			break;
//		case EV_PLUS_3:
		case 7:
			err = s5k6aafx_write_regs(s5k6aafx_bright_p3, \
				sizeof(s5k6aafx_bright_p3) / sizeof(s5k6aafx_bright_p3[0]));
			break;
//		case EV_PLUS_4:
		case 8 :
			err = s5k6aafx_write_regs(s5k6aafx_bright_p4, \
				sizeof(s5k6aafx_bright_p4) / sizeof(s5k6aafx_bright_p4[0]));
			break;
		default:
			printk("%s : there's no brightness value with [%d]\n", __func__,value);
			return err;
			break;
	}

	if (err < 0)
	{
		printk("%s : i2c_write for set brightness\n", __func__);
		return -EIO;
	}
	
	return err;
}

static int s5k6aafx_set_blur(int value)
{
//	struct i2c_client *client = s5k6aafx_client;
	
	int err = -EINVAL;

	switch (value)
	{
//		case BLUR_LEVEL_0:
		case 0 :
			err = s5k6aafx_write_regs(s5k6aafx_vt_pretty_default, \
				sizeof(s5k6aafx_vt_pretty_default) / sizeof(s5k6aafx_vt_pretty_default[0]));
			break;
//		case BLUR_LEVEL_1:
		case 1 :
			err = s5k6aafx_write_regs(s5k6aafx_vt_pretty_1, \
				sizeof(s5k6aafx_vt_pretty_1) / sizeof(s5k6aafx_vt_pretty_1[0]));
			break;
//		case BLUR_LEVEL_2:
		case 2 :
			err = s5k6aafx_write_regs(s5k6aafx_vt_pretty_2, \
				sizeof(s5k6aafx_vt_pretty_2) / sizeof(s5k6aafx_vt_pretty_2[0]));
			break;
//		case BLUR_LEVEL_3:
//		case BLUR_LEVEL_MAX:
		case 4 :
			err = s5k6aafx_write_regs(s5k6aafx_vt_pretty_3, \
				sizeof(s5k6aafx_vt_pretty_3) / sizeof(s5k6aafx_vt_pretty_3[0]));
			break;
		default:
			printk("%s : there's no blur value with [%d]\n", __func__,value);
			return err;
			break;
	}

	if (err < 0)
	{
		printk("%s : i2c_write for set blur\n", __func__);
		return -EIO;
	}
	
	return err;
}

static int s5k6aafx_check_dataline_stop()
{
	struct i2c_client *client = s5k6aafx_client;
	int err = -EINVAL;

	extern int s5k6aafx_power_reset(void);

	printk( "pattern off setting~~~~~~~~~~~~~~\n");	
    
	s5k6aafx_write(client, 0xFCFCD000);
	s5k6aafx_write(client, 0x0028D000);
	s5k6aafx_write(client, 0x002A3100);
    	s5k6aafx_write(client, 0x0F120000);
        
   //	err =  s5k6aafx_write_regs(sd, s5k6aafx_pattern_off,	sizeof(s5k6aafx_pattern_off) / sizeof(s5k6aafx_pattern_off[0]));
	printk("%s: sensor reset\n", __func__);
#if defined(CONFIG_TARGET_LOCALE_KOR) || defined(CONFIG_TARGET_LOCALE_EUR) || defined(CONFIG_TARGET_LOCALE_HKTW) || defined(CONFIG_TARGET_LOCALE_HKTW_FET) || defined(CONFIG_TARGET_LOCALE_USAGSM)
       s5k6aafx_power_reset();
 #endif       
 
	printk("%s: load camera init setting \n", __func__);
	err =  s5k6aafx_write_regs(s5k6aafx_common,	sizeof(s5k6aafx_common) / sizeof(s5k6aafx_common[0]));
//       mdelay(100);
	return err;
}

static int s5k6aafx_set_flip(int value)
{
	int err = 0;

	if((value) == 1)
	{
			err = s5k6aafx_write_regs(s5k6aafx_vhflip_on,
					sizeof(s5k6aafx_vhflip_on) / sizeof(s5k6aafx_vhflip_on[0]));
	}
	else
	{
			err = s5k6aafx_write_regs(s5k6aafx_vhflip_off,
					sizeof(s5k6aafx_vhflip_off) / sizeof(s5k6aafx_vhflip_off[0]));
	}
	return err;
}

static int s5k6aafx_set_frame_rate(int value)
{
	int err = 0;

	switch(value)
	{
		case 7:
			err = s5k6aafx_write_regs(s5k6aafx_vt_7fps,
					sizeof(s5k6aafx_vt_7fps) / sizeof(s5k6aafx_vt_7fps[0]));
			break;
		case 10:
			err = s5k6aafx_write_regs(s5k6aafx_vt_10fps,
					sizeof(s5k6aafx_vt_10fps) / sizeof(s5k6aafx_vt_10fps[0]));

			break;
		case 12:
			err = s5k6aafx_write_regs(s5k6aafx_vt_12fps,
					sizeof(s5k6aafx_vt_12fps) / sizeof(s5k6aafx_vt_12fps[0]));

			break;
		case 15:
			err = s5k6aafx_write_regs(s5k6aafx_vt_13fps,
					sizeof(s5k6aafx_vt_13fps) / sizeof(s5k6aafx_vt_13fps[0]));
			break;
		case 30:
			printk("frame rate is 30\n");
			break;
		default:
			printk("%s: no such framerate\n", __func__);
			break;
	}
	return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////


//static long s5k6aafx_set_sensor_mode(enum sensor_mode_t mode)
static long s5k6aafx_set_sensor_mode(int mode)
{
	unsigned short value =0;
	int shade_value = 0;
	unsigned short agc_value = 0;
	switch (mode) 
	{
	case SENSOR_PREVIEW_MODE:
		printk("[PGH] S5K6AAFX SENSOR_PREVIEW_MODE START\n");
		
		s5k6aafx_set_preview_start();

		//test
		printk("<=PCAM=> test delay 150~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
		//mdelay(150);
		msleep(300);
	
		break;

	case SENSOR_SNAPSHOT_MODE:

		printk("<=PCAM=> SENSOR_SNAPSHOT_MODE START\n");
		
		s5k6aafx_set_capture_start();
		
		//test
		printk("<=PCAM=> so many 200msecdelay~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
		//mdelay(200);
		msleep(200);
		
		break;

	
	//case SENSOR_SNAPSHOT_TRANSFER:
	//	CAM_DEBUG("SENSOR_SNAPSHOT_TRANSFER START\n");


		break;
		
	default:
		return -EFAULT;
	}

	return 0;
}


static long s5k6aafx_set_effect(
	int mode,
	int effect
)
{
	long rc = 0;
	switch(effect)
	{
		default:
			printk("[Effect]Invalid Effect !!!\n");
			return -EINVAL;
	}
	return rc;
}


static int s5k6aafx_reset(void)
{
	printk("s5k6aafx_reset");
	gpio_set_value(CAM_STANDBY, 0);
	//gpio_set_value(CAM_RESET, 0);
	gpio_set_value_cansleep(CAM_RESET, 0);

	printk(" 2. CAM_VT_nSTBY = 0 ");
	gpio_set_value_cansleep(CAM_VT_nSTBY, 0);

	printk(" 3. CAM_VT_RST = 0 ");
	gpio_set_value_cansleep(CAM_VT_RST, 0);

	mdelay(30);	
	CAM_DEBUG(" CAM PM already on \n");
	//CAM_DEBUG(" 1. PMIC ON ");
	//cam_pmic_onoff(ON);

	//CAM_DEBUG(" 1. CAM_8M_RST = 0 ");
	//gpio_set_value(CAM_8M_RST, LOW);
	
	CAM_DEBUG(" 2. CAM_VT_nSTBY = 1 ");
	gpio_set_value_cansleep(CAM_VT_nSTBY, 1);
	mdelay(20);

	CAM_DEBUG(" 3. CAM_VT_RST = 1 ");
	gpio_set_value_cansleep(CAM_VT_RST, 1);
	mdelay(30); // min 350ns

	return 0;
}


static int s5k6aafx_set_ev(
	int ev)
{
	CAM_DEBUG(" ev : %d \n",ev);

#if 0
	switch(ev)
	{
		case s5k6aafx_EV_MINUS_4:
			s5k6aafx_i2c_write_list(reg_brightness_0_list,sizeof(reg_brightness_0_list)/sizeof(reg_brightness_0_list[0]),"reg_brightness_0_list");
			break;
		case s5k6aafx_EV_MINUS_3:
			s5k6aafx_i2c_write_list(reg_brightness_1_list,sizeof(reg_brightness_1_list)/sizeof(reg_brightness_1_list[0]),"reg_brightness_1_list");
			break;
		case s5k6aafx_EV_MINUS_2:
			s5k6aafx_i2c_write_list(reg_brightness_2_list,sizeof(reg_brightness_2_list)/sizeof(reg_brightness_2_list[0]),"reg_brightness_2_list");
			break;
		case s5k6aafx_EV_MINUS_1:
			s5k6aafx_i2c_write_list(reg_brightness_3_list,sizeof(reg_brightness_3_list)/sizeof(reg_brightness_3_list[0]),"reg_brightness_3_list");
			break;
		case s5k6aafx_EV_DEFAULT:
			s5k6aafx_i2c_write_list(reg_brightness_4_list,sizeof(reg_brightness_4_list)/sizeof(reg_brightness_4_list[0]),"reg_brightness_4_list");
			break;
		case s5k6aafx_EV_PLUS_1:
			s5k6aafx_i2c_write_list(reg_brightness_5_list,sizeof(reg_brightness_5_list)/sizeof(reg_brightness_5_list[0]),"reg_brightness_5_list");
			break;
		case s5k6aafx_EV_PLUS_2:
			s5k6aafx_i2c_write_list(reg_brightness_6_list,sizeof(reg_brightness_6_list)/sizeof(reg_brightness_6_list[0]),"reg_brightness_6_list");
			break;	
		case s5k6aafx_EV_PLUS_3:
			s5k6aafx_i2c_write_list(reg_brightness_7_list,sizeof(reg_brightness_7_list)/sizeof(reg_brightness_7_list[0]),"reg_brightness_7_list");
			break;
		case s5k6aafx_EV_PLUS_4:
			s5k6aafx_i2c_write_list(reg_brightness_8_list,sizeof(reg_brightness_8_list)/sizeof(reg_brightness_8_list[0]),"reg_brightness_8_list");
			break;
		default:
			printk("[EV] Invalid EV !!!\n");
			return -EINVAL;
	}
#endif

	return 0;
}

static int s5k6aafx_set_dtp(
	int* onoff)
{
	CAM_DEBUG(" onoff : %d ",*onoff);

#if 0
	switch((*onoff))
	{
		case s5k6aafx_DTP_OFF:
			if(s5k6aafx_ctrl->dtp_mode)
				s5k6aafx_reset();			
			s5k6aafx_ctrl->dtp_mode = 0;

			/* set ACK value */
			
			//s5k6aafx_start();
			s5k6aafx_i2c_write_list(reg_init_list,sizeof(reg_init_list)/sizeof(reg_init_list[0]),"reg_init_list");
			
			//s5k6aafx_set_sensor_mode(SENSOR_PREVIEW_MODE);

			(*onoff) = M5MO_DTP_OFF_ACK;
			break;
		case s5k6aafx_DTP_ON:
			s5k6aafx_reset();
			s5k6aafx_i2c_write_list(reg_DTP_list,sizeof(reg_DTP_list)/sizeof(reg_DTP_list[0]),"reg_DTP_list");
			s5k6aafx_ctrl->dtp_mode = 1;

			/* set ACK value */
			(*onoff) = M5MO_DTP_ON_ACK;
			break;
		default:
			printk("[DTP]Invalid DTP mode!!!\n");
			return -EINVAL;
	}
#endif

	return 0;
}

#if 0
int cam_pmic_onoff(int onoff)
{
	static int last_state = -1;

	if(last_state == onoff)
	{
		CAM_DEBUG("%s : PMIC already %d\n",__FUNCTION__,onoff);
		return 0;
	}
	
	if(onoff)		// ON
	{
		CAM_DEBUG("%s ON\n",__FUNCTION__);

		//gpio_direction_output(CAM_PMIC_STBY, LOW);		// set PMIC to STBY mode
		gpio_set_value(CAM_PMIC_STBY, 0);
		mdelay(2);
		
		cam_pm_lp8720_i2c_write(0x07, 0x09); // BUCKS2:1.2V, no delay
		cam_pm_lp8720_i2c_write(0x04, 0x05); // LDO4:1.2V, no delay
		cam_pm_lp8720_i2c_write(0x02, 0x19); // LDO2 :2.8V, no delay
		cam_pm_lp8720_i2c_write(0x05, 0x0C); // LDO5 :1.8V, no delay
		cam_pm_lp8720_i2c_write(0x01, 0x19); // LDO1 :2.8V, no delay
		cam_pm_lp8720_i2c_write(0x08, 0xBB); // Enable all power without LDO3
		
		//gpio_direction_output(CAM_PMIC_STBY, HIGH);
		gpio_set_value(CAM_PMIC_STBY, 1);
		mdelay(5);
	}
	else
	{
		CAM_DEBUG("%s OFF\n",__FUNCTION__);
	}

	return 0;
}
#endif

#if 0
static int s5k6aafx_start(void)
{
	int rc=0;
	unsigned short value=0;

	CAM_DEBUG("%s E\n",__FUNCTION__);

	//s5k6aafx_i2c_test();
	
	//s5k6aafx_i2c_write_list(INIT_DATA,sizeof(INIT_DATA)/sizeof(INIT_DATA[0]),"INIT_DATA");
#if 0
	s5k6aafx_i2c_write_list(reg_init_list,sizeof(reg_init_list)/sizeof(reg_init_list[0]),"reg_init_list");
	s5k6aafx_i2c_write_list(reg_effect_none_list,sizeof(reg_effect_none_list)/sizeof(reg_effect_none_list[0]),"reg_effect_none_list");
	s5k6aafx_i2c_write_list(reg_meter_center_list,sizeof(reg_meter_center_list)/sizeof(reg_meter_center_list[0]),"reg_meter_center_list");
	s5k6aafx_i2c_write_list(reg_wb_auto_list,sizeof(reg_wb_auto_list)/sizeof(reg_wb_auto_list[0]),"reg_wb_auto_list");
#endif	
	
	s5k6aafx_i2c_read(0x00,0x7b,&value);
	CAM_DEBUG(" 0x7b value : 0x%x\n",value);
/*
	s5k6aafx_i2c_read(0x00,0x00,&value);
	CAM_DEBUG("read test -- reg - 0x%x, value - 0x%x\n",0x00 , value);
*/	return rc;
}
#endif

//for debug//
static void cam_gpio_print(int i)
{
	int test_value;
    test_value = gpio_get_value(CAM_VT_RST);
    printk("[PGH] %d CAM_VT_RST 175  : %d\n",i,test_value);	

    test_value = gpio_get_value(CAM_VT_nSTBY);
    printk("[PGH] %d CAM_VT_nSTBY 2 : %d\n",i,test_value);	

    test_value = gpio_get_value(CAM_RESET);
    printk("[PGH] %d CAM_RESET 75  : %d\n",i,test_value);	

    test_value = gpio_get_value(CAM_STANDBY);
    printk("[PGH] %d CAM_STANDBY 74  : %d\n",i, test_value);	

	test_value = gpio_get_value(CAM_EN);
	printk("[PGH] %d CAM_EN 3 : %d\n",i, test_value);	

	test_value = gpio_get_value(4);
	printk("[PGH] %d CAM_D0 4 : %d\n",i, test_value);	

	test_value = gpio_get_value(5);
	printk("[PGH] %d CAM_D1 5 : %d\n",i, test_value);	

	test_value = gpio_get_value(6);
	printk("[PGH] %d CAM_D2 6 : %d\n",i, test_value);	

	test_value = gpio_get_value(7);
	printk("[PGH] %d CAM_D3 7 : %d\n",i, test_value);	

	test_value = gpio_get_value(8);
	printk("[PGH] %d CAM_D4 8 : %d\n",i, test_value);	

	test_value = gpio_get_value(9);
	printk("[PGH] %d CAM_D5 9 : %d\n",i, test_value);	

	test_value = gpio_get_value(10);
	printk("[PGH] %d CAM_D6 10 : %d\n",i, test_value);	

	test_value = gpio_get_value(11);
	printk("[PGH] %d CAM_D7 11 : %d\n",i, test_value);	

	test_value = gpio_get_value(12);
	printk("[PGH] %d CAM_PCLK 12  : %d\n",i, test_value);	

	test_value = gpio_get_value(13);
	printk("[PGH] %d CAM_HSYNC 13  : %d\n",i, test_value);	

	test_value = gpio_get_value(14);
	printk("[PGH] %d CAM_VSYNC 14  : %d\n",i, test_value);	

	test_value = gpio_get_value(15);
	printk("[PGH] %d CAM_MCLK 15  : %d\n",i, test_value);	

	test_value = gpio_get_value(CAM_I2C_SDA);
	printk("[PGH] %d CAM_I2C_SDA 174  : %d\n",i, test_value);	

	test_value = gpio_get_value(CAM_I2C_SCL);
	printk("[PGH] %d CAM_I2C_SCL 176  : %d\n",i, test_value);

}

int s5k6aafx_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
//	struct i2c_client *client = s5k6aafx_client;
	int err = 0;
	//int err = -EINVAL;

	/* set initial regster value */
	if (s5k6aafx_ctrl->app_mode == CAMERA_MODE)
	{
		printk("%s: load camera common setting \n", __func__);
		err = s5k6aafx_write_regs(s5k6aafx_common,	sizeof(s5k6aafx_common) / sizeof(s5k6aafx_common[0]));
	}
	else
	{
		printk("%s: load camera VT call setting \n", __func__);
		err = s5k6aafx_write_regs(s5k6aafx_vt_common, sizeof(s5k6aafx_vt_common) / sizeof(s5k6aafx_vt_common[0]));

	}

	if (unlikely(err)) 
	{
		printk("%s: failed to init\n", __func__);
		return err;
	}

/*
#if defined(CONFIG_TARGET_LOCALE_LTN)
	//latin_cam VT Cam Antibanding
	if (state->anti_banding == ANTI_BANDING_60HZ)
	{
		err = s5k6aafx_set_60hz_antibanding(sd);
		if (unlikely(err)) 
		{
			printk("%s: failed to s5k6aafx_set_60hz_antibanding \n", __func__);
			return err;
		}
	}
	//hmin84.park -10.07.06
#endif
	*/

	cam_gpio_print(4);
	return err;
}

#if 0
static int s5k6aafx_sensor_init_probe(struct msm_camera_sensor_info *data)
{
	int rc = 0;
//	int read_value=-1;
//	unsigned short read_value_1 = 0;
//	int i; //for loop
//	int cnt = 0;
	CAM_DEBUG("s5k6aafx_sensor_init_probe start");
	
	gpio_set_value(CAM_STANDBY, 0);
	//gpio_set_value(CAM_RESET, 0);
	gpio_set_value_cansleep(CAM_RESET, 0);
	
	CAM_DEBUG(" CAM PM already on \n");
	
	//CAM_DEBUG(" 1. PMIC ON ");
	//cam_pmic_onoff(ON);

	//CAM_DEBUG(" 1. CAM_8M_RST = 0 ");
	//gpio_set_value(CAM_8M_RST, LOW);

	CAM_DEBUG(" 2. CAM_VT_nSTBY = 1 ");
	gpio_set_value_cansleep(CAM_VT_nSTBY, 1);
	mdelay(20);

	CAM_DEBUG(" 3. CAM_VT_RST = 1 ");
	gpio_set_value_cansleep(CAM_VT_RST, 1);
	mdelay(30); // min 350ns

	
#if 0
////////////////////////////////////////////////////
#if 0//Mclk_timing for M4Mo spec.		// -Jeonhk clock was enabled in vfe31_init
	msm_camio_clk_enable(CAMIO_VFE_CLK);
	msm_camio_clk_enable(CAMIO_MDC_CLK);
	msm_camio_clk_enable(CAMIO_VFE_MDC_CLK);
#endif	

	CAM_DEBUG("START MCLK:24Mhz~~~~~");
//	msm_camio_clk_rate_set(24000000);
	mdelay(5);
	msm_camio_camif_pad_reg_reset();		// this is not
	mdelay(10);
////////////////////////////////////////////////////
#endif

	s5k6aafx_start();

	return rc;

init_probe_fail:
	return rc;
}
#endif

static int s5k6aafx_hw_init()
{
	int rc = 0;

	printk("<=PCAM=> ++++++++++++++++++++++++++s5k6aafx test driver++++++++++++++++++++++++++++++++++++ \n");

	gpio_tlmm_config(GPIO_CFG(CAM_RESET, 0,GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE); //CAM_RESET
	gpio_tlmm_config(GPIO_CFG(CAM_STANDBY, 0,GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE); //CAM_STANDBY
	gpio_tlmm_config(GPIO_CFG(CAM_EN, 0,GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE); //CAM_EN
	gpio_tlmm_config(GPIO_CFG(CAM_VT_RST, 0,GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE); //CAM_VT_RST
	gpio_tlmm_config(GPIO_CFG(CAM_VT_nSTBY, 0,GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE); //CAM_VT_nSTBY

	gpio_set_value(CAM_VT_RST, 0); //VT_RST -> DOWN
	mdelay(30);
	gpio_set_value(CAM_VT_nSTBY, 0); //VT_nSTBY -> DOWN
	mdelay(30);
	struct vreg* vreg_L8;
	vreg_L8 = vreg_get(NULL, "gp7");
	vreg_set_level(vreg_L8, 1500);
	vreg_disable(vreg_L8);
	mdelay(30);
	gpio_set_value(CAM_EN, 0); //EN -> DOWN

	msleep(300);
	
	gpio_set_value(CAM_EN, 1); //EN -> UP
	vreg_enable(vreg_L8);
	mdelay(1);

	gpio_set_value(CAM_VT_nSTBY, 1); //VT_nSTBY -> UP
	mdelay(1);

	/* Input MCLK = 24MHz */
       	gpio_tlmm_config(GPIO_CFG(CAM_MCLK, 1,GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE); //CAM_MCLK            
	msm_camio_clk_rate_set(24000000);	//MCLK
	msm_camio_camif_pad_reg_reset();
	mdelay(1);

	gpio_set_value(CAM_STANDBY, 1); //STBY -> UP
	mdelay(1);

	gpio_set_value(CAM_RESET, 1); //REST -> UP
	mdelay(1);

	gpio_set_value(CAM_STANDBY, 0); //STBY -> DOWN
	mdelay(1);

	gpio_set_value(CAM_VT_RST, 1); //VT_RST -> UP
	mdelay(30);

	cam_gpio_print(3);			//debug

	return rc;

}



int s5k6aafx_sensor_init(struct msm_camera_sensor_info *data)
{
	int rc = 0;

	printk("[PGH] %s/%d \n", __func__, __LINE__);

	s5k6aafx_ctrl = kzalloc(sizeof(struct s5k6aafx_ctrl_t), GFP_KERNEL);
	if (!s5k6aafx_ctrl) {
		CDBG("s5k6aafx_sensor_init failed!\n");
		rc = -ENOMEM;
		goto init_done;
	}	
	
	if (data)
		s5k6aafx_ctrl->sensordata = data;

	rc = s5k6aafx_hw_init();
	if (rc < 0) 
	{
		printk("<=PCAM=> s5k6aafx_hw_init failed!\n");
		goto init_fail;
	}

	msleep(100);

#if 0
	rc = s5k6aafx_sensor_init_probe(data);
	if (rc < 0) {
		CDBG("s5k5cc_sensor_init failed!\n");
		goto init_fail;
	}
	printk("[PGH]  3333333 %s/%d \n", __func__, __LINE__);	
#endif
	s5k6aafx_sensor_init_probe(data);

	s5k6aafx_ctrl->app_mode = CAMERA_MODE;	

init_done:
	return rc;

init_fail:
	kfree(s5k6aafx_ctrl);
	return rc;
}

static int s5k6aafx_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&s5k6aafx_wait_queue);
	return 0;
}

/*
int s5k6aafx_sensor_ext_config(void __user *argp)
{
	sensor_ext_cfg_data		cfg_data;
	int rc=0;

	if(copy_from_user((void *)&cfg_data, (const void *)argp, sizeof(cfg_data)))
	{
		printk("<=PCAM=> %s fail copy_from_user!\n", __func__);
	}

	CAM_DEBUG("s5k6aafx_sensor_ext_config, cmd = %d ",cfg_data.cmd);
	
	switch(cfg_data.cmd) {
	case EXT_CFG_SET_BRIGHTNESS:
		rc = s5k6aafx_set_ev(cfg_data.value_1);
		break;
		
	case EXT_CFG_SET_DTP:
		rc = s5k6aafx_set_dtp(&cfg_data.value_1);
		break;

	default:
		break;
	}

	if(copy_to_user((void *)argp, (const void *)&cfg_data, sizeof(cfg_data)))
	{
		printk(" %s : copy_to_user Failed \n", __func__);
	}
	
	return rc;	
}
*/

int s5k6aafx_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cfg_data;
	long   rc = 0;

	if (copy_from_user(
				&cfg_data,
				(void *)argp,
				sizeof(struct sensor_cfg_data)))
		return -EFAULT;

	/* down(&m5mo_sem); */

	CDBG("s5k6aafx_sensor_config, cfgtype = %d, mode = %d\n",
		cfg_data.cfgtype, cfg_data.mode);

	switch (cfg_data.cfgtype) {
	case CFG_SET_MODE:
		rc = s5k6aafx_set_sensor_mode(cfg_data.mode);
		break;

	case CFG_SET_EFFECT:
	//	rc = s5k6aafx_set_effect(cfg_data.mode, cfg_data.cfg.effect);
		break;
		
	default:
		rc = -EFAULT;
		break;
	}

	/* up(&m5mo_sem); */

	return rc;
}

#if 0
int s5k6aafx_sensor_release(void)
{
	int rc = 0;

	/* down(&m5mo_sem); */

	CAM_DEBUG("POWER OFF");
	printk("camera turn off\n");

	CAM_DEBUG(" 1. CAM_VT_RST = 0 ");
	gpio_set_value_cansleep(CAM_VT_RST, 0);

	mdelay(2);
	
	CAM_DEBUG(" 2. CAM_VT_nSTBY = 0 ");
	gpio_set_value_cansleep(CAM_VT_nSTBY, 0);

	CAM_DEBUG(" 2. CAM_EN = 0 ");
	gpio_set_value(CAM_EN, 0);

/*
	if(system_rev >= 4)
	{
		CAM_DEBUG(" 2. CAM_SENSOR_A_EN = 0 ");
		gpio_set_value_cansleep(CAM_SENSOR_A_EN, 0);

		CAM_DEBUG(" 2. CAM_SENSOR_A_EN_ALTER = 0 ");
		gpio_set_value_cansleep(CAM_SENSOR_A_EN_ALTER, 0);

		vreg_disable(vreg_CAM_AF28);
	}
*/
	//cam_pmic_onoff(0);
	kfree(s5k6aafx_ctrl);
	
#ifdef CONFIG_LOAD_FILE
	s5k6aafx_regs_table_exit();
#endif

	/* up(&m5mo_sem); */

	return rc;
}
#endif

int s5k6aafx_sensor_release(void)
{
	int rc = 0;
        s5k6aafx_preview_start = 0;
	/* down(&s5k5cc_sem); */

	gpio_set_value(CAM_VT_RST, 0); //REST -> DOWN
	mdelay(1);

	gpio_set_value(CAM_RESET, 0); //REST -> DOWN
	mdelay(1);

	gpio_set_value(CAM_VT_nSTBY, 0); //STBY -> DOWN
	mdelay(1);

	printk("[PGH] %s/%d, disable vreg gp7!\n", __func__, __LINE__);
	vreg_disable(vreg_get(NULL, "gp7"));
	mdelay(1);
	gpio_set_value(CAM_EN, 0); //EN -> DOWN
	mdelay(1);

	kfree(s5k6aafx_ctrl);
	/* up(&s5k5cc_sem); */

	cam_gpio_print(0); 			//debug
	return rc;
}

//PGH:KERNEL2.6.25static int m5mo_probe(struct i2c_client *client)
int s5k6aafx_i2c_probe(struct i2c_client *client, 
	const struct i2c_device_id *id)
{
	int rc = 0;
	printk("[PGH]  %s/%d \n", __func__, __LINE__);	

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		goto probe_failure;
	}

	s5k6aafx_sensorw =
		kzalloc(sizeof(struct s5k6aafx_work_t), GFP_KERNEL);

	if (!s5k6aafx_sensorw) {
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, s5k6aafx_sensorw);
	s5k6aafx_init_client(client);
	s5k6aafx_client = client;

	printk("[PGH] s5k6aafx_i2c_probe successed! %s/%d \n", __func__, __LINE__);
	CDBG("s5k6aafx_i2c_probe successed!\n");

	return 0;

probe_failure:
	kfree(s5k6aafx_sensorw);
	s5k6aafx_sensorw = NULL;
	CDBG("s5k6aafx_i2c_probe failed!\n");
	return rc;
}

/*
int __exit s5k6aafx_i2c_remove(struct i2c_client *client)
{
	struct s5k6aafx_work_t *sensorw = i2c_get_clientdata(client);
	free_irq(client->irq, sensorw);
//	i2c_detach_client(client);
	s5k6aafx_client = NULL;
	s5k6aafx_sensorw = NULL;
	kfree(sensorw);
	return 0;
}
*/

static const struct i2c_device_id s5k6aafx_i2c_id[] = {
    { "s5k6aafx", 0 },
    { }
};

//PGH MODULE_DEVICE_TABLE(i2c, s5k6aafx);

static struct i2c_driver s5k6aafx_i2c_driver = {
	.id_table	= s5k6aafx_i2c_id,
	.probe  	= s5k6aafx_i2c_probe,
	.remove 	= __exit_p(s5k6aafx_i2c_remove),
	.driver 	= {
		.name = "s5k6aafx",
	},
};

//int m5mo_sensor_probe(void *dev, void *ctrl)
int s5k6aafx_sensor_probe(const struct msm_camera_sensor_info *info,
				struct msm_sensor_ctrl *s)
{

	printk("[PGH] 3 %s/%d \n", __func__, __LINE__);	
	int rc = i2c_add_driver(&s5k6aafx_i2c_driver);
	if (rc < 0 || s5k6aafx_client == NULL) {
		rc = -ENOTSUPP;
		goto probe_done;
	}
	printk("[PGH] %s/%d \n", __func__, __LINE__);	

	printk("[PGH] s5k6aafx_client->addr : %x\n", s5k6aafx_client->addr);
	printk("[PGH] s5k6aafx_client->adapter->nr : %d\n", s5k6aafx_client->adapter->nr);


#if 0//bestiq
	rc = s5k5cc_sensor_init_probe(info);
	if (rc < 0)
		goto probe_done;
#endif

	s->s_init		= s5k6aafx_sensor_init;
	s->s_release	= s5k6aafx_sensor_release;
	s->s_config	= s5k6aafx_sensor_config;
	s->s_camera_type = FRONT_CAMERA_2D;        
    	s->s_mount_angle = 180;

probe_done:
	CDBG("%s %s:%d\n", __FILE__, __func__, __LINE__);
	return rc;
	
}

/*
int __s5k6aafa_remove(struct platform_device *pdev)
{
	return msm_camera_drv_remove(pdev);
}
*/

static int __s5k6aafa_probe(struct platform_device *pdev)
{
	printk("[PGH] 2 %s/%d\n",  __func__, __LINE__);
	return msm_camera_drv_start(pdev, s5k6aafx_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __s5k6aafa_probe,
//	.remove	 = __s5k6aafx_remove,
	.driver = {
		.name = "msm_camera_s5k6aafx",
		.owner = THIS_MODULE,
	},
};

int __init s5k6aafx_init(void)
{
	printk("[PGH] 1  %s/%d\n",  __func__, __LINE__);
	return platform_driver_register(&msm_camera_driver);
};

/*
static void __exit s5k6aafx_exit(void)
{
	platform_driver_unregister(&msm_camera_driver);
}
*/

module_init(s5k6aafx_init);
//module_exit(s5k6aafx_exit);
