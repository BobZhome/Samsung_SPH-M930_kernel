/* Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/bootmem.h>
#include <linux/io.h>
#include <linux/lcd.h>
#ifdef CONFIG_SPI_QSD
#include <linux/spi/spi.h>
#endif
#include <linux/leds-pmic8058.h>
#include <linux/mfd/pmic8058.h>
#include <linux/mfd/marimba.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/input.h>
#include <linux/smsc911x.h>
#include <linux/ofn_atlab.h>
#include <linux/power_supply.h>
#ifdef CONFIG_SENSORS_MAX9879
#include <linux/i2c/max9879.h>
#endif

#ifdef CONFIG_SENSORS_MAX9877
#include <linux/i2c/max9877.h>
#endif
#ifdef CONFIG_SENSORS_YDA165
#include <linux/i2c/yda165.h>
#endif
#include <linux/input/pmic8058-keypad.h>
#include <linux/pwm.h>
#include <linux/pmic8058-pwm.h>
#include <linux/msm_adc.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/setup.h>

#include <mach/mpp.h>
#include <mach/gpio.h>
#include <mach/board.h>
#include <mach/camera.h>
#include <mach/memory.h>
#include <mach/msm_iomap.h>
#include <mach/msm_hsusb.h>
#include <mach/rpc_hsusb.h>
#include <mach/msm_spi.h>
#include <mach/qdsp5v2/msm_lpa.h>
#include <mach/dma.h>
#include <linux/android_pmem.h>
#include <linux/input/msm_ts.h>
#include <mach/pmic.h>
#include <mach/rpc_pmapp.h>
#include <mach/qdsp5v2/aux_pcm.h>
#include <mach/qdsp5v2/mi2s.h>
#include <mach/qdsp5v2/audio_dev_ctl.h>
#include <mach/msm_battery.h>
#include <mach/rpc_server_handset.h>

#include <asm/mach/mmc.h>
#include <asm/mach/flash.h>
#include <mach/vreg.h>
#include "devices.h"
#include "timer.h"
#include "mach/socinfo.h"
#ifdef CONFIG_USB_ANDROID
#include <linux/usb/android_composite.h>
#endif
#include "pm.h"
#include "spm.h"
#include <linux/msm_kgsl.h>
#include <mach/dal_axi.h>
#include <mach/msm_serial_hs.h>
#include <mach/msm_reqs.h>
#ifdef CONFIG_SAMSUNG_JACK
#include <linux/sec_jack.h>
#endif
#include <linux/wlan_plat.h>//CONFIG_WIFI_CONTROL_FUNC

#include <../../../drivers/bluetooth/bluesleep.c>//shiks_test
#include "proc_comm.h"
#include <linux/i2c/gp2a.h>
#ifdef CONFIG_INPUT_YAS_MAGNETOMETER
#include <linux/i2c/yas.h>
#endif
#ifdef CONFIG_SENSORS_YAS529_MAGNETIC
#include <linux/i2c/yas529.h>
#endif
#include <linux/i2c/kr3dh.h>
#include <linux/i2c/fsa9480.h>

#ifdef CONFIG_SERIAL_MSM_RX_WAKEUP
struct msm_serial_platform_data {
	int wakeup_irq;  
	unsigned char inject_rx_on_wakeup;
	char rx_to_inject;
};
#endif

int charging_boot = 0;
int fota_boot = 0;

EXPORT_SYMBOL(charging_boot);
EXPORT_SYMBOL(fota_boot);

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
#define WLAN_DEVICE "bcm4330_wlan"
#define WLAN_DEVICE_IRQ "bcm4330_wlan_irq"
#else
#define WLAN_DEVICE "bcm4329_wlan"
#define WLAN_DEVICE_IRQ "bcm4329_wlan_irq"
#endif

// #define ATH_POLLING 1
/* Stealth REV01 GPIO Mapping */
#define WLAN_GPIO_RESET 		150
#define WLAN_GPIO_HOST_WAKE 	((system_rev>=5)?42:26)
//#define WLAN_HOST_WAKE

#ifdef CONFIG_WIFI_CONTROL_FUNC
//Vital2 GPIO
//#define GPIO_BT_RESET			165
//#define GPIO_BT_WLAN_REG_ON		164
#define GPIO_WLAN_RESET			150
//#define GPIO_BT_WLAN_REG_ON		164
#define GPIO_WLAN_WAKES_MSM_01	132//WLAN_HOST_WAKE
#define GPIO_WLAN_WAKES_MSM_02	66
#define GPIO_WLAN_WAKES_MSM_REV05	42
#endif

//shiks_test start
#define GPIO_BT_WAKE         162
#define GPIO_BT_HOST_WAKE_01	163
#define GPIO_BT_HOST_WAKE_02	68
#define GPIO_BT_HOST_WAKE_REV05	51

#define GPIO_BT_WLAN_REG_ON  164
#define GPIO_BT_RESET        165

#define GPIO_BT_UART_RTS     134
#define GPIO_BT_UART_CTS     135
#define GPIO_BT_UART_RXD     136
#define GPIO_BT_UART_TXD     137
#define GPIO_BT_PCM_DOUT     138
#define GPIO_BT_PCM_DIN      139
#define GPIO_BT_PCM_SYNC     140
#define GPIO_BT_PCM_CLK      141
//shiks_test end

#ifdef CONFIG_WIFI_CONTROL_FUNC
#define GPIO_WLAN_LEVEL_LOW			0
#define GPIO_WLAN_LEVEL_HIGH			1
#define GPIO_WLAN_LEVEL_NONE			2

#define PREALLOC_WLAN_SEC_NUM		4
#define PREALLOC_WLAN_BUF_NUM		160
#define PREALLOC_WLAN_SECTION_HEADER	24

#define WLAN_SECTION_SIZE_0	(PREALLOC_WLAN_BUF_NUM * 128)
#define WLAN_SECTION_SIZE_1	(PREALLOC_WLAN_BUF_NUM * 128)
#define WLAN_SECTION_SIZE_2	(PREALLOC_WLAN_BUF_NUM * 512)
#define WLAN_SECTION_SIZE_3	(PREALLOC_WLAN_BUF_NUM * 1024)

#define WLAN_SKB_BUF_NUM	17

static struct sk_buff *wlan_static_skb[WLAN_SKB_BUF_NUM];

struct wifi_mem_prealloc {
	void *mem_ptr;
	unsigned long size;
};
#endif

struct class *sec_class;
EXPORT_SYMBOL(sec_class);
struct device *switch_dev;
EXPORT_SYMBOL(switch_dev);

#define RORY_CONTROL	1

#define MSM_PMEM_SF_SIZE	0x1500000 
// #define MSM_FB_SIZE		0x500000  //fb1 RGB565 single
#define MSM_FB_SIZE          0xA46000  //fb1 ARGB8888 double
#define MSM_GPU_PHYS_SIZE       SZ_2M
#define MSM_PMEM_ADSP_SIZE      0x1800000
#define PMEM_KERNEL_EBI1_SIZE   0x600000
#define MSM_PMEM_AUDIO_SIZE     0x200000

//vital2.boot #define PMEM_HDMI
#ifdef PMEM_HDMI
#define PMEM_HDMI_SIZE					0x500000
#endif

#define PMIC_GPIO_INT		27
#define PMIC_VREG_WLAN_LEVEL	2900


/* Common PMIC GPIO ************************************************************/
#define PMIC_GPIO_POPUP_SW_EN              PM8058_GPIO(25)
#define PMIC_GPIO_VOLUME_UP		       PM8058_GPIO(26)
#define PMIC_GPIO_UART_SEL				PM8058_GPIO(7)

#define PMIC_GPIO_CAM_FLASH_SET		PM8058_GPIO(27)
#define PMIC_GPIO_CAM_FLASH_EN		PM8058_GPIO(28)
#define PMIC_GPIO_LCD_BL_CTRL			PM8058_GPIO(29)
#define PMIC_GPIO_PS_VOUT				PM8058_GPIO(30)
#define PMIC_GPIO_MSENSE_RST			PM8058_GPIO(35)

#define PMIC_GPIO_TA_CURRENT_SEL		PM8058_GPIO(18)
#define PMIC_GPIO_LCD_ID				PM8058_GPIO(19)

//#define PMIC_GPIO_ALS_SCL 			  	PM8058_GPIO(31)
//#define PMIC_GPIO_ALS_SDA   			PM8058_GPIO(32)

#define PMIC_GPIO_DDR_EXT_SMPS_EN	PM8058_GPIO(34)
#define PMIC_GPIO_SD_DET				PM8058_GPIO(36)
#define PMIC_GPIO_VOLUME_DOWN		PM8058_GPIO(37)

#define MSM_GPIO_ALS_SDA			128 //vital2.rev01 : 124 -> 128
#define MSM_GPIO_ALS_SCL			124 //vital2.rev01 : 128 -> 124


/* Macros assume PMIC GPIOs start at 0 */
#define PM8058_GPIO_PM_TO_SYS(pm_gpio)     (pm_gpio + NR_GPIO_IRQS)
#define PM8058_GPIO_SYS_TO_PM(sys_gpio)    (sys_gpio - NR_GPIO_IRQS)

#define PMIC_GPIO_FLASH_BOOST_ENABLE	15	/* PMIC GPIO Number 16 */
#define PMIC_GPIO_HAP_ENABLE   16  /* PMIC GPIO Number 17 */

#define HAP_LVL_SHFT_MSM_GPIO 24

#define PMIC_GPIO_QUICKVX_CLK 37 /* PMIC GPIO 38 */

#define	PM_FLIP_MPP 5 /* PMIC MPP 06 */
static int pm8058_gpios_init(void)
{
	int rc = 0;
	struct pm8058_gpio popup_sw_en = {
		.direction      = PM_GPIO_DIR_OUT,
		.pull           = PM_GPIO_PULL_UP_1P5,
		.vin_sel        = 2,
		.function       = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol    = 0,
	};
	
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	struct pm8058_gpio sdcc_det = {
		.direction      = PM_GPIO_DIR_IN,
		.pull           = PM_GPIO_PULL_UP_1P5,
		.vin_sel        = 2,
		.function       = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol    = 0,
	};
#endif

#ifdef CONFIG_MACH_VITAL2
	struct pm8058_gpio volume_key_inconfig = {
		.direction      = PM_GPIO_DIR_IN,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel        = 2,
		.function       = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol    = 0,
	};
#endif

#if defined(CONFIG_INPUT_YAS_MAGNETOMETER) || defined(CONFIG_SENSORS_YAS529_MAGNETIC)
	struct pm8058_gpio msense_nRST = {
		.direction      = PM_GPIO_DIR_OUT,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel        = 2,
		.function       = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol    = 0,
	};
#endif

	struct pm8058_gpio ps_vout = {
		.direction      = PM_GPIO_DIR_IN,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel        = 2,
		.function       = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol    = 0,
		//.out_strength = PM_GPIO_STRENGTH_HIGH,
	};

	struct pm8058_gpio als_pm_scl = {
		.direction      = PM_GPIO_DIR_OUT,
		.pull           = PM_GPIO_PULL_UP_1P5,
		.vin_sel        = 2,
		.function       = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol    = 0,
		//.out_strength = PM_GPIO_STRENGTH_HIGH,
	};
	struct pm8058_gpio als_pm_sda = {
		.direction      = PM_GPIO_DIR_OUT,
		.pull           = PM_GPIO_PULL_UP_1P5,
		.vin_sel        = 2,
		.function       = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol    = 0,
		//.out_strength = PM_GPIO_STRENGTH_HIGH,
	};

	struct pm8058_gpio out_config = {
		.direction      = PM_GPIO_DIR_OUT,
		.pull           = PM_GPIO_PULL_UP_1P5,
		.vin_sel        = 2,
		.function       = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol    = 0,
	};


#if 1 //vital2.boot.temp
	struct pm8058_gpio lcd_bl_config = {
		.direction	= PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_BUF_CMOS,
		.output_value = 1,
		.pull		= PM_GPIO_PULL_DN,
		.vin_sel	= 2,
		.out_strength	= PM_GPIO_STRENGTH_HIGH,
		.function	= PM_GPIO_FUNC_NORMAL,
		.inv_int_pol = 0,
	};
	struct pm8058_gpio uart_sel_config = {
		.direction	= PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_BUF_CMOS,
		.output_value = 0,
		.pull		= PM_GPIO_PULL_NO,
		.vin_sel	= 2,
		.out_strength	= PM_GPIO_STRENGTH_HIGH,
		.function	= PM_GPIO_FUNC_NORMAL,
		.inv_int_pol = 0,
	};

	rc = pm8058_gpio_config(PMIC_GPIO_LCD_BL_CTRL, &lcd_bl_config);
	if (rc) {
		pr_err("%s PMIC_GPIO_LCD_BL_CTRL config failed\n", __func__);
		return rc;
	}

	rc = pm8058_gpio_config(PMIC_GPIO_UART_SEL, &uart_sel_config);
	if (rc) {
		pr_err("%s PMIC_GPIO_UART_SEL config failed\n", __func__);
		return rc;
	}
	rc = pm8058_gpio_config(PMIC_GPIO_POPUP_SW_EN, &popup_sw_en);
	if (rc) {
		pr_err("%s PMIC_GPIO_POPUP_SW_EN config failed\n", __func__);
		return rc;
	}
#endif

#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	sdcc_det.inv_int_pol = 1;

	rc = pm8058_gpio_config(PMIC_GPIO_SD_DET, &sdcc_det);
	if (rc) {
		pr_err("%s PMIC_GPIO_SD_DET config failed\n", __func__);
		return rc;
	}
#endif

#ifdef CONFIG_MACH_VITAL2
	rc = pm8058_gpio_config(PMIC_GPIO_VOLUME_UP, &volume_key_inconfig);
	if (rc) {
		pr_err("%s PMIC_GPIO_VOLUME_UP config failed\n", __func__);
		return rc;
	}

	rc = pm8058_gpio_config(PMIC_GPIO_VOLUME_DOWN, &volume_key_inconfig);
	if (rc) {
		pr_err("%s PMIC_GPIO_VOLUME_DOWN config failed\n", __func__);
		return rc;
	}
#endif

#if defined(CONFIG_INPUT_YAS_MAGNETOMETER) || defined(CONFIG_SENSORS_YAS529_MAGNETIC)
	rc = pm8058_gpio_config(PMIC_GPIO_MSENSE_RST, &msense_nRST);
	if (rc) {
		pr_err("%s PMIC_GPIO_MSENSE_RST config failed\n", __func__);
		return rc;
	}
	gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_MSENSE_RST), 0);	
#endif

	rc = pm8058_gpio_config(PMIC_GPIO_PS_VOUT, &ps_vout);
	if (rc) {
			pr_err("%s PMIC_GPIO_PS_VOUT config failed\n", __func__);
			return rc;
	}
#if 0
	rc = pm8058_gpio_config(PMIC_GPIO_ALS_SCL, &als_pm_scl);
	if (rc) {
		pr_err("%s PMIC_GPIO_ALS_SCL config failed\n", __func__);
		return rc;
	}
	gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_ALS_SCL), 1);

	rc = pm8058_gpio_config(PMIC_GPIO_ALS_SDA, &als_pm_sda);
	if (rc) {
		pr_err("%s PMIC_GPIO_ALS_SDA config failed\n", __func__);
		return rc;
	}
	gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_ALS_SDA), 1);
#endif
  
#if 0 //vital2.boot
	rc = pm8058_gpio_config(PMIC_GPIO_DDR_EXT_SMPS_EN, &out_config);
	if (rc) {
		pr_err("%s PMIC_GPIO_DDR_EXT_SMPS_EN config failed\n", __func__);
		return rc;
	}
	gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_DDR_EXT_SMPS_EN), 1);
#endif

struct pm8058_gpio cam_flash_set = {
		.direction	= PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_BUF_CMOS,
		.output_value = 0,
		.pull		= PM_GPIO_PULL_NO,
		.vin_sel	= 2,
		.out_strength	= PM_GPIO_STRENGTH_HIGH,
		.function	= PM_GPIO_FUNC_NORMAL,
		.inv_int_pol = 0,
	};

      rc = pm8058_gpio_config(PMIC_GPIO_CAM_FLASH_SET, &cam_flash_set);
	if (rc) {
		pr_err("%s PMIC_GPIO_CAM_FLASH_SET config failed\n", __func__);
		return rc;
	}

struct pm8058_gpio cam_flash_en = {
		.direction	= PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_BUF_CMOS,
		.output_value = 0,
		.pull		= PM_GPIO_PULL_NO,
		.vin_sel	= 2,
		.out_strength	= PM_GPIO_STRENGTH_HIGH,
		.function	= PM_GPIO_FUNC_NORMAL,
		.inv_int_pol = 0,
	};

      rc = pm8058_gpio_config(PMIC_GPIO_CAM_FLASH_EN, &cam_flash_en);
	if (rc) {
		pr_err("%s PMIC_GPIO_CAM_FLASH_EN config failed\n", __func__);
		return rc;
	}


	return 0;
}

#ifdef CONFIG_SAMSUNG_JACK
static struct platform_device sec_device_jack = {
	.name           = "sec_jack",
	.id             = -1,
};
#endif

static int pm8058_pwm_config(struct pwm_device *pwm, int ch, int on)
{
	struct pm8058_gpio pwm_gpio_config = {
		.direction      = PM_GPIO_DIR_OUT,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 0,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel        = PM_GPIO_VIN_S3,
		.out_strength   = PM_GPIO_STRENGTH_HIGH,
		.function       = PM_GPIO_FUNC_2,
	};
	int	rc = -EINVAL;
	int	id, mode, max_mA;

	id = mode = max_mA = 0;
	switch (ch) {
	case 0:
	case 1:
	case 2:
		if (on) {
			id = 24 + ch;
			rc = pm8058_gpio_config(id - 1, &pwm_gpio_config);
			if (rc)
				pr_err("%s: pm8058_gpio_config(%d): rc=%d\n",
				       __func__, id, rc);
		}
		break;

	case 3:
		id = PM_PWM_LED_KPD;
		mode = PM_PWM_CONF_DTEST3;
		max_mA = 200;
		break;

	case 4:
		id = PM_PWM_LED_0;
		mode = PM_PWM_CONF_PWM1;
		max_mA = 6;
		break;

	case 5:
		id = PM_PWM_LED_1;
		mode = PM_PWM_CONF_PWM2;
		max_mA = 6;
		break;

	case 6:
		id = PM_PWM_LED_2;
		mode = PM_PWM_CONF_PWM3;
		max_mA = 6;
		break;

	default:
		break;
	}

	if (ch >= 3 && ch <= 6) {
		if (!on) {
			mode = PM_PWM_CONF_NONE;
			max_mA = 0;
		}
		rc = pm8058_pwm_config_led(pwm, id, mode, max_mA);
		if (rc)
			pr_err("%s: pm8058_pwm_config_led(ch=%d): rc=%d\n",
			       __func__, ch, rc);
	}

	return rc;
}

static int pm8058_pwm_enable(struct pwm_device *pwm, int ch, int on)
{
	int	rc;

	switch (ch) {
	case 7:
		rc = pm8058_pwm_set_dtest(pwm, on);
		if (rc)
			pr_err("%s: pwm_set_dtest(%d): rc=%d\n",
			       __func__, on, rc);
		break;
	default:
		rc = -EINVAL;
		break;
	}
	return rc;
}

static struct pm8058_pwm_pdata pm8058_pwm_data = {
	.config		= pm8058_pwm_config,
	.enable		= pm8058_pwm_enable,
};

static const unsigned int vital2_keymap[] = {
	KEY(0, 0, KEY_POWER),  //116
	KEY(0, 1, KEY_Q), //16
	KEY(0, 2, KEY_A), //30
	KEY(0, 3, KEY_F1), //59
	KEY(0, 4, KEY_LEFTSHIFT), //42
	
	KEY(1, 0, KEY_F2), //60
	KEY(1, 1, KEY_W), //17
	KEY(1, 2, KEY_S), //31
	KEY(1, 3, KEY_Z), //44
	KEY(1, 4, KEY_F3), //61
	
	KEY(2, 0, KEY_RESERVED), //0
	KEY(2, 1, KEY_E), //18
	KEY(2, 2, KEY_D), //32
	KEY(2, 3, KEY_X), //45
	KEY(2, 4, KEY_F4), //62
	
	KEY(3, 0, KEY_RESERVED), //0
	KEY(3, 1, KEY_R), //19
	KEY(3, 2, KEY_F), //33
	KEY(3, 3, KEY_C), //46
	KEY(3, 4, KEY_F5), //63
	
	KEY(4, 0, KEY_RESERVED), //0
	KEY(4, 1, KEY_T), //20
	KEY(4, 2, KEY_G), //34
	KEY(4, 3, KEY_V), //47
	KEY(4, 4, KEY_SPACE), //57
	
	KEY(5, 0, KEY_MENU), //139
	KEY(5, 1, KEY_Y),  //21
	KEY(5, 2, KEY_H), //35
	KEY(5, 3, KEY_B), //48
	KEY(5, 4, KEY_BACK), //158
	
	KEY(6, 0, KEY_HOME), //102
	KEY(6, 1, KEY_U), //22
	KEY(6, 2, KEY_J), //36
	KEY(6, 3, KEY_N),  //49
	KEY(6, 4, KEY_SEARCH), //217
	
	KEY(7, 0, KEY_CAMERA_SNAPSHOT), //0x2fe
	KEY(7, 1, KEY_I), //23
	KEY(7, 2, KEY_K), //37
	KEY(7, 3, KEY_M), //50
	KEY(7, 4, KEY_F6), //64
	
	KEY(8, 0, KEY_CAMERA_FOCUS), //0x210
	KEY(8, 1, KEY_O),  //24
	KEY(8, 2, KEY_L), //38
	KEY(8, 3, KEY_QUESTION), //214
	KEY(8, 4, KEY_LEFT), //105
	
	KEY(9, 0, KEY_RESERVED),  //0
	KEY(9, 1, KEY_P), //25
	KEY(9, 2, KEY_DOLLAR),  //0x1b2
	KEY(9, 3, KEY_UP),  //103
	KEY(9, 4, KEY_DOWN), //108

       KEY(10, 0, KEY_RESERVED), //0
	KEY(10, 1, KEY_DELETE), //111
	KEY(10, 2, KEY_ENTER),  //28
	KEY(10, 3, KEY_OK),  //0x160
	KEY(10, 4, KEY_RIGHT),  //106
	
	KEY(11, 0, KEY_RESERVED),
	KEY(11, 1, KEY_RESERVED),
	KEY(11, 2, KEY_RESERVED),
	KEY(11, 3, KEY_VOLUMEUP),  //115
	KEY(11, 4, KEY_VOLUMEDOWN), //114
};

static struct resource resources_keypad[] = {
	{
		.start	= PM8058_KEYPAD_IRQ(PMIC8058_IRQ_BASE),
		.end	= PM8058_KEYPAD_IRQ(PMIC8058_IRQ_BASE),
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= PM8058_KEYSTUCK_IRQ(PMIC8058_IRQ_BASE),
		.end	= PM8058_KEYSTUCK_IRQ(PMIC8058_IRQ_BASE),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct matrix_keymap_data vital2_keymap_data = {
	.keymap_size	= ARRAY_SIZE(vital2_keymap),
	.keymap		= vital2_keymap,
};

static struct pmic8058_keypad_data vital2_keypad_data = {
	.input_name		= "vital2-keypad",
	.input_phys_device	= "vital2-keypad/input0",

	//vital2.boot
	.num_rows		= 12,
	.num_cols		= 5,
	
	.rows_gpio_start	= PM8058_GPIO(9),
	.cols_gpio_start	= PM8058_GPIO(1),
	.debounce_ms		= {8, 10},
	.scan_delay_ms	= 32,
	.row_hold_ns		= 91500,
	.wakeup			= 1,
	.keymap_data		= &vital2_keymap_data,
};

static struct pmic8058_led pmic8058_fluid_flash_leds[] = {
	[0] = {
		.name		= "keyboard-backlight", /* qwerty keyboard */
		.max_brightness = 10, /* 5 mA led0 sink */
		.id		= PMIC8058_ID_LED_0,
	},
	[1] = {
		.name		= "blue",
		.max_brightness = 10, /* 5 mA led1 sink */
		.id		= PMIC8058_ID_LED_1,
	},
	[2] = {
		.name		= "red",
		.max_brightness = 10, /* 5 mA led2 sink */
		.id		= PMIC8058_ID_LED_2,
	},
	[3] = {
		.name		= "button-backlight", /* front 4 keys */
		.max_brightness = 10, /* 5 mA upper led sink */
		.id		= PMIC8058_ID_LED_KB_LIGHT,
	},
};

static struct pmic8058_leds_platform_data pm8058_fluid_flash_leds_data = {
	.num_leds = ARRAY_SIZE(pmic8058_fluid_flash_leds),
	.leds	= pmic8058_fluid_flash_leds,
};


/* Put sub devices with fixed location first in sub_devices array */
#define	PM8058_SUBDEV_KPD	0
#define	PM8058_SUBDEV_LED	1

static struct pm8058_gpio_platform_data pm8058_gpio_data = {
	.gpio_base	= PM8058_GPIO_PM_TO_SYS(0),
	.irq_base	= PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, 0),
	.init		= pm8058_gpios_init,
};

static struct pm8058_gpio_platform_data pm8058_mpp_data = {
	.gpio_base	= PM8058_GPIO_PM_TO_SYS(PM8058_GPIOS),
	.irq_base	= PM8058_MPP_IRQ(PMIC8058_IRQ_BASE, 0),
};

static struct mfd_cell pm8058_subdevs[] = {
	{	.name = "pm8058-keypad",
		.id		= -1,
		.num_resources	= ARRAY_SIZE(resources_keypad),
		.resources	= resources_keypad,
	},
	{	.name = "pm8058-led",
		.id		= -1,
	},
	{	.name = "pm8058-gpio",
		.id		= -1,
		.platform_data	= &pm8058_gpio_data,
		.data_size	= sizeof(pm8058_gpio_data),
	},
	{	.name = "pm8058-mpp",
		.id		= -1,
		.platform_data	= &pm8058_mpp_data,
		.data_size	= sizeof(pm8058_mpp_data),
	},
	{	.name = "pm8058-pwm",
		.id		= -1,
		.platform_data	= &pm8058_pwm_data,
		.data_size	= sizeof(pm8058_pwm_data),
	},
	{	.name = "pm8058-nfc",
		.id		= -1,
	},
	{	.name = "pm8058-upl",
		.id		= -1,
	},
};

static struct pm8058_platform_data pm8058_7x30_data = {
	.irq_base = PMIC8058_IRQ_BASE,

	.num_subdevs = ARRAY_SIZE(pm8058_subdevs),
	.sub_devices = pm8058_subdevs,
	.irq_trigger_flags = IRQF_TRIGGER_LOW,
};

static struct i2c_board_info pm8058_boardinfo[] __initdata = {
	{
		I2C_BOARD_INFO("pm8058-core", 0x55),
		.irq = MSM_GPIO_TO_INT(PMIC_GPIO_INT),
		.platform_data = &pm8058_7x30_data,
	},
};

static struct i2c_board_info msm_camera_boardinfo_01[] __initdata = {
#ifdef CONFIG_MT9D112
	{
		I2C_BOARD_INFO("mt9d112", 0x78 >> 1),
	},
#endif
#ifdef CONFIG_S5K3E2FX
	{
		I2C_BOARD_INFO("s5k3e2fx", 0x20 >> 1),
	},
#endif
#ifdef CONFIG_MT9P012
	{
		I2C_BOARD_INFO("mt9p012", 0x6C >> 1),
	},
#endif
#ifdef CONFIG_VX6953
	{
		I2C_BOARD_INFO("vx6953", 0x20),
	},
#endif
#ifdef CONFIG_SN12M0PZ
	{
		I2C_BOARD_INFO("sn12m0pz", 0x34 >> 1),
	},
#endif
#if defined(CONFIG_MT9T013) || defined(CONFIG_SENSORS_MT9T013)
	{
		I2C_BOARD_INFO("mt9t013", 0x6C),
	},
#endif
#ifdef CONFIG_SENSOR_M5MO
	{
		I2C_BOARD_INFO("m5mo_i2c", 0x3F>>1),
	},
#endif
#ifdef CONFIG_SR030PC30
	{
		I2C_BOARD_INFO("sr030pc30", 0x60>>1),
	},
#endif
#ifdef CONFIG_S5K5CCGX
	{
		I2C_BOARD_INFO("s5k5ccgx", 0x78>>1),
	},
#endif
};

static struct i2c_board_info msm_camera_boardinfo_02[] __initdata = {
#ifdef CONFIG_MT9D112
	{
		I2C_BOARD_INFO("mt9d112", 0x78 >> 1),
	},
#endif
#ifdef CONFIG_S5K3E2FX
	{
		I2C_BOARD_INFO("s5k3e2fx", 0x20 >> 1),
	},
#endif
#ifdef CONFIG_MT9P012
	{
		I2C_BOARD_INFO("mt9p012", 0x6C >> 1),
	},
#endif
#ifdef CONFIG_VX6953
	{
		I2C_BOARD_INFO("vx6953", 0x20),
	},
#endif
#ifdef CONFIG_SN12M0PZ
	{
		I2C_BOARD_INFO("sn12m0pz", 0x34 >> 1),
	},
#endif
#if defined(CONFIG_MT9T013) || defined(CONFIG_SENSORS_MT9T013)
	{
		I2C_BOARD_INFO("mt9t013", 0x6C),
	},
#endif
#ifdef CONFIG_SENSOR_M5MO
	{
		I2C_BOARD_INFO("m5mo_i2c", 0x3F>>1),
	},
#endif
#ifdef CONFIG_S5K5CCGX
	{
		I2C_BOARD_INFO("s5k5ccgx", 0x78>>1),
	},
#endif
};

#ifdef CONFIG_MSM_CAMERA
static uint32_t camera_off_vcm_gpio_table[] = {
GPIO_CFG(1, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* VCM */
};

static uint32_t camera_off_gpio_table[] = {
	/* parallel CAMERA interfaces */
	GPIO_CFG(3,  0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* EN */
	GPIO_CFG(4,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT4 */
	GPIO_CFG(5,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT5 */
	GPIO_CFG(6,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT6 */
	GPIO_CFG(7,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT7 */
	GPIO_CFG(8,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT8 */
	GPIO_CFG(9,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT9 */
	GPIO_CFG(10, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT10 */
	GPIO_CFG(11, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT11 */
	GPIO_CFG(12, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* PCLK */
	GPIO_CFG(13, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* HSYNC_IN */
	GPIO_CFG(14, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* VSYNC_IN */
	GPIO_CFG(15, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* MCLK */
	GPIO_CFG(30, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), //CAM_SCL
	GPIO_CFG(17, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), //CAM_SDA	
};

static uint32_t camera_on_vcm_gpio_table[] = {
GPIO_CFG(1, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), /* VCM */
};

static uint32_t camera_on_gpio_table[] = {
	/* parallel CAMERA interfaces */
//	GPIO_CFG(3,  0, GPIO_CFG_OUTPUT, GO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* CAM_IO_EN */
	GPIO_CFG(3,  0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), /* CAM_IO_EN */
	GPIO_CFG(4,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT4 */
	GPIO_CFG(5,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT5 */
	GPIO_CFG(6,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT6 */
	GPIO_CFG(7,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT7 */
	GPIO_CFG(8,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT8 */
	GPIO_CFG(9,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT9 */
	GPIO_CFG(10, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT10 */
	GPIO_CFG(11, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT11 */
	GPIO_CFG(12, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* PCLK */
	GPIO_CFG(13, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* HSYNC_IN */
	GPIO_CFG(14, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* VSYNC_IN */
//	GPIO_CFG(15, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_14MA), /* MCLK */ //requested by curos
	GPIO_CFG(30, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), //CAM_SCL
	GPIO_CFG(17, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), //CAM_SDA
};

static uint32_t camera_off_gpio_fluid_table[] = {
	/* FLUID: CAM_VGA_RST_N */
	GPIO_CFG(31, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	/* FLUID: Disable CAMIF_STANDBY */
	GPIO_CFG(143, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA)
};

static uint32_t camera_on_gpio_fluid_table[] = {
	/* FLUID: CAM_VGA_RST_N */
	GPIO_CFG(31, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	/* FLUID: Disable CAMIF_STANDBY */
	GPIO_CFG(143, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA)
};

static void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}
static int config_camera_on_gpios(void)
{
	config_gpio_table(camera_on_gpio_table,
		ARRAY_SIZE(camera_on_gpio_table));

	if (adie_get_detected_codec_type() != TIMPANI_ID)
		/* GPIO1 is shared also used in Timpani RF card so
		only configure it for non-Timpani RF card */
		config_gpio_table(camera_on_vcm_gpio_table,
			ARRAY_SIZE(camera_on_vcm_gpio_table));

	if (machine_is_msm7x30_fluid()) {
		config_gpio_table(camera_on_gpio_fluid_table,
			ARRAY_SIZE(camera_on_gpio_fluid_table));
		/* FLUID: turn on 5V booster */
		gpio_set_value(
			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_FLASH_BOOST_ENABLE), 1);
	}
	return 0;
}

static void config_camera_off_gpios(void)
{
	config_gpio_table(camera_off_gpio_table,
		ARRAY_SIZE(camera_off_gpio_table));

	if (adie_get_detected_codec_type() != TIMPANI_ID)
		/* GPIO1 is shared also used in Timpani RF card so
		only configure it for non-Timpani RF card */
		config_gpio_table(camera_off_vcm_gpio_table,
			ARRAY_SIZE(camera_off_vcm_gpio_table));

	if (machine_is_msm7x30_fluid()) {
		config_gpio_table(camera_off_gpio_fluid_table,
			ARRAY_SIZE(camera_off_gpio_fluid_table));
		/* FLUID: turn off 5V booster */
		gpio_set_value(
			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_FLASH_BOOST_ENABLE), 0);
	}
}

struct resource msm_camera_resources[] = {
	{
		.start	= 0xA6000000,
		.end	= 0xA6000000 + SZ_1M - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_VFE,
		.end	= INT_VFE,
		.flags	= IORESOURCE_IRQ,
	},
};

struct msm_camera_device_platform_data msm_camera_device_data = {
	.camera_gpio_on  = config_camera_on_gpios,
	.camera_gpio_off = config_camera_off_gpios,
	.ioext.camifpadphy = 0xAB000000,
	.ioext.camifpadsz  = 0x00000400,
	.ioext.csiphy = 0xA6100000,
	.ioext.csisz  = 0x00000400,
	.ioext.csiirq = INT_CSI,
	.ioclk.mclk_clk_rate = 24000000,
	.ioclk.vfe_clk_rate  = 122880000,
};

static struct i2c_gpio_platform_data camera_i2c_gpio_data = {
	.sda_pin    = 17,
	.scl_pin    = 30,
	.udelay = 2, //i2c speed : 500 / 2 = 250k	
};

static struct platform_device camera_i2c_gpio_device = {  
	.name       = "i2c-gpio",
	.id     = 4,
	.dev        = {
		.platform_data  = &camera_i2c_gpio_data,
	},
};

static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_PWM,
	._fsrc.pwm_src.freq  = 1000,
	._fsrc.pwm_src.max_load = 300,
	._fsrc.pwm_src.low_load = 30,
	._fsrc.pwm_src.high_load = 100,
	._fsrc.pwm_src.channel = 7,
};

#ifdef CONFIG_SENSOR_M5MO

static struct msm_camera_sensor_flash_data flash_m5mo = {
	.flash_type = MSM_CAMERA_FLASH_LED,
//	.flash_type = MSM_CAMERA_FLASH_NONE,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_m5mo_data = {
	.sensor_name    = "m5mo",
	.sensor_reset   = 0,
	.sensor_pwd     = 85,
	.vcm_pwd        = 1,
	.vcm_enable		= 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_m5mo,
	.csi_if         = 0
};
static struct platform_device msm_camera_sensor_m5mo = {
	.name  	= "msm_camera_m5mo",
	.dev   	= {
		.platform_data = &msm_camera_sensor_m5mo_data,
	},
};
#endif

#ifdef CONFIG_SENSOR_S5K5AAFA
static struct msm_camera_sensor_flash_data flash_s5k5aafa = {
//	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_type = MSM_CAMERA_FLASH_NONE,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k5aafa_data = {
	.sensor_name    = "s5k5aafa",
	.sensor_reset   = 0,
	.sensor_pwd     = 85,
	.vcm_pwd        = 1,
	.vcm_enable		= 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_s5k5aafa,
	.csi_if         = 0
};
static struct platform_device msm_camera_sensor_s5k5aafa = {
	.name  	= "msm_camera_s5k5aafa",
	.dev   	= {
		.platform_data = &msm_camera_sensor_s5k5aafa_data,
	},
};
#endif

#ifdef CONFIG_S5K5CCGX
static struct msm_camera_sensor_flash_data flash_s5k5ccgx = {
	.flash_type = MSM_CAMERA_FLASH_LED,
//	.flash_type = MSM_CAMERA_FLASH_NONE,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k5ccgx_data = {
	.sensor_name    = "s5k5ccgx",
	.sensor_reset   = 0,
	.sensor_pwd     = 85,
	.vcm_pwd        = 1,
	.vcm_enable		= 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_s5k5ccgx,
	.csi_if         = 0
};
static struct platform_device msm_camera_sensor_s5k5ccgx = {
	.name  	= "msm_camera_s5k5ccgx",
	.dev   	= {
		.platform_data = &msm_camera_sensor_s5k5ccgx_data,
	},
};
#endif

#ifdef CONFIG_SR030PC30
static struct msm_camera_sensor_flash_data flash_sr030pc30 = {
//	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_type = MSM_CAMERA_FLASH_NONE,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_sr030pc30_data = {
	.sensor_name    = "sr030pc30",
	.sensor_reset   = 0,
	.sensor_pwd     = 3,
	.vcm_pwd        = 1,
	.vcm_enable		= 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_sr030pc30,
	.csi_if         = 0
};
static struct platform_device msm_camera_sensor_sr030pc30 = {
	.name  	= "msm_camera_sr030pc30",
	.dev   	= {
		.platform_data = &msm_camera_sensor_sr030pc30_data,
	},
};

static struct i2c_gpio_platform_data VTcamera_i2c_gpio_data_02 = {
	.sda_pin    = 174,
	.scl_pin    = 177,
};

static struct platform_device VTcamera_i2c_gpio_device_02 = {  
	.name       = "i2c-gpio",
	.id     = 20,
	.dev        = {
		.platform_data  = &VTcamera_i2c_gpio_data_02,
	},
};

static struct i2c_gpio_platform_data VTcamera_i2c_gpio_data_03 = {
	.sda_pin    = 177,
	.scl_pin    = 174,
};

static struct platform_device VTcamera_i2c_gpio_device_03 = {  
	.name       = "i2c-gpio",
	.id     = 20,
	.dev        = {
		.platform_data  = &VTcamera_i2c_gpio_data_03,
	},
};

static struct i2c_board_info msm_VTcamera_boardinfo_02[] = {
#ifdef CONFIG_S5K6AAFX
  {
			I2C_BOARD_INFO("s5k6aafx", 0x78>>1),
  },	
#endif
#ifdef CONFIG_SR030PC30
  {
			I2C_BOARD_INFO("sr030pc30", 0x60>>1),
  },	
#endif
};
#endif

#ifdef CONFIG_S5K3E2FX
static struct msm_camera_sensor_flash_data flash_s5k3e2fx = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k3e2fx_data = {
	.sensor_name    = "s5k3e2fx",
	.sensor_reset   = 0,
	.sensor_pwd     = 85,
	.vcm_pwd        = 1,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_s5k3e2fx,
	.csi_if         = 0
};

static struct platform_device msm_camera_sensor_s5k3e2fx = {
	.name      = "msm_camera_s5k3e2fx",
	.dev       = {
		.platform_data = &msm_camera_sensor_s5k3e2fx_data,
	},
};
#endif

#ifdef CONFIG_MSM_GEMINI
static struct resource msm_gemini_resources[] = {
	{
		.start  = 0xA3A00000,
		.end    = 0xA3A00000 + 0x0150 - 1,
		.flags  = IORESOURCE_MEM,
	},
	{
		.start  = INT_JPEG,
		.end    = INT_JPEG,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device msm_gemini_device = {
	.name           = "msm_gemini",
	.resource       = msm_gemini_resources,
	.num_resources  = ARRAY_SIZE(msm_gemini_resources),
};
#endif

#ifdef CONFIG_MSM_VPE
static struct resource msm_vpe_resources[] = {
	{
		.start	= 0xAD200000,
		.end	= 0xAD200000 + SZ_1M - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_VPE,
		.end	= INT_VPE,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device msm_vpe_device = {
       .name = "msm_vpe",
       .id   = 0,
       .num_resources = ARRAY_SIZE(msm_vpe_resources),
       .resource = msm_vpe_resources,
};
#endif

#endif /*CONFIG_MSM_CAMERA*/

static struct platform_device msm_vibrator_device = {
	.name 		    = "msm_vibrator",
	.id		    = -1,
};

static uint32_t vibrator_device_gpio_config[] = {
	GPIO_CFG(133, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(16, 3, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
};
static int __init vibrator_device_gpio_init(void)
{

	config_gpio_table(vibrator_device_gpio_config,
		ARRAY_SIZE(vibrator_device_gpio_config));

	return 0;
}




static struct i2c_gpio_platform_data amp_i2c_gpio_data = {
	.sda_pin    = 33,
	.scl_pin    = 31,
};

static struct platform_device amp_i2c_gpio_device = {  
	.name       = "i2c-gpio",
	.id     = 9,
	.dev        = {
		.platform_data  = &amp_i2c_gpio_data,
	},
};

#ifdef CONFIG_SENSORS_YDA165
static struct snd_set_ampgain init_ampgain[] = {
	[0] = {
		.in1_gain = 0,
		.in2_gain = 2,
		.hp_att = 31,
		.hp_gainup = 1,
		.sp_att = 31,
		.sp_gainup = 0,
	},
	[1] = {
		.in1_gain = 0,
		.in2_gain = 2,
		.hp_att = 31,
		.hp_gainup = 0,
		.sp_att = 31,
		.sp_gainup = 0,
	},
};
static struct yda165_i2c_data yda165_data = {
	.ampgain = init_ampgain,
	.num_modes = ARRAY_SIZE(init_ampgain),
};

static struct i2c_board_info yamahaamp_boardinfo[] = {
	{
		I2C_BOARD_INFO("yda165", 0xD8 >> 1),
		.platform_data = &yda165_data,
	},
};
#endif

#ifdef CONFIG_SENSORS_MAX9877
static struct i2c_board_info maxamp_boardinfo[] = {
	{
		I2C_BOARD_INFO("max9877", 0x9A >> 1),
	},
};
#endif

#ifdef CONFIG_SENSORS_MAX9879
/*
static struct platform_device amp_device_max9879 = {
	.name = "max9879_amp",
	.id = -1,
};
*/
static struct i2c_board_info maxamp_boardinfo[] = {
	{
		I2C_BOARD_INFO("max9879_i2c", 0x9A >> 1),
	},
};
#endif

#ifdef CONFIG_MSM7KV2_AUDIO
static int __init snddev_poweramp_gpio_init(void)
{
	int rc;
	struct vreg *vreg_s3_1;

	vreg_s3_1 = vreg_get(NULL, "s3");		// VREG_1.8V
	if (IS_ERR(vreg_s3_1)) {
		rc = PTR_ERR(vreg_s3_1);
		printk(KERN_ERR "%s: vreg get failed (%d)\n", __func__, rc);
		return rc;
	}
	rc = vreg_enable(vreg_s3_1);
	if (rc)
		pr_err("%s: vreg_enable(s3) failed (%d)\n", __func__, rc);
	return rc;
}

void msm_snddev_tx_route_config(void)
{
	pr_info("%s micbias on\n",__func__);
	gpio_tlmm_config(GPIO_CFG(MSM_GPIO_MICBIAS_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_set_value_cansleep(MSM_GPIO_MICBIAS_EN, 1);

	return;
}

void msm_snddev_tx_route_deconfig(void)
{
	pr_info("%s micbias off\n",__func__);
	gpio_set_value_cansleep(MSM_GPIO_MICBIAS_EN, 0);

	return;
}

void msm_snddev_headset_tx_route_config(void)
{
	pr_info("%s micbias on\n",__func__);
	//gpio_set_value_cansleep(MSM_GPIO_EAR_MICBIAS_EN, 1);

	return;
}

void msm_snddev_headset_tx_route_deconfig(void)
{
	pr_info("%s micbias off\n",__func__);
	//gpio_set_value_cansleep(MSM_GPIO_EAR_MICBIAS_EN, 0);

	return;
}

void msm_snddev_aux_dock_rx_route_config(void)
{
	pr_info("%s aux docking on\n",__func__);
  
//  if(system_rev >= 8)
//    gpio_set_value_cansleep(163, 1);

	return;
}

void msm_snddev_aux_dock_rx_route_deconfig(void)
{
	pr_info("%s aux docking off\n",__func__);

//  if(system_rev >= 8)
//    gpio_set_value_cansleep(163, 0);

	return;
}

void msm_snddev_poweramp_on(void)
{
	gpio_set_value(82, 1);	/* enable spkr poweramp */
	pr_info("%s: power on amplifier\n", __func__);
}

void msm_snddev_poweramp_off(void)
{
	gpio_set_value(82, 0);	/* disable spkr poweramp */
	pr_info("%s: power off amplifier\n", __func__);
}

void msm_snddev_poweramp_on_speaker(void)
{
#ifdef CONFIG_SENSORS_MAX9879
	max9879_i2c_speaker_onoff(1);
	gpio_set_value(167, 1);
#endif

#ifdef CONFIG_SENSORS_MAX9877
	max9877_i2c_speaker_onoff(1);
#endif

#ifdef CONFIG_SENSORS_YDA165
	yda165_speaker_onoff(1);
#endif

	pr_info("%s: power on amplifier\n", __func__);
}

void msm_snddev_poweramp_off_speaker(void)
{
#ifdef CONFIG_SENSORS_MAX9879
	max9879_i2c_speaker_onoff(0);
#endif

#ifdef CONFIG_SENSORS_MAX9877
	max9877_i2c_speaker_onoff(0);
#endif

#ifdef CONFIG_SENSORS_YDA165
	yda165_speaker_onoff(0);
#endif
	pr_info("%s: power off amplifier\n", __func__);
}

void msm_snddev_poweramp_on_speaker_incall(void)
{
#ifdef CONFIG_SENSORS_MAX9879
	max9879_i2c_speaker_onoff(1);
	gpio_set_value(167, 1);
#endif

#ifdef CONFIG_SENSORS_MAX9877
	max9877_i2c_speaker_onoff(1);
#endif

#ifdef CONFIG_SENSORS_YDA165
	yda165_speaker_onoff_incall(1);
#endif

	pr_info("%s: power on amplifier\n", __func__);
}

void msm_snddev_poweramp_off_speaker_incall(void)
{
#ifdef CONFIG_SENSORS_MAX9879
	max9879_i2c_speaker_onoff(0);
#endif

#ifdef CONFIG_SENSORS_MAX9877
	max9877_i2c_speaker_onoff(0);
#endif

#ifdef CONFIG_SENSORS_YDA165
	yda165_speaker_onoff_incall(0);
#endif
	pr_info("%s: power off amplifier\n", __func__);
}

void msm_snddev_poweramp_on_speaker_inVoip(void)
{

#ifdef CONFIG_SENSORS_YDA165
	yda165_speaker_onoff_inVoip(1);
#endif

	pr_info("%s: power on amplifier\n", __func__);
}

void msm_snddev_poweramp_off_speaker_inVoip(void)
{

#ifdef CONFIG_SENSORS_YDA165
	yda165_speaker_onoff_inVoip(0);
#endif

	pr_info("%s: power off amplifier\n", __func__);
}

void msm_snddev_poweramp_on_headset(void)
{
#ifdef CONFIG_SENSORS_YDA165
	yda165_headset_onoff(1);
#endif
}
void msm_snddev_poweramp_off_headset(void)
{
#ifdef CONFIG_SENSORS_YDA165
	yda165_headset_onoff(0);
#endif
}
void msm_snddev_poweramp_on_together(void)
{
#ifdef CONFIG_SENSORS_MAX9879
	max9879_i2c_speaker_onoff(1);
	gpio_set_value(167, 1);
#endif

#ifdef CONFIG_SENSORS_MAX9877
	max9877_i2c_speaker_headset_onoff(1);
#endif

#ifdef CONFIG_SENSORS_YDA165
	yda165_speaker_headset_onoff(1);
#endif
	pr_info("%s: power on amplifier\n", __func__);
}
void msm_snddev_poweramp_off_together(void)
{
#ifdef CONFIG_SENSORS_MAX9879
	max9879_i2c_speaker_onoff(0);
#endif
#ifdef CONFIG_SENSORS_MAX9877
	max9877_i2c_speaker_headset_onoff(0);
#endif
#ifdef CONFIG_SENSORS_YDA165
	yda165_speaker_headset_onoff(0);
#endif
	pr_info("%s: power off amplifier\n", __func__);
}

void msm_snddev_poweramp_on_tty(void)
{
#ifdef CONFIG_SENSORS_MAX9877
	max9877_i2c_tty_onoff(1);
#endif

#ifdef CONFIG_SENSORS_YDA165
	yda165_tty_onoff(1);
#endif	
	pr_info("%s: power on amplifier\n", __func__);
}

void msm_snddev_poweramp_off_tty(void)
{
#ifdef CONFIG_SENSORS_MAX9877
	max9877_i2c_tty_onoff(0);
#endif

#ifdef CONFIG_SENSORS_YDA165
	yda165_tty_onoff(0);
#endif	
	pr_info("%s: power off amplifier\n", __func__);
}

static struct vreg *snddev_vreg_ncp, *snddev_vreg_gp4;

void msm_snddev_hsed_voltage_on(void)
{
	int rc;

	snddev_vreg_gp4 = vreg_get(NULL, "gp4");
	if (IS_ERR(snddev_vreg_gp4)) {
		pr_err("%s: vreg_get(%s) failed (%ld)\n",
		__func__, "gp4", PTR_ERR(snddev_vreg_gp4));
		return;
	}
	rc = vreg_enable(snddev_vreg_gp4);
	if (rc)
		pr_err("%s: vreg_enable(gp4) failed (%d)\n", __func__, rc);

	snddev_vreg_ncp = vreg_get(NULL, "ncp");
	if (IS_ERR(snddev_vreg_ncp)) {
		pr_err("%s: vreg_get(%s) failed (%ld)\n",
		__func__, "ncp", PTR_ERR(snddev_vreg_ncp));
		return;
	}
	rc = vreg_enable(snddev_vreg_ncp);
	if (rc)
		pr_err("%s: vreg_enable(ncp) failed (%d)\n", __func__, rc);
}

void msm_snddev_hsed_voltage_off(void)
{
	int rc;

	if (IS_ERR(snddev_vreg_ncp)) {
		pr_err("%s: vreg_get(%s) failed (%ld)\n",
		__func__, "ncp", PTR_ERR(snddev_vreg_ncp));
		return;
	}
	rc = vreg_disable(snddev_vreg_ncp);
	if (rc)
		pr_err("%s: vreg_disable(ncp) failed (%d)\n", __func__, rc);
	vreg_put(snddev_vreg_ncp);

	if (IS_ERR(snddev_vreg_gp4)) {
		pr_err("%s: vreg_get(%s) failed (%ld)\n",
		__func__, "gp4", PTR_ERR(snddev_vreg_gp4));
		return;
	}
	rc = vreg_disable(snddev_vreg_gp4);
	if (rc)
		pr_err("%s: vreg_disable(gp4) failed (%d)\n", __func__, rc);

	vreg_put(snddev_vreg_gp4);

}

static unsigned aux_pcm_gpio_on[] = {
	GPIO_CFG(138, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),   /* PCM_DOUT */
	GPIO_CFG(139, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),   /* PCM_DIN  */
	GPIO_CFG(140, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),   /* PCM_SYNC */
	GPIO_CFG(141, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),   /* PCM_CLK  */
};

static int __init aux_pcm_gpio_init(void)
{
	int pin, rc;

	pr_info("aux_pcm_gpio_init \n");
	for (pin = 0; pin < ARRAY_SIZE(aux_pcm_gpio_on); pin++) {
		rc = gpio_tlmm_config(aux_pcm_gpio_on[pin],
					GPIO_CFG_ENABLE);
		if (rc) {
			printk(KERN_ERR
				"%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, aux_pcm_gpio_on[pin], rc);
		}
	}
	return rc;
}

static struct msm_gpio mi2s_clk_gpios[] = {
	{ GPIO_CFG(145, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_SCLK"},
	{ GPIO_CFG(144, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_WS"},
	{ GPIO_CFG(120, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_MCLK_A"},
};

static struct msm_gpio mi2s_rx_data_lines_gpios[] = {
	{ GPIO_CFG(121, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_DATA_SD0_A"},
	{ GPIO_CFG(122, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_DATA_SD1_A"},
	{ GPIO_CFG(123, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_DATA_SD2_A"},
	{ GPIO_CFG(146, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_DATA_SD3"},
};

static struct msm_gpio mi2s_tx_data_lines_gpios[] = {
	{ GPIO_CFG(146, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_DATA_SD3"},
};

int mi2s_config_clk_gpio(void)
{
	int rc = 0;

	rc = msm_gpios_request_enable(mi2s_clk_gpios,
			ARRAY_SIZE(mi2s_clk_gpios));
	if (rc) {
		pr_err("%s: enable mi2s clk gpios  failed\n",
					__func__);
		return rc;
	}
	return 0;
}

int  mi2s_unconfig_data_gpio(u32 direction, u8 sd_line_mask)
{
	int i, rc = 0;
	sd_line_mask &= MI2S_SD_LINE_MASK;

	switch (direction) {
	case DIR_TX:
		msm_gpios_disable_free(mi2s_tx_data_lines_gpios, 1);
		break;
	case DIR_RX:
		i = 0;
		while (sd_line_mask) {
			if (sd_line_mask & 0x1)
				msm_gpios_disable_free(
					mi2s_rx_data_lines_gpios + i , 1);
			sd_line_mask = sd_line_mask >> 1;
			i++;
		}
		break;
	default:
		pr_err("%s: Invaild direction  direction = %u\n",
						__func__, direction);
		rc = -EINVAL;
		break;
	}
	return rc;
}

int mi2s_config_data_gpio(u32 direction, u8 sd_line_mask)
{
	int i , rc = 0;
	u8 sd_config_done_mask = 0;

	sd_line_mask &= MI2S_SD_LINE_MASK;

	switch (direction) {
	case DIR_TX:
		if ((sd_line_mask & MI2S_SD_0) || (sd_line_mask & MI2S_SD_1) ||
		   (sd_line_mask & MI2S_SD_2) || !(sd_line_mask & MI2S_SD_3)) {
			pr_err("%s: can not use SD0 or SD1 or SD2 for TX"
				".only can use SD3. sd_line_mask = 0x%x\n",
				__func__ , sd_line_mask);
			rc = -EINVAL;
		} else {
			rc = msm_gpios_request_enable(mi2s_tx_data_lines_gpios,
							 1);
			if (rc)
				pr_err("%s: enable mi2s gpios for TX failed\n",
					   __func__);
		}
		break;
	case DIR_RX:
		i = 0;
		while (sd_line_mask && (rc == 0)) {
			if (sd_line_mask & 0x1) {
				rc = msm_gpios_request_enable(
					mi2s_rx_data_lines_gpios + i , 1);
				if (rc) {
					pr_err("%s: enable mi2s gpios for"
					 "RX failed.  SD line = %s\n",
					 __func__,
					 (mi2s_rx_data_lines_gpios + i)->label);
					mi2s_unconfig_data_gpio(DIR_RX,
						sd_config_done_mask);
				} else
					sd_config_done_mask |= (1 << i);
			}
			sd_line_mask = sd_line_mask >> 1;
			i++;
		}
		break;
	default:
		pr_err("%s: Invaild direction  direction = %u\n",
			__func__, direction);
		rc = -EINVAL;
		break;
	}
	return rc;
}

int mi2s_unconfig_clk_gpio(void)
{
	msm_gpios_disable_free(mi2s_clk_gpios, ARRAY_SIZE(mi2s_clk_gpios));
	return 0;
}

#endif /* CONFIG_MSM7KV2_AUDIO */

#ifdef CONFIG_SENSORS_FSA9480
#define micro_usb_i2c_gpio_sda 35
#define micro_usb_i2c_gpio_scl 36
static unsigned fsa9480_gpio_on[] = {
	GPIO_CFG(micro_usb_i2c_gpio_sda, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
	GPIO_CFG(micro_usb_i2c_gpio_scl, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
};

static int __init fsa9480_gpio_init(void)
{
	int pin, rc;

	pr_info("fsa9480_gpio_init \n");
	for (pin = 0; pin < ARRAY_SIZE(fsa9480_gpio_on); pin++) {
		rc = gpio_tlmm_config(fsa9480_gpio_on[pin],
					GPIO_CFG_ENABLE);
		if (rc) {
			printk(KERN_ERR
				"%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, fsa9480_gpio_on[pin], rc);
		}
	}
	return rc;
}
#endif

#ifdef CONFIG_MAX17043_FUEL_GAUGE

static unsigned fg17043_gpio_on[] = {
	GPIO_CFG(82, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
	GPIO_CFG(181, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
};

static int __init fg17043_gpio_init(void)
{
	int pin, rc;

	pr_info("fg17043_gpio_init \n");
	
	for (pin = 0; pin < ARRAY_SIZE(fg17043_gpio_on); pin++) {
		rc = gpio_tlmm_config(fg17043_gpio_on[pin],
					GPIO_CFG_ENABLE);
		if (rc) {
			printk(KERN_ERR
				"%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, fg17043_gpio_on[pin], rc);
		}
	}
	
	return rc;
}
#endif

static int __init buses_init(void)
{
	if (gpio_tlmm_config(GPIO_CFG(PMIC_GPIO_INT, 1, GPIO_CFG_INPUT,
				  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE))
		pr_err("%s: gpio_tlmm_config (gpio=%d) failed\n",
		       __func__, PMIC_GPIO_INT);

	pm8058_7x30_data.sub_devices[PM8058_SUBDEV_KPD].platform_data
		= &vital2_keypad_data;
	pm8058_7x30_data.sub_devices[PM8058_SUBDEV_KPD].data_size
		= sizeof(vital2_keypad_data);

	i2c_register_board_info(6 /* I2C_SSBI ID */, pm8058_boardinfo,
				ARRAY_SIZE(pm8058_boardinfo));

	/* configure pmic leds */
	pm8058_7x30_data.sub_devices[PM8058_SUBDEV_LED].platform_data
		= &pm8058_fluid_flash_leds_data;
	pm8058_7x30_data.sub_devices[PM8058_SUBDEV_LED].data_size
		= sizeof(pm8058_fluid_flash_leds_data);

	return 0;
}

#define TIMPANI_RESET_GPIO	1

static struct vreg *vreg_marimba_1;
static struct vreg *vreg_marimba_2;
static struct vreg *vreg_marimba_3;

static struct msm_gpio timpani_reset_gpio_cfg[] = {
{ GPIO_CFG(TIMPANI_RESET_GPIO, 0, GPIO_CFG_OUTPUT,
	GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "timpani_reset"} };

static int config_timpani_reset(void)
{
	int rc;

	rc = msm_gpios_request_enable(timpani_reset_gpio_cfg,
				ARRAY_SIZE(timpani_reset_gpio_cfg));
	if (rc < 0) {
		printk(KERN_ERR
			"%s: msm_gpios_request_enable failed (%d)\n",
				__func__, rc);
	}
	return rc;
}

static unsigned int msm_timpani_setup_power(void)
{
	int rc;

	pr_info("%s\n", __func__);
	return 0; //vital2.sleep.temp  msm_timpani_shutdown_power does not get called..
#if 0
	rc = config_timpani_reset();
	if (rc < 0)
		goto out;
#endif
	rc = vreg_enable(vreg_marimba_1);
	if (rc) {
		printk(KERN_ERR "%s: vreg_enable() = %d\n",
					__func__, rc);
		goto out;
	}
	rc = vreg_enable(vreg_marimba_2);
	if (rc) {
		printk(KERN_ERR "%s: vreg_enable() = %d\n",
					__func__, rc);
		goto fail_disable_vreg_marimba_1;
	}
#if 0
	rc = gpio_direction_output(TIMPANI_RESET_GPIO, 1);
	if (rc < 0) {
		printk(KERN_ERR
			"%s: gpio_direction_output failed (%d)\n",
				__func__, rc);
		msm_gpios_free(timpani_reset_gpio_cfg,
				ARRAY_SIZE(timpani_reset_gpio_cfg));
		vreg_disable(vreg_marimba_2);
	} else
		goto out;
#endif	


fail_disable_vreg_marimba_1:
	vreg_disable(vreg_marimba_1);

out:
	return rc;
};

static void msm_timpani_shutdown_power(void)
{
	int rc;

	pr_info("%s\n", __func__);
	
	rc = vreg_disable(vreg_marimba_1);
	if (rc) {
		printk(KERN_ERR "%s: return val: %d\n",
					__func__, rc);
	}
	rc = vreg_disable(vreg_marimba_2);
	if (rc) {
		printk(KERN_ERR "%s: return val: %d\n",
					__func__, rc);
	}
#if 0
	rc = gpio_direction_output(TIMPANI_RESET_GPIO, 0);
	if (rc < 0) {
		printk(KERN_ERR
			"%s: gpio_direction_output failed (%d)\n",
				__func__, rc);
	}

	msm_gpios_free(timpani_reset_gpio_cfg,
				   ARRAY_SIZE(timpani_reset_gpio_cfg));
#endif	
};

static unsigned int msm_bahama_setup_power(void)
{
	int rc;

	rc = vreg_enable(vreg_marimba_3);
	if (rc) {
		printk(KERN_ERR "%s: vreg_enable() = %d\n",
				__func__, rc);
	}

	return rc;
};

static unsigned int msm_bahama_shutdown_power(int value)
{
	int rc = 0;

	if (value != BAHAMA_ID) {
		rc = vreg_disable(vreg_marimba_3);
		if (rc) {
			printk(KERN_ERR "%s: return val: %d\n",
					__func__, rc);
		}
	}

	return rc;
};

static struct msm_gpio marimba_svlte_config_clock[] = {
	{ GPIO_CFG(34, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		"MARIMBA_SVLTE_CLOCK_ENABLE" },
};

static unsigned int msm_marimba_gpio_config_svlte(int gpio_cfg_marimba)
{
	if (machine_is_msm8x55_svlte_surf() ||
		machine_is_msm8x55_svlte_ffa()) {
		if (gpio_cfg_marimba)
			gpio_set_value(GPIO_PIN
				(marimba_svlte_config_clock->gpio_cfg), 1);
		else
			gpio_set_value(GPIO_PIN
				(marimba_svlte_config_clock->gpio_cfg), 0);
	}

	return 0;
};

static unsigned int msm_marimba_setup_power(void)
{
	int rc;

	rc = vreg_enable(vreg_marimba_1);
	if (rc) {
		printk(KERN_ERR "%s: vreg_enable() = %d \n",
					__func__, rc);
		goto out;
	}
	rc = vreg_enable(vreg_marimba_2);
	if (rc) {
		printk(KERN_ERR "%s: vreg_enable() = %d \n",
					__func__, rc);
		goto out;
	}

out:
	return rc;
};

static void msm_marimba_shutdown_power(void)
{
	int rc;

	rc = vreg_disable(vreg_marimba_1);
	if (rc) {
		printk(KERN_ERR "%s: return val: %d \n",
					__func__, rc);
	}
	rc = vreg_disable(vreg_marimba_2);
	if (rc) {
		printk(KERN_ERR "%s: return val: %d \n",
					__func__, rc);
	}
};

static int fm_radio_setup(struct marimba_fm_platform_data *pdata)
{
	int rc;
	uint32_t irqcfg;
	const char *id = "FMPW";

	pdata->vreg_s2 = vreg_get(NULL, "s2");
	if (IS_ERR(pdata->vreg_s2)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			__func__, PTR_ERR(pdata->vreg_s2));
		return -1;
	}

	rc = pmapp_vreg_level_vote(id, PMAPP_VREG_S2, 1300);
	if (rc < 0) {
		printk(KERN_ERR "%s: voltage level vote failed (%d)\n",
			__func__, rc);
		return rc;
	}

	rc = vreg_enable(pdata->vreg_s2);
	if (rc) {
		printk(KERN_ERR "%s: vreg_enable() = %d \n",
					__func__, rc);
		return rc;
	}

	rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
					  PMAPP_CLOCK_VOTE_ON);
	if (rc < 0) {
		printk(KERN_ERR "%s: clock vote failed (%d)\n",
			__func__, rc);
		goto fm_clock_vote_fail;
	}
	irqcfg = GPIO_CFG(147, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
					GPIO_CFG_2MA);
	rc = gpio_tlmm_config(irqcfg, GPIO_CFG_ENABLE);
	if (rc) {
		printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, irqcfg, rc);
		rc = -EIO;
		goto fm_gpio_config_fail;

	}
	return 0;
fm_gpio_config_fail:
	pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
				  PMAPP_CLOCK_VOTE_OFF);
fm_clock_vote_fail:
	vreg_disable(pdata->vreg_s2);
	return rc;

};

static void fm_radio_shutdown(struct marimba_fm_platform_data *pdata)
{
	int rc;
	const char *id = "FMPW";
	uint32_t irqcfg = GPIO_CFG(147, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
					GPIO_CFG_2MA);
	rc = gpio_tlmm_config(irqcfg, GPIO_CFG_ENABLE);
	if (rc) {
		printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, irqcfg, rc);
	}
	rc = vreg_disable(pdata->vreg_s2);
	if (rc) {
		printk(KERN_ERR "%s: return val: %d \n",
					__func__, rc);
	}
	rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
					  PMAPP_CLOCK_VOTE_OFF);
	if (rc < 0)
		printk(KERN_ERR "%s: clock_vote return val: %d \n",
						__func__, rc);
	rc = pmapp_vreg_level_vote(id, PMAPP_VREG_S2, 0);
	if (rc < 0)
		printk(KERN_ERR "%s: vreg level vote return val: %d \n",
						__func__, rc);
}

static struct marimba_fm_platform_data marimba_fm_pdata = {
	.fm_setup =  fm_radio_setup,
	.fm_shutdown = fm_radio_shutdown,
	.irq = MSM_GPIO_TO_INT(147),
	.vreg_s2 = NULL,
	.vreg_xo_out = NULL,
};


/* Slave id address for FM/CDC/QMEMBIST
 * Values can be programmed using Marimba slave id 0
 * should there be a conflict with other I2C devices
 * */
#define MARIMBA_SLAVE_ID_FM_ADDR	0x2A
#define MARIMBA_SLAVE_ID_CDC_ADDR	0x77
#define MARIMBA_SLAVE_ID_QMEMBIST_ADDR	0X66

#define BAHAMA_SLAVE_ID_FM_ADDR         0x2A
#define BAHAMA_SLAVE_ID_QMEMBIST_ADDR   0x7B

static struct vreg *vreg_codec_s4;
static int msm_marimba_codec_power(int vreg_on)
{
	int rc = 0;

	if (!vreg_codec_s4) {

		vreg_codec_s4 = vreg_get(NULL, "s4");

		if (IS_ERR(vreg_codec_s4)) {
			printk(KERN_ERR "%s: vreg_get() failed (%ld)\n",
				__func__, PTR_ERR(vreg_codec_s4));
			rc = PTR_ERR(vreg_codec_s4);
			goto  vreg_codec_s4_fail;
		}
	}

	if (vreg_on) {
		rc = vreg_enable(vreg_codec_s4);
		if (rc)
			printk(KERN_ERR "%s: vreg_enable() = %d \n",
					__func__, rc);
		goto vreg_codec_s4_fail;
	} else {
		rc = vreg_disable(vreg_codec_s4);
		if (rc)
			printk(KERN_ERR "%s: vreg_disable() = %d \n",
					__func__, rc);
		goto vreg_codec_s4_fail;
	}

vreg_codec_s4_fail:
	return rc;
}

static struct marimba_codec_platform_data mariba_codec_pdata = {
	.marimba_codec_power =  msm_marimba_codec_power,
#ifdef CONFIG_MARIMBA_CODEC
	.snddev_profile_init = msm_snddev_init,
#endif
};

static struct marimba_platform_data marimba_pdata = {
	.slave_id[MARIMBA_SLAVE_ID_FM]       = MARIMBA_SLAVE_ID_FM_ADDR,
	.slave_id[MARIMBA_SLAVE_ID_CDC]	     = MARIMBA_SLAVE_ID_CDC_ADDR,
	.slave_id[MARIMBA_SLAVE_ID_QMEMBIST] = MARIMBA_SLAVE_ID_QMEMBIST_ADDR,
	.slave_id[SLAVE_ID_BAHAMA_FM]        = BAHAMA_SLAVE_ID_FM_ADDR,
	.slave_id[SLAVE_ID_BAHAMA_QMEMBIST]  = BAHAMA_SLAVE_ID_QMEMBIST_ADDR,
	.marimba_setup = msm_marimba_setup_power,
	.marimba_shutdown = msm_marimba_shutdown_power,
	.bahama_setup = msm_bahama_setup_power,
	.bahama_shutdown = msm_bahama_shutdown_power,
//	.marimba_gpio_config = msm_marimba_gpio_config_svlte,
	.fm = &marimba_fm_pdata,
	.codec = &mariba_codec_pdata,
};

static void __init msm7x30_init_marimba(void)
{
	int rc;

	vreg_marimba_1 = vreg_get(NULL, "s3");
	if (IS_ERR(vreg_marimba_1)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			__func__, PTR_ERR(vreg_marimba_1));
		return;
	}
	rc = vreg_set_level(vreg_marimba_1, 1800);

	vreg_marimba_2 = vreg_get(NULL, "gp16");
	if (IS_ERR(vreg_marimba_1)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			__func__, PTR_ERR(vreg_marimba_1));
		return;
	}
	rc = vreg_set_level(vreg_marimba_2, 1200);

	vreg_marimba_3 = vreg_get(NULL, "usb2");
	if (IS_ERR(vreg_marimba_3)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			__func__, PTR_ERR(vreg_marimba_3));
		return;
	}
	rc = vreg_set_level(vreg_marimba_3, 1800);
}

static struct marimba_codec_platform_data timpani_codec_pdata = {
	.marimba_codec_power =  msm_marimba_codec_power,
#ifdef CONFIG_TIMPANI_CODEC
	.snddev_profile_init = msm_snddev_init_timpani,
#endif
};

static struct marimba_platform_data timpani_pdata = {
	.slave_id[MARIMBA_SLAVE_ID_CDC]	= MARIMBA_SLAVE_ID_CDC_ADDR,
	.slave_id[MARIMBA_SLAVE_ID_QMEMBIST] = MARIMBA_SLAVE_ID_QMEMBIST_ADDR,
	.marimba_setup = msm_timpani_setup_power,
	.marimba_shutdown = msm_timpani_shutdown_power,
	.codec = &timpani_codec_pdata,
//	.tsadc = &marimba_tsadc_pdata,
};

#define TIMPANI_I2C_SLAVE_ADDR	0xD

static struct i2c_board_info msm_i2c_gsbi7_timpani_info[] = {
	{
		I2C_BOARD_INFO("timpani", TIMPANI_I2C_SLAVE_ADDR),
		.platform_data = &timpani_pdata,
	},
};

#ifdef CONFIG_MSM7KV2_AUDIO
static struct resource msm_aictl_resources[] = {
	{
		.name = "aictl",
		.start = 0xa5000100,
		.end = 0xa5000100,
		.flags = IORESOURCE_MEM,
	}
};

static struct resource msm_mi2s_resources[] = {
	{
		.name = "hdmi",
		.start = 0xac900000,
		.end = 0xac900038,
		.flags = IORESOURCE_MEM,
	},
	{
		.name = "codec_rx",
		.start = 0xac940040,
		.end = 0xac940078,
		.flags = IORESOURCE_MEM,
	},
	{
		.name = "codec_tx",
		.start = 0xac980080,
		.end = 0xac9800B8,
		.flags = IORESOURCE_MEM,
	}

};

static struct msm_lpa_platform_data lpa_pdata = {
	.obuf_hlb_size = 0x2BFF8,
	.dsp_proc_id = 0,
	.app_proc_id = 2,
	.nosb_config = {
		.llb_min_addr = 0,
		.llb_max_addr = 0x3ff8,
		.sb_min_addr = 0,
		.sb_max_addr = 0,
	},
	.sb_config = {
		.llb_min_addr = 0,
		.llb_max_addr = 0x37f8,
		.sb_min_addr = 0x3800,
		.sb_max_addr = 0x3ff8,
	}
};

static struct resource msm_lpa_resources[] = {
	{
		.name = "lpa",
		.start = 0xa5000000,
		.end = 0xa50000a0,
		.flags = IORESOURCE_MEM,
	}
};

static struct resource msm_aux_pcm_resources[] = {

	{
		.name = "aux_codec_reg_addr",
		.start = 0xac9c00c0,
		.end = 0xac9c00c8,
		.flags = IORESOURCE_MEM,
	},
	{
		.name   = "aux_pcm_dout",
		.start  = 138,
		.end    = 138,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_din",
		.start  = 139,
		.end    = 139,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_syncout",
		.start  = 140,
		.end    = 140,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_clkin_a",
		.start  = 141,
		.end    = 141,
		.flags  = IORESOURCE_IO,
	},
};

static struct platform_device msm_aux_pcm_device = {
	.name   = "msm_aux_pcm",
	.id     = 0,
	.num_resources  = ARRAY_SIZE(msm_aux_pcm_resources),
	.resource       = msm_aux_pcm_resources,
};

struct platform_device msm_aictl_device = {
	.name = "audio_interct",
	.id   = 0,
	.num_resources = ARRAY_SIZE(msm_aictl_resources),
	.resource = msm_aictl_resources,
};

struct platform_device msm_mi2s_device = {
	.name = "mi2s",
	.id   = 0,
	.num_resources = ARRAY_SIZE(msm_mi2s_resources),
	.resource = msm_mi2s_resources,
};

struct platform_device msm_lpa_device = {
	.name = "lpa",
	.id   = 0,
	.num_resources = ARRAY_SIZE(msm_lpa_resources),
	.resource = msm_lpa_resources,
	.dev		= {
		.platform_data = &lpa_pdata,
	},
};
#endif /* CONFIG_MSM7KV2_AUDIO */

#define DEC0_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC1_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
 #define DEC2_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
 #define DEC3_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC4_FORMAT (1<<MSM_ADSP_CODEC_MIDI)

static unsigned int dec_concurrency_table[] = {
	/* Audio LP */
	0,
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_MODE_LP)|
	(1<<MSM_ADSP_OP_DM)),

	/* Concurrency 1 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),

	 /* Concurrency 2 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),

	/* Concurrency 3 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),

	/* Concurrency 4 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),

	/* Concurrency 5 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),

	/* Concurrency 6 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
};

#define DEC_INFO(name, queueid, decid, nr_codec) { .module_name = name, \
	.module_queueid = queueid, .module_decid = decid, \
	.nr_codec_support = nr_codec}

#define DEC_INSTANCE(max_instance_same, max_instance_diff) { \
	.max_instances_same_dec = max_instance_same, \
	.max_instances_diff_dec = max_instance_diff}

static struct msm_adspdec_info dec_info_list[] = {
	DEC_INFO("AUDPLAY4TASK", 17, 4, 1),  /* AudPlay4BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY3TASK", 16, 3, 11),  /* AudPlay3BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY2TASK", 15, 2, 11),  /* AudPlay2BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY1TASK", 14, 1, 11),  /* AudPlay1BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY0TASK", 13, 0, 11), /* AudPlay0BitStreamCtrlQueue */
};

static struct dec_instance_table dec_instance_list[][MSM_MAX_DEC_CNT] = {
	/* Non Turbo Mode */
	{
		DEC_INSTANCE(4, 3), /* WAV */
		DEC_INSTANCE(4, 3), /* ADPCM */
		DEC_INSTANCE(4, 2), /* MP3 */
		DEC_INSTANCE(0, 0), /* Real Audio */
		DEC_INSTANCE(4, 2), /* WMA */
		DEC_INSTANCE(3, 2), /* AAC */
		DEC_INSTANCE(0, 0), /* Reserved */
		DEC_INSTANCE(0, 0), /* MIDI */
		DEC_INSTANCE(4, 3), /* YADPCM */
		DEC_INSTANCE(4, 3), /* QCELP */
		DEC_INSTANCE(4, 3), /* AMRNB */
		DEC_INSTANCE(1, 1), /* AMRWB/WB+ */
		DEC_INSTANCE(4, 3), /* EVRC */
		DEC_INSTANCE(1, 1), /* WMAPRO */
	},
	/* Turbo Mode */
	{
		DEC_INSTANCE(4, 3), /* WAV */
		DEC_INSTANCE(4, 3), /* ADPCM */
		DEC_INSTANCE(4, 3), /* MP3 */
		DEC_INSTANCE(0, 0), /* Real Audio */
		DEC_INSTANCE(4, 3), /* WMA */
		DEC_INSTANCE(4, 3), /* AAC */
		DEC_INSTANCE(0, 0), /* Reserved */
		DEC_INSTANCE(0, 0), /* MIDI */
		DEC_INSTANCE(4, 3), /* YADPCM */
		DEC_INSTANCE(4, 3), /* QCELP */
		DEC_INSTANCE(4, 3), /* AMRNB */
		DEC_INSTANCE(2, 3), /* AMRWB/WB+ */
		DEC_INSTANCE(4, 3), /* EVRC */
		DEC_INSTANCE(1, 2), /* WMAPRO */
	},
};

static struct msm_adspdec_database msm_device_adspdec_database = {
	.num_dec = ARRAY_SIZE(dec_info_list),
	.num_concurrency_support = (ARRAY_SIZE(dec_concurrency_table) / \
					ARRAY_SIZE(dec_info_list)),
	.dec_concurrency_table = dec_concurrency_table,
	.dec_info_list = dec_info_list,
	.dec_instance_list = &dec_instance_list[0][0],
};

static struct platform_device msm_device_adspdec = {
	.name = "msm_adspdec",
	.id = -1,
	.dev    = {
		.platform_data = &msm_device_adspdec_database
	},
};


static struct i2c_gpio_platform_data acc_gyro_mag_i2c_gpio_data = {
	.sda_pin    = 1,
	.scl_pin    = 0,
};

static struct platform_device acc_gyro_mag_i2c_gpio_device = {  
	.name       = "i2c-gpio",
	.id     = 8,
	.dev        = {
		.platform_data  = &acc_gyro_mag_i2c_gpio_data,
	},
};


#if defined(CONFIG_SENSORS_KR3D_ACCEL) || defined(CONFIG_INPUT_YAS_MAGNETOMETER) || defined(CONFIG_SENSORS_YAS529_MAGNETIC)

#if defined(CONFIG_INPUT_YAS_MAGNETOMETER) || defined(CONFIG_SENSORS_YAS529_MAGNETIC)
struct vreg* mag_vreg_main;
struct vreg* mag_vreg_io;
int yas_reset_n(int on)
{
	gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_MSENSE_RST), !!on);
}

int yas_vreg_en(int on)
{
	pr_info("%s: %d\n", __func__, on);
	
	if(mag_vreg_main == NULL) {
		pr_err("%s: mag_vreg_main null!\n", __func__);
		return -1;
	}
	if(mag_vreg_io == NULL) {
		pr_err("%s: mag_vreg_io null!\n", __func__);
		return -1;
	}

	if(on) {
		vreg_enable(mag_vreg_main);
		vreg_enable(mag_vreg_io);
	} else {
		vreg_disable(mag_vreg_main);
		vreg_disable(mag_vreg_io);
	}

	udelay(500);
}

static struct yas_platform_data yas_plat_data = {
	.reset_n = yas_reset_n,
	.vreg_en = yas_vreg_en,
};
#endif

#ifdef CONFIG_SENSORS_KR3D_ACCEL
int kr3d_vreg_en(int on)
{
	static struct vreg* accel_vreg_main = NULL;	
	static struct vreg* accel_vreg_io = NULL;	

	pr_info("%s: %d\n", __func__, on);

	if(accel_vreg_main == NULL) {
		if(system_rev >= 5) {
			accel_vreg_main = vreg_get(NULL, "wlan"); /* sensor source is changed to L13(wlan) from L10(gp4) for control */
		}
		else {
			accel_vreg_main = vreg_get(NULL, "gp4");
		}

		if(accel_vreg_main == NULL) {
			pr_err("%s: accel_vreg_main null!\n", __func__);
			return -1;
		}
		if(vreg_set_level(accel_vreg_main, 2600) < 0) {
			pr_err("%s: accel_vreg_main set level failed!\n", __func__);
			return -1;
		}
	}

	if(accel_vreg_io == NULL) {
		accel_vreg_io = vreg_get(NULL, "ruim");

		if(accel_vreg_io == NULL) {
			pr_err("%s: accel_vreg_io null!\n", __func__);
			return -1;
		}
		if(vreg_set_level(accel_vreg_io, 1800) < 0) {
			pr_err("%s: accel_vreg_io set level failed!\n", __func__);
			return -1;
		}
	}

	if(on) {
		vreg_enable(accel_vreg_main);
		vreg_enable(accel_vreg_io);
	} else {
		vreg_disable(accel_vreg_main);
		vreg_disable(accel_vreg_io);
	}

	udelay(500);
}

static struct kr3d_platform_data kr3d_plat_data = {
	.vreg_en = kr3d_vreg_en,
};
#endif

static struct i2c_board_info acc_gyro_mag_i2c_gpio_devices[] = {

#ifdef CONFIG_SENSORS_KR3D_ACCEL
	{
		I2C_BOARD_INFO("kr3dm_accel", 0x12 >> 1),
		.platform_data = &kr3d_plat_data,
	},
#endif
#if 0
	{
		I2C_BOARD_INFO("l3g4200d", 0x69),
		.platform_data  = &l3g4200d_i2c_gpio_data,
	},
#endif
#ifdef CONFIG_INPUT_YAS_MAGNETOMETER
	{
		I2C_BOARD_INFO("geomagnetic", 0x2E),		// 0x2E
		.platform_data = &yas_plat_data,
	},
#endif
#ifdef CONFIG_SENSORS_YAS529_MAGNETIC
	{
		I2C_BOARD_INFO("yas529", 0x2E),		// 0x2E
		.platform_data = &yas_plat_data,
	},
#endif
	
};
#endif

static struct i2c_gpio_platform_data opt_i2c_gpio_data = {
	.sda_pin    = MSM_GPIO_ALS_SDA,
	.scl_pin    = MSM_GPIO_ALS_SCL,
	.udelay = 2,
};

static struct platform_device opt_i2c_gpio_device = {
	.name       = "i2c-gpio",
	.id     = 14,
	.dev        = {
		.platform_data  = &opt_i2c_gpio_data,
	},
};

static struct vreg* opt_vreg_L10;
static struct vreg* opt_vreg_L11;

int gp2a_power_en(int on)
{
	pr_info("%s: %d\n", __func__, on);
	
	if(opt_vreg_L10 == NULL) {
		pr_err("%s: vreg L10 null!\n", __func__);
		return -1;
	}
	
	if(opt_vreg_L11 == NULL) {
		pr_err("%s: opt_vreg_L11 null!\n", __func__);
		return -1;
	}
	
	if(on) {
		vreg_enable(opt_vreg_L10);
		vreg_enable(opt_vreg_L11);
	} else {
		vreg_disable(opt_vreg_L10);
		vreg_disable(opt_vreg_L11);
	}

	/* ensure power is stablized */
	msleep(1); //udelay(500);
}

static struct gp2a_platform_data gp2a_plat_data = {
	.power_en = gp2a_power_en,
	.wakeup = 1,
};

static struct i2c_board_info opt_i2c_borad_info[] = {
	{
		I2C_BOARD_INFO("gp2a", 0x88>>1),
      	.platform_data  = &gp2a_plat_data,
	},
};

static uint32_t optical_device_gpio_config[] = {
	GPIO_CFG(PMIC_GPIO_PS_VOUT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static struct platform_device opt_gp2a = {
	.name = "gp2a-opt",
	.id = -1,
};


static int __init optical_device_init(void)
{
		int rc;

		if(system_rev >= 5) {
			opt_vreg_L10 = vreg_get(NULL, "wlan"); /* sensor source is changed to L13(wlan) from L10(gp4) for control */
		}
		else {
			opt_vreg_L10 = vreg_get(NULL, "gp4"); 
		}

		if (IS_ERR(opt_vreg_L10)) {
			rc = PTR_ERR(opt_vreg_L10);
			pr_err("%s: opt_vreg_L10 get failed (%d)\n",
				   __func__, rc);
			return rc;
		}
		rc = vreg_set_level(opt_vreg_L10, 2600);
		if (rc) {
			pr_err("%s: opt_vreg_L10 set level failed (%d)\n",
				   __func__, rc);
			return rc;
		}
		rc = vreg_enable(opt_vreg_L10);
		if (rc) {
			pr_err("%s: opt_vreg_L10 enable failed (%d)\n",
				   __func__, rc);
			return rc;
		}

	/*
	config_gpio_table(optical_device_gpio_config,
		ARRAY_SIZE(optical_device_gpio_config));
*/
		opt_vreg_L11 = vreg_get(NULL, "gp2"); 
		
		if (IS_ERR(opt_vreg_L11)) {
			rc = PTR_ERR(opt_vreg_L11);
			pr_err("%s: opt_vreg_L11 get failed (%d)\n",
				   __func__, rc);
			return rc;
		}
		rc = vreg_set_level(opt_vreg_L11, 3000);
		if (rc) {
			pr_err("%s: opt_vreg_L11 set level failed (%d)\n",
				   __func__, rc);
			return rc;
		}
		rc = vreg_enable(opt_vreg_L11);
		if (rc) {
			pr_err("%s: opt_vreg_L11 enable failed (%d)\n",
				   __func__, rc);
			return rc;
		}
			
		return 0;
}

static int __init magnetic_device_init(void)
{
	int rc;

	if(system_rev >= 5) {
		mag_vreg_main = vreg_get(NULL, "wlan"); /* sensor source is changed to L13(wlan) from L10(gp4) for control */
	}
	else {
		mag_vreg_main = vreg_get(NULL, "gp4");
	}

	if (IS_ERR(mag_vreg_main)) {
		rc = PTR_ERR(mag_vreg_main);
		pr_err("%s: mag_vreg_main get failed (%d)\n",
		       __func__, rc);
		return rc;
	}

	rc = vreg_set_level(mag_vreg_main, 2600);
	if (rc) {
		pr_err("%s: mag_vreg_main set level failed (%d)\n",
		       __func__, rc);
		return rc;
	}

	rc = vreg_enable(mag_vreg_main);

	if (rc) {
		pr_err("%s: L3 vreg enable failed (%d)\n",
		       __func__, rc);
		return rc;
	}
		
	// VREG_SENSOR_1.8V
	mag_vreg_io = vreg_get(NULL, "ruim"); 

	if (IS_ERR(mag_vreg_io)) {
		rc = PTR_ERR(mag_vreg_io);
		pr_err("%s: ruim vreg get failed (%d)\n",
		       __func__, rc);
		return rc;
	}

	rc = vreg_set_level(mag_vreg_io, 1800);
	if (rc) {
		pr_err("%s: vreg L3 set level failed (%d)\n",
		       __func__, rc);
		return rc;
	}

	rc = vreg_enable(mag_vreg_io);

	if (rc) {
		pr_err("%s: L3 vreg enable failed (%d)\n",
		       __func__, rc);
		return rc;
	}

	return 0;
}

static struct i2c_gpio_platform_data fuelgauge_i2c_gpio_data = {
	.sda_pin    = 181,
	.scl_pin    = 82,
};

static struct platform_device fuelgauge_i2c_gpio_device = {  
	.name       = "i2c-gpio",
	.id     = 11,
	.dev        = {
		.platform_data  = &fuelgauge_i2c_gpio_data,
	},
};

#ifdef CONFIG_MAX17043_FUEL_GAUGE
static struct i2c_board_info fuelgauge_i2c_devices[] = {
    {
            I2C_BOARD_INFO("fuelgauge_max17043", 0x6D>>1),
            .irq = MSM_GPIO_TO_INT(180),
    },
};
#endif

static struct i2c_gpio_platform_data micro_usb_i2c_gpio_data = {
	.sda_pin    = micro_usb_i2c_gpio_sda,
	.scl_pin    = micro_usb_i2c_gpio_scl,
};

static struct platform_device micro_usb_i2c_gpio_device = {  
	.name       = "i2c-gpio",
	.id     = 10,
	.dev        = {
		.platform_data  = &micro_usb_i2c_gpio_data,
	},
};
#ifdef CONFIG_WIBRO_CMC
static struct i2c_gpio_platform_data max8893_i2c_gpio_data = {
	.sda_pin  = 114,	
	.scl_pin  = 115,	
};
static struct platform_device max8893_i2c_gpio_device = {  
	.name       = "i2c-gpio",
	.id     = 12,
	.dev        = {
		.platform_data  = &max8893_i2c_gpio_data,
	},
};
static struct i2c_board_info max8893_i2c_devices[] = {
  {
			I2C_BOARD_INFO("max8893_wmx",0x3E),
  }	
};
static struct i2c_gpio_platform_data wmxeeprom_i2c_gpio_data = {
	.sda_pin  = 83,	
	.scl_pin  = 84,	
};
static struct platform_device wmxeeprom_i2c_gpio_device = {  
	.name       = "i2c-gpio",
	.id     = 13,
	.dev        = {
		.platform_data  = &wmxeeprom_i2c_gpio_data,
	},
};
static struct i2c_board_info wmxeeprom_i2c_devices[] = {
  {
			I2C_BOARD_INFO("wmxeeprom",0x50),
  }	
};
#endif

#ifdef CONFIG_SENSORS_FSA9480
#if defined(CONFIG_MACH_VITAL2)
int fsa_vreg_en(int on)
{
       //To increase refcnt, this function is invoked.
	struct vreg* fsa_vreg_io = NULL;	
	
	pr_info("%s: %d\n", __func__, on);
	
	if(fsa_vreg_io == NULL) {
		fsa_vreg_io = vreg_get(NULL, "gp4");

		if(fsa_vreg_io == NULL) {
			pr_err("%s: fsa_vreg_io null!\n", __func__);
			return -1;
		}

              if(vreg_set_level(fsa_vreg_io, 2600) < 0) {
			pr_err("%s: fsa_vreg_io set level failed!\n", __func__);
			return -1;
		}
	}

	if(on) {
		vreg_enable(fsa_vreg_io);
	} else {
		//vreg_disable(fsa_vreg_io);
		//LDO_10 is  always on  power
		return 0;
	}

      //this function does'nt have ldo ramp up time 

      return 1;
}

static struct fsa_platform_data fsa_plat_data = {
     .vreg_en = fsa_vreg_en,
     .i2c_sda = micro_usb_i2c_gpio_sda,
     .i2c_scl = micro_usb_i2c_gpio_scl
};
#endif

static struct i2c_board_info micro_usb_i2c_devices[] = {
	{
		I2C_BOARD_INFO("fsa9480", 0x4A>>1),
		.irq = MSM_GPIO_TO_INT(147),
       #if defined(CONFIG_MACH_VITAL2)
		.platform_data = &fsa_plat_data,
	#endif	
	},
};
#endif

#ifdef CONFIG_USB_FUNCTION
static struct usb_mass_storage_platform_data usb_mass_storage_pdata = {
	.nluns          = 0x02,
	.buf_size       = 16384,
	.vendor         = "GOOGLE",
	.product        = "Mass storage",
	.release        = 0xffff,
};

static struct platform_device mass_storage_device = {
	.name           = "usb_mass_storage",
	.id             = -1,
	.dev            = {
		.platform_data          = &usb_mass_storage_pdata,
	},
};
#endif
#ifdef CONFIG_USB_ANDROID
static char *usb_functions_default[] = {
	"diag",
	"acm",
//	"nmea",
//	"rmnet",
	"usb_mass_storage",
};

static char *usb_functions_default_adb[] = {
	"diag",
	"acm",
//	"nmea",
//	"rmnet",
	"usb_mass_storage",
	"adb",
};

static char *fusion_usb_functions_default[] = {
	"diag",
	"nmea",
	"usb_mass_storage",
};

static char *fusion_usb_functions_default_adb[] = {
	"diag",
	"nmea",
	"usb_mass_storage",
	"adb",
};

static char *usb_functions_rndis[] = {
	"rndis",
};

static char *usb_functions_rndis_adb[] = {
	"rndis",
	"adb",
};

static char *usb_functions_rndis_diag[] = {
	"rndis",
	"diag",
};

static char *usb_functions_rndis_adb_diag[] = {
	"rndis",
	"diag",
	"adb",
};
static char *usb_functions_mtp_only[] = {
	"usb_mtp_gadget",
};

static char *usb_functions_all[] = {
#ifdef CONFIG_USB_ANDROID_ACM
	"acm",
#endif
#ifdef CONFIG_USB_ANDROID_DIAG
	"diag",
#endif
	"usb_mass_storage",
	"adb",	
#ifdef CONFIG_USB_ANDROID_RNDIS
	"rndis",
#endif
#ifdef CONFIG_USB_ANDROID_RMNET
	"rmnet",
#endif
#ifdef CONFIG_USB_F_SERIAL
	"modem",
#ifndef CONFIG_USB_SAMSUNG_DRIVER
	"nmea",
#endif
#endif
};

static struct android_usb_product usb_products[] = {
/*
	{
		.product_id	= 0x9026,
		.num_functions	= ARRAY_SIZE(usb_functions_default),
		.functions	= usb_functions_default,
	},
*/
	{
		.product_id	= 0x689E,
		.num_functions	= ARRAY_SIZE(usb_functions_default),
		.functions	= usb_functions_default,
	},
	{
		.product_id	= 0x689E,
		.num_functions	= ARRAY_SIZE(usb_functions_default_adb),
		.functions	= usb_functions_default_adb,
	},
	{
		.product_id	= 0x5A0F,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp_only),
		.functions	= usb_functions_mtp_only,
	},
	{
		.product_id	= 0x6881,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis),
		.functions	= usb_functions_rndis,
	},
/*	
	{
		.product_id	= 0x9024,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis_adb),
		.functions	= usb_functions_rndis_adb,
	},
*/
};

static struct usb_mass_storage_platform_data mass_storage_pdata = {
	.nluns		= 1,
	.vendor		= "Qualcomm Incorporated",
	.product        = "Mass storage",
	.release	= 0x0100,
};

static struct platform_device usb_mass_storage_device = {
	.name           = "usb_mass_storage",
	.id             = -1,
	.dev            = {
		.platform_data          = &mass_storage_pdata,
	},
};

static struct usb_ether_platform_data rndis_pdata = {
	/* ethaddr is filled by board_serialno_setup */
	.vendorID	= 0x04E8,
	.vendorDescr	= "Qualcomm Incorporated",
};

static struct platform_device rndis_device = {
	.name	= "rndis",
	.id	= -1,
	.dev	= {
		.platform_data = &rndis_pdata,
	},
};

static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id	= 0x04E8, // Samsung Vendor ID
	.product_id	= 0x689E,
	.version	= 0x0100,
	.product_name	= "Samsung Android USB Device",
	.manufacturer_name = "SAMSUNG Electronics Co., Ltd.",
	.num_products = ARRAY_SIZE(usb_products),
	.products = usb_products,

	.num_functions = ARRAY_SIZE(usb_functions_all),
	.functions = usb_functions_all,
	.serial_number = "1234567890ABCDEF",
};

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id		= -1,
	.dev		= {
		.platform_data = &android_usb_pdata,
	},
};

static int __init board_serialno_setup(char *serialno)
{
	int i;
	char *src = serialno;

	/* create a fake MAC address from our serial number.
	 * first byte is 0x02 to signify locally administered.
	 */
	rndis_pdata.ethaddr[0] = 0x02;
	for (i = 0; *src; i++) {
		/* XOR the USB serial across the remaining bytes */
		rndis_pdata.ethaddr[i % (ETH_ALEN - 1) + 1] ^= *src++;
	}

	android_usb_pdata.serial_number = serialno;
	return 1;
}
__setup("androidboot.serialno=", board_serialno_setup);
#endif

#ifdef CONFIG_TOUCHSCREEN_QT602240
static struct platform_device touchscreen_device_qt602240 = {
	.name = "qt602240-ts",
	.id = -1,
};

static struct i2c_board_info qt602240_touch_boardinfo[] = {
	{
		I2C_BOARD_INFO("qt602240_i2c", 0x4A ),
		.irq = MSM_GPIO_TO_INT(89),
	}
};
#endif


#ifdef CONFIG_TOUCHSCREEN_MELFAS


static struct platform_device touchscreen_device_melfas = {
	.name = "melfas-ts",
	.id = -1,
};

static struct i2c_board_info melfas_touch_boardinfo[] = {
	{
		I2C_BOARD_INFO("melfas_ts_i2c",0x20), 
		.irq = MSM_GPIO_TO_INT(89),
	}
};


static struct platform_device touchscreen_device_melfas_05 = {
	.name = "melfas-ts",
	.id = -1,
};

static struct i2c_board_info melfas_touch_boardinfo_05[] = {
	{
		I2C_BOARD_INFO("melfas_ts_i2c",0x48),
		.irq = MSM_GPIO_TO_INT(89),
	}
};

#endif

static struct i2c_gpio_platform_data touchscreen_i2c_gpio_data = {
	.sda_pin    = 87,
	.scl_pin    = 88,
};
static struct platform_device touchscreen_i2c_gpio_device = {  
	.name       = "i2c-gpio",
	.id     = 5,
	.dev        = {
		.platform_data  = &touchscreen_i2c_gpio_data,
	},
};

#ifdef CONFIG_FB_MSM_HDMI_SII9024_PANEL
static struct msm_hdmi_platform_data hdmi_sii9024_i2c_data = {
	.irq = MSM_GPIO_TO_INT(2),
};
#endif

static struct i2c_board_info msm_i2c_board_info[] = {
	// for CAM PM
	{
		I2C_BOARD_INFO("cam_pm_lp8720_i2c", 0x7D),
	},
#ifdef CONFIG_FB_MSM_HDMI_SII9024_PANEL	
	{
		I2C_BOARD_INFO("sii9024", 0x72 >> 1),
		.platform_data = &hdmi_sii9024_i2c_data,
	},
#endif
};

static struct i2c_board_info msm_marimba_board_info[] = {
	{
		I2C_BOARD_INFO("marimba", 0xc),
		.platform_data = &marimba_pdata,
	}
};

#ifdef CONFIG_USB_FUNCTION
static struct usb_function_map usb_functions_map[] = {
	{"diag", 0},
	{"adb", 1},
	{"modem", 2},
	{"nmea", 3},
	{"mass_storage", 4},
	{"ethernet", 5},
};

static struct usb_composition usb_func_composition[] = {
	{
		.product_id         = 0x9012,
		.functions	    = 0x5, /* 0101 */
	},

	{
		.product_id         = 0x9013,
		.functions	    = 0x15, /* 10101 */
	},

	{
		.product_id         = 0x9014,
		.functions	    = 0x30, /* 110000 */
	},

	{
		.product_id         = 0x9016,
		.functions	    = 0xD, /* 01101 */
	},

	{
		.product_id         = 0x9017,
		.functions	    = 0x1D, /* 11101 */
	},

	{
		.product_id         = 0xF000,
		.functions	    = 0x10, /* 10000 */
	},

	{
		.product_id         = 0xF009,
		.functions	    = 0x20, /* 100000 */
	},

	{
		.product_id         = 0x9018,
		.functions	    = 0x1F, /* 011111 */
	},

};
static struct msm_hsusb_platform_data msm_hsusb_pdata = {
	.version	= 0x0100,
	.phy_info	= USB_PHY_INTEGRATED | USB_PHY_MODEL_45NM,
	.vendor_id	= 0x5c6,
	.product_name	= "Qualcomm HSUSB Device",
	.serial_number	= "1234567890ABCDEF",
	.manufacturer_name
			= "Qualcomm Incorporated",
	.compositions	= usb_func_composition,
	.num_compositions
			= ARRAY_SIZE(usb_func_composition),
	.function_map	= usb_functions_map,
	.num_functions	= ARRAY_SIZE(usb_functions_map),
	.core_clk	= 1,
};
#endif


static struct msm_handset_platform_data hs_platform_data = {
	.hs_name = "pwr_key",
	.pwr_key_delay_ms = 500, /* 0 will disable end key */
};

static struct platform_device hs_device = {
	.name   = "msm-handset",
	.id     = -1,
	.dev    = {
		.platform_data = &hs_platform_data,
	},
};

static struct msm_pm_platform_data msm_pm_data[MSM_PM_SLEEP_MODE_NR] = {
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].supported = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].latency = 8594,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].residency = 23740,

	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].supported = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].latency = 4594,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].residency = 23740,

	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE].supported = 1,
#ifdef CONFIG_MSM_STANDALONE_POWER_COLLAPSE
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE].suspend_enabled = 0,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE].idle_enabled = 1,
#else /*CONFIG_MSM_STANDALONE_POWER_COLLAPSE*/
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE].suspend_enabled = 0,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE].idle_enabled = 0,
#endif /*CONFIG_MSM_STANDALONE_POWER_COLLAPSE*/
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE].latency = 500,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE].residency = 6000,

	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].supported = 1,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].suspend_enabled
		= 1,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].idle_enabled = 0,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency = 443,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].residency = 1098,

	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].supported = 1,
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].latency = 2,
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].residency = 0,
};

#ifdef CONFIG_SPI_QSD
static struct resource qsd_spi_resources[] = {
	{
		.name   = "spi_irq_in",
		.start	= INT_SPI_INPUT,
		.end	= INT_SPI_INPUT,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name   = "spi_irq_out",
		.start	= INT_SPI_OUTPUT,
		.end	= INT_SPI_OUTPUT,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name   = "spi_irq_err",
		.start	= INT_SPI_ERROR,
		.end	= INT_SPI_ERROR,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name   = "spi_base",
		.start	= 0xA8000000,
		.end	= 0xA8000000 + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name   = "spidm_channels",
		.flags  = IORESOURCE_DMA,
	},
	{
		.name   = "spidm_crci",
		.flags  = IORESOURCE_DMA,
	},
};

#define AMDH0_BASE_PHYS		0xAC200000
#define ADMH0_GP_CTL		(ct_adm_base + 0x3D8)
static int msm_qsd_spi_dma_config(void)
{
	void __iomem *ct_adm_base = 0;
	u32 spi_mux = 0;
	int ret = 0;

	ct_adm_base = ioremap(AMDH0_BASE_PHYS, PAGE_SIZE);
	if (!ct_adm_base) {
		pr_err("%s: Could not remap %x\n", __func__, AMDH0_BASE_PHYS);
		return -ENOMEM;
	}

	spi_mux = (ioread32(ADMH0_GP_CTL) & (0x3 << 12)) >> 12;

	qsd_spi_resources[4].start  = DMOV_USB_CHAN;
	qsd_spi_resources[4].end    = DMOV_TSIF_CHAN;

	switch (spi_mux) {
	case (1):
		qsd_spi_resources[5].start  = DMOV_HSUART1_RX_CRCI;
		qsd_spi_resources[5].end    = DMOV_HSUART1_TX_CRCI;
		break;
	case (2):
		qsd_spi_resources[5].start  = DMOV_HSUART2_RX_CRCI;
		qsd_spi_resources[5].end    = DMOV_HSUART2_TX_CRCI;
		break;
	case (3):
		qsd_spi_resources[5].start  = DMOV_CE_OUT_CRCI;
		qsd_spi_resources[5].end    = DMOV_CE_IN_CRCI;
		break;
	default:
		ret = -ENOENT;
	}

	iounmap(ct_adm_base);

	return ret;
}

static struct platform_device qsd_device_spi = {
	.name		= "spi_qsd",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(qsd_spi_resources),
	.resource	= qsd_spi_resources,
};

static struct msm_gpio qsd_spi_gpio_config_data[] = {
	{ GPIO_CFG(45, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_clk" },
	{ GPIO_CFG(46, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_cs0" },
	{ GPIO_CFG(47, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_8MA), "spi_mosi" },
};

static int msm_qsd_spi_gpio_config(void)
{
	return msm_gpios_request_enable(qsd_spi_gpio_config_data,
		ARRAY_SIZE(qsd_spi_gpio_config_data));
}

static void msm_qsd_spi_gpio_release(void)
{
	msm_gpios_disable_free(qsd_spi_gpio_config_data,
		ARRAY_SIZE(qsd_spi_gpio_config_data));
}

static struct msm_spi_platform_data qsd_spi_pdata = {
	.max_clock_speed = 26331429,
	.clk_name = "spi_clk",
	.pclk_name = "spi_pclk",
	.gpio_config  = msm_qsd_spi_gpio_config,
	.gpio_release = msm_qsd_spi_gpio_release,
	.dma_config = msm_qsd_spi_dma_config,
};

static void __init msm_qsd_spi_init(void)
{
	qsd_device_spi.dev.platform_data = &qsd_spi_pdata;
}
#endif

#ifdef CONFIG_USB_EHCI_MSM
static void msm_hsusb_vbus_power(unsigned phy_info, int on)
{
#if 0 //(CONFIG_BOARD_REVISION <= 0x02)
	int rc;
	static int vbus_is_on;
	struct pm8058_gpio usb_vbus = {
		.direction      = PM_GPIO_DIR_OUT,
		.pull           = PM_GPIO_PULL_NO,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 1,
		.vin_sel        = 2,
		.out_strength   = PM_GPIO_STRENGTH_MED,
		.function       = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol    = 0,
	};

	/* If VBUS is already on (or off), do nothing. */
	if (unlikely(on == vbus_is_on))
		return;

	if (on) {
		rc = pm8058_gpio_config(36, &usb_vbus);
		if (rc) {
			pr_err("%s PMIC GPIO 36 write failed\n", __func__);
			return;
		}
	} else
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(36), 0);

	vbus_is_on = on;
#endif
}

static struct msm_usb_host_platform_data msm_usb_host_pdata = {
	.phy_info   = (USB_PHY_INTEGRATED | USB_PHY_MODEL_45NM),
	.power_budget   = 180,
};
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
static int hsusb_rpc_connect(int connect)
{
	if (connect)
		return msm_hsusb_rpc_connect();
	else
		return msm_hsusb_rpc_close();
}
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
static struct vreg *vreg_3p3;
static int msm_hsusb_ldo_init(int init)
{
	uint32_t version = 0;
	int def_vol = 3400;

	version = socinfo_get_version();

	if (SOCINFO_VERSION_MAJOR(version) >= 2 &&
			SOCINFO_VERSION_MINOR(version) >= 1) {
		def_vol = 3075;
		pr_debug("%s: default voltage:%d\n", __func__, def_vol);
	}

	pr_info("%s: vreg_3p3 voltage:%d\n",__func__, def_vol);
	
	if (init) {
		vreg_3p3 = vreg_get(NULL, "usb");
		if (IS_ERR(vreg_3p3))
			return PTR_ERR(vreg_3p3);
		vreg_set_level(vreg_3p3, def_vol);
	} else
		vreg_put(vreg_3p3);

	return 0;
}

static int msm_hsusb_ldo_enable(int enable)
{
	static int ldo_status;

	pr_info("%s: %d\n",__func__, enable);
	
	if (!vreg_3p3 || IS_ERR(vreg_3p3))
		return -ENODEV;

	if (ldo_status == enable)
		return 0;

	ldo_status = enable;

	if (enable)
		return vreg_enable(vreg_3p3);

	return vreg_disable(vreg_3p3);
}

static int msm_hsusb_ldo_set_voltage(int mV)
{
	static int cur_voltage = 3400;

	if (!vreg_3p3 || IS_ERR(vreg_3p3))
		return -ENODEV;

	if (cur_voltage == mV)
		return 0;

	cur_voltage = mV;

	pr_debug("%s: (%d)\n", __func__, mV);

	return vreg_set_level(vreg_3p3, mV);
}
#endif

#ifndef CONFIG_USB_EHCI_MSM
static int msm_hsusb_pmic_notif_init(void (*callback)(int online), int init);
#endif
static struct msm_otg_platform_data msm_otg_pdata = {
	.rpc_connect	= hsusb_rpc_connect,

#ifndef CONFIG_USB_EHCI_MSM
	.pmic_notif_init         = msm_hsusb_pmic_notif_init,
#else
	.vbus_power = msm_hsusb_vbus_power,
#endif
	.core_clk	= 1,
	.pemp_level     = PRE_EMPHASIS_WITH_20_PERCENT,
	.cdr_autoreset  = CDR_AUTO_RESET_DISABLE,
	.drv_ampl       = HS_DRV_AMPLITUDE_DEFAULT,
	.se1_gating		 = SE1_GATING_DISABLE,
	.chg_vbus_draw		 = hsusb_chg_vbus_draw,
	.chg_connected		 = hsusb_chg_connected,
	.chg_init		 = hsusb_chg_init,
	.ldo_enable		 = msm_hsusb_ldo_enable,
	.ldo_init		 = msm_hsusb_ldo_init,
	.ldo_set_voltage	 = msm_hsusb_ldo_set_voltage,
};

#ifdef CONFIG_USB_GADGET
static struct msm_hsusb_gadget_platform_data msm_gadget_pdata;
#endif
#ifndef CONFIG_USB_EHCI_MSM
typedef void (*notify_vbus_state) (int);
notify_vbus_state notify_vbus_state_func_ptr;
int vbus_on_irq;
static irqreturn_t pmic_vbus_on_irq(int irq, void *data)
{
	pr_info("%s: vbus notification from pmic\n", __func__);

	(*notify_vbus_state_func_ptr) (1);

	return IRQ_HANDLED;
}
static int msm_hsusb_pmic_notif_init(void (*callback)(int online), int init)
{
	int ret;

	if (init) {
		if (!callback)
			return -ENODEV;

		notify_vbus_state_func_ptr = callback;
		vbus_on_irq = platform_get_irq_byname(&msm_device_otg,
			"vbus_on");
		if (vbus_on_irq <= 0) {
			pr_err("%s: unable to get vbus on irq\n", __func__);
			return -ENODEV;
		}

		ret = request_irq(vbus_on_irq, pmic_vbus_on_irq,
			IRQF_TRIGGER_RISING, "msm_otg_vbus_on", NULL);
		if (ret) {
			pr_info("%s: request_irq for vbus_on"
				"interrupt failed\n", __func__);
			return ret;
		}
		msm_otg_pdata.pmic_vbus_irq = vbus_on_irq;
		return 0;
	} else {
		free_irq(vbus_on_irq, 0);
		notify_vbus_state_func_ptr = NULL;
		return 0;
	}
}
#endif

static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.allocator_type = PMEM_ALLOCATORTYPE_ALLORNOTHING,
	.cached = 1,
};

static struct platform_device android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = { .platform_data = &android_pmem_pdata },
};

#ifndef CONFIG_SPI_QSD
static int lcdc_gpio_array_num[] = {
				45, /* LCD_SPI_SCLK */
				46, /* LCD_SPI_CS_N  */
				47, /* LCD_SPI_DATA_IN */
				145 /* MLCD_RST */ //vital2.rev01 : 72 -> 145
				};

static struct msm_gpio lcdc_gpio_config_data[] = {
	{ GPIO_CFG(45, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_clk" },
	{ GPIO_CFG(46, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_cs0" },
	{ GPIO_CFG(47, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_mosi" },
	{ GPIO_CFG(145, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcd_reset" }, //vital2.rev01 : 72 -> 145
};

/* vital2.sleep */
static struct msm_gpio lcdc_gpio_sleep_config_data[] = {
	{ GPIO_CFG(45, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_clk" },
	{ GPIO_CFG(46, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_cs0" },
	{ GPIO_CFG(47, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_mosi" },
	{ GPIO_CFG(145, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcd_reset" },
	{ GPIO_CFG(48, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_miso" },
};

static void lcdc_config_gpios(int enable)
{
	struct msm_gpio *lcdc_gpio_cfg_data;
	struct msm_gpio *lcdc_gpio_sleep_cfg_data;
	int array_size;
	int sleep_cfg_arry_size;

	lcdc_gpio_cfg_data = lcdc_gpio_config_data;
	lcdc_gpio_sleep_cfg_data = lcdc_gpio_sleep_config_data;
	array_size = ARRAY_SIZE(lcdc_gpio_config_data);
	sleep_cfg_arry_size = ARRAY_SIZE(lcdc_gpio_sleep_config_data);

	if (enable) {
		msm_gpios_request_enable(lcdc_gpio_cfg_data,
					      array_size);
	}
	else {
		/* vital2.sleep let's control it ourself. if we are to use msm_gpios_disable */
		/* we need to set sleep configuration values in SLEEP_CFG of TLMMBsp.c in modem */
		if(lcdc_gpio_sleep_cfg_data) {
			msm_gpios_enable(lcdc_gpio_sleep_cfg_data,
						      array_size);
			msm_gpios_free(lcdc_gpio_sleep_cfg_data,
						      array_size);
		}
		else {
			msm_gpios_disable_free(lcdc_gpio_config_data,
							    array_size);
		}
	}
}
#endif

static struct msm_panel_common_pdata lcdc_panel_data = {
#ifndef CONFIG_SPI_QSD
	.panel_config_gpio = lcdc_config_gpios,
	.gpio_num          = lcdc_gpio_array_num,
#endif
};

#ifdef CONFIG_FB_MSM_LCDC_S6D16A0_WVGA_PANEL
static struct platform_device lcdc_s6d16a0_panel_device = {
	.name   = "lcdc_s6d16a0_wvga",
	.id     = 0,
	.dev    = {
		.platform_data = &lcdc_panel_data,
	}
};
#endif

#ifdef CONFIG_FB_MSM_LCDC_LD9040_WVGA_PANEL
static struct platform_device lcdc_ld9040_panel_device = {
	.name   = "lcdc_ld9040_wvga",
	.id     = 0,
	.dev    = {
		.platform_data = &lcdc_panel_data,
	}
};
#endif
#ifdef CONFIG_FB_MSM_LCDC_S6D05A1_HVGA_PANEL
static struct platform_device lcdc_s6d05a1_panel_device = {
	.name   = "lcdc_s6d05a1_hvga",
	.id     = 0,
	.dev    = {
		.platform_data = &lcdc_panel_data,
	}
};
#endif
#if 0 //(CONFIG_BOARD_REVISION >= 0x01)
static struct msm_gpio dtv_panel_gpios[] = {
	{ GPIO_CFG(120, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "wca_mclk" },
	{ GPIO_CFG(121, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "wca_sd0" },
	{ GPIO_CFG(122, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "wca_sd1" },
	{ GPIO_CFG(123, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "wca_sd2" },
	{ GPIO_CFG(124, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_8MA), "dtv_pclk" },
	{ GPIO_CFG(125, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_en" },
	{ GPIO_CFG(126, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_vsync" },
	{ GPIO_CFG(127, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_hsync" },
	{ GPIO_CFG(128, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_data0" },
	{ GPIO_CFG(129, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_data1" },
	{ GPIO_CFG(130, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_data2" },
	{ GPIO_CFG(131, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_data3" },
	{ GPIO_CFG(132, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_data4" },
	{ GPIO_CFG(160, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_data5" },
	{ GPIO_CFG(161, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_data6" },
	{ GPIO_CFG(162, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_data7" },
	{ GPIO_CFG(163, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_data8" },
	{ GPIO_CFG(164, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_data9" },
	{ GPIO_CFG(165, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat10" },
	{ GPIO_CFG(166, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat11" },
	{ GPIO_CFG(167, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat12" },
	{ GPIO_CFG(168, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat13" },
	{ GPIO_CFG(169, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat14" },
	{ GPIO_CFG(170, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat15" },
	{ GPIO_CFG(171, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat16" },
	{ GPIO_CFG(172, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat17" },
	{ GPIO_CFG(173, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat18" },
	{ GPIO_CFG(174, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat19" },
	{ GPIO_CFG(175, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat20" },
	{ GPIO_CFG(176, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat21" },
	{ GPIO_CFG(177, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat22" },
	{ GPIO_CFG(178, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat23" },
};

#ifdef HDMI_RESET
static unsigned dtv_reset_gpio =
	GPIO_CFG(37, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA);
#endif

static int dtv_panel_power(int on)
{
	int flag_on = !!on;
	static int dtv_power_save_on;
	struct vreg *vreg_ldo17, *vreg_ldo8;
	int rc;

	if (dtv_power_save_on == flag_on)
		return 0;

	dtv_power_save_on = flag_on;
	pr_info("%s: %d >>\n", __func__, on);

#ifdef HDMI_RESET
	if (on) {
		rc = gpio_tlmm_config(dtv_reset_gpio, GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, dtv_reset_gpio, rc);
			return rc;
		}

		/* bring reset line low to hold reset*/
		gpio_set_value_cansleep(37, 0);
	}
#endif

	if (on) {
		rc = msm_gpios_enable(dtv_panel_gpios,
				ARRAY_SIZE(dtv_panel_gpios));
		if (rc < 0) {
			printk(KERN_ERR "%s: gpio enable failed: %d\n",
				__func__, rc);
			return rc;
		}
	} else {
		rc = msm_gpios_disable(dtv_panel_gpios,
				ARRAY_SIZE(dtv_panel_gpios));
		if (rc < 0) {
			printk(KERN_ERR "%s: gpio disable failed: %d\n",
				__func__, rc);
			return rc;
		}
	}

#ifdef HDMI_RESET
	if (on) {
		gpio_set_value_cansleep(37, 1);	/* bring reset line high */
		mdelay(10);		/* 10 msec before IO can be accessed */
	}
#endif
	pr_info("%s: %d <<\n", __func__, on);

	return rc;
}

static struct lcdc_platform_data dtv_pdata = {
	.lcdc_power_save   = dtv_panel_power,
};
#endif

#ifdef CONFIG_FB_MSM_HDMI_SII9024_PANEL	
static struct platform_device hdmi_sii9024_panel_device = {
	.name   = "sii9024",
	.id     = 0,
};
#endif

static struct msm_serial_hs_platform_data msm_uart_dm1_pdata = {
       .inject_rx_on_wakeup = 1,
       .rx_to_inject = 0xFD,
};

#ifdef CONFIG_SERIAL_MSM_RX_WAKEUP
static struct msm_serial_platform_data msm_uart3_pdata = {
	.wakeup_irq = MSM_GPIO_TO_INT(35),
       .inject_rx_on_wakeup = 1,
       .rx_to_inject = 0x32,
};
#endif

static struct resource msm_fb_resources[] = {
	{
		.flags  = IORESOURCE_DMA,
	}
};

static int msm_fb_detect_panel(const char *name)
{
#if defined (CONFIG_FB_MSM_LCDC_S6D16A0_WVGA_PANEL)
	if (!strcmp(name, "lcdc_s6d16a0_wvga"))
		return 0;
#elif defined (CONFIG_FB_MSM_LCDC_LD9040_WVGA_PANEL)
	if (!strcmp(name, "lcdc_ld9040_wvga"))
		return 0;
#elif defined (CONFIG_FB_MSM_LCDC_S6D05A1_HVGA_PANEL)
	if (!strcmp(name, "lcdc_s6d05a1_hvga"))
		return 0;	
#endif
	else
		return -ENODEV;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = msm_fb_detect_panel,
	.mddi_prescan = 1,
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources  = ARRAY_SIZE(msm_fb_resources),
	.resource       = msm_fb_resources,
	.dev    = {
		.platform_data = &msm_fb_pdata,
	}
};

static struct platform_device msm_migrate_pages_device = {
	.name   = "msm_migrate_pages",
	.id     = -1,
};

static struct android_pmem_platform_data android_pmem_kernel_ebi1_pdata = {
       .name = PMEM_KERNEL_EBI1_DATA_NAME,
	/* if no allocator_type, defaults to PMEM_ALLOCATORTYPE_BITMAP,
	* the only valid choice at this time. The board structure is
	* set to all zeros by the C runtime initialization and that is now
	* the enum value of PMEM_ALLOCATORTYPE_BITMAP, now forced to 0 in
	* include/linux/android_pmem.h.
	*/
       .cached = 0,
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
       .name = "pmem_adsp",
       .allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
       .cached = 0,
};

static struct android_pmem_platform_data android_pmem_audio_pdata = {
       .name = "pmem_audio",
       .allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
       .cached = 0,
};

#ifdef PMEM_HDMI
static struct android_pmem_platform_data android_pmem_hdmi_pdata = {
       .name = "pmem_hdmi",
       .allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
       .cached = 0,
};
#endif

static struct platform_device android_pmem_kernel_ebi1_device = {
       .name = "android_pmem",
       .id = 1,
       .dev = { .platform_data = &android_pmem_kernel_ebi1_pdata },
};

static struct platform_device android_pmem_adsp_device = {
       .name = "android_pmem",
       .id = 2,
       .dev = { .platform_data = &android_pmem_adsp_pdata },
};

static struct platform_device android_pmem_audio_device = {
       .name = "android_pmem",
       .id = 4,
       .dev = { .platform_data = &android_pmem_audio_pdata },
};

#ifdef PMEM_HDMI
static struct platform_device android_pmem_hdmi_device = {
       .name = "android_pmem",
       .id = 5,
       .dev = { .platform_data = &android_pmem_hdmi_pdata },
};
#endif

static struct kgsl_platform_data kgsl_pdata = 
{
#ifdef CONFIG_MSM_NPA_SYSTEM_BUS
	/* NPA Flow IDs */
	.high_axi_3d = MSM_AXI_FLOW_3D_GPU_HIGH,
	.high_axi_2d = MSM_AXI_FLOW_2D_GPU_HIGH,
#else
	/* AXI rates in KHz */
	.high_axi_3d = 192000,
	.high_axi_2d = 192000,
#endif
	.max_grp2d_freq = 0,
	.min_grp2d_freq = 0,
	.set_grp2d_async = NULL, /* HW workaround, run Z180 SYNC @ 192 MHZ */
	.max_grp3d_freq = 245760000,
	.min_grp3d_freq = 192 * 1000*1000,
	.set_grp3d_async = set_grp3d_async,
	.imem_clk_name = "imem_clk",
	.grp3d_clk_name = "grp_clk",
	.grp3d_pclk_name = "grp_pclk",
#ifdef CONFIG_MSM_KGSL_2D
	.grp2d0_clk_name = "grp_2d_clk",
	.grp2d0_pclk_name = "grp_2d_pclk",
#else
	.grp2d0_clk_name = NULL,
#endif
	.idle_timeout_3d = HZ/20,
	.idle_timeout_2d = HZ/10,

#ifdef CONFIG_KGSL_PER_PROCESS_PAGE_TABLE
	.pt_va_size = SZ_32M,
	/* Maximum of 32 concurrent processes */
	.pt_max_count = 32,
#else
	.pt_va_size = SZ_128M,
	/* We only ever have one pagetable for everybody */
	.pt_max_count = 1,
#endif
};

static struct resource kgsl_resources[] = {
	{
		.name = "kgsl_reg_memory",
		.start = 0xA3500000, /* 3D GRP address */
		.end = 0xA351ffff,
		.flags = IORESOURCE_MEM,
	},
	{
		.name = "kgsl_yamato_irq",
		.start = INT_GRP_3D,
		.end = INT_GRP_3D,
		.flags = IORESOURCE_IRQ,
	},
	{
		.name = "kgsl_2d0_reg_memory",
		.start = 0xA3900000, /* Z180 base address */
		.end = 0xA3900FFF,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = "kgsl_2d0_irq",
		.start = INT_GRP_2D,
		.end = INT_GRP_2D,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device msm_device_kgsl = {
	.name = "kgsl",
	.id = -1,
	.num_resources = ARRAY_SIZE(kgsl_resources),
	.resource = kgsl_resources,
	.dev = {
		.platform_data = &kgsl_pdata,
	},
};

extern void lcdc_panel_reset(int on);

#ifdef LCD_WAKEUP_PERFORMANCE
static volatile unsigned int panel_cpu = UINT_MAX;
unsigned long nanosec_1st, nanosec_2nd;


void check_system_time(int step) {
	unsigned long long sec_sys;
	unsigned long nanosec_sys, result_sys;

	/* Follow the token with the time */
	sec_sys = cpu_clock(panel_cpu);
	nanosec_sys = do_div(sec_sys, 1000000000);
	result_sys = ((unsigned long) sec_sys) * 1000 + (nanosec_sys / 1000000); /* means XX.XXX sec */
	//printk(KERN_ERR "@LCDINIT@ step %d. system time : %lu\n", step, result_sys);

	if(step == 0x1) {
		nanosec_1st = result_sys;
	}
	else {
		nanosec_2nd = result_sys;
	}
}

#define SMD_DELAY_REQ	300 /* request the delay(ms) */

unsigned check_delay_time(void) {
	unsigned ret_delta;

	if(nanosec_1st && nanosec_2nd) {
		if(nanosec_2nd > nanosec_1st) {
			ret_delta = (unsigned)(nanosec_2nd - nanosec_1st);
			pr_info("@LCDINIT@:delta of system time : %d msec.\n", ret_delta);	
		}
		else {
			ret_delta = SMD_DELAY_REQ;
		}

		if(ret_delta >= SMD_DELAY_REQ)
			ret_delta = 0;
		else
			ret_delta = SMD_DELAY_REQ - ret_delta;

	}
	else {
		ret_delta = SMD_DELAY_REQ;
	}

	if(ret_delta > SMD_DELAY_REQ) {
		ret_delta = SMD_DELAY_REQ;
	}

	nanosec_1st = nanosec_2nd = 0;
	return ret_delta;

}
#else

static
#endif
int display_common_power(int on)
{
	int rc = 0, delay = 0, flag_on = !!on;
	static int display_common_power_save_on;
	struct vreg *vreg_ldo15, *vreg_ldo12 = NULL;
	unsigned gp6_cnt, gp9_cnt;

#ifdef LCD_WAKEUP_PERFORMANCE
	gp6_cnt = vreg_get_refcnt(NULL, "gp6");
	gp9_cnt =  vreg_get_refcnt(NULL, "gp9");
	pr_info("@LCDINIT@:CHECK VDD VCC - on: 0x%x, gp6_cnt: %d, gp9_cnt: %d\n", on, gp6_cnt, gp9_cnt);

	if(on && gp6_cnt && gp9_cnt) { //both ldo is ON
		if(on != 0xFEE) {
			goto panel_reset;
		}
		else {
			return 0;
		}
	}

	if(!on && !gp6_cnt && !gp9_cnt) { //both ldo is OFF
		return 0;
	}
#else
	if (display_common_power_save_on == flag_on)
		return 0;

	display_common_power_save_on = flag_on;	
#endif

	pr_info("@LCDINIT@:POWER VDD VCC - on: 0x%x\n", on);
	vreg_ldo15 = vreg_get(NULL, "gp6");
	if (IS_ERR(vreg_ldo15)) {
		rc = PTR_ERR(vreg_ldo15);
		pr_err("%s: gp6 vreg get failed (%d)\n", __func__, rc);
		return rc;
	}

	vreg_ldo12 = vreg_get(NULL, "gp9");
	if (IS_ERR(vreg_ldo12)) {
		rc = PTR_ERR(vreg_ldo12);
		pr_err("%s: gp9 vreg get failed (%d)\n", __func__, rc);
		return rc;
	}

	/* LCD sequence : 2.8V > 1.8V > RST */
	/* restored to L15 with 2.8V & L12 with 1.8V on h/w rev.06 by curos */
	if (system_rev <= 5) {
		rc = vreg_set_level(vreg_ldo12, 2800);
		if (rc) {
			pr_err("%s: vreg LDO12 set level failed (%d)\n", __func__, rc);
			return rc;
		}
		rc = vreg_set_level(vreg_ldo15, 1800);
		if (rc) {
			pr_err("%s: vreg LDO15 set level failed (%d)\n", __func__, rc);
			return rc;
		}

		if (on) {
			rc = vreg_enable(vreg_ldo12);
			usleep(1000); /* vital2.lcd.timing */
		}
		else {
			if (system_rev >= 3) //requested by curos h/w team
				rc = vreg_disable(vreg_ldo12);
		}
		if (rc) {
			pr_err("%s: LDO12 vreg %s failed (%d)\n", __func__, (on)?"enable":"disable", rc);
			return rc;
		}

		if (on)
			rc = vreg_enable(vreg_ldo15);
		else
			rc = vreg_disable(vreg_ldo15);

		if (rc) {
			pr_err("%s: LDO15 vreg %s failed (%d)\n", __func__, (on)?"enable":"disable", rc);
			return rc;
		}	
	}
	else 
	{
		rc = vreg_set_level(vreg_ldo15, 2800);
		if (rc) {
			pr_err("%s: vreg LDO15 set level failed (%d)\n", __func__, rc);
			return rc;
		}

		rc = vreg_set_level(vreg_ldo12, 1800);
		if (rc) {
			pr_err("%s: vreg LDO12 set level failed (%d)\n", __func__, rc);
			return rc;
		}

		if (on) {
			rc = vreg_enable(vreg_ldo15);
			usleep(1000); /* vital2.lcd.timing */
		}
		else {
			rc = vreg_disable(vreg_ldo15);
		}
		if (rc) {
			pr_err("%s: LDO15 vreg %s failed (%d)\n", __func__, (on)?"enable":"disable", rc);
			return rc;
		}

		if (on)
			rc = vreg_enable(vreg_ldo12);
		else
			rc = vreg_disable(vreg_ldo12);

		if (rc) {
			pr_err("%s: LDO12 vreg %s failed (%d)\n", __func__, (on)?"enable":"disable", rc);
			return rc;
		}
	}

	/* delay */
	if(on) {
#ifdef LCD_WAKEUP_PERFORMANCE
		if(on == 0xFEE) {
			check_system_time(0x1);
			return rc; /* set ldo only */
		}
#else
		usleep(1000); /* ensure power is stable */
#endif
	}

#ifdef LCD_WAKEUP_PERFORMANCE
	goto panel_reset;
#else
	lcdc_panel_reset(on); /* vital2.lcd.timing */
	return rc;
#endif


#ifdef LCD_WAKEUP_PERFORMANCE
panel_reset:
	if(on && on != 0xFEE) {
		check_system_time(0x2);	
		delay = check_delay_time();
		pr_info("@LCDINIT@:DELAY with %d msec.\n", delay);
		if(!!delay)
			msleep(delay);
	}
	lcdc_panel_reset(on);

	return rc;
#endif
}

static struct msm_panel_common_pdata mdp_pdata = {
	.hw_revision_addr = 0xac001270,
//	.mdp_core_clk_rate = 122880000,
	.mdp_core_clk_rate = 192000000,
};

static struct msm_gpio lcd_panel_gpios[] = {
	{ GPIO_CFG(18, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_grn0" },
	{ GPIO_CFG(19, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_grn1" },
	{ GPIO_CFG(20, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_blu0" },
	{ GPIO_CFG(21, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_blu1" },
	{ GPIO_CFG(22, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_blu2" },
	{ GPIO_CFG(23, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_red0" },
	{ GPIO_CFG(24, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_red1" },
	{ GPIO_CFG(25, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_red2" },
	{ GPIO_CFG(90, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_pclk" },
	{ GPIO_CFG(91, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_en" },
	{ GPIO_CFG(92, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_vsync" },
	{ GPIO_CFG(93, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_hsync" },
	{ GPIO_CFG(94, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_grn2" },
	{ GPIO_CFG(95, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_grn3" },
	{ GPIO_CFG(96, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_grn4" },
	{ GPIO_CFG(97, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_grn5" },
	{ GPIO_CFG(98, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_grn6" },
	{ GPIO_CFG(99, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_grn7" },
	{ GPIO_CFG(100, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_blu3" },
	{ GPIO_CFG(101, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_blu4" },
	{ GPIO_CFG(102, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_blu5" },
	{ GPIO_CFG(103, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_blu6" },
	{ GPIO_CFG(104, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_blu7" },
	{ GPIO_CFG(105, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_red3" },
	{ GPIO_CFG(106, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_red4" },
	{ GPIO_CFG(107, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_red5" },
	{ GPIO_CFG(108, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_red6" },
	{ GPIO_CFG(109, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "lcdc_red7" },
};

/* vital2.sleep */
static struct msm_gpio lcd_panel_gpios_sleep_cfg[] = {
	{ GPIO_CFG(18, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn0" },
	{ GPIO_CFG(19, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn1" },
	{ GPIO_CFG(20, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu0" },
	{ GPIO_CFG(21, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu1" },
	{ GPIO_CFG(22, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu2" },
	{ GPIO_CFG(23, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red0" },
	{ GPIO_CFG(24, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red1" },
	{ GPIO_CFG(25, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red2" },
	{ GPIO_CFG(90, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_pclk" },
	{ GPIO_CFG(91, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_en" },
	{ GPIO_CFG(92, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_vsync" },
	{ GPIO_CFG(93, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_hsync" },
	{ GPIO_CFG(94, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn2" },
	{ GPIO_CFG(95, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn3" },
	{ GPIO_CFG(96, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn4" },
	{ GPIO_CFG(97, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn5" },
	{ GPIO_CFG(98, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn6" },
	{ GPIO_CFG(99, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn7" },
	{ GPIO_CFG(100, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu3" },
	{ GPIO_CFG(101, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu4" },
	{ GPIO_CFG(102, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu5" },
	{ GPIO_CFG(103, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu6" },
	{ GPIO_CFG(104, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu7" },
	{ GPIO_CFG(105, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red3" },
	{ GPIO_CFG(106, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red4" },
	{ GPIO_CFG(107, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red5" },
	{ GPIO_CFG(108, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red6" },
	{ GPIO_CFG(109, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red7" },
};

static int lcdc_common_panel_power(int on)
{
	int rc = 0;

	display_common_power(on);

	if (on) {
		rc = msm_gpios_enable(lcd_panel_gpios,
				ARRAY_SIZE(lcd_panel_gpios));
		if (rc < 0) {
			printk(KERN_ERR "%s: gpio config failed: %d\n",
				__func__, rc);
		}
	} 
	else {
#if 1/* vital2.sleep let's control it ourself. if we are to use msm_gpios_disable */
	/* we need to set sleep configuration values in SLEEP_CFG of TLMMBsp.c in modem */
		msm_gpios_enable(lcd_panel_gpios_sleep_cfg,
				ARRAY_SIZE(lcd_panel_gpios_sleep_cfg));
#else
		msm_gpios_disable(lcd_panel_gpios,
				ARRAY_SIZE(lcd_panel_gpios));
#endif
	}
	return rc;
}

static int lcdc_panel_power(int on)
{
	int flag_on = !!on;
	static int lcdc_power_save_on;

	if (lcdc_power_save_on == flag_on)
		return 0;

	lcdc_power_save_on = flag_on;

	return lcdc_common_panel_power(on);
}

static struct lcdc_platform_data lcdc_pdata = {
	.lcdc_power_save   = lcdc_panel_power,
};

static void __init msm_fb_add_devices(void)
{
	msm_fb_register_device("mdp", &mdp_pdata);
	msm_fb_register_device("lcdc", &lcdc_pdata);
#if 0// (CONFIG_BOARD_REVISION >= 0x01)
	msm_fb_register_device("dtv", &dtv_pdata);
#endif
}

#if 0//shiks_test
#if defined(CONFIG_MARIMBA_CORE) && \
    (defined(CONFIG_MSM_BT_POWER) || defined(CONFIG_MSM_BT_POWER_MODULE))
    static struct platform_device msm_bt_power_device = {
    .name = "bt_power",
    .id     = -1
    };

    enum {
    BT_RFR,
    BT_CTS,
    BT_RX,
    BT_TX,
    };

    static struct msm_gpio bt_config_power_on[] = {
    { GPIO_CFG(134, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
    "UART1DM_RFR" },
    { GPIO_CFG(135, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
    "UART1DM_CTS" },
    { GPIO_CFG(136, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
    "UART1DM_Rx" },
    { GPIO_CFG(137, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
    "UART1DM_Tx" }
    };

    static struct msm_gpio bt_config_power_off[] = {
    { GPIO_CFG(134, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
    "UART1DM_RFR" },
    { GPIO_CFG(135, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
    "UART1DM_CTS" },
    { GPIO_CFG(136, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
    "UART1DM_Rx" },
    { GPIO_CFG(137, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
    "UART1DM_Tx" }
    };

    static struct msm_gpio bt_config_clock[] = {
    { GPIO_CFG(34, 0, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL,    GPIO_CFG_2MA),
    "BT_REF_CLOCK_ENABLE" },
    };

    static int marimba_bt(int on)
    {
    int rc;
    int i;
    struct marimba config = { .mod_id = MARIMBA_SLAVE_ID_MARIMBA };

    struct marimba_config_register {
    u8 reg;
    u8 value;
    u8 mask;
    };

    struct marimba_variant_register {
    const size_t size;
    const struct marimba_config_register *set;
    };

    const struct marimba_config_register *p;

    u8 version;

    const struct marimba_config_register v10_bt_on[] = {
    { 0xE5, 0x0B, 0x0F },
    { 0x05, 0x02, 0x07 },
    { 0x06, 0x88, 0xFF },
    { 0xE7, 0x21, 0x21 },
    { 0xE3, 0x38, 0xFF },
    { 0xE4, 0x06, 0xFF },
    };

    const struct marimba_config_register v10_bt_off[] = {
    { 0xE5, 0x0B, 0x0F },
    { 0x05, 0x08, 0x0F },
    { 0x06, 0x88, 0xFF },
    { 0xE7, 0x00, 0x21 },
    { 0xE3, 0x00, 0xFF },
    { 0xE4, 0x00, 0xFF },
    };

    const struct marimba_config_register v201_bt_on[] = {
    { 0x05, 0x08, 0x07 },
    { 0x06, 0x88, 0xFF },
    { 0xE7, 0x21, 0x21 },
    { 0xE3, 0x38, 0xFF },
    { 0xE4, 0x06, 0xFF },
    };

    const struct marimba_config_register v201_bt_off[] = {
    { 0x05, 0x08, 0x07 },
    { 0x06, 0x88, 0xFF },
    { 0xE7, 0x00, 0x21 },
    { 0xE3, 0x00, 0xFF },
    { 0xE4, 0x00, 0xFF },
    };

    const struct marimba_config_register v210_bt_on[] = {
    { 0xE9, 0x01, 0x01 },
    { 0x06, 0x88, 0xFF },
    { 0xE7, 0x21, 0x21 },
    { 0xE3, 0x38, 0xFF },
    { 0xE4, 0x06, 0xFF },
    };

    const struct marimba_config_register v210_bt_off[] = {
    { 0x06, 0x88, 0xFF },
    { 0xE7, 0x00, 0x21 },
    { 0xE9, 0x00, 0x01 },
    { 0xE3, 0x00, 0xFF },
    { 0xE4, 0x00, 0xFF },
    };

    const struct marimba_variant_register bt_marimba[2][4] = {
    {
    { ARRAY_SIZE(v10_bt_off), v10_bt_off },
    { 0, NULL },
    { ARRAY_SIZE(v201_bt_off), v201_bt_off },
    { ARRAY_SIZE(v210_bt_off), v210_bt_off }
    },
    {
    { ARRAY_SIZE(v10_bt_on), v10_bt_on },
    { 0, NULL },
    { ARRAY_SIZE(v201_bt_on), v201_bt_on },
    { ARRAY_SIZE(v210_bt_on), v210_bt_on }
    }
    };

    on = on ? 1 : 0;

    rc = marimba_read_bit_mask(&config, 0x11,  &version, 1, 0x1F);
    if (rc < 0) {
    printk(KERN_ERR
    "%s: version read failed: %d\n",
    __func__, rc);
    return rc;
    }

    if ((version >= ARRAY_SIZE(bt_marimba[on])) ||
    (bt_marimba[on][version].size == 0)) {
    printk(KERN_ERR
    "%s: unsupported version\n",
    __func__);
    return -EIO;
    }

    p = bt_marimba[on][version].set;

    printk(KERN_INFO "%s: found version %d\n", __func__, version);

        for (i = 0; i < bt_marimba[on][version].size; i++) {
            u8 value = (p+i)->value;
            rc = marimba_write_bit_mask(&config,
            (p+i)->reg,
            &value,
            sizeof((p+i)->value),
            (p+i)->mask);
            if (rc < 0) {
            printk(KERN_ERR
            "%s: reg %d write failed: %d\n",
            __func__, (p+i)->reg, rc);
            return rc;
            }
            printk(KERN_INFO "%s: reg 0x%02x value 0x%02x mask 0x%02x\n",
            __func__, (p+i)->reg,
            value, (p+i)->mask);
        }
        return 0;
    }

    static const char *vregs_bt_name[] = {
    "gp16",
    "s2",
    "wlan"
    };
    static struct vreg *vregs_bt[ARRAY_SIZE(vregs_bt_name)];

    static int bluetooth_power_regulators(int on)
    {
    int i, rc;

    for (i = 0; i < ARRAY_SIZE(vregs_bt_name); i++) {
    rc = on ? vreg_enable(vregs_bt[i]) :
    vreg_disable(vregs_bt[i]);
    if (rc < 0) {
    printk(KERN_ERR "%s: vreg %s %s failed (%d)\n",
    __func__, vregs_bt_name[i],
        on ? "enable" : "disable", rc);
    return -EIO;
    }
    }
    return 0;
    }

    static int bluetooth_power(int on)
    {
    int rc;
    const char *id = "BTPW";

    if (on) {
    rc = pmapp_vreg_level_vote(id, PMAPP_VREG_S2, 1300);
    if (rc < 0) {
    printk(KERN_ERR "%s: vreg level on failed (%d)\n",
    __func__, rc);
    return rc;
    }

    rc = bluetooth_power_regulators(on);
    if (rc < 0)
    return -EIO;

    rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
     PMAPP_CLOCK_VOTE_ON);
    if (rc < 0)
    return -EIO;

    gpio_set_value_cansleep(GPIO_PIN(bt_config_clock->gpio_cfg), 1);

    rc = marimba_bt(on);
    if (rc < 0)
    return -EIO;

    msleep(10);

    rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
     PMAPP_CLOCK_VOTE_PIN_CTRL);
    if (rc < 0)
    return -EIO;

    gpio_set_value_cansleep(GPIO_PIN(bt_config_clock->gpio_cfg), 0);

    rc = msm_gpios_enable(bt_config_power_on,
    ARRAY_SIZE(bt_config_power_on));

    if (rc < 0)
    return rc;

    } else {
    rc = msm_gpios_enable(bt_config_power_off,
    ARRAY_SIZE(bt_config_power_off));
    if (rc < 0)
    return rc;

    /* check for initial RFKILL block (power off) */
    if (platform_get_drvdata(&msm_bt_power_device) == NULL)
    goto out;

    rc = marimba_bt(on);
    if (rc < 0)
    return -EIO;

    //gpio_set_value_cansleep(GPIO_PIN(bt_config_clock->gpio_cfg), 0);

    rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
     PMAPP_CLOCK_VOTE_OFF);
    if (rc < 0)
    return -EIO;

    rc = bluetooth_power_regulators(on);
    if (rc < 0)
    return -EIO;

    rc = pmapp_vreg_level_vote(id, PMAPP_VREG_S2, 0);
    if (rc < 0) {
    printk(KERN_INFO "%s: vreg level off failed (%d)\n",
    __func__, rc);
    }
    }

    out:
    printk(KERN_DEBUG "Bluetooth power switch: %d\n", on);

    return 0;
    }

    static void __init bt_power_init(void)
    {
    int i, rc;

    for (i = 0; i < ARRAY_SIZE(vregs_bt_name); i++) {
    vregs_bt[i] = vreg_get(NULL, vregs_bt_name[i]);
    if (IS_ERR(vregs_bt[i])) {
    printk(KERN_ERR "%s: vreg get %s failed (%ld)\n",
        __func__, vregs_bt_name[i],
        PTR_ERR(vregs_bt[i]));
    return;
    }
    }

    rc = msm_gpios_request_enable(bt_config_clock,
    ARRAY_SIZE(bt_config_clock));
    if (rc < 0) {
    printk(KERN_ERR
    "%s: msm_gpios_request_enable failed (%d)\n",
    __func__, rc);
    return;
    }

    rc = gpio_direction_output(GPIO_PIN(bt_config_clock->gpio_cfg),
    0);
    if (rc < 0) {
    printk(KERN_ERR
    "%s: gpio_direction_output failed (%d)\n",
    __func__, rc);
    return;
    }

    msm_bt_power_device.dev.platform_data = &bluetooth_power;
    }
#else
#define bt_power_init(x) do {} while (0)
#endif
#else //shiks_test
 static struct resource bluesleep_resources_01[] = {
     {
     .name = "gpio_host_wake",
     .start = GPIO_BT_HOST_WAKE_01,
     .end = GPIO_BT_HOST_WAKE_01,
     .flags = IORESOURCE_IO,
     },
     {
     .name = "gpio_ext_wake",
     .start = GPIO_BT_WAKE,//81,//35,
     .end = GPIO_BT_WAKE,//81, //35,
     .flags = IORESOURCE_IO,
     },
     {
     .name = "host_wake",
     .start = MSM_GPIO_TO_INT(GPIO_BT_HOST_WAKE_01),
     .end = MSM_GPIO_TO_INT(GPIO_BT_HOST_WAKE_01),
     .flags = IORESOURCE_IRQ,
     },
 };

static struct resource bluesleep_resources_02[] = {
     {
     .name = "gpio_host_wake",
     .start = GPIO_BT_HOST_WAKE_02,
     .end = GPIO_BT_HOST_WAKE_02,
     .flags = IORESOURCE_IO,
     },
     {
     .name = "gpio_ext_wake",
     .start = GPIO_BT_WAKE,//81,//35,
     .end = GPIO_BT_WAKE,//81, //35,
     .flags = IORESOURCE_IO,
     },
     {
     .name = "host_wake",
     .start = MSM_GPIO_TO_INT(GPIO_BT_HOST_WAKE_02),
     .end = MSM_GPIO_TO_INT(GPIO_BT_HOST_WAKE_02),
     .flags = IORESOURCE_IRQ,
     },
 };

static struct resource bluesleep_resources_rev05[] = {
     {
     .name = "gpio_host_wake",
     .start = GPIO_BT_HOST_WAKE_REV05,
     .end = GPIO_BT_HOST_WAKE_REV05,
     .flags = IORESOURCE_IO,
     },
     {
     .name = "gpio_ext_wake",
     .start = GPIO_BT_WAKE,
     .end = GPIO_BT_WAKE,
     .flags = IORESOURCE_IO,
     },
     {
     .name = "host_wake",
     .start = MSM_GPIO_TO_INT(GPIO_BT_HOST_WAKE_REV05),
     .end = MSM_GPIO_TO_INT(GPIO_BT_HOST_WAKE_REV05),
     .flags = IORESOURCE_IRQ,
     },
 };

 static struct platform_device msm_bluesleep_device_01 = {
     .name = "bluesleep",
     .id = -1,
     .num_resources = ARRAY_SIZE(bluesleep_resources_01),
     .resource = bluesleep_resources_01,
 };

  static struct platform_device msm_bluesleep_device_02 = {
     .name = "bluesleep",
     .id = -1,
     .num_resources = ARRAY_SIZE(bluesleep_resources_02),
     .resource = bluesleep_resources_02,
 };

  static struct platform_device msm_bluesleep_device_rev05 = {
     .name = "bluesleep",
     .id = -1,
     .num_resources = ARRAY_SIZE(bluesleep_resources_rev05),
     .resource = bluesleep_resources_rev05,
 };


 extern int bluesleep_start(void);
 extern void bluesleep_stop(void);
 static struct platform_device msm_bt_power_device = {
 .name = "bt_power",
 //shiks_EA26.id     = -1
 };

 static unsigned bt_config_power_on_01[] = {
     GPIO_CFG(GPIO_BT_WAKE,     0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* WAKE */
     GPIO_CFG(GPIO_BT_UART_RTS, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* RFR */ //shiks_EA26
     GPIO_CFG(GPIO_BT_UART_CTS, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* CTS *///shiks_EA26
     GPIO_CFG(GPIO_BT_UART_RXD, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* Rx */ //shiks_EA26
     GPIO_CFG(GPIO_BT_UART_TXD, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* Tx */ //shiks_EA26
     GPIO_CFG(GPIO_BT_PCM_DOUT, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* PCM_DOUT */
     GPIO_CFG(GPIO_BT_PCM_DIN,  1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* PCM_DIN */
     GPIO_CFG(GPIO_BT_PCM_SYNC, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* PCM_SYNC */ //shiks_EA26
     GPIO_CFG(GPIO_BT_PCM_CLK,  1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* PCM_CLK */  //shiks_EA26
     GPIO_CFG(GPIO_BT_HOST_WAKE_01, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* HOST_WAKE */
 };
  static unsigned bt_config_power_on_02[] = {
     GPIO_CFG(GPIO_BT_WAKE,     0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* WAKE */
     GPIO_CFG(GPIO_BT_UART_RTS, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* RFR */ //shiks_EA26
     GPIO_CFG(GPIO_BT_UART_CTS, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* CTS *///shiks_EA26
     GPIO_CFG(GPIO_BT_UART_RXD, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* Rx */ //shiks_EA26
     GPIO_CFG(GPIO_BT_UART_TXD, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* Tx */ //shiks_EA26
     GPIO_CFG(GPIO_BT_PCM_DOUT, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* PCM_DOUT */
     GPIO_CFG(GPIO_BT_PCM_DIN,  1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* PCM_DIN */
     GPIO_CFG(GPIO_BT_PCM_SYNC, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* PCM_SYNC */ //shiks_EA26
     GPIO_CFG(GPIO_BT_PCM_CLK,  1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* PCM_CLK */  //shiks_EA26
     GPIO_CFG(GPIO_BT_HOST_WAKE_02, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* HOST_WAKE */
 };
  static unsigned bt_config_power_on_rev05[] = {
     GPIO_CFG(GPIO_BT_WAKE,     0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* WAKE */
     GPIO_CFG(GPIO_BT_UART_RTS, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* RFR */
     GPIO_CFG(GPIO_BT_UART_CTS, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* CTS */
     GPIO_CFG(GPIO_BT_UART_RXD, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* Rx */
     GPIO_CFG(GPIO_BT_UART_TXD, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* Tx */
     GPIO_CFG(GPIO_BT_PCM_DOUT, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* PCM_DOUT */
     GPIO_CFG(GPIO_BT_PCM_DIN,  1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* PCM_DIN */
     GPIO_CFG(GPIO_BT_PCM_SYNC, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* PCM_SYNC */
     GPIO_CFG(GPIO_BT_PCM_CLK,  1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* PCM_CLK */
     GPIO_CFG(GPIO_BT_HOST_WAKE_REV05, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),    /* HOST_WAKE */
 };

 static unsigned bt_config_power_off_01[] = {
     GPIO_CFG(GPIO_BT_WAKE,     0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* WAKE */
     GPIO_CFG(GPIO_BT_UART_RTS, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* RFR */
     GPIO_CFG(GPIO_BT_UART_CTS, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* CTS */
     GPIO_CFG(GPIO_BT_UART_RXD, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* Rx */
     GPIO_CFG(GPIO_BT_UART_TXD, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* Tx */
     GPIO_CFG(GPIO_BT_PCM_DOUT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* PCM_DOUT */
     GPIO_CFG(GPIO_BT_PCM_DIN,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* PCM_DIN */
     GPIO_CFG(GPIO_BT_PCM_SYNC, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* PCM_SYNC */
     GPIO_CFG(GPIO_BT_PCM_CLK,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* PCM_CLK */
     GPIO_CFG(GPIO_BT_HOST_WAKE_01, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* HOST_WAKE */
 }; 
  static unsigned bt_config_power_off_02[] = {
     GPIO_CFG(GPIO_BT_WAKE,     0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* WAKE */
     GPIO_CFG(GPIO_BT_UART_RTS, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* RFR */
     GPIO_CFG(GPIO_BT_UART_CTS, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* CTS */
     GPIO_CFG(GPIO_BT_UART_RXD, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* Rx */
     GPIO_CFG(GPIO_BT_UART_TXD, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* Tx */
     GPIO_CFG(GPIO_BT_PCM_DOUT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* PCM_DOUT */
     GPIO_CFG(GPIO_BT_PCM_DIN,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* PCM_DIN */
     GPIO_CFG(GPIO_BT_PCM_SYNC, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* PCM_SYNC */
     GPIO_CFG(GPIO_BT_PCM_CLK,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* PCM_CLK */
     GPIO_CFG(GPIO_BT_HOST_WAKE_02, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* HOST_WAKE */
 };
  static unsigned bt_config_power_off_rev05[] = {
     GPIO_CFG(GPIO_BT_WAKE,     0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* WAKE */
     GPIO_CFG(GPIO_BT_UART_RTS, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* RFR */
     GPIO_CFG(GPIO_BT_UART_CTS, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* CTS */
     GPIO_CFG(GPIO_BT_UART_RXD, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* Rx */
     GPIO_CFG(GPIO_BT_UART_TXD, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* Tx */
     GPIO_CFG(GPIO_BT_PCM_DOUT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* PCM_DOUT */
     GPIO_CFG(GPIO_BT_PCM_DIN,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* PCM_DIN */
     GPIO_CFG(GPIO_BT_PCM_SYNC, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* PCM_SYNC */
     GPIO_CFG(GPIO_BT_PCM_CLK,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* PCM_CLK */
     GPIO_CFG(GPIO_BT_HOST_WAKE_REV05, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),    /* HOST_WAKE */
 };


 static int bluetooth_power(int on)
 {
     struct vreg *vreg_bt;
     int pin, rc;

     pr_info("bluetooth_power \n");

     printk(KERN_DEBUG "%s\n", __func__);

     if (on) {
         if(system_rev >= 5)
             config_gpio_table(bt_config_power_on_rev05, ARRAY_SIZE(bt_config_power_on_rev05));
         else if(system_rev >= 2)
             config_gpio_table(bt_config_power_on_02, ARRAY_SIZE(bt_config_power_on_02));
         else
            config_gpio_table(bt_config_power_on_01, ARRAY_SIZE(bt_config_power_on_01));

         //pr_info("bluetooth_power BT_WAKE:%d, HOST_WAKE:%d, REG_ON:%d\n", gpio_get_value(GPIO_BT_WAKE), gpio_get_value((system_rev>=2)?GPIO_BT_HOST_WAKE_02 : GPIO_BT_HOST_WAKE_01), gpio_get_value(GPIO_BT_WLAN_REG_ON));
         gpio_direction_output(GPIO_BT_WAKE, GPIO_WLAN_LEVEL_HIGH);
         gpio_direction_output(GPIO_BT_WLAN_REG_ON, GPIO_WLAN_LEVEL_HIGH);
         //mdelay(150);
         usleep(150000);//shiks_EA26
         gpio_direction_output(GPIO_BT_RESET, GPIO_WLAN_LEVEL_HIGH);

         //pr_info("bluetooth_power BT_WAKE:%d, HOST_WAKE:%d, REG_ON:%d\n", gpio_get_value(GPIO_BT_WAKE), gpio_get_value((system_rev>=2)?GPIO_BT_HOST_WAKE_02 : GPIO_BT_HOST_WAKE_01), gpio_get_value(GPIO_BT_WLAN_REG_ON));
         //shiks_EA26        mdelay(100);   

         bluesleep_start();
     } 
     else 
     {    
         bluesleep_stop();
         gpio_direction_output(GPIO_BT_RESET, GPIO_WLAN_LEVEL_LOW);/* BT_VREG_CTL */

         if( gpio_get_value(GPIO_WLAN_RESET) == GPIO_WLAN_LEVEL_LOW ) //SEC_BLUETOOTH : pjh_2010.06.30
         {
             gpio_direction_output(GPIO_BT_WLAN_REG_ON, GPIO_WLAN_LEVEL_LOW);/* BT_RESET */
             mdelay(150);
         }
         gpio_direction_output(GPIO_BT_WAKE, GPIO_WLAN_LEVEL_LOW);/* BT_VREG_CTL */

         if(system_rev >= 5)
             config_gpio_table(bt_config_power_off_rev05, ARRAY_SIZE(bt_config_power_off_rev05));
         else if(system_rev >= 2)
             config_gpio_table(bt_config_power_off_02, ARRAY_SIZE(bt_config_power_off_02));
         else
             config_gpio_table(bt_config_power_off_01, ARRAY_SIZE(bt_config_power_off_01));
     }
     return 0;
 }

 static void __init bt_power_init(void)
 {
     pr_info("bt_power_init \n");

     msm_bt_power_device.dev.platform_data = &bluetooth_power;
 }
 //shiks_EA26
 static int bluetooth_gpio_init(void)
 {
     pr_info("bluetooth_gpio_init on system_rev:%d\n", system_rev);

    if(system_rev == 1)
      config_gpio_table(bt_config_power_on_01, ARRAY_SIZE(bt_config_power_on_01));
    else if(system_rev >= 2)
      config_gpio_table(bt_config_power_on_02, ARRAY_SIZE(bt_config_power_on_02));
    else
      config_gpio_table(bt_config_power_on_01, ARRAY_SIZE(bt_config_power_on_01));
     return 0;
 }
 //shiks_EA26
#endif//shiks_test

static struct msm_psy_batt_pdata msm_psy_batt_data = {
	.voltage_min_design 	= 3400,	//2800,
	.voltage_max_design	= 4200,	//4300,
	.avail_chg_sources   	= AC_CHG | USB_CHG ,
	.batt_technology        = POWER_SUPPLY_TECHNOLOGY_LION,
};

static struct platform_device samsung_batt_device = {
	.name 		    = "samsung-battery",
	.id		    = -1,
	.dev.platform_data  = &msm_psy_batt_data,
};

/* ATHENV */
static struct platform_device msm_wlan_ar6000_pm_device = {
	.name		= "wlan_ar6000_pm",
	.id		= 1,
	.num_resources	= 0,
	.resource	= NULL,
};
/* ATHENV */

static char *msm_adc_surf_device_names[] = {
	"XO_ADC",
};

static struct msm_adc_platform_data msm_adc_pdata;

static struct platform_device msm_adc_device = {
	.name   = "msm_adc",
	.id = -1,
	.dev = {
		.platform_data = &msm_adc_pdata,
	},
};

static void vital2_switch_init(void)
{
	sec_class = class_create(THIS_MODULE, "sec");
	
	if (IS_ERR(sec_class))
		pr_err("Failed to create class(sec)!\n");

	switch_dev = device_create(sec_class, NULL, 0, NULL, "switch");
	
	if (IS_ERR(switch_dev))
		pr_err("Failed to create device(switch)!\n");

};

#ifdef CONFIG_MSM_SDIO_AL
static struct msm_gpio mdm2ap_status = {
	GPIO_CFG(77, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	"mdm2ap_status"
};

struct platform_device msm_device_sdio_al = {
	.name = "msm_sdio_al",
	.id = -1,
	.dev		= {
		.platform_data	= &mdm2ap_status,
	},
};

#endif /* CONFIG_MSM_SDIO_AL */

#ifndef ATH_POLLING
static void (*wlan_status_notify_cb)(int card_present, void *dev_id);
void *wlan_devid;

static int register_wlan_status_notify(void (*callback)(int card_present, void *dev_id), void *dev_id)
{
	wlan_status_notify_cb = callback;
	wlan_devid = dev_id;
	return 0;
}

static unsigned int wlan_status(struct device *dev)
{
	int rc;
	rc = gpio_get_value(GPIO_WLAN_RESET);

	return rc;
}
#endif /* ATH_POLLING */

#ifdef CONFIG_WIFI_CONTROL_FUNC
/*
static unsigned int wlan_sdio_on_table[][4] = {
	{GPIO_WLAN_SDIO_CLK, GPIO_WLAN_SDIO_CLK_AF, GPIO_LEVEL_NONE,
		S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_CMD, GPIO_WLAN_SDIO_CMD_AF, GPIO_LEVEL_NONE,
		S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D0, GPIO_WLAN_SDIO_D0_AF, GPIO_LEVEL_NONE,
		S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D1, GPIO_WLAN_SDIO_D1_AF, GPIO_LEVEL_NONE,
		S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D2, GPIO_WLAN_SDIO_D2_AF, GPIO_LEVEL_NONE,
		S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D3, GPIO_WLAN_SDIO_D3_AF, GPIO_LEVEL_NONE,
		S3C_GPIO_PULL_NONE},
};

static unsigned int wlan_sdio_off_table[][4] = {
	{GPIO_WLAN_SDIO_CLK, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_CMD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
};
*/

static uint32_t wlan_gpio_table_rev05[] = {
	/* GPIO_WLAN_WAKES_MSM */
	GPIO_CFG(GPIO_WLAN_WAKES_MSM_REV05, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	/* BT_WLAN_REG_ON */
	GPIO_CFG(GPIO_BT_WLAN_REG_ON, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	/* GPIO_WLAN_RESET */
	GPIO_CFG(GPIO_WLAN_RESET, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA)
};

static uint32_t wlan_gpio_table_02[] = {
	/* GPIO_WLAN_WAKES_MSM */
	GPIO_CFG(GPIO_WLAN_WAKES_MSM_02, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	
	/* BT_WLAN_REG_ON */
	GPIO_CFG(GPIO_BT_WLAN_REG_ON, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	/* GPIO_WLAN_RESET */
	GPIO_CFG(GPIO_WLAN_RESET, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA)
};

static uint32_t wlan_gpio_table_01[] = {
	/* GPIO_WLAN_WAKES_MSM */
	GPIO_CFG(GPIO_WLAN_WAKES_MSM_01, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	
	/* BT_WLAN_REG_ON */
	//GPIO_CFG(GPIO_BT_WLAN_REG_ON, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	/* GPIO_WLAN_RESET */
	//GPIO_CFG(GPIO_WLAN_RESET, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA)
};


static void config_wlan_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
static int wlan_init (void)
{
#if 0
	gpio_direction_output(GPIO_BT_WLAN_REG_ON, GPIO_WLAN_LEVEL_LOW);
	msleep(100);
#endif
	gpio_direction_output(GPIO_WLAN_RESET, GPIO_WLAN_LEVEL_LOW);
	msleep (100);
}

static int wlan_power_en(int onoff)
{
#if 0
	if (onoff) {
		/* Wlan Gpio Table */				
		if(system_rev >= 5) {
			config_wlan_gpio_table(wlan_gpio_table_rev05, ARRAY_SIZE(wlan_gpio_table_rev05));
		}
		else {
			config_wlan_gpio_table(wlan_gpio_table_01, ARRAY_SIZE(wlan_gpio_table_01));				
		}
		msleep(200);	

		if (gpio_get_value (GPIO_BT_WLAN_REG_ON) == GPIO_WLAN_LEVEL_LOW)
			gpio_set_value (GPIO_BT_WLAN_REG_ON, GPIO_WLAN_LEVEL_HIGH);
		msleep(150);	
		gpio_set_value (GPIO_WLAN_RESET, GPIO_WLAN_LEVEL_HIGH);
		msleep (150);
	} else {
		gpio_set_value (GPIO_WLAN_RESET, GPIO_WLAN_LEVEL_LOW);
		msleep (150);
		if (gpio_get_value(GPIO_BT_RESET) == GPIO_WLAN_LEVEL_LOW) {
			msleep(150);
			gpio_set_value (GPIO_BT_WLAN_REG_ON, GPIO_WLAN_LEVEL_LOW);
		}
	}
	#endif
	
	if(onoff)
	gpio_set_value(GPIO_WLAN_RESET, GPIO_WLAN_LEVEL_HIGH);
	else
	gpio_set_value(GPIO_WLAN_RESET, GPIO_WLAN_LEVEL_LOW);
#ifndef ATH_POLLING
			msleep(250);
#endif /* ATH_POLLING */

	/* Detect card */
	if (wlan_status_notify_cb){
		
		printk(KERN_ERR "wlan_carddetect_en2 = %d ~~~\n", onoff);
		wlan_status_notify_cb(onoff, wlan_devid);
		}
	else
		printk(KERN_ERR "WLAN: No notify available\n");
	
	return 0;
}
#else  //CONFIG_MMC_MSM_SDC2_SUPPORT
static int wlan_init (void)
{
	gpio_direction_output(GPIO_BT_WLAN_REG_ON, GPIO_WLAN_LEVEL_LOW);
		msleep(100);

	gpio_direction_output(GPIO_WLAN_RESET, GPIO_WLAN_LEVEL_LOW);
	msleep (100);
}
static int wlan_power_en(int onoff)
{
	if (onoff) {
		/* Wlan Gpio Table */				
		if(system_rev == 1)
		{
			config_wlan_gpio_table(wlan_gpio_table_01,
				ARRAY_SIZE(wlan_gpio_table_01));
			msleep(200);		
		}	

		if (gpio_get_value (GPIO_BT_WLAN_REG_ON) == GPIO_WLAN_LEVEL_LOW)
			gpio_set_value (GPIO_BT_WLAN_REG_ON, GPIO_WLAN_LEVEL_HIGH);
		msleep(150);	
		gpio_set_value (GPIO_WLAN_RESET, GPIO_WLAN_LEVEL_HIGH);
		msleep (150);
	} else {
		gpio_set_value (GPIO_WLAN_RESET, GPIO_WLAN_LEVEL_LOW);
		msleep (150);
		if (gpio_get_value(GPIO_BT_RESET) == GPIO_WLAN_LEVEL_LOW) {
			msleep(150);
			gpio_set_value (GPIO_BT_WLAN_REG_ON, GPIO_WLAN_LEVEL_LOW);
		}
	}

#ifndef ATH_POLLING
			msleep(250);
#endif /* ATH_POLLING */

	/* Detect card */
	if (wlan_status_notify_cb){
		
		printk(KERN_ERR "wlan_carddetect_en2 = %d ~~~\n", onoff);
		wlan_status_notify_cb(onoff, wlan_devid);
		}
	else
		printk(KERN_ERR "WLAN: No notify available\n");
	
	return 0;
}

#endif  //CONFIG_MMC_MSM_SDC2_SUPPORT

static int wlan_reset_en(int onoff)
{
	gpio_direction_output(GPIO_WLAN_RESET,
			onoff ? GPIO_WLAN_LEVEL_HIGH : GPIO_WLAN_LEVEL_LOW);
	return 0;
}

static int wlan_carddetect_en(int onoff)
{
	printk(KERN_ERR "wlan_carddetect_en = %d ~~~\n", onoff);
    if(!onoff) msleep(500);
 
//	u32 i;
//	u32 sdio;
/*
	if (onoff) {
		for (i = 0; i < ARRAY_SIZE(wlan_sdio_on_table); i++) {
			sdio = wlan_sdio_on_table[i][0];
			s3c_gpio_cfgpin(sdio,
					S3C_GPIO_SFN(wlan_sdio_on_table[i][1]));
			s3c_gpio_setpull(sdio, wlan_sdio_on_table[i][3]);
			if (wlan_sdio_on_table[i][2] != GPIO_LEVEL_NONE)
				gpio_set_value(sdio, wlan_sdio_on_table[i][2]);
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(wlan_sdio_off_table); i++) {
			sdio = wlan_sdio_off_table[i][0];
			s3c_gpio_cfgpin(sdio,
				S3C_GPIO_SFN(wlan_sdio_off_table[i][1]));
			s3c_gpio_setpull(sdio, wlan_sdio_off_table[i][3]);
			if (wlan_sdio_off_table[i][2] != GPIO_LEVEL_NONE)
				gpio_set_value(sdio, wlan_sdio_off_table[i][2]);
		}
	}
	udelay(5);

	sdhci_s3c_force_presence_change(&s3c_device_hsmmc1);
*/	
	return 0;
}

static struct resource wifi_resources_01[] = {
	[0] = {
		.name	= WLAN_DEVICE_IRQ,
		.start	= MSM_GPIO_TO_INT(GPIO_WLAN_WAKES_MSM_01),
		.end	= MSM_GPIO_TO_INT(GPIO_WLAN_WAKES_MSM_01),
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL,
	},
};

static struct resource wifi_resources_02[] = {
	[0] = {
		.name	= WLAN_DEVICE_IRQ,
		.start	= MSM_GPIO_TO_INT(GPIO_WLAN_WAKES_MSM_02),
		.end	= MSM_GPIO_TO_INT(GPIO_WLAN_WAKES_MSM_02),
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL,
	},
};

static struct resource wifi_resources_rev05[] = {
	[0] = {
		.name	= WLAN_DEVICE_IRQ,
		.start	= MSM_GPIO_TO_INT(GPIO_WLAN_WAKES_MSM_REV05),
		.end	= MSM_GPIO_TO_INT(GPIO_WLAN_WAKES_MSM_REV05),
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL,
	},
};

static struct wifi_mem_prealloc wifi_mem_array[PREALLOC_WLAN_SEC_NUM] = {
	{NULL, (WLAN_SECTION_SIZE_0 + PREALLOC_WLAN_SECTION_HEADER)},
	{NULL, (WLAN_SECTION_SIZE_1 + PREALLOC_WLAN_SECTION_HEADER)},
	{NULL, (WLAN_SECTION_SIZE_2 + PREALLOC_WLAN_SECTION_HEADER)},
	{NULL, (WLAN_SECTION_SIZE_3 + PREALLOC_WLAN_SECTION_HEADER)}
};

void *wlan_mem_prealloc(int section, unsigned long size)
{
	if (section == PREALLOC_WLAN_SEC_NUM)
		return wlan_static_skb;
	if ((section < 0) || (section > PREALLOC_WLAN_SEC_NUM))
		return NULL;
	if (wifi_mem_array[section].size < size)
		return NULL;

	return wifi_mem_array[section].mem_ptr;
}
EXPORT_SYMBOL(wlan_mem_prealloc);
#endif  //CONFIG_WIFI_CONTROL_FUNC

#define DHD_SKB_HDRSIZE 		336
#define DHD_SKB_1PAGE_BUFSIZE	((PAGE_SIZE*1)-DHD_SKB_HDRSIZE)
#define DHD_SKB_2PAGE_BUFSIZE	((PAGE_SIZE*2)-DHD_SKB_HDRSIZE)
#define DHD_SKB_4PAGE_BUFSIZE	((PAGE_SIZE*4)-DHD_SKB_HDRSIZE)

//[ use wlan static buffer 
int __init aries_init_wifi_mem(void)
{
	int i;
	int j;

	printk("aries_init_wifi_mem\n");
	for (i = 0; i < 8; i++) {
		wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_1PAGE_BUFSIZE);
		if (!wlan_static_skb[i])
			goto err_skb_alloc;
	}
	
	for (; i < 16; i++) {
		wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_2PAGE_BUFSIZE);
		if (!wlan_static_skb[i])
			goto err_skb_alloc;
	}
	
	wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_4PAGE_BUFSIZE);
	if (!wlan_static_skb[i])
		goto err_skb_alloc;

	for (i = 0; i < PREALLOC_WLAN_SEC_NUM; i++) {
		wifi_mem_array[i].mem_ptr = 
			kmalloc(wifi_mem_array[i].size, GFP_KERNEL);

		if (!wifi_mem_array[i].mem_ptr)
			goto err_mem_alloc;
	}
	printk("%s: WIFI MEM Allocated\n", __FUNCTION__);
	return 0;

err_mem_alloc:
	pr_err("Failed to mem_alloc for WLAN\n");
	for (j = 0; j < i; j++)
		kfree(wifi_mem_array[j].mem_ptr);

	i = WLAN_SKB_BUF_NUM;

 err_skb_alloc:
	pr_err("Failed to skb_alloc for WLAN\n");
	for (j = 0; j < i; j++)
		dev_kfree_skb(wlan_static_skb[j]);

	return -ENOMEM;
}
// use wlan static buffer ]

static struct wifi_platform_data wifi_pdata = {
	.set_power		= wlan_power_en,
	.set_reset		= wlan_reset_en,
	.set_carddetect		= wlan_carddetect_en,
	.mem_prealloc		= wlan_mem_prealloc,
};

static struct platform_device sec_device_wifi_01 = {
	.name			= WLAN_DEVICE,
	.id			= 1,
	.num_resources		= ARRAY_SIZE(wifi_resources_01),
	.resource		= wifi_resources_01,
	.dev			= {
		.platform_data = &wifi_pdata,
	},
};

static struct platform_device sec_device_wifi_02 = {
	.name			= WLAN_DEVICE,
	.id			= 1,
	.num_resources		= ARRAY_SIZE(wifi_resources_02),
	.resource		= wifi_resources_02,
	.dev			= {
		.platform_data = &wifi_pdata,
	},
};

static struct platform_device sec_device_wifi_rev05 = {
	.name			= WLAN_DEVICE,
	.id			= 1,
	.num_resources		= ARRAY_SIZE(wifi_resources_rev05),
	.resource		= wifi_resources_rev05,
	.dev			= {
		.platform_data = &wifi_pdata,
	},
};


static struct platform_device *uart3_device[] __initdata = {
#if defined(CONFIG_SERIAL_MSM) || defined(CONFIG_MSM_SERIAL_DEBUGGER)
	&msm_device_uart3,
#endif
};

static struct platform_device *devices_01[] __initdata = {
/* ATHENV */
    /*
     * It is necessary to put here in order to support WoW.
     * Put it before MMC host controller in worst case
     */
	&msm_wlan_ar6000_pm_device,
/* ATHENV */
	&msm_device_smd,
	&msm_device_dmov,
	&msm_device_nand,
#ifdef CONFIG_USB_FUNCTION
	&msm_device_hsusb_peripheral,
	&mass_storage_device,
#endif
#ifdef CONFIG_USB_MSM_OTG_72K
	&msm_device_otg,
#ifdef CONFIG_USB_GADGET
	&msm_device_gadget_peripheral,
#endif
#endif
#ifdef CONFIG_USB_ANDROID
	&usb_mass_storage_device,
	&rndis_device,
#ifdef CONFIG_USB_ANDROID_DIAG
	&usb_diag_device,
#endif
	&android_usb_device,
#endif
#ifdef CONFIG_SPI_QSD
	&qsd_device_spi,
#endif
#ifdef CONFIG_I2C_SSBI
	&msm_device_ssbi6,
	&msm_device_ssbi7,
#endif
	&android_pmem_device,
	&msm_fb_device,
	&msm_migrate_pages_device,
#ifdef CONFIG_MSM_ROTATOR
	&msm_rotator_device,
#endif
#ifdef CONFIG_FB_MSM_LCDC_S6D16A0_WVGA_PANEL
	&lcdc_s6d16a0_panel_device,
#endif
#ifdef CONFIG_FB_MSM_LCDC_LD9040_WVGA_PANEL
	&lcdc_ld9040_panel_device,
#endif
#ifdef CONFIG_FB_MSM_LCDC_S6D05A1_HVGA_PANEL
	&lcdc_s6d05a1_panel_device,
#endif
#ifdef CONFIG_FB_MSM_HDMI_SII9024_PANEL
	&hdmi_sii9024_panel_device,
#endif
	&android_pmem_kernel_ebi1_device,
	&android_pmem_adsp_device,
	&android_pmem_audio_device,
#ifdef PMEM_HDMI
	&android_pmem_hdmi_device,
#endif
//	&msm_device_i2c,
	&msm_device_i2c_2,
	&msm_device_uart_dm1,
	&hs_device,
#ifdef CONFIG_MSM7KV2_AUDIO
	&msm_aictl_device,
	&msm_mi2s_device,
	&msm_lpa_device,
	&msm_aux_pcm_device,
#endif
	&msm_device_adspdec,
//	&qup_device_i2c,
	&touchscreen_i2c_gpio_device,
#ifdef CONFIG_SAMSUNG_JACK
       &sec_device_jack,
#endif
	&camera_i2c_gpio_device,
	&amp_i2c_gpio_device,
	&opt_i2c_gpio_device,
	&micro_usb_i2c_gpio_device,
#ifdef CONFIG_WIBRO_CMC
	&max8893_i2c_gpio_device,
	&wmxeeprom_i2c_gpio_device,
#endif
#if defined(CONFIG_MARIMBA_CORE) && \
   (defined(CONFIG_MSM_BT_POWER) || defined(CONFIG_MSM_BT_POWER_MODULE))
//shiks_test    &msm_bt_power_device,
#endif
	&msm_bt_power_device,//shiks_test
	&msm_bluesleep_device_01, //shiks_test
	&msm_device_kgsl,
#ifdef CONFIG_S5K3E2FX
	&msm_camera_sensor_s5k3e2fx,
#endif
#ifdef CONFIG_SENSOR_M5MO
	&msm_camera_sensor_m5mo,
#endif
#ifdef CONFIG_SENSOR_S5K5AAFA
	&msm_camera_sensor_s5k5aafa,		
#endif
#ifdef CONFIG_S5K5CCGX
	&msm_camera_sensor_s5k5ccgx,		
#endif
#ifdef CONFIG_SR030PC30
	&msm_camera_sensor_sr030pc30,		
#endif
	&msm_device_vidc_720p,
#ifdef CONFIG_MSM_GEMINI
	&msm_gemini_device,
#endif
#ifdef CONFIG_MSM_VPE
	&msm_vpe_device,
#endif
#ifdef CONFIG_TOUCHSCREEN_QT602240
	&touchscreen_device_qt602240,
#endif
#ifdef CONFIG_TOUCHSCREEN_MELFAS
	&touchscreen_device_melfas,
#endif
#ifdef CONFIG_MSM_SDIO_AL
	&msm_device_sdio_al,
#endif
	&samsung_batt_device,
	&msm_adc_device,
	&msm_vibrator_device,

	&opt_gp2a,

	&acc_gyro_mag_i2c_gpio_device,
	&fuelgauge_i2c_gpio_device,
#ifdef CONFIG_WIFI_CONTROL_FUNC
	&sec_device_wifi_01,
#endif
};

static struct platform_device *devices_02[] __initdata = {
/* ATHENV */
    /*
     * It is necessary to put here in order to support WoW.
     * Put it before MMC host controller in worst case
     */
	&msm_wlan_ar6000_pm_device,
/* ATHENV */
	&msm_device_smd,
	&msm_device_dmov,
	&msm_device_nand,
#ifdef CONFIG_USB_FUNCTION
	&msm_device_hsusb_peripheral,
	&mass_storage_device,
#endif
#ifdef CONFIG_USB_MSM_OTG_72K
	&msm_device_otg,
#ifdef CONFIG_USB_GADGET
	&msm_device_gadget_peripheral,
#endif
#endif
#ifdef CONFIG_USB_ANDROID
	&usb_mass_storage_device,
	&rndis_device,
#ifdef CONFIG_USB_ANDROID_DIAG
	&usb_diag_device,
#endif
	&android_usb_device,
#endif
#ifdef CONFIG_SPI_QSD
	&qsd_device_spi,
#endif
#ifdef CONFIG_I2C_SSBI
	&msm_device_ssbi6,
	&msm_device_ssbi7,
#endif
	&android_pmem_device,
	&msm_fb_device,
	&msm_migrate_pages_device,
#ifdef CONFIG_MSM_ROTATOR
	&msm_rotator_device,
#endif
#ifdef CONFIG_FB_MSM_LCDC_S6D16A0_WVGA_PANEL
	&lcdc_s6d16a0_panel_device,
#endif
#ifdef CONFIG_FB_MSM_LCDC_LD9040_WVGA_PANEL
	&lcdc_ld9040_panel_device,
#endif
#ifdef CONFIG_FB_MSM_LCDC_S6D05A1_HVGA_PANEL
	&lcdc_s6d05a1_panel_device,
#endif
#ifdef CONFIG_FB_MSM_HDMI_SII9024_PANEL
	&hdmi_sii9024_panel_device,
#endif
	&android_pmem_kernel_ebi1_device,
	&android_pmem_adsp_device,
	&android_pmem_audio_device,
#ifdef PMEM_HDMI
	&android_pmem_hdmi_device,
#endif
//	&msm_device_i2c,
	&msm_device_i2c_2,
	&msm_device_uart_dm1,
	&hs_device,
#ifdef CONFIG_MSM7KV2_AUDIO
	&msm_aictl_device,
	&msm_mi2s_device,
	&msm_lpa_device,
	&msm_aux_pcm_device,
#endif
	&msm_device_adspdec,
//	&qup_device_i2c,
	&touchscreen_i2c_gpio_device,
#ifdef CONFIG_SAMSUNG_JACK
       &sec_device_jack,
#endif
	&camera_i2c_gpio_device,
	&amp_i2c_gpio_device,
	&opt_i2c_gpio_device,
	&micro_usb_i2c_gpio_device,
#ifdef CONFIG_WIBRO_CMC
	&max8893_i2c_gpio_device,
	&wmxeeprom_i2c_gpio_device,
#endif
#if defined(CONFIG_MARIMBA_CORE) && \
   (defined(CONFIG_MSM_BT_POWER) || defined(CONFIG_MSM_BT_POWER_MODULE))
//shiks_test    &msm_bt_power_device,
#endif
	&msm_bt_power_device,//shiks_test
	&msm_bluesleep_device_02, //shiks_test
	&msm_device_kgsl,
#ifdef CONFIG_S5K3E2FX
	&msm_camera_sensor_s5k3e2fx,
#endif
#ifdef CONFIG_SENSOR_M5MO
	&msm_camera_sensor_m5mo,
#endif
#ifdef CONFIG_SENSOR_S5K5AAFA
	&msm_camera_sensor_s5k5aafa,		
#endif
#ifdef CONFIG_S5K5CCGX
	&msm_camera_sensor_s5k5ccgx,		
#endif
#ifdef CONFIG_SR030PC30
	&VTcamera_i2c_gpio_device_02,
	&msm_camera_sensor_sr030pc30,		
#endif
	&msm_device_vidc_720p,
#ifdef CONFIG_MSM_GEMINI
	&msm_gemini_device,
#endif
#ifdef CONFIG_MSM_VPE
	&msm_vpe_device,
#endif
#ifdef CONFIG_TOUCHSCREEN_QT602240
	&touchscreen_device_qt602240,
#endif
#ifdef CONFIG_TOUCHSCREEN_MELFAS
	&touchscreen_device_melfas,
#endif
#ifdef CONFIG_MSM_SDIO_AL
	&msm_device_sdio_al,
#endif
	&samsung_batt_device,
	&msm_adc_device,
	&msm_vibrator_device,

	&opt_gp2a,

	&acc_gyro_mag_i2c_gpio_device,
	&fuelgauge_i2c_gpio_device,
#ifdef CONFIG_WIFI_CONTROL_FUNC
	&sec_device_wifi_02,
#endif
};

static struct platform_device *devices_03[] __initdata = {
/* ATHENV */
    /*
     * It is necessary to put here in order to support WoW.
     * Put it before MMC host controller in worst case
     */
	&msm_wlan_ar6000_pm_device,
/* ATHENV */
	&msm_device_smd,
	&msm_device_dmov,
	&msm_device_nand,
#ifdef CONFIG_USB_FUNCTION
	&msm_device_hsusb_peripheral,
	&mass_storage_device,
#endif
#ifdef CONFIG_USB_MSM_OTG_72K
	&msm_device_otg,
#ifdef CONFIG_USB_GADGET
	&msm_device_gadget_peripheral,
#endif
#endif
#ifdef CONFIG_USB_ANDROID
	&usb_mass_storage_device,
	&rndis_device,
#ifdef CONFIG_USB_ANDROID_DIAG
	&usb_diag_device,
#endif
	&android_usb_device,
#endif
#ifdef CONFIG_SPI_QSD
	&qsd_device_spi,
#endif
#ifdef CONFIG_I2C_SSBI
	&msm_device_ssbi6,
	&msm_device_ssbi7,
#endif
	&android_pmem_device,
	&msm_fb_device,
	&msm_migrate_pages_device,
#ifdef CONFIG_MSM_ROTATOR
	&msm_rotator_device,
#endif
#ifdef CONFIG_FB_MSM_LCDC_S6D16A0_WVGA_PANEL
	&lcdc_s6d16a0_panel_device,
#endif
#ifdef CONFIG_FB_MSM_LCDC_LD9040_WVGA_PANEL
	&lcdc_ld9040_panel_device,
#endif
#ifdef CONFIG_FB_MSM_LCDC_S6D05A1_HVGA_PANEL
	&lcdc_s6d05a1_panel_device,
#endif
#ifdef CONFIG_FB_MSM_HDMI_SII9024_PANEL
	&hdmi_sii9024_panel_device,
#endif
	&android_pmem_kernel_ebi1_device,
	&android_pmem_adsp_device,
	&android_pmem_audio_device,
#ifdef PMEM_HDMI
	&android_pmem_hdmi_device,
#endif
//	&msm_device_i2c,
	&msm_device_i2c_2,
	&msm_device_uart_dm1,
	&hs_device,
#ifdef CONFIG_MSM7KV2_AUDIO
	&msm_aictl_device,
	&msm_mi2s_device,
	&msm_lpa_device,
	&msm_aux_pcm_device,
#endif
	&msm_device_adspdec,
//	&qup_device_i2c,
	&touchscreen_i2c_gpio_device,
#ifdef CONFIG_SAMSUNG_JACK
       &sec_device_jack,
#endif
	&camera_i2c_gpio_device,
	&amp_i2c_gpio_device,
	&opt_i2c_gpio_device,
	&micro_usb_i2c_gpio_device,
#ifdef CONFIG_WIBRO_CMC
	&max8893_i2c_gpio_device,
	&wmxeeprom_i2c_gpio_device,
#endif
#if defined(CONFIG_MARIMBA_CORE) && \
   (defined(CONFIG_MSM_BT_POWER) || defined(CONFIG_MSM_BT_POWER_MODULE))
//shiks_test    &msm_bt_power_device,
#endif
	&msm_bt_power_device,//shiks_test
	&msm_bluesleep_device_02, //shiks_test
	&msm_device_kgsl,
#ifdef CONFIG_S5K3E2FX
	&msm_camera_sensor_s5k3e2fx,
#endif
#ifdef CONFIG_SENSOR_M5MO
	&msm_camera_sensor_m5mo,
#endif
#ifdef CONFIG_SENSOR_S5K5AAFA
	&msm_camera_sensor_s5k5aafa,		
#endif
#ifdef CONFIG_S5K5CCGX
	&msm_camera_sensor_s5k5ccgx,		
#endif
#ifdef CONFIG_SR030PC30
	&VTcamera_i2c_gpio_device_03,
	&msm_camera_sensor_sr030pc30,		
#endif
	&msm_device_vidc_720p,
#ifdef CONFIG_MSM_GEMINI
	&msm_gemini_device,
#endif
#ifdef CONFIG_MSM_VPE
	&msm_vpe_device,
#endif
#ifdef CONFIG_TOUCHSCREEN_QT602240
	&touchscreen_device_qt602240,
#endif
#ifdef CONFIG_TOUCHSCREEN_MELFAS
	&touchscreen_device_melfas,
#endif
#ifdef CONFIG_MSM_SDIO_AL
	&msm_device_sdio_al,
#endif
	&samsung_batt_device,
	&msm_adc_device,
	&msm_vibrator_device,

	&opt_gp2a,

	&acc_gyro_mag_i2c_gpio_device,
	&fuelgauge_i2c_gpio_device,
#ifdef CONFIG_WIFI_CONTROL_FUNC
	&sec_device_wifi_02,
#endif
};


static struct platform_device *devices_05[] __initdata = {
/* ATHENV */
    /*
     * It is necessary to put here in order to support WoW.
     * Put it before MMC host controller in worst case
     */
	&msm_wlan_ar6000_pm_device,
/* ATHENV */
	&msm_device_smd,
	&msm_device_dmov,
//not needed	&msm_device_nand,
#ifdef CONFIG_USB_FUNCTION
	&msm_device_hsusb_peripheral,
	&mass_storage_device,
#endif
#ifdef CONFIG_USB_MSM_OTG_72K
	&msm_device_otg,
#ifdef CONFIG_USB_GADGET
	&msm_device_gadget_peripheral,
#endif
#endif
#ifdef CONFIG_USB_ANDROID
	&usb_mass_storage_device,
	&rndis_device,
#ifdef CONFIG_USB_ANDROID_DIAG
	&usb_diag_device,
#endif
	&android_usb_device,
#endif
#ifdef CONFIG_SPI_QSD
	&qsd_device_spi,
#endif
#ifdef CONFIG_I2C_SSBI
	&msm_device_ssbi6,
	&msm_device_ssbi7,
#endif
	&android_pmem_device,
	&msm_fb_device,
	&msm_migrate_pages_device,
#ifdef CONFIG_MSM_ROTATOR
	&msm_rotator_device,
#endif
#ifdef CONFIG_FB_MSM_LCDC_S6D16A0_WVGA_PANEL
	&lcdc_s6d16a0_panel_device,
#endif
#ifdef CONFIG_FB_MSM_LCDC_LD9040_WVGA_PANEL
	&lcdc_ld9040_panel_device,
#endif
#ifdef CONFIG_FB_MSM_LCDC_S6D05A1_HVGA_PANEL
	&lcdc_s6d05a1_panel_device,
#endif
#ifdef CONFIG_FB_MSM_HDMI_SII9024_PANEL
	&hdmi_sii9024_panel_device,
#endif
	&android_pmem_kernel_ebi1_device,
	&android_pmem_adsp_device,
	&android_pmem_audio_device,
#ifdef PMEM_HDMI
	&android_pmem_hdmi_device,
#endif
//	&msm_device_i2c,
	&msm_device_i2c_2,
	&msm_device_uart_dm1,
	&hs_device,
#ifdef CONFIG_MSM7KV2_AUDIO
	&msm_aictl_device,
	&msm_mi2s_device,
	&msm_lpa_device,
	&msm_aux_pcm_device,
#endif
	&msm_device_adspdec,
//	&qup_device_i2c,
	&touchscreen_i2c_gpio_device,
#ifdef CONFIG_SAMSUNG_JACK
       &sec_device_jack,
#endif
	&camera_i2c_gpio_device,
	&amp_i2c_gpio_device,
	&opt_i2c_gpio_device,
	&micro_usb_i2c_gpio_device,
#ifdef CONFIG_WIBRO_CMC
	&max8893_i2c_gpio_device,
	&wmxeeprom_i2c_gpio_device,
#endif
#if defined(CONFIG_MARIMBA_CORE) && \
   (defined(CONFIG_MSM_BT_POWER) || defined(CONFIG_MSM_BT_POWER_MODULE))
//shiks_test    &msm_bt_power_device,
#endif
	&msm_bt_power_device,//shiks_test
	&msm_bluesleep_device_rev05,
	&msm_device_kgsl,
#ifdef CONFIG_S5K3E2FX
	&msm_camera_sensor_s5k3e2fx,
#endif
#ifdef CONFIG_SENSOR_M5MO
	&msm_camera_sensor_m5mo,
#endif
#ifdef CONFIG_SENSOR_S5K5AAFA
	&msm_camera_sensor_s5k5aafa,		
#endif
#ifdef CONFIG_S5K5CCGX
	&msm_camera_sensor_s5k5ccgx,		
#endif
#ifdef CONFIG_SR030PC30
	&VTcamera_i2c_gpio_device_02,
	&msm_camera_sensor_sr030pc30,		
#endif
	&msm_device_vidc_720p,
#ifdef CONFIG_MSM_GEMINI
	&msm_gemini_device,
#endif
#ifdef CONFIG_MSM_VPE
	&msm_vpe_device,
#endif
#ifdef CONFIG_TOUCHSCREEN_QT602240
	&touchscreen_device_qt602240,
#endif
#ifdef CONFIG_TOUCHSCREEN_MELFAS
	&touchscreen_device_melfas_05,
#endif
#ifdef CONFIG_MSM_SDIO_AL
	&msm_device_sdio_al,
#endif
	&samsung_batt_device,
	&msm_adc_device,
	&msm_vibrator_device,

	&opt_gp2a,

	&acc_gyro_mag_i2c_gpio_device,
	&fuelgauge_i2c_gpio_device,
#ifdef CONFIG_WIFI_CONTROL_FUNC
	&sec_device_wifi_rev05,
#endif
};


static struct msm_gpio msm_i2c_gpios_hw[] = {
	{ GPIO_CFG(70, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "i2c_scl" },
	{ GPIO_CFG(71, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "i2c_sda" },
};

static struct msm_gpio msm_i2c_gpios_io[] = {
	{ GPIO_CFG(70, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "i2c_scl" },
	{ GPIO_CFG(71, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "i2c_sda" },
};

static struct msm_gpio qup_i2c_gpios_io[] = {
	{ GPIO_CFG(16, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "qup_scl" },
	{ GPIO_CFG(17, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "qup_sda" },
};
static struct msm_gpio qup_i2c_gpios_hw[] = {
	{ GPIO_CFG(16, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "qup_scl" },
	{ GPIO_CFG(17, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "qup_sda" },
};

static void
msm_i2c_gpio_config(int adap_id, int config_type)
{
	struct msm_gpio *msm_i2c_table;

	/* Each adapter gets 2 lines from the table */
	if (adap_id > 0)
		return;
	if (config_type)
		msm_i2c_table = &msm_i2c_gpios_hw[adap_id*2];
	else
		msm_i2c_table = &msm_i2c_gpios_io[adap_id*2];
	msm_gpios_enable(msm_i2c_table, 2);
}
/*This needs to be enabled only for OEMS*/
#ifndef CONFIG_QUP_EXCLUSIVE_TO_CAMERA
static struct vreg *qup_vreg;
#endif
static void
qup_i2c_gpio_config(int adap_id, int config_type)
{
	int rc = 0;
	struct msm_gpio *qup_i2c_table;
	/* Each adapter gets 2 lines from the table */
	if (adap_id != 4)
		return;
	if (config_type)
		qup_i2c_table = qup_i2c_gpios_hw;
	else
		qup_i2c_table = qup_i2c_gpios_io;
	rc = msm_gpios_enable(qup_i2c_table, 2);
	if (rc < 0)
		printk(KERN_ERR "QUP GPIO enable failed: %d\n", rc);
	/*This needs to be enabled only for OEMS*/
#ifndef CONFIG_QUP_EXCLUSIVE_TO_CAMERA
	if (qup_vreg) {
		int rc = vreg_set_level(qup_vreg, 1800);
		if (rc) {
			pr_err("%s: vreg LVS1 set level failed (%d)\n",
			__func__, rc);
		}
		rc = vreg_enable(qup_vreg);
		if (rc) {
			pr_err("%s: vreg_enable() = %d \n",
			__func__, rc);
		}
	}
#endif
}

static struct msm_i2c_platform_data msm_i2c_pdata = {
	.clk_freq = 100000,
	.pri_clk = 70,
	.pri_dat = 71,
	.rmutex  = 1,
	.rsl_id = "D:I2C02000021",
	.msm_i2c_config_gpio = msm_i2c_gpio_config,
};

static void __init msm_device_i2c_init(void)
{
	if (msm_gpios_request(msm_i2c_gpios_hw, ARRAY_SIZE(msm_i2c_gpios_hw)))
		pr_err("failed to request I2C gpios\n");

	msm_device_i2c.dev.platform_data = &msm_i2c_pdata;
}

static struct msm_i2c_platform_data msm_i2c_2_pdata = {
	.clk_freq = 100000,
	.rmutex  = 1,
	.rsl_id = "D:I2C02000022",
	.msm_i2c_config_gpio = msm_i2c_gpio_config,
};

static void __init msm_device_i2c_2_init(void)
{
	msm_device_i2c_2.dev.platform_data = &msm_i2c_2_pdata;
}

static struct msm_i2c_platform_data qup_i2c_pdata = {
	.clk_freq = 384000,
	.pclk = "camif_pad_pclk",
	.msm_i2c_config_gpio = qup_i2c_gpio_config,
};

static void __init qup_device_i2c_init(void)
{
	if (msm_gpios_request(qup_i2c_gpios_hw, ARRAY_SIZE(qup_i2c_gpios_hw)))
		pr_err("failed to request I2C gpios\n");

	qup_device_i2c.dev.platform_data = &qup_i2c_pdata;
	/*This needs to be enabled only for OEMS*/
#ifndef CONFIG_QUP_EXCLUSIVE_TO_CAMERA
	qup_vreg = vreg_get(NULL, "lvsw1");
	if (IS_ERR(qup_vreg)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			__func__, PTR_ERR(qup_vreg));
	}
#endif
}

static unsigned cam_i2c_gpio[] = {
	GPIO_CFG(30, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),	// CAM_SCL
	GPIO_CFG(17, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),	// CAM_SDA
};

static int __init cam_gpio_init(void)
{
	int pin, rc;

	for (pin = 0; pin < ARRAY_SIZE(cam_i2c_gpio); pin++) {
		rc = gpio_tlmm_config(cam_i2c_gpio[pin],
					GPIO_CFG_ENABLE);
		if (rc) {
			printk(KERN_ERR
				"%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, cam_i2c_gpio[pin], rc);
		}
	}
	return rc;
}

static unsigned sensors_i2c_gpio[] = {
	GPIO_CFG(128, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),	// ALS_SCL
	GPIO_CFG(124, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),	// ALS_SDA
	GPIO_CFG(0, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),//ACC_MAG_SCL
	GPIO_CFG(1, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),//ACC_MAG_SDA
	GPIO_CFG(149, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),//ACC_INT
};
static int __init sensors_gpio_init(void)
{
	int pin, rc;

	for (pin = 0; pin < ARRAY_SIZE(sensors_i2c_gpio); pin++) {
		rc = gpio_tlmm_config(sensors_i2c_gpio[pin],
					GPIO_CFG_ENABLE);
		if (rc) {
			printk(KERN_ERR
				"%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, sensors_i2c_gpio[pin], rc);
		}
	}
	return rc;
}
#ifdef CONFIG_I2C_SSBI
static struct msm_ssbi_platform_data msm_i2c_ssbi6_pdata = {
	.rsl_id = "D:PMIC_SSBI",
	.controller_type = MSM_SBI_CTRL_SSBI2,
};

static struct msm_ssbi_platform_data msm_i2c_ssbi7_pdata = {
	.rsl_id = "D:CODEC_SSBI",
	.controller_type = MSM_SBI_CTRL_SSBI,
};
#endif

static struct msm_acpu_clock_platform_data msm7x30_clock_data = {
	.acpu_switch_time_us = 50,
	.vdd_switch_time_us = 62,
};

static void __init msm7x30_init_irq(void)
{
	msm_init_irq();
}

struct vreg *vreg_s3;
struct vreg *vreg_l5;

#if (defined(CONFIG_MMC_MSM_SDC1_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC2_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC3_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC4_SUPPORT))

struct sdcc_gpio {
	struct msm_gpio *cfg_data;
	uint32_t size;
	struct msm_gpio *sleep_cfg_data;
};

static struct msm_gpio sdc1_cfg_data[] = {
	{GPIO_CFG(38, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "sdc1_clk"},
	{GPIO_CFG(39, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc1_cmd"},
	{GPIO_CFG(40, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc1_dat_3"},
	{GPIO_CFG(41, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc1_dat_2"},
	{GPIO_CFG(42, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc1_dat_1"},
	{GPIO_CFG(43, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc1_dat_0"},
};

static struct msm_gpio sdc2_cfg_data[] = { /* INAND */
	{GPIO_CFG(64, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "sdc2_clk"},
	{GPIO_CFG(65, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_cmd"},
	{GPIO_CFG(66, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_3"},
	{GPIO_CFG(67, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_2"},
	{GPIO_CFG(68, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_1"},
	{GPIO_CFG(69, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_0"},

#ifdef CONFIG_MMC_MSM_SDC2_8_BIT_SUPPORT
	{GPIO_CFG(115, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_4"},
	{GPIO_CFG(114, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_5"},
	{GPIO_CFG(113, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_6"},
	{GPIO_CFG(112, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc2_dat_7"},
#endif
};

static struct msm_gpio sdc3_cfg_data[] = {
	{GPIO_CFG(110, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "sdc3_clk"},
	{GPIO_CFG(111, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc3_cmd"},
	{GPIO_CFG(116, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc3_dat_3"},
	{GPIO_CFG(117, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc3_dat_2"},
	{GPIO_CFG(118, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc3_dat_1"},
	{GPIO_CFG(119, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc3_dat_0"},
};

static struct msm_gpio sdc3_sleep_cfg_data[] = {
	{GPIO_CFG(110, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 	"sdc3_clk"},
	{GPIO_CFG(111, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 	"sdc3_cmd"},
	{GPIO_CFG(116, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 	"sdc3_dat_3"},
	{GPIO_CFG(117, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 	"sdc3_dat_2"},
	{GPIO_CFG(118, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 	"sdc3_dat_1"},
	{GPIO_CFG(119, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 	"sdc3_dat_0"},
};

/* internal pull-up */
static struct msm_gpio sdc4_cfg_data[] = {
	{GPIO_CFG(58, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "sdc4_clk"},
	{GPIO_CFG(59, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), "sdc4_cmd"},
	{GPIO_CFG(60, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), "sdc4_dat_3"},
	{GPIO_CFG(61, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), "sdc4_dat_2"},
	{GPIO_CFG(62, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), "sdc4_dat_1"},
	{GPIO_CFG(63, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), "sdc4_dat_0"},
};

//vital2.sleep
static struct msm_gpio sdc4_sleep_cfg_data[] = {
	{GPIO_CFG(58, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "sdc4_clk"},
	{GPIO_CFG(59, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "sdc4_cmd"},
	{GPIO_CFG(60, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "sdc4_dat_3"},
	{GPIO_CFG(61, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "sdc4_dat_2"},
	{GPIO_CFG(62, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "sdc4_dat_1"},
	{GPIO_CFG(63, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "sdc4_dat_0"},
};

static struct sdcc_gpio sdcc_cfg_data[] = {
	{
		.cfg_data = sdc1_cfg_data,
		.size = ARRAY_SIZE(sdc1_cfg_data),
		.sleep_cfg_data = NULL,
	},
	{
		.cfg_data = sdc2_cfg_data,
		.size = ARRAY_SIZE(sdc2_cfg_data),
		.sleep_cfg_data = NULL,
	},
	{
		.cfg_data = sdc3_cfg_data,
		.size = ARRAY_SIZE(sdc3_cfg_data),
		.sleep_cfg_data = NULL,
	},
	{
		.cfg_data = sdc4_cfg_data,
		.size = ARRAY_SIZE(sdc4_cfg_data),
		.sleep_cfg_data = sdc4_sleep_cfg_data, //vital2.sleep
	},
};

struct sdcc_vreg {
	struct vreg *vreg_data;
	unsigned level;
};

static struct sdcc_vreg sdcc_vreg_data[4];

static unsigned long vreg_sts, gpio_sts;

static uint32_t msm_sdcc_setup_gpio(int dev_id, unsigned int enable)
{
	int rc = 0;
	struct sdcc_gpio *curr;

	//printk(KERN_ERR "%s: dev_id %d, enable %d\n", __func__, dev_id, enable);
	curr = &sdcc_cfg_data[dev_id - 1];

	if (!(test_bit(dev_id, &gpio_sts)^enable))
		return rc;

	if (enable) {
		set_bit(dev_id, &gpio_sts);
		rc = msm_gpios_request_enable(curr->cfg_data, curr->size);
		if (rc)
			printk(KERN_ERR "%s: Failed to turn on GPIOs for slot %d\n",
				__func__,  dev_id);
	} else {
		clear_bit(dev_id, &gpio_sts);
		if (curr->sleep_cfg_data) {
			msm_gpios_enable(curr->sleep_cfg_data, curr->size);
			msm_gpios_free(curr->sleep_cfg_data, curr->size);
		} else {
			if(dev_id ==2 || dev_id == 3) //vital2.sleep wifi wants to control their own gpios so skip it here...
				msm_gpios_free(curr->cfg_data, curr->size);
			else
				msm_gpios_disable_free(curr->cfg_data, curr->size);
		}
	}

	return rc;
}

static uint32_t msm_sdcc_setup_vreg(int dev_id, unsigned int enable)
{
	int rc = 0;
	struct sdcc_vreg *curr;
	static int enabled_once[] = {0, 0, 0, 0};

	curr = &sdcc_vreg_data[dev_id - 1];

	if (!(test_bit(dev_id, &vreg_sts)^enable))
		return rc;

	if (!enable || enabled_once[dev_id - 1])
		return 0;

	if (enable) {
		set_bit(dev_id, &vreg_sts);
		rc = vreg_set_level(curr->vreg_data, curr->level);
		if (rc) {
			printk(KERN_ERR "%s: vreg_set_level() = %d \n",
					__func__, rc);
		}
		rc = vreg_enable(curr->vreg_data);
		if (rc) {
			printk(KERN_ERR "%s: vreg_enable() = %d \n",
					__func__, rc);
		}
		enabled_once[dev_id - 1] = 1;
	} else {
		clear_bit(dev_id, &vreg_sts);
		rc = vreg_disable(curr->vreg_data);
		if (rc) {
			printk(KERN_ERR "%s: vreg_disable() = %d \n",
					__func__, rc);
		}
	}
	return rc;
}

static uint32_t msm_sdcc_setup_power(struct device *dv, unsigned int vdd)
{
	int rc = 0;
	struct platform_device *pdev;

	pdev = container_of(dv, struct platform_device, dev);
	rc = msm_sdcc_setup_gpio(pdev->id, (vdd ? 1 : 0));
	if (rc)
		goto out;

	if (pdev->id == 4) /* S3 is always ON and cannot be disabled */
		rc = msm_sdcc_setup_vreg(pdev->id, (vdd ? 1 : 0));
out:
	return rc;
}
#endif


#ifdef CONFIG_WIBRO_CMC
void (*wimax_status_notify_cb)(int card_present, void *dev_id);
EXPORT_SYMBOL(wimax_status_notify_cb);

void *wimax_devid;
EXPORT_SYMBOL(wimax_devid);


static int register_wimax_status_notify(void (*callback)(int card_present, void *dev_id), void *dev_id)
{
	wimax_status_notify_cb = callback;
	wimax_devid = dev_id;
	return 0;
}

static unsigned int wimax_status(struct device *dev)
{
	return (gpio_get_value(131));	// Wimax Reset N (131)
}
#endif



#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
static struct mmc_platform_data msm7x30_sdc1_data = {
	.ocr_mask	= MMC_VDD_165_195,
	.translate_vdd	= msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#ifdef CONFIG_WIBRO_CMC 
   	.status = wimax_status,
   	.register_status_notify = register_wimax_status_notify,
#endif
#if 1
	.dummy52_required = 1,
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
	.nonremovable	= 0,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
static struct mmc_platform_data msm7x30_sdc2_data = {
	.ocr_mask	= MMC_VDD_165_195 | MMC_VDD_27_28,
	.translate_vdd	= msm_sdcc_setup_power,
#ifdef CONFIG_MMC_MSM_SDC2_8_BIT_SUPPORT
	.mmc_bus_width  = MMC_CAP_8_BIT_DATA,
#else
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#endif
#ifdef CONFIG_MMC_MSM_SDC2_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
	.nonremovable	= 1,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
static struct mmc_platform_data msm7x30_sdc3_data = {
	.ocr_mask	= MMC_VDD_27_28 | MMC_VDD_28_29,
	.translate_vdd	= msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#ifdef CONFIG_MMC_MSM_SDIO_SUPPORT
//Wi-Fi    .sdiowakeup_irq = MSM_GPIO_TO_INT(118),
#endif
/* ATHEROS */
#ifndef ATH_POLLING
	.status = wlan_status,
	.register_status_notify	= register_wlan_status_notify,
#endif /* ATH_POLLING */
#if 1 // #ifdef CONFIG_MMC_MSM_SDC3_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
/* ATHEROS */
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
    .nonremovable    = 0,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
#if defined (CONFIG_MMC_MSM_CARD_HW_DETECTION) && !defined(CONFIG_MOVINAND_ON_SDCC4)
static unsigned int msm7x30_sdcc_slot_status(struct device *dev)
{
	return (unsigned int)
		(gpio_get_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SD_DET)));
}
#endif

static struct mmc_platform_data msm7x30_sdc4_data = {
	.ocr_mask	= MMC_VDD_27_28 | MMC_VDD_28_29,
	.translate_vdd	= msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#if defined (CONFIG_MMC_MSM_CARD_HW_DETECTION) && !defined(CONFIG_MOVINAND_ON_SDCC4)
	.status      = msm7x30_sdcc_slot_status,
	.status_irq  = PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, (PMIC_GPIO_SD_DET)),
	.irq_flags   = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
#endif
#ifdef CONFIG_MMC_MSM_SDC4_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
#ifdef CONFIG_MOVINAND_ON_SDCC4
	.nonremovable	= 1,
#else	
	.nonremovable	= 0,
#endif	
};
#endif

static int sdcc_vreg_en(int on) 
{
	int rc;

	vreg_l5 = vreg_get(NULL, "mmc"); //VREG_MSM_SD(L5)
	if (IS_ERR(vreg_l5)) {
		printk(KERN_ERR "%s: vreg_l5 get failed (%ld)\n", __func__, PTR_ERR(vreg_l5));
		return;
	}
	pr_info("%s: set vreg_l5 %s\n", __func__, (on==1)?"ON":"OFF");

	if(on == 0) {
		rc = vreg_disable(vreg_l5);
		if (rc) {
			printk(KERN_ERR "%s: vreg_l5 enable failed (%d)\n", __func__, rc);
			return;
		}
	}
	else {
		rc = vreg_set_level(vreg_l5, 2850);
		if (rc) {
			printk(KERN_ERR "%s: vreg_l5 set level failed (%d)\n", __func__, rc);
			return;
		}

		rc = vreg_enable(vreg_l5);
		if (rc) {
			printk(KERN_ERR "%s: vreg_l5 enable failed (%d)\n", __func__, rc);
			return;
		}
	}		
}

static void __init msm7x30_init_mmc(void)
{
	int rc;

	vreg_s3 = vreg_get(NULL, "s3");		// VREG_1.8V
	if (IS_ERR(vreg_s3)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n", __func__, PTR_ERR(vreg_s3));
		return;
	}
	wlan_init ();
#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
	sdcc_vreg_data[0].vreg_data = vreg_s3;
	sdcc_vreg_data[0].level = 1800;
	msm_add_sdcc(1, &msm7x30_sdc1_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	//if (machine_is_msm8x55_svlte_surf())
	//msm7x30_sdc2_data.msmsdcc_fmax =  24576000;
#if 1
	sdcc_vreg_data[1].vreg_data = vreg_s3;
#else
#if (CONFIG_BOARD_REVISION >= 0x01)
	sdcc_vreg_data[1].vreg_data = vreg_s3;
#else
	sdcc_vreg_data[1].vreg_data = vreg_movi;
#endif
#endif
	sdcc_vreg_data[1].level = 1800;
	msm_add_sdcc(2, &msm7x30_sdc2_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
	sdcc_vreg_data[2].vreg_data = vreg_s3;
	sdcc_vreg_data[2].level = 1800;
	//msm_sdcc_setup_gpio(3, 1);
	msm_add_sdcc(3, &msm7x30_sdc3_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
#if 1 //vital.sleep
	sdcc_vreg_en(1);

	sdcc_vreg_data[3].vreg_data = vreg_s3;
	sdcc_vreg_data[3].level = 1800;
#else
#if (CONFIG_BOARD_REVISION >= 0x01)
	sdcc_vreg_data[3].vreg_data = vreg_movi29;
#else
	sdcc_vreg_data[3].vreg_data = vreg_mmc;
#endif
	sdcc_vreg_data[3].level = 2900;
#endif

	if (!charging_boot) {
		msm_add_sdcc(4, &msm7x30_sdc4_data);
	}
#endif

}

#ifdef WLAN_HOST_WAKE

struct wlansleep_info {
	unsigned host_wake;
	unsigned host_wake_irq;
	struct wake_lock wake_lock;
};

static struct wlansleep_info *wsi, ws_info;
static struct tasklet_struct hostwake_task;

static void wlan_hostwake_task(unsigned long data)
{
	printk(KERN_INFO "WLAN: wake lock timeout 0.5 sec...\n");

	wake_lock_timeout(&wsi->wake_lock, HZ / 2);
}

static irqreturn_t wlan_hostwake_isr(int irq, void *dev_id)
{
//	gpio_clear_detect_status(wsi->host_wake_irq);

	/* schedule a tasklet to handle the change in the host wake line */
	tasklet_schedule(&hostwake_task);
	return IRQ_HANDLED;
}

static int wlan_host_wake_init(void)
{
	int ret;

	wsi = &ws_info;

	wake_lock_init(&wsi->wake_lock, WAKE_LOCK_SUSPEND, "bluesleep");
	tasklet_init(&hostwake_task, wlan_hostwake_task, 0);

	wsi->host_wake = WLAN_GPIO_HOST_WAKE;
	wsi->host_wake_irq = MSM_GPIO_TO_INT(wsi->host_wake);

	ret = request_irq(wsi->host_wake_irq, wlan_hostwake_isr,
				IRQF_DISABLED | IRQF_TRIGGER_RISING,
				"wlan hostwake", NULL);
	if (ret	< 0) {
		printk(KERN_ERR "WLAN: Couldn't acquire WLAN_HOST_WAKE IRQ");
		return -1;
	}

	ret = enable_irq_wake(wsi->host_wake_irq);
	if (ret < 0) {
		printk(KERN_ERR "WLAN: Couldn't enable WLAN_HOST_WAKE as wakeup interrupt");
		free_irq(wsi->host_wake_irq, NULL);
		return -1;
	}

	return 0;
}

static void wlan_host_wake_exit(void)
{
	if (disable_irq_wake(wsi->host_wake_irq))
		printk(KERN_ERR "WLAN: Couldn't disable hostwake IRQ wakeup mode \n");
	free_irq(wsi->host_wake_irq, NULL);

	wake_lock_destroy(&wsi->wake_lock);
}
#endif /* WLAN_HOST_WAKE */

void wlan_setup_power(int on, int detect)
{
	int rc;
	uint32_t host_wake_cfg;

	printk("%s(on = %d, dectect = %d)\n", __func__, on, detect);

	host_wake_cfg = GPIO_CFG(WLAN_GPIO_HOST_WAKE, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA);
	rc = gpio_tlmm_config(host_wake_cfg, GPIO_CFG_ENABLE);
	if (rc) {
		printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, host_wake_cfg, rc);
	}

	if (on) {
		gpio_set_value_cansleep(WLAN_GPIO_RESET, 0);	/* WLAN_RESET */
		msleep(10);
		gpio_set_value_cansleep(WLAN_GPIO_RESET, 1);	/* WLAN_RESET */
#ifdef WLAN_HOST_WAKE
		wlan_host_wake_init();
#endif /* WLAN_HOST_WAKE */
	}
	else {
#ifdef WLAN_HOST_WAKE
		wlan_host_wake_exit();
#endif /* WLAN_HOST_WAKE */
		gpio_set_value_cansleep(WLAN_GPIO_RESET, 0);	/* WLAN_RESET */
	}
	msleep(100);
#ifndef ATH_POLLING
	/* Detect card */
	if (detect) {
		if (wlan_status_notify_cb)
			wlan_status_notify_cb(on, wlan_devid);
		else
			printk(KERN_ERR "WLAN: No notify available\n");
	}
#endif /* ATH_POLLING */
}
EXPORT_SYMBOL(wlan_setup_power);


#ifdef CONFIG_SERIAL_MSM_CONSOLE
static struct msm_gpio uart3_config_data[] = {
	{ GPIO_CFG(53, 1, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "UART3_Rx"},
	{ GPIO_CFG(54, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "UART3_Tx"},
};

static void msm7x30_init_uart3(void)
{
	msm_gpios_request_enable(uart3_config_data,
			ARRAY_SIZE(uart3_config_data));

}
#endif

static struct msm_spm_platform_data msm_spm_data __initdata = {
	.reg_base_addr = MSM_SAW_BASE,

	.reg_init_values[MSM_SPM_REG_SAW_CFG] = 0x05,
	.reg_init_values[MSM_SPM_REG_SAW_SPM_CTL] = 0x18,
	.reg_init_values[MSM_SPM_REG_SAW_SPM_SLP_TMR_DLY] = 0x00006666,
	.reg_init_values[MSM_SPM_REG_SAW_SPM_WAKE_TMR_DLY] = 0xFF000666,

	.reg_init_values[MSM_SPM_REG_SAW_SLP_CLK_EN] = 0x01,
	.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_PRECLMP_EN] = 0x03,
	.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_POSTCLMP_EN] = 0x00,

	.reg_init_values[MSM_SPM_REG_SAW_SLP_CLMP_EN] = 0x01,
	.reg_init_values[MSM_SPM_REG_SAW_SLP_RST_EN] = 0x00,
	.reg_init_values[MSM_SPM_REG_SAW_SPM_MPM_CFG] = 0x00,

	.awake_vlevel = 0xF2,
	.retention_vlevel = 0xE0,
	.collapse_vlevel = 0x72,
	.retention_mid_vlevel = 0xE0,
	.collapse_mid_vlevel = 0xE0,

	.vctl_timeout_us = 50,
};

static int __init boot_mode_boot(char *onoff)
{
	if (strcmp(onoff, "batt") == 0) {
		charging_boot = 1;
	} 
	else if(strcmp(onoff, "fota") == 0) {
		fota_boot = 1;
	}
	else {
		charging_boot = 0;
		fota_boot =0;
	}
	return 1;
}
__setup("androidboot.boot_pause=", boot_mode_boot);


unsigned int customer_binary=0;
static int __init customer_download(char *rooting)
{
	if(!strcmp(rooting,"custom_bin")){
		customer_binary = 1;
	}
	return 1;
}
__setup("androidboot.custom_kernel=", customer_download);


void uart_set_exclusive(void)
{
	unsigned int service;
	unsigned int device;
	int ret;
	
	/* get service for uart device */
	service = (unsigned int)(-1);
	device = 0xf000;
	if((ret = msm_proc_comm(PCOM_OEM_SANSUNG_RDM_GET_SERVICE_CMD, &service, &device)) < 0){
		pr_err("%s rdm_get_service failed : %d \n", __func__, ret);
		return;
	}
	
	pr_info("%s current service on uart : %x\n", __func__, service);
	
	/* close the current service */
	device = 0;
	if((ret = msm_proc_comm(PCOM_OEM_SANSUNG_RDM_ASSIGN_CMD, &service, &device)) < 0) {
		pr_err("%s rdm_assign_port failed : %d \n", __func__, ret);
		return;
	}
	
	pr_info("%s Done...\n", __func__);
	
}


#include <mach/parameters.h>

int emmc_boot_param_read(struct BOOT_PARAM *param_data)
{
	pr_info("%s this function is depricated\n", __func__);
	return param_read(PARAM_TYPE_BOOT, param_data, sizeof(struct BOOT_PARAM));
}

int emmc_boot_param_write(struct BOOT_PARAM *param_data)
{
	pr_info("%s this function is depricated\n", __func__);
	return param_write(PARAM_TYPE_BOOT, param_data, sizeof(struct BOOT_PARAM));
}

int emmc_kernel_param_read(struct samsung_parameter *param_data)
{
	pr_info("%s this function is depricated\n", __func__);
	return param_read(PARAM_TYPE_KERNEL, param_data, sizeof(struct samsung_parameter));
}

int emmc_kernel_param_write(struct samsung_parameter *param_data)
{
	pr_info("%s this function is depricated\n", __func__);
	return param_write(PARAM_TYPE_KERNEL, param_data, sizeof(struct samsung_parameter));
}


static struct proc_dir_entry *debug_proc_entry;
static int param_cmdline_proc_read(char *page,
				char **start,
				off_t offset, int count, int *eof, void *data)
{
	char *buf = page;
	struct BOOT_PARAM param;
	
	emmc_boot_param_read(&param);
	
	buf += sprintf(buf,"%s\n", param.cmdline);
	
	return buf - page < count ? buf - page : count;
}

static int param_cmdline_proc_write(struct file *file, const char *buf,
					 unsigned long count, void *data)
{
	struct BOOT_PARAM param;
	
	emmc_boot_param_read(&param);
	strncpy(param.cmdline, buf, sizeof(param.cmdline));
	emmc_boot_param_write(&param);
	
	return count;
}

static int init_param_proc_entry(void)
{
	debug_proc_entry = create_proc_entry("param_cmdline",
					      S_IRUSR | S_IWUSR, NULL);
	if (!debug_proc_entry)
		printk(KERN_ERR "%s: failed creating procfile\n",
		       __func__);
	else {
		debug_proc_entry->read_proc = param_cmdline_proc_read;
		debug_proc_entry->write_proc = param_cmdline_proc_write;
		debug_proc_entry->data = (void *) NULL;
	}
}

#ifdef RORY_CONTROL
extern struct class *sec_class;
struct device *rory_dev;

static ssize_t rory_control_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct BOOT_PARAM param;
	
	param_read(PARAM_TYPE_BOOT, &param, sizeof(param));
	printk(KERN_ERR "%s: param.multi_download_slot_num: %d\n", __func__, param.multi_download_slot_num);

	return sprintf(buf, "%d\n", param.multi_download_slot_num);
}

static ssize_t rory_control_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct BOOT_PARAM param;
	int slot_num;
	int ret;
	
	sscanf(buf, "%i", &slot_num);
	printk(KERN_ERR "%s: store multi_download_slot_num: %d\n", __func__, slot_num);

	ret = param_read(PARAM_TYPE_BOOT, &param, sizeof(param));
	if(ret) {
		pr_err("%s: param read failed %d\n", __func__, ret);
	} else {
		param.multi_download_slot_num = slot_num;
		param_write(PARAM_TYPE_BOOT, &param, sizeof(param));
	}

	return size;
}
static DEVICE_ATTR(rory_control, S_IRUGO | S_IWUGO, rory_control_show, rory_control_store);

/* rory : samsung multi download solution */
/* for sysfs control (/sys/class/sec/rory/rory_control) */
static void rory_init(void)
{
	rory_dev = device_create(sec_class, NULL, 0, NULL, "rory");

	if (IS_ERR(rory_dev)) {
		pr_err("Failed to create device(rory)!\n");
	}
	if (device_create_file(rory_dev, &dev_attr_rory_control) < 0) {
		pr_err("Failed to create device file(%s)!\n", dev_attr_rory_control.attr.name);
	}
	printk(KERN_ERR "%s: initalized for rory!\n", __func__);
}
#endif /*RORY_CONTROL*/


#ifndef PRODUCT_SHIP
#define MEMDBG_MEM_SIZE 256
static struct proc_dir_entry *memdbg_proc_entry;
static char memdbg_debug_cs0[MEMDBG_MEM_SIZE];
static char *memdbg_debug_cs1;
static int memdbg_proc_read(char *page,
				char **start,
				off_t offset, int count, int *eof, void *data)
{
	char *buf = page;
	int i;

	// print cs0 mem data
	buf += sprintf(buf,"CS0 memory @ va:0x%08x pa:0x%08x\n", memdbg_debug_cs0, __pa(memdbg_debug_cs0));
	for(i=0;i<MEMDBG_MEM_SIZE;i++) {
		if(i%0x10==0 && i!=0)
			buf += sprintf(buf,"\n");
		buf += sprintf(buf,"%02x ", memdbg_debug_cs0[i]);
	}
	buf += sprintf(buf,"\n\n");
	
	// print cs1 mem data
	buf += sprintf(buf,"CS1 memory @ va:0x%08x pa:0x%08x\n", memdbg_debug_cs1, __pa(memdbg_debug_cs1));
	for(i=0;i<MEMDBG_MEM_SIZE;i++) {
		if(i%0x10==0 && i!=0)
			buf += sprintf(buf,"\n");
		buf += sprintf(buf,"%02x ", memdbg_debug_cs1[i]);
	}
	buf += sprintf(buf,"\n\n");
	
	return buf - page < count ? buf - page : count;
}

static int memdbg_proc_entry_init(void)
{
	
	memdbg_proc_entry = create_proc_entry("memdbg",
					      S_IRUSR | S_IWUSR, NULL);
	if (!memdbg_proc_entry)
		printk(KERN_ERR "%s: failed creating procfile\n",
		       __func__);
	else {
		memdbg_proc_entry->read_proc = memdbg_proc_read;
		memdbg_proc_entry->write_proc = NULL;
		memdbg_proc_entry->data = (void *) NULL;
	}
}

void memdbg_debug_init(void)
{
	//setup cs0 debug memory data
	memset(memdbg_debug_cs0, 0xff, MEMDBG_MEM_SIZE);
	strcpy(memdbg_debug_cs0, "CS0 MEMORY");

	//setup cs1 debug memory data
	memdbg_debug_cs1 = kmalloc(MEMDBG_MEM_SIZE, GFP_KERNEL);
	if(memdbg_debug_cs1) {
		memset(memdbg_debug_cs1, 0xff, MEMDBG_MEM_SIZE);
		strcpy(memdbg_debug_cs1, "CS1 MEMORY");
	}

	pr_info("%s: cs0 debug mem @ 0x%08x\n", memdbg_debug_cs0);
	pr_info("%s: cs1 debug mem @ 0x%08x\n", memdbg_debug_cs1);

	memdbg_proc_entry_init();
}
#endif

extern int no_console;
static void __init msm7x30_init(void)
{
	uint32_t soc_version = 0;

	if (socinfo_init() < 0)
		printk(KERN_ERR "%s: socinfo_init() failed!\n",
		       __func__);

	soc_version = socinfo_get_version();
	printk("[Vital2] msm7x30_init(), h/w revision : %d.\n", system_rev);

	if ( !no_console)
		msm_clock_init(msm_clocks_7x30, msm_num_clocks_7x30);
	else
		msm_clock_init(msm_clocks_no_uart3_7x30, msm_num_clocks_no_uart3_7x30);
	
#ifdef CONFIG_SERIAL_MSM_CONSOLE
	if ( !no_console)
		msm7x30_init_uart3();
#endif
	msm_spm_init(&msm_spm_data, 1);
	msm_acpu_clock_init(&msm7x30_clock_data);
#ifdef CONFIG_USB_FUNCTION
	msm_hsusb_pdata.swfi_latency =
		msm_pm_data
		[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
	msm_device_hsusb_peripheral.dev.platform_data = &msm_hsusb_pdata;
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
	if (SOCINFO_VERSION_MAJOR(soc_version) >= 2 &&
			SOCINFO_VERSION_MINOR(soc_version) >= 1) {
		pr_debug("%s: SOC Version:2.(1 or more)\n", __func__);
		msm_otg_pdata.ldo_set_voltage = 0;
	}

	msm_device_otg.dev.platform_data = &msm_otg_pdata;
#ifdef CONFIG_USB_GADGET
	msm_otg_pdata.swfi_latency =
		msm_pm_data
		[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
	msm_device_gadget_peripheral.dev.platform_data = &msm_gadget_pdata;
#endif
#endif
	msm_uart_dm1_pdata.wakeup_irq = gpio_to_irq(136);
	msm_device_uart_dm1.dev.platform_data = &msm_uart_dm1_pdata;

#ifdef CONFIG_SERIAL_MSM_RX_WAKEUP
	if ( !no_console)
		msm_device_uart3.dev.platform_data = &msm_uart3_pdata;
#endif

	msm_adc_pdata.dev_names = msm_adc_surf_device_names;
	msm_adc_pdata.num_adc = ARRAY_SIZE(msm_adc_surf_device_names);
	
	if ( !no_console) {
		platform_add_devices(uart3_device, ARRAY_SIZE(uart3_device));
		uart_set_exclusive();
	}
	
	if(system_rev >= 5)
		platform_add_devices(devices_05, ARRAY_SIZE(devices_05));
	else if(system_rev >= 4)
		platform_add_devices(devices_02, ARRAY_SIZE(devices_02));
	else if(system_rev >= 3)
		platform_add_devices(devices_03, ARRAY_SIZE(devices_03));
	else if(system_rev >= 2)
		platform_add_devices(devices_02, ARRAY_SIZE(devices_02));
	else if(system_rev >= 1)
		platform_add_devices(devices_01, ARRAY_SIZE(devices_01));
	else
		platform_add_devices(devices_03, ARRAY_SIZE(devices_03));

#ifdef CONFIG_USB_EHCI_MSM
	msm_add_host(0, &msm_usb_host_pdata);
#endif

	msm7x30_init_mmc();
#ifdef CONFIG_SPI_QSD
	msm_qsd_spi_init();
#endif
	msm_fb_add_devices();
	msm_pm_set_platform_data(msm_pm_data, ARRAY_SIZE(msm_pm_data));

//	msm_device_i2c_init();
	msm_device_i2c_2_init();
//	qup_device_i2c_init();

	cam_gpio_init();
       sensors_gpio_init();

	buses_init();
	msm7x30_init_marimba();
#ifdef CONFIG_MSM7KV2_AUDIO
	snddev_poweramp_gpio_init();
	aux_pcm_gpio_init();
#endif

#ifdef CONFIG_SENSORS_FSA9480
	fsa9480_gpio_init();
#endif

#ifdef CONFIG_MAX17043_FUEL_GAUGE 
	fg17043_gpio_init();
#endif
	vital2_switch_init();
#if defined (CONFIG_SENSORS_MAX9879) || defined (CONFIG_SENSORS_MAX9877)
	i2c_register_board_info(9, maxamp_boardinfo,
		ARRAY_SIZE(maxamp_boardinfo));
	pr_info("i2c_register_board_info 9 \n");
#endif
#ifdef CONFIG_SENSORS_YDA165
	i2c_register_board_info(9, yamahaamp_boardinfo,
		ARRAY_SIZE(yamahaamp_boardinfo));
	pr_info("yda165:register yamaha amp device \n");
#endif

//	i2c_register_board_info(0, msm_i2c_board_info,
//			ARRAY_SIZE(msm_i2c_board_info));

//	i2c_register_board_info(2, msm_marimba_board_info,
//			ARRAY_SIZE(msm_marimba_board_info));

	i2c_register_board_info(2, msm_i2c_gsbi7_timpani_info,
			ARRAY_SIZE(msm_i2c_gsbi7_timpani_info));


   if(system_rev == 1)
	  i2c_register_board_info(4 /* QUP ID */, msm_camera_boardinfo_01,
		  ARRAY_SIZE(msm_camera_boardinfo_01));
   else if(system_rev >= 2)
     i2c_register_board_info(4 /* QUP ID */, msm_camera_boardinfo_02,
		  ARRAY_SIZE(msm_camera_boardinfo_02));
   else
    i2c_register_board_info(4 /* QUP ID */, msm_camera_boardinfo_01,
		  ARRAY_SIZE(msm_camera_boardinfo_01));


   #ifdef CONFIG_SR030PC30
     if(system_rev >= 2)
       i2c_register_board_info(20,msm_VTcamera_boardinfo_02,
                                                   ARRAY_SIZE(msm_VTcamera_boardinfo_02));		//1.3M front camera new add
   #endif
   
#ifdef CONFIG_TOUCHSCREEN_QT602240
	i2c_register_board_info(5, qt602240_touch_boardinfo,
		ARRAY_SIZE(qt602240_touch_boardinfo));
#endif

#ifdef CONFIG_TOUCHSCREEN_MELFAS
	if(system_rev >= 5)
		i2c_register_board_info(5, melfas_touch_boardinfo_05,
			ARRAY_SIZE(melfas_touch_boardinfo_05));
	else
		i2c_register_board_info(5, melfas_touch_boardinfo,
			ARRAY_SIZE(melfas_touch_boardinfo));
#endif


#if defined(CONFIG_SENSORS_KR3D_ACCEL) || defined(CONFIG_INPUT_YAS_MAGNETOMETER) || defined(CONFIG_SENSORS_YAS529_MAGNETIC)
	i2c_register_board_info(8, acc_gyro_mag_i2c_gpio_devices,
		ARRAY_SIZE(acc_gyro_mag_i2c_gpio_devices));
#endif

	i2c_register_board_info(14, opt_i2c_borad_info,
		ARRAY_SIZE(opt_i2c_borad_info));

#ifdef CONFIG_MAX17043_FUEL_GAUGE
	i2c_register_board_info(11, fuelgauge_i2c_devices,
			ARRAY_SIZE(fuelgauge_i2c_devices));
#endif

#ifdef CONFIG_SENSORS_FSA9480
	i2c_register_board_info(10, micro_usb_i2c_devices, 
			ARRAY_SIZE(micro_usb_i2c_devices));
#endif
#ifdef CONFIG_WIBRO_CMC
	i2c_register_board_info(12, max8893_i2c_devices, ARRAY_SIZE(max8893_i2c_devices));
	i2c_register_board_info(13, wmxeeprom_i2c_devices, ARRAY_SIZE(wmxeeprom_i2c_devices));
#endif
	bt_power_init();
	vibrator_device_gpio_init();
#if defined(CONFIG_INPUT_YAS_MAGNETOMETER) || defined(CONFIG_SENSORS_YAS529_MAGNETIC)
	magnetic_device_init();	// yas529 nRST gpio pin configue
#endif
	optical_device_init();
#ifdef CONFIG_I2C_SSBI
	msm_device_ssbi6.dev.platform_data = &msm_i2c_ssbi6_pdata;
	msm_device_ssbi7.dev.platform_data = &msm_i2c_ssbi7_pdata;
#endif
	aries_init_wifi_mem();	 //use wlan static buffer

#ifdef RORY_CONTROL
	rory_init();
#endif

#ifndef PRODUCT_SHIP
	init_param_proc_entry();
#endif

	printk(KERN_ERR " !@#$ android_usb_pdata.num_functions = %d\n", android_usb_pdata.num_functions);

#ifndef PRODUCT_SHIP
	do {
		extern void debug_panic_init(void);
		memdbg_debug_init();
		debug_panic_init();
	} while(0);
#endif

}

static unsigned pmem_sf_size = MSM_PMEM_SF_SIZE;
static void __init pmem_sf_size_setup(char **p)
{
	pmem_sf_size = memparse(*p, p);
}
__early_param("pmem_sf_size=", pmem_sf_size_setup);

static unsigned fb_size = MSM_FB_SIZE;
static void __init fb_size_setup(char **p)
{
	fb_size = memparse(*p, p);
}
__early_param("fb_size=", fb_size_setup);

static unsigned gpu_phys_size = MSM_GPU_PHYS_SIZE;
static void __init gpu_phys_size_setup(char **p)
{
	gpu_phys_size = memparse(*p, p);
}
__early_param("gpu_phys_size=", gpu_phys_size_setup);

static unsigned pmem_adsp_size = MSM_PMEM_ADSP_SIZE;
static void __init pmem_adsp_size_setup(char **p)
{
	pmem_adsp_size = memparse(*p, p);
}
__early_param("pmem_adsp_size=", pmem_adsp_size_setup);

static unsigned pmem_audio_size = MSM_PMEM_AUDIO_SIZE;
static void __init pmem_audio_size_setup(char **p)
{
	pmem_audio_size = memparse(*p, p);
}
__early_param("pmem_audio_size=", pmem_audio_size_setup);

static unsigned pmem_kernel_ebi1_size = PMEM_KERNEL_EBI1_SIZE;
static void __init pmem_kernel_ebi1_size_setup(char **p)
{
	pmem_kernel_ebi1_size = memparse(*p, p);
}
__early_param("pmem_kernel_ebi1_size=", pmem_kernel_ebi1_size_setup);

static void __init msm7x30_allocate_memory_regions(void)
{
	void *addr;
	unsigned long size;
/*
   Request allocation of Hardware accessible PMEM regions
   at the beginning to make sure they are allocated in EBI-0.
   This will allow 7x30 with two mem banks enter the second
   mem bank into Self-Refresh State during Idle Power Collapse.

    The current HW accessible PMEM regions are
    1. Frame Buffer.
       LCDC HW can access msm_fb_resources during Idle-PC.

    2. Audio
       LPA HW can access android_pmem_audio_pdata during Idle-PC.
*/
	size = fb_size ? : MSM_FB_SIZE;
	addr = alloc_bootmem(size);
	msm_fb_resources[0].start = __pa(addr);
	msm_fb_resources[0].end = msm_fb_resources[0].start + size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for fb\n",
		size, addr, __pa(addr));

	size = pmem_audio_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_audio_pdata.start = __pa(addr);
		android_pmem_audio_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for audio "
			"pmem arena\n", size, addr, __pa(addr));
	}

	size = pmem_kernel_ebi1_size;
	if (size) {
		addr = alloc_bootmem_aligned(size, 0x100000);
		android_pmem_kernel_ebi1_pdata.start = __pa(addr);
		android_pmem_kernel_ebi1_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for kernel"
			" ebi1 pmem arena\n", size, addr, __pa(addr));
	}

	size = pmem_sf_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_pdata.start = __pa(addr);
		android_pmem_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for sf "
			"pmem arena\n", size, addr, __pa(addr));
	}

	size = pmem_adsp_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_adsp_pdata.start = __pa(addr);
		android_pmem_adsp_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for adsp "
			"pmem arena\n", size, addr, __pa(addr));
	}

#ifdef PMEM_HDMI
	size = PMEM_HDMI_SIZE;
	addr = alloc_bootmem(size);
	android_pmem_hdmi_pdata.start = __pa(addr);
	android_pmem_hdmi_pdata.size = size;
	pr_info("allocating %lu bytes at %p (%lx physical) for hdmi "
		"pmem arena\n", size, addr, __pa(addr));
#endif

#ifdef CONFIG_KERNEL_DEBUG_SEC
	do {
		extern void set_crit_kmsg_addr(char*);
		extern unsigned int get_crit_kmsg_size(void);
		addr = alloc_bootmem_nopanic_noinit(get_crit_kmsg_size());
		set_crit_kmsg_addr(addr);
		pr_info("allocating %lu bytes at %p (%lx phys) for critical kmsg\n"
			, get_crit_kmsg_size(), addr, __pa(addr));
	} while(0);
#endif
}

static void __init msm7x30_map_io(void)
{
#if defined (CONFIG_APPSBOOT_3M_CONFIG)
	msm_shared_ram_phys = 0x00300000;
#else
	msm_shared_ram_phys = 0x00100000;
#endif
	msm_map_msm7x30_io();
	msm7x30_allocate_memory_regions();
}

MACHINE_START(VITAL2, "SPH-M930")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io  = MSM_DEBUG_UART_PHYS,
	.io_pg_offst = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params = PHYS_OFFSET + 0x100,
	.map_io = msm7x30_map_io,
	.init_irq = msm7x30_init_irq,
	.init_machine = msm7x30_init,
	.timer = &msm_timer,
MACHINE_END
