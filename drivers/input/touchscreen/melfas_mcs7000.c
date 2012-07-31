/* drivers/input/touchscreen/melfas_ts_i2c_tsi.c
 *
 * Copyright (C) 2007 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/lcd.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/miscdevice.h>
#include <linux/completion.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <mach/vreg.h>

#include "mcs7000_download.h"

#define INPUT_INFO_REG 0x10
#define IRQ_TOUCH_INT   MSM_GPIO_TO_INT(GPIO_TOUCH_INT)

#define FINGER_NUM	      5 

#undef CONFIG_CPU_FREQ
#undef CONFIG_MOUSE_OPTJOY

#ifdef CONFIG_CPU_FREQ
#include <plat/s3c64xx-dvfs.h>
#endif

#include <linux/i2c/fsa9480.h> 

static uint8_t buf1[6];
static int debug_level = 5;
#define debugprintk(level,x...)  do {if(debug_level>=level) printk(x);}while(0)

extern int mcsdl_download_binary_data(void);
extern int fsa9480_i2c_read(unsigned char u_addr, unsigned char *pu_data); 
static u8 fsa9480_device1 = 0x0;


#ifdef CONFIG_MOUSE_OPTJOY
extern int get_sending_oj_event();
#endif

#if defined(CONFIG_MACH_VITAL2)
#define TSP_TEST_MODE
#endif

extern struct class *sec_class;

struct input_info {
	int max_x;
	int max_y;
	int state;
	int x;
	int y;
	int z;
	int width;
	int finger_id;
};

struct melfas_ts_driver {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct work_struct  work;
	int irq;
	int hw_rev;
	int fw_ver;
	struct input_info info[FINGER_NUM+1];
	int suspended;
	struct early_suspend	early_suspend;
	
	#ifdef TSP_TEST_MODE
	struct mutex lock;
	touch_screen_driver_t *melfas_test_mode;
	touch_screen_t *touch_screen;	
	#endif	
};


struct melfas_ts_driver *melfas_ts = NULL;
struct i2c_driver melfas_ts_i2c;
struct workqueue_struct *melfas_ts_wq;

static struct vreg *vreg_touch;
static struct vreg *vreg_touchio;

//fot test

/***************************************************************************
* Declare custom type definitions
***************************************************************************/
int melfas_ts_suspend(pm_message_t mesg);
int melfas_ts_resume(void);

#ifdef LCD_WAKEUP_PERFORMANCE
static void ts_resume_work_func(struct work_struct *ignored);
static DECLARE_DELAYED_WORK(ts_resume_work, ts_resume_work_func);
static DECLARE_COMPLETION(ts_completion);
#endif

#ifdef TSP_TEST_MODE
static uint16_t tsp_test_temp[11];
static uint16_t tsp_test_reference[15][11];
static uint16_t tsp_test_inspection[15][11];
static uint16_t tsp_test_delta[5];
static bool sleep_state = false;
uint8_t refer_y_channel_num = 1;
uint8_t inspec_y_channel_num = 1;

int touch_screen_ctrl_testmode(int cmd, touch_screen_testmode_info_t *test_info, int test_info_num);
static int ts_melfas_test_mode(int cmd, touch_screen_testmode_info_t *test_info, int test_info_num);

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

static uint16_t melfas_ts_inspection_spec_table[TS_MELFAS_SENSING_CHANNEL_NUM*TS_MELFAS_EXCITING_CHANNEL_NUM*2] = {
	166, 234, 180, 252,	181, 252, 182, 253 , 184, 251, 183, 254, 184, 254, 184,	254, 186, 254, 186,	256, 175, 239,
	183, 236, 198, 255,	198, 255, 200, 253 , 201, 256, 202, 257, 202, 258, 202,	257, 203, 257, 200,	259, 187, 243,
	185, 237, 199, 256,	198, 256, 201, 256 , 201, 255, 202, 258, 202, 258, 203,	257, 203, 258, 201,	260, 188, 244,
	186, 239, 201, 258,	200, 258, 202, 259 , 202, 258, 203, 260, 203, 259, 203,	259, 204, 260, 201,	261, 190, 246,
	188, 241, 202, 261,	202, 261, 204, 260 , 204, 260, 206, 263, 205, 261, 206,	261, 205, 262, 203,	263, 192, 249,
	192, 245, 205, 265,	205, 264, 207, 265 , 208, 264, 206, 266, 209, 265, 207,	265, 207, 267, 205,	266, 199, 266,
	193, 250, 208, 271,	207, 271, 209, 270 , 209, 270, 210, 270, 210, 269, 211,	270, 210, 270, 210,	272, 195, 330,
	201, 259, 213, 280,	213, 278, 215, 276 , 215, 275, 214, 276, 214, 274, 213,	273, 213, 272, 209,	274, 188, 252,
	206, 265, 220, 287,	219, 286, 220, 285 , 221, 284, 220, 284, 220, 284, 220,	282, 220, 282, 215,	284, 193, 261,
	214, 278, 229, 300,	229, 299, 230, 297 , 231, 297, 231, 298, 231, 295, 230,	293, 230, 292, 225,	293, 203, 271,
	227, 292, 242, 316,	242, 315, 243, 312 , 243, 312, 243, 313, 243, 311, 242,	309, 241, 308, 236,	309, 213, 285,
	242, 314, 259, 337,	258, 336, 259, 334 , 259, 333, 258, 334, 258, 334, 256,	331, 256, 331, 251,	331, 226, 306,
	260, 342, 277, 366,	277, 365, 280, 363 , 279, 361, 279, 363, 278, 362, 278,	359, 277, 357, 273,	358, 244, 332,
	286, 380, 304, 401,	305, 399, 305, 398 , 305, 398, 305, 399, 304, 398, 305,	394, 304, 392, 299,	392, 268, 367,
	355, 480, 361, 497,	356, 492, 352, 485 , 348, 477, 343, 463, 347, 476, 352,	483, 353, 486, 355,	490, 336, 473
};

touch_screen_t touch_screen = {	
	{0},
	1,
	{0}
};

touch_screen_driver_t melfas_test_mode = {
	{
		TS_MELFAS_VENDOR_NAME,
		TS_MELFAS_VENDOR_CHIP_NAME,
		TS_MELFAS_VENDOR_ID, 
		0, 
		0, 
		TS_MELFAS_TESTMODE_MAX_INTENSITY, 
		TS_MELFAS_SENSING_CHANNEL_NUM, TS_MELFAS_EXCITING_CHANNEL_NUM, 
		2, 
		2850, 3750, 
		melfas_ts_inspection_spec_table
	},
	0,
	ts_melfas_test_mode
};
#endif


#ifdef CONFIG_HAS_EARLYSUSPEND
void melfas_ts_early_suspend(struct early_suspend *h);
void melfas_ts_late_resume(struct early_suspend *h);
#endif	/* CONFIG_HAS_EARLYSUSPEND */

#define TOUCH_HOME	KEY_HOME
#define TOUCH_MENU	KEY_MENU
#define TOUCH_BACK	KEY_BACK
#define TOUCH_SEARCH  KEY_SEARCH

int melfas_ts_tk_keycode[] =
{ TOUCH_HOME, TOUCH_MENU, TOUCH_BACK, TOUCH_SEARCH, };

struct device *ts_dev;

void mcsdl_vdd_on(void)
{
	int ret;

	 /* VTOUCH_2.8V */
	vreg_touch = vreg_get(NULL, "wlan2");
	ret = vreg_set_level(vreg_touch, 2800);
	if (ret) {
		printk(KERN_ERR "%s: VTOUCH_2.8V set level failed (%d)\n", __func__, ret);
	}

	ret = vreg_enable(vreg_touch);
	if (ret) {
		printk(KERN_ERR "%s: vreg_touch enable failed (%d)\n", __func__, ret);
		//return -EIO;
	}
	else {
		#ifndef PRODUCT_SHIP
		printk(KERN_INFO "%s: wlan2_vreg_touch enable success!\n", __func__);
		#endif
	}

	/* VTOUCHIO_1.8V */
	vreg_touchio = vreg_get(NULL, "gp13");
	ret = vreg_set_level(vreg_touchio, 1800);
	if (ret) {
		printk(KERN_ERR "%s: VTOUCHIO_1.8V set level failed (%d)\n", __func__, ret);
	}

	ret = vreg_enable(vreg_touchio);
	if (ret) {
		printk(KERN_ERR "%s: vreg_touchio enable failed (%d)\n", __func__, ret);
		//return -EIO;
	}
	else {
		#ifndef PRODUCT_SHIP
		printk(KERN_INFO "%s: gp13_vreg_touchio enable success!\n", __func__);
		#endif
	}

  	mdelay(25); //MUST wait for 25ms after vreg_enable()
}

void mcsdl_vdd_off(void)
{
  vreg_disable(vreg_touch);
  vreg_disable(vreg_touchio);
  mdelay(100); //MUST wait for 100ms before vreg_enable()
}

static int melfas_i2c_write(struct i2c_client* p_client, u8* data, int len)
{
	struct i2c_msg msg;

	msg.addr = p_client->addr;
	msg.flags = 0; /* I2C_M_WR */
	msg.len = len;
	msg.buf = data ;

	if (1 != i2c_transfer(p_client->adapter, &msg, 1))
	{
		printk("%s set data pointer fail!\n", __func__);
		return -EIO;
	}

	return 0;
}

static int melfas_i2c_read(struct i2c_client* p_client, u8 reg, u8* data, int len)
{

	struct i2c_msg msg;

	/* set start register for burst read */
	/* send separate i2c msg to give STOP signal after writing. */

	msg.addr = p_client->addr;
	msg.flags = 0;
	msg.len = 1;
	msg.buf = &reg;

	if (1 != i2c_transfer(p_client->adapter, &msg, 1))
	{
		printk("%s set data pointer fail! reg(%x)\n", __func__, reg);
		return -EIO;
	}

	/* begin to read from the starting address */

	msg.addr = p_client->addr;
	msg.flags = I2C_M_RD;
	msg.len = len;
	msg.buf = data;

	if (1 != i2c_transfer(p_client->adapter, &msg, 1))
	{
		printk("%s fail! reg(%x)\n", __func__, reg);
		return -EIO;
	}

	return 0;
}

static void melfas_read_version(void)
{
	u8 buf[2] = {0,};

	if (0 == melfas_i2c_read(melfas_ts->client, MCSTS_MODULE_VER_REG, buf, 2))
	{
		melfas_ts->hw_rev = buf[0];
		melfas_ts->fw_ver = buf[1];
		//printk("%s :HW Ver : 0x%02x, FW Ver : 0x%02x\n", __func__,buf[0],buf[1]);
	}
	else
	{
		melfas_ts->hw_rev = 0;
		melfas_ts->fw_ver = 0;
		printk("%s : Can't find HW Ver, FW ver!\n", __func__);
	}
}

static void melfas_read_resolution(void)
{
	uint16_t max_x=0, max_y=0;
	u8 buf[3] = {0,};

	if(0 == melfas_i2c_read(melfas_ts->client, MCSTS_RESOL_HIGH_REG , buf, 3)){

		//printk("%s :buf[0] : 0x%02x, buf[1] : 0x%02x, buf[2] : 0x%02x\n", __func__,buf[0],buf[1],buf[2]);

		if(buf[0] == 0){
			melfas_ts->info[0].max_x = 320;
			melfas_ts->info[0].max_y = 480;
		}
		else{
			max_x = buf[1] | ((uint16_t)(buf[0] & 0x0f) << 8);
			max_y = buf[2] | (((uint16_t)(buf[0] & 0xf0) >> 4) << 8);
			melfas_ts->info[0].max_x = max_x;
			melfas_ts->info[0].max_y = max_y;
		}
	}
	else
	{
		melfas_ts->info[0].max_x = 320;
		melfas_ts->info[0].max_y = 480;
	}
	//printk("%s :, Set as max_x: %d, max_y: %d\n", __func__,melfas_ts->info.max_x, melfas_ts->info.max_y);
}

static void melfas_firmware_download(void)
{

	int ret;

	disable_irq(melfas_ts->client->irq);

	gpio_tlmm_config(GPIO_CFG(GPIO_I2C0_SCL,  0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_I2C0_SDA,  0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_TOUCH_INT, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	#if 0 //sdcard update
	ret = mcsdl_download_binary_data_sdcard();
	#else //binary update
	ret = mcsdl_download_binary_data();
	#endif

	enable_irq(melfas_ts->client->irq);
	melfas_read_version();
	if(ret > 0) {
			if (melfas_ts->hw_rev < 0) {
				printk(KERN_ERR "i2c_transfer failed\n");
			}

			if (melfas_ts->fw_ver < 0) {
				printk(KERN_ERR "i2c_transfer failed\n");
			}

			#ifndef PRODUCT_SHIP
			printk("[TOUCH] Firmware update success! [Melfas H/W version: 0x%02x., Current F/W version: 0x%02x.]\n", melfas_ts->hw_rev, melfas_ts->fw_ver);
			#endif
	}
	else {
		printk("[TOUCH] Firmware update failed.. RESET!\n");
		mcsdl_vdd_off();
		mdelay(500);
		mcsdl_vdd_on();
		mdelay(200);

	}

}



static ssize_t registers_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int status, mode_ctl, hw_rev, fw_ver;

	status  = i2c_smbus_read_byte_data(melfas_ts->client, MCSTS_STATUS_REG);
	if (status < 0) {
		printk(KERN_ERR "i2c_smbus_read_byte_data failed\n");;
	}
	mode_ctl = i2c_smbus_read_byte_data(melfas_ts->client, MCSTS_MODE_CONTROL_REG);
	if (mode_ctl < 0) {
		printk(KERN_ERR "i2c_smbus_read_byte_data failed\n");;
	}
	hw_rev = i2c_smbus_read_byte_data(melfas_ts->client, MCSTS_MODULE_VER_REG);
	if (hw_rev < 0) {
		printk(KERN_ERR "i2c_smbus_read_byte_data failed\n");;
	}
	fw_ver = i2c_smbus_read_byte_data(melfas_ts->client, MCSTS_FIRMWARE_VER_REG);
	if (fw_ver < 0) {
		printk(KERN_ERR "i2c_smbus_read_byte_data failed\n");;
	}

	sprintf(buf, "[TOUCH] Melfas Tsp Register Info.\n");
	sprintf(buf, "%sRegister 0x00 (status)  : 0x%08x\n", buf, status);
	sprintf(buf, "%sRegister 0x01 (mode_ctl): 0x%08x\n", buf, mode_ctl);
	sprintf(buf, "%sRegister 0x30 (hw_rev)  : 0x%08x\n", buf, hw_rev);
	sprintf(buf, "%sRegister 0x31 (fw_ver)  : 0x%08x\n", buf, fw_ver);

	return sprintf(buf, "%s", buf);
}

static ssize_t registers_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	int ret;
	if(strncmp(buf, "RESET", 5) == 0 || strncmp(buf, "reset", 5) == 0) {
		ret = i2c_smbus_write_byte_data(melfas_ts->client, 0x01, 0x01);
		if (ret < 0) {
			printk(KERN_ERR "i2c_smbus_write_byte_data failed\n");
		}
		printk("[TOUCH] software reset.\n");
	}
	return size;
}

static ssize_t gpio_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	sprintf(buf, "[TOUCH] Melfas Tsp Gpio Info.\n");
	sprintf(buf, "%sGPIO TOUCH_INT : %s\n", buf, gpio_get_value(GPIO_TOUCH_INT)? "HIGH":"LOW");
	return sprintf(buf, "%s", buf);
}

static ssize_t gpio_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	if(strncmp(buf, "ON", 2) == 0 || strncmp(buf, "on", 2) == 0) {
		mcsdl_vdd_on();
		//gpio_set_value(GPIO_TOUCH_EN, GPIO_LEVEL_HIGH);
		printk("[TOUCH] enable.\n");
		mdelay(200);
	}

	if(strncmp(buf, "OFF", 3) == 0 || strncmp(buf, "off", 3) == 0) {
		mcsdl_vdd_off();
		printk("[TOUCH] disable.\n");
	}

	if(strncmp(buf, "RESET", 5) == 0 || strncmp(buf, "reset", 5) == 0) {
		mcsdl_vdd_off();
		mdelay(500);
		mcsdl_vdd_on();
		printk("[TOUCH] reset.\n");
		mdelay(200);
	}
	return size;
}


static ssize_t firmware_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	melfas_read_version();

	sprintf(buf, "H/W rev. 0x%x F/W ver. 0x%x\n", melfas_ts->hw_rev, melfas_ts->fw_ver);
	return sprintf(buf, "%s", buf);
}



static ssize_t firmware_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
#ifdef CONFIG_TOUCHSCREEN_MELFAS_FIRMWARE_UPDATE

	if(strncmp(buf, "UPDATE", 6) == 0 || strncmp(buf, "update", 6) == 0) {
		#if defined(CONFIG_MACH_CHIEF)
		if(system_rev >= 8){
			melfas_firmware_download();
			}
		else
			printk("\nFirmware update error :: Check the your devices version.\n");
		#else //vital2
			if(system_rev >= 5){
			melfas_firmware_download();
			}
		else
			printk("\nFirmware update error :: Check the your devices version.\n");
		#endif
		}

#endif

	return size;
}



static ssize_t debug_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", debug_level);
}

static ssize_t debug_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	if(buf[0]>'0' && buf[0]<='9') {
		debug_level = buf[0] - '0';
	}

	return size;
}

#ifdef TSP_TEST_MODE
void touch_screen_sleep()
{	
	melfas_ts_suspend(PMSG_SUSPEND);
}

void touch_screen_wakeup()
{
	melfas_ts_resume();
}

int touch_screen_get_tsp_info(touch_screen_info_t *tsp_info)
{
		int ret = 0;
			
		/* chipset independent */
		tsp_info->driver = touch_screen.tsp_info.driver;
		tsp_info->reference.bad_point = touch_screen.tsp_info.reference.bad_point;
		tsp_info->reference.table = touch_screen.tsp_info.reference.table;

		/* chipset dependent */
		/* melfas */
		tsp_info->inspection.bad_point = touch_screen.tsp_info.inspection.bad_point;
		tsp_info->inspection.table = touch_screen.tsp_info.inspection.table;	
		return ret;
}

//****************************************************************************
//
// Function Name:   touch_screen_ctrl_testmode
//
// Description:     
//
// Notes: 
//
//****************************************************************************
int touch_screen_ctrl_testmode(int cmd, touch_screen_testmode_info_t *test_info, int test_info_num)
{
	int ret = 0; 
	bool prev_device_state = FALSE;
	touch_screen.driver = &melfas_test_mode;
	touch_screen.tsp_info.driver = &(touch_screen.driver->ts_info);

	if ( touch_screen.device_state == FALSE )
	{
		touch_screen_wakeup();
		touch_screen.device_state = TRUE;
		msleep(100);
		prev_device_state = TRUE;
	}

 	switch (cmd)
 	{
		case TOUCH_SCREEN_TESTMODE_SET_REFERENCE_SPEC_LOW:	
		{
			touch_screen.tsp_info.driver->reference_spec_low = test_info->reference;
			break;
		}
		
		case TOUCH_SCREEN_TESTMODE_SET_REFERENCE_SPEC_HIGH:	
		{
			touch_screen.tsp_info.driver->reference_spec_high = test_info->reference;
			break;
		}

		case TOUCH_SCREEN_TESTMODE_SET_INSPECTION_SPEC_LOW:	
		{
			//touch_screen.tsp_info.driver->inspection_spec_low = test_info->reference;
			break;
		}
		
		case TOUCH_SCREEN_TESTMODE_SET_INSPECTION_SPEC_HIGH:	
		{
			//touch_screen.tsp_info.driver->inspection_spec_high = test_info->reference;
			break;
		}
		
		case TOUCH_SCREEN_TESTMODE_RUN_SELF_TEST:
		{
			printk(KERN_DEBUG "STRAT TOUCH_SCREEN_TESTMODE_RUN_SELF_TEST\n") ; 	
			int i;
			uint16_t reference;
			uint16_t inspection, inspection_spec_low, inspection_spec_high;
			uint16_t* inspection_spec_table = NULL;
			uint16_t reference_table_size, inspection_table_size;
			touch_screen_testmode_info_t* reference_table = NULL;
			touch_screen_testmode_info_t* inspection_table = NULL;
			
			reference_table_size = inspection_table_size = touch_screen.tsp_info.driver->x_channel_num * touch_screen.tsp_info.driver->y_channel_num;
			/* chipset independent check item */
			/* reference */
			if ( touch_screen.tsp_info.reference.table == NULL )
			{
				 touch_screen.tsp_info.reference.table = (touch_screen_testmode_info_t*)kzalloc(sizeof(touch_screen_testmode_info_t) * reference_table_size,GFP_KERNEL);
			}
			else
			{	
				/* delete previous reference table */
				kzfree( touch_screen.tsp_info.reference.table);				
				touch_screen.tsp_info.reference.table = NULL;
				touch_screen.tsp_info.reference.table = (touch_screen_testmode_info_t*)kzalloc(sizeof(touch_screen_testmode_info_t) * reference_table_size,GFP_KERNEL);				
			}
			reference_table = touch_screen.tsp_info.reference.table;

			/* init reference info */
			test_info->bad_point = (uint16_t)0xfffe; // good sensor
			memset(reference_table, 0x00, sizeof(touch_screen_testmode_info_t) * reference_table_size);
			if (test_info != NULL)
			{
				touch_screen.driver->test_mode(TOUCH_SCREEN_TESTMODE_GET_REFERENCE, reference_table, reference_table_size);				
				for ( i = 0; i < reference_table_size; i++ )
				{
					reference = ((reference_table+i)->reference >> 8) & 0x00ff;					
					reference |= (reference_table+i)->reference << 8;				
					if (reference < touch_screen.tsp_info.driver->reference_spec_low || reference > touch_screen.tsp_info.driver->reference_spec_high )
					{
						/* bad sensor */
						touch_screen.tsp_info.reference.bad_point = test_info->bad_point = (uint16_t)i;
						printk("BAD POINT >> %5d: %5x: %5d:\n",i,((reference_table+i)->reference),reference);
					}
				}
				if ( test_info->bad_point == 0xfffe )
				{		
					touch_screen.tsp_info.reference.bad_point = test_info->bad_point; 
					/* good sensor, we don't need to save reference table */
					//free( touch_screen.tsp_info.reference.table);
					//touch_screen.tsp_info.reference.table = NULL;
				}
			}
			else
			{
				ret = -1;
			}	

			/* chipset dependent check item */
			/* melfas : inspection */
			if ( touch_screen.tsp_info.driver->ven_id == 0x50 && test_info_num > 1 )
			{
				if ( touch_screen.tsp_info.inspection.table == NULL )
				{
					 touch_screen.tsp_info.inspection.table = (touch_screen_testmode_info_t*)kzalloc(sizeof(touch_screen_testmode_info_t) * inspection_table_size,GFP_KERNEL);
				}
				else
				{
					/* delete previous reference table */
					kzfree( touch_screen.tsp_info.inspection.table);				
					touch_screen.tsp_info.inspection.table = NULL;
					touch_screen.tsp_info.inspection.table = (touch_screen_testmode_info_t*)kzalloc(sizeof(touch_screen_testmode_info_t) * inspection_table_size,GFP_KERNEL);
				}

				inspection_table = touch_screen.tsp_info.inspection.table;
				inspection_spec_table =  touch_screen.tsp_info.driver->inspection_spec_table;

				/* init inspection info */
				(test_info+1)->bad_point = (uint16_t)0xfffe; // good sensor
				memset(inspection_table, 0x00, sizeof(touch_screen_testmode_info_t) * inspection_table_size);

				if ( test_info != NULL )
				{
					touch_screen.driver->test_mode(TOUCH_SCREEN_TESTMODE_GET_INSPECTION, inspection_table, inspection_table_size);

					for ( i = 0; i < inspection_table_size; i++ )
					{
						inspection = ((((inspection_table+i)->inspection) & 0x0f) << 8);
						inspection +=((((inspection_table+i)->inspection) >> 8) & 0xff);
						
						inspection_spec_low = *(inspection_spec_table+i*2);
						inspection_spec_high = *(inspection_spec_table+i*2+1);

						if ( inspection < inspection_spec_low  || inspection > inspection_spec_high )							
						{
							/* bad sensor */
							touch_screen.tsp_info.inspection.bad_point = (test_info+1)->bad_point = (uint16_t)i;
							printk("BAD POINT >> %3d: %5d [%5d] %5d:\n",i,inspection_spec_low,inspection,inspection_spec_high);
						}
					}

					if ( (test_info+1)->bad_point == 0xfffe )
					{			
						touch_screen.tsp_info.inspection.bad_point = (test_info+1)->bad_point; 
						
						/* good sensor, we don't need to save inspection table */
						//free( touch_screen.tsp_info.inspection.table);
						//touch_screen.tsp_info.inspection.table = NULL;
					}
				}
				else
				{
					ret = -1;
				}
			}
			else
			{
				if ( test_info_num > 1 )
				{
					(test_info+1)->bad_point = 0xfffe;
				}
			}
	
			break;
		}
			
		default:
		{
			if (test_info != NULL)
			{
				printk(KERN_DEBUG "DEFAULT\n");
				ret = touch_screen.driver->test_mode(cmd, test_info, test_info_num);
			}	
			else
			{
				ret = -1;
			}
			break;
		}

 	}

	if ( prev_device_state == TRUE )
	{
		touch_screen_sleep();
	}

	return ret;
}

//****************************************************************************
//
// Function Name:   ts_melfas_test_mode
//
// Description:     
//
// Notes: 
//
//****************************************************************************
static int ts_melfas_test_mode(int cmd, touch_screen_testmode_info_t *test_info, int test_info_num)
{
	int i, ret = 0;
	uint8_t buf[TS_MELFAS_SENSING_CHANNEL_NUM*2];

	switch (cmd)
	{
		case TOUCH_SCREEN_TESTMODE_ENTER:
		{	
			//melfas_test_mode.ts_mode = TOUCH_SCREEN_TESTMODE;
			break;
		}
		
		case TOUCH_SCREEN_TESTMODE_EXIT:
		{
			//melfas_test_mode.ts_state = TOUCH_SCREEN_NORMAL;
			break;			
 		}

		case TOUCH_SCREEN_TESTMODE_GET_OP_MODE:
		{
			break;			
		}

		case TOUCH_SCREEN_TESTMODE_GET_THRESHOLD:
		{	
			ret = melfas_i2c_read(melfas_ts->client, TS_MELFAS_TESTMODE_TSP_THRESHOLD_REG, buf, 1); 
			test_info->threshold = buf[0];
			break;
		}

		case TOUCH_SCREEN_TESTMODE_GET_DELTA:
		{
			printk(KERN_DEBUG "DELTA\n");
		#if 1	
			buf[0] = TS_MELFAS_TESTMODE_CTRL_REG;
			buf[1] = 0x01;
			ret = melfas_i2c_write(melfas_ts->client, buf, 2); 	
	
			if ( ret > 0 )
			{	
				melfas_test_mode.ts_mode = TOUCH_SCREEN_TESTMODE;				
				mdelay(50);
			}
 			if ( melfas_test_mode.ts_mode == TOUCH_SCREEN_TESTMODE && 
				melfas_test_mode.ts_info.delta_point_num == test_info_num )
 			{
	 			for ( i = 0; i < TS_MELFAS_TESTMODE_MAX_INTENSITY; i++)
	 			{
	 				ret |= melfas_i2c_read(melfas_ts->client, TS_MELFAS_TESTMODE_1ST_INTENSITY_REG+i, buf, 1); 
					test_info[i].delta = buf[0];
					
					mdelay(20); // min 10ms, typical 50ms					
	 			}
 			}
			else
			{
				ret = -3;
			}
			buf[0] = TS_MELFAS_TESTMODE_CTRL_REG;
			buf[1] = 0x00;
			ret = melfas_i2c_write(melfas_ts->client, buf, 2); 	
			if ( ret > 0 )
			{	
				melfas_test_mode.ts_mode = TOUCH_SCREEN_NORMAL;				
			}
		#else
					melfas_test_mode.ts_mode = TOUCH_SCREEN_TESTMODE;	
				
				if ( melfas_test_mode.ts_mode == TOUCH_SCREEN_TESTMODE &&
					(melfas_test_mode.ts_info.x_channel_num * melfas_test_mode.ts_info.y_channel_num) == test_info_num )
				{	
					
					buf[0] = TS_MELFAS_TESTMODE_CTRL_REG;
					buf[1] = 0x02;
					ret = melfas_i2c_write(melfas_ts->client, buf, 2);
					mdelay(500);
					mutex_lock(&melfas_ts->lock);
				
					for ( i = 0; i < TS_MELFAS_EXCITING_CHANNEL_NUM; i++)
					{	
						ret |= melfas_i2c_read(melfas_ts->client,TS_MELFAS_TESTMODE_1ST_INTENSITY_REG, buf, TS_MELFAS_SENSING_CHANNEL_NUM); 
						memcpy(&(test_info[i*TS_MELFAS_SENSING_CHANNEL_NUM].delta), buf, TS_MELFAS_SENSING_CHANNEL_NUM);
				
					}
				
					buf[0] = TS_MELFAS_TESTMODE_CTRL_REG;
					buf[1] = 0x00;
					ret = melfas_i2c_write(melfas_ts->client, buf, 2); 	
					if ( ret == 0 )
					{	
						melfas_test_mode.ts_mode = TOUCH_SCREEN_NORMAL; 			
						mdelay(50);
					}	
				}
				else
				{
					ret = -3;
				}	
				
				mutex_unlock(&melfas_ts->lock);
		#endif
			break;
		}

		case TOUCH_SCREEN_TESTMODE_GET_REFERENCE:
		{				
			printk(KERN_DEBUG "REFERENCE\n");
			melfas_test_mode.ts_mode = TOUCH_SCREEN_TESTMODE;	
		
			if ( melfas_test_mode.ts_mode == TOUCH_SCREEN_TESTMODE &&
				(melfas_test_mode.ts_info.x_channel_num * melfas_test_mode.ts_info.y_channel_num) == test_info_num )
			{	
 				buf[0] = TS_MELFAS_TESTMODE_CTRL_REG;
				buf[1] = 0x02;
				ret = melfas_i2c_write(melfas_ts->client, buf, 2);
				mdelay(500);
 				mutex_lock(&melfas_ts->lock);
	 			for ( i = 0; i < TS_MELFAS_EXCITING_CHANNEL_NUM; i++)
	 			{	
					ret |= melfas_i2c_read(melfas_ts->client, TS_MELFAS_TESTMODE_REFERENCE_DATA_START_REG, buf, TS_MELFAS_SENSING_CHANNEL_NUM*2); 
					//printk("REFERENCE RAW DATA : [%2x%2x]\n",buf[1],buf[0]);
					memcpy(&(test_info[i*TS_MELFAS_SENSING_CHANNEL_NUM].reference), buf, TS_MELFAS_SENSING_CHANNEL_NUM*2);
 					
	 			}
 				buf[0] = TS_MELFAS_TESTMODE_CTRL_REG;
				buf[1] = 0x00;
				ret = melfas_i2c_write(melfas_ts->client, buf, 2);
 				if ( ret > 0 )
				{	
					melfas_test_mode.ts_mode = TOUCH_SCREEN_NORMAL;				
					mdelay(50);
				}	
			}
			else
			{
				ret = -3;
			}	

			mutex_unlock(&melfas_ts->lock);
 
			break;
		}

		case TOUCH_SCREEN_TESTMODE_GET_INSPECTION:
		{	
			int j;
					
				melfas_test_mode.ts_mode = TOUCH_SCREEN_TESTMODE;	
			printk(KERN_DEBUG "INSPECTION\n");
			if ( melfas_test_mode.ts_mode == TOUCH_SCREEN_TESTMODE &&
				(melfas_test_mode.ts_info.x_channel_num * melfas_test_mode.ts_info.y_channel_num) == test_info_num )
			{	
				buf[0] = TS_MELFAS_TESTMODE_INSPECTION_DATA_CTRL_REG;
				buf[1] = 0x1A;
				buf[2] = 0x0;
				buf[3] = 0x0;
				buf[4] = 0x0;
				buf[5] = 0x01;	// start flag
				ret = melfas_i2c_write(melfas_ts->client, buf, 6);
				mdelay(1000);
 				mutex_lock(&melfas_ts->lock);

				ret |= melfas_i2c_read(melfas_ts->client, TS_MELFAS_TESTMODE_INSPECTION_DATA_READ_REG, buf, 2); // dummy read
 	 			for ( j = 0; j < TS_MELFAS_EXCITING_CHANNEL_NUM; j++)
	 			{	
		 			for ( i = 0; i < TS_MELFAS_SENSING_CHANNEL_NUM; i++)
		 			{	
		 				buf[0] = TS_MELFAS_TESTMODE_INSPECTION_DATA_CTRL_REG;
						buf[1] = 0x1A;
						buf[2] = j;		// exciting ch
						buf[3] = i;		// sensing ch
						buf[4] = 0x0;		// reserved
						buf[5] = 0x02;	// start flag, 2: output inspection data, 3: output low data
						ret = melfas_i2c_write(melfas_ts->client, buf, 6);
						ret |= melfas_i2c_read(melfas_ts->client, TS_MELFAS_TESTMODE_INSPECTION_DATA_READ_REG, buf, 2);
						//printk("INSPECTION RAW DATA : [%2d,%2d][%02x%02x]\n",buf[2],buf[3],buf[1],buf[0]);
 						memcpy(&(test_info[i+(j*TS_MELFAS_SENSING_CHANNEL_NUM)].inspection), buf, 2);
	 				}
	 			}

				buf[0] = TS_MELFAS_TESTMODE_CTRL_REG;
				buf[1] = 0x00;
				ret = melfas_i2c_write(melfas_ts->client, buf, 2);
 				if ( ret > 0 )
				{	
					melfas_test_mode.ts_mode = TOUCH_SCREEN_NORMAL;				
					mdelay(50);
				}
				/*
				mcsdl_vdd_off();
				mcsdl_vdd_on();
 				mdelay(100);
				
				melfas_test_mode.ts_mode = TOUCH_SCREEN_NORMAL;				
				//mdelay(50);*/
			}
			else
			{
				ret = -3;
			}	
			
			mutex_unlock(&melfas_ts->lock); 			

			break;
		}

		default:
		{
			ret = -2;
			break;
		}
	}

	return ret;
}

static ssize_t tsp_name_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "Manufacturer : Melfas\nChip name : MMS100\n");
}

static ssize_t tsp_test_show(struct device *dev, struct device_attribute *attr, char *buf)
{

	int i ;
	for (i=0 ; i<11 ; i++){
		printk("%d,", tsp_test_temp[i]);
		}
	return sprintf(buf, "%5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d\n", 
		tsp_test_temp[0],tsp_test_temp[1],tsp_test_temp[2],tsp_test_temp[3],
		tsp_test_temp[4],tsp_test_temp[5],tsp_test_temp[6],tsp_test_temp[7],
		tsp_test_temp[8],tsp_test_temp[9],tsp_test_temp[10]);
}

static ssize_t tsp_test_store(struct device *dev, struct device_attribute *attr, char *buf, size_t size)
{
	touch_screen.driver = &melfas_test_mode;
	touch_screen.tsp_info.driver = &(touch_screen.driver->ts_info);
	if(strncmp(buf, "self", 4) == 0) {
        /* disable TSP_IRQ */		
		printk(KERN_DEBUG "START %s\n", __func__) ;
        disable_irq(melfas_ts->client->irq);
		touch_screen_info_t tsp_info = {0};
		touch_screen_get_tsp_info(&tsp_info);
		
		uint16_t reference_table_size;
		touch_screen_testmode_info_t* reference_table = NULL;
		reference_table_size = tsp_info.driver->x_channel_num * tsp_info.driver->y_channel_num;
		reference_table = (touch_screen_testmode_info_t*)kzalloc(sizeof(touch_screen_testmode_info_t) * reference_table_size,GFP_KERNEL);
		touch_screen_ctrl_testmode(TOUCH_SCREEN_TESTMODE_RUN_SELF_TEST, reference_table, reference_table_size);	

		mcsdl_vdd_off();
		mdelay(500);
		mcsdl_vdd_on();
		printk("[TOUCH] reset.\n");
		mdelay(200);

        /* enable TSP_IRQ */
		enable_irq(melfas_ts->client->irq);
		}

	else {
		debugprintk(5, "TSP Error Unknwon commad!!!\n");
		}

	return size ;
}
static ssize_t tsp_test_reference_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i, j ;
	uint8_t k;
	
	for ( j = 0; j < refer_y_channel_num ; j++ ) {
			for (i=0 ; i<11 ; i++) {
				printk("%5d ", tsp_test_reference[j][i]);
				}
				printk("\n");
			}
	k = refer_y_channel_num-1;
	return sprintf(buf, "%5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d\n", 
		tsp_test_reference[k][0],tsp_test_reference[k][1],tsp_test_reference[k][2],tsp_test_reference[k][3],
		tsp_test_reference[k][4],tsp_test_reference[k][5],tsp_test_reference[k][6],tsp_test_reference[k][7],
		tsp_test_reference[k][8],tsp_test_reference[k][9],tsp_test_reference[k][10]);

}
static void tsp_test_reference_func(uint8_t y_num)
{
	int i, j;
	touch_screen.driver = &melfas_test_mode;
	touch_screen.tsp_info.driver = &(touch_screen.driver->ts_info);
	refer_y_channel_num = y_num;
			/* disable TSP_IRQ */		
			printk(KERN_DEBUG "Reference START %s\n", __func__) ;
			disable_irq(melfas_ts->client->irq);
			touch_screen_info_t tsp_info = {0};
			touch_screen_get_tsp_info(&tsp_info);
			
			uint16_t reference[11];
			uint16_t reference_table_size;
			touch_screen_testmode_info_t* reference_table = NULL;
			reference_table_size = tsp_info.driver->x_channel_num * tsp_info.driver->y_channel_num;
			reference_table = (touch_screen_testmode_info_t*)kzalloc(sizeof(touch_screen_testmode_info_t) * reference_table_size,GFP_KERNEL);
			touch_screen_ctrl_testmode(TOUCH_SCREEN_TESTMODE_GET_REFERENCE, reference_table, reference_table_size); 

			for ( j = 0; j < y_num ; j++ ) {
				for ( i = 0; i < tsp_info.driver->x_channel_num; i++ ) {
					reference[i] = ((reference_table+i+(j*8))->reference >> 8) & 0x00ff;
					reference[i] |= (reference_table+i+(j*8))->reference << 8;
					tsp_test_reference[j][i] = reference[i];
					}
				}
			
			mcsdl_vdd_off();
			mdelay(500);
			mcsdl_vdd_on();
			printk("[TOUCH] reset.\n");
			mdelay(200);
	
			/* enable TSP_IRQ */
			enable_irq(melfas_ts->client->irq);

}
static ssize_t tsp_test_reference_store(struct device *dev, struct device_attribute *attr, char *buf, size_t size)
{
	if(strncmp(buf, "01", 2) == 0) 	
		tsp_test_reference_func(1);			
	else if(strncmp(buf, "02", 2) == 0)		
		tsp_test_reference_func(2);
 	else if(strncmp(buf, "03", 2) == 0)		
		tsp_test_reference_func(3);
 	else if(strncmp(buf, "04", 2) == 0)		
		tsp_test_reference_func(4);
 	else if(strncmp(buf, "05", 2) == 0)		
		tsp_test_reference_func(5);
 	else if(strncmp(buf, "06", 2) == 0)		
		tsp_test_reference_func(6);
	else if(strncmp(buf, "07", 2) == 0)		
		tsp_test_reference_func(7);
	else if(strncmp(buf, "08", 2) == 0)		
		tsp_test_reference_func(8);
	else if(strncmp(buf, "09", 2) == 0)		
		tsp_test_reference_func(9);
	else if(strncmp(buf, "10", 2) == 0)		
		tsp_test_reference_func(10);
	else if(strncmp(buf, "11", 2) == 0)		
		tsp_test_reference_func(11);
	else if(strncmp(buf, "12", 2) == 0)		
		tsp_test_reference_func(12);
	else if(strncmp(buf, "13", 2) == 0)		
		tsp_test_reference_func(13);
	else if(strncmp(buf, "14", 2) == 0)		
		tsp_test_reference_func(14);
	else if(strncmp(buf, "15", 2) == 0)		
		tsp_test_reference_func(15);
	else 
		debugprintk(5, "TSP Error Unknwon commad!!!\n");
		
	return size ;
}

static ssize_t tsp_test_inspection_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i, j;
	uint8_t k;
	
	for ( j = 0; j < inspec_y_channel_num ; j++ ) {
			for (i=0 ; i < 11 ; i++) {
				printk("%5d ", tsp_test_inspection[j][i]);
				}
				printk("\n");
			}
	k = inspec_y_channel_num-1;
	return sprintf(buf, "%5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d\n", 
		tsp_test_inspection[k][0],tsp_test_inspection[k][1],tsp_test_inspection[k][2],tsp_test_inspection[k][3],
		tsp_test_inspection[k][4],tsp_test_inspection[k][5],tsp_test_inspection[k][6],tsp_test_inspection[k][7],
		tsp_test_inspection[k][8],tsp_test_inspection[k][9],tsp_test_inspection[k][10]);
	
}
static void tsp_test_inspection_func(uint8_t y_num)
{
	int i, j;
	touch_screen.driver = &melfas_test_mode;
	touch_screen.tsp_info.driver = &(touch_screen.driver->ts_info);	
	inspec_y_channel_num = y_num;
			/* disable TSP_IRQ */		
			printk(KERN_DEBUG "Inspection START %s\n", __func__) ;
			disable_irq(melfas_ts->client->irq);
			touch_screen_info_t tsp_info = {0};
			touch_screen_get_tsp_info(&tsp_info);

			uint16_t inspection[11];
			uint16_t inspection_table_size;
			touch_screen_testmode_info_t* inspection_table = NULL;
									
			inspection_table_size = tsp_info.driver->x_channel_num * tsp_info.driver->y_channel_num;			
			inspection_table = (touch_screen_testmode_info_t*)kzalloc(sizeof(touch_screen_testmode_info_t) * inspection_table_size,GFP_KERNEL);
			touch_screen_ctrl_testmode(TOUCH_SCREEN_TESTMODE_GET_INSPECTION, inspection_table, inspection_table_size); 
			
			for ( j = 0; j < y_num; j++ )
			{
				for ( i = 0; i < tsp_info.driver->x_channel_num; i++ )
				{
					inspection[i] = (((inspection_table+i+(j*8))->inspection) & 0x0f) << 8; 				
					inspection[i] += ((((inspection_table+i+(j*8))->inspection) >> 8) & 0xff) ;
					tsp_test_inspection[j][i] = inspection[i];
				}
			}
			
			mcsdl_vdd_off();
			mdelay(500);
			mcsdl_vdd_on();
			printk("[TOUCH] reset.\n");
			mdelay(200);
	
			/* enable TSP_IRQ */
			enable_irq(melfas_ts->client->irq);

}
static ssize_t tsp_test_inspection_store(struct device *dev, struct device_attribute *attr, char *buf, size_t size)
	{
		if(strncmp(buf, "01", 2) == 0)	
			tsp_test_inspection_func(1); 		
		else if(strncmp(buf, "02", 2) == 0)		
			tsp_test_inspection_func(2);
		else if(strncmp(buf, "03", 2) == 0)		
			tsp_test_inspection_func(3);
		else if(strncmp(buf, "04", 2) == 0)		
			tsp_test_inspection_func(4);
		else if(strncmp(buf, "05", 2) == 0)		
			tsp_test_inspection_func(5);
		else if(strncmp(buf, "06", 2) == 0)		
			tsp_test_inspection_func(6);
		else if(strncmp(buf, "07", 2) == 0)		
			tsp_test_inspection_func(7);
		else if(strncmp(buf, "08", 2) == 0)		
			tsp_test_inspection_func(8);
		else if(strncmp(buf, "09", 2) == 0)		
			tsp_test_inspection_func(9);
		else if(strncmp(buf, "10", 2) == 0) 	
			tsp_test_inspection_func(10);
		else if(strncmp(buf, "11", 2) == 0) 	
			tsp_test_inspection_func(11);
		else if(strncmp(buf, "12", 2) == 0) 	
			tsp_test_inspection_func(12);
		else if(strncmp(buf, "13", 2) == 0) 	
			tsp_test_inspection_func(13);
		else if(strncmp(buf, "14", 2) == 0) 	
			tsp_test_inspection_func(14);
		else if(strncmp(buf, "15", 2) == 0) 	
			tsp_test_inspection_func(15);
		else 
			debugprintk(5, "TSP Error Unknwon commad!!!\n");
			
		return size ;
	}


static ssize_t tsp_test_delta_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%5d, %5d, %5d, %5d, %5d\n", 
		tsp_test_delta[0],tsp_test_delta[1],tsp_test_delta[2],tsp_test_delta[3],tsp_test_delta[4]);

}
static ssize_t tsp_test_delta_store(struct device *dev, struct device_attribute *attr, char *buf, size_t size)
{
	int i ;
	touch_screen.driver = &melfas_test_mode;
	touch_screen.tsp_info.driver = &(touch_screen.driver->ts_info);
	if(strncmp(buf, "delta", 5) == 0) {
			/* disable TSP_IRQ */		
			printk(KERN_DEBUG "Delta START %s\n", __func__) ;
			disable_irq(melfas_ts->client->irq);
			touch_screen_info_t tsp_info = {0};
			touch_screen_get_tsp_info(&tsp_info);
			
			touch_screen_testmode_info_t tsp_test_info[5];
			touch_screen_ctrl_testmode(TOUCH_SCREEN_TESTMODE_GET_DELTA, tsp_test_info, 5);

			for (i=0 ; i<5 ; i++){
					tsp_test_delta[i] = tsp_test_info[i].delta;
					}
			
			mcsdl_vdd_off();
			mdelay(500);
			mcsdl_vdd_on();
			printk("[TOUCH] reset.\n");
			mdelay(200);
	
			/* enable TSP_IRQ */
			enable_irq(melfas_ts->client->irq);
		}
	return size ;
}

static ssize_t tsp_test_sleep_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if(sleep_state)
		sprintf(buf, "sleep\n");
	else
		sprintf(buf, "wakeup\n");

	return sprintf(buf, "%s", buf);
}

static ssize_t tsp_test_sleep_store(struct device *dev, struct device_attribute *attr, char *buf, size_t size)
{
	if(strncmp(buf, "sleep", 5) == 0) {
		touch_screen_sleep();
		sleep_state = true;
		}
    else {
		debugprintk(5, "TSP Error Unknwon commad!!!\n");
		}

    return size ;
}

static ssize_t tsp_test_wakeup_store(struct device *dev, struct device_attribute *attr, char *buf, size_t size)
{
	if(strncmp(buf, "wakeup", 6) == 0) {
		touch_screen_wakeup();
		sleep_state = false;
		}
    else {
		debugprintk(5, "TSP Error Unknwon commad!!!\n");
		}

    return size ;
}

#endif

static DEVICE_ATTR(gpio, S_IRUGO | S_IWUSR, gpio_show, gpio_store);
static DEVICE_ATTR(registers, S_IRUGO | S_IWUSR, registers_show, registers_store);
static DEVICE_ATTR(firmware, S_IRUGO | S_IWUSR, firmware_show, firmware_store);
static DEVICE_ATTR(debug, S_IRUGO | S_IWUSR, debug_show, debug_store);

#ifdef TSP_TEST_MODE
static DEVICE_ATTR(tsp_name, S_IRUGO | S_IWUSR | S_IWGRP, tsp_name_show, NULL) ;
static DEVICE_ATTR(tsp_test, S_IRUGO | S_IWUSR | S_IWGRP, tsp_test_show, tsp_test_store) ;
static DEVICE_ATTR(tsp_reference, S_IRUGO | S_IWUSR | S_IWGRP, tsp_test_reference_show, tsp_test_reference_store) ;
static DEVICE_ATTR(tsp_inspection, S_IRUGO | S_IWUSR | S_IWGRP, tsp_test_inspection_show, tsp_test_inspection_store) ;
static DEVICE_ATTR(tsp_delta, S_IRUGO | S_IWUSR | S_IWGRP, tsp_test_delta_show, tsp_test_delta_store) ;
static DEVICE_ATTR(tsp_sleep, S_IRUGO | S_IWUSR | S_IWGRP, tsp_test_sleep_show, tsp_test_sleep_store) ;
static DEVICE_ATTR(tsp_wakeup, S_IRUGO | S_IWUSR | S_IWGRP, NULL, tsp_test_wakeup_store) ;
#endif

void melfas_read_reg()
{
	int ret;
	int ret1;
	struct i2c_msg msg[2];
	uint8_t start_reg;
	uint8_t buf;

	msg[0].addr = melfas_ts->client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &start_reg;

	start_reg = MCSTS_CABLE_DET_REG;

	msg[1].addr = melfas_ts->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = sizeof(buf);
	msg[1].buf = &buf;

	ret  = i2c_transfer(melfas_ts->client->adapter, &msg[0], 1);
	ret1 = i2c_transfer(melfas_ts->client->adapter, &msg[1], 1);

	if((ret < 0)||(ret1 < 0)) {
		printk(KERN_ERR "==melfas_ts_work_func: i2c_transfer failed!!== ret:%d ,ret1:%d\n",ret,ret1);
	}
	printk("===melfas_read_reg CABLE_DET_REG VALUE : 0x%02x===\n",buf);
}

void melfas_write_reg(u8 data)
{
	struct i2c_msg msg;
	u8 buf[2];

	buf[0] = MCSTS_CABLE_DET_REG;
	buf[1] = data;

	msg.addr = melfas_ts->client->addr;
	msg.flags = 0;
	msg.len = 2;
	msg.buf = buf;

	if (1 != i2c_transfer(melfas_ts->client->adapter, &msg, 1)){
		printk("%s fail! data(0x%x)\n", __func__,data);
		}
	#ifndef PRODUCT_SHIP
	printk("%s : data(0x%x)\n", __func__,data);
	#endif
}

void set_tsp_noise_filter_reg()
{
	fsa9480_i2c_read(REGISTER_DEVICETYPE1, &fsa9480_device1);
	if ((fsa9480_device1 & CRA_USB)||(fsa9480_device1 & CRA_DEDICATED_CHG)){
		melfas_write_reg(0x01);
		}
}

void tsp_reset(void)
{
	printk(KERN_DEBUG "for esd %s\n", __func__) ;

	mcsdl_vdd_off();
	gpio_direction_output(GPIO_I2C0_SCL, 0);  // TOUCH SCL DIS
	gpio_direction_output(GPIO_I2C0_SDA, 0);  // TOUCH SDA DIS

    gpio_direction_output(GPIO_I2C0_SCL, 1);  // TOUCH SCL EN
    gpio_direction_output(GPIO_I2C0_SDA, 1);  // TOUCH SDA EN	
    mcsdl_vdd_on();
    msleep(300); 
}
static void melfas_work_func(void)
{
	int i = 0;
	u8 id = 0;
	int z = 0;
	int width = 0;

	int x = buf1[2] | ((uint16_t)(buf1[1] & 0x0f) << 8);
	int y = buf1[3] | (((uint16_t)(buf1[1] & 0xf0) >> 4) << 8);

#if defined(CONFIG_MACH_CHIEF)
        if(system_rev >= 8){
                z = buf1[5];   //strength
                width = buf1[4]; // width
        }
        else
                z = buf1[4];   //strength
#else //vital2
		if(system_rev >= 5){
                z = buf1[5];   //strength
                width = buf1[4]; // width
        }
        else
                z = buf1[4];   //strength
#endif

        int finger = buf1[0] & 0x0f;  //Touch Point ID
        int touchaction = (int)((buf1[0] >> 4) & 0x3); //Touch action
#ifdef CONFIG_CPU_FREQ
        set_dvfs_perf_level();
#endif

        id = finger;
        //printk("===touchaction : 0x%02x===\n",touchaction);
			switch(touchaction) {
				case 0x0: // Non-touched state
					melfas_ts->info[id].x = -1;
					melfas_ts->info[id].y = -1;
					melfas_ts->info[id].z = -1;
					melfas_ts->info[id].finger_id = finger;
					z = 0;
					break;

				case 0x1: //touched state
					melfas_ts->info[id].x = x;
					melfas_ts->info[id].y = y;
					melfas_ts->info[id].z = z;
					melfas_ts->info[id].finger_id = finger;
					break;

				case 0x2:
					break;

				case 0x3: // Palm Touch
					printk(KERN_DEBUG "[TOUCH] Palm Touch!\n");
					break;

				case 0x7: // Proximity
					printk(KERN_DEBUG "[TOUCH] Proximity!\n");
					break;
			}

			melfas_ts->info[id].state = touchaction;
        for ( i= 1; i<FINGER_NUM+1; ++i ) {
#if defined(CONFIG_MACH_CHIEF)
                if(system_rev >= 8) {
                        //debugprintk(5,"[TOUCH_MT] x1: %4d, y1: %4d, z1: %4d, finger: %4d,\n", x, y, z, finger);
                        input_report_abs(melfas_ts->input_dev, ABS_MT_POSITION_X, melfas_ts->info[i].x);
                        input_report_abs(melfas_ts->input_dev, ABS_MT_POSITION_Y, melfas_ts->info[i].y);
                        input_report_abs(melfas_ts->input_dev, ABS_MT_TOUCH_MAJOR, melfas_ts->info[i].z);
                        input_report_abs(melfas_ts->input_dev, ABS_MT_WIDTH_MAJOR, melfas_ts->info[i].width);
                        input_report_abs(melfas_ts->input_dev, ABS_MT_TRACKING_ID, melfas_ts->info[i].finger_id-1);
                }
                else {
                        //debugprintk(5,"[TOUCH_MT] x1: %4d, y1: %4d, z1: %4d, finger: %4d,\n", x, y, z, finger);
                        input_report_abs(melfas_ts->input_dev, ABS_MT_POSITION_X, melfas_ts->info[i].x);
                        input_report_abs(melfas_ts->input_dev, ABS_MT_POSITION_Y, melfas_ts->info[i].y);
                        input_report_abs(melfas_ts->input_dev, ABS_MT_TOUCH_MAJOR, melfas_ts->info[i].z);
                        input_report_abs(melfas_ts->input_dev, ABS_MT_TRACKING_ID, melfas_ts->info[i].finger_id-1);
                }
#else //vital2
                if(system_rev >= 4) {
                        //debugprintk(5,"[TOUCH_MT] x1: %4d, y1: %4d, z1: %4d, finger: %4d,\n", x, y, z, finger);
                        input_report_abs(melfas_ts->input_dev, ABS_MT_POSITION_X, melfas_ts->info[i].x);
                        input_report_abs(melfas_ts->input_dev, ABS_MT_POSITION_Y, melfas_ts->info[i].y);
                        input_report_abs(melfas_ts->input_dev, ABS_MT_TOUCH_MAJOR, melfas_ts->info[i].z);
                        input_report_abs(melfas_ts->input_dev, ABS_MT_WIDTH_MAJOR, melfas_ts->info[i].width);
                        input_report_abs(melfas_ts->input_dev, ABS_MT_TRACKING_ID, melfas_ts->info[i].finger_id-1);
                }
                else {
                        //debugprintk(5,"[TOUCH_MT] x1: %4d, y1: %4d, z1: %4d, finger: %4d,\n", x, y, z, finger);
                        input_report_abs(melfas_ts->input_dev, ABS_MT_POSITION_X, melfas_ts->info[i].x);
                        input_report_abs(melfas_ts->input_dev, ABS_MT_POSITION_Y, melfas_ts->info[i].y);
                        input_report_abs(melfas_ts->input_dev, ABS_MT_TOUCH_MAJOR, melfas_ts->info[i].z);
                        input_report_abs(melfas_ts->input_dev, ABS_MT_TRACKING_ID, melfas_ts->info[i].finger_id-1);
                }
#endif
                input_mt_sync(melfas_ts->input_dev);
        }

#ifndef PRODUCT_SHIP
#if defined(CONFIG_MACH_CHIEF)
        if(system_rev >= 8)
                debugprintk(5,"[TOUCH] x: %4d, y: %4d, z: %4d, f: %4d, w: %4d\n", x, y, z, finger, width);
        else
                debugprintk(5,"[TOUCH] x: %4d, y: %4d, z: %4d, finger: %4d,\n", x, y, z, finger);	
#else
        if(system_rev >= 4)
                debugprintk(5,"[TOUCH] x: %4d, y: %4d, z: %4d, f: %4d, w: %4d\n", x, y, z, finger, width);
        else
                debugprintk(5,"[TOUCH] x: %4d, y: %4d, z: %4d, finger: %4d,\n", x, y, z, finger);
#endif
#endif
        input_sync(melfas_ts->input_dev);
}

irqreturn_t melfas_ts_irq_handler(int irq, void *dev_id)
{
  int ret;
  int ret1;

  struct i2c_msg msg[2];

  uint8_t start_reg;

  msg[0].addr = melfas_ts->client->addr;
  msg[0].flags = 0;
  msg[0].len = 1;
  msg[0].buf = &start_reg;
  start_reg = MCSTS_INPUT_INFO_REG;

  msg[1].addr = melfas_ts->client->addr;
  msg[1].flags = I2C_M_RD;
  msg[1].len = sizeof(buf1);
  msg[1].buf = buf1;

#if defined(CONFIG_MACH_CHIEF)
	if(system_rev >= 8){
		ret = i2c_transfer(melfas_ts->client->adapter, msg, 2);

		if (ret < 0){
			printk(KERN_ERR "melfas_ts_work_func: i2c_transfer failed\n");
			}
		else{
			melfas_work_func();
			}
		}
	else{
		ret  = i2c_transfer(melfas_ts->client->adapter, &msg[0], 1);
		ret1 = i2c_transfer(melfas_ts->client->adapter, &msg[1], 1);

		if((ret < 0) ||  (ret1 < 0)){
			printk(KERN_ERR "==melfas_ts_work_func: i2c_transfer failed!!== ret:%d ,ret1:%d\n",ret,ret1);
			}
		else{
			melfas_work_func();
			}
		}
#else //vital2
	if(system_rev >= 4){
		ret = i2c_transfer(melfas_ts->client->adapter, msg, 2);
		if (ret < 0){
			printk(KERN_ERR "melfas_ts_work_func: i2c_transfer failed\n");
			}
		else{
			if(buf1[0] == 0x0F)
			tsp_reset();
			else
			melfas_work_func();
			}
		}
	else{
		ret  = i2c_transfer(melfas_ts->client->adapter, &msg[0], 1);
		ret1 = i2c_transfer(melfas_ts->client->adapter, &msg[1], 1);

	  	if((ret < 0) ||  (ret1 < 0)){
	  		printk(KERN_ERR "==melfas_ts_work_func: i2c_transfer failed!!== ret:%d ,ret1:%d\n",ret,ret1);
			}
	  	else{
			if(buf1[0] == 0x0F)
			tsp_reset();
			else
	  		melfas_work_func();
			}
		}
#endif

  return IRQ_HANDLED;
}

int melfas_ts_probe(void)
{
	int ret = 0;
	uint16_t max_x=0, max_y=0;
	
#ifndef PRODUCT_SHIP 
	printk("\n====================================================");
	printk("\n=======         [TOUCH SCREEN] PROBE       =========");
	printk("\n====================================================\n");
#endif

	if (!i2c_check_functionality(melfas_ts->client->adapter, I2C_FUNC_I2C/*I2C_FUNC_SMBUS_BYTE_DATA*/)) {
		#ifndef PRODUCT_SHIP 
		printk(KERN_ERR "melfas_ts_probe: need I2C_FUNC_I2C\n");
		#endif
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	melfas_read_version();
#ifndef PRODUCT_SHIP 
	printk(KERN_INFO "[TOUCH] Melfas  H/W version: 0x%02x.\n", melfas_ts->hw_rev);
	printk(KERN_INFO "[TOUCH] Current F/W version: 0x%02x.\n", melfas_ts->fw_ver);
#endif

	mdelay(100);
	mcsdl_vdd_off();
	mdelay(300);
	mcsdl_vdd_on();
	
#if defined(CONFIG_MACH_CHIEF)
	if((system_rev >= 8) && (melfas_ts->fw_ver < 0x21))
		melfas_firmware_download();
#else //vital2
	if((system_rev >= 5) && (melfas_ts->fw_ver != 0x29))
		melfas_firmware_download(); 
#endif


	melfas_read_resolution();
	max_x = melfas_ts->info[0].max_x ;
	max_y = melfas_ts->info[0].max_y ;

#ifndef PRODUCT_SHIP 
	printk("melfas_ts_probe: max_x: %d, max_y: %d\n", max_x, max_y);
#endif

	melfas_ts->input_dev = input_allocate_device();
	if (melfas_ts->input_dev == NULL) {
		ret = -ENOMEM;
		#ifndef PRODUCT_SHIP
		printk(KERN_ERR "melfas_ts_probe: Failed to allocate input device\n");
		#endif
		goto err_input_dev_alloc_failed;
	}

	melfas_ts->input_dev->name = "melfas_ts_input";

	set_bit(EV_SYN, melfas_ts->input_dev->evbit);
	set_bit(EV_KEY, melfas_ts->input_dev->evbit);
	set_bit(TOUCH_HOME, melfas_ts->input_dev->keybit);
	set_bit(TOUCH_MENU, melfas_ts->input_dev->keybit);
	set_bit(TOUCH_BACK, melfas_ts->input_dev->keybit);
	set_bit(TOUCH_SEARCH, melfas_ts->input_dev->keybit);

	melfas_ts->input_dev->keycode = melfas_ts_tk_keycode;
	set_bit(BTN_TOUCH, melfas_ts->input_dev->keybit);
	set_bit(EV_ABS, melfas_ts->input_dev->evbit);

	input_set_abs_params(melfas_ts->input_dev, ABS_MT_POSITION_X,  0, max_x, 0, 0);
	input_set_abs_params(melfas_ts->input_dev, ABS_MT_POSITION_Y,  0, max_y, 0, 0);
	input_set_abs_params(melfas_ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(melfas_ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 30, 0, 0);
	input_set_abs_params(melfas_ts->input_dev, ABS_MT_TRACKING_ID, 0, 4, 0, 0);

	ret = input_register_device(melfas_ts->input_dev);
	if (ret) {
		#ifndef PRODUCT_SHIP 
		printk(KERN_ERR "melfas_ts_probe: Unable to register %s input device\n", melfas_ts->input_dev->name);
		#endif
		goto err_input_register_device_failed;
	}

	melfas_ts->irq = melfas_ts->client->irq;
	//ret = request_irq(melfas_ts->client->irq, melfas_ts_irq_handler, IRQF_DISABLED, "melfas_ts irq", 0);
	ret = request_threaded_irq(melfas_ts->client->irq, NULL, melfas_ts_irq_handler,IRQF_ONESHOT,"melfas_ts irq", 0);
	if(ret == 0) {
		#ifndef PRODUCT_SHIP 
		printk(KERN_INFO "melfas_ts_probe: Start touchscreen %s \n", melfas_ts->input_dev->name);
		#endif
	}
	else {
		printk("request_irq failed\n");
	}


	#if defined(CONFIG_MACH_CHIEF)
		set_tsp_noise_filter_reg();
	#endif


#ifdef CONFIG_HAS_EARLYSUSPEND
	melfas_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	melfas_ts->early_suspend.suspend = melfas_ts_early_suspend;
	melfas_ts->early_suspend.resume = melfas_ts_late_resume;
	register_early_suspend(&melfas_ts->early_suspend);
#endif	/* CONFIG_HAS_EARLYSUSPEND */

#ifdef LCD_WAKEUP_PERFORMANCE
	/* for synchronous ts operations */
	complete(&ts_completion);
#endif

	return 0;
err_misc_register_device_failed:
err_input_register_device_failed:
	input_free_device(melfas_ts->input_dev);

err_input_dev_alloc_failed:
err_detect_failed:
	kfree(melfas_ts);
err_alloc_data_failed:
err_check_functionality_failed:
	return ret;

}

int melfas_ts_remove(struct i2c_client *client)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&melfas_ts->early_suspend);
#endif	/* CONFIG_HAS_EARLYSUSPEND */
	free_irq(melfas_ts->irq, 0);
	input_unregister_device(melfas_ts->input_dev);
	return 0;
}

int melfas_ts_gen_touch_up(void)
{
        // report up key if needed
        int i;
        int j;
        int finger = 0;
        int x = 0;
        int y = 0;
        int z = 0;

        for ( i= 1; i<FINGER_NUM+1; ++i ) {
                if(melfas_ts->info[i].state == 0x1) { /*down state*/
                       finger = melfas_ts->info[i].finger_id;
                       x = melfas_ts->info[i].x;
                       y = melfas_ts->info[i].y;
                       z = melfas_ts->info[i].z;
					#ifndef PRODUCT_SHIP 
                       printk("[TOUCH_GEN] idx (%d) DOWN KEY x: %4d, y: %4d, z: %4d, finger: %4d\n", i, x, y, z,finger);
					#endif
                       input_report_abs(melfas_ts->input_dev, ABS_MT_POSITION_X,  x);
                       input_report_abs(melfas_ts->input_dev, ABS_MT_POSITION_Y,  y);
                       input_report_abs(melfas_ts->input_dev, ABS_MT_TOUCH_MAJOR, z);		
                       input_report_abs(melfas_ts->input_dev, ABS_MT_TRACKING_ID, finger-1);
                       input_mt_sync(melfas_ts->input_dev);
                }
                input_sync(melfas_ts->input_dev);
        }

        for ( j= 1; j<FINGER_NUM+1; ++j ) {
                if(melfas_ts->info[j].state == 0x1) { /*down state*/
                        finger = melfas_ts->info[j].finger_id;
                        x = melfas_ts->info[j].x;
                        y = melfas_ts->info[j].y;
                        z = 0;
					#ifndef PRODUCT_SHIP 
                        printk("[TOUCH_GEN] idx (%d) UP   KEY x: %4d, y: %4d, z: %4d, finger: %4d\n", j, x, y, z,finger);
					#endif
                        input_report_abs(melfas_ts->input_dev, ABS_MT_POSITION_X,  x);
                        input_report_abs(melfas_ts->input_dev, ABS_MT_POSITION_Y,  y);
                        input_report_abs(melfas_ts->input_dev, ABS_MT_TOUCH_MAJOR, z);
                        input_report_abs(melfas_ts->input_dev, ABS_MT_TRACKING_ID, finger-1);
                        input_mt_sync(melfas_ts->input_dev);

                        melfas_ts->info[j].state = 0x0;
                        melfas_ts->info[j].z = -1;
                }
                input_sync(melfas_ts->input_dev);
        }
}

int melfas_ts_suspend(pm_message_t mesg)
{
#ifdef LCD_WAKEUP_PERFORMANCE
    /* for synchronous ts operations, wait until the ts resume work is complete */
    wait_for_completion(&ts_completion);
#endif

    melfas_ts->suspended = true;

#ifdef TSP_TEST_MODE
    touch_screen.device_state = false; 
#endif
    melfas_ts_gen_touch_up();
    disable_irq(melfas_ts->irq);
	
    gpio_tlmm_config(GPIO_CFG(GPIO_I2C0_SCL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),GPIO_CFG_ENABLE);
    gpio_tlmm_config(GPIO_CFG(GPIO_I2C0_SDA, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),GPIO_CFG_ENABLE);

    mcsdl_vdd_off();
    gpio_set_value(GPIO_I2C0_SCL, 0);  // TOUCH SCL DIS
    gpio_set_value(GPIO_I2C0_SDA, 0);  // TOUCH SDA DIS

    return 0;
}

#ifdef LCD_WAKEUP_PERFORMANCE
static void ts_resume_work_func(struct work_struct *ignored)
{
    melfas_ts->suspended = false;

#ifdef TSP_TEST_MODE
    touch_screen.device_state = true; 
#endif
    enable_irq(melfas_ts->irq);

#if defined(CONFIG_MACH_CHIEF)
    set_tsp_noise_filter_reg();
#endif

    /* we should always call complete, otherwise the caller will be deadlocked */
    complete(&ts_completion);
}
#endif

int melfas_ts_resume(void)
{
    gpio_tlmm_config(GPIO_CFG(GPIO_I2C0_SCL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA),GPIO_CFG_ENABLE);
    gpio_tlmm_config(GPIO_CFG(GPIO_I2C0_SDA, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA),GPIO_CFG_ENABLE);
    gpio_set_value(GPIO_I2C0_SCL, 1);  // TOUCH SCL EN
    gpio_set_value(GPIO_I2C0_SDA, 1);  // TOUCH SDA EN

    mcsdl_vdd_on();

#ifdef LCD_WAKEUP_PERFORMANCE
    schedule_delayed_work(&ts_resume_work, msecs_to_jiffies(200));
#else
    mdelay(200); /* melfas's recommendation */
    printk(KERN_INFO "%s: vdd on with 200 msec.\n", __func__);

    melfas_ts->suspended = false;

#ifdef TSP_TEST_MODE
    touch_screen.device_state = true; 
#endif
    enable_irq(melfas_ts->irq);

#if defined(CONFIG_MACH_CHIEF)
    set_tsp_noise_filter_reg();
#endif
#endif

    return 0;
}

int tsp_preprocess_suspend(void)
{
#if 0 // blocked for now.. we will gen touch when suspend func is called
  // this function is called before kernel calls suspend functions
  // so we are going suspended if suspended==false
  if(melfas_ts->suspended == false) {
    // fake as suspended
    melfas_ts->suspended = true;

    //generate and report touch event
    melfas_ts_gen_touch_up();
  }
#endif
  return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
void melfas_ts_early_suspend(struct early_suspend *h)
{
	melfas_ts_suspend(PMSG_SUSPEND);
}

void melfas_ts_late_resume(struct early_suspend *h)
{
	melfas_ts_resume();
}
#endif	/* CONFIG_HAS_EARLYSUSPEND */


int melfas_i2c_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	melfas_ts->client = client;
	i2c_set_clientdata(client, melfas_ts);
	return 0;
}

static int __devexit melfas_i2c_remove(struct i2c_client *client)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&melfas_ts->early_suspend);
#endif  /* CONFIG_HAS_EARLYSUSPEND */
	free_irq(melfas_ts->client->irq, 0);
	input_unregister_device(melfas_ts->input_dev);

	melfas_ts = i2c_get_clientdata(client);
	kfree(melfas_ts);
	return 0;
}

struct i2c_device_id melfas_id[] = {
	{ "melfas_ts_i2c", 0 },
	{ }
};

struct i2c_driver melfas_ts_i2c = {
	.driver = {
		.name	= "melfas_ts_i2c",
		.owner	= THIS_MODULE,
	},
	.probe 		= melfas_i2c_probe,
	.remove		= __devexit_p(melfas_i2c_remove),
	.id_table	= melfas_id,
};


void init_hw_setting(void)
{
	mcsdl_vdd_on();
	mdelay(100);

	gpio_tlmm_config(GPIO_CFG(GPIO_I2C0_SCL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA),GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_I2C0_SDA, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA),GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_TOUCH_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA),GPIO_CFG_ENABLE);

	set_irq_type(IRQ_TOUCH_INT, IRQ_TYPE_LEVEL_LOW); //chief.boot.temp changed from edge low to level low VERIFY!!!

	mdelay(10);

}

struct platform_driver melfas_ts_driver =  {
	.probe	= melfas_ts_probe,
	.remove = melfas_ts_remove,
	.driver = {
		.name = "melfas-ts",
		.owner	= THIS_MODULE,
	},
};

int __init melfas_ts_init(void)
{
	int ret;
#ifndef PRODUCT_SHIP 	
	printk("\n====================================================");
	printk("\n=======         [TOUCH SCREEN] INIT        =========");
	printk("\n====================================================\n");
#endif
	init_hw_setting();

	ts_dev = device_create(sec_class, NULL, 0, NULL, "ts");
	if (IS_ERR(ts_dev))
		pr_err("Failed to create device(ts)!\n");
	if (device_create_file(ts_dev, &dev_attr_gpio) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_gpio.attr.name);
	if (device_create_file(ts_dev, &dev_attr_registers) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_registers.attr.name);
	if (device_create_file(ts_dev, &dev_attr_firmware) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_firmware.attr.name);
	if (device_create_file(ts_dev, &dev_attr_debug) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_debug.attr.name);
	
#ifdef TSP_TEST_MODE
	if (device_create_file(ts_dev, &dev_attr_tsp_name) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_tsp_name.attr.name);	
	if (device_create_file(ts_dev, &dev_attr_tsp_test) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_tsp_test.attr.name);	
	if (device_create_file(ts_dev, &dev_attr_tsp_reference) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_tsp_reference.attr.name);
	if (device_create_file(ts_dev, &dev_attr_tsp_inspection) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_tsp_inspection.attr.name);
	if (device_create_file(ts_dev, &dev_attr_tsp_delta) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_tsp_delta.attr.name);
	if (device_create_file(ts_dev, &dev_attr_tsp_sleep) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_tsp_sleep.attr.name);
	if (device_create_file(ts_dev, &dev_attr_tsp_wakeup) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_tsp_wakeup.attr.name);
#endif	

	melfas_ts = kzalloc(sizeof(struct melfas_ts_driver), GFP_KERNEL);
	if(melfas_ts == NULL) {
		return -ENOMEM;
	}
#ifdef TSP_TEST_MODE
	mutex_init(&melfas_ts->lock);
#endif

	ret = i2c_add_driver(&melfas_ts_i2c);
	if(ret) printk("[%s], i2c_add_driver failed...(%d)\n", __func__, ret);

	if(!melfas_ts->client) {
		printk("###################################################\n");
		printk("##                                               ##\n");
		printk("##    WARNING! TOUCHSCREEN DRIVER CAN'T WORK.    ##\n");
		printk("##    PLEASE CHECK YOUR TOUCHSCREEN CONNECTOR!   ##\n");
		printk("##                                               ##\n");
		printk("###################################################\n");
		i2c_del_driver(&melfas_ts_i2c);
		return 0;
	}
	melfas_ts_wq = create_singlethread_workqueue("melfas_ts_wq");
	if (!melfas_ts_wq)
		return -ENOMEM;

	return platform_driver_register(&melfas_ts_driver);

}

void __exit melfas_ts_exit(void)
{
	i2c_del_driver(&melfas_ts_i2c);
	if (melfas_ts_wq)
		destroy_workqueue(melfas_ts_wq);
}
late_initcall(melfas_ts_init);
//module_init(melfas_ts_init);
module_exit(melfas_ts_exit);

MODULE_DESCRIPTION("Melfas Touchscreen Driver");
MODULE_LICENSE("GPL");

