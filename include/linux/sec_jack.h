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

#ifndef __ASM_ARCH_SEC_HEADSET_H
#define __ASM_ARCH_SEC_HEADSET_H

#ifdef __KERNEL__


/***MSM GPIO***/
#ifdef CONFIG_MACH_CHIEF
#define MSM_GPIO_EAR_DET		((system_rev>=6)?142:160)
#define MSM_GPIO_MICBIAS_EN		((system_rev>=6)?160:142)

#elif CONFIG_MACH_VITAL2
#define MSM_GPIO_EAR_DET		((system_rev>=2)?142:160)
#define MSM_GPIO_MICBIAS_EN		((system_rev>=2)?160:142)
#endif

#define MSM_GPIO_EAR_MICBIAS_EN		173
#define MSM_GPIO_EAR_SEND_END		44


#endif

#endif
