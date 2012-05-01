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

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/bitops.h>
#include <linux/mfd/pmic8058.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>

#include <linux/input/pmic8058-keypad.h>

#if defined CONFIG_MACH_CHIEF || defined CONFIG_MACH_VITAL2  || defined CONFIG_MACH_ROOKIE2 || \
	defined(CONFIG_MACH_PREVAIL2)
#include <linux/gpio.h>
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#define PM8058_MAX_ROWS		18
#define PM8058_MAX_COLS		8
#define PM8058_ROW_SHIFT	3
#define PM8058_MATRIX_MAX_SIZE	(PM8058_MAX_ROWS * PM8058_MAX_COLS)

#define PM8058_MIN_ROWS		5
#define PM8058_MIN_COLS		5

#define MAX_SCAN_DELAY		128
#define MIN_SCAN_DELAY		1

/* in nanoseconds */
#define MAX_ROW_HOLD_DELAY	122000
#define MIN_ROW_HOLD_DELAY	30500

#define MAX_DEBOUNCE_B0_TIME	20
#define MIN_DEBOUNCE_B0_TIME	5

#define MAX_DEBOUNCE_A0_TIME	8
#define MIN_DEBOUNCE_A0_TIME	1

#define KEYP_CTRL			0x148

#define KEYP_CTRL_EVNTS			BIT(0)
#define KEYP_CTRL_EVNTS_MASK		0x3

#define KEYP_CTRL_SCAN_COLS_SHIFT	5
#define KEYP_CTRL_SCAN_COLS_MIN		5
#define KEYP_CTRL_SCAN_COLS_BITS	0x3

#define KEYP_CTRL_SCAN_ROWS_SHIFT	2
#define KEYP_CTRL_SCAN_ROWS_MIN		5
#define KEYP_CTRL_SCAN_ROWS_BITS	0x7

#define KEYP_CTRL_KEYP_EN		BIT(7)

#define KEYP_SCAN			0x149

#define KEYP_SCAN_READ_STATE		BIT(0)
#define KEYP_SCAN_DBOUNCE_SHIFT		1
#define KEYP_SCAN_PAUSE_SHIFT		3
#define KEYP_SCAN_ROW_HOLD_SHIFT	6

#define KEYP_TEST			0x14A

#define KEYP_TEST_CLEAR_RECENT_SCAN	BIT(6)
#define KEYP_TEST_CLEAR_OLD_SCAN	BIT(5)
#define KEYP_TEST_READ_RESET		BIT(4)
#define KEYP_TEST_DTEST_EN		BIT(3)
#define KEYP_TEST_ABORT_READ		BIT(0)

#define KEYP_TEST_DBG_SELECT_SHIFT	1

/* bits of these register represent
 * '0' for key press
 * '1' for key release
 */
#define KEYP_RECENT_DATA		0x14B
#define KEYP_OLD_DATA			0x14C

#define KEYP_CLOCK_FREQ			32768

/* Internal flags */
#define KEYF_FIX_LAST_ROW		0x01

#if defined CONFIG_MACH_CHIEF
#define TACT_SW_CHATTERING_DISABLED	/* Panasonic Light Touch Switch, EVQP3N01K */
#define PMIC_GPIO_VOLUME_UPKEY	((system_rev>=5)?PM8058_GPIO(26):PM8058_GPIO(6))  /* PMIC GPIO Number 6 HW Rev 01 */
#elif defined CONFIG_MACH_VITAL2 || defined CONFIG_MACH_ROOKIE2 || defined(CONFIG_MACH_PREVAIL2)
#define PMIC_GPIO_VOLUME_UPKEY	PM8058_GPIO(26)
#define MSM_HALL_IC  ((system_rev>=2)?40:68)
#endif

#if defined CONFIG_MACH_CHIEF || defined CONFIG_MACH_VITAL2 || defined CONFIG_MACH_ROOKIE2 || \
	defined(CONFIG_MACH_PREVAIL2)
#define PMIC_GPIO_VOLUME_DOWNKEY	PM8058_GPIO(37)  /* PMIC GPIO Number 6 HW Rev 06 */
#define PMIC_GPIO_1             	PM8058_GPIO(1)   /* PMIC GPIO Number 1 HW Rev 06 */
#define PMIC_GPIO_2             	PM8058_GPIO(2)   /* PMIC GPIO Number 2 HW Rev 06 */
#define PMIC_GPIO_3             	PM8058_GPIO(3)   /* PMIC GPIO Number 3 HW Rev 06 */
#define PMIC_GPIO_4             	PM8058_GPIO(4)   /* PMIC GPIO Number 4 HW Rev 06 */
#define PMIC_GPIO_9             	PM8058_GPIO(9)   /* PMIC GPIO Number 9 HW Rev 06 */
#endif

/* define GPIO IRQ No. */
#if defined CONFIG_MACH_CHIEF || defined CONFIG_MACH_VITAL2 || defined CONFIG_MACH_ROOKIE2 || \
	defined(CONFIG_MACH_PREVAIL2)
#define	MSM_GPIO_KEY_VOLUP_IRQ 		PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, (PMIC_GPIO_VOLUME_UPKEY)) 
#define	MSM_GPIO_KEY_VOLDOWN_IRQ 	PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, (PMIC_GPIO_VOLUME_DOWNKEY))  /* yhkim + L&T prshnt:- for rev06*/
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
void pmic8058_kp_early_suspend(struct early_suspend *h);
void pmic8058_kp_late_resume(struct early_suspend *h);

bool is_early_suspend; //for Chief HW rev06 only
#endif

#if defined CONFIG_MACH_VITAL2 || defined CONFIG_MACH_ROOKIE2 || defined(CONFIG_MACH_PREVAIL2)
static struct hrtimer left_key_timer;
static struct hrtimer right_key_timer;

#define chatter_time 300000000 //ns demension
static struct chattering_key{
	char left;
	int left_value;
	int left_chatter_value;
	
	char right;
	int right_value;
	int right_chatter_value;
} chattering_key;
#endif

int get_voice_call_status(void);

/* ---------------------------------------------------------------------*/

struct pmic8058_kp {
	const struct pmic8058_keypad_data *pdata;
	struct input_dev *input;
	int key_sense_irq;
	int key_stuck_irq;

	unsigned short *keycodes;

	struct device *dev;
	u16 keystate[PM8058_MAX_ROWS];
	u16 stuckstate[PM8058_MAX_ROWS];

	u32	flags;
	struct pm8058_chip	*pm_chip;

	/* protect read/write */
	struct mutex		mutex;
	bool			user_disabled;
	u32			disable_depth;

	u8			ctrl_reg;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend	early_suspend;
#endif
	int volume_key_wake_enabled;
};


static int pmic8058_kp_write_u8(struct pmic8058_kp *kp,
				 u8 data, u16 reg)
{
	int rc;

	rc = pm8058_write(kp->pm_chip, reg, &data, 1);
	if (rc < 0)
		dev_warn(kp->dev, "Error writing pmic8058: %X - ret %X\n",
				reg, rc);
	return rc;
}

static int pmic8058_kp_read(struct pmic8058_kp *kp,
				 u8 *data, u16 reg, unsigned num_bytes)
{
	int rc;

	rc = pm8058_read(kp->pm_chip, reg, data, num_bytes);
	if (rc < 0)
		dev_warn(kp->dev, "Error reading pmic8058: %X - ret %X\n",
				reg, rc);

	return rc;
}

static int pmic8058_kp_read_u8(struct pmic8058_kp *kp,
				 u8 *data, u16 reg)
{
	int rc;

	rc = pmic8058_kp_read(kp, data, reg, 1);
	if (rc < 0)
		dev_warn(kp->dev, "Error reading pmic8058: %X - ret %X\n",
				reg, rc);
	return rc;
}

static u8 pmic8058_col_state(struct pmic8058_kp *kp, u8 col)
{
	/* all keys pressed on that particular row? */
	if (col == 0x00)
		return 1 << kp->pdata->num_cols;
	else
		return col & ((1 << kp->pdata->num_cols) - 1);
}
/* REVISIT: just for debugging, will be removed in final working version */
static void __dump_kp_regs(struct pmic8058_kp *kp, char *msg)
{
	u8 temp;

	dev_dbg(kp->dev, "%s\n", msg);

	pmic8058_kp_read_u8(kp, &temp, KEYP_CTRL);
	dev_dbg(kp->dev, "KEYP_CTRL - %X\n", temp);
	pmic8058_kp_read_u8(kp, &temp, KEYP_SCAN);
	dev_dbg(kp->dev, "KEYP_SCAN - %X\n", temp);
	pmic8058_kp_read_u8(kp, &temp, KEYP_TEST);
	dev_dbg(kp->dev, "KEYP_TEST - %X\n", temp);
}

/* H/W constraint:
 * One should read recent/old data registers equal to the
 * number of columns programmed in the keyp_control register,
 * otherwise h/w state machine may get stuck. In order to avoid this
 * situation one should check readstate bit in keypad scan
 * register to be '0' at the end of data read, to make sure
 * the keypad state machine is not in READ state.
 */
static int pmic8058_chk_read_state(struct pmic8058_kp *kp, u16 data_reg)
{
	u8 temp, scan_val;
	int retries = 10, rc;

	do {
		rc = pmic8058_kp_read_u8(kp, &scan_val, KEYP_SCAN);
		if (scan_val & 0x1)
			rc = pmic8058_kp_read_u8(kp, &temp, data_reg);
	} while ((scan_val & 0x1) && (--retries > 0));

	if (retries == 0)
		dev_dbg(kp->dev, "Unable to clear read state bit\n");

	return 0;
}
/*
 * Synchronous read protocol for RevB0 onwards:
 *
 * 1. Write '1' to ReadState bit in KEYP_SCAN register
 * 2. Wait 2*32KHz clocks, so that HW can successfully enter read mode
 *    synchronously
 * 3. Read rows in old array first if events are more than one
 * 4. Read rows in recent array
 * 5. Wait 4*32KHz clocks
 * 6. Write '0' to ReadState bit of KEYP_SCAN register so that hw can
 *    synchronously exit read mode.
 */
static int pmic8058_chk_sync_read(struct pmic8058_kp *kp)
{
	int rc;
	u8 scan_val;

	rc = pmic8058_kp_read_u8(kp, &scan_val, KEYP_SCAN);
	scan_val |= 0x1;
	rc = pmic8058_kp_write_u8(kp, scan_val, KEYP_SCAN);

	/* 2 * 32KHz clocks */
	udelay((2 * USEC_PER_SEC / KEYP_CLOCK_FREQ) + 1);

	return rc;
}

static int pmic8058_kp_read_data(struct pmic8058_kp *kp, u16 *state,
					u16 data_reg, int read_rows)
{
	int rc, row;
	u8 new_data[PM8058_MAX_ROWS];

	rc = pmic8058_kp_read(kp, new_data, data_reg, read_rows);

	if (!rc) {
		if (pm8058_rev(kp->pm_chip) == PM_8058_REV_1p0)
			pmic8058_chk_read_state(kp, data_reg);
		for (row = 0; row < kp->pdata->num_rows; row++) {
			dev_dbg(kp->dev, "new_data[%d] = %d\n", row,
						new_data[row]);
			state[row] = pmic8058_col_state(kp, new_data[row]);
		}
	}

	return rc;
}

static int pmic8058_kp_read_matrix(struct pmic8058_kp *kp, u16 *new_state,
					 u16 *old_state)
{
	int rc, read_rows;
	u8 scan_val;
	static u8 rows[] = {
		5, 6, 7, 8, 10, 10, 12, 12, 15, 15, 15, 18, 18, 18
	};

	if (kp->flags & KEYF_FIX_LAST_ROW &&
			(kp->pdata->num_rows != PM8058_MAX_ROWS))
		read_rows = rows[kp->pdata->num_rows - KEYP_CTRL_SCAN_ROWS_MIN
					 + 1];
#if defined CONFIG_MACH_CHIEF || defined CONFIG_MACH_VITAL2 || defined CONFIG_MACH_ROOKIE2  || \
	defined(CONFIG_MACH_PREVAIL2)//Qualcomm patch for keypad matrix 2*5 
	else if (kp->pdata->num_rows < PM8058_MIN_ROWS) 
		read_rows = PM8058_MIN_ROWS;				
#endif
	else
		read_rows = kp->pdata->num_rows;

	if (pm8058_rev(kp->pm_chip) > PM_8058_REV_1p0)
		pmic8058_chk_sync_read(kp);

	if (old_state)
		rc = pmic8058_kp_read_data(kp, old_state, KEYP_OLD_DATA,
						read_rows);

	rc = pmic8058_kp_read_data(kp, new_state, KEYP_RECENT_DATA,
					 read_rows);

	if (pm8058_rev(kp->pm_chip) > PM_8058_REV_1p0) {
		/* 4 * 32KHz clocks */
		udelay((4 * USEC_PER_SEC / KEYP_CLOCK_FREQ) + 1);

		rc = pmic8058_kp_read(kp, &scan_val, KEYP_SCAN, 1);
		scan_val &= 0xFE;
		rc = pmic8058_kp_write_u8(kp, scan_val, KEYP_SCAN);
	}

	return rc;
}

static int __pmic8058_kp_scan_matrix(struct pmic8058_kp *kp, u16 *new_state,
					 u16 *old_state)
{
	int row, col, code;
	
	for (row = 0; row < kp->pdata->num_rows; row++) {
		int bits_changed = new_state[row] ^ old_state[row];

		if (!bits_changed)
			continue;

		for (col = 0; col < kp->pdata->num_cols; col++) {
			if (!(bits_changed & (1 << col)))
				continue;
//      Protected the personal information : Google Logchecker issue
//			dev_dbg(kp->dev, "key [%d:%d] %s\n", row, col,
//					!(new_state[row] & (1 << col)) ?
//					"pressed" : "released");

			code = MATRIX_SCAN_CODE(row, col, PM8058_ROW_SHIFT);

#if defined(CONFIG_MACH_VITAL2) || defined (CONFIG_MACH_ROOKIE2) || defined(CONFIG_MACH_PREVAIL2)
				if((kp->keycodes[code] == KEY_MENU) || (kp->keycodes[code] == KEY_HOME)){
					if(chattering_key.left && (chattering_key.left_value == 0 ? 0 : kp->keycodes[code] != chattering_key.left_value)){
						//pressed
						chattering_key.left_chatter_value = kp->keycodes[code];
						printk("%s returned by left side value : %s\n",__func__,kp->keycodes[code] == KEY_MENU ? "KEY_MENU":"KEY_HOME");
						return 0;
					}

					if((chattering_key.left_chatter_value == kp->keycodes[code]) && ((new_state[row] & (1 << col)))){
						//released
						chattering_key.left_chatter_value = 0; 
						return 0;
					}
						
					hrtimer_cancel(&left_key_timer);
					
					if(!(new_state[row] & (1 << col))){
						//pressed
						chattering_key.left = 1;
						chattering_key.left_value = kp->keycodes[code];
						hrtimer_start(&left_key_timer,ktime_set(0,chatter_time), HRTIMER_MODE_REL);
					}else{
						//released
						chattering_key.left = 0;
						chattering_key.left_value = 0;
					}
				}else if((kp->keycodes[code] == KEY_BACK) || (kp->keycodes[code] == KEY_SEARCH)){
					if(chattering_key.right && (chattering_key.right_value == 0 ? 0 : kp->keycodes[code] != chattering_key.right_value)){
						//pressed
						chattering_key.right_chatter_value = kp->keycodes[code];
						printk("%s returned by right side value : %s\n",__func__,kp->keycodes[code] == KEY_BACK ? "KEY_BACK":"KEY_SEARCH");
						return 0;
					}

					if((chattering_key.right_chatter_value == kp->keycodes[code]) && ((new_state[row] & (1 << col)))){
						//released
						chattering_key.right_chatter_value = 0; 
						return 0;
					}
					
					hrtimer_cancel(&right_key_timer);
					
					if(!(new_state[row] & (1 << col))){
						//pressed
						chattering_key.right = 1;
						chattering_key.right_value = kp->keycodes[code];
						hrtimer_start(&right_key_timer,ktime_set(0, chatter_time), HRTIMER_MODE_REL);
					}else{
						//released
						chattering_key.right = 0;
						chattering_key.right_value = 0;
					}
				}	

				input_event(kp->input, EV_MSC, MSC_SCAN, code);
				input_report_key(kp->input, kp->keycodes[code], !(new_state[row] & (1 << col)));
				input_sync(kp->input);
				
#else
				input_event(kp->input, EV_MSC, MSC_SCAN, code);
				input_report_key(kp->input, kp->keycodes[code], !(new_state[row] & (1 << col)));

				input_sync(kp->input);
#endif
#if 0  //Fixing Log Checker issues
#ifndef PRODUCT_SHIP
//			printk("key [%d:%d] keycode [%d] %s\n", row, col, kp->keycodes[code], !(new_state[row] & (1 << col)) ?"pressed" : "released");
#endif
#endif
		}
	}

	return 0;
}

static int pmic8058_detect_ghost_keys(struct pmic8058_kp *kp, u16 *new_state)
{
	int row, found_first = -1;
	u16 check, row_state;

	check = 0;
	for (row = 0; row < kp->pdata->num_rows; row++) {
		row_state = (~new_state[row]) &
				 ((1 << kp->pdata->num_cols) - 1);

		if (hweight16(row_state) > 1) {
			if (found_first == -1)
				found_first = row;
			if (check & row_state) {
				dev_dbg(kp->dev, "detected ghost key on row[%d]"
						 "row[%d]\n", found_first, row);
				return 1;
			}
		}
		check |= row_state;
	}
	return 0;
}

static int pmic8058_kp_scan_matrix(struct pmic8058_kp *kp, unsigned int events)
{
	u16 new_state[PM8058_MAX_ROWS];
	u16 old_state[PM8058_MAX_ROWS];
	int rc;

	switch (events) {
	case 0x1:
		rc = pmic8058_kp_read_matrix(kp, new_state, NULL);
		if (pmic8058_detect_ghost_keys(kp, new_state))
			return -EINVAL;
		__pmic8058_kp_scan_matrix(kp, new_state, kp->keystate);
		memcpy(kp->keystate, new_state, sizeof(new_state));
	break;
	case 0x3: /* two events - eventcounter is gray-coded */
		rc = pmic8058_kp_read_matrix(kp, new_state, old_state);
		__pmic8058_kp_scan_matrix(kp, old_state, kp->keystate);
		__pmic8058_kp_scan_matrix(kp, new_state, old_state);
		memcpy(kp->keystate, new_state, sizeof(new_state));
	break;
	case 0x2:
		dev_dbg(kp->dev, "Some key events are missed\n");
		rc = pmic8058_kp_read_matrix(kp, new_state, old_state);
		__pmic8058_kp_scan_matrix(kp, old_state, kp->keystate);
		__pmic8058_kp_scan_matrix(kp, new_state, old_state);
		memcpy(kp->keystate, new_state, sizeof(new_state));
	break;
	default:
		rc = -1;
	}
	return rc;
}

static inline int pmic8058_kp_disabled(struct pmic8058_kp *kp)
{
	return kp->disable_depth != 0;
}

static void pmic8058_kp_enable(struct pmic8058_kp *kp)
{
	if (!pmic8058_kp_disabled(kp))
		return;

	if (--kp->disable_depth == 0) {

		kp->ctrl_reg |= KEYP_CTRL_KEYP_EN;
		pmic8058_kp_write_u8(kp, kp->ctrl_reg, KEYP_CTRL);

		enable_irq(kp->key_sense_irq);
		enable_irq(kp->key_stuck_irq);
	}
}

static void pmic8058_kp_disable(struct pmic8058_kp *kp)
{
	if (kp->disable_depth++ == 0) {
		disable_irq(kp->key_sense_irq);
		disable_irq(kp->key_stuck_irq);

		kp->ctrl_reg &= ~KEYP_CTRL_KEYP_EN;
		pmic8058_kp_write_u8(kp, kp->ctrl_reg, KEYP_CTRL);
	}
}

static ssize_t pmic8058_kp_disable_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct pmic8058_kp *kp = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", pmic8058_kp_disabled(kp));
}

static ssize_t pmic8058_kp_disable_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct pmic8058_kp *kp = dev_get_drvdata(dev);
	long i = 0;
	int rc;

	rc = strict_strtoul(buf, 10, &i);
	if (rc)
		return -EINVAL;

	i = !!i;

	mutex_lock(&kp->mutex);
	if (i == kp->user_disabled) {
		mutex_unlock(&kp->mutex);
		return count;
	}

	kp->user_disabled = i;

	if (i)
		pmic8058_kp_disable(kp);
	else
		pmic8058_kp_enable(kp);
	mutex_unlock(&kp->mutex);

	return count;
}

static DEVICE_ATTR(disable_kp, 0664, pmic8058_kp_disable_show,
			pmic8058_kp_disable_store);


/*L&T prshnt:- added a code for 1-5 application test*/
/* TODO:- Need to check-> 'mask' value */
#if defined CONFIG_MACH_CHIEF || defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || \
	defined(CONFIG_MACH_PREVAIL2)
int keyshort_test_status;
static ssize_t keyshort_test(struct device *dev, struct device_attribute *attr, char *buf)
{
    if(keyshort_test_status)
      return sprintf(buf,"PRESS\n");     
    else
      return sprintf(buf,"RELEASE\n");
}


#if defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || defined(CONFIG_MACH_PREVAIL2)
static enum hrtimer_restart left_key_timer_func(struct hrtimer *timer)
{
	chattering_key.left=0;
	chattering_key.left_value=0;
	printk("%s\n",__func__);
	return HRTIMER_NORESTART;
}

static enum hrtimer_restart right_key_timer_func(struct hrtimer *timer)
{
	chattering_key.right=0;
	chattering_key.right_value=0;
	printk("%s\n",__func__);
	return HRTIMER_NORESTART;
}
#endif

static DEVICE_ATTR(key, S_IRUGO | S_IWUSR , keyshort_test, NULL);
#endif

/*
 * NOTE: We are reading recent and old data registers blindly
 * whenever key-stuck interrupt happens, because events counter doesn't
 * get updated when this interrupt happens due to key stuck doesn't get
 * considered as key state change.
 *
 * We are not using old data register contents after they are being read
 * because it might report the key which was pressed before the key being stuck
 * as stuck key because it's pressed status is stored in the old data
 * register.
 */
static irqreturn_t pmic8058_kp_stuck_irq(int irq, void *data)
{
	u16 new_state[PM8058_MAX_ROWS];
	u16 old_state[PM8058_MAX_ROWS];
	int rc;
	struct pmic8058_kp *kp = data;

	rc = pmic8058_kp_read_matrix(kp, new_state, old_state);
	__pmic8058_kp_scan_matrix(kp, new_state, kp->stuckstate);

	return IRQ_HANDLED;
}

/*
 * NOTE: Any row multiple interrupt issue - PMIC4 Rev A0
 *
 * If the S/W responds to the key-event interrupt too early and reads the
 * recent data, the keypad FSM will mistakenly go to the IDLE state, instead
 * of the scan pause state as it is supposed too. Since the key is still
 * pressed, the keypad scanner will go through the debounce, scan, and generate
 * another key event interrupt. The workaround for this issue is to add delay
 * of 1ms between servicing the key event interrupt and reading the recent data.
 */
static irqreturn_t pmic8058_kp_irq(int irq, void *data)
{
	struct pmic8058_kp *kp = data;
	u8 ctrl_val, events;
	int rc;

	if (pm8058_rev(kp->pm_chip) == PM_8058_REV_1p0)
		mdelay(1);

	dev_dbg(kp->dev, "key sense irq\n");
	__dump_kp_regs(kp, "pmic8058_kp_irq");

	rc = pmic8058_kp_read(kp, &ctrl_val, KEYP_CTRL, 1);
	events = ctrl_val & KEYP_CTRL_EVNTS_MASK;

	rc = pmic8058_kp_scan_matrix(kp, events);

	return IRQ_HANDLED;
}

static irqreturn_t pmic8058_volume_up_irq(int irq, void *data)
{
    struct pmic8058_kp *kp = data;
    static int pre_up_key_state = -1;
    int key_state = 1;
    int code;

#ifdef TACT_SW_CHATTERING_DISABLED
    disable_irq_nosync(irq);
    msleep(10);
#endif

#if defined CONFIG_MACH_CHIEF
    if(!gpio_get_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_VOLUME_UPKEY)))
        key_state = 0;

    code = MATRIX_SCAN_CODE(1, 3, PM8058_ROW_SHIFT);
#elif defined(CONFIG_MACH_PREVAIL2)
    if(!gpio_get_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_VOLUME_UPKEY)))
        key_state = 0;

    code = MATRIX_SCAN_CODE(0, 2, PM8058_ROW_SHIFT);  
#elif  defined(CONFIG_MACH_VITAL2)
    if(!gpio_get_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_VOLUME_UPKEY)))
        key_state = 0;

    code = MATRIX_SCAN_CODE(11, 3, PM8058_ROW_SHIFT);  
#elif  defined (CONFIG_MACH_ROOKIE2)
if(!gpio_get_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_VOLUME_UPKEY)))
        key_state = 0;

    code = MATRIX_SCAN_CODE(2, 0, PM8058_ROW_SHIFT);  

#endif

    if(pre_up_key_state == key_state && pre_up_key_state != -1 && key_state == 0) {
        key_state = 1;
#ifndef PRODUCT_SHIP
        printk("%s : Fix key state\n", __func__);
#endif
    }

    pre_up_key_state = key_state;

#ifndef PRODUCT_SHIP
//    printk("%s : keycode [%d] %s\n", __func__, kp->keycodes[code], !key_state?"pressed":"released");
#endif

    input_event(kp->input, EV_MSC, MSC_SCAN, code);
    input_report_key(kp->input, kp->keycodes[code], !key_state);
    input_sync(kp->input);

#ifdef TACT_SW_CHATTERING_DISABLED
    enable_irq(irq);
#endif

    return IRQ_HANDLED;
}

static irqreturn_t pmic8058_volume_down_irq(int irq, void *data)
{
    struct pmic8058_kp *kp = data;
    static int pre_down_key_state = -1;
    int key_state = 1;
    int code;

#ifdef TACT_SW_CHATTERING_DISABLED
    disable_irq_nosync(irq);
    msleep(10);
#endif

#if defined CONFIG_MACH_CHIEF
    if(!gpio_get_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_VOLUME_DOWNKEY)))
        key_state = 0;

    code = MATRIX_SCAN_CODE(1, 4, PM8058_ROW_SHIFT);
#elif  defined(CONFIG_MACH_PREVAIL2)
    if(!gpio_get_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_VOLUME_DOWNKEY)))
        key_state = 0;

    code = MATRIX_SCAN_CODE(0, 3, PM8058_ROW_SHIFT);         
#elif  defined(CONFIG_MACH_VITAL2)
    if(!gpio_get_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_VOLUME_DOWNKEY)))
        key_state = 0;

    code = MATRIX_SCAN_CODE(11, 4, PM8058_ROW_SHIFT);
#elif   defined (CONFIG_MACH_ROOKIE2)
    if(!gpio_get_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_VOLUME_DOWNKEY)))
        key_state = 0;

    code = MATRIX_SCAN_CODE(2, 1, PM8058_ROW_SHIFT);
#endif

    if(pre_down_key_state == key_state && pre_down_key_state != -1 && key_state == 0) {
        key_state = 1;        
#ifndef PRODUCT_SHIP
        printk("%s : Fix key state\n", __func__);
#endif
    }

    pre_down_key_state = key_state;

#ifndef PRODUCT_SHIP	
//    printk("%s : keycode [%d] %s\n", __func__, kp->keycodes[code], !key_state?"pressed":"released");
#endif
    input_event(kp->input, EV_MSC, MSC_SCAN, code);
    input_report_key(kp->input, kp->keycodes[code], !key_state);
    input_sync(kp->input);

#ifdef TACT_SW_CHATTERING_DISABLED
    enable_irq(irq);
#endif

    return IRQ_HANDLED;
}

#if defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || defined(CONFIG_MACH_PREVAIL2)
static irqreturn_t hall_ic_irq(int irq, void *data)
{
   struct pmic8058_kp *kp = data;
   int hall_status = gpio_get_value(MSM_HALL_IC);

    if (system_rev >= 6) { //0 open, 1 close
      if(hall_status)
          input_report_switch(kp->input, SW_LID, !!hall_status);
      else
          input_report_switch(kp->input, SW_LID, !!hall_status);
    }
    else { //0 close, 1 open
      if(hall_status)
        input_report_switch(kp->input, SW_LID, !hall_status);
      else
        input_report_switch(kp->input, SW_LID, !hall_status);
    }

    input_sync(kp->input);

    if (system_rev >= 6)
      printk("[input_report_switch] slide_int on hall_ic_irq() - !gpio_hall_ic %s\n", hall_status==0?"OPEN":"CLOSE");
    else
      printk("[input_report_switch] slide_int on hall_ic_irq() - !gpio_hall_ic %s\n", hall_status==1?"OPEN":"CLOSE");
   
   return IRQ_HANDLED;
}
#endif
/*
 * NOTE: Last row multi-interrupt issue
 *
 * In PMIC Rev A0, if any key in the last row of the keypad matrix
 * is pressed and held, the H/W keeps on generating interrupts.
 * Software work-arounds it by programming the keypad controller next level
 * up rows (for 8x12 matrix it is 15 rows) so the keypad controller
 * thinks of more-rows than the actual ones, so the actual last-row
 * in the matrix won't generate multiple interrupts.
 */
static int pmic8058_kpd_init(struct pmic8058_kp *kp)
{
	int bits, rc, cycles;
	u8 scan_val = 0, ctrl_val = 0;
	static u8 row_bits[] = {
		0, 1, 2, 3, 4, 4, 5, 5, 6, 6, 6, 7, 7, 7,
	};

	/* Find column bits */
	if (kp->pdata->num_cols < KEYP_CTRL_SCAN_COLS_MIN)
		bits = 0;
	else
		bits = kp->pdata->num_cols - KEYP_CTRL_SCAN_COLS_MIN;
	ctrl_val = (bits & KEYP_CTRL_SCAN_COLS_BITS) <<
		KEYP_CTRL_SCAN_COLS_SHIFT;

	/* Find row bits */
	if (kp->pdata->num_rows < KEYP_CTRL_SCAN_ROWS_MIN)
		bits = 0;
	else if (kp->pdata->num_rows > PM8058_MAX_ROWS)
		bits = KEYP_CTRL_SCAN_ROWS_BITS;
	else
		bits = row_bits[kp->pdata->num_rows - KEYP_CTRL_SCAN_ROWS_MIN];

	/* Use max rows to fix last row problem if actual rows are less */
	if (kp->flags & KEYF_FIX_LAST_ROW &&
			 (kp->pdata->num_rows != PM8058_MAX_ROWS))
		bits = row_bits[kp->pdata->num_rows - KEYP_CTRL_SCAN_ROWS_MIN
					 + 1];

	ctrl_val |= (bits << KEYP_CTRL_SCAN_ROWS_SHIFT);

	rc = pmic8058_kp_write_u8(kp, ctrl_val, KEYP_CTRL);

	if (pm8058_rev(kp->pm_chip) == PM_8058_REV_1p0)
		bits = fls(kp->pdata->debounce_ms[0]) - 1;
	else
		bits = (kp->pdata->debounce_ms[1] / 5) - 1;

	scan_val |= (bits << KEYP_SCAN_DBOUNCE_SHIFT);

	bits = fls(kp->pdata->scan_delay_ms) - 1;
	scan_val |= (bits << KEYP_SCAN_PAUSE_SHIFT);

	/* Row hold time is a multiple of 32KHz cycles. */
	cycles = (kp->pdata->row_hold_ns * KEYP_CLOCK_FREQ) / NSEC_PER_SEC;

	scan_val |= (cycles << KEYP_SCAN_ROW_HOLD_SHIFT);

	rc = pmic8058_kp_write_u8(kp, scan_val, KEYP_SCAN);

	return rc;
}

static int pm8058_kp_config_drv(int gpio_start, int num_gpios)
{
	int	rc;
	struct pm8058_gpio kypd_drv = {
		.direction	= PM_GPIO_DIR_OUT,
		.output_buffer	= PM_GPIO_OUT_BUF_OPEN_DRAIN,
		.output_value	= 0,
		.pull		= PM_GPIO_PULL_NO,
		.vin_sel	= 2,
		.out_strength	= PM_GPIO_STRENGTH_LOW,
		.function	= PM_GPIO_FUNC_1,
		.inv_int_pol	= 1,
	};

	if (gpio_start < 0 || num_gpios < 0 || num_gpios > PM8058_GPIOS)
		return -EINVAL;

	while (num_gpios--) {
		rc = pm8058_gpio_config(gpio_start++, &kypd_drv);
		if (rc) {
			pr_err("%s: FAIL pm8058_gpio_config(): rc=%d.\n",
				__func__, rc);
			return rc;
		}
	}

	return 0;
}

static int pm8058_kp_config_sns(int gpio_start, int num_gpios)
{
	int	rc;
	struct pm8058_gpio kypd_sns = {
		.direction	= PM_GPIO_DIR_IN,
		.pull		= PM_GPIO_PULL_UP_31P5,
		.vin_sel	= 2,
		.out_strength	= PM_GPIO_STRENGTH_NO,
		.function	= PM_GPIO_FUNC_NORMAL,
		.inv_int_pol	= 1,
	};

	if (gpio_start < 0 || num_gpios < 0 || num_gpios > PM8058_GPIOS)
		return -EINVAL;

	while (num_gpios--) {
		rc = pm8058_gpio_config(gpio_start++, &kypd_sns);
		if (rc) {
			pr_err("%s: FAIL pm8058_gpio_config(): rc=%d.\n",
				__func__, rc);
			return rc;
		}
	}

	return 0;
}

/*
 * keypad controller should be initialized in the following sequence
 * only, otherwise it might get into FSM stuck state.
 *
 * - Initialize keypad control parameters, like no. of rows, columns,
 *   timing values etc.,
 * - configure rows and column gpios pull up/down.
 * - set irq edge type.
 * - enable the keypad controller.
 */
static int __devinit pmic8058_kp_probe(struct platform_device *pdev)
{
	struct pmic8058_keypad_data *pdata = pdev->dev.platform_data;
	const struct matrix_keymap_data *keymap_data;
	struct pmic8058_kp *kp;
	int rc;
	unsigned short *keycodes;
	u8 ctrl_val;
	struct pm8058_chip	*pm_chip;
#if defined CONFIG_MACH_CHIEF || defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2)  || \
	defined(CONFIG_MACH_PREVAIL2)/*L&T */
	struct class *key_class;
       struct device *keypress;
#endif

	pm_chip = platform_get_drvdata(pdev);
	if (pm_chip == NULL) {
		dev_err(&pdev->dev, "no parent data passed in\n");
		return -EFAULT;
	}

#if 0  //temp kjb
	if (!pdata || !pdata->num_cols || !pdata->num_rows ||
		pdata->num_cols > PM8058_MAX_COLS ||
		pdata->num_rows > PM8058_MAX_ROWS ||
		pdata->num_cols < PM8058_MIN_COLS ||
		pdata->num_rows < PM8058_MIN_ROWS) {
		dev_err(&pdev->dev, "invalid platform data\n");
		return -EINVAL;
	}


	if (pdata->rows_gpio_start < 0 || pdata->cols_gpio_start < 0) {
		dev_err(&pdev->dev, "invalid gpio_start platform data\n");
		return -EINVAL;
	}
#endif
	if (!pdata->scan_delay_ms || pdata->scan_delay_ms > MAX_SCAN_DELAY
		|| pdata->scan_delay_ms < MIN_SCAN_DELAY ||
		!is_power_of_2(pdata->scan_delay_ms)) {
		dev_err(&pdev->dev, "invalid keypad scan time supplied\n");
		return -EINVAL;
	}

	if (!pdata->row_hold_ns || pdata->row_hold_ns > MAX_ROW_HOLD_DELAY
		|| pdata->row_hold_ns < MIN_ROW_HOLD_DELAY ||
		((pdata->row_hold_ns % MIN_ROW_HOLD_DELAY) != 0)) {
		dev_err(&pdev->dev, "invalid keypad row hold time supplied\n");
		return -EINVAL;
	}

	if (pm8058_rev(pm_chip) == PM_8058_REV_1p0) {
		if (!pdata->debounce_ms
			|| !is_power_of_2(pdata->debounce_ms[0])
			|| pdata->debounce_ms[0] > MAX_DEBOUNCE_A0_TIME
			|| pdata->debounce_ms[0] < MIN_DEBOUNCE_A0_TIME) {
			dev_err(&pdev->dev, "invalid debounce time supplied\n");
			return -EINVAL;
		}
	} else {
		if (!pdata->debounce_ms
			|| ((pdata->debounce_ms[1] % 5) != 0)
			|| pdata->debounce_ms[1] > MAX_DEBOUNCE_B0_TIME
			|| pdata->debounce_ms[1] < MIN_DEBOUNCE_B0_TIME) {
			dev_err(&pdev->dev, "invalid debounce time supplied\n");
			return -EINVAL;
		}
	}

	keymap_data = pdata->keymap_data;
	if (!keymap_data) {
		dev_err(&pdev->dev, "no keymap data supplied\n");
		return -EINVAL;
	}

	kp = kzalloc(sizeof(*kp), GFP_KERNEL);

	if (!kp)
		return -ENOMEM;

	keycodes = kzalloc(PM8058_MATRIX_MAX_SIZE * sizeof(*keycodes),
				 GFP_KERNEL);
	if (!keycodes) {
		rc = -ENOMEM;
		goto err_alloc_mem;
	}

	platform_set_drvdata(pdev, kp);
	mutex_init(&kp->mutex);

	kp->pdata	= pdata;
	kp->dev		= &pdev->dev;
	kp->keycodes	= keycodes;
	kp->pm_chip	= pm_chip;

	if (pm8058_rev(pm_chip) == PM_8058_REV_1p0)
		kp->flags |= KEYF_FIX_LAST_ROW;

	kp->input = input_allocate_device();
	if (!kp->input) {
		dev_err(&pdev->dev, "unable to allocate input device\n");
		rc = -ENOMEM;
		goto err_alloc_device;
	}

	/* Enable runtime PM ops, start in ACTIVE mode */
	rc = pm_runtime_set_active(&pdev->dev);
	if (rc < 0)
		dev_dbg(&pdev->dev, "unable to set runtime pm state\n");
	pm_runtime_enable(&pdev->dev);

	kp->key_sense_irq = platform_get_irq(pdev, 0);
	if (kp->key_sense_irq < 0) {
		dev_err(&pdev->dev, "unable to get keypad sense irq\n");
		rc = -ENXIO;
		goto err_get_irq;
	}

	kp->key_stuck_irq = platform_get_irq(pdev, 1);
	if (kp->key_stuck_irq < 0) {
		dev_err(&pdev->dev, "unable to get keypad stuck irq\n");
		rc = -ENXIO;
		goto err_get_irq;
	}

	if (pdata->input_name)
		kp->input->name = pdata->input_name;
	else
		kp->input->name = "PMIC8058 keypad";

	if (pdata->input_phys_device)
		kp->input->phys = pdata->input_phys_device;
	else
		kp->input->phys = "pmic8058_keypad/input0";

	kp->input->dev.parent	= &pdev->dev;

	kp->input->id.bustype	= BUS_HOST;
	kp->input->id.version	= 0x0001;
	kp->input->id.product	= 0x0001;
	kp->input->id.vendor	= 0x0001;

	kp->input->evbit[0]	= BIT_MASK(EV_KEY);

	if (pdata->rep)
		__set_bit(EV_REP, kp->input->evbit);

	kp->input->keycode	= keycodes;
	kp->input->keycodemax	= PM8058_MATRIX_MAX_SIZE;
	kp->input->keycodesize	= sizeof(*keycodes);

	matrix_keypad_build_keymap(keymap_data, PM8058_ROW_SHIFT,
					kp->input->keycode, kp->input->keybit);

	input_set_capability(kp->input, EV_MSC, MSC_SCAN);
	input_set_drvdata(kp->input, kp);

	rc = input_register_device(kp->input);
	if (rc < 0) {
		dev_err(&pdev->dev, "unable to register keypad input device\n");
		goto err_get_irq;
	}

	/* initialize keypad state */
	memset(kp->keystate, 0xff, sizeof(kp->keystate));
	memset(kp->stuckstate, 0xff, sizeof(kp->stuckstate));

	rc = pmic8058_kpd_init(kp);
	if (rc < 0) {
		dev_err(&pdev->dev, "unable to initialize keypad controller\n");
		goto err_kpd_init;
	}

	rc = pm8058_kp_config_sns(pdata->cols_gpio_start,
			pdata->num_cols);
	if (rc < 0) {
		dev_err(&pdev->dev, "unable to configure keypad sense lines\n");
		goto err_gpio_config;
	}

	rc = pm8058_kp_config_drv(pdata->rows_gpio_start,
			pdata->num_rows);
	if (rc < 0) {
		dev_err(&pdev->dev, "unable to configure keypad drive lines\n");
		goto err_gpio_config;
	}

	#if defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || defined(CONFIG_MACH_PREVAIL2)
	hrtimer_init(&left_key_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	left_key_timer.function = left_key_timer_func;

	hrtimer_init(&right_key_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	right_key_timer.function = right_key_timer_func;
	
	#endif
	rc = request_threaded_irq(kp->key_sense_irq, NULL, pmic8058_kp_irq,
				 IRQF_TRIGGER_RISING, "pmic-keypad", kp);
	if (rc < 0) {
		dev_err(&pdev->dev, "failed to request keypad sense irq\n");
		goto err_req_sense_irq;
	}

#if defined CONFIG_MACH_CHIEF || defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || \
	defined(CONFIG_MACH_PREVAIL2)
	rc = request_threaded_irq( MSM_GPIO_KEY_VOLUP_IRQ , 
			NULL, pmic8058_volume_up_irq, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "vol_up", kp);

	if (rc < 0) {
		dev_err(&pdev->dev, "failed to request vol_up irq\n");
		goto err_req_sense_irq;
	}

      rc = request_threaded_irq( MSM_GPIO_KEY_VOLDOWN_IRQ , 
             NULL, pmic8058_volume_down_irq, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "vol_down", kp);

      if (rc < 0) {
          dev_err(&pdev->dev, "failed to request vol_down irq\n");
          goto err_req_sense_irq;
      }
#endif	

#if defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || defined(CONFIG_MACH_PREVAIL2)
    if (system_rev >= 6) {
      gpio_tlmm_config(GPIO_CFG(MSM_HALL_IC, 1, GPIO_CFG_INPUT,
     					GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    }
    else {
      gpio_tlmm_config(GPIO_CFG(MSM_HALL_IC, 0, GPIO_CFG_INPUT,
     					GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    }

    input_set_capability(kp->input, EV_SW, SW_LID);
    
    if(gpio_get_value(MSM_HALL_IC)) {
      if (system_rev >= 6)
        input_report_switch(kp->input, SW_LID, 1); //0 open, 1 close
      else
        input_report_switch(kp->input, SW_LID, 0); //0 close, 1 open
    }
    else {
      if (system_rev >= 6)
        input_report_switch(kp->input, SW_LID, 0);
      else
        input_report_switch(kp->input, SW_LID, 1);
    }

    input_sync(kp->input);

    if (system_rev >= 6)
      printk("[input_report_switch] slide_int - !gpio_hall_ic %s\n", gpio_get_value(MSM_HALL_IC)==0?"OPEN":"CLOSE");
    else
      printk("[input_report_switch] slide_int - !gpio_hall_ic %s\n", gpio_get_value(MSM_HALL_IC)==1?"OPEN":"CLOSE");

    rc = request_threaded_irq(MSM_GPIO_TO_INT(MSM_HALL_IC) , 
         NULL, hall_ic_irq, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "hall_ic", kp);

    if (rc < 0) {
      dev_err(&pdev->dev, "failed to request hall_ic irq\n");
      goto err_hall_ic_irq;
    }

#endif

	rc = request_threaded_irq(kp->key_stuck_irq, NULL,
				 pmic8058_kp_stuck_irq, IRQF_TRIGGER_RISING,
				 "pmic-keypad-stuck", kp);
	if (rc < 0) {
		dev_err(&pdev->dev, "failed to request keypad stuck irq\n");
		goto err_req_stuck_irq;
	}

	rc = pmic8058_kp_read_u8(kp, &ctrl_val, KEYP_CTRL);
	ctrl_val |= KEYP_CTRL_KEYP_EN;
	rc = pmic8058_kp_write_u8(kp, ctrl_val, KEYP_CTRL);

	kp->ctrl_reg = ctrl_val;

	__dump_kp_regs(kp, "probe");

	rc = device_create_file(&pdev->dev, &dev_attr_disable_kp);
	if (rc < 0)
		goto err_create_file;

/* L&T :-
 * for 1-5 application 
 *   /sys/devices/virtual/key/key/key 
 */
#if defined CONFIG_MACH_CHIEF || defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || \
	defined(CONFIG_MACH_PREVAIL2)
	key_class = class_create(THIS_MODULE,"key");
	if(IS_ERR(key_class)){
		printk("failed to keyclass\n");
                goto err_create_file;
	}

	keypress = device_create(key_class, NULL, 0 , NULL, "key");
	if(IS_ERR(keypress))	
		printk("failed to create a keypress device\n");
	else{
		if(device_create_file(keypress, &dev_attr_key)<0)
			printk("failed to create device file key");	
	}
#endif
	
	device_init_wakeup(&pdev->dev, pdata->wakeup);

#ifdef CONFIG_HAS_EARLYSUSPEND

extern int charging_boot;

#if defined CONFIG_MACH_CHIEF
	if(system_rev >= 6 && charging_boot == 0)
		is_early_suspend = true;
	else
#endif
#if defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || defined(CONFIG_MACH_PREVAIL2)
	if(system_rev >= 2 && charging_boot == 0)
		is_early_suspend = true;
	else
#endif
		is_early_suspend = false;

	printk("pmic8058_kp_probe - system_rev : %d, is_early_suspend : %d\n", system_rev, is_early_suspend);

	if(is_early_suspend == true) {
		kp->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
		kp->early_suspend.suspend = pmic8058_kp_early_suspend;
		kp->early_suspend.resume = pmic8058_kp_late_resume;
		register_early_suspend(&kp->early_suspend);
	}
#endif

	return 0;

err_create_file:
	free_irq(kp->key_stuck_irq, NULL);
err_req_stuck_irq:
	free_irq(kp->key_sense_irq, NULL);
err_hall_ic_irq:
err_req_sense_irq:
err_gpio_config:
err_kpd_init:
	input_unregister_device(kp->input);
	kp->input = NULL;
err_get_irq:
	pm_runtime_set_suspended(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	input_free_device(kp->input);
err_alloc_device:
	kfree(keycodes);
err_alloc_mem:
	kfree(kp);
	return rc;
}

static int __devexit pmic8058_kp_remove(struct platform_device *pdev)
{
	struct pmic8058_kp *kp = platform_get_drvdata(pdev);

	pm_runtime_set_suspended(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	device_remove_file(&pdev->dev, &dev_attr_disable_kp);
	device_init_wakeup(&pdev->dev, 0);
	free_irq(kp->key_stuck_irq, NULL);
	free_irq(kp->key_sense_irq, NULL);
	input_unregister_device(kp->input);
	platform_set_drvdata(pdev, NULL);
	kfree(kp->input->keycode);
	kfree(kp);

	return 0;
}

#ifdef CONFIG_PM
static int pmic8058_kp_suspend(struct device *dev)
{
	struct pmic8058_kp *kp = dev_get_drvdata(dev);
#if 0
	if (device_may_wakeup(dev) && !pmic8058_kp_disabled(kp)) {
		printk("pmic8058_kp_suspend, but enable_irq_wake!\n");
		enable_irq_wake(kp->key_sense_irq);
	} else
#endif
	{
		mutex_lock(&kp->mutex);
		pmic8058_kp_disable(kp);
#if defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || defined(CONFIG_MACH_PREVAIL2)
		enable_irq_wake(MSM_GPIO_TO_INT(MSM_HALL_IC)); /* to wakeup in case of sleep */
#endif
		if(get_voice_call_status()==1 && kp->volume_key_wake_enabled == 0) {
			enable_irq_wake(MSM_GPIO_KEY_VOLUP_IRQ);
			enable_irq_wake(MSM_GPIO_KEY_VOLDOWN_IRQ);
			kp->volume_key_wake_enabled = 1;
			pr_info("\t\t@#@#@#@#[%s] volume key : irq wake enabled\n", __func__);
		} else if(!get_voice_call_status() && kp->volume_key_wake_enabled) {
			disable_irq_wake(MSM_GPIO_KEY_VOLUP_IRQ);
			disable_irq_wake(MSM_GPIO_KEY_VOLDOWN_IRQ);
			kp->volume_key_wake_enabled = 0;
			pr_info("\t\t@#@#@#@#[%s] volume key : irq wake disabled\n", __func__);
		}
		mutex_unlock(&kp->mutex);
	}

	return 0;
}

static int pmic8058_kp_resume(struct device *dev)
{
	struct pmic8058_kp *kp = dev_get_drvdata(dev);
#if 0
	if (device_may_wakeup(dev) && !pmic8058_kp_disabled(kp)) {
		printk("pmic8058_kp_resume, but disable_irq_wake!\n");
		disable_irq_wake(kp->key_sense_irq);
	} else
#endif
	{
		mutex_lock(&kp->mutex);
#if defined CONFIG_MACH_VITAL2 || defined (CONFIG_MACH_ROOKIE2) || defined(CONFIG_MACH_PREVAIL2)
		disable_irq_wake(MSM_GPIO_TO_INT(MSM_HALL_IC)); /* to match irq pair */
#endif
		pmic8058_kp_enable(kp);

		if(kp->volume_key_wake_enabled) {
			disable_irq_wake(MSM_GPIO_KEY_VOLUP_IRQ);
			disable_irq_wake(MSM_GPIO_KEY_VOLDOWN_IRQ);
			kp->volume_key_wake_enabled = 0;		
			pr_info("\t\t@#@#@#@#[%s] volume key : irq wake disabled\n", __func__);
		}
		mutex_unlock(&kp->mutex);
	}

	return 0;
}

static struct dev_pm_ops pm8058_kp_pm_ops = {
	.suspend	= pmic8058_kp_suspend,
	.resume	= pmic8058_kp_resume,
};
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
void pmic8058_kp_early_suspend(struct early_suspend *h)
{
	struct pmic8058_kp *kp = container_of(h, struct pmic8058_kp, early_suspend);
	//printk("pmic8058_kp_early_suspend\n");
	if(!kp->dev)
		printk(KERN_INFO "pmic8058_kp_early_suspend is blocked\n");
	else
		pmic8058_kp_suspend(kp->dev);
}

void pmic8058_kp_late_resume(struct early_suspend *h)
{
	struct pmic8058_kp *kp = container_of(h, struct pmic8058_kp, early_suspend);
	//printk("pmic8058_kp_late_resume\n");
	if(!kp->dev)
		printk(KERN_INFO "pmic8058_kp_late_resume is blocked\n");
	else
		pmic8058_kp_resume(kp->dev);
}
#endif


static struct platform_driver pmic8058_kp_driver = {
	.probe		= pmic8058_kp_probe,
	.remove		= __devexit_p(pmic8058_kp_remove),
	.driver		= {
		.name = "pm8058-keypad",
		.owner = THIS_MODULE,
#ifndef CONFIG_HAS_EARLYSUSPEND
#ifdef CONFIG_PM
		.pm = &pm8058_kp_pm_ops,
#endif
#endif
	},
};

static int __init pmic8058_kp_init(void)
{
	return platform_driver_register(&pmic8058_kp_driver);
}
module_init(pmic8058_kp_init);

static void __exit pmic8058_kp_exit(void)
{
	platform_driver_unregister(&pmic8058_kp_driver);
}
module_exit(pmic8058_kp_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("PMIC8058 keypad driver");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:pmic8058_keypad");
