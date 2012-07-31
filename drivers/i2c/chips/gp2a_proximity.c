/*
 * Copyright (c) 2010 SAMSUNG
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <mach/hardware.h>
#include <linux/wakelock.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <asm/uaccess.h>
#include <linux/mfd/pmic8058.h>
#include <linux/i2c/gp2a.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#if defined(CONFIG_MACH_CHIEF)
#define __HEADSET_NOISE_HW_BUG_WORKAROUND__
#endif

/*********** for debug **********************************************************/
#if 1
#define gprintk(fmt, x... ) printk( "%s(%d): " fmt, __FUNCTION__ ,__LINE__, ## x)
#else
#define gprintk(x...) do { } while (0)
#endif
/*******************************************************************************/ 
#if defined CONFIG_MACH_CHIEF
#define IRQ_GP2A_INT	((system_rev>=5)?PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, (PM8058_GPIO(30))):PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, (PM8058_GPIO(14))))
#define GPIO_PS_VOUT	((system_rev>=5)?PM8058_GPIO_PM_TO_SYS(PM8058_GPIO(30)):PM8058_GPIO_PM_TO_SYS(PM8058_GPIO(14)))
#endif

#if defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || defined(CONFIG_MACH_PREVAIL2)
#define IRQ_GP2A_INT	PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, (PM8058_GPIO(30)))
#define GPIO_PS_VOUT	PM8058_GPIO_PM_TO_SYS(PM8058_GPIO(30))
#endif

#define SENSOR_DEFAULT_DELAY            (200)   /* 200 ms */
#define SENSOR_MAX_DELAY                (2000)  /* 2000 ms */
#define ABS_STATUS                      (ABS_BRAKE)
#define ABS_WAKE                        (ABS_MISC)
#define ABS_CONTROL_REPORT              (ABS_THROTTLE)

/* global var */
static struct wake_lock prx_wake_lock;

static struct i2c_driver opt_i2c_driver;
static struct i2c_client *opt_i2c_client = NULL;
static struct gp2a_platform_data *gp2a_pdata;
int proximity_enabled;

#ifdef USE_MODE_B
static char mvo_value; // Master's VO
static char proximity_value; // Local proximity value
#endif
	
/* driver data */
struct gp2a_data {
	struct input_dev *input_dev;
	struct delayed_work work;  /* for proximity sensor */
	struct mutex enable_mutex;
	struct mutex data_mutex;

	int   enabled;
	int   delay;
	int   prox_data;
	int   irq;
#ifdef __HEADSET_NOISE_HW_BUG_WORKAROUND__
	int   headset_connected;
	int   fake_enable;
	int   debug_irq_nest;
#endif

  	struct kobject *uevent_kobj;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct platform_device *pdev;
	struct early_suspend	early_suspend;
#endif
};


static struct gp2a_data *prox_data = NULL;

struct opt_state{
	struct i2c_client	*client;
};

struct opt_state *opt_state;

static int proximity_onoff(u8 onoff);

#ifdef CONFIG_HAS_EARLYSUSPEND
void gp2a_opt_early_suspend(struct early_suspend *h);
void gp2a_opt_late_resume(struct early_suspend *h);
#endif


/* Proximity Sysfs interface */
static ssize_t
proximity_delay_show(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    struct input_dev *input_data = to_input_dev(dev);
    struct gp2a_data *data = input_get_drvdata(input_data);
    int delay;

    delay = data->delay;

    return sprintf(buf, "%d\n", delay);
}

static ssize_t
proximity_delay_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf,
        size_t count)
{
    struct input_dev *input_data = to_input_dev(dev);
    struct gp2a_data *data = input_get_drvdata(input_data);
    int delay = simple_strtoul(buf, NULL, 10);

    if (delay < 0) {
        return count;
    }

    if (SENSOR_MAX_DELAY < delay) {
        delay = SENSOR_MAX_DELAY;
    }

    data->delay = delay;

	mutex_lock(&data->enable_mutex);

	if( data->enabled )
	{
		cancel_delayed_work_sync(&data->work);
		schedule_delayed_work(&data->work, msecs_to_jiffies(delay));
	}

    input_report_abs(input_data, ABS_CONTROL_REPORT, (data->delay<<16) | delay);

	mutex_unlock(&data->enable_mutex);

    return count;
}

static ssize_t
proximity_enable_show(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    struct input_dev *input_data = to_input_dev(dev);
    struct gp2a_data *data = input_get_drvdata(input_data);
    int enabled;

    enabled = data->enabled;

    return sprintf(buf, "%d\n", enabled);
}

static ssize_t
proximity_enable_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf,
        size_t count)
{
    struct input_dev *input_data = to_input_dev(dev);
    struct gp2a_data *data = input_get_drvdata(input_data);
    int value = simple_strtoul(buf, NULL, 10);

    if (value != 0 && value != 1) {
        return count;
    }

    mutex_lock(&data->enable_mutex);

    if (data->enabled && !value) { 			/* Proximity power off */
		proximity_enabled = 0;
#ifdef	__HEADSET_NOISE_HW_BUG_WORKAROUND__
		if(!data->fake_enable) {
			disable_irq(IRQ_GP2A_INT);
			data->debug_irq_nest--;
			cancel_delayed_work_sync(&data->work);
			proximity_onoff(0);
			pr_err("%s: proxmity disabled!\n", __func__);
		} else {
			pr_err("%s: proxmity disable skipped because it was in a fake enable state\n", __func__);
		}
		data->fake_enable = 0;
#else
		disable_irq(IRQ_GP2A_INT);
		cancel_delayed_work_sync(&data->work);
		proximity_onoff(0);
		pr_err("%s: proxmity disabled!\n", __func__);
#endif
    }

    if (!data->enabled && value) {			/* proximity power on */
		proximity_enabled = 1;
#ifdef	__HEADSET_NOISE_HW_BUG_WORKAROUND__
		if(data->headset_connected) {
			data->fake_enable = 1;
			pr_err("%s: proxmity not enabled because headset is connected!\n", __func__);
		} else {
			proximity_onoff(1);
	#ifdef USE_MODE_B
			enable_irq(IRQ_GP2A_INT);
	#else
			schedule_delayed_work(&data->work, 0);
			enable_irq(IRQ_GP2A_INT);
	#endif

			data->debug_irq_nest++;
			data->fake_enable = 0;
			pr_err("%s: proxmity enabled!\n", __func__);
		}
#else
		proximity_onoff(1);
	#ifdef USE_MODE_B
		enable_irq(IRQ_GP2A_INT);
	#else
		schedule_delayed_work(&data->work, 0);
		enable_irq(IRQ_GP2A_INT);
	#endif
#endif
    }
    data->enabled = value;

    input_report_abs(input_data, ABS_CONTROL_REPORT, (value<<16) | data->delay);

    mutex_unlock(&data->enable_mutex);

    return count;
}

static ssize_t
proximity_wake_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf,
        size_t count)
{
    struct input_dev *input_data = to_input_dev(dev);
    static int cnt = 1;

    input_report_abs(input_data, ABS_WAKE, cnt++);

    return count;
}

static ssize_t
proximity_data_show(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct gp2a_data *data = input_get_drvdata(input_data);
	int x;

	mutex_lock(&data->data_mutex);
#ifdef USE_MODE_B
	x = proximity_value;
#else
	x = data->prox_data;
#endif
	mutex_unlock(&data->data_mutex);

    return sprintf(buf, "%d\n", x);
}

#if defined(__HEADSET_NOISE_HW_BUG_WORKAROUND__)
static ssize_t
proximity_headset_connected_show(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    struct input_dev *input_data = to_input_dev(dev);
    struct gp2a_data *data = input_get_drvdata(input_data);
	
	pr_info("%s: headset_connected=%d, fake_enable=%d debug_irq_nest=%d\n", __func__, data->headset_connected, data->fake_enable, data->debug_irq_nest);
	
    return sprintf(buf, "%d\n", data->headset_connected);
}

static ssize_t
proximity_headset_connected_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf,
        size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct gp2a_data *data = input_get_drvdata(input_data);
	int value = simple_strtoul(buf, NULL, 10);

	if (value != 0 && value != 1) {
		return count;
	}

	mutex_lock(&data->enable_mutex);

	data->headset_connected = value;

	pr_info("%s: headset status : %d\n", __func__, data->headset_connected);
	
	if(data->fake_enable == 1 && data->headset_connected == 0 && data->enabled == 1) {
		// headset is disconnected and proximity sensor was enabled
		pr_info("%s: fake_enable -> enable\n", __func__);
		proximity_onoff(1);
#ifdef USE_MODE_B
		enable_irq(IRQ_GP2A_INT);
#else
		schedule_delayed_work(&data->work, 0);
		enable_irq(IRQ_GP2A_INT);
#endif
		data->debug_irq_nest++;
		data->fake_enable = 0;
	} else if(data->headset_connected == 1 && data->fake_enable == 0 && data->enabled == 1) {
		// headset is connected after proximity sensor was enabled
		pr_info("%s: enable -> fake_enable\n", __func__);
		disable_irq(IRQ_GP2A_INT);
		data->debug_irq_nest--;
		cancel_delayed_work_sync(&data->work);
		proximity_onoff(0);
		data->fake_enable = 1;
	}

    mutex_unlock(&data->enable_mutex);

    return count;
}
#endif

static DEVICE_ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP,
        proximity_delay_show, proximity_delay_store);
static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP,
        proximity_enable_show, proximity_enable_store);
static DEVICE_ATTR(wake, S_IWUSR|S_IWGRP,
        NULL, proximity_wake_store);
static DEVICE_ATTR(data, S_IRUGO, proximity_data_show, NULL);
#if defined(__HEADSET_NOISE_HW_BUG_WORKAROUND__)
static DEVICE_ATTR(headset_connected, S_IRUGO|S_IWUSR|S_IWGRP,
        proximity_headset_connected_show, proximity_headset_connected_store);
#endif

static struct attribute *proximity_attributes[] = {
    &dev_attr_delay.attr,
    &dev_attr_enable.attr,
    &dev_attr_wake.attr,
    &dev_attr_data.attr,
#if defined(__HEADSET_NOISE_HW_BUG_WORKAROUND__)
    &dev_attr_headset_connected.attr,
#endif
    NULL
};
 
static struct attribute_group proximity_attribute_group = {
    .attrs = proximity_attributes
};


static char get_ps_vout_value(void)
{
  char value = 0;
  unsigned char int_val;

#if defined CONFIG_MACH_CHIEF || defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || \
	defined(CONFIG_MACH_PREVAIL2)
#ifdef USE_MODE_B
  int_val = REGS_PROX;
  opt_i2c_read((u8)(int_val), &value, 1);
#else
  value = gpio_get_value_cansleep(GPIO_PS_VOUT);
#endif
#else
  value = gpio_get_value(GPIO_PS_VOUT);
#endif

  return value;
}
/*****************************************************************************************
 *
 *  function    : gp2a_work_func_prox
 *  description : This function is for proximity sensor (using B-1 Mode ).
 *                when INT signal is occured , it gets value from VO register.
 *
 *
 */
static void gp2a_work_func_prox(struct work_struct *work)
{
	struct gp2a_data *gp2a = container_of((struct delayed_work *)work,
							struct gp2a_data, work);
	char value;

#ifdef USE_MODE_B
	// VOUT terminal changes fro "H" to "L"
	value = 0x10;
	opt_i2c_write((u8)(REGS_CON),&value);
	
	disable_irq(IRQ_GP2A_INT);
	
	// mvo_value : 1 bit (odd or even)
	// Write HYS register
	if(!mvo_value) // detect
	{
		mvo_value = 1;
		value = 0x20;
		opt_i2c_write((u8)(REGS_HYS),&value);
	}
	else // nodetect
	{
		mvo_value = 0;
		value = 0x40;
		opt_i2c_write((u8)(REGS_HYS),&value);
	}

	// address:6 data:0x18 forcing Vout to go H
	value = 0x18;
	opt_i2c_write((u8)(REGS_CON),&value);

	// Read the VO value ( buf[1]'s LSB is the VO.)
	value = 0x01 & get_ps_vout_value();
	
	if(mvo_value == value)
	{
		// converting : detect = 0, nodetect = 1
		if(value)
			value = 0;
		else
			value = 1;

		printk(KERN_INFO "%s value=%d",__func__,value);
		
		input_report_abs(gp2a->input_dev, ABS_X, value);
	    input_sync(gp2a->input_dev);

	    if(value == 1)
	      kobject_uevent(gp2a->uevent_kobj, KOBJ_OFFLINE);
	    else if(value == 0)
	      kobject_uevent(gp2a->uevent_kobj, KOBJ_ONLINE);

		gp2a->prox_data= value;
		proximity_value = value;

		enable_irq(IRQ_GP2A_INT);

		value = 0x00;
		opt_i2c_write((u8)(REGS_CON),&value);
	}
	else
	{
		gprintk(KERN_INFO "mvo_value = %d , value = %d\n", mvo_value, value);
		gprintk(KERN_INFO "mvo_value != value , need to reset\n");
		value = 0x02;
		opt_i2c_write((u8)(REGS_OPMOD),&value);
		mvo_value = 0;
		value = 0x03;
		opt_i2c_write((u8)(REGS_OPMOD),&value);
		proximity_onoff(1);
		enable_irq(IRQ_GP2A_INT);
		schedule_delayed_work(&gp2a->work, msecs_to_jiffies(gp2a->delay));
	}
	#else
	value = get_ps_vout_value();
    input_report_abs(gp2a->input_dev, ABS_X,  value);
    input_sync(gp2a->input_dev);

    if(value == 1)
      kobject_uevent(gp2a->uevent_kobj, KOBJ_OFFLINE);
    else if(value == 0)
      kobject_uevent(gp2a->uevent_kobj, KOBJ_ONLINE);

	gp2a->prox_data= value;

	schedule_delayed_work(&gp2a->work, msecs_to_jiffies(gp2a->delay));
#endif
}

#ifdef USE_MODE_B
static void gp2a_forced_workqueue(void)
{
	char value;
	struct gp2a_data *gp2a = prox_data;
	
	// VOUT terminal changes fro "H" to "L"
	value = 0x10;
	opt_i2c_write((u8)(REGS_CON),&value);
	
	disable_irq(IRQ_GP2A_INT);
	
	// mvo_value : 1 bit (odd or even)
	// Write HYS register
	if(!mvo_value) // detect
	{
		mvo_value = 1;
		value = 0x20;
		opt_i2c_write((u8)(REGS_HYS),&value);
	}
	else // nodetect
	{
		mvo_value = 0;
		value = 0x40;
		opt_i2c_write((u8)(REGS_HYS),&value);
	}

	// address:6 data:0x18 forcing Vout to go H
	value = 0x18;
	opt_i2c_write((u8)(REGS_CON),&value);

	// Read the VO value ( buf[1]'s LSB is the VO.)
	value = 0x01 & get_ps_vout_value();
	
	if(mvo_value == value)
	{
		// converting : detect = 0, nodetect = 1
		if(value)
			value = 0;
		else
			value = 1;

		printk(KERN_INFO "%s value=%d\n",__func__,value);

		input_report_abs(gp2a->input_dev, ABS_X, value);
		input_sync(gp2a->input_dev);

		if(value == 1)
			kobject_uevent(gp2a->uevent_kobj, KOBJ_OFFLINE);
		else if(value == 0)
			kobject_uevent(gp2a->uevent_kobj, KOBJ_ONLINE);

		gp2a->prox_data= value;
		proximity_value = value;

		enable_irq(IRQ_GP2A_INT);

		value = 0x00;
		opt_i2c_write((u8)(REGS_CON),&value);
	}
	else
	{
		gprintk(KERN_INFO "mvo_value = %d , value = %d\n", mvo_value, value);
		gprintk(KERN_INFO "mvo_value != value , need to reset\n");
		value = 0x02;
		opt_i2c_write((u8)(REGS_OPMOD),&value);
		mvo_value = 0;
		value = 0x03;
		opt_i2c_write((u8)(REGS_OPMOD),&value);
		proximity_onoff(1);
		enable_irq(IRQ_GP2A_INT);
		//schedule_delayed_work(&gp2a->work, msecs_to_jiffies(gp2a->delay));
	}
}
#endif


irqreturn_t gp2a_irq_handler(int irq, void *dev_id)
{
	char value;

#ifdef USE_MODE_B
	cancel_delayed_work_sync(&prox_data->work);
	wake_lock_timeout(&prx_wake_lock, 3*HZ);
	schedule_delayed_work(&prox_data->work, msecs_to_jiffies(prox_data->delay));
	gprintk(KERN_INFO "[PROXIMITY] IRQ_HANDLED ! \n");
#else
	value = get_ps_vout_value();

	cancel_delayed_work_sync(&prox_data->work);
	wake_lock_timeout(&prx_wake_lock, 3*HZ);

	input_report_abs(prox_data->input_dev, ABS_DISTANCE,  value);
	input_sync(prox_data->input_dev);

	if(value == 1)
		kobject_uevent(prox_data->uevent_kobj, KOBJ_OFFLINE);
	else if(value == 0)
		kobject_uevent(prox_data->uevent_kobj, KOBJ_ONLINE);

	prox_data->prox_data = value;

	schedule_delayed_work(&prox_data->work, msecs_to_jiffies(prox_data->delay));
	gprintk(KERN_INFO "[PROXIMITY] IRQ_HANDLED ! (value : %d)\n", value);
#endif

	return IRQ_HANDLED;
}

static int opt_i2c_init(void)
{
	if( i2c_add_driver(&opt_i2c_driver))
	{
		gprintk(KERN_ERR "i2c_add_driver failed\n");
		return -ENODEV;
	}
	return 0;
}


int opt_i2c_read(u8 reg, u8 *val, unsigned int len )
{
	int err;
	u8 buf[2];
	struct i2c_msg msg[1];

	buf[0] = reg;

	msg[0].addr = opt_i2c_client->addr;
	msg[0].flags = 1;

	msg[0].len = 2;
	msg[0].buf = buf;
	err = i2c_transfer(opt_i2c_client->adapter, msg, 1);
	//gprintk(KERN_INFO "i2c read test : %d\n", err);

	// buf[0] : Dummy byte
	*val = buf[1];

	if (err >= 0) return 0;

	gprintk(KERN_INFO "%s %d i2c transfer error\n", __func__, __LINE__);
	return err;
}

int opt_i2c_write( u8 reg, u8 *val )
{
    int err = 0;
    struct i2c_msg msg[1];
    unsigned char data[2];
    int retry = 10;

    if( (opt_i2c_client == NULL) || (!opt_i2c_client->adapter) ){
        return -ENODEV;
    }

    while(retry--)
    {
        data[0] = reg;
        data[1] = *val;

        msg->addr = opt_i2c_client->addr;
        msg->flags = I2C_M_WR;
        msg->len = 2;
        msg->buf = data;

        err = i2c_transfer(opt_i2c_client->adapter, msg, 1);
        //gprintk(KERN_INFO "i2c write test : %d reg:%d value:%d\n", err, reg, *val);

        if (err >= 0) return 0;
    }
    gprintk(KERN_ERR "%s i2c transfer (%d) error! reg:%d, val:%d\n", __func__, err, reg, *val);
    return err;
}

static int proximity_input_init(struct gp2a_data *data)
	{
	struct input_dev *dev;
	int err;

	dev = input_allocate_device();
	if (!dev) {
		return -ENOMEM;
	}

	set_bit(EV_ABS, dev->evbit);
	input_set_capability(dev, EV_ABS, ABS_X);

	input_set_capability(dev, EV_ABS, ABS_STATUS); /* status */
	input_set_capability(dev, EV_ABS, ABS_WAKE); /* wake */
	input_set_capability(dev, EV_ABS, ABS_CONTROL_REPORT); /* enabled/delay */

	dev->name = "proximity";
	input_set_drvdata(dev, data);

	err = input_register_device(dev);
	if (err < 0) {
		input_free_device(dev);
		return err;
	}
	data->input_dev = dev;

	return 0;
}
 
static int gp2a_opt_probe( struct platform_device* pdev )
{
    struct gp2a_data *gp2a;
    u8 value;
    int err = 0;
	gprintk(KERN_INFO "gp2a_opt_probe proximity is start \n");
	/* allocate driver_data */
	gp2a = kzalloc(sizeof(struct gp2a_data),GFP_KERNEL);
	if(!gp2a)
	{
		pr_err("kzalloc couldn't allocate memory\n");
		return -ENOMEM;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	gp2a->pdev = pdev;
#endif
	gp2a->enabled = 0;
	gp2a->delay = SENSOR_DEFAULT_DELAY;
	
	/* Local value initialize */
#ifdef USE_MODE_B
	mvo_value = 0;
	proximity_value = 1;
#endif

	prox_data = gp2a;

	mutex_init(&gp2a->enable_mutex);
	mutex_init(&gp2a->data_mutex);

	INIT_DELAYED_WORK(&gp2a->work, gp2a_work_func_prox);
	gprintk(KERN_INFO "gp2a_opt_probe proximity regiter workfunc!!\n");

	err = proximity_input_init(gp2a);
	if(err < 0) {
		goto error_1;
	}
	err = sysfs_create_group(&gp2a->input_dev->dev.kobj,
				&proximity_attribute_group);
	if(err < 0)
	{
		goto error_2;
	}

	/* set platdata */
	platform_set_drvdata(pdev, gp2a);

	gp2a->uevent_kobj = &pdev->dev.kobj;

	/* wake lock init */
	wake_lock_init(&prx_wake_lock, WAKE_LOCK_SUSPEND, "prx_wake_lock");

	/* init i2c */
	opt_i2c_init();

	if(opt_i2c_client == NULL)
	{
		pr_err("opt_probe failed : i2c_client is NULL\n");
		return -ENODEV;
	}
	else
		gprintk(KERN_INFO "opt_i2c_client : (0x%p)\n",opt_i2c_client);

	/* GP2A Regs INIT SETTINGS */
	value = 0x03;
	opt_i2c_write((u8)(REGS_OPMOD),&value);

	/* INT Settings */
  	err = request_threaded_irq( IRQ_GP2A_INT ,
		NULL, gp2a_irq_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "proximity_int", gp2a);

	if (err < 0) {
		gprintk(KERN_ERR "failed to request proximity_irq\n");
		goto error_2;
	}

	device_init_wakeup(&pdev->dev, gp2a_pdata->wakeup);

#ifdef CONFIG_HAS_EARLYSUSPEND
	gp2a->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	gp2a->early_suspend.suspend = gp2a_opt_early_suspend;
	gp2a->early_suspend.resume = gp2a_opt_late_resume;
	register_early_suspend(&gp2a->early_suspend);
#endif

	disable_irq(IRQ_GP2A_INT);

#ifdef USE_MODE_B
	/* set initial proximity value as 1 */
	input_report_abs(gp2a->input_dev, ABS_X, proximity_value);
	input_sync(gp2a->input_dev);
#endif

	gprintk(KERN_INFO "gp2a_opt_probe proximity is OK!!\n");

	return 0;

error_2:
	input_unregister_device(gp2a->input_dev);
	input_free_device(gp2a->input_dev);
error_1:
	kfree(gp2a);
	return err;
}

static int gp2a_opt_remove( struct platform_device* pdev )
{
	struct gp2a_data *gp2a = platform_get_drvdata(pdev);

	if (gp2a->input_dev!= NULL) {
		sysfs_remove_group(&gp2a->input_dev->dev.kobj, &proximity_attribute_group);
		input_unregister_device(gp2a->input_dev);
		if (gp2a->input_dev != NULL) {
			kfree(gp2a->input_dev);
		}
	}

	kfree(gp2a);

	return 0;
}

static int gp2a_opt_suspend( struct platform_device* pdev )
{
	struct gp2a_data *gp2a = platform_get_drvdata(pdev);
	char value;
	gprintk(KERN_INFO "enabled : %d\n", gp2a->enabled);

	if(gp2a->enabled)
	{
		#ifdef USE_MODE_B
			if(cancel_delayed_work_sync(&gp2a->work)){ //do not turn off proximity sensor.
				gp2a_forced_workqueue();
				printk(KERN_INFO "%s work_queue_is_pending\n",__func__);
			}
		#else
			cancel_delayed_work_sync(&gp2a->work); //do not turn off proximity sensor.
		#endif
		
		//execution gp2a_work_function to defence uevent error
		{
			#ifdef USE_MODE_B
			value = proximity_value;
			#else
			value = get_ps_vout_value();
			#endif

			input_report_abs(gp2a->input_dev, ABS_X,  value);
			input_sync(gp2a->input_dev);

			if(value == 1)
				kobject_uevent(gp2a->uevent_kobj, KOBJ_OFFLINE);
			else if(value == 0)
				kobject_uevent(gp2a->uevent_kobj, KOBJ_ONLINE);

			gp2a->prox_data= value;

			gprintk(KERN_INFO "%s get_ps_vout_value: %d\n",__func__,value);
		}

		if (device_may_wakeup(&pdev->dev)) {
			enable_irq_wake(IRQ_GP2A_INT);
			gprintk(KERN_INFO "enable IRQ_GP2A_INT : %d\n", IRQ_GP2A_INT);
		}
	}
	else
	{
		//proximity_onoff(0); //turn off
		//gprintk(KERN_INFO "The proximity sensor is off\n");
	}
	return 0;
}

static int gp2a_opt_resume( struct platform_device* pdev )
{
	struct gp2a_data *gp2a = platform_get_drvdata(pdev);
	
	gprintk(KERN_INFO "enabled : %d\n", gp2a->enabled);

	if(gp2a->enabled)
	{
		if (device_may_wakeup(&pdev->dev)) {
			disable_irq_wake(IRQ_GP2A_INT);
			gprintk(KERN_INFO "disable IRQ_GP2A_INT\n");
		}
#ifndef USE_MODE_B
		schedule_delayed_work(&gp2a->work, 0);
#endif
	}
	else
	{
		//proximity_onoff(1); //turn on
		//gprintk(KERN_INFO "The proximity sensor is on\n");
	}
	return 0;
}


#ifdef CONFIG_HAS_EARLYSUSPEND
void gp2a_opt_early_suspend(struct early_suspend *h)
{
	struct gp2a_data *gp2a = container_of(h, struct gp2a_data, early_suspend);
	gprintk(KERN_INFO "gp2a_opt_early_suspend\n");
	if(!gp2a->pdev)
		gprintk(KERN_INFO "gp2a_opt_early_suspend is blocked\n");
	else
		gp2a_opt_suspend(gp2a->pdev);
}

void gp2a_opt_late_resume(struct early_suspend *h)
{
	struct gp2a_data *gp2a = container_of(h, struct gp2a_data, early_suspend);
	//gprintk(KERN_INFO "gp2a_opt_late_resume\n");   Fixing Log Checker issues
	if(!gp2a->pdev)
		gprintk(KERN_INFO "gp2a_opt_late_resume is blocked\n");
	else
		gp2a_opt_resume(gp2a->pdev);
}
#endif

static int proximity_onoff(u8 onoff)
{
	u8 value;
	int i, j, err = 0;

	gprintk(KERN_INFO "The proximity sensor is %s\n", (onoff)?"ON":"OFF");
	if(onoff)
	{
		if(gp2a_pdata && gp2a_pdata->power_en)
			gp2a_pdata->power_en(1);

#ifdef USE_MODE_B
		mvo_value = 0;
		/* set shutdown mode */
		// ASD : Select switch for analog sleep function ( 0:ineffective, 1:effective )
		// OCON[1:0] : Select switches for enabling/disabling VOUT terminal 
		//             ( 00:enable, 11:force to go High, 10:forcr to go Low )
		//value = 0x18;	// 11:force to go High
		//opt_i2c_write((u8)(REGS_CON),&value);
#endif
		for(j=0; j<3; j++) {
			if((err < 0) && gp2a_pdata && gp2a_pdata->power_en) {
				gprintk(KERN_INFO "gp2a need to reset!\n");
				gp2a_pdata->power_en(0);
				gp2a_pdata->power_en(1);
			}
			for(i=1; i<5; i++) {
				err = opt_i2c_write((u8)(i),&gp2a_original_image[i]);
				if(err < 0)
					break;
			}
			if (err >= 0)
				break; //normal behavior
		}

#ifdef USE_MODE_B
		// OCON[1:0] : Select switches for enabling/disabling VOUT terminal 
		//             ( 00:enable, 11:force to go High, 10:forcr to go Low )
		value = 0x00;	// 00:enable
		opt_i2c_write((u8)(REGS_CON),&value);
#endif
	}
	else
	{
		/* set shutdown mode */
		value = 0x02;
		opt_i2c_write((u8)(REGS_OPMOD),&value);

		if(gp2a_pdata && gp2a_pdata->power_en)
			gp2a_pdata->power_en(0);
	}

	return 0;
}

static int opt_i2c_remove(struct i2c_client *client)
{
    struct opt_state *data = i2c_get_clientdata(client);

	kfree(data);
	opt_i2c_client = NULL;

	return 0;
}

static int opt_i2c_probe(struct i2c_client *client,  const struct i2c_device_id *id)
{
	struct opt_state *opt;

    gprintk(KERN_INFO "\n");
	opt = kzalloc(sizeof(struct opt_state), GFP_KERNEL);
	if (opt == NULL) {
		gprintk(KERN_ERR "failed to allocate memory \n");
		return -ENOMEM;
	}

	opt->client = client;
	i2c_set_clientdata(client, opt);

	/* rest of the initialisation goes here. */

	gprintk(KERN_INFO "GP2A opt i2c attach success!!!\n");

	gp2a_pdata = client->dev.platform_data;

	opt_i2c_client = client;

	return 0;
}


static const struct i2c_device_id opt_device_id[] = {
	{"gp2a", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, opt_device_id);

static struct i2c_driver opt_i2c_driver = {
	.driver = {
		.name = "gp2a",
		.owner= THIS_MODULE,
	},
	.probe		= opt_i2c_probe,
	.remove		= opt_i2c_remove,
	.id_table	= opt_device_id,
};


static struct platform_driver gp2a_opt_driver = {
	.probe 	 = gp2a_opt_probe,
	.remove = gp2a_opt_remove,
	//.suspend = gp2a_opt_suspend,
	//.resume  = gp2a_opt_resume,
	.driver  = {
		.name = "gp2a-opt",
		.owner = THIS_MODULE,
	},
};

static int __init gp2a_opt_init(void)
{
	int ret;
	ret = platform_driver_register(&gp2a_opt_driver);

	return ret;
}

static void __exit gp2a_opt_exit(void)
{
	platform_driver_unregister(&gp2a_opt_driver);
}

module_init( gp2a_opt_init );
module_exit( gp2a_opt_exit );

MODULE_AUTHOR("SAMSUNG");
MODULE_DESCRIPTION("Optical Sensor driver for GP2AP002A00F");
MODULE_LICENSE("GPL");

