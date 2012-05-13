/*
 * Copyright (C) 2008 Samsung Electronics, Inc.
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

#ifndef __WIMAX_CMC732_H
#define __WIMAX_CMC732_H

#ifdef __KERNEL__

#define WIMAX_POWER_SUCCESS		0
#define WIMAX_ALREADY_POWER_ON		-1
#define WIMAX_PROBE_FAIL		-2
#define WIMAX_ALREADY_POWER_OFF		-3

/* wimax mode */
enum {
	SDIO_MODE = 0,
	WTM_MODE,
	MAC_IMEI_WRITE_MODE,
	USIM_RELAY_MODE,
	DM_MODE,
	USB_MODE,
	AUTH_MODE
};


struct wimax_cfg {
	int			temp_tgid;	/* handles unexpected close */
	int			uart_sel;
//	int			uart_sel1;
	struct wake_lock	wimax_wake_lock;	/* resume wake lock */
	struct wake_lock	wimax_rxtx_lock;/* sdio wake lock */
	struct mutex		poweroff_mutex; /*To avoid executing poweroff simultaneously*/
	u8		enable_dump_msg;/* prints control dump msg */
	u8		wimax_mode;/* wimax mode (SDIO, USB, etc..) */
	u8		sleep_mode;/* suspend mode (0: VI, 1: IDLE) */
	u8		card_removed;/*
						 * set if host has acknowledged
						 * card removal
						 */
	u8 		probe_complete /*Flag to sleep poweroff till probe complete*/
};


struct wimax732_platform_data {
	int (*power) (int);
	void (*set_mode) (void);
	void (*signal_ap_active) (int);
	int (*get_sleep_mode) (void);
	int (*is_modem_awake) (void);
	void (*switch_eeprom_wimax) (int);
	void (*switch_uart_ap) (void);
	void (*switch_uart_wimax) (void);
	void (*restore_uart_path) (void);
	void (*wakeup_assert) (int);
	void (*display_gpios) (void);
	void (*eeprom_power) (int);
	void (*eeprom_i2c_set_high) (void);	
	struct wimax_cfg *g_cfg;
	int wimax_int;
	int (*get_wimax_int) (void);
	int (*get_wimax_en) (void);
	int (*is_ap_awake) (void);
};

#define dump_debug(args...)	\
{	\
	printk(KERN_ALERT"\x1b[1;33m[WiMAX] ");	\
	printk(args);	\
	printk("\x1b[0m\n");	\
}

#define WIMAX_EN		122
#define WIMAX_RESET		131
#define WIMAX_USB_EN		123

#define WIMAX_WAKEUP		166
#define WIMAX_IF_MODE0		171
#define WIMAX_IF_MODE1		172
#define WIMAX_CON0		167
#define WIMAX_CON1		168
#define WIMAX_CON2		169
#define WIMAX_INT		113

#define I2C_SEL			120
#define EEPROM_SCL		84
#define EEPROM_SDA		83

#define PMIC_GPIO_UART_SEL		PM8058_GPIO((system_rev>=5)?20:7)
#define USB_SEL			121
#define DBGEN			127

#define WIMAX_SDIO_CLK      38
#define WIMAX_SDIO_CMD      39
#define WIMAX_SDIO_D3       40
#define WIMAX_SDIO_D2       41
#define WIMAX_SDIO_D1       42
#define WIMAX_SDIO_D0       43

#define GPIO_LEVEL_NONE   2
#define GPIO_LEVEL_HIGH   1
#define GPIO_LEVEL_LOW    0

#define POWER_ON	1
#define POWER_OFF	0

#define ALIGNMENT 4

extern void (*wimax_status_notify_cb)(int card_present, void *dev_id);
extern void *wimax_devid;
#endif

#endif
