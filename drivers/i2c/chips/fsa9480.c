/*
 * Copyright (C) 2007-2010 SAMSUNG Electronics Corporation.
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

#include <linux/i2c.h>
#include <linux/slab.h>
#include <mach/gpio.h>
#include <linux/delay.h>
#include <linux/input.h>

#include <mach/vreg.h>
#include <asm/io.h>

#include <linux/uaccess.h>
#include <linux/i2c/fsa9480.h>

#if defined CONFIG_MACH_CHIEF || defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || \
	defined(CONFIG_MACH_PREVAIL2)
#include <linux/switch.h>
#include <mach/parameters.h>
extern struct device *switch_dev;

#include <linux/usb/android_composite.h>

#include "../../../arch/arm/mach-msm/smd_private.h"

extern int android_usb_get_current_mode(void);
extern void android_usb_switch(int mode);
extern void melfas_write_reg(u8 data);
extern struct melfas_ts_driver *melfas_ts;

unsigned char backup_reg[10];
#endif
extern int no_console;
static int fsa9480_reset_debug;

module_param_named(
	fsa9480_reset_debug, fsa9480_reset_debug, int, S_IRUGO | S_IWUSR | S_IWGRP
);

static struct i2c_client *pclient;

struct fsa9480_data {
	struct work_struct work;
       struct fsa_platform_data *pdata;
};

#if defined CONFIG_MACH_CHIEF //20100520_inchul
#define USB_CABLE_50K 1
#define USB_CABLE_255K 2
#define CABLE_DISCONNECT 0
int wimax_usb_state = 0;
int MicroJigUSBOffStatus = 0;
static u8 wimax_255k_check_in_booting = 0;
#endif

#ifdef CONFIG_USBHUB_USB3803
#include <linux/usb3803.h>
extern int usb3803_set_mode(int mode);
#endif

static int curr_usb_status = 0;
int curr_ta_status = 0;
EXPORT_SYMBOL(curr_ta_status);

int curr_dock_flag = 0;
int pre_dock_status = 0;

int curr_uart_status = 0;
EXPORT_SYMBOL(curr_uart_status);

static u8 fsa9480_device1 = 0x0;
static u8 fsa9480_device2 = 0x0;
static u8 Intr1_ovp_en = 0x0;


int fsa9480_i2c_write(unsigned char u_addr, unsigned char u_data);
int fsa9480_i2c_read(unsigned char u_addr, unsigned char *pu_data);

static unsigned int fsa_i2c_reset_mode=0;

#if defined CONFIG_MACH_CHIEF || defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2)  || \ 
	defined(CONFIG_MACH_PREVAIL2) //20101125_inchul
static int g_dock;
static int g_default_ESN_status = 1; //20101129_inchul
static int curr_usb_path = 0;
static int curr_uart_path = 0;

#if 0
static ssize_t print_switch_name(struct switch_dev *sdev, char *buf)
{
        if(g_dock == DESK_DOCK_CONNECTED)
            return sprintf(buf, "%s\n", "Desk Dock");
        else if(g_dock == CAR_DOCK_CONNECTED)
            return sprintf(buf, "%s\n", "Car Dock");
        else
            return sprintf(buf, "%s\n", "No Device");
}

static ssize_t print_switch_state(struct switch_dev *sdev, char *buf)
{
        if((g_dock == DESK_DOCK_CONNECTED) ||( g_dock == CAR_DOCK_CONNECTED))
            return sprintf(buf, "%s\n", "Online");
        else
            return sprintf(buf, "%s\n", "Offline");
}
#endif

struct switch_dev switch_dock_detection = {
		.name = "dock",
		//.print_name = print_switch_name,
		//.print_state = print_switch_state,
};

#define DOCK_KEY_MASK	0x18
#define DOCK_KEY_SHIFT  3

typedef enum
{
	DOCK_KEY_VOLUMEUP = 0,
	DOCK_KEY_VOLUMEDOWN,
	DOCK_KEY_MAX,
} dock_key_type;

static unsigned int dock_keys[DOCK_KEY_MAX] =
{
	KEY_VOLUMEDOWN,
	KEY_VOLUMEUP,
};

static const char * dock_keys_string[DOCK_KEY_MAX] =
{
	"KEY_VOLUMEDOWN",
	"KEY_VOLUMEUP",
};

static struct input_dev * dock_key_input_dev;

static void dock_keys_input(dock_key_type key, int press)
{
	if( key >= DOCK_KEY_MAX )
		return;

	input_report_key(dock_key_input_dev, dock_keys[key], press);
	input_sync(dock_key_input_dev);

	DEBUG_FSA9480("key pressed(%d) [%s] \n", press, dock_keys_string[key]);
}
#endif


void fsa9480_read_interrupt_register(void);
struct delayed_work fsa9480_delayed_wq;
void fsa9480_reset_device(void)
{
        struct fsa_platform_data *pdata;;
        u8 temp_reg;

        if(!gpio_get_value(MSM_INT_TO_GPIO(pclient->irq)))
        {
                //I2C Reset Mode
                pdata = pclient->dev.platform_data;
                gpio_set_value(pdata->i2c_scl, 0);
                gpio_set_value(pdata->i2c_sda, 0);
                msleep(35);

                //re-store register after i2c reset
                fsa9480_i2c_write(REGISTER_CONTROL, backup_reg[0]);
                fsa9480_i2c_write(REGISTER_INTERRUPTMASK1, backup_reg[1]);
                fsa9480_i2c_write(REGISTER_INTERRUPTMASK2, backup_reg[2]);
                fsa9480_i2c_write(REGISTER_TIMINGSET1, backup_reg[3]);
                fsa9480_i2c_write(REGISTER_TIMINGSET2, backup_reg[4]);
                fsa9480_i2c_write(REGISTER_CARKITSTATUS, backup_reg[5]);
                fsa9480_i2c_write(REGISTER_CARKITMASK1, backup_reg[6]);
                fsa9480_i2c_write(REGISTER_CARKITMASK2, backup_reg[7]);
                fsa9480_i2c_write(REGISTER_MANUALSW1, backup_reg[8]);
                fsa9480_i2c_write(REGISTER_MANUALSW2, backup_reg[9]);

                //read interrupt register to reset
                fsa9480_i2c_read(REGISTER_INTERRUPT1, &temp_reg);
                fsa9480_i2c_read(REGISTER_INTERRUPT2, &temp_reg);
                fsa9480_i2c_read(REGISTER_CARKITINT1, &temp_reg);
                fsa9480_i2c_read(REGISTER_CARKITINT2, &temp_reg);

                fsa_i2c_reset_mode++; //to do debug
                if(fsa_i2c_reset_mode == 0xffffffff)
                        fsa_i2c_reset_mode = 0;

                fsa9480_read_interrupt_register();
        }

        printk("%s gpio_%d value = %d	 fsa_int_low : %d\n",__func__,MSM_INT_TO_GPIO(pclient->irq),\
                gpio_get_value(MSM_INT_TO_GPIO(pclient->irq)),fsa_i2c_reset_mode);
}

static void fsa9480_delayed_work_function(struct work_struct *work)
{
	fsa9480_reset_device();
}

static DECLARE_WAIT_QUEUE_HEAD(g_data_ready_wait_queue);
extern int batt_restart(void);

#if defined CONFIG_MACH_CHIEF
u8 FSA9480_Get_USB_Status(void)
{
	if(wimax_usb_state)/*20100520_inchul*/
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(FSA9480_Get_USB_Status);


int get_wimax_usb_cable_state(void)
{
	return wimax_usb_state;
}

static ssize_t wimax_usb_state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int cable_state;

	cable_state = get_wimax_usb_cable_state();

	//sprintf(buf, "%s\n", (cable_state & (CRB_JIG_USB<<8 | CRA_USB<<0 ))?"CONNECTED":"DISCONNECTED");
	//sprintf(buf, "%s\n", cable_state ?"CONNECTED":"DISCONNECTED");
	if(cable_state == USB_CABLE_50K){
	sprintf(buf, "%s\n", "authcable");
	}
 	else if(cable_state == USB_CABLE_255K){
	sprintf(buf, "%s\n", "wtmcable");
 	}
 	else{
	sprintf(buf, "%s\n", "Disconnected");
 	}

	return sprintf(buf, "%s\n", buf);
}

static ssize_t wimax_usb_state_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
    DEBUG_FSA9480("[FSA9480]%s\n ", __func__);

	return 0;
}

/*sysfs for wimax usb cable's state.*/
static DEVICE_ATTR(wimax_usb_state, S_IRUGO | S_IWUSR, wimax_usb_state_show, wimax_usb_state_store);
#endif
int fsa9480_i2c_tx_data(char* txData, int length)
{
	int rc;

	struct i2c_msg msg[] = {
		{
			.addr = pclient->addr,
			.flags = 0,
			.len = length,
			.buf = txData,
		},
	};

	rc = i2c_transfer(pclient->adapter, msg, 1);
	if (rc < 0) {
		printk(KERN_ERR "[FSA9480]: fsa9480_i2c_tx_data error %d\n", rc);
		return rc;
	}

	return 0;
}


int fsa9480_i2c_write(unsigned char u_addr, unsigned char u_data)
{
	int rc;
	unsigned char buf[2];

	//back up register data berfore i2C reset
      if(u_addr == REGISTER_CONTROL) backup_reg[0] = u_data;
      else if(u_addr == REGISTER_INTERRUPTMASK1) backup_reg[1] = u_data;
      else if(u_addr == REGISTER_INTERRUPTMASK2) backup_reg[2] = u_data;
      else if(u_addr == REGISTER_TIMINGSET1)  backup_reg[3] = u_data;
      else if(u_addr == REGISTER_TIMINGSET2)  backup_reg[4] = u_data;
      else if(u_addr == REGISTER_CARKITMASK1) backup_reg[6] = u_data;
      else if(u_addr == REGISTER_CARKITMASK2) backup_reg[7] = u_data;
      else if(u_addr == REGISTER_MANUALSW1) backup_reg[8] = u_data;
      else if(u_addr == REGISTER_MANUALSW2) backup_reg[9] = u_data;


	buf[0] = u_addr;
	buf[1] = u_data;

	rc = fsa9480_i2c_tx_data(buf, 2);
	if(rc < 0)
		printk(KERN_ERR "[FSA9480]: txdata error %d add:0x%02x data:0x%02x\n",
			rc, u_addr, u_data);

	return rc;
}

static int fsa9480_i2c_rx_data(char* rxData, int length)
{
	int rc;

	struct i2c_msg msgs[] = {
		{
			.addr = pclient->addr,
			.flags = 0,
			.len = 1,
			.buf = rxData,
		},
		{
			.addr = pclient->addr,
			.flags = I2C_M_RD|I2C_M_NO_RD_ACK,
			.len = length,
			.buf = rxData,
		},
	};

	rc = i2c_transfer(pclient->adapter, msgs, 2);

	if (rc < 0) {
		printk(KERN_ERR "[FSA9480]: fsa9480_i2c_rx_data error %d\n", rc);
		return rc;
	}

	return 0;
}

int fsa9480_i2c_read(unsigned char u_addr, unsigned char *pu_data)
{
	int rc;
	unsigned char buf;

	buf = u_addr;
	rc = fsa9480_i2c_rx_data(&buf, 1);
	if (!rc)
		*pu_data = buf;
	else
		printk(KERN_ERR "[FSA9480]: i2c read failed\n");
	
	return rc;
}

void fsa9480_select_mode(int mode)
{
	unsigned char cont_reg = 0;

	printk(KERN_INFO "[fsa9480_select_mode]: mode = %x\n", mode);
	if (!pclient)
		return;

    /* fsa9480 init sequence */
	fsa9480_i2c_write(REGISTER_CONTROL, mode); // FSA9480 Set AutoMode
	fsa9480_i2c_read(REGISTER_CONTROL, &cont_reg); // FSA9480 initilaization check
	printk("[fsa9480_select_mode]: Changed control reg 0x02 : 0x%x\n", cont_reg);

	/* delay 2 ms */
	msleep(2);
}

EXPORT_SYMBOL(fsa9480_select_mode);

static void fsa9480_chip_init(void)
{
	unsigned char cont_reg = 0;
	u8 intr1 = 0;

	printk(KERN_INFO "[FSA9480]: init\n");
	if (!pclient)
		return;

    /* fsa9480 init sequence */
	fsa9480_i2c_write(REGISTER_CONTROL, 0x1E); // FSA9480 Set AutoMode
	fsa9480_i2c_read(REGISTER_CONTROL, &cont_reg); // FSA9480 initilaization check
	printk("[FSA9480]: Initial control reg 0x02 : 0x%x\n", cont_reg);

	fsa9480_i2c_write(REGISTER_CARKITMASK1, 0xFF);
	fsa9480_i2c_write(REGISTER_CARKITMASK2, 0xFF);

	/* delay 2 ms */
	printk(KERN_INFO "[FSA9480]: fsa9480 sensor init sequence done\n");

	/* Remember attached devices */
	fsa9480_i2c_read(REGISTER_DEVICETYPE1, &fsa9480_device1);
	fsa9480_i2c_read(REGISTER_DEVICETYPE2, &fsa9480_device2);
	fsa9480_i2c_read(REGISTER_INTERRUPT1, &intr1);

	if(intr1 & OVP_EN)	Intr1_ovp_en = 1;

	if ( (fsa9480_device1 & CRA_USB) && !Intr1_ovp_en )
	{
		curr_usb_status = 1;
	}
	else if ( (fsa9480_device1 & CRA_DEDICATED_CHG || fsa9480_device1 & CRA_CARKIT) && !Intr1_ovp_en )
	{
		curr_ta_status = 1;
	}
	else if ( (fsa9480_device2 & CRB_JIG_UART_OFF || fsa9480_device2 & CRB_JIG_UART_ON || fsa9480_device2 & CRB_AV) && !Intr1_ovp_en )
	{
		curr_dock_flag = 1;
	}
}

static int fsa9480_client(struct i2c_client *client)
{
	/* Initialize the fsa9480 Chip */
	init_waitqueue_head(&g_data_ready_wait_queue);
	return 0;
}

int fsa9480_get_jig_status(void)
{
	u8 jig_devices = CRB_JIG_USB_ON | CRB_JIG_USB_OFF | CRB_JIG_UART_ON | CRB_JIG_UART_OFF;

	if (fsa9480_device2 & jig_devices)
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(fsa9480_get_jig_status);

int fsa9480_get_charger_status(void)
{
	if (curr_usb_status)
		return 2;	// (CHARGER_TYPE_USB_PC)
	else if (curr_ta_status)
		return 1;	// (CHARGER_TYPE_WALL)
	else
		return 0;	// no charger (CHARGER_TYPE_NONE)
}
EXPORT_SYMBOL(fsa9480_get_charger_status);

void fsa9480_connect_charger(void)	// vbus connected
{
	u8 dev1, dev2;

	fsa9480_i2c_read(REGISTER_DEVICETYPE1, &dev1);
	msleep(5);

	fsa9480_i2c_read(REGISTER_DEVICETYPE2, &dev2);

	if ( (dev1 == CRA_USB) && !Intr1_ovp_en)
	{
		curr_usb_status = 1;
		pr_info("[FSA9480] %s: USB connected...\n", __func__);
	}
	else if( (dev1 == CRA_DEDICATED_CHG) && !Intr1_ovp_en )	// consider as dedicated charger any device with vbus power
	{
		curr_ta_status = 1;
		pr_info("[FSA9480] %s: TA connected...\n", __func__);
	}

	batt_restart();
}
EXPORT_SYMBOL(fsa9480_connect_charger);

void fsa9480_disconnect_charger(void)	// vbus disconnected
{
	u8 dev1, dev2;

	fsa9480_i2c_read(REGISTER_DEVICETYPE1, &dev1);
	msleep(5);

	fsa9480_i2c_read(REGISTER_DEVICETYPE2, &dev2);

	Intr1_ovp_en = 0;
	curr_usb_status = 0;
	curr_ta_status = 0;

	pr_info("[FSA9480] %s\n", __func__);
	batt_restart();
}
EXPORT_SYMBOL(fsa9480_disconnect_charger);

#if defined CONFIG_MACH_CHIEF || defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || \
	defined(CONFIG_MACH_PREVAIL2) //20101125_inchul
/* USB SWITCH CONTROL */
/* 0: MSM , 1 : MDM , 2 : CYAS */
#define SWITCH_MSM		0
#define SWITCH_MDM		1
#define SWITCH_CYAS	2

#define USB_SW_EN	56
#define USB_SW		73

static void usb_switch_mode(int mode)
{
	if (mode == SWITCH_MSM) {
		curr_usb_path = SWITCH_MSM;
		gpio_set_value(USB_SW_EN, 0);
		gpio_set_value(USB_SW, 1);
		fsa9480_i2c_write(REGISTER_CONTROL, 0x1E);
	}
	else if(mode == SWITCH_MDM)
	{
		curr_usb_path = SWITCH_MDM;
		gpio_set_value(USB_SW_EN, 1);
		gpio_set_value(USB_SW, 1);
//		fsa9480_i2c_write(REGISTER_MANUALSW1, 0x0);
//		fsa9480_i2c_write(REGISTER_CONTROL, 0x1A);
              fsa9480_i2c_write(REGISTER_CONTROL, 0x0E);
	}
	else if(mode == SWITCH_CYAS)
	{
		curr_usb_path = SWITCH_CYAS;
		gpio_set_value(USB_SW_EN, 1);
		gpio_set_value(USB_SW, 0);
		fsa9480_i2c_write(REGISTER_CONTROL, 0x0E);
	}
}

static int get_current_mode(void)
{
	int mode = 0;

	if( gpio_get_value(USB_SW_EN) == 1 ) {
		if ( gpio_get_value(USB_SW) == 1 ) {
			mode = SWITCH_MDM;
		} else {
			mode = SWITCH_CYAS;
		}
	} else {
		if ( gpio_get_value(USB_SW) == 1 ) {
			mode = SWITCH_MSM;
		}
	}

	return mode;
}

/* for sysfs control (/sys/class/sec/switch/usb_sel) */
static ssize_t usb_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int usb_path;

	usb_path = get_current_mode();

	switch(usb_path)
	{
		case SWITCH_MSM:
			sprintf(buf, "USB Switch : PDA");
			break;
		case SWITCH_MDM:
			sprintf(buf, "USB Switch : LTEMODEM");
			break;
		case SWITCH_CYAS:
			sprintf(buf, "USB Switch : CYAS");
			break;
		default:
			break;
	}

	return sprintf(buf, "%s\n", buf);
}

static ssize_t usb_switch_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct samsung_parameter *param_data;

	if ( !(param_data = kzalloc(sizeof(struct samsung_parameter),GFP_KERNEL))) {
		printk("######### can not alloc memory for param_data ! ##################\n");
		}
	else{
		memset(param_data,0,sizeof(struct samsung_parameter));
		}

	msm_read_param(param_data);

	if(strncmp(buf, "PDA", 3) == 0 || strncmp(buf, "pda", 3) == 0) {
		usb_switch_mode(SWITCH_MSM);
		param_data->usb_sel = SWITCH_MSM;
	}

	if(strncmp(buf, "LTEMODEM", 8) == 0 || strncmp(buf, "ltemodem", 8) == 0) {
		usb_switch_mode(SWITCH_MDM);
		param_data->usb_sel = SWITCH_MDM;
	}

	if(strncmp(buf, "CYAS", 4) == 0 || strncmp(buf, "cyas", 4) == 0) {
		usb_switch_mode(SWITCH_CYAS);
	}

	msm_write_param(param_data);
	kfree(param_data);

	return size;
}
static DEVICE_ATTR(usb_sel, S_IRUGO | S_IWUSR, usb_switch_show, usb_switch_store);

static void uart_switch_mode(int mode)
{
	if (mode == SWITCH_MSM) {
                curr_uart_path = SWITCH_MSM;
                fsa9480_select_mode(0x1E);
                DEBUG_FSA9480("UART path changed - MSM");
	}
	else if(mode == SWITCH_MDM)
	{
                curr_uart_path = SWITCH_MDM;
                fsa9480_i2c_write(REGISTER_MANUALSW1, 0x90); //switching by V_Audio_L/R
                msleep(2);
                fsa9480_select_mode(0x1A);
                DEBUG_FSA9480("UART path changed - MDM");
	}
}

void uart_switch_mode_select(int mode) {
	struct samsung_parameter param_data;
	memset(&param_data,0,sizeof(struct samsung_parameter));

	msm_read_param(&param_data);

	uart_switch_mode(mode);
	param_data.uart_sel = mode;

	msm_write_param(&param_data);
}
EXPORT_SYMBOL(uart_switch_mode_select);

/* for sysfs control (/sys/class/sec/switch/uart_sel) */
static ssize_t uart_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	switch(curr_uart_path)
	{
		case SWITCH_MSM:
			sprintf(buf, "UART Switch : PDA");
			break;
		case SWITCH_MDM:
			sprintf(buf, "UART Switch : LTEMODEM");
			break;
		default:
			break;
	}

	return sprintf(buf, "%s\n", buf);
}

static ssize_t uart_switch_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	if(strncmp(buf, "PDA", 3) == 0 || strncmp(buf, "pda", 3) == 0) {
		uart_switch_mode_select(SWITCH_MSM);
	}

	if(strncmp(buf, "LTEMODEM", 8) == 0 || strncmp(buf, "ltemodem", 8) == 0) {
		uart_switch_mode_select(SWITCH_MDM);
	}

	return size;
}
static DEVICE_ATTR(uart_sel, S_IRUGO | S_IWUSR, uart_switch_show, uart_switch_store);

static ssize_t UsbMenuSel_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int usbmode = android_usb_get_current_mode();

    if (usbmode & USB_MODE_RNDIS)
        return sprintf(buf, "[UsbMenuSel] VTP\n");
    if (usbmode & USB_MODE_MTP)
        return sprintf(buf,"[UsbMenuSel] MTP\n");
    if (usbmode & USB_MODE_UMS) {
        if (usbmode & USB_MODE_ADB)
            return sprintf(buf, "[UsbMenuSel] ACM_ADB_UMS\n");
        else
            return sprintf(buf, "[UsbMenuSel] UMS\n");
    }
    if (usbmode & USB_MODE_ADB)
        return sprintf(buf, "[UsbMenuSel] ADB\n");
    if (usbmode & USB_MODE_ASKON)
        return sprintf(buf, "[UsbMenuSel] ASK\n");

    return 0;
}
static ssize_t UsbMenuSel_switch_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int mode = 0;

    if (strncmp(buf, "MTP", 3) == 0) {
        mode = USB_MODE_MTP;
    }
    else if (strncmp(buf, "VTP", 3) == 0) {
        mode = USB_MODE_MTP;
    }
    else if (strncmp(buf, "UMS", 3) == 0) {
        mode = USB_MODE_UMS;
    }
    else if (strncmp(buf, "ASKON", 3) == 0) {
        mode = USB_MODE_ASKON;
    }

    if (mode) {
        struct samsung_parameter param_data;
        memset(&param_data,0,sizeof(struct samsung_parameter));
        msm_read_param(&param_data);
        if (param_data.usb_mode != mode) {
            param_data.usb_mode = mode;
            msm_write_param(&param_data);
        }

        android_usb_switch(mode);
    }

    return size;
}
static DEVICE_ATTR(UsbMenuSel, S_IRUGO | S_IWUSR, UsbMenuSel_switch_show, UsbMenuSel_switch_store);

static ssize_t AskOnStatus_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int usbmode = android_usb_get_current_mode();
    if(usbmode == USB_MODE_ASKON)
        return sprintf(buf, "%s\n", "NonBlocking");
    else
        return sprintf(buf, "%s\n", "Blocking");
}


static ssize_t AskOnStatus_switch_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    return size;
}

static DEVICE_ATTR(AskOnStatus, S_IRUGO | S_IWUSR, AskOnStatus_switch_show, AskOnStatus_switch_store);

static ssize_t AskOnMenuSel_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "[AskOnMenuSel] Port test ready!! \n");
}

static ssize_t AskOnMenuSel_switch_store(struct device *dev, struct device_attribute *attr,	const char *buf, size_t size)
{
    int mode = 0;
    if (strncmp(buf, "MTP", 3) == 0) {
        mode = USB_MODE_MTP;
    }
    if (strncmp(buf, "UMS", 3) == 0) {
        mode = USB_MODE_UMS;
    }
    android_usb_switch(mode | USB_MODE_ASKON);

    return size;
}

static DEVICE_ATTR(AskOnMenuSel, S_IRUGO | S_IWUSR, AskOnMenuSel_switch_show, AskOnMenuSel_switch_store);

static ssize_t DefaultESNStatus_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    if (g_default_ESN_status) {
        return sprintf(buf, "DefaultESNStatus : TRUE\n");
    }
    else{
        return sprintf(buf, "DefaultESNStatus : FALSE\n");
    }
}

static ssize_t DefaultESNStatus_switch_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    if ((strncmp(buf, "TRUE", 4) == 0) ||(strncmp(buf, "true", 4) == 0)) {
        g_default_ESN_status = 1;
    }
    if ((strncmp(buf, "FALSE", 5) == 0) ||(strncmp(buf, "false", 5) == 0)) {
        g_default_ESN_status = 0;
    }
}

static DEVICE_ATTR(DefaultESNStatus, S_IRUGO | S_IWUSR, DefaultESNStatus_switch_show, DefaultESNStatus_switch_store);


static ssize_t dock_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (g_dock == DOCK_REMOVED)
		return sprintf(buf, "0\n");
	else if (g_dock == DESK_DOCK_CONNECTED)
		return sprintf(buf, "1\n");
	else if (g_dock == CAR_DOCK_CONNECTED)
		return sprintf(buf, "2\n");
}

static ssize_t dock_switch_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	if (strncmp(buf, "0", 1) == 0)
	{
		printk("remove dock\n");
		g_dock = DOCK_REMOVED;
	}
	else if (strncmp(buf, "1", 1) == 0)
	{
		printk("dsek dock inserted\n");
		g_dock = DESK_DOCK_CONNECTED;
	}
	else if (strncmp(buf, "2", 1) == 0)
	{
		printk("car dock inserted\n");
		g_dock = CAR_DOCK_CONNECTED;
	}
	return size;
}

static DEVICE_ATTR(dock, S_IRUGO | S_IWUSR, dock_switch_show, dock_switch_store);


void usb_switch_state(void)
{
	struct samsung_parameter param_data;
	memset(&param_data,0,sizeof(struct samsung_parameter));

	msm_read_param(&param_data);

        switch(param_data.usb_sel)
        {
            case SWITCH_MSM:
                usb_switch_mode(SWITCH_MSM);
                break;
            case SWITCH_MDM:
                usb_switch_mode(SWITCH_MDM);
                break;
            case SWITCH_CYAS:
                usb_switch_mode(SWITCH_CYAS);
                break;
            default:
                break;
        }
}

/* for setting dock audio path */
void fsa9480_change_path_to_audio(u8 enable)
{
	if(enable)
	{
		mdelay(10);
		fsa9480_i2c_write(REGISTER_MANUALSW1, 0x48);

		mdelay(10);
		fsa9480_select_mode(0x1A);
	}
	else
	{
		mdelay(10);
		fsa9480_select_mode(0x1E);
	}
}

static void fsa9480_process_device(u8 dev1, u8 dev2, u8 attach)
{
#ifdef CONFIG_USBHUB_USB3803
	unsigned int usb3803_hub = 0xFF;
#endif

	pr_info("[FSA9480:%s] dev1:%d dev2:%d attach:%d\n", __func__, dev1, dev2, attach);

	if (dev1)
	{
		switch (dev1)
		{
			case CRA_AUDIO_TYPE1:
				DEBUG_FSA9480("AUDIO_TYPE1 \n");
				break;

			case CRA_AUDIO_TYPE2:
				DEBUG_FSA9480("AUDIO_TYPE2 \n");
				break;

			case CRA_USB:
#if defined CONFIG_MACH_CHIEF || defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || \
	defined(CONFIG_MACH_PREVAIL2)
                                     // reset askon connect usb mode
                                     if (android_usb_get_current_mode() & USB_MODE_ASKON)
                                          android_usb_switch(USB_MODE_ASKON);
#endif
				if((attach & ATTACH) && !Intr1_ovp_en){
				    DEBUG_FSA9480("USB --- ATTACH\n");
				    usb_switch_state();

#if defined(CONFIG_MACH_CHIEF)
					if(melfas_ts != NULL)
						melfas_write_reg(0x01);
#endif
#ifdef CONFIG_USBHUB_USB3803
					usb3803_hub = 0x1;
#endif
					curr_usb_status = 1;
				}
				else if(attach & DETACH){
				    DEBUG_FSA9480("USB --- DETACH\n");

#if defined(CONFIG_MACH_CHIEF)
					if(melfas_ts != NULL)
						melfas_write_reg(0x00);
#endif
#ifdef CONFIG_USBHUB_USB3803
					usb3803_hub = 0x0;
#endif
					curr_usb_status = 0;
					Intr1_ovp_en = 0;
				}

				if (attach & (ATTACH|DETACH))
					batt_restart();
				break;

			case CRA_UART:
				if(attach & ATTACH){
					curr_uart_status = 1;
					}
				else if(attach & DETACH){
					curr_uart_status = 0;
				}
				DEBUG_FSA9480("UART \n");
				break;

			case CRA_CARKIT:
				if( (attach & ATTACH) && !Intr1_ovp_en ){
				    DEBUG_FSA9480("CARKIT_CHARGER --- ATTACH\n");

#if defined(CONFIG_MACH_CHIEF)
					if(melfas_ts != NULL)
						melfas_write_reg(0x01);
#endif
				    curr_ta_status = 1;
				}
				else if(attach & DETACH){
				    DEBUG_FSA9480("CARKIT_CHARGER --- DETACH\n");

#if defined(CONFIG_MACH_CHIEF)
					if(melfas_ts != NULL)
						melfas_write_reg(0x00);
#endif
					curr_ta_status = 0;
					Intr1_ovp_en = 0;
				}

				if (attach & (ATTACH|DETACH))
					batt_restart();
				//DEBUG_FSA9480("CARKIT \n");
				break;

			case CRA_USB_CHARGER:
				DEBUG_FSA9480("USB_CHARGER \n");
				break;

			case CRA_DEDICATED_CHG:
				if( (attach & ATTACH) && !Intr1_ovp_en ){
				    DEBUG_FSA9480("DEDICATED_CHARGER --- ATTACH\n");

#if defined(CONFIG_MACH_CHIEF)
					if(melfas_ts != NULL)
						melfas_write_reg(0x01);
#endif
				    curr_ta_status = 1;
				}
				else if(attach & DETACH){
				    DEBUG_FSA9480("DEDICATED_CHARGER --- DETACH\n");

#if defined(CONFIG_MACH_CHIEF)
					if(melfas_ts != NULL)
						melfas_write_reg(0x00);
#endif
					curr_ta_status = 0;
					Intr1_ovp_en = 0;
				}

				if (attach & (ATTACH|DETACH))
					batt_restart();
				break;

			case CRA_USB_OTG:
				DEBUG_FSA9480("USB_OTG \n");
				break;

			default:
				break;
		}
	}

	if (dev2)
	{
		switch (dev2)
		{
			case CRB_JIG_USB_ON:
				if(attach & ATTACH){
					DEBUG_FSA9480("JIG_USB_ON --- ATTACH\n");
					usb_switch_state();
#ifdef CONFIG_USBHUB_USB3803
					usb3803_hub = 0x1;
#endif
				}
				else if(attach & DETACH){
					DEBUG_FSA9480("JIG_USB_ON --- DETACH\n");
#ifdef CONFIG_USBHUB_USB3803
					usb3803_hub = 0x0;
#endif
				}
				break;

			case CRB_JIG_USB_OFF:
				if(attach & ATTACH){
					DEBUG_FSA9480("JIG_USB_OFF --- ATTACH\n");
					usb_switch_state();
#if defined CONFIG_MACH_CHIEF //20100520_inchul
					wimax_usb_state= USB_CABLE_255K;
					MicroJigUSBOffStatus = 1;
#endif
#ifdef CONFIG_USBHUB_USB3803
					usb3803_hub = 0x1;
#endif
				}
				else if(attach & DETACH){
				    DEBUG_FSA9480("JIG_USB_OFF --- DETACH\n");
#if defined CONFIG_MACH_CHIEF //20100520_inchul
					wimax_usb_state= CABLE_DISCONNECT;
					MicroJigUSBOffStatus = 0;
#endif
#ifdef CONFIG_USBHUB_USB3803
					usb3803_hub = 0x0;
#endif
				}
				break;

			case CRB_JIG_UART_ON:
				curr_uart_status = 1;
				DEBUG_FSA9480("JIG_UART_ON \n");

				if(attach & ATTACH){
					unsigned size;
					samsung_vendor1_id *smem_vendor1_id = (samsung_vendor1_id *)smem_get_entry(SMEM_ID_VENDOR1, &size);
					DEBUG_FSA9480("AUDIO/VIDEO(CAR_DOCK) --- ATTACHED, no_console:%d, default_esn:0x%x\n", no_console, smem_vendor1_id->default_esn);

					if(no_console && (smem_vendor1_id->default_esn != 0xFAC7049)) {
						fsa9480_change_path_to_audio(1);
					}
					else
						DEBUG_FSA9480("\t\t console enabled. path not changed to audio!!");

						switch_set_state(&switch_dock_detection, (int)CAR_DOCK_CONNECTED);
						g_dock = CAR_DOCK_CONNECTED;
				}
				else if(attach & DETACH){
					DEBUG_FSA9480("AUDIO/VIDEO(CAR_DOCK) --- DETACHED\n");
					switch_set_state(&switch_dock_detection, (int)DOCK_REMOVED);
					g_dock = DOCK_REMOVED;

					fsa9480_change_path_to_audio(0);
				}

				if (attach & (ATTACH|DETACH))
					batt_restart();
				break;

			case CRB_JIG_UART_OFF:
				curr_uart_status = 0;
				DEBUG_FSA9480("JIG_UART_OFF \n");
				break;

			case CRB_PPD:
				DEBUG_FSA9480("PPD \n");
				break;

			case CRB_TTY:
				DEBUG_FSA9480("TTY \n");
				break;

			case CRB_AV:
				DEBUG_FSA9480("AUDIO/VIDEO \n");
				if(attach & ATTACH){
					DEBUG_FSA9480("AUDIO/VIDEO(DESK_DOCK) --- ATTACHED\n");
					fsa9480_change_path_to_audio(1);
					switch_set_state(&switch_dock_detection, (int)DESK_DOCK_CONNECTED);
					g_dock = DESK_DOCK_CONNECTED;
				}
				else if(attach & DETACH){
					DEBUG_FSA9480("AUDIO/VIDEO(DESK_DOCK) --- DETACHED\n");
					fsa9480_change_path_to_audio(1);
					switch_set_state(&switch_dock_detection, (int)DOCK_REMOVED);
					g_dock = DOCK_REMOVED;
				}

				if (attach & (ATTACH|DETACH))
					batt_restart();
				break;

			default:
				break;
		}
	}

#ifdef CONFIG_USBHUB_USB3803

extern int charging_boot;

	if(charging_boot)
	{
		DEBUG_FSA9480("set USB3803 to BYPASS - dev1 : %d, dev2 : %d\n", dev1, dev2);
		usb3803_set_mode(USB_3803_MODE_BYPASS);
	}
	else
	{
		if( usb3803_hub != 0xFF) {
			if( usb3803_hub == 0x1) {
				DEBUG_FSA9480("set USB3803 to HUB - dev1 : %d, dev2 : %d\n", dev1, dev2);
				usb3803_set_mode(USB_3803_MODE_HUB);
			}
			else if( usb3803_hub == 0x0) {
				DEBUG_FSA9480("set USB3803 to BYPASS - dev1 : %d, dev2 : %d\n", dev1, dev2);
				usb3803_set_mode(USB_3803_MODE_BYPASS);
			}
		}
	}
#endif

	if ( curr_uart_status && ( attach & DETACH ) ) {
		curr_uart_status = 0;
		DEBUG_FSA9480("UART_OFF \n");
	}

         if( attach & (KEY_PRESS|LONG_KEY_PRESS|LONG_KEY_RELEASE) )
         {
		u8 button2;
		bool is_press = 0;
		bool is_longkey = 0;

		/* read button register */
		fsa9480_i2c_read(REGISTER_BUTTON2, &button2);

		if( attach & KEY_PRESS )
		{
			is_longkey = 0;

			DEBUG_FSA9480("KEY PRESS\n");
		}
		else
		{
			is_longkey = 1;

			if( attach & LONG_KEY_PRESS )
			{
				is_press = 1;

				DEBUG_FSA9480("LONG KEY PRESS\n");
			}
			else
			{
				is_press = 0;

				DEBUG_FSA9480("LONG KEY RELEASE\n");
			}
		}

    	        if( g_dock == DESK_DOCK_CONNECTED )
    	        {
		        u8 key_mask;
		        int index = 0;

		        key_mask = (button2 & DOCK_KEY_MASK) >> DOCK_KEY_SHIFT;

		        DEBUG_FSA9480("key_mask 0x%x \n", key_mask);

    		        while(key_mask)
    		        {
    			    if( key_mask & 0x1 )
			    {
				if( is_longkey )
				{
    				    dock_keys_input(index, is_press);
				}
				else
				{
				    dock_keys_input(index, 1);
				    dock_keys_input(index, 0);
				}
			    }

    			    key_mask >>= 1;
    			    index++;
    		        }
    	        }
         }

}
#endif

void fsa9480_read_interrupt_register(void)
{
	u8 dev1=0, dev2=0, intr1=0;
	u8 carkit_int1=0,carkit_int2=0;
	u8 interrupt2=0, adc=0;
	u8 valid_5v = 0, control_reg = 0;
	int ret = 0;
	int retrycnt = 2;
	int i = 0, intr1_count = 0, intr2_count = 0;

	while(retrycnt--) {
		ret = fsa9480_i2c_read(REGISTER_INTERRUPT1, &intr1);  //0x03
		ret |= fsa9480_i2c_read(REGISTER_INTERRUPT2, &interrupt2);  //0x04

		ret |= fsa9480_i2c_read(REGISTER_CARKITINT1, &carkit_int1);  //0x0F
		ret |= fsa9480_i2c_read(REGISTER_CARKITINT2, &carkit_int2);  //0x10

		ret |= fsa9480_i2c_read(REGISTER_ADC, &adc); //0x07

		ret |= fsa9480_i2c_read(REGISTER_DEVICETYPE1, &dev1); //0x0A
		ret |= fsa9480_i2c_read(REGISTER_DEVICETYPE2, &dev2); //0x0B

		ret |= fsa9480_i2c_read(0x1D, &valid_5v);	//hidden

/*FSA9480 Issue*/
//////////////////////////////////////////////////////////////////////////////
		for( i  = 0 ; i < 8 ; i++ )
		{
			if( intr1 & (0x01 << i))	intr1_count++;
			if( interrupt2 & (0x01 << i))	intr2_count++;
		}

		if(intr1_count > 1)
		{
			printk("[FSA9480] intr1 register is wrong!!! After 30m seconds read!!\n");
			msleep(30);
			ret |= fsa9480_i2c_read(REGISTER_INTERRUPT1, &intr1);  //0x03
		}

		if(intr2_count > 1)
		{
			printk("[FSA9480] intr2 register is wrong!!! After 30m seconds read!!\n");
			msleep(30);
			ret |= fsa9480_i2c_read(REGISTER_INTERRUPT2, &interrupt2);  //0x04
		}
//////////////////////////////////////////////////////////////////////////////

		printk("[FSA9480] dev1=0x%x, dev2=0x%x, intr1=0x%x, intr2=0x%x\n ",dev1,dev2,intr1,interrupt2);
		printk("[FSA9480]  carkit_int1=0x%x, carkit_int2=0x%x, valid_5v=0x%x\n",carkit_int1,carkit_int2, valid_5v);

		// read was successful break out of this loop
		if(!ret) break;

		// i2c read failed! reset the device and retry!!
		pr_err("!![FSA9480:%s] i2c failed. reset device!\n", __func__);
		fsa9480_reset_device();
	}

#if defined CONFIG_MACH_CHIEF || defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || \
	defined(CONFIG_MACH_PREVAIL2)
	if(dev1 & CRA_USB_CHARGER)
	{
		printk("[FSA9480] dev1 = CRA_USB_CHARGER, Not used. re-check!!!\n");
		fsa9480_i2c_read(REGISTER_DEVICETYPE1, &dev1); //0x0A
	}	
#endif

	if(!intr1)
	{
		fsa9480_i2c_read(REGISTER_CONTROL, &control_reg);
		printk("[FSA9480] control_reg : 0x%x\n", control_reg);
		if(control_reg == 0x1F)
		{
			fsa9480_i2c_write(REGISTER_CONTROL, 0x1E);
			printk("[FSA9480] write control_register 0x1E\n");
			msleep(10);
		}

		fsa9480_i2c_read(REGISTER_DEVICETYPE1, &dev1); //0x0A
		fsa9480_i2c_read(REGISTER_DEVICETYPE2, &dev2); //0x0B

		printk("[FSA9480 : dev1 = 0x%x, dev2 = 0x%x\n", dev1, dev2);

		if(!dev1 && !dev2)
			intr1 |= DETACH;
	}
	
	if( dev1 && !intr1 && (valid_5v & 0x02) )	
		intr1 |= ATTACH;

	if (intr1 & OVP_EN)	Intr1_ovp_en = 1;

#if defined CONFIG_MACH_CHIEF
	if ( !wimax_255k_check_in_booting && ( dev2 == CRB_JIG_USB_OFF)) {
		printk("[FSA9480] target booting with 255k jig cable attached\n");
		intr1 = intr1 | ATTACH;
	}
	wimax_255k_check_in_booting = 1;
#endif

	/* Device attached */
	if (intr1 & ATTACH)
	{
		fsa9480_device1 = dev1;
		fsa9480_device2 = dev2;
	}
#ifdef CONFIG_USBHUB_USB3803
	else if(dev1 || dev2) //for support boot sequence & need to sync. with fsa9480_process_device()
	{
		if(dev1 == CRA_USB || dev1 == CRA_DEDICATED_CHG) {
			fsa9480_device1 = dev1;
			intr1 = intr1 | ATTACH;
		} else if(dev2 == CRB_JIG_USB_ON || dev2 == CRB_JIG_USB_OFF) {
			fsa9480_device2 = dev2;
			intr1 = intr1 | ATTACH;
		}
	}
#endif
#if defined CONFIG_MACH_CHIEF || defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || \
	defined(CONFIG_MACH_PREVAIL2)
	fsa9480_process_device(fsa9480_device1, fsa9480_device2, intr1);
#else
	if (fsa9480_device1)
	{
		switch (fsa9480_device1)
		{
			case CRA_USB:
#if 0
                // reset askon connect usb mode
                if (android_usb_get_current_mode() & USB_MODE_ASKON)
                    android_usb_switch(USB_MODE_ASKON);
#endif

#ifdef CONFIG_USB_GADGET_WESTBRIDGE
    //Vova: Detection of USB attach/detach for WestBridge chip
     //TBD - probaby need to be moved to gadget controller

				if (intr1 & ATTACH)
					curr_usb_status = 1;
				else if (intr1 & DETACH)
					curr_usb_status = 0;
				if (intr1 & (ATTACH|DETACH))
					batt_restart();
				break;
			case CRA_DEDICATED_CHG:
				if (intr1 & ATTACH)
					curr_ta_status = 1;
				else if (intr1 & DETACH)
					curr_ta_status = 0;
				if (intr1 & (ATTACH|DETACH))
					batt_restart();
#endif//Vova: End of detection block:
				break;
			default:
				break;
		}
	}

#endif

#if defined CONFIG_MACH_CHIEF  //20100520_inchul
	 if (fsa9480_device2)
	 {
		 switch (fsa9480_device2)
		 {
		 	 case CRB_AV:
			 case CRB_JIG_UART_OFF:
		 	 case CRB_JIG_UART_ON:
			 	if (intr1 & ATTACH)
					 curr_dock_flag = 1;
				 else if (intr1 & DETACH)
				 {
					 curr_dock_flag = 0;
					 curr_ta_status = 0;
					 pre_dock_status = 0;
					 batt_restart();
				 }
			 	break;

			 case CRB_JIG_USB_OFF:
				 if (intr1 & ATTACH)
					 curr_ta_status = 1;
				 else if (intr1 & DETACH)
					 curr_ta_status = 0;
				 if (intr1 & (ATTACH|DETACH))
					 batt_restart();
				 break;

			 default:
				 break;
		 }
	 }

		 if((interrupt2 & FSA9480_INT2_RESERVED_ATTACH) && (adc == RESERVED_ACCESSORY_4))
	  {
		 wimax_usb_state = USB_CABLE_50K;
//				s3c_usb_cable(1);//mkh
		 usb_switch_state();
		 }
	  else
	  {
		 if(!MicroJigUSBOffStatus)
		 {
			wimax_usb_state = CABLE_DISCONNECT;
		 }
	  }
#endif

#if defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2)  || defined(CONFIG_MACH_PREVAIL2)
	 if (fsa9480_device2)
	 {
		 switch (fsa9480_device2)
		 {
		 	 case CRB_AV:
			 case CRB_JIG_UART_OFF:
			 case CRB_JIG_UART_ON:
				 if (intr1 & ATTACH)
					 curr_dock_flag = 1;
				 else if (intr1 & DETACH)
				 {
					 curr_dock_flag = 0;
					 curr_ta_status = 0;
					 pre_dock_status = 0;
					 batt_restart();
				 }
				 break;

			 default:
				 break;
		 }
	 }
#endif
	
	/* Device detached */
	if (intr1 & DETACH)
	{
		fsa9480_device1 = 0x0;
		fsa9480_device2 = 0x0;

		if(curr_ta_status || curr_usb_status)
		{
			curr_ta_status = 0;
			curr_usb_status = 0;
			batt_restart();
		}
	}

}

void fsa9480_irq_disable(int disable)
{
	disable_irq_nosync(pclient->irq);
	cancel_delayed_work_sync(&fsa9480_delayed_wq);
}

static irqreturn_t fsa9480_interrupt_handler(int irq, void *data)
{
        struct i2c_client *client;
        struct fsa_platform_data *pdata;;
        u8 temp_reg;

        cancel_delayed_work_sync(&fsa9480_delayed_wq);

        printk(KERN_INFO "[FSA9480]: interrupt called\n");

        fsa9480_read_interrupt_register();

        schedule_delayed_work(&fsa9480_delayed_wq,
            round_jiffies_relative(msecs_to_jiffies(1000)));
        return IRQ_HANDLED;
}

static int fsa9480_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct fsa9480_data *mt;
#if defined CONFIG_MACH_CHIEF || defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || \
	defined(CONFIG_MACH_PREVAIL2)
	struct samsung_parameter *param_data;
	int i;
#endif
	int err = -1;
	int ret;

	printk(KERN_INFO "[FSA9480]: probe\n");

	if(!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		goto exit_check_functionality_failed;

	if(!(mt = kzalloc( sizeof(struct fsa9480_data), GFP_KERNEL))) {
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

#if defined CONFIG_MACH_CHIEF || defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || \
	defined(CONFIG_MACH_PREVAIL2)
     mt->pdata = client->dev.platform_data;
     if(mt->pdata && mt->pdata->vreg_en)
		mt->pdata->vreg_en(1);
#endif

	i2c_set_clientdata(client, mt);
	fsa9480_client(client);
	pclient = client;

	fsa9480_chip_init();

      INIT_DELAYED_WORK(&fsa9480_delayed_wq, fsa9480_delayed_work_function);


//	if(request_threaded_irq(pclient->irq, NULL, fsa9480_interrupt_handler, IRQF_ONESHOT |IRQF_TRIGGER_FALLING, "MICROUSB", pclient)) {
		if(request_threaded_irq(pclient->irq, NULL, fsa9480_interrupt_handler, IRQF_ONESHOT |IRQF_TRIGGER_FALLING, "MICROUSB", pclient)) {
		free_irq(pclient->irq, NULL);
		printk("[FSA9480] fsa9480_interrupt_handler can't register the handler! and passing....\n");
	}

#if defined CONFIG_MACH_CHIEF || defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || \
	defined(CONFIG_MACH_PREVAIL2)
	if ( !(param_data = kzalloc(sizeof(struct samsung_parameter),GFP_KERNEL))) {
		printk("######### can not alloc memory for param_data ! ##################\n");
		kfree(param_data);
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}
	memset(param_data,0,sizeof(struct samsung_parameter));

	msm_read_param(param_data);

	if ( param_data->usb_sel == SWITCH_MDM ) {
		usb_switch_mode(SWITCH_MDM);
		printk("Current USB Owner -> LTEMODEM\n");
	}
	else if ( param_data->usb_sel == SWITCH_MSM ) {
		//param_data->usb_mode = USB_MODE_UMS;
		usb_switch_mode(SWITCH_MSM);
		printk("Current USB Owner -> PDA\n");
	}
	else if ( param_data->usb_sel == SWITCH_CYAS) {
		usb_switch_mode(SWITCH_CYAS);
		printk("Current USB Owner -> CYAS\n");
	 }
	else {
		printk("usb_sel parameter initialize to MSM default.\n");
		param_data->usb_sel = SWITCH_MSM;
		param_data->usb_mode = USB_MODE_UMS;
		msm_write_param(param_data);
	}

#if 1 //20101206_inchul
	if ( param_data->uart_sel == SWITCH_MDM ) {
		DEBUG_FSA9480("Current UART Owner on Booting -> LTEMODEM\n");
		DEBUG_FSA9480("Uart path is changed as MSM(Default)\n");
		uart_switch_mode_select(SWITCH_MSM);
	}
	else if ( param_data->uart_sel == SWITCH_MSM ) {
		DEBUG_FSA9480("Current UART Owner on Booting -> MSM\n");
	}
         else{
		DEBUG_FSA9480("uart_sel parameter initialize to MSM default.\n");
		param_data->uart_sel = SWITCH_MSM;
		msm_write_param(param_data);
         }
#endif

	android_usb_switch(param_data->usb_mode);

	kfree(param_data);

	if (device_create_file(switch_dev, &dev_attr_usb_sel) < 0)
		printk(KERN_ERR "[FSA9480]: Failed to create device file(%s)!\n", dev_attr_usb_sel.attr.name);
	if (device_create_file(switch_dev, &dev_attr_uart_sel) < 0)
		printk(KERN_ERR "[FSA9480]: Failed to create device file(%s)!\n", dev_attr_uart_sel.attr.name);
	if (device_create_file(switch_dev, &dev_attr_UsbMenuSel) < 0)
		printk("Failed to create device file(%s)!\n", dev_attr_UsbMenuSel.attr.name);
	if (device_create_file(switch_dev, &dev_attr_AskOnMenuSel) < 0)
		printk("Failed to create device file(%s)!\n", dev_attr_AskOnMenuSel.attr.name);
	if (device_create_file(switch_dev, &dev_attr_AskOnStatus) < 0)
		printk("Failed to create device file(%s)!\n", dev_attr_AskOnStatus.attr.name);
	if (device_create_file(switch_dev, &dev_attr_DefaultESNStatus) < 0) //20101129_inchul
		printk("Failed to create device file(%s)!\n", dev_attr_DefaultESNStatus.attr.name);
#if defined CONFIG_MACH_CHIEF //20100520_inchul
	if (device_create_file(switch_dev, &dev_attr_wimax_usb_state) < 0)
		printk("Failed to create device file(%s)!\n", dev_attr_wimax_usb_state.attr.name);
#endif

//20101125_inchul
	dock_key_input_dev = input_allocate_device();
	if( !dock_key_input_dev )
		return -ENOMEM;

	dock_key_input_dev->name = switch_dock_detection.name;
	set_bit(EV_SYN, dock_key_input_dev->evbit);
	set_bit(EV_KEY, dock_key_input_dev->evbit);

	for(i=0; i < DOCK_KEY_MAX; i++)
	{
		set_bit(dock_keys[i], dock_key_input_dev->keybit);
	}

	err = input_register_device(dock_key_input_dev);
	if( err ) {
		input_free_device(dock_key_input_dev);
		return err;
	}

	ret = switch_dev_register(&switch_dock_detection);
	if( ret ) {
		printk(KERN_ERR "Failed to register driver\n");
	}
//20101125_inchul
#endif

	printk(KERN_INFO "[FSA9480]: probe complete\n");
	return 0;

exit_alloc_data_failed:
exit_check_functionality_failed:

	return err;
}


static int fsa9480_remove(struct i2c_client *client)
{
	struct fsa9480_data *mt;

	mt = i2c_get_clientdata(client);
	free_irq(client->irq, mt);
	pclient = NULL;
	kfree(mt);
	return 0;
}


static const struct i2c_device_id fsa9480_id[] = {
	{ "fsa9480", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, fsa9480_id);


static struct i2c_driver fsa9480_driver = {
	.probe 		= fsa9480_probe,
	.remove 	= fsa9480_remove,
//	.shutdown = fsa9480_remove,
	.id_table	= fsa9480_id,
	.driver = {
		.name   = "fsa9480",
	},
};

static int __init fsa9480_init(void)
{
	return i2c_add_driver(&fsa9480_driver);
}

static void __exit fsa9480_exit(void)
{
	i2c_del_driver(&fsa9480_driver);
}


EXPORT_SYMBOL(fsa9480_i2c_read);
EXPORT_SYMBOL(fsa9480_i2c_write);

module_init(fsa9480_init);
module_exit(fsa9480_exit);

MODULE_AUTHOR("");
MODULE_DESCRIPTION("fsa9480 Driver");
MODULE_LICENSE("GPL");
