/* arch/arm/mach-msm/vreg.c
 *
 * Copyright (C) 2008 Google, Inc.
 * Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 * Author: Brian Swetland <swetland@google.com>
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

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/string.h>
#include <mach/vreg.h>

#include "proc_comm.h"

#define _DEBUG_VREG_

#if defined(CONFIG_MSM_VREG_SWITCH_INVERTED)
#define VREG_SWITCH_ENABLE 0
#define VREG_SWITCH_DISABLE 1
#else
#define VREG_SWITCH_ENABLE 1
#define VREG_SWITCH_DISABLE 0
#endif

struct vreg {
	const char *name;
	unsigned id;
	int status;
	unsigned refcnt;
#ifdef _DEBUG_VREG_
	int mv;
#endif

};

#define VREG(_name, _id, _status, _refcnt) \
	{ .name = _name, .id = _id, .status = _status, .refcnt = _refcnt }

static struct vreg vregs[] = {
	VREG("msma",	0, 0, 0),
	VREG("msmp",	1, 0, 0),
	VREG("msme1",	2, 0, 0),
	VREG("msmc1",	3, 0, 0),
	VREG("msmc2",	4, 0, 0),
	VREG("gp3",	5, 0, 0),
	VREG("msme2",	6, 0, 0),
	VREG("gp4",	7, 0, 0),
	VREG("gp1",	8, 0, 0),
	VREG("tcxo",	9, 0, 0),
	VREG("pa",	10, 0, 0),
	VREG("rftx",	11, 0, 0),
	VREG("rfrx1",	12, 0, 0),
	VREG("rfrx2",	13, 0, 0),
	VREG("synt",	14, 0, 0),
	VREG("wlan",	15, 0, 0),
	VREG("usb",	16, 0, 0),
	VREG("boost",	17, 0, 0),
	VREG("mmc",	18, 0, 0),
	VREG("ruim",	19, 0, 0),
	VREG("msmc0",	20, 0, 0),
	VREG("gp2",	21, 0, 0),
	VREG("gp5",	22, 0, 0),
	VREG("gp6",	23, 0, 0),
	VREG("rf",	24, 0, 0),
	VREG("rf_vco",	26, 0, 0),
	VREG("mpll",	27, 0, 0),
	VREG("s2",	28, 0, 0),
	VREG("s3",	29, 0, 0),
	VREG("rfubm",	30, 0, 0),
	VREG("ncp",	31, 0, 0),
	VREG("gp7",	32, 0, 0),
	VREG("gp8",	33, 0, 0),
	VREG("gp9",	34, 0, 0),
	VREG("gp10",	35, 0, 0),
	VREG("gp11",	36, 0, 0),
	VREG("gp12",	37, 0, 0),
	VREG("gp13",	38, 0, 0),
	VREG("gp14",	39, 0, 0),
	VREG("gp15",	40, 0, 0),
	VREG("gp16",	41, 0, 0),
	VREG("gp17",	42, 0, 0),
	VREG("s4",	43, 0, 0),
	VREG("usb2",	44, 0, 0),
	VREG("wlan2",	45, 0, 0),
	VREG("xo_out",	46, 0, 0),
	VREG("lvsw0",	47, 0, 0),
	VREG("lvsw1",	48, 0, 0),
};

struct vreg *vreg_get(struct device *dev, const char *id)
{
	int n;
	for (n = 0; n < ARRAY_SIZE(vregs); n++) {
		if (!strcmp(vregs[n].name, id))
			return vregs + n;
	}
	return ERR_PTR(-ENOENT);
}
EXPORT_SYMBOL(vreg_get);

unsigned vreg_get_refcnt(struct device *dev, const char *id)
{
	int n;
	for (n = 0; n < ARRAY_SIZE(vregs); n++) {
		if (!strcmp(vregs[n].name, id))
			return vregs[n].refcnt;
	}
	return 0;
}
EXPORT_SYMBOL(vreg_get_refcnt);

void vreg_put(struct vreg *vreg)
{
}

int vreg_enable(struct vreg *vreg)
{
	unsigned id = vreg->id;
	int enable = VREG_SWITCH_ENABLE;

	if (vreg->refcnt == 0)
		vreg->status = msm_proc_comm(PCOM_VREG_SWITCH, &id, &enable);

	if ((vreg->refcnt < UINT_MAX) && (!vreg->status))
		vreg->refcnt++;

	return vreg->status;
}
EXPORT_SYMBOL(vreg_enable);

int vreg_disable(struct vreg *vreg)
{
	unsigned id = vreg->id;
	int disable = VREG_SWITCH_DISABLE;

	if (!vreg->refcnt)
		return 0;

	if (vreg->refcnt == 1)
		vreg->status = msm_proc_comm(PCOM_VREG_SWITCH, &id, &disable);

	if (!vreg->status)
		vreg->refcnt--;

	return vreg->status;
}
EXPORT_SYMBOL(vreg_disable);

#ifdef _DEBUG_VREG_
const char *pm8058_pad_name[50] = {
	" ",		//VREG("msma",	0, 0, 0),  PM_VREG_INVALID_ID
	" ",		//VREG("msmp",	1, 0, 0),  PM_VREG_INVALID_ID
	" ",		//VREG("msme1",	2, 0, 0),  PM_VREG_INVALID_ID
	"S0",		//VREG("msmc1",	3, 0, 0),  PM_VREG_MSMC1_ID
	"S1",		//VREG("msmc2",	4, 0, 0),  PM_VREG_MSMC2_ID
	"L0",		//VREG("gp3",	5, 0, 0),  PM_VREG_GP3_ID
	" ",		//VREG("msme2",	6, 0, 0),  PM_VREG_INVALID_ID
	"L10",		//VREG("gp4",	7, 0, 0),  PM_VREG_GP4_ID
	"L9",		//VREG("gp1",	8, 0, 0),  PM_VREG_GP1_ID
	"L4",		//VREG("tcxo",	9, 0, 0),  PM_VREG_TCXO_ID
	" ",		//VREG("pa",	10, 0, 0), PM_VREG_INVALID_ID
	" ",		//VREG("rftx",	11, 0, 0), PM_VREG_INVALID_ID
	" ",		//VREG("rfrx1",	12, 0, 0), PM_VREG_INVALID_ID
	" ",		//VREG("rfrx2",	13, 0, 0), PM_VREG_INVALID_ID
	" ",		//VREG("synt",	14, 0, 0), PM_VREG_INVALID_ID
	"L13",		//VREG("wlan",	15, 0, 0), PM_VREG_WLAN1_ID
	"L6",		//VREG("usb",	16, 0, 0), PM_VREG_USB3P3_ID
	" ",		//VREG("boost",	17, 0, 0), PM_VREG_INVALID_ID
	"L5",		//VREG("mmc",	18, 0, 0), PM_VREG_SDCC1_ID
	"L3",		//VREG("ruim",	19, 0, 0), PM_VREG_USIM_ID
	" ",		//VREG("msmc0",	20, 0, 0), PM_VREG_INVALID_ID
	"L11",		//VREG("gp2",	21, 0, 0), PM_VREG_GP2_ID
	"L23",		//VREG("gp5",	22, 0, 0), PM_VREG_GP5_ID
	"L15",		//VREG("gp6",	23, 0, 0), PM_VREG_GP6_ID
	" ",		//VREG("rf",	24, 0, 0), PM_VREG_RF_ID
	" ", 
	" ",		//VREG("rf_vco",26, 0, 0), PM_VREG_INVALID_ID
	" ",		//VREG("mpll",	27, 0, 0), PM_VREG_INVALID_ID
	"S2",		//VREG("s2",	28, 0, 0), PM_VREG_RF1_ID
	"S3",		//VREG("s3",	29, 0, 0), PM_VREG_MSME_ID
	" ",		//VREG("rfubm",	30, 0, 0), PM_VREG_INVALID_ID
	" ",		//VREG("ncp",	31, 0, 0), PM_VREG_NCP_ID
	"L8",		//VREG("gp7",	32, 0, 0), PM_VREG_GP7_ID
	"L1",		//VREG("gp8",	33, 0, 0), PM_VREG_GP8_ID
	"L12",		//VREG("gp9",	34, 0, 0), PM_VREG_GP9_ID
	"L16",		//VREG("gp10",	35, 0, 0), PM_VREG_GP10_ID
	"L17",		//VREG("gp11",	36, 0, 0), PM_VREG_GP11_ID
	"L18",		//VREG("gp12",	37, 0, 0), PM_VREG_GP12_ID
	"L20",		//VREG("gp13",	38, 0, 0), PM_VREG_GP13_ID
	"L21",		//VREG("gp14",	39, 0, 0), PM_VREG_GP14_ID
	"L22",		//VREG("gp15",	40, 0, 0), PM_VREG_GP15_ID
	"L24",		//VREG("gp16",	41, 0, 0), PM_VREG_GP16_ID
	"L25",		//VREG("gp17",	42, 0, 0), PM_VREG_GP17_ID
	"S4",		//VREG("s4",	43, 0, 0), PM_VREG_RF2_ID
	"L7",		//VREG("usb2",	44, 0, 0), PM_VREG_USB1P8_ID
	"L19",		//VREG("wlan2",	45, 0, 0), PM_VREG_WLAN2_ID
	"L2",		//VREG("xo_out",46, 0, 0), PM_VREG_XO_OUT_D0_ID
	"LVS0",		//VREG("lvsw0",	47, 0, 0), PM_VREG_LVSW0_ID
	"LVS1",		//VREG("lvsw1",	48, 0, 0), PM_VREG_LVSW1_ID
};

int is_enabled_lcd(void)
{
	return vregs[23].refcnt;
}
EXPORT_SYMBOL(is_enabled_lcd);

int vreg_print_info(const char *what)
{
	int n;
	int type;
	
	if(what == NULL) return -1;

	if(strncmp(what, "all", 3) == 0) type = 0;
	else if(strncmp(what, "enabled", 7) == 0) type = 1;
	else if(strncmp(what, "disabled", 8) == 0) type = 2;
	else return -1;
	
	printk(KERN_ERR "\nVREG: %8s %5s %5s %3s %6s %6s\n", "name", "pad", "mV", "id", "status", "refcnt");
	for (n = 0; n < ARRAY_SIZE(vregs); n++) {
		if(type == 0 || ((type == 1 && vregs[n].refcnt > 0) || (type == 2 && vregs[n].refcnt == 0)))
			printk(KERN_ERR "VREG: %8s %5s %5d %3d %6d %6d\n", vregs[n].name, pm8058_pad_name[vregs[n].id], vregs[n].mv,
				vregs[n].id, vregs[n].status, vregs[n].refcnt);
	}
	printk(KERN_ERR "\n");
	
	return 0;
}
#else
int vreg_print_info(const char *what)
{
	return 0;
}
#endif

EXPORT_SYMBOL(vreg_print_info);

int vreg_set_level(struct vreg *vreg, unsigned mv)
{
	unsigned id = vreg->id;
#ifdef _DEBUG_VREG_
	vreg->mv = mv;
#endif
	vreg->status = msm_proc_comm(PCOM_VREG_SET_LEVEL, &id, &mv);
	return vreg->status;
}
EXPORT_SYMBOL(vreg_set_level);

#if defined(CONFIG_DEBUG_FS)

static int vreg_debug_set(void *data, u64 val)
{
	struct vreg *vreg = data;
	switch (val) {
	case 0:
		vreg_disable(vreg);
		break;
	case 1:
		vreg_enable(vreg);
		break;
	default:
		vreg_set_level(vreg, val);
		break;
	}
	return 0;
}

static int vreg_debug_get(void *data, u64 *val)
{
	struct vreg *vreg = data;

	if (!vreg->status)
		*val = 0;
	else
		*val = 1;

	return 0;
}

static int vreg_debug_count_set(void *data, u64 val)
{
	struct vreg *vreg = data;
	if (val > UINT_MAX)
		val = UINT_MAX;
	vreg->refcnt = val;
	return 0;
}

static int vreg_debug_count_get(void *data, u64 *val)
{
	struct vreg *vreg = data;

	*val = vreg->refcnt;

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(vreg_fops, vreg_debug_get, vreg_debug_set, "%llu\n");
DEFINE_SIMPLE_ATTRIBUTE(vreg_count_fops, vreg_debug_count_get,
			vreg_debug_count_set, "%llu\n");

static int __init vreg_debug_init(void)
{
	struct dentry *dent;
	int n;
	char name[32];
	const char *refcnt_name = "_refcnt";

	dent = debugfs_create_dir("vreg", 0);
	if (IS_ERR(dent))
		return 0;

	for (n = 0; n < ARRAY_SIZE(vregs); n++) {
		(void) debugfs_create_file(vregs[n].name, 0644,
					   dent, vregs + n, &vreg_fops);

		strlcpy(name, vregs[n].name, sizeof(name));
		strlcat(name, refcnt_name, sizeof(name));
		(void) debugfs_create_file(name, 0644,
					   dent, vregs + n, &vreg_count_fops);
	}

	return 0;
}

device_initcall(vreg_debug_init);
#endif
