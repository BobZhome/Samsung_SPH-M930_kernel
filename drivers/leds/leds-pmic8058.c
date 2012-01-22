/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
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
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/mfd/pmic8058.h>
#include <linux/leds-pmic8058.h>
#include <linux/delay.h>
#include <mach/vreg.h>
#include <mach/gpio.h>

#if defined(CONFIG_MACH_CHIEF) || defined(CONFIG_MACH_VITAL2)
#define CONFIG_PMIC8058_LPG
#endif

#if defined(CONFIG_PMIC8058_LPG)
#include <linux/pwm.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#endif

#define SSBI_REG_ADDR_DRV_KEYPAD	0x48
#define PM8058_DRV_KEYPAD_BL_MASK	0xf0
#define PM8058_DRV_KEYPAD_BL_SHIFT	0x04

#define SSBI_REG_ADDR_FLASH_DRV0        0x49
#define PM8058_DRV_FLASH_MASK           0xf0
#define PM8058_DRV_FLASH_SHIFT          0x04

#define SSBI_REG_ADDR_FLASH_DRV1        0xFB

#define SSBI_REG_ADDR_LED_CTRL_BASE	0x131
#define SSBI_REG_ADDR_LED_CTRL(n)	(SSBI_REG_ADDR_LED_CTRL_BASE + (n))
#define PM8058_DRV_LED_CTRL_MASK	0xf8
#define PM8058_DRV_LED_CTRL_SHIFT	0x03

#define MAX_FLASH_CURRENT	300
#define MAX_KEYPAD_CURRENT 300
#define MAX_KEYPAD_BL_LEVEL	(1 << 4)
#define MAX_LED_DRV_LEVEL	20 /* 2 * 20 mA */

#define PMIC8058_LED_OFFSET(id) ((id) - PMIC8058_ID_LED_0)

struct pmic8058_led_data {
	struct led_classdev	cdev;
	int			id;
	enum led_brightness	brightness;
	u8			flags;
	struct pm8058_chip	*pm_chip;
	struct work_struct	work;
	struct mutex		lock;
	spinlock_t		value_lock;
	u8			reg_kp;
	u8			reg_led_ctrl[3];
	u8			reg_flash_led0;
	u8			reg_flash_led1;
#if defined(CONFIG_PMIC8058_LPG)
	struct pwm_device *pwm_dev;
#endif

};

#ifdef CONFIG_HAS_EARLYSUSPEND
//#define PMIC8058_DEFFERED_LPG_BLINK
#endif

#if defined(CONFIG_PMIC8058_LPG) && defined(PMIC8058_DEFFERED_LPG_BLINK)
extern struct workqueue_struct *suspend_work_queue;
struct deferred_lpg_blink_set {
	struct work_struct blink_set_workqueue;
	struct led_classdev *led_cdev;
	unsigned long delay_on;
	unsigned long delay_off;
};
#endif

#define PM8058_MAX_LEDS		7
#define MSM_GPIO_SUB_LED_EN	57

static struct pmic8058_led_data led_data[PM8058_MAX_LEDS];


void sub_led_config(int onoff)
{
	if(onoff) {
		gpio_tlmm_config(GPIO_CFG(MSM_GPIO_SUB_LED_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_10MA), GPIO_CFG_ENABLE);
		gpio_set_value_cansleep(MSM_GPIO_SUB_LED_EN, 1);
	}
	else {
		gpio_set_value_cansleep(MSM_GPIO_SUB_LED_EN, 0);
	}
	return;
}
	
static void kp_bl_set(struct pmic8058_led_data *led, enum led_brightness value)
{
	int rc;
	u8 level;
	unsigned long flags;

#ifdef CONFIG_MACH_VITAL2
	{
		if (led->id == PMIC8058_ID_LED_KB_LIGHT) {
			 if (system_rev >= 5) {
				sub_led_config(value);
			 }
			 else {
				struct vreg *vreg_l13;
				vreg_l13 = vreg_get(NULL, "wlan");

				if(value) {
					printk(KERN_INFO "[LED] %s - vreg_enable for led->id:%d\n", __func__, led->id);
					rc = vreg_set_level(vreg_l13, 3000);
					if (rc) {
						printk(KERN_ERR "%s: vreg_l13 set level failed (%d)\n", __func__, rc);
						return;
					}

					rc = vreg_enable(vreg_l13);
					if (rc) {
						printk(KERN_ERR "%s: vreg_l13 enable failed (%d)\n", __func__, rc);
						return;
					}
				}
				else {
					printk(KERN_INFO "[LED] %s - vreg_disable for led->id:%d\n", __func__, led->id);
					rc = vreg_disable(vreg_l13);
					if (rc) {
						printk(KERN_ERR "%s: vreg_l13 enable failed (%d)\n", __func__, rc);
						return;
					}
				}				
			}
			return;
		}		
	}
#else
	spin_lock_irqsave(&led->value_lock, flags);
	level = (value << PM8058_DRV_KEYPAD_BL_SHIFT) &
		PM8058_DRV_KEYPAD_BL_MASK;

	led->reg_kp &= ~PM8058_DRV_KEYPAD_BL_MASK;
	led->reg_kp |= level;
	spin_unlock_irqrestore(&led->value_lock, flags);

	rc = pm8058_write(led->pm_chip, SSBI_REG_ADDR_DRV_KEYPAD,
			&led->reg_kp, 1);
	if (rc)
		pr_err("%s: can't set keypad backlight level\n", __func__);
#endif
}

static enum led_brightness kp_bl_get(struct pmic8058_led_data *led)
{
#ifdef CONFIG_MACH_VITAL2
	if(vreg_get_refcnt(NULL, "wlan"))
		return LED_FULL;
	else
		return LED_OFF;
#else
	if ((led->reg_kp & PM8058_DRV_KEYPAD_BL_MASK) >>
			PM8058_DRV_KEYPAD_BL_SHIFT)
		return LED_FULL;
	else
		return LED_OFF;
#endif
}

extern int is_sleep_debug_enabled(void);

static void led_lc_set(struct pmic8058_led_data *led, enum led_brightness value)
{
	unsigned long flags;
	int rc, offset;
	u8 level, tmp;

	pr_info("[%s] id:%d value:%d\n", __func__, led->id, value);
	
#if 1 //sleep.led.debug
	if(is_sleep_debug_enabled()) {
		pr_info("[%s] sleep debug is enabled!\n", __func__);
		return;
	}
#endif

	spin_lock_irqsave(&led->value_lock, flags);

	if (led->id == PMIC8058_ID_LED_1 || led->id == PMIC8058_ID_LED_2) {
		if(value) value = 3;	//led brightness fix (value 3)
	}

	level = (value << PM8058_DRV_LED_CTRL_SHIFT) & PM8058_DRV_LED_CTRL_MASK; //led->brightness

	offset = PMIC8058_LED_OFFSET(led->id);
	tmp = led->reg_led_ctrl[offset];

	tmp &= ~PM8058_DRV_LED_CTRL_MASK;
	tmp |= level;
	spin_unlock_irqrestore(&led->value_lock, flags);
	
	rc = pm8058_write(led->pm_chip,	SSBI_REG_ADDR_LED_CTRL(offset),
			&tmp, 1);
	if (rc) {
		dev_err(led->cdev.dev, "can't set (%d) led value\n",
				led->id);
		return;
	}

	spin_lock_irqsave(&led->value_lock, flags);
	led->reg_led_ctrl[offset] = tmp;
	spin_unlock_irqrestore(&led->value_lock, flags);
}

/*
 *led_lc_panic_blink - for blinking the LED'S during panic
 *@count: The count passed by panic.c file-not used in the function
 *Returns delay(delay required for ssbi to turn-on/off LED repeatedly)
 *in milliseconds on success and -EINVAL if couldn't get proper id of LED.
 */
static long led_lc_panic_blink(long count)
{
	unsigned long flags;
	struct pmic8058_led_data *led;
	enum pmic8058_leds id;
	long delay = 20;
	int rc, offset;
	static int tmp=0;

	tmp = (tmp == 80?0:80);
	id = PMIC8058_ID_LED_0;
	led = &led_data[id];
	if (!led) {
		pr_err("%s:  led not available\n", __func__);
		return -EINVAL;
	}

	spin_lock_irqsave(&led->value_lock, flags);
	offset = PMIC8058_LED_OFFSET(led->id);
	spin_unlock_irqrestore(&led->value_lock, flags);	
	//TODO-need to remove this delay
	mdelay(delay);						//This delay is required as without this delay the LED is not getting turned off/on
	rc = pm8058_write(led->pm_chip,	SSBI_REG_ADDR_LED_CTRL(offset),
			&tmp, 1);
	if (rc) {
		dev_err(led->cdev.dev, "can't set (%d) led value\n",
				led->id);
		return;
	}
	return delay;
}
EXPORT_SYMBOL(led_lc_panic_blink);

static enum led_brightness led_lc_get(struct pmic8058_led_data *led)
{
	int offset;
	u8 value;

	offset = PMIC8058_LED_OFFSET(led->id);
	value = led->reg_led_ctrl[offset];

	if ((value & PM8058_DRV_LED_CTRL_MASK) >>
			PM8058_DRV_LED_CTRL_SHIFT)
		return LED_FULL;
	else
		return LED_OFF;
}

static void
led_flash_set(struct pmic8058_led_data *led, enum led_brightness value)
{
	int rc;
	u8 level;
	unsigned long flags;
	u8 reg_flash_led;
	u16 reg_addr;

	spin_lock_irqsave(&led->value_lock, flags);
	level = (value << PM8058_DRV_FLASH_SHIFT) &
		PM8058_DRV_FLASH_MASK;

	if (led->id == PMIC8058_ID_FLASH_LED_0) {
		led->reg_flash_led0 &= ~PM8058_DRV_FLASH_MASK;
		led->reg_flash_led0 |= level;
		reg_flash_led	    = led->reg_flash_led0;
		reg_addr	    = SSBI_REG_ADDR_FLASH_DRV0;
	} else {
		led->reg_flash_led1 &= ~PM8058_DRV_FLASH_MASK;
		led->reg_flash_led1 |= level;
		reg_flash_led	    = led->reg_flash_led1;
		reg_addr	    = SSBI_REG_ADDR_FLASH_DRV1;
	}
	spin_unlock_irqrestore(&led->value_lock, flags);

	rc = pm8058_write(led->pm_chip, reg_addr, &reg_flash_led, 1);
	if (rc)
		pr_err("%s: can't set flash led%d level %d\n", __func__,
				led->id, rc);
}

int pm8058_set_flash_led_current(enum pmic8058_leds id, unsigned mA)
{
	struct pmic8058_led_data *led;

	if ((id < PMIC8058_ID_FLASH_LED_0) || (id >= PMIC8058_ID_MAX)) {
		pr_err("%s: invalid LED ID (%d) specified\n", __func__, id);
		return -EINVAL;
	}

	led = &led_data[id];
	if (!led) {
		pr_err("%s: flash led not available\n", __func__);
		return -EINVAL;
	}

	if (mA > MAX_FLASH_CURRENT)
		return -EINVAL;

	led_flash_set(led, mA / 20);

	return 0;
}
EXPORT_SYMBOL(pm8058_set_flash_led_current);

int pm8058_set_led_current(enum pmic8058_leds id, unsigned mA)
{
	struct pmic8058_led_data *led;
	int brightness = 0;

	if ((id < PMIC8058_ID_LED_KB_LIGHT) || (id >= PMIC8058_ID_MAX)) {
		pr_err("%s: invalid LED ID (%d) specified\n", __func__, id);
		return -EINVAL;
	}
	printk(KERN_INFO "[LED] %s - id:%d\n", __func__, id);

	led = &led_data[id];
	if (!led || (led->pm_chip == NULL)) {
		pr_err("%s: flash led not available\n", __func__);
		return -EINVAL;
	}

	switch (id) {
		case PMIC8058_ID_LED_0:
		case PMIC8058_ID_LED_1:
		case PMIC8058_ID_LED_2:
			brightness = mA / 2;
			pr_err("%s: id:%d, brightness:%d, max_brightness:%d", __func__, id, brightness, led->cdev.max_brightness);

			if (brightness  > led->cdev.max_brightness) {
				brightness = led->cdev.max_brightness;
				//return -EINVAL;
			}
			led_lc_set(led, brightness);
			break;

		case PMIC8058_ID_LED_KB_LIGHT:
		case PMIC8058_ID_FLASH_LED_0:
		case PMIC8058_ID_FLASH_LED_1:
			brightness = mA / 20;
			if (brightness  > led->cdev.max_brightness)
				return -EINVAL;
			if (id == PMIC8058_ID_LED_KB_LIGHT)
				kp_bl_set(led, brightness);
			else
				led_flash_set(led, brightness);
			break;
	}

	return 0;
}
EXPORT_SYMBOL(pm8058_set_led_current);

static void pmic8058_led_set(struct led_classdev *led_cdev,
		enum led_brightness value)
{
	struct pmic8058_led_data *led;
	unsigned long flags;
	
	led = container_of(led_cdev, struct pmic8058_led_data, cdev);
	
	spin_lock_irqsave(&led->value_lock, flags);
	led->brightness = value;
	schedule_work(&led->work);
	spin_unlock_irqrestore(&led->value_lock, flags);
}

static void pmic8058_led_work(struct work_struct *work)
{
	struct pmic8058_led_data *led = container_of(work,
			struct pmic8058_led_data, work);

	mutex_lock(&led->lock);

	switch (led->id) {
		case PMIC8058_ID_LED_KB_LIGHT:
			kp_bl_set(led, led->brightness);
			break;
		case PMIC8058_ID_LED_0:
		case PMIC8058_ID_LED_1:
		case PMIC8058_ID_LED_2:
			led_lc_set(led, led->brightness);
			break;
		case PMIC8058_ID_FLASH_LED_0:
		case PMIC8058_ID_FLASH_LED_1:
			led_flash_set(led, led->brightness);
			break;
	}

	mutex_unlock(&led->lock);
}

static enum led_brightness pmic8058_led_get(struct led_classdev *led_cdev)
{
	struct pmic8058_led_data *led;

	led = container_of(led_cdev, struct pmic8058_led_data, cdev);

	switch (led->id) {
		case PMIC8058_ID_LED_KB_LIGHT:
			return kp_bl_get(led);
		case PMIC8058_ID_LED_0:
		case PMIC8058_ID_LED_1:
		case PMIC8058_ID_LED_2:
			return led_lc_get(led);
	}
	return LED_OFF;
}
#if defined(CONFIG_PMIC8058_LPG)

static int lpg_blink_set_internal(struct led_classdev *led_cdev, unsigned long *delay_on, unsigned long *delay_off)
{
	struct pmic8058_led_data *led;

	led = container_of(led_cdev, struct pmic8058_led_data, cdev);

	pr_info("[%s] led_id=%d delay_on=%d delay_off=%d \n", __func__, led->id, *delay_on, *delay_off);

	if(*delay_on && *delay_off) {
		led_cdev->flags = led_cdev->flags & ~LED_CORE_SUSPENDRESUME; // disable suspend
		if(led->pwm_dev) {
			pwm_disable(led->pwm_dev);
			pwm_free(led->pwm_dev);
			led->pwm_dev = NULL;
		}
		led->pwm_dev = pwm_request(led->id+2, "pm8058_lpg_pwm"); //led->id is 2,3,4... add 2 to make is a pwm id.. see pm8058_pwm_config function in board-chief to see what it means

		pwm_config(led->pwm_dev, (*delay_on)*1000, ((*delay_on)+(*delay_off))*1000);
		pwm_enable(led->pwm_dev);
	} else {
		led_cdev->flags = led_cdev->flags | LED_CORE_SUSPENDRESUME; // enable suspend
		if(led->pwm_dev) {
			pwm_disable(led->pwm_dev);
			pwm_free(led->pwm_dev);
			led->pwm_dev = NULL;
		}
	}

	return 0;
}

#ifdef PMIC8058_DEFFERED_LPG_BLINK
static void lpg_blink_set_work(struct work_struct *data)
{
	struct deferred_lpg_blink_set *lpg_data
		= container_of(data, struct deferred_lpg_blink_set, blink_set_workqueue);

	lpg_blink_set_internal(lpg_data->led_cdev, lpg_data->delay_on, 
		lpg_data->delay_off);

	kfree(lpg_data);
}
#endif


static int lpg_blink_set(struct led_classdev *led_cdev, unsigned long *delay_on, unsigned long *delay_off)
{
#ifdef PMIC8058_DEFFERED_LPG_BLINK
	struct deferred_lpg_blink_set *deffered_lpg_data;
#endif
	struct pmic8058_led_data *led = container_of(led_cdev, struct pmic8058_led_data, cdev);

	pr_info("[%s] led_id=%d delay_on=%d delay_off=%d \n", __func__, led->id, *delay_on, *delay_off);

#ifdef PMIC8058_DEFFERED_LPG_BLINK
	deffered_lpg_data = kzalloc(sizeof(struct deferred_lpg_blink_set), GFP_KERNEL);
	if(deffered_lpg_data == NULL) {
		pr_err("!![%s] failed to allocate memory", __func__);
		return -ENOMEM;
	} else {
		deffered_lpg_data->led_cdev = led_cdev;
		deffered_lpg_data->delay_on = delay_on;
		deffered_lpg_data->delay_off = delay_off;
		INIT_WORK(&(deffered_lpg_data->blink_set_workqueue), lpg_blink_set_work);
		queue_work(suspend_work_queue, &(deffered_lpg_data->blink_set_workqueue));
	}
#else
	lpg_blink_set_internal(led_cdev, delay_on, delay_off);
#endif

	return 0;
}
#endif

static int pmic8058_led_probe(struct platform_device *pdev)
{
	struct pmic8058_leds_platform_data *pdata = pdev->dev.platform_data;
	struct pmic8058_led_data *led_dat;
	struct pmic8058_led *curr_led;
	int rc, i = 0;
	struct pm8058_chip	*pm_chip;
	u8			reg_kp;
	u8			reg_led_ctrl[3];
	u8			reg_flash_led0;
	u8			reg_flash_led1;
	panic_blink = led_lc_panic_blink;				//for triggering LED'S during panic anish.singh@samsung.com
	pm_chip = platform_get_drvdata(pdev);
	if (pm_chip == NULL) {
		dev_err(&pdev->dev, "no parent data passed in\n");
		return -EFAULT;
	}

	if (pdata == NULL) {
		dev_err(&pdev->dev, "platform data not supplied\n");
		return -EINVAL;
	}

	rc = pm8058_read(pm_chip, SSBI_REG_ADDR_DRV_KEYPAD, &reg_kp,
			1);
	if (rc) {
		dev_err(&pdev->dev, "can't get keypad backlight level\n");
		goto err_reg_read;
	}

	rc = pm8058_read(pm_chip, SSBI_REG_ADDR_LED_CTRL_BASE,
			reg_led_ctrl, 3);
	if (rc) {
		dev_err(&pdev->dev, "can't get led levels\n");
		goto err_reg_read;
	}

	rc = pm8058_read(pm_chip, SSBI_REG_ADDR_FLASH_DRV0,
			&reg_flash_led0, 1);
	if (rc) {
		dev_err(&pdev->dev, "can't read flash led0\n");
		goto err_reg_read;
	}

	rc = pm8058_read(pm_chip, SSBI_REG_ADDR_FLASH_DRV1,
			&reg_flash_led1, 1);
	if (rc) {
		dev_err(&pdev->dev, "can't get flash led1\n");
		goto err_reg_read;
	}

	for (i = 0; i < pdata->num_leds; i++) {
		curr_led	= &pdata->leds[i];
		led_dat		= &led_data[curr_led->id];

		led_dat->cdev.name		= curr_led->name;
		led_dat->cdev.default_trigger   = curr_led->default_trigger;
		led_dat->cdev.brightness_set    = pmic8058_led_set;
		led_dat->cdev.brightness_get    = pmic8058_led_get;
		led_dat->cdev.brightness	= LED_OFF;
		led_dat->cdev.max_brightness	= curr_led->max_brightness;
#if defined(CONFIG_PMIC8058_LPG)
		led_dat->cdev.blink_set = lpg_blink_set;
#endif
		led_dat->cdev.flags		= LED_CORE_SUSPENDRESUME;
		led_dat->id		        = curr_led->id;
		led_dat->reg_kp			= reg_kp;
		memcpy(led_data->reg_led_ctrl, reg_led_ctrl,
				sizeof(reg_led_ctrl));
		led_dat->reg_flash_led0		= reg_flash_led0;
		led_dat->reg_flash_led1		= reg_flash_led1;

		if (!((led_dat->id >= PMIC8058_ID_LED_KB_LIGHT) &&
					(led_dat->id < PMIC8058_ID_MAX))) {
			dev_err(&pdev->dev, "invalid LED ID (%d) specified\n",
					led_dat->id);
			rc = -EINVAL;
			goto fail_id_check;
		}

		led_dat->pm_chip		= pm_chip;

		mutex_init(&led_dat->lock);
		spin_lock_init(&led_dat->value_lock);
		INIT_WORK(&led_dat->work, pmic8058_led_work);

		rc = led_classdev_register(&pdev->dev, &led_dat->cdev);
		if (rc) {
			dev_err(&pdev->dev, "unable to register led %d\n",
					led_dat->id);
			goto fail_id_check;
		}
	}

	platform_set_drvdata(pdev, led_data);

	return 0;

err_reg_read:
fail_id_check:
	if (i > 0) {
		for (i = i - 1; i >= 0; i--)
			led_classdev_unregister(&led_data[i].cdev);
	}
	return rc;
}

static int __devexit pmic8058_led_remove(struct platform_device *pdev)
{
	int i;
	struct pmic8058_leds_platform_data *pdata = pdev->dev.platform_data;
	struct pmic8058_led_data *led = platform_get_drvdata(pdev);

	for (i = 0; i < pdata->num_leds; i++) {
		led_classdev_unregister(&led[led->id].cdev);
		cancel_work_sync(&led[led->id].work);
	}

	return 0;
}

static struct platform_driver pmic8058_led_driver = {
	.probe		= pmic8058_led_probe,
	.remove		= __devexit_p(pmic8058_led_remove),
	.driver		= {
		.name	= "pm8058-led",
		.owner	= THIS_MODULE,
	},
};

static int __init pmic8058_led_init(void)
{
	return platform_driver_register(&pmic8058_led_driver);
}
module_init(pmic8058_led_init);

static void __exit pmic8058_led_exit(void)
{
	platform_driver_unregister(&pmic8058_led_driver);
}
module_exit(pmic8058_led_exit);

MODULE_DESCRIPTION("PMIC8058 LEDs driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:pmic8058-led");
