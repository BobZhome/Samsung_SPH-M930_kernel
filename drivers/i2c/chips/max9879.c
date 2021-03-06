/*
 * Copyright (C) 2007-2008 SAMSUNG Corporation.
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
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <mach/vreg.h>
#include <asm/io.h>
#include <linux/uaccess.h>

#include <linux/i2c/max9879.h>

#define ALLOW_USPACE_RW		1

static struct i2c_client *pclient;

static int opened;
static int pclk_set;

#define MAX9879_INPUTMODE_CTRL 0x00
#define MAX9879_SPKVOL_CTRL 0x01
#define MAX9879_LEFT_HPHVOL_CTRL 0x02
#define MAX9879_RIGHT_HPHVOL_CTRL 0x03
#define MAX9879_OUTPUTMODE_CTRL 0x04

#if defined(CONFIG_SAMSUNG_TARGET)
#define PATH_MIC_SEL		26
#endif

static struct workqueue_struct *headset_on_work_queue;
static void headset_on_work(struct work_struct *work);
static DECLARE_WORK(g_headset_on_work, headset_on_work);
unsigned char is_incall;
static int is_incallMode = 0;

#ifdef CONFIG_ANDROID_POWER
#include <linux/android_power.h>
static android_suspend_lock_t max9879_suspend_lock = {
	.name = "max9879"
};
static inline void init_suspend(void)
{
	android_init_suspend_lock(&max9879_suspend_lock);
}

static inline void deinit_suspend(void)
{
	android_uninit_suspend_lock(&max9879_suspend_lock);
}

static inline void prevent_suspend(void)
{
	android_lock_idle(&max9879_suspend_lock);
}

static inline void allow_suspend(void)
{
	android_unlock_suspend(&max9879_suspend_lock);
}
#else
static inline void init_suspend(void) {}
static inline void deinit_suspend(void) {}
static inline void prevent_suspend(void) {}
static inline void allow_suspend(void) {}
#endif

#ifdef CONFIG_ANDROID_POWER
static void max9879_early_suspend(struct android_early_suspend *h);
static void max9879_late_resume(struct android_early_suspend *h);
#endif


DECLARE_MUTEX(audio_sem);

struct max9879_data {
	struct work_struct work;
#ifdef CONFIG_ANDROID_POWER
	struct android_early_suspend early_suspend;
#endif
};

static DECLARE_WAIT_QUEUE_HEAD(g_data_ready_wait_queue);

int max9879_i2c_sensor_init(void);
int max9879_i2c_hph_gain(uint8_t gain);
int max9879_i2c_spk_gain(uint8_t gain);
int max9879_i2c_speaker_onoff(int nOnOff);		// no-in-call
int max9879_i2c_receiver_onoff(int nOnOff);
int max9879_i2c_headset_onoff(int nOnOff);		// in-call
int max9879_i2c_speaker_headset_onoff(int nOnOff);
static int max9879_i2c_write(unsigned char u_addr, unsigned char u_data);
static int max9879_i2c_read(unsigned char u_addr, unsigned char *pu_data);

unsigned char spk_vol, hpl_vol, hpr_vol;
unsigned char spk_vol_mute, hpl_vol_mute, hpr_vol_mute;
static int mic_status = 1;		// 1 sub_mic
static int rec_status = 0;

#define I2C_WRITE(reg,data) if (!max9879_i2c_write(reg, data) < 0) printk("MAX9879_i2c_write error\n");
#define I2C_READ(reg,data) if (max9879_i2c_read(reg,data) < 0 ) printk("MAX9879_i2c_read error\n");

char max9879_outmod_reg;

//extern int audio_enabled;		
int audio_enabled = 1;		// TEMP for TEST

static void headset_on_work(struct work_struct *work)
{
	unsigned char reg_value;

//	I2C_WRITE(MAX9879_INPUTMODE_CTRL,0x70); // dINA = 1(mono), dINB = 1(mono)

	I2C_READ(MAX9879_OUTPUTMODE_CTRL,&reg_value);

	if(reg_value & 0x80)
	{
		reg_value &= 0x7F;
		I2C_WRITE(MAX9879_OUTPUTMODE_CTRL, reg_value);
		I2C_WRITE(MAX9879_SPKVOL_CTRL,0x0); // MAX 0x1F : 0dB
		I2C_WRITE(MAX9879_LEFT_HPHVOL_CTRL,0x0); // MAX 0x1F : 0dB
		I2C_WRITE(MAX9879_RIGHT_HPHVOL_CTRL,0x0); // MAX 0x1F : 0dB

	}

//	msleep(1000);
//	I2C_WRITE(MAX9879_LEFT_HPHVOL_CTRL,0x17); // MAX 0x1F : 0dB
//	I2C_WRITE(MAX9879_RIGHT_HPHVOL_CTRL,0x17); // MAX 0x1F : 0dB

#if 0
	if(is_incall == 0)
	{
		msleep(6000);
	} else {
		msleep(300);
	}
#else
	msleep(600);
#endif
	max9879_i2c_headset_onoff(true);
}

int audio_i2c_tx_data(char* txData, int length)
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
    	printk(KERN_ERR "MAX9879: audio_i2c_tx_data addr %d\n", pclient->addr);
	rc = i2c_transfer(pclient->adapter, msg, 1);
	if (rc < 0) {
		printk(KERN_ERR "MAX9879: audio_i2c_tx_data error %d\n", rc);
		return rc;
	}

#if 0
	else {
		int i;
		/* printk(KERN_INFO "mt_i2c_lens_tx_data: af i2c client addr = %x,"
		   " register addr = 0x%02x%02x:\n", slave_addr, txData[0], txData[1]); 
		   */
		for (i = 0; i < length; i++)
			printk("\tdata[%d]: 0x%02x\n", i, txData[i]);
	}
#endif
	return 0;
}


static int max9879_i2c_write(unsigned char u_addr, unsigned char u_data)
{
	int rc;
	unsigned char buf[2];

	buf[0] = u_addr;
	buf[1] = u_data;
    
	rc = audio_i2c_tx_data(buf, 2);
	if(rc < 0)
		printk(KERN_ERR "MAX9879: txdata error %d add:0x%02x data:0x%02x\n",
			rc, u_addr, u_data);

	return rc;	
}

static int audio_i2c_rx_data(char* rxData, int length)
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
		printk(KERN_ERR "MAX9879: audio_i2c_rx_data error %d\n", rc);
		return rc;
	}
      
#if 0
	else {
		int i;
		for (i = 0; i < length; i++)
			printk(KERN_INFO "\tdata[%d]: 0x%02x\n", i, rxData[i]);
	}
#endif

	return 0;
}

static int max9879_i2c_read(unsigned char u_addr, unsigned char *pu_data)
{
	int rc;
	unsigned char buf;

	buf = u_addr;
	rc = audio_i2c_rx_data(&buf, 1);
	if (!rc)
		*pu_data = buf;
	else printk(KERN_ERR "MAX9879: i2c read failed\n");
	return rc;	
}


static void max9879_chip_init(void)
{
//	int ret;
//	printk(KERN_INFO "MAX9879: init\n");
	if (!pclient) 
		return;

      /* MAX9879 init sequence */
      
	/* delay 2 ms */
	msleep(2);
	printk(KERN_INFO "MAX9879: MAX9879 sensor init sequence done\n");
}

static int max9879_open(struct inode *ip, struct file *fp)
{
	int rc = -EBUSY;
	down(&audio_sem);
	printk(KERN_INFO "MAX9879: open\n");
	if (!opened) {
		printk(KERN_INFO "MAX9879: prevent collapse on idle\n");
#ifndef CONFIG_ANDROID_POWER
		prevent_suspend();
#endif
//		opened = 1;
		rc = 0;
	}
	up(&audio_sem);
	return rc;
}

static int max9879_release(struct inode *ip, struct file *fp)
{
	int rc = -EBADF;
	printk(KERN_INFO "MAX9879: release\n");
	down(&audio_sem);
	if (opened) {
		printk(KERN_INFO "MAX9879: release clocks\n");
             // PWR_DOWN ó�� MAX9879_i2c_power_down();
		printk(KERN_INFO "MAX9879: allow collapse on idle\n");
#ifndef CONFIG_ANDROID_POWER            
		allow_suspend();
#endif
		rc = pclk_set = opened = 0;
	}
	up(&audio_sem);
	return rc;
}

#if ALLOW_USPACE_RW
#define COPY_FROM_USER(size) ({                                         \
        if (copy_from_user(rwbuf, argp, size)) rc = -EFAULT;            \
        !rc; })
#endif

static long max9879_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
#if ALLOW_USPACE_RW
	void __user *argp = (void __user *)arg;
#endif
	int rc = 0;
	unsigned char reg_value;

#if ALLOW_USPACE_RW
	unsigned char addr = 0;
	unsigned char data = 0;
    unsigned char temp_buf;
	char rwbuf[2];
#endif

	down(&audio_sem);

	spk_vol_mute = hpl_vol_mute = hpr_vol_mute = 0;

	switch(cmd) {
#if ALLOW_USPACE_RW
	case MAX9879_I2C_IOCTL_W:
		if (COPY_FROM_USER(2)) {
			addr = *((unsigned char *)rwbuf);
			data = *((unsigned char *)(rwbuf+1));
			rc = max9879_i2c_write(addr, data);

			switch(addr)
			{
				case 0x01:
					spk_vol = data;
					break;
				case 0x02:
					hpl_vol = data;
					break;
				case 0x03:
					hpr_vol = data;
					break;
				default:
					break;
			}

		}
		else printk("MAX9879: write: err %d\n", rc);
		break;

	case MAX9879_I2C_IOCTL_R:
		if (COPY_FROM_USER(2)) {
			addr = *((unsigned char*) rwbuf);
			rc = max9879_i2c_read(addr, (unsigned char *)(rwbuf+1));
			if (!rc) {
				if (copy_to_user(argp, rwbuf, 2)) {
					printk("MAX9879: read: err writeback -EFAULT\n");
					rc = -EFAULT;
				}
			}
		}
		else printk("MAX9879: read: err %d\n", rc);
		break;

        case MAX9879_HPH_VOL_SET:
        {
            if (COPY_FROM_USER(2)) {
                temp_buf = *((unsigned char *)rwbuf);
                max9879_i2c_hph_gain(temp_buf);
            }
        }
        break;

    case MAX9879_SPK_VOL_SET:
        {
            if (COPY_FROM_USER(2)) {
                temp_buf = *((unsigned char *)rwbuf);
                max9879_i2c_spk_gain(temp_buf);
            }
        }
        break;
#endif /* ALLOW_USPACE_RW */

    case MAX9879_I2C_IOCTL_SWITCH_DEVICE:
        {
        
        }
        break;

    case MAX9879_SPEAKER_ON:
             {
                printk("MAX9879 : speaker on\n");
                max9879_i2c_speaker_onoff(true);
             }
		break;    

    case MAX9879_SPEAKER_OFF:
             {
                printk("MAX9879 : speaker off\n");
                max9879_i2c_speaker_onoff(false);
             }
		break;        

    case MAX9879_HEADSET_ON:
            {
                printk("MAX9879 : headset on\n");
#if 1
				if (COPY_FROM_USER(2)) {
					is_incall = *((unsigned char *)rwbuf);
				}

				queue_work(headset_on_work_queue, &g_headset_on_work);
#else
                MAX9879_i2c_headset_onoff(true);
#endif
            }
		break;   

    case MAX9879_HEADSET_OFF:
            {
                printk("MAX9879 : headset off\n");
                max9879_i2c_headset_onoff(false);
            }
		break;       

    case MAX9879_SPK_EAR_ON: 
                printk("MAX9879 : speaker headset on\n");
		max9879_i2c_speaker_headset_onoff(true);
		break;    

    case MAX9879_RCV_ON:
            {
                printk("MAX9879 : receiver on\n");
                max9879_i2c_receiver_onoff(true);
            }
            break;     

    case MAX9879_RCV_OFF:
            {
                printk("MAX9879 : receiver off\n");
                max9879_i2c_receiver_onoff(false);
            }
            break;

    case MAX9879_I2C_IOCTL_INIT: 
        {
             printk("MAX9879 : i2c init \n");
		rc = max9879_i2c_sensor_init();
        }
        break;    
       
	case MAX9879_AMP_SUSPEND:
		{
			if(audio_enabled)
			{
				printk("MAX9879 : AMP suspend\n");
				rc = max9879_i2c_read(MAX9879_OUTPUTMODE_CTRL,&reg_value);
				if (rc < 0 )
				{
					printk(KERN_ERR "MAX9879_suspend: MAX9879_i2c_read failed\n");
					return -EIO;
				}

				max9879_i2c_write(MAX9879_SPKVOL_CTRL,spk_vol_mute);
				max9879_i2c_write(MAX9879_LEFT_HPHVOL_CTRL,hpl_vol_mute);
				max9879_i2c_write(MAX9879_RIGHT_HPHVOL_CTRL,hpr_vol_mute);

				reg_value = reg_value & 0x7f; // SHDN = 0 MAX9879 shut down

				rc = max9879_i2c_write(MAX9879_OUTPUTMODE_CTRL, reg_value);
				if (rc < 0 )
				{
					printk(KERN_ERR "MAX9879_suspend: MAX9879_i2c_write failed\n");
					return -EIO;
				}

//				msleep(10); // 10m startup time delay
			}
		}
		break;

	case MAX9879_AMP_RESUME:
		{
			if(audio_enabled)
			{
				printk("MAX9879 : AMP resume\n");
				rc = max9879_i2c_read(MAX9879_OUTPUTMODE_CTRL,&reg_value);
				if (rc < 0 )
				{
					printk(KERN_ERR "MAX9879_resume: MAX9879_i2c_read failed\n");
					return -EIO;
				}

				reg_value = reg_value | 0x80; // SHDN = 1 MAX9879 wakeup

				rc = max9879_i2c_write(MAX9879_OUTPUTMODE_CTRL, reg_value);
				if (rc < 0 )
				{
					printk(KERN_ERR "MAX9879_resume: MAX9879_i2c_write failed\n");
					return -EIO;
				}

				msleep(10); // 10m startup time delay
//				printk("MAX9879 : 0x%x, 0x%x, 0x%x.\n", spk_vol, hpl_vol, hpr_vol);

				max9879_i2c_write(MAX9879_SPKVOL_CTRL,spk_vol);
				max9879_i2c_write(MAX9879_LEFT_HPHVOL_CTRL,hpl_vol);
				max9879_i2c_write(MAX9879_RIGHT_HPHVOL_CTRL,hpr_vol);
			}
		}
		break;

	case MAX9879_AMP_RECORDING_MAIN_MIC:
#if defined(CONFIG_SAMSUNG_TARGET)
		if(gpio_direction_output(PATH_MIC_SEL, 0))		// main mic
		{
			printk("%s:%d set MIC_SEL fail\n", __func__, __LINE__);
		}
		rec_status = 1;
		printk("%s- MIC_SEL=%d.\n", __func__, gpio_get_value(PATH_MIC_SEL));
#endif
		break;

	case MAX9879_AMP_RECORDING_SUB_MIC:
#if defined(CONFIG_SAMSUNG_TARGET)
		if(gpio_direction_output(PATH_MIC_SEL, 1))		// sub mic
		{
			printk("%s:%d set MIC_SEL fail\n", __func__, __LINE__);
		}
		rec_status = 0;
		printk("%s- MIC_SEL=%d.\n", __func__, gpio_get_value(PATH_MIC_SEL));
#endif
		break;

    default:
        printk(KERN_INFO "MAX9879: unknown ioctl %d\n", cmd);
        break;
	}

	up(&audio_sem);

	return rc;
}

int max9879_gpio_recording_start(int state)		// for recording
{
#if defined(CONFIG_SAMSUNG_TARGET)
		if(gpio_direction_output(PATH_MIC_SEL, state ? 0 : mic_status))		// main mic
		{
			printk("%s:%d set MIC_SEL fail\n", __func__, __LINE__);
			return -1;
		}
		rec_status = state;
		printk("%s- MIC_SEL=%d.\n", __func__, gpio_get_value(PATH_MIC_SEL));
#endif
		return 0;
}

int max9879_i2c_sensor_init(void)
{
	I2C_WRITE(MAX9879_OUTPUTMODE_CTRL,0x06); // output all, SHDN mode
        
	I2C_WRITE(MAX9879_INPUTMODE_CTRL,0x50); // pre-gain 0dB

	spk_vol = hpl_vol = hpr_vol = 0x1F;

	I2C_WRITE(MAX9879_SPKVOL_CTRL,spk_vol); // MAX 0x1F : 0dB

	I2C_WRITE(MAX9879_LEFT_HPHVOL_CTRL,hpl_vol); // MAX 0x1F : 0dB

	I2C_WRITE(MAX9879_RIGHT_HPHVOL_CTRL,hpr_vol); // MAX 0x1F : 0dB

	printk("MAX9879 : %s is done.\n", __func__);

	return 0;
}

int max9879_i2c_hph_gain(uint8_t gain)
{
	static const uint8_t max_legal_gain  = 0x1F;
	
	if (gain > max_legal_gain) gain = max_legal_gain;

	hpl_vol = hpr_vol = gain;

	I2C_WRITE(MAX9879_LEFT_HPHVOL_CTRL, gain);
	I2C_WRITE(MAX9879_RIGHT_HPHVOL_CTRL, gain);
	return 0;
}

int max9879_i2c_spk_gain(uint8_t gain)
{
	static const uint8_t max_legal_gain  = 0x1F;
	
	if (gain > max_legal_gain) gain = max_legal_gain;

	spk_vol = gain;

	I2C_WRITE(MAX9879_SPKVOL_CTRL, gain);
	return 0;
}

int max9879_i2c_speaker_headset_onoff(int nOnOff)
{
        unsigned char reg_value;

//		printk("%s(%d) - MIC_SEL=%d.\n", __func__, nOnOff, gpio_get_value(PATH_MIC_SEL));
//		is_incallMode = 0;
        I2C_WRITE(MAX9879_INPUTMODE_CTRL,0x50); // dINA = 0(stereo), dINB = 1(mono)
        if( nOnOff )
        {
            I2C_READ(MAX9879_OUTPUTMODE_CTRL,&reg_value);

			reg_value &= 0xB0;
			reg_value |= 0x09;

            I2C_WRITE(MAX9879_OUTPUTMODE_CTRL, reg_value);

        }
        else
        {
#if 1		// return to isr last value
            I2C_WRITE(MAX9879_OUTPUTMODE_CTRL, max9879_outmod_reg); 
#endif
        }

	return 0;
}

int max9879_i2c_speaker_onoff(int nOnOff)
{
	unsigned char reg_value;

	if (nOnOff)
		printk("MAX9879 speaker on\n");
	else
		printk("MAX9879 speaker off\n");
	
#if 0	// defined(CONFIG_SAMSUNG_TARGET)
		if(gpio_direction_output(PATH_MIC_SEL, rec_status ? 0 :nOnOff))
		{
			printk("%s:%d set MIC_SEL fail\n", __func__, __LINE__);
		}
//		printk("%s(%d) - MIC_SEL=%d.\n", __func__, nOnOff, gpio_get_value(PATH_MIC_SEL));
		mic_status = nOnOff;
#endif
	is_incallMode = 0;
	I2C_WRITE(MAX9879_INPUTMODE_CTRL,0x50); // dINA = 0(stereo), dINB = 1(mono)

	spk_vol = hpl_vol = hpr_vol = 0x1F;
	if( nOnOff )
	{
		I2C_READ(MAX9879_OUTPUTMODE_CTRL,&reg_value);

		if(audio_enabled)
		{
			reg_value &= 0xB0;
			reg_value |= 0x80;
		}
		else
		{
			reg_value &= 0x30;
		}

		reg_value |= 0x1F;	//for test	// 06 for max9879 speak LR Enable

		I2C_WRITE(MAX9879_OUTPUTMODE_CTRL, reg_value);
	}
	else
	{
            I2C_READ(MAX9879_OUTPUTMODE_CTRL,&reg_value);

		if(audio_enabled)
		{
			reg_value &= 0xB0;
			reg_value |= 0x80;
		}
		else
		{
			reg_value &= 0x30;
		}

		reg_value |= 0x00;		// 08

		I2C_WRITE(MAX9879_OUTPUTMODE_CTRL,reg_value); 
	}

	max9879_outmod_reg = reg_value;

	return 0;
}

int max9879_i2c_receiver_onoff(int nOnOff)
{
        unsigned char reg_value;

#if defined(CONFIG_SAMSUNG_TARGET)
		if(gpio_direction_output(PATH_MIC_SEL, rec_status ? 0 : !nOnOff))
		{
			printk("%s:%d set MIC_SEL fail\n", __func__, __LINE__);
		}
//		printk("%s(%d) - MIC_SEL=%d.\n", __func__, nOnOff, gpio_get_value(PATH_MIC_SEL));
		mic_status = nOnOff ? 0 : 1;	// reverse
#endif
        if( nOnOff )
        {
            I2C_READ(MAX9879_OUTPUTMODE_CTRL,&reg_value); 

            reg_value &= 0x0F; // Clear
    //        reg_value |= 0x40; // BYPASS_ON & earjack mode : prevent echo
            
            I2C_WRITE(MAX9879_OUTPUTMODE_CTRL,reg_value);
        }
        else
        {
            I2C_READ(MAX9879_OUTPUTMODE_CTRL,&reg_value);

            reg_value &= 0xBF; // BYPASS_OFF

            I2C_WRITE(MAX9879_OUTPUTMODE_CTRL,reg_value); 
        }
		max9879_outmod_reg = reg_value;

	return 0;
}

int max9879_i2c_headset_onoff(int nOnOff)
{
        unsigned char reg_value;

#if defined(CONFIG_SAMSUNG_TARGET)
		if(gpio_direction_output(PATH_MIC_SEL, rec_status ? 0 : !nOnOff))
		{
			printk("%s:%d set MIC_SEL fail\n", __func__, __LINE__);
		}
//		printk("%s(%d) - MIC_SEL=%d.\n", __func__, nOnOff, gpio_get_value(PATH_MIC_SEL));
		mic_status = nOnOff ? 0 : 1;	// reverse
#endif
#if 0
        spk_vol = 0x0A;
		hpl_vol = hpr_vol = 0x15; // 0x17(-9dB) -> 0x1C(-3dB) -> 0x1F(0dB) -> 0x15(-13dB)
#endif
		if(is_incallMode)
		{
			if(audio_enabled)
			{
				I2C_WRITE(MAX9879_INPUTMODE_CTRL,0x50); // dINA = 0(stereo), dINB = 1(mono)
			} else {
				I2C_WRITE(MAX9879_INPUTMODE_CTRL,0x70); // dINA = 1(mono), dINB = 1(mono)
			}
		} else {
			I2C_WRITE(MAX9879_INPUTMODE_CTRL,0x70); // dINA = 1(mono), dINB = 1(mono)
			is_incallMode = 1;
		}

        if( nOnOff )
        {
			I2C_READ(MAX9879_OUTPUTMODE_CTRL,&reg_value);

			reg_value &= 0xB0;
			reg_value |= 0x82;		// 08
	
			I2C_WRITE(MAX9879_OUTPUTMODE_CTRL,reg_value);
#if 0
			msleep(10);

			I2C_WRITE(MAX9879_SPKVOL_CTRL, spk_vol); // MAX 0x1F : 0dB

			I2C_WRITE(MAX9879_LEFT_HPHVOL_CTRL, hpl_vol); // 0x17(-9dB) -> 0x1C(-3dB) MAX 0x1F : 0dB -> 0x15(-13dB)

			I2C_WRITE(MAX9879_RIGHT_HPHVOL_CTRL, hpr_vol); // 0x17(-9dB) -> 0x1C(-3dB) MAX 0x1F : 0dB  -> 0x15(-13dB)
#endif
		}
		else
		{
			I2C_READ(MAX9879_OUTPUTMODE_CTRL,&reg_value);

			reg_value &= 0xB0;
			reg_value |= 0x84;		// 07

			I2C_WRITE(MAX9879_OUTPUTMODE_CTRL, reg_value);

#if 0
			I2C_WRITE(MAX9879_SPKVOL_CTRL, spk_vol); // MAX 0x1F : 0dB

			I2C_WRITE(MAX9879_LEFT_HPHVOL_CTRL, hpl_vol); // 0x17(-9dB) -> 0x1C(-3dB) MAX 0x1F : 0dB -> 0x15(-13dB)

			I2C_WRITE(MAX9879_RIGHT_HPHVOL_CTRL, hpr_vol); // 0x17(-9dB) -> 0x1C(-3dB) MAX 0x1F : 0dB  -> 0x15(-13dB)
#endif
		}
		max9879_outmod_reg = reg_value;

	return 0;
}



#undef I2C_WRITE
#undef I2C_READ

static int max9879_init_client(struct i2c_client *client)
{
	/* Initialize the MAX9879 Chip */
	init_waitqueue_head(&g_data_ready_wait_queue);
	return 0;
}

static struct file_operations max9879_fops = {
        .owner 	= THIS_MODULE,
        .open 	= max9879_open,
        .release = max9879_release,
        .unlocked_ioctl = max9879_ioctl,
};

static struct miscdevice max9879_device = {
        .minor 	= MISC_DYNAMIC_MINOR,
        .name 	= "max9879_amp",
        .fops 	= &max9879_fops,
};

static int max9879_probe(struct i2c_client *client, const struct i2c_device_id * dev_id)
{
	struct max9879_data *mt;
	int err = 0;
	printk(KERN_INFO "MAX9879: probe\n");
	if(!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		goto exit_check_functionality_failed;		
	
	if(!(mt = kzalloc( sizeof(struct max9879_data), GFP_KERNEL))) {
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}
	pclient = client;
	i2c_set_clientdata(client, mt);
	max9879_init_client(client);
	printk("[max9879_probe] max9879_probe->client name : %s\n",pclient->name);
	
	max9879_chip_init();
	
	/* Register a misc device */
	err = misc_register(&max9879_device);
	if(err) {
		printk(KERN_ERR "MAX9879_probe: misc_register failed \n");
		goto exit_misc_device_register_failed;
	}

#ifdef CONFIG_ANDROID_POWER
	mt->early_suspend.level = ANDROID_EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	mt->early_suspend.suspend =max9879_early_suspend;
	mt->early_suspend.resume = max9879_late_resume;
	android_register_early_suspend(&mt->early_suspend);
	if (android_power_is_driver_suspended())
		max9879_early_suspend(&mt->early_suspend);
#else
	init_suspend();
#endif    

	headset_on_work_queue = create_workqueue("headset_on");
	if (headset_on_work_queue == NULL) {
		goto exit_check_functionality_failed;
	}

	max9879_i2c_sensor_init();
	return 0;
	
exit_misc_device_register_failed:
exit_alloc_data_failed:
exit_check_functionality_failed:
	
	return err;
}

	
static int max9879_remove(struct i2c_client *client)
{
	struct max9879_data *mt = i2c_get_clientdata(client);
	free_irq(client->irq, mt);
#ifdef CONFIG_ANDROID_POWER
	android_unregister_early_suspend(&mt->early_suspend);
#else
	deinit_suspend();
#endif
	pclient = NULL;
	misc_deregister(&max9879_device);
	kfree(mt);
	destroy_workqueue(headset_on_work_queue);
	return 0;
}

void max9879_shutdown(struct i2c_client *client)
{
	int ret;
	max9879_i2c_write(MAX9879_SPKVOL_CTRL,spk_vol_mute);
	max9879_i2c_write(MAX9879_LEFT_HPHVOL_CTRL,hpl_vol_mute);
	max9879_i2c_write(MAX9879_RIGHT_HPHVOL_CTRL,hpr_vol_mute);

	ret = max9879_i2c_write(MAX9879_OUTPUTMODE_CTRL, 0x00);
	if (ret < 0 )
	{
		printk(KERN_ERR "MAX9879_suspend: MAX9879_i2c_write failed\n");
		return;
	}

	printk(KERN_EMERG "MAX9879_shutdown: success\n");
	return;
}

int max9879_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret; 
	unsigned char reg_value;

	return 0; //TEMP
   
	spk_vol_mute = hpl_vol_mute = hpr_vol_mute = 0;
	if(!audio_enabled && !is_incallMode) {
		max9879_i2c_write(MAX9879_SPKVOL_CTRL,spk_vol_mute);
		max9879_i2c_write(MAX9879_LEFT_HPHVOL_CTRL,hpl_vol_mute);
		max9879_i2c_write(MAX9879_RIGHT_HPHVOL_CTRL,hpr_vol_mute);
	
		ret = max9879_i2c_read(MAX9879_OUTPUTMODE_CTRL,&reg_value);
		if (ret < 0 )
		{
			printk(KERN_ERR "MAX9879_suspend: MAX9879_i2c_read failed\n");
			//return -EIO;
		}

		reg_value = reg_value & 0x7f; // SHDN = 0 MAX9879 shut down

		ret = max9879_i2c_write(MAX9879_OUTPUTMODE_CTRL, reg_value);
		if (ret < 0 )
		{
			printk(KERN_ERR "MAX9879_suspend: MAX9879_i2c_write failed\n");
			//return -EIO;
		}

		printk("MAX9879_suspend: success\n");
	}

    return 0;
}

int max9879_legacy_suspend(void)
{
	int ret; 
	unsigned char reg_value;

	return 0; //TEMP
	
	spk_vol_mute = hpl_vol_mute = hpr_vol_mute = 0;
	if(!audio_enabled && !is_incallMode) {
		max9879_i2c_write(MAX9879_SPKVOL_CTRL,spk_vol_mute);
		max9879_i2c_write(MAX9879_LEFT_HPHVOL_CTRL,hpl_vol_mute);
		max9879_i2c_write(MAX9879_RIGHT_HPHVOL_CTRL,hpr_vol_mute);
	
		ret = max9879_i2c_read(MAX9879_OUTPUTMODE_CTRL,&reg_value);
		if (ret < 0 )
		{
			printk(KERN_ERR "MAX9879_suspend: MAX9879_i2c_read failed\n");
			//return -EIO;
		}

		reg_value = reg_value & 0x7f; // SHDN = 0 MAX9879 shut down

		ret = max9879_i2c_write(MAX9879_OUTPUTMODE_CTRL, reg_value);
		if (ret < 0 )
		{
			printk(KERN_ERR "MAX9879_suspend: MAX9879_i2c_write failed\n");
			//return -EIO;
		}

		printk("MAX9879_suspend: success\n");
	}

    return 0;
}
	
int max9879_resume(struct i2c_client *client)
{
    int ret; 
    unsigned char reg_value;

	ret = max9879_i2c_read(MAX9879_OUTPUTMODE_CTRL,&reg_value);
	if (ret < 0 )
	{
		printk(KERN_ERR "MAX9879_resume: MAX9879_i2c_read failed\n");
		//return -EIO;
	}
	if(audio_enabled && !(reg_value & 0x80))
	{
		reg_value = reg_value | 0x80; // SHDN = 1 MAX9879 wakeup

		ret = max9879_i2c_write(MAX9879_OUTPUTMODE_CTRL, reg_value);
		if (ret < 0 )
		{
			printk(KERN_ERR "MAX9879_resume: MAX9879_i2c_write failed\n");
			//return -EIO;
		}

		msleep(10); // 10m startup time delay

	}
	max9879_i2c_write(MAX9879_SPKVOL_CTRL,spk_vol);
	max9879_i2c_write(MAX9879_LEFT_HPHVOL_CTRL,hpl_vol);
	max9879_i2c_write(MAX9879_RIGHT_HPHVOL_CTRL,hpr_vol);

	printk("MAX9879_resume: success\n");

    return 0;
}

int max9879_legacy_resume(void)
{
    int ret; 
    unsigned char reg_value;

	ret = max9879_i2c_read(MAX9879_OUTPUTMODE_CTRL,&reg_value);
	if (ret < 0 )
	{
		printk(KERN_ERR "MAX9879_resume: MAX9879_i2c_read failed\n");
		//return -EIO;
	}
	if(audio_enabled && !(reg_value & 0x80))
	{
		reg_value = reg_value | 0x80; // SHDN = 1 MAX9879 wakeup

		ret = max9879_i2c_write(MAX9879_OUTPUTMODE_CTRL, reg_value);
		if (ret < 0 )
		{
			printk(KERN_ERR "MAX9879_resume: MAX9879_i2c_write failed\n");
			//return -EIO;
		}

		msleep(10); // 10m startup time delay

	}
	max9879_i2c_write(MAX9879_SPKVOL_CTRL,spk_vol);
	max9879_i2c_write(MAX9879_LEFT_HPHVOL_CTRL,hpl_vol);
	max9879_i2c_write(MAX9879_RIGHT_HPHVOL_CTRL,hpr_vol);

	printk("MAX9879_resume: success\n");

    return 0;
}

#ifdef CONFIG_ANDROID_POWER
static void max9879_early_suspend(struct android_early_suspend *h)
{
	struct max9879_data *mt;
	mt = container_of(h, struct max9879_data, early_suspend);
	max9879_suspend();
}

static void max9879_late_resume(struct android_early_suspend *h)
{
	struct max9879_data *mt;
	mt = container_of(h, struct max9879_data, early_suspend);
	max9879_resume();
}
#endif

static const struct i2c_device_id max9879_id[] = {
	{ "max9879_i2c", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, MAX9879_id);


static struct i2c_driver max9879_driver = {
	.id_table	= max9879_id,
	.probe = max9879_probe,
	.remove = max9879_remove,
#ifndef CONFIG_ANDROID_POWER
	.suspend	= max9879_suspend,
	.resume	= max9879_resume,
#endif
	.shutdown = max9879_shutdown,
	.driver = {		
		.name   = "max9879_i2c",
	},
};

static int __init max9879_init(void)
{
	int ret; 

	ret = i2c_add_driver(&max9879_driver);
	if (ret < 0 )
	{
		printk(KERN_ERR "max9879_init: i2c_add_driver failed\n");
		//return -EIO;
	}
	
	return ret;
}

static void __exit max9879_exit(void)
{
	i2c_del_driver(&max9879_driver);
}

EXPORT_SYMBOL(max9879_i2c_speaker_onoff);
EXPORT_SYMBOL(max9879_i2c_receiver_onoff);
EXPORT_SYMBOL(max9879_i2c_headset_onoff);
EXPORT_SYMBOL(max9879_i2c_speaker_headset_onoff);
EXPORT_SYMBOL(max9879_legacy_suspend);
EXPORT_SYMBOL(max9879_legacy_resume);
EXPORT_SYMBOL(max9879_gpio_recording_start);

module_init(max9879_init);
module_exit(max9879_exit);

MODULE_AUTHOR("");
MODULE_DESCRIPTION("MAX9879 Driver");
MODULE_LICENSE("GPL");

