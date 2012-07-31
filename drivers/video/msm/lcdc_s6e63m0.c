/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora Forum nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * Alternatively, provided that this notice is retained in full, this software
 * may be relicensed by the recipient under the terms of the GNU General Public
 * License version 2 ("GPL") and only version 2, in which case the provisions of
 * the GPL apply INSTEAD OF those given above.  If the recipient relicenses the
 * software under the GPL, then the identification text in the MODULE_LICENSE
 * macro must be changed to reflect "GPLv2" instead of "Dual BSD/GPL".  Once a
 * recipient changes the license terms to the GPL, subsequent recipients shall
 * not relicense under alternate licensing terms, including the BSD or dual
 * BSD/GPL terms.  In addition, the following license statement immediately
 * below and between the words START and END shall also then apply when this
 * software is relicensed under the GPL:
 *
 * START
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 and only version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/delay.h>
#include <mach/gpio.h>
#include "msm_fb.h"

#define LCDC_DEBUG

#ifdef LCDC_DEBUG
#define DPRINT(x...)	printk("s6e63m0 " x)
#else
#define DPRINT(x...)	
#endif

/*
 * Serial Interface
 */
#define LCD_CSX_HIGH	gpio_set_value(spi_cs, 1);
#define LCD_CSX_LOW	gpio_set_value(spi_cs, 0);

#define LCD_SCL_HIGH	gpio_set_value(spi_sclk, 1);
#define LCD_SCL_LOW		gpio_set_value(spi_sclk, 0);

#define LCD_SDI_HIGH	gpio_set_value(spi_sdi, 1);
#define LCD_SDI_LOW		gpio_set_value(spi_sdi, 0);

#define DEFAULT_USLEEP	1	

struct setting_table {
	unsigned char command;	
	unsigned char parameters;
	unsigned char parameter[32];
	long wait;
};

static struct setting_table power_on_sequence[] = {
	// Panel Condition Set
	{ 0xF8,	14, 
	  	{ 0x01, 0x27, 0x27, 0x07, 0x07, 0x54, 0x9F, 0x63, 0x86, 0x1A, 0x33, 0x0D, 0x00, 0x00, 0x00, 0x00, 
		   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	0 },

	// Display Condition Set
	{ 0xF2,	5, 
		{ 0x02, 0x03, 0x1C, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	0 },
	{ 0xF7,	3, 
		{ 0x03, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	0 },		// If you change first parameter 0x00 to 0x03, it will invert display //

	// Gamma Condition Set. This setting is 300cd
	{ 0xFA,	22, 
		{ 0x00, 0x18, 0x08, 0x24, 0x62, 0x58, 0x44, 0xBD, 0xC0, 0xB2, 0xB4, 0xB9, 0xAA, 0xC7, 0xC9, 0xBF, 
		   0x00, 0xBB, 0x00, 0xBB, 0x00, 0xF2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   
	0 },
	{ 0xFA,	1, 
		{ 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   
	10 },		// Gamma Set Update

	// ETC Condition Set
	{ 0xF6,	3, 
		{ 0x00, 0x8C, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   
	0 },		
	{ 0xB3,	1, 
		{ 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   
	0 },
	{ 0xB5,	32, 
		{ 0x2C, 0x12, 0x0C, 0x0A, 0x10, 0x0E, 0x17, 0x13, 0x1F, 0x1A, 0x2A, 0x24, 0x1F, 0x1B, 0x1A, 0x17, 
		   0x2B, 0x26, 0x22, 0x20, 0x3A, 0x34, 0x30, 0x2C, 0x29, 0x26, 0x25, 0x23, 0x21, 0x20, 0x1E, 0x1E },   
	0 },		
	{ 0xB6,	16, 
		{ 0x00, 0x00, 0x11, 0x22, 0x33, 0x44, 0x44, 0x44, 0x55, 0x55, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 
		   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   
	0 },		
	{ 0xB7,	32, 
		{ 0x2C, 0x12, 0x0C, 0x0A, 0x10, 0x0E, 0x17, 0x13, 0x1F, 0x1A, 0x2A, 0x24, 0x1F, 0x1B, 0x1A, 0x17, 
		   0x2B, 0x26, 0x22, 0x20, 0x3A, 0x34, 0x30, 0x2C, 0x29, 0x26, 0x25, 0x23, 0x21, 0x20, 0x1E, 0x1E },   
	0 },		
	{ 0xB8,	16, 
		{ 0x00, 0x00, 0x11, 0x22, 0x33, 0x44, 0x44, 0x44, 0x55, 0x55, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 
		   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   
	0 },		
	{ 0xB9,	32, 
		{ 0x2C, 0x12, 0x0C, 0x0A, 0x10, 0x0E, 0x17, 0x13, 0x1F, 0x1A, 0x2A, 0x24, 0x1F, 0x1B, 0x1A, 0x17, 
		   0x2B, 0x26, 0x22, 0x20, 0x3A, 0x34, 0x30, 0x2C, 0x29, 0x26, 0x25, 0x23, 0x21, 0x20, 0x1E, 0x1E },   
	0 },		
	{ 0xBA,	16, 
		{ 0x00, 0x00, 0x11, 0x22, 0x33, 0x44, 0x44, 0x44, 0x55, 0x55, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 
		   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   
	0 },		

	// Stand-by Off Command	
	{ 0x11,	0, 
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   
	120 },		

	// Display On Command	
	{ 0x29,	0, 
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   
	0 },		
};
#define POWER_ON_SEQ	(int)(sizeof(power_on_sequence)/sizeof(struct setting_table))

static struct setting_table power_off_sequence[] = {
	{ 0x10,	0, 
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   
	120 },		
};
#define POWER_OFF_SEQ	(int)(sizeof(power_off_sequence)/sizeof(struct setting_table))

#if 0
static struct setting_table wake_up_sequence[] = {
	{ 0x11,	0, 
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   
	120 },		
};
#define WAKE_UP_SEQ	(int)(sizeof(wake_up_sequence)/sizeof(struct setting_table))

static struct setting_table stand_by_sequence[] = {
	{ 0x10,	0, 
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   
	120 },		
};
#define STAND_BY_SEQ	(int)(sizeof(stand_by_sequence)/sizeof(struct setting_table))
#endif

static int lcdc_s6e63m0_panel_off(struct platform_device *pdev);

static int lcd_brightness = -1;

static int spi_cs;
static int spi_sclk;
//static int spi_sdo;
static int spi_sdi;
//static int spi_dac;
static int lcd_reset;

static int delayed_backlight_value = -1;
static void lcdc_s6e63m0_set_brightness(int level);

struct s6e63m0_state_type{
	boolean disp_initialized;
	boolean display_on;
	boolean disp_powered_up;
};

static struct s6e63m0_state_type s6e63m0_state = { 0 };
static struct msm_panel_common_pdata *lcdc_s6e63m0_pdata;

static void setting_table_write(struct setting_table *table)
{
	long i, j;

	LCD_CSX_HIGH
	udelay(DEFAULT_USLEEP);
	LCD_SCL_HIGH 
	udelay(DEFAULT_USLEEP);

	/* Write Command */
	LCD_CSX_LOW
	udelay(DEFAULT_USLEEP);
	LCD_SCL_LOW 
	udelay(DEFAULT_USLEEP);		
	LCD_SDI_LOW 
	udelay(DEFAULT_USLEEP);
	
	LCD_SCL_HIGH 
	udelay(DEFAULT_USLEEP); 

   	for (i = 7; i >= 0; i--) { 
		LCD_SCL_LOW
		udelay(DEFAULT_USLEEP);
		if ((table->command >> i) & 0x1)
			LCD_SDI_HIGH
		else
			LCD_SDI_LOW
		udelay(DEFAULT_USLEEP);	
		LCD_SCL_HIGH
		udelay(DEFAULT_USLEEP);	
	}

	LCD_CSX_HIGH
	udelay(DEFAULT_USLEEP);	

	/* Write Parameter */
	if ((table->parameters) > 0) {
		for (j = 0; j < table->parameters; j++) {
			LCD_CSX_LOW 
			udelay(DEFAULT_USLEEP); 	
			
			LCD_SCL_LOW 
			udelay(DEFAULT_USLEEP); 	
			LCD_SDI_HIGH 
			udelay(DEFAULT_USLEEP);
			LCD_SCL_HIGH 
			udelay(DEFAULT_USLEEP); 	

			for (i = 7; i >= 0; i--) { 
				LCD_SCL_LOW
				udelay(DEFAULT_USLEEP);	
				if ((table->parameter[j] >> i) & 0x1)
					LCD_SDI_HIGH
				else
					LCD_SDI_LOW
				udelay(DEFAULT_USLEEP);	
				LCD_SCL_HIGH
				udelay(DEFAULT_USLEEP);					
			}

			LCD_CSX_HIGH
			udelay(DEFAULT_USLEEP);	
		}
	}

	msleep(table->wait);
}

static void spi_init(void)
{
	/* Setting the Default GPIO's */
	spi_sclk = *(lcdc_s6e63m0_pdata->gpio_num);
	spi_cs   = *(lcdc_s6e63m0_pdata->gpio_num + 1);
	spi_sdi  = *(lcdc_s6e63m0_pdata->gpio_num + 2);
	lcd_reset= *(lcdc_s6e63m0_pdata->gpio_num + 3);

	/* Set the output so that we dont disturb the slave device */
	gpio_set_value(spi_sclk, 0);
	gpio_set_value(spi_sdi, 0);

	/* Set the Chip Select De-asserted */
	gpio_set_value(spi_cs, 0);

}

static void s6e63m0_disp_powerup(void)
{
	DPRINT("start %s\n", __func__);	

	if (!s6e63m0_state.disp_powered_up && !s6e63m0_state.display_on) {
		/* Reset the hardware first */

		//TODO: turn on ldo
		msleep(50);
		
		//LCD_RESET_N_HI
		gpio_set_value(lcd_reset, 1);
		msleep(10);
		//LCD_RESET_N_LO
		gpio_set_value(lcd_reset, 0);
		msleep(1);	
		//LCD_RESET_N_HI
		gpio_set_value(lcd_reset, 1);
		msleep(20);

		
		/* Include DAC power up implementation here */
		
	    s6e63m0_state.disp_powered_up = TRUE;
	}
}

static void s6e63m0_disp_powerdown(void)
{
	DPRINT("start %s\n", __func__);	

	/* Reset Assert */
	gpio_set_value(lcd_reset, 0);		

	/* turn off LDO */
	//TODO: turn off LDO

	s6e63m0_state.disp_powered_up = FALSE;
}

static void s6e63m0_init(void)
{
	mdelay(1);
}

static void s6e63m0_disp_on(void)
{
	int i;

	DPRINT("start %s\n", __func__);	

	if (s6e63m0_state.disp_powered_up && !s6e63m0_state.display_on) {
		s6e63m0_init();
		mdelay(20);

		/* s6e63m0 setting */
		for (i = 0; i < POWER_ON_SEQ; i++)
			setting_table_write(&power_on_sequence[i]);			

		mdelay(1);
		s6e63m0_state.display_on = TRUE;
	}
}

static int lcdc_s6e63m0_panel_on(struct platform_device *pdev)
{
	DPRINT("start %s\n", __func__);	

	if (!s6e63m0_state.disp_initialized) {
		/* Configure reset GPIO that drives DAC */
		lcdc_s6e63m0_pdata->panel_config_gpio(1);
		spi_init();	/* LCD needs SPI */
		s6e63m0_disp_powerup();
		s6e63m0_disp_on();
		s6e63m0_state.disp_initialized = TRUE;
	}

	return 0;
}

static int lcdc_s6e63m0_panel_off(struct platform_device *pdev)
{
	int i;

	DPRINT("start %s\n", __func__);	

	if (s6e63m0_state.disp_powered_up && s6e63m0_state.display_on) {
		
		for (i = 0; i < POWER_OFF_SEQ; i++)
			setting_table_write(&power_off_sequence[i]);	
		
		lcdc_s6e63m0_pdata->panel_config_gpio(0);
		s6e63m0_state.display_on = FALSE;
		s6e63m0_state.disp_initialized = FALSE;
		s6e63m0_disp_powerdown();
	}
	return 0;
}

// brightness tuning
#define MAX_BRIGHTNESS_LEVEL 255
#define LOW_BRIGHTNESS_LEVEL 30
#define MAX_BACKLIGHT_VALUE 30
#define LOW_BACKLIGHT_VALUE 6
#define DIM_BACKLIGHT_VALUE 2

static DEFINE_SPINLOCK(bl_ctrl_lock);

static void lcdc_s6e63m0_set_brightness(int level)
{
	unsigned long irqflags;
	int tune_level = level;
	// LCD should be turned on prior to backlight
	if(s6e63m0_state.disp_initialized == FALSE && tune_level > 0) {
		//DPRINT("skip bl:%d\n", tune_level);	
		delayed_backlight_value = tune_level;
		return;
	}
	else {
		//DPRINT("set bl:%d\n", tune_level);	
		delayed_backlight_value = -1;
	}
	
	//TODO: lock
	spin_lock_irqsave(&bl_ctrl_lock, irqflags);

	if (tune_level <= 0) {
		lcd_brightness = tune_level;
	} else {
		/* keep back light ON */
		if(unlikely(lcd_brightness < 0)) {
			lcd_brightness = 0;
		}
		
		lcd_brightness = tune_level;
	}
	
	//TODO: unlock
	spin_unlock_irqrestore(&bl_ctrl_lock, irqflags);

}

static void lcdc_s6e63m0_set_backlight(struct msm_fb_data_type *mfd)
{	
	int bl_level = mfd->bl_level;
	int tune_level;

	// brightness tuning
	if(bl_level >= LOW_BRIGHTNESS_LEVEL)
		tune_level = (bl_level - LOW_BRIGHTNESS_LEVEL) * (MAX_BACKLIGHT_VALUE-LOW_BACKLIGHT_VALUE) / (MAX_BRIGHTNESS_LEVEL-LOW_BRIGHTNESS_LEVEL) + LOW_BACKLIGHT_VALUE;
	else if(bl_level > 0)
		tune_level = DIM_BACKLIGHT_VALUE;
	else
		tune_level = bl_level;

	// turn on lcd if needed
	if(tune_level > 0)	{
		if(!s6e63m0_state.disp_powered_up)
			s6e63m0_disp_powerup();
		if(!s6e63m0_state.display_on)
			s6e63m0_disp_on();
	}

	lcdc_s6e63m0_set_brightness(tune_level);
}

static int __init s6e63m0_probe(struct platform_device *pdev)
{
	DPRINT("start %s\n", __func__);	

	if (pdev->id == 0) {
		lcdc_s6e63m0_pdata = pdev->dev.platform_data;
		return 0;
	}
	msm_fb_add_device(pdev);
	return 0;
}

static void s6e63m0_shutdown(struct platform_device *pdev)
{
	DPRINT("start %s\n", __func__);	

	lcdc_s6e63m0_panel_off(pdev);
}

static struct platform_driver this_driver = {
	.probe  = s6e63m0_probe,
	.shutdown	= s6e63m0_shutdown,
	.driver = {
		.name   = "lcdc_s6e63m0_wvga",
	},
};

static struct msm_fb_panel_data s6e63m0_panel_data = {
	.on = lcdc_s6e63m0_panel_on,
	.off = lcdc_s6e63m0_panel_off,
	.set_backlight = lcdc_s6e63m0_set_backlight,
};

static struct platform_device this_device = {
	.name   = "lcdc_panel",
	.id	= 1,
	.dev	= {
		.platform_data = &s6e63m0_panel_data,
	}
};

#define LCDC_FB_XRES	480
#define LCDC_FB_YRES	800
#define LCDC_HPW		2
#define LCDC_HBP		16
#define LCDC_HFP		16
#define LCDC_VPW		2
#define LCDC_VBP		3
#define LCDC_VFP		28
 
static int __init lcdc_s6e63m0_panel_init(void)
{
	int ret;
	struct msm_panel_info *pinfo;

#ifdef CONFIG_FB_MSM_MDDI_AUTO_DETECT
	if (msm_fb_detect_client("lcdc_s6e63m0_wvga"))
	{
		printk(KERN_ERR "%s: msm_fb_detect_client failed!\n", __func__); 
		return 0;
	}
#endif
	DPRINT("start %s\n", __func__);	
	
	ret = platform_driver_register(&this_driver);
	if (ret)
	{
		printk(KERN_ERR "%s: platform_driver_register failed! ret=%d\n", __func__, ret); 
		return ret;
	}

	pinfo = &s6e63m0_panel_data.panel_info;
	pinfo->xres = LCDC_FB_XRES;
	pinfo->yres = LCDC_FB_YRES;
	pinfo->type = LCDC_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 24;
	pinfo->fb_num = 2;
	pinfo->clk_rate = 25530000;
	pinfo->bl_max = 255;
	pinfo->bl_min = 1;

	pinfo->lcdc.h_back_porch = LCDC_HBP;
	pinfo->lcdc.h_front_porch = LCDC_HFP;
	pinfo->lcdc.h_pulse_width = LCDC_HPW;
	pinfo->lcdc.v_back_porch = LCDC_VBP;
	pinfo->lcdc.v_front_porch = LCDC_VFP;
	pinfo->lcdc.v_pulse_width = LCDC_VPW;
	pinfo->lcdc.border_clr = 0;     /* blk */
	pinfo->lcdc.underflow_clr = 0xff;       /* blue */
	pinfo->lcdc.hsync_skew = 0;

	printk("%s\n", __func__);

	ret = platform_device_register(&this_device);
	if (ret)
	{
		printk(KERN_ERR "%s: platform_device_register failed! ret=%d\n", __func__, ret); 
		platform_driver_unregister(&this_driver);
	}

	return ret;
}

module_init(lcdc_s6e63m0_panel_init);
