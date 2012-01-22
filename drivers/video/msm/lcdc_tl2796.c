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
#if defined (CONFIG_FB_MSM_CMC623)
#include "tune_cmc623.h"
#endif

#define LCDC_DEBUG

#ifdef LCDC_DEBUG
#define DPRINT(x...)	printk("tl2796 " x)
#else
#define DPRINT(x...)	
#endif

/*
  Serial Interface
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
	unsigned char parameter[45];
	long wait;
};

static struct setting_table nt35510_SEQ_SETTING[] = {
	{ 0xFF,	4, //MANUFACTURE Command Access Protect
	  {	0xAA, 0x55, 0x25, 0x01, 0x00, 
		0x00, 0x00, 0x00, 0x00, 0x00,},
	  10 },
	
  
    { 0xFA,	31, //MANUFACTURE Command Access Protect
	  {	0x00, 0x80, 0x00, 0x00, 0x00, 
		0x00, 0x30, 0x02, 0x20, 0x84, 
		0x0F, 0x0F, 0x20, 0x40, 0x40, 
		0x00, 0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x3F, 0xA0, 
		0x30, 0x00, 0x00, 0x00, 0x00 },
	  0 },

	{ 0xF3,	8, 
	  {	0x00, 0x32, 0x00, 0x38, 0x00, 
		0x08, 0x11, 0x00, 0x00, 0x00,},
	  0 },

	{ 0xF0,	5,
	  {	0x55, 0xAA, 0x52, 0x08, 0x01, 
		0x00, 0x00, 0x00, 0x00, 0x00,}, 
	  0 },

	{ 0xB6,	3, 
	  {	0x04, 0x04, 0x04, 0x00, 0x00,}, 
	  0 },

	{ 0xBF,	1, 
	  {	0x01, 0x00, 0x00, 0x00, 0x00,},
	  0 },

	{ 0xB9,	3, 
	  {	0x34, 0x34, 0x34, 0x00, 0x00,},
	  0 },

	{ 0xB3,	3, 
	  {	0x0A, 0x0A, 0x0A, 0x00, 0x00,},
	  0 },

	{ 0xBA,	3, 
	  {	0x24, 0x24, 0x24, 0x00, 0x00,},
	  0 },

	{ 0xB5,	3, 
	  {	0x08, 0x08, 0x08, 0x00, 0x00,},
	  0 },
  
	{ 0xBE,	2, // VCOM
	  {	0x00, 0x50, 0x00, 0x00, 0x00,},
	  0 },

	{ 0xF0,	5, // Enable CM2_Page0
	  {	0x55, 0xAA, 0x52, 0x08, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00,}, 
	  0 },

	{ 0xB0,	5, 
	  {	0x04, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00,}, 
	  0 },

	{ 0xB1,	2, 
	  {	0xCC, 0x00, 0x00, 0x00, 0x00,},
	  0 },

	{ 0xBC,	3, 
	  {	0x00, 0x00, 0x00, 0x00, 0x00,},
	  0 },

	{ 0xB8,	1,
	  {	0x01, 0x07, 0x07, 0x07, 0x00,},
	  0 },

	{ 0xCC,	3, 
	  {	0x01, 0x3F, 0x3F, 0x00, 0x00,},
	  0 },

	{ 0xB3,	1,
	  {	0x00, 0x00, 0x00, 0x00, 0x00,},
	  0 },

	{ 0x36,	1, 
	  {	0x03, 0x00, 0x00, 0x00, 0x00,},
	  0 },

	{ 0x51,	1,
	  {	0x7F, 0x00, 0x00, 0x00, 0x00,},
	  0 },

	{ 0x53,	1, 
	  {	0x24, 0x00, 0x00, 0x00, 0x00,},
	  0 },
};

static struct setting_table tl2796_SEQ_STANDBY_OFF[] = {
	{ 0x11,	0, //MANUFACTURE Command Access Protect
	  {	0x00,}, 
	  120 },
};

static struct setting_table tl2796_SEQ_STANDBY_ON[] = {
	{ 0x10,	0, //MANUFACTURE Command Access Protect
	  {	0x00,},
	  120 },
};

static struct setting_table tl2796_SEQ_DISPLAY_ON[] = {
	{ 0x29,	0, //MANUFACTURE Command Access Protect
	  {	0x00,},
	  0 },
};

static struct setting_table tl2796_SEQ_DISPLAY_OFF[] = {
	{ 0x28,	0, //MANUFACTURE Command Access Protect
	  {	0x00,},
	  27 },
};

static struct setting_table tl2796_SEQ_SLEEP_IN[] = {
	{ 0x10,	0, //MANUFACTURE Command Access Protect
	  {	0x00,},
	  155 },
};

#define POWER_ON_SEQ	(int)(sizeof(nt35510_SEQ_SETTING)/sizeof(struct setting_table))
#define STANDBY_ON_SEQ	(int)(sizeof(tl2796_SEQ_STANDBY_OFF)/sizeof(struct setting_table))
#define STANDBY_OFF_SEQ	(int)(sizeof(tl2796_SEQ_STANDBY_ON)/sizeof(struct setting_table))
#define DISPLAY_ON_SEQ	(int)(sizeof(tl2796_SEQ_DISPLAY_ON)/sizeof(struct setting_table))
#define DISPLAY_OFF_SEQ	(int)(sizeof(tl2796_SEQ_DISPLAY_OFF)/sizeof(struct setting_table))
#define SLEEP_IN_SEQ	(int)(sizeof(tl2796_SEQ_SLEEP_IN)/sizeof(struct setting_table))

static int lcdc_tl2796_panel_off(struct platform_device *pdev);

static int lcd_brightness = -1;

static int spi_cs;
static int spi_sclk;
static int spi_sdi;
static int lcd_reset;

static void lcdc_tl2796_set_brightness(int level);

struct tl2796_state_type{
	boolean disp_initialized;
	boolean display_on;
	boolean disp_powered_up;
};

static struct tl2796_state_type tl2796_state = { 0 };
static struct msm_panel_common_pdata *lcdc_tl2796_pdata;

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
#if 1
	/* Setting the Default GPIO's */
	spi_sclk = *(lcdc_tl2796_pdata->gpio_num);
	spi_cs   = *(lcdc_tl2796_pdata->gpio_num + 1);
	spi_sdi  = *(lcdc_tl2796_pdata->gpio_num + 2);
	lcd_reset= *(lcdc_tl2796_pdata->gpio_num + 3);
	/* Set the output so that we dont disturb the slave device */
	gpio_set_value(spi_sclk, 0);
	gpio_set_value(spi_sdi, 0);

	/* Set the Chip Select De-asserted */
	gpio_set_value(spi_cs, 0);
	printk("spi_sclk: %d, spi_cs: %d, spi_sdi: %d, lcd_reset: %d\n", spi_sclk, spi_cs, spi_sdi, lcd_reset);
#endif
}

static void tl2796_disp_powerup(void)
{
	DPRINT("start %s\n", __func__);	

	if (!tl2796_state.disp_powered_up && !tl2796_state.display_on) {
		/* Reset the hardware first */

		msleep(50);
		
		//LCD_RESET_N_HI
		gpio_set_value(lcd_reset, 1);
		msleep(10);
		//LCD_RESET_N_LO
		gpio_set_value(lcd_reset, 0);
		msleep(1);	
		//LCD_RESET_N_HI
		gpio_set_value(lcd_reset, 1);
		msleep(5);

		
		/* Include DAC power up implementation here */
		
	    tl2796_state.disp_powered_up = TRUE;
	}
}

static void tl2796_disp_powerdown(void)
{
	DPRINT("start %s\n", __func__);	

	/* Reset Assert */
	gpio_set_value(lcd_reset, 0);		

	tl2796_state.disp_powered_up = FALSE;
}

static void tl2796_init(void)
{
	mdelay(1);
}

static void tl2796_disp_on(void)
{
	int i;

	DPRINT("start %s\n", __func__);	
	if (tl2796_state.disp_powered_up && !tl2796_state.display_on) {
		tl2796_init();
		mdelay(20);

		/* tl2796 setting */
		for (i = 0; i < POWER_ON_SEQ; i++)
			setting_table_write(&nt35510_SEQ_SETTING[i]);	

		mdelay(1);
		for (i = 0; i < STANDBY_OFF_SEQ; i++)
			setting_table_write(&tl2796_SEQ_STANDBY_OFF[i]);

		mdelay(1);
		for (i = 0; i < DISPLAY_ON_SEQ; i++)
			setting_table_write(&tl2796_SEQ_STANDBY_ON[i]);

		tl2796_state.display_on = TRUE;
	}
}

static int lcdc_tl2796_panel_on(struct platform_device *pdev)
{
	DPRINT("start %s\n", __func__);	
#if defined(CONFIG_FB_MSM_CMC623)
	tune_cmc623_pre_resume();
	tune_cmc623_resume();
	/* CMC623 Power On */
#endif
	if (!tl2796_state.disp_initialized) {
		/* Configure reset GPIO that drives DAC */
		lcdc_tl2796_pdata->panel_config_gpio(1);
		tl2796_disp_powerup();
    	spi_init();	/* LCD needs SPI */
		tl2796_disp_on();
		tl2796_state.disp_initialized = TRUE;
	}

	return 0;
}

static int lcdc_tl2796_panel_off(struct platform_device *pdev)
{
	int i;

	DPRINT("start %s\n", __func__);	

	if (tl2796_state.disp_powered_up && tl2796_state.display_on) {
		
		for (i = 0; i < DISPLAY_OFF_SEQ; i++)
			setting_table_write(&tl2796_SEQ_DISPLAY_OFF[i]);	
	
		for (i = 0; i < SLEEP_IN_SEQ; i++)
			setting_table_write(&tl2796_SEQ_SLEEP_IN[i]);	
	
		lcdc_tl2796_pdata->panel_config_gpio(0);
		tl2796_state.display_on = FALSE;
		tl2796_state.disp_initialized = FALSE;
		tl2796_disp_powerdown();
	}
#if defined(CONFIG_FB_MSM_CMC623)
	tune_cmc623_suspend();
#endif
	/* CMC623 Power Off */
	
	return 0;
}

// brightness tuning
#define MAX_BRIGHTNESS_LEVEL 255
#define LOW_BRIGHTNESS_LEVEL 30
#define MAX_BACKLIGHT_VALUE 30
#define LOW_BACKLIGHT_VALUE 6
#define DIM_BACKLIGHT_VALUE 2

static DEFINE_SPINLOCK(bl_ctrl_lock);

static void lcdc_tl2796_set_brightness(int level)
{
	unsigned long irqflags;
	int tune_level = level;

	//TODO: lock
	spin_lock_irqsave(&bl_ctrl_lock, irqflags);

	printk("%s\n",__func__);
	cmc623_manual_pwm_brightness(tune_level);
	//TODO: unlock
	spin_unlock_irqrestore(&bl_ctrl_lock, irqflags);

}

static void lcdc_tl2796_set_backlight(struct msm_fb_data_type *mfd)
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
		if(!tl2796_state.disp_powered_up)
			tl2796_disp_powerup();
		if(!tl2796_state.display_on)
			tl2796_disp_on();
	}

	lcdc_tl2796_set_brightness(tune_level);
}

static int __init tl2796_probe(struct platform_device *pdev)
{
	DPRINT("start %s\n", __func__);	

	if (pdev->id == 0) {
		lcdc_tl2796_pdata = pdev->dev.platform_data;
		return 0;
	}
	msm_fb_add_device(pdev);
	return 0;
}

static void tl2796_shutdown(struct platform_device *pdev)
{
	DPRINT("start %s\n", __func__);	

	lcdc_tl2796_panel_off(pdev);
}

static struct platform_driver this_driver = {
	.probe  = tl2796_probe,
	.shutdown	= tl2796_shutdown,
	.driver = {
		.name   = "lcdc_tl2796_wvga",
	},
};

static struct msm_fb_panel_data tl2796_panel_data = {
	.on = lcdc_tl2796_panel_on,
	.off = lcdc_tl2796_panel_off,
	.set_backlight = lcdc_tl2796_set_backlight,
};

static struct platform_device this_device = {
	.name   = "lcdc_panel",
	.id	= 1,
	.dev	= {
		.platform_data = &tl2796_panel_data,
	}
};


#define LCDC_FB_XRES	480
#define LCDC_FB_YRES	800
#define DOT_CLK			12288000
#define LCDC_HPW		10
#define LCDC_HBP		20
#define LCDC_HFP		10
#define LCDC_VPW		6
#define LCDC_VBP		8
#define LCDC_VFP		6

static int __init lcdc_tl2796_panel_init(void)
{
	int ret;
	struct msm_panel_info *pinfo;

	DPRINT("start %s\n", __func__);	
	
	ret = platform_driver_register(&this_driver);
	if (ret)
	{
		printk(KERN_ERR "%s: platform_driver_register failed! ret=%d\n", __func__, ret); 
		return ret;
	}

	pinfo = &tl2796_panel_data.panel_info;
	pinfo->xres = LCDC_FB_XRES;
	pinfo->yres = LCDC_FB_YRES;
	pinfo->type = LCDC_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 24;
	pinfo->fb_num = 2;
	pinfo->clk_rate = DOT_CLK;
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

	tl2796_state.disp_initialized = FALSE;

	printk("%s\n", __func__);

	ret = platform_device_register(&this_device);
	if (ret)
	{
		printk(KERN_ERR "%s: platform_device_register failed! ret=%d\n", __func__, ret); 
		platform_driver_unregister(&this_driver);
	}

	return ret;
}

module_init(lcdc_tl2796_panel_init);
