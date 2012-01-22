/*
  SEC M5MO
 */
/***************************************************************
CAMERA DRIVER FOR 5M CAM(FUJITSU M4Mo) by PGH
ver 0.1 : only preview (base on universal)
****************************************************************/

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include <mach/camera.h>
#include <mach/vreg.h>
#include <linux/vmalloc.h>


#include "sec_m5mo.h"

#include <asm/gpio.h> 

#if 1//Mclk_timing for M4Mo spec.
#include <linux/clk.h>
#include <linux/io.h>
#include <mach/board.h>
#endif

//#define FORCE_FIRMWARE_UPDATE

static int m5mo_reset_for_update(void);
static int m5mo_load_fw(int location);
static int firmware_update_mode = 0;

//#define EARLY_PREVIEW_AFTER_CAPTURE

#define	CAMERA_DEBUG	
#ifdef CAMERA_DEBUG
#define CAM_DEBUG(fmt, arg...)	\
		do{\
		printk("\033[[M5MO] %s:%d: " fmt "\033[0m\n", __FUNCTION__, __LINE__, ##arg);}\
		while(0)
#else
#define CAM_DEBUG(fmt, arg...)	
#endif

#define CHECK_ERR(x)   if ((x) < 0) { \
							printk("i2c falied, err %d\n", x); \
							return x; \
						}

static struct vreg *vreg_CAM_AF28;

//#if defined(CONFIG_SAMSUNG_TARGET)
#if 1
static struct i2c_client *cam_pm_lp8720_pclient; 

struct cam_pm_lp8720_data {
	struct work_struct work;
#if 0//PGH
	#ifdef CONFIG_ANDROID_POWER
		struct android_early_suspend early_suspend;
	#endif
#endif//PGH
};

static DECLARE_WAIT_QUEUE_HEAD(cam_pm_lp8720_wait_queue);
DECLARE_MUTEX(cam_pm_lp8720_sem);

#endif//PGH FOR CAM PM LP8720 ////////////////////////////////////


struct m5mo_work_t {
	struct work_struct work;
};

static struct  m5mo_work_t *m5mo_sensorw;
static struct  i2c_client *m5mo_client;

struct m5mo_picture_size m5mo_picture_sizes[] = {
			{3264,2448,M5MO_SHOT_8M_SIZE},
			{3264,1960,M5MO_SHOT_6M_2_SIZE},
			{2560,1920,M5MO_SHOT_5M_SIZE},
			{2560,1536,M5MO_SHOT_4M_2_SIZE},
			{1600,1200,M5MO_SHOT_2M_SIZE},
			{1600,960,M5MO_SHOT_W1_5_SIZE},
			{640,480,M5MO_SHOT_VGA_SIZE},
			{800,480,M5MO_SHOT_WVGA_SIZE}			
};

#define	M5MO_ZOOM_LEVEL_MAX	31
static unsigned int m5mo_zoom_table[M5MO_ZOOM_LEVEL_MAX] 
      = {  1,  
           2, 3, 5, 7, 9,11,13,15,17,20,
          21,22,23,24,25,26,27,28,29,30,
          31,32,33,34,35,36,37,38,39,40};

static struct m5mo_ctrl_t *m5mo_ctrl;
static struct m5mo_exif_data * m5mo_exif;

static DECLARE_WAIT_QUEUE_HEAD(m5mo_wait_queue);
DECLARE_MUTEX(m5mo_sem);


#if 0 //not used
static int m5mo_reset(struct msm_camera_sensor_info *dev)
{
	int rc = 0;

	rc = gpio_request(dev->sensor_reset, "m5mo");

	if (!rc) {
		rc = gpio_direction_output(dev->sensor_reset, 0);
		msleep(20);
		rc = gpio_direction_output(dev->sensor_reset, 1);
	}

	gpio_free(dev->sensor_reset);
	return rc;
}
#endif

static int32_t m5mo_i2c_txdata(unsigned short saddr, char *txdata, int length)
{
	unsigned long	flags;

	struct i2c_msg msg[] = {
	{
		.addr = saddr,
		.flags = 0,
		.len = length,
		.buf = txdata,
	},
	};

	local_irq_save(flags);


	if (i2c_transfer(m5mo_client->adapter, msg, 1) < 0) {
		printk("m5mo_i2c_txdata failed\n");
		local_irq_restore(flags);
		return -EIO;
	}

	local_irq_restore(flags);
	return 0;
}



static int32_t m5mo_i2c_write_8bit(unsigned char category, unsigned char byte, unsigned char value)
{
	int32_t rc = -EFAULT;
	char buf[5] = {5, 2, category, byte, value & 0xFF};
	int i;
//	int read_value = 3;

	//CAM_DEBUG("START");
//	CAM_DEBUG(" category = 0x%x, byte = 0x%x value = 0x%x!\n", category, byte, value);

	for(i=0; i<10; i++)
	{
		rc = m5mo_i2c_txdata(0x1F, buf, 5); //1F=3E>>1=M5MO's ADDRESS,  5=>buf's length

		if(rc == 0)
		{
		//	msleep(2);
		//	printk("i2c_write success, category = 0x%x, byte = 0x%x value = 0x%x!\n", category, byte, value);
			return rc;
		}

		else
		{
			printk("i2c_write failed, category = 0x%x, byte = 0x%x value = 0x%x!\n", category, byte, value);
/*
			for(j=0; j<10; j++)
			{
				msleep(j*20); // delay first, on the purpose
				read_value = gpio_get_value(60);
				
				if(read_value == 1) //SCL IS FINE
				{
					printk("DC ACT i2c_write_8bit, SCL IS FINE\n");
					break;
				}
				else
				{
					printk("current SCL : %d now wating %dmsec\n", read_value, j*20);
				}

		}
*/
	}

	}

	return rc;
}



#if 1//PGH 
int32_t m5mo_i2c_write_8bit_external(unsigned char category, unsigned char byte, unsigned char value)
{
	int32_t rc = -EFAULT;
	char buf[5] = {5, 2, category, byte, value & 0xFF};
	int i;

	CAM_DEBUG("START");

	for(i=0; i<10; i++)
	{
	rc = m5mo_i2c_txdata(0x1F, buf, 5); //1F=3E>>1=M5MO's ADDRESS,  5=>buf's length

		if(rc == 0)
		{
			msleep(2);
			return rc;
		}

		else
		{
			msleep(i*2);
			printk("i2c_write failed, category = 0x%x, byte = 0x%x value = 0x%x!\n", category, byte, value);
		}
	}

	return rc;

}
#endif//PGH 

static int32_t m5mo_i2c_write_16bit(unsigned char category, unsigned char byte, short value)
{
	int32_t rc = -EFAULT;
	char buf[6] = {6, 2, category, byte, 0,0};
	int i;

	//CAM_DEBUG("START");
	
	buf[4] = (value & 0xFF00) >>8;
	buf[5] = (value & 0x00FF);
	

	for(i=0; i<10; i++)
	{
	rc = m5mo_i2c_txdata(0x1F, buf, 6); //1F=3E>>1=M5MO's ADDRESS,  6=>buf's length

		if(rc == 0)
		{
//			msleep(2);
			return rc;
		}
		else
		{
//			msleep(i*2);
			printk("i2c_write failed, category = 0x%x, byte = 0x%x value = 0x%x!\n", category, byte, value);
		}
	}


//	msleep(20);

	return rc;
}


static int32_t m5mo_i2c_write_32bit(unsigned char category, unsigned char byte, int value)
{
	int32_t rc = -EFAULT;
	char buf[8] = {8, 2, category, byte, 0,0,0,0};
	int i;

	//CAM_DEBUG("START");
	
	buf[4] = (value & 0xFF000000) >>24;
	buf[5] = (value & 0x00FF0000) >>16;
	buf[6] = (value & 0x0000FF00) >> 8;
	buf[7] = (value & 0x000000FF);

	for(i=0; i<10; i++)
	{
	rc = m5mo_i2c_txdata(0x1F, buf, 8);

		if(rc == 0)
		{
			msleep(2);
			return rc;
		}
		else
		{
			msleep(i*2);
			printk("i2c_write failed, category = 0x%x, byte = 0x%x value = 0x%x!\n", category, byte, value);
		}

	}

//	msleep(20);


	return rc;
}

static int32_t m5mo_i2c_write_memory(int address, short size, char *value)
{
	int32_t rc = -EFAULT;
	char *buf;
	int i;

	CAM_DEBUG("START");

	buf = kmalloc(8 + size, GFP_KERNEL);
	if(buf <0) {
		printk("[PGH] m5mo_i2c_write_memory  memory alloc fail!\n");
		return -1;
	}

	buf[0] = 0x00;
	buf[1] = 0x04;

	buf[2] = (address & 0xFF000000) >> 24;
	buf[3] = (address & 0x00FF0000) >> 16;
	buf[4] = (address & 0x0000FF00) >> 8;
	buf[5] = (address & 0x000000FF) ;

	buf[6] = (size & 0xFF00) >> 8;
	buf[7] = (size & 0x00FF);

	memcpy(buf+8 , value, size);


	for(i=0; i<10; i++)
	{
	rc = m5mo_i2c_txdata(0x1F, buf,  8 + size);

		if(rc == 0)
		{
			msleep(5);
	kfree(buf);
			return rc;
		}
		
		else
		{
			msleep(i*2);
			printk("m5mo_i2c_write_memory fail!\n");
		}
	}


	msleep(20);

	kfree(buf);
	return rc;
}

static inline int m5mo_i2c_read(unsigned char len, unsigned char category, unsigned char byte, unsigned int *val)
{
	int rc = 0;

	unsigned char tx_buf[5];	
	unsigned char rx_buf[len + 1];

	*val = 0;

	struct i2c_msg msg = {
		.addr = m5mo_client->addr,
		.flags = 0,		
		.len = 5,		
		.buf = tx_buf,	
	};


	tx_buf[0] = 0x05;	//just protocol
	tx_buf[1] = 0x01;	//just protocol
	tx_buf[2] = category;
	tx_buf[3] = byte;
	tx_buf[4] = len;


	rc = i2c_transfer(m5mo_client->adapter, &msg, 1);
	if (likely(rc == 1)) 
	{
		msg.flags = I2C_M_RD;		
		msg.len = len + 1;		
		msg.buf = rx_buf;		
		rc = i2c_transfer(m5mo_client->adapter, &msg, 1);	
	} 
	else
	{		
		printk(KERN_ERR "M5MO: %s: failed at category=%x,  byte=%x  \n", __func__,category,  byte);		
		return -EIO;	
	}


	if (likely(rc == 1)) 
	{		
		if (len == 1)			
			*val = rx_buf[1];		
		else if (len == 2)			
			*(unsigned short *)val = be16_to_cpu(*(unsigned short *)(&rx_buf[1]));		
		else			
			*val = be32_to_cpu(*(unsigned int *)(&rx_buf[1]));		

		return 0;	
	}	


		printk(KERN_ERR "M5MO: %s: 2  failed at category=%x,  byte=%x  \n", __func__,category,  byte);		

	return -EIO;

}

static int 
m5mo_i2c_verify(unsigned char category, unsigned char byte, unsigned char value)
{
	unsigned int val = 0;
	unsigned char 	i;


	for(i= 0; i < M5MO_I2C_VERIFY_RETRY; i++) 
	{
		m5mo_i2c_read(1, category, byte, &val);
		msleep(10);
		if((unsigned char)val == value) 
		{
			CAM_DEBUG("[m5mo] i2c verified [c,b,v] [%02x, %02x, %02x] (try = %d)\n", category, byte, value, i);
			return 0;
		}
	}

	printk("m5mo_i2c_verify Failed !! \n");
	return -EBUSY;
}

static int wait_interrupt_mode(int mode)
{
//	int rc = 0;
	int wait_count = 200;
	unsigned short value;
	do
	{
		m5mo_i2c_read(1, 0x00, 0x10, &value);
		msleep(10);
		CAM_DEBUG(" (%d) Wait interrupt mode : %d (0x10 regs value = 0x%x)",wait_count,mode,value);
	}while((--wait_count) > 0 && !(value & mode));

	return wait_count;
}

//#if defined(CONFIG_SAMSUNG_TARGET)
#if 1
int cam_pm_lp8720_i2c_tx_data(char* txData, int length)
{
	int rc; 

	struct i2c_msg msg[] = {
		{
			.addr = cam_pm_lp8720_pclient->addr,  
			.flags = 0,
			.len = length,
			.buf = txData,		
		},
	};
    
	rc = i2c_transfer(cam_pm_lp8720_pclient->adapter, msg, 1);
	if (rc < 0) {
		printk(KERN_ERR "cam_pm_lp8720: cam_pm_lp8720_i2c_tx_data error %d\n", rc);
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

static int cam_pm_lp8720_i2c_write(unsigned char u_addr, unsigned char u_data)
{
	int rc;
	unsigned char buf[2];

	CAM_DEBUG("START");

	buf[0] = u_addr;
	buf[1] = u_data;
    
	rc = cam_pm_lp8720_i2c_tx_data(buf, 2);

	if(rc < 0)
		printk(KERN_ERR "cam_pm_lp8720 : txdata error %d add:0x%02x data:0x%02x\n", rc, u_addr, u_data);

	CAM_DEBUG("SUCCESS");
	
	return rc;	
}


#if 0 //not used
static int cam_pm_lp8720_i2c_rx_data(char* rxData, int length)
{
	int rc;

	struct i2c_msg msgs[] = {
		{
			.addr = cam_pm_lp8720_pclient->addr,
			.flags = 0,      
			.len = 1,
			.buf = rxData,
		},
		{
			.addr = cam_pm_lp8720_pclient->addr,
			.flags = I2C_M_RD|I2C_M_NO_RD_ACK,  //CHECK!!
			.len = length,
			.buf = rxData,
		},
	};

	rc = i2c_transfer(cam_pm_lp8720_pclient->adapter, msgs, 2);
      
	if (rc < 0) {
		printk(KERN_ERR "cam_pm_lp8720: cam_pm_lp8720_i2c_rx_data error %d\n", rc);
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


static int cam_pm_lp8720_i2c_read(unsigned char u_addr, unsigned char *pu_data)
{
	int rc;
	unsigned char buf;

	buf = u_addr;
	rc = cam_pm_lp8720_i2c_rx_data(&buf, 1);
	if (!rc)
		*pu_data = buf;
	else printk(KERN_ERR "cam_pm_lp8720: i2c read failed\n");
	return rc;	
}
#endif
#endif//PGH FOR CAM PM LP8720 /////////////////////////////////////////////////////////////////


static int
m5mo_set_ae_awb(int lock)
{	
	CAM_DEBUG(" lock : %d ",lock);

	if(lock)
	{
	}
	else
	{
		m5mo_i2c_write_8bit(0x03, 0x00, 0x00); 
		m5mo_i2c_write_8bit(0x06, 0x00, 0x00);
	}
}

static int
m5mo_set_dtp(int* onoff)
{	
	unsigned int value=0;
	CAM_DEBUG(" onoff : %d ",*onoff);
	m5mo_i2c_read(1, 0x00, 0x0B, &value);
	CAM_DEBUG(" Current mode : %s(%d)",(value==1)?"ParamSet":"Monitor",value);
	switch((*onoff))
	{
		case M5MO_DTP_OFF:
			m5mo_i2c_write_8bit(0x0D, 0x1B, 0x00);

			/* set ACK value */
			(*onoff) = M5MO_DTP_OFF_ACK;
			
			break;
		case M5MO_DTP_ON:
			/* change to paramset mode */
			m5mo_i2c_write_8bit(0x00, 0x0B, M5MO_PARMSET_MODE);
			m5mo_i2c_verify(0x00, 0x0B, M5MO_PARMSET_MODE);
	
			/* MON_SIZE(QVGA size) */
			m5mo_i2c_write_8bit(0x01, 0x01, 0x09);
			/* YUVOUT_MAIN(YUV422) */
			m5mo_i2c_write_8bit(0x0B, 0x00, 0x00);
			/* MAIN_IMAGE_SIZE(QVGA) */
			m5mo_i2c_write_8bit(0x0B, 0x01, 0x02);
			/* PREVIEW_IMAGE_SIZE(QVGA size) */
			m5mo_i2c_write_8bit(0x0B, 0x06, 0x01);
			/* MON_FPS(AUTO) */
			m5mo_i2c_write_8bit(0x01, 0x02, 0x01);

			m5mo_i2c_write_8bit( 0x00, 0x11, M5MO_INT_MODE);
			/* change to monitor mode */
			m5mo_i2c_write_8bit(0x00, 0x0B, M5MO_MONITOR_MODE);
			m5mo_i2c_verify(0x00, 0x0B, M5MO_MONITOR_MODE);
			
			wait_interrupt_mode(M5MO_INT_MODE);

			/* Start output test data */
			m5mo_i2c_write_8bit(0x0D, 0x1B, 0x01);

			m5mo_ctrl->settings.preview_size_idx = 0xFF;

			/* set ACK value */
			(*onoff) = M5MO_DTP_ON_ACK;
			break;
		default:
			printk("[DTP]Invalid DTP mode!!!\n");
			return -EINVAL;
	}
	
	return 0;
}

static int
m5mo_set_face_detection(int mode)
{
	CAM_DEBUG(" mode : %d \n",mode);
	switch(mode)
	{
		case M5MO_FACE_DETECTION_OFF:
			m5mo_i2c_write_8bit(0x09, 0x00, 0x00);
			break;
		case M5MO_FACE_DETECTION_ON:
			m5mo_i2c_write_8bit(0x09, 0x00, 0x11);
			break;
		default:
			printk("[Face Detection]Invalid face detect mode!!!\n");
			return -EINVAL;
	}
	
	return 0;
}

static int
m5mo_set_wdr(int wdr)
{
	CAM_DEBUG(" wdr : %d \n",wdr);

	switch(wdr)
	{
		case M5MO_WDR_ON : 
			m5mo_i2c_write_8bit(0x02, 0x25, 0x09);
			m5mo_i2c_write_8bit(0x0B, 0x2C, 0x01);
			break;
		case M5MO_WDR_OFF :			
			m5mo_i2c_write_8bit(0x0B, 0x2C, 0x00);
			m5mo_i2c_write_8bit(0x02, 0x25, 0x05);
			break;
		default:
			printk("[WDR] Invalid wdr value !!!\n");
			return -EINVAL;
			
	}
	return 0;
}

static int
m5mo_set_isc(int value)
{
	/* WDR & ISC are orthogonal!*/
	switch(value)
	{
		case M5MO_ISC_STILL_OFF:
			m5mo_i2c_write_8bit(0x0C, 0x00, 0x00);
			break;
		case M5MO_ISC_STILL_ON:
			m5mo_i2c_write_8bit(0x0C, 0x00, 0x02);
			break;
		case M5MO_ISC_STILL_AUTO:
			m5mo_i2c_write_8bit(0x0C, 0x00, 0x02);
			break;
		default:
			printk("[ISC] Invalid isc value !!!\n");
			return -EINVAL;
	}

	return 0;
}

static int 
m5mo_set_sharpness(int sharpness)
{
	CAM_DEBUG(" sharpness : %d \n",sharpness);
#if 0
	CAM_DEBUG("set Sharp EN\n");
	m5mo_i2c_write_8bit(0x02, 0x16, 0x01);
			
	m5mo_i2c_write_8bit(0x02, 0x18, (unsigned char)value);
#endif
	switch(sharpness)
	{
		case M5MO_SHARPNESS_MINUS_2:
			m5mo_i2c_write_8bit( 0x02, 0x11, 0x03);
			break;
		case M5MO_SHARPNESS_MINUS_1:
			m5mo_i2c_write_8bit( 0x02, 0x11, 0x04);
			break;
		case M5MO_SHARPNESS_DEFAULT:
			m5mo_i2c_write_8bit( 0x02, 0x11, 0x05);
			break;
		case M5MO_SHARPNESS_PLUS_1:
			m5mo_i2c_write_8bit( 0x02, 0x11, 0x06);
			break;
		case M5MO_SHARPNESS_PLUS_2:
			m5mo_i2c_write_8bit( 0x02, 0x11, 0x07);
			break;
		default:
			printk("[sharpness] Invalid  sharpness value!!!\n");
			return -EINVAL;
	}

	m5mo_ctrl->settings.sharpness= sharpness;
	return 0;
}

static int 
m5mo_set_exposure(int value)
{
	CAM_DEBUG(" value : %d \n",value);

	return 0;
}

#if 0
static int
m5mo_set_mode(int mode)
{
	/* Set Monitor Mode */
	m5mo_i2c_write_8bit(0x00, 0x0B, M5MO_MONITOR_MODE);
	msleep(5);			

	/* AE/AWB Unlock */
	m5mo_i2c_write_8bit(0x03, 0x00, 0x00); 
	m5mo_i2c_write_8bit(0x06, 0x00, 0x00);

	/* wait interrupt - INT_STATUS_MODE */
	do
	{
		m5mo_i2c_read(1, 0x00, 0x10, &value);
		msleep(50);
		printk(" Wait INT_STATUS_MODE ... %d (0x10 regs value = 0x%x)\n",wait_count,value);
	}while((--wait_count) > 0 && (value != M5MO_INT_MODE));
}
#endif



			
int find_picture_size_value(int w, int h)
{
	int i = 0;
	for( ; i<sizeof(m5mo_picture_sizes) ; i++)
	{
		if((m5mo_picture_sizes[i].width == w) && (m5mo_picture_sizes[i].height== h))
		{
			return i;
		}
	}

	return -1;
}
static int
m5mo_set_picture_size(
	 u32 width, u32 height)
{

	int size_idx = 0,size_val = 0;
	size_idx = find_picture_size_value(width, height);
	size_val = m5mo_picture_sizes[size_idx].reg_val;
	
	CAM_DEBUG(" picture size : %d \n",size_val);
	
	switch(size_val)
	{
		case M5MO_SHOT_8M_SIZE:
			CAM_DEBUG("8M size : 3200X2400\n");
			m5mo_i2c_write_8bit(0x0B, 0x01, M5MO_SHOT_8M_SIZE);
			break;
		case M5MO_SHOT_6M_2_SIZE:
			CAM_DEBUG("SVGA size : 3264x1960\n");
			m5mo_i2c_write_8bit(0x0B, 0x01,M5MO_SHOT_6M_2_SIZE);
			break;
		case M5MO_SHOT_5M_SIZE:
			CAM_DEBUG("SVGA size : 2560x1920\n");
			m5mo_i2c_write_8bit(0x0B, 0x01,M5MO_SHOT_5M_SIZE);
			break;
		case M5MO_SHOT_4M_2_SIZE:
			CAM_DEBUG("W4M size : 2560x1536\n");
			m5mo_i2c_write_8bit(0x0B, 0x01,M5MO_SHOT_4M_2_SIZE);
			break;
		case M5MO_SHOT_3M_2_SIZE:
			CAM_DEBUG("3M size : 2048x1536\n");
			m5mo_i2c_write_8bit(0x0B, 0x01, M5MO_SHOT_3M_2_SIZE);
			break;
		case M5MO_SHOT_3M_1_SIZE:
			CAM_DEBUG("W4M size : 2048x1152\n");
			m5mo_i2c_write_8bit(0x0B, 0x01,M5MO_SHOT_3M_1_SIZE);
			break;
		case M5MO_SHOT_FULL_HD_SIZE:
			CAM_DEBUG("2M size : 1920x1080\n");
			m5mo_i2c_write_8bit(0x0B, 0x01, M5MO_SHOT_FULL_HD_SIZE);
			break;
		case M5MO_SHOT_2M_SIZE:
			CAM_DEBUG("2M size : 1600x1200\n");
			m5mo_i2c_write_8bit(0x0B, 0x01, M5MO_SHOT_2M_SIZE);
			break;	
		case M5MO_SHOT_W1_5_SIZE:
			CAM_DEBUG("W1.5M size : 1600x960\n");
			m5mo_i2c_write_8bit(0x0B, 0x01, M5MO_SHOT_W1_5_SIZE);
			break;
		case M5MO_SHOT_WVGA_SIZE:
			CAM_DEBUG("WVGA size : 800x480\n");
			m5mo_i2c_write_8bit(0x0B, 0x01, M5MO_SHOT_WVGA_SIZE);
			break;
		case M5MO_SHOT_VGA_SIZE:
			CAM_DEBUG("VGA size : 640x480\n");
			m5mo_i2c_write_8bit(0x0B, 0x01, M5MO_SHOT_VGA_SIZE);
			break;
		
		default:
			printk("[picture size] Invalid size value !!!\n");
			return -EINVAL;
	}

	m5mo_ctrl->settings.capture_size = size_idx;
	
	return 0;
}


static int
m5mo_set_preview_size(
	 int8_t size)
{
	unsigned int value=0;
	
	CAM_DEBUG("##### preview size : %d #####\n",size);

	/* change to paramset mode */
	m5mo_i2c_write_8bit(0x00, 0x0B, M5MO_PARMSET_MODE);
	m5mo_i2c_verify(0x00, 0x0B, M5MO_PARMSET_MODE);

	if(m5mo_ctrl->settings.preview_size_idx == size) {
		CAM_DEBUG("same preview size");
		return 0;
	}
	
	switch(size)
	{
		case M5MO_VGA_SIZE:
		case PREVIEW_NORMAL:
			CAM_DEBUG("VGA size\n");
			m5mo_i2c_write_8bit(0x01, 0x01, 0x17);
			m5mo_ctrl->settings.preview_size.width = 640;
			m5mo_ctrl->settings.preview_size.height = 480;
			break;
		case M5MO_WVGA_SIZE:
		case PREVIEW_WIDE:
			CAM_DEBUG("WVGA size\n");
			m5mo_i2c_write_8bit(0x01, 0x01, 0x1A);
			m5mo_ctrl->settings.preview_size.width = 800;
			m5mo_ctrl->settings.preview_size.height = 480;
			break;
		case M5MO_FULL_HD_SIZE:
		case PREVIEW_MAX:	
			CAM_DEBUG("720P size\n");
			m5mo_i2c_write_8bit(0x01, 0x01, 0x21);
			m5mo_ctrl->settings.preview_size.width = 1280;
			m5mo_ctrl->settings.preview_size.height = 720;
			break;
		default:
			printk("[preview size] Invalid size value !!!\n");
			return -EINVAL;
	}
	/* change to monitor mode */
	/* Interrupt Clear */
	m5mo_i2c_read( 1,0x00, 0x10, &value);
	CAM_DEBUG("INT STATUS(0x10 ) = 0x%x\n",value);
	/* Enable YUV-Output Interrupt */
	m5mo_i2c_write_8bit( 0x00, 0x11, M5MO_INT_MODE);
	
	/* change to monitor mode */
	m5mo_i2c_write_8bit(0x00, 0x0B, M5MO_MONITOR_MODE);
	m5mo_i2c_verify(0x00, 0x0B, M5MO_MONITOR_MODE);

	wait_interrupt_mode(M5MO_INT_MODE);
	m5mo_ctrl->settings.preview_size_idx = size;
	//m5mo_ctrl->settings.preview_size = size;
	
	return 0;
}


static int 
m5mo_set_jpeg_quality(
	unsigned int jpeg_quality)
{

	CAM_DEBUG(" jpeg_quality : %d \n",jpeg_quality);
	
//	if(m5mo_ctrl->settings.jpeg_quality == jpeg_quality)
//		return 0;
	switch(jpeg_quality)
	{
		case M5MO_JPEG_SUPERFINE:
			CAM_DEBUG("SuperFine");
			m5mo_i2c_write_8bit(0x0B, 0x17, 0x62);
			m5mo_i2c_write_8bit(0x0B, 0x34, 0x00);
			break;
		case M5MO_JPEG_FINE:
			CAM_DEBUG("Fine");
			m5mo_i2c_write_8bit( 0x0B, 0x17, 0x62);
			m5mo_i2c_write_8bit( 0x0B, 0x34, 0x05);
			break;
		case M5MO_JPEG_NORMAL:
			CAM_DEBUG("Normal");
			m5mo_i2c_write_8bit( 0x0B, 0x17, 0x62);
			m5mo_i2c_write_8bit( 0x0B, 0x34, 0x0A);
			break;
		case M5MO_JPEG_ECONOMY:
			CAM_DEBUG("Economy");
			m5mo_i2c_write_8bit( 0x0B, 0x17, 0x62);
			m5mo_i2c_write_8bit( 0x0B, 0x34, 0x0F);
			break;
		default:
			printk("[JPEG Quality]Invalid value is ordered!!!\n");
			return -EINVAL;
	}

	m5mo_ctrl->settings.jpeg_quality = jpeg_quality;

	return 0;
}


static m5mo_set_exif_data()
{
	u32 read_1,read_2;
	CAM_DEBUG("start");

	/* exposure time*/
	m5mo_i2c_read(4, 0x07, 0x00, &read_1);
	m5mo_i2c_read(4, 0x07, 0x04, &read_2);
	m5mo_exif->ep_numerator = read_1;
	m5mo_exif->ep_denominator = read_2;
	CAM_DEBUG("ep : 0x%x, 0x%x",read_1, read_2);
	
	/* shutter speed */
	m5mo_i2c_read(4, 0x07, 0x08, &read_1);
	m5mo_i2c_read(4, 0x07, 0x0C, &read_2);
	m5mo_exif->tv_numerator = read_1;
	m5mo_exif->tv_denominator = read_2;
	CAM_DEBUG("tv : 0x%x, 0x%x",read_1, read_2);

	/* aperture */
	m5mo_i2c_read(4, 0x07, 0x10, &read_1);
	m5mo_i2c_read(4, 0x07, 0x14, &read_2);
	m5mo_exif->av_numerator = read_1;
	m5mo_exif->av_denominator = read_2;
	CAM_DEBUG("av : 0x%x, 0x%x",read_1, read_2);

	/* brightness */
	m5mo_i2c_read(4, 0x07, 0x18, &read_1);
	m5mo_i2c_read(4, 0x07, 0x1C, &read_2);
	m5mo_exif->bv_numerator = read_1;
	m5mo_exif->bv_denominator = read_2;
	CAM_DEBUG("bv : 0x%x, 0x%x",read_1, read_2);

	/* exposure bias */
	m5mo_i2c_read(4, 0x07, 0x20, &read_1);
	m5mo_i2c_read(4, 0x07, 0x24, &read_2);
	m5mo_exif->ebv_numerator = read_1;
	m5mo_exif->ebv_denominator = read_2;
	CAM_DEBUG("ebv : 0x%x, 0x%x",read_1, read_2);

	/* ISO */
	m5mo_i2c_read(2, 0x07, 0x28, &read_1);
	m5mo_exif->iso = read_1;
	CAM_DEBUG("iso : 0x%x",read_1 );
	
	/* Flash */
	m5mo_i2c_read(2, 0x07, 0x2A, &read_1);
	m5mo_exif->flash = read_1;
	CAM_DEBUG("flash : 0x%x",read_1);
	CAM_DEBUG("end");
	return 0;
}

static int  m5mo_get_exif(int32_t *read_1, int32_t *read_2)
{
	unsigned int param_ver = 0;
	/* read test */
	/* parameter version - 5055 */
	m5mo_i2c_read(2, 0x00, 0x06, &param_ver);
	CAM_DEBUG("Read Test in m5mo_get_exif : Param Version - %d",param_ver);
#if 0	
	switch(*read_1){
		
	case EXIF_EXPOSURE_TIME:
		{
			m5mo_i2c_read(4, 0x07, 0x00, read_1); //exposure time numerator
			printk("<=PCAM=> exposure time numerator : %x\n", *read_1);	
			
			m5mo_i2c_read(4, 0x07, 0x04, read_2); //exposure time denominator
			printk("<=PCAM=> exposure time denominator : %x\n", *read_2);	
			
		}
		break;

	case EXIF_TV://SHUTTER  SPEED
		{
			m5mo_i2c_read(4, 0x07, 0x08, read_1); //TV numerator
			printk("<=PCAM=> TV numerator : %x\n", *read_1);	
			
			m5mo_i2c_read(4, 0x07, 0x0C, read_2); //TV denominator
			printk("<=PCAM=> TV denominator : %x\n", *read_2);	
		}
		break;

	case EXIF_AV:
		{
			m5mo_i2c_read(4, 0x07, 0x10, read_1); //AV numerator
			printk("<=PCAM=> AV numerator : %x\n", *read_1);	

			m5mo_i2c_read(4, 0x07, 0x14, read_2); //AV denominator
			printk("<=PCAM=> AV denominator : %x\n", *read_2);	
		}
		break;

	case EXIF_BV:
		{
			m5mo_i2c_read(4, 0x07, 0x18, read_1); //BV numerator
			printk("<=PCAM=> BV numerator : %x\n", *read_1);	
			
			m5mo_i2c_read(4, 0x07, 0x1C, read_2); //BV denominator
			printk("<=PCAM=> BV denominator : %x\n", *read_2);				
		}
		break;

	case EXIF_EBV:
		{
			m5mo_i2c_read(4, 0x07, 0x20, read_1); //EBV numerator
			printk("<=PCAM=> EBV numerator : %x\n", *read_1);	
			
			m5mo_i2c_read(4, 0x07, 0x24, read_2); //EBV denominator  //??? 2's complment & numerator & unsigned??? check from Fujitus 
			printk("<=PCAM=> EBV denominator : %x\n", *read_2);				
			
		}
		break;

	case EXIF_ISO:
		{
			m5mo_i2c_read(2, 0x07, 0x28, read_1); //EXIF_ISO
			printk("<=PCAM=> EXIF_ISO : %x\n", *read_1);	
		}
		break;


	case EXIF_FLASH:
		{
			m5mo_i2c_read(2, 0x07, 0x2A, read_1); //EXIF_FLASH
			printk("<=PCAM=> EXIF_FLASH : %x\n", *read_1);				
		}
		break;


	default:
		{
			printk("<=PCAM=> unknown EXIF command\n");
		}
		break;
	
	}
#endif
	switch(*read_1){	
	case EXIF_EXPOSURE_TIME:
		(*read_1) = m5mo_exif->ep_numerator;
		(*read_2) = m5mo_exif->ep_denominator;
		break;
	case EXIF_TV:
		(*read_1) = m5mo_exif->tv_numerator;
		(*read_2) = m5mo_exif->tv_denominator;
		break;
	case EXIF_AV:
		(*read_1) = m5mo_exif->av_numerator;
		(*read_2) = m5mo_exif->av_denominator;
		break;	
	case EXIF_BV:
		(*read_1) = m5mo_exif->bv_numerator;
		(*read_2) = m5mo_exif->bv_denominator;
		break;
	case EXIF_EBV:
		(*read_1) = m5mo_exif->ebv_numerator;
		(*read_2) = m5mo_exif->ebv_denominator;
		break;
	case EXIF_ISO:
		(*read_1) = m5mo_exif->iso;
		break;
	case EXIF_FLASH:
		(*read_1) = m5mo_exif->flash;
		break;
	default:
		printk("[Exif] Invalid Exif information!");
	}

/*
	m5mo_i2c_read(4, 0x07, 0x00, &read_uint);
	printk("<=PCAM=>total  exif read_uint : %x\n", read_uint);	
	read_uing_temp = 0;	

	printk("<=PCAM=> m5mo_get_exif~~~~~~sdgjsdkldsjlkdsjglkdsjdklsgj~~~~~~~~~~\n");
	printk("<=PCAM=> read_1 :%x,  read_2 : %x\n", read_1, read_2);
*/
	return 0;
}


static int
m5mo_set_fps(
	unsigned int mode, unsigned int fps)
{
	CAM_DEBUG(" %s -mode : %d, fps : %d \n",__FUNCTION__,mode,fps);

	/* change to paramset mode */
	m5mo_i2c_write_8bit(0x00, 0x0B, M5MO_PARMSET_MODE);
	m5mo_i2c_verify(0x00, 0x0B, M5MO_PARMSET_MODE);
	if(mode)
	{
		m5mo_i2c_write_8bit(0x01, 0x31, 0x00);	// have to set auto fps except 15fps
		m5mo_i2c_write_8bit(0x01, 0x32, 0x01);	// movie mode (camcorder app)
		switch(fps)
		{
		case M5MO_5_FPS:
			//m5mo_i2c_write_8bit(0x01, 0x31, 0x05);
			break;

		case M5MO_15_FPS:
			m5mo_i2c_write_8bit(0x01, 0x31, 0x0F);	// 15fps for MMS
			break;

		case M5MO_30_FPS:
			//m5mo_i2c_write_8bit(0x01, 0x31, 0x1E);
			break;

		default:
			printk("[FPS] Invalid fps !!!\n");
			return -EINVAL;
		}
	}
	else		// mode == 0 , auto fps
	{
		CAM_DEBUG("Set Auto FPS ! \n");
		m5mo_i2c_write_8bit(0x01, 0x32, 0x00);	// monitor mode (camera app)
		m5mo_i2c_write_8bit(0x01, 0x31, 0x00);	// auto fps
	}

	m5mo_i2c_write_8bit( 0x00, 0x11, M5MO_INT_MODE);
	/* change to monitor mode */
	m5mo_i2c_write_8bit(0x00, 0x0B, M5MO_MONITOR_MODE);
	m5mo_i2c_verify(0x00, 0x0B, M5MO_MONITOR_MODE);
	
	wait_interrupt_mode(M5MO_INT_MODE);
	
	m5mo_ctrl->settings.fps = fps;
	return 0;
	
}
static int
m5mo_set_ev(
	int8_t ev)
{
	CAM_DEBUG(" %s - ev : %d \n",__FUNCTION__,ev);
	
	switch(ev)
	{
		case M5MO_EV_MINUS_4:
			m5mo_i2c_write_8bit( 0x03, 0x38, 0x00);
			break;
		case M5MO_EV_MINUS_3:
			m5mo_i2c_write_8bit( 0x03, 0x38, 0x01);
			break;
		case M5MO_EV_MINUS_2:
			m5mo_i2c_write_8bit( 0x03, 0x38, 0x02);
			break;
		case M5MO_EV_MINUS_1:
			m5mo_i2c_write_8bit( 0x03, 0x38, 0x03);
			break;
		case M5MO_EV_DEFAULT:
			m5mo_i2c_write_8bit( 0x03, 0x38, 0x04);
			break;
		case M5MO_EV_PLUS_1:
			m5mo_i2c_write_8bit( 0x03, 0x38, 0x05);
			break;
		case M5MO_EV_PLUS_2:
			m5mo_i2c_write_8bit( 0x03, 0x38, 0x06);
			break;	
		case M5MO_EV_PLUS_3:
			m5mo_i2c_write_8bit( 0x03, 0x38, 0x07);
			break;
		case M5MO_EV_PLUS_4:
			m5mo_i2c_write_8bit( 0x03, 0x38, 0x08);
			break;
		default:
			printk("[EV] Invalid EV !!!\n");
			return -EINVAL;
	}

	m5mo_ctrl->settings.ev = ev;
	return 0;
}

static int
m5mo_set_whitebalance(
	int8_t wb)
{
	CAM_DEBUG(" %s - wb : %d \n",__FUNCTION__,wb);
	
	switch(wb)
	{
		case M5MO_WB_AUTO:
			m5mo_i2c_write_8bit( 0x06, 0x02, 0x01);
			break;
		case M5MO_WB_INCANDESCENT:
			m5mo_i2c_write_8bit( 0x06, 0x02, 0x02);
			m5mo_i2c_write_8bit( 0x06, 0x03, 0x01);
			break;
		case M5MO_WB_FLUORESCENT:
			m5mo_i2c_write_8bit( 0x06, 0x02, 0x02);
			m5mo_i2c_write_8bit( 0x06, 0x03, 0x02);
			break;
		case M5MO_WB_DAYLIGHT:
			m5mo_i2c_write_8bit( 0x06, 0x02, 0x02);
			m5mo_i2c_write_8bit( 0x06, 0x03, 0x04);
			break;
		case M5MO_WB_CLOUDY:
			m5mo_i2c_write_8bit( 0x06, 0x02, 0x02);
			m5mo_i2c_write_8bit( 0x06, 0x03, 0x05);
			break;
		case M5MO_WB_SHADE:
			m5mo_i2c_write_8bit( 0x06, 0x02, 0x02);
			m5mo_i2c_write_8bit( 0x06, 0x03, 0x06);
			break;
		case M5MO_WB_HORIZON:
			m5mo_i2c_write_8bit( 0x06, 0x02, 0x02);
			m5mo_i2c_write_8bit( 0x06, 0x03, 0x07);
			break;
		case M5MO_WB_LED:
			m5mo_i2c_write_8bit( 0x06, 0x02, 0x02);
			m5mo_i2c_write_8bit( 0x06, 0x03, 0x09);
			break;
		default:
			printk("[WB] Invalid WB!!!\n");
			return -EINVAL;
	}

	m5mo_ctrl->settings.wb = wb;
	
	return 0;
	
}

static int
m5mo_set_metering(
	 int8_t metering)
{
	CAM_DEBUG(" metering : %d \n",metering);
	
	switch(metering)
	{
		case M5MO_METERING_AVERAGE:
			CAM_DEBUG("average\n");
			m5mo_i2c_write_8bit( 0x03, 0x01, 0x01);
			break;
		case M5MO_METERING_CENTER:
			CAM_DEBUG("center\n");
			m5mo_i2c_write_8bit( 0x03, 0x01, 0x03);
			break;
		case M5MO_METERING_SPOT:
			CAM_DEBUG("spot\n");
			m5mo_i2c_write_8bit( 0x03, 0x01, 0x06);
			break;
		default:
			printk("[Metering] Invalid metering value !!!\n");
			return -EINVAL;
	}

	m5mo_ctrl->settings.metering = metering;
	
	return 0;
}

static int
m5mo_set_saturation(
	 int8_t saturation)
{
	CAM_DEBUG(" saturation : %d \n",saturation);
	
	switch(saturation)
	{
		case M5MO_SATURATION_MINUS_2:
			m5mo_i2c_write_8bit( 0x02, 0x0F, 0x01);
			break;
		case M5MO_SATURATION_MINUS_1:
			m5mo_i2c_write_8bit( 0x02, 0x0F, 0x02);
			break;
		case M5MO_SATURATION_DEFAULT:
			m5mo_i2c_write_8bit( 0x02, 0x0F, 0x03);
			break;
		case M5MO_SATURATION_PLUS_1:
			m5mo_i2c_write_8bit( 0x02, 0x0F, 0x04);
			break;
		case M5MO_SATURATION_PLUS_2:
			m5mo_i2c_write_8bit( 0x02, 0x0F, 0x05);
			break;
		default:
			printk("[Saturation] Invalid  Saturation value!!!\n");
			return -EINVAL;
	}
	
	m5mo_ctrl->settings.saturation= saturation;
	return 0;
}


static int
m5mo_set_contrast(
	 int8_t contrast)
{
	CAM_DEBUG(" contrast : %d \n",contrast);
	
	switch(contrast)
	{
		case M5MO_CONTRAST_MINUS_2:
			m5mo_i2c_write_8bit( 0x02, 0x25, 0x03);
			break;
		case M5MO_CONTRAST_MINUS_1:
			m5mo_i2c_write_8bit( 0x02, 0x25, 0x04);
			break;
		case M5MO_CONTRAST_DEFAULT:
			m5mo_i2c_write_8bit( 0x02, 0x25, 0x05);
			break;
		case M5MO_CONTRAST_PLUS_1:
			m5mo_i2c_write_8bit( 0x02, 0x25, 0x06);
			break;
		case M5MO_CONTRAST_PLUS_2:
			m5mo_i2c_write_8bit( 0x02, 0x25, 0x07);
			break;
		default:
			printk("[Contrast] Invalid Contrast value!!!\n");
			return -EINVAL;
	}

	m5mo_ctrl->settings.contrast = contrast;
	return 0;
}

static int
m5mo_set_iso(
	int8_t iso)
{
	CAM_DEBUG(" iso : %d \n",iso);
	
	switch(iso)
	{
		case M5MO_ISO_AUTO:
			m5mo_i2c_write_8bit( 0x03, 0x05, 0x00);
			break;
		case M5MO_ISO_50:
			m5mo_i2c_write_8bit( 0x03, 0x05, 0x01);
			break;
		case M5MO_ISO_100:
			m5mo_i2c_write_8bit( 0x03, 0x05, 0x02);
			break;
		case M5MO_ISO_200:
			m5mo_i2c_write_8bit( 0x03, 0x05, 0x03);
			break;
		case M5MO_ISO_400:
			m5mo_i2c_write_8bit( 0x03, 0x05, 0x04);
			break;
		case M5MO_ISO_800:
			m5mo_i2c_write_8bit( 0x03, 0x05, 0x05);
			break;
		case M5MO_ISO_1600:
			m5mo_i2c_write_8bit( 0x03, 0x05, 0x06);
			break;
		case M5MO_ISO_3200:
			m5mo_i2c_write_8bit( 0x03, 0x05, 0x07);
			break;
		default:
			printk("[ISO] Invalid ISO value !!!\n");
			return -EINVAL;
	}

	m5mo_ctrl->settings.iso = iso;
	return 0;
}



static int 
m5mo_set_flash(int value)
{
	CAM_DEBUG(" flash mode : %d \n",value);

	switch(value)
	{
		case M5MO_FLASH_CAPTURE_OFF:
			CAM_DEBUG("Flash Off is set\n");
			m5mo_i2c_write_8bit(0x0B, 0x40, 0x00);
			m5mo_i2c_write_8bit(0x0B, 0x41, 0x00);
			//if(m5mo_ctrl->settings.scenemode == M5MO_SCENE_BACKLIGHT)
				//m5mo_set_metering(M5MO_METERING_SPOT);
			break;
		case M5MO_FLASH_CAPTURE_ON:
			CAM_DEBUG("Flash On is set\n");
			m5mo_i2c_write_8bit(0x0B, 0x40, 0x01);
			m5mo_i2c_write_8bit(0x0B, 0x41, 0x01);
			//if(m5mo_ctrl->settings.scenemode == M5MO_SCENE_BACKLIGHT)
				//m5mo_set_metering(M5MO_METERING_CENTER);
			break;
		case M5MO_FLASH_CAPTURE_AUTO:
			CAM_DEBUG("Flash Auto is set\n");
			m5mo_i2c_write_8bit(0x0B, 0x40, 0x02);
			m5mo_i2c_write_8bit(0x0B, 0x41, 0x02);
			break;
		case M5MO_FLASH_MOVIE_ON:
			CAM_DEBUG("Flash Movie On is set\n");
			m5mo_i2c_write_8bit(0x0B, 0x40, 0x03);
			break;
		default:
			printk("[FLASH CAPTURE] Invalid value is ordered!!!\n");
			return -EINVAL;
	}

	return 0;
}


static int 
m5mo_set_effect_color(
	int8_t effect)
{
	unsigned int value= 0;
//	int org_mode = 0;
	
	CAM_DEBUG(" effect : %d - ",effect);
	
	/* read gamma effect flag */
	m5mo_i2c_read(1, 0x01, 0x0B, &value);
	if(value)
	{
//		m5mo_i2c_read(0x00, 0x0B, &org_mode);	
		
		/* change to paramset mode */
		m5mo_i2c_write_8bit(0x00, 0x0B, M5MO_PARMSET_MODE);
		m5mo_i2c_verify(0x00, 0x0B, M5MO_PARMSET_MODE);

		/* gamma effect off */
		CAM_DEBUG("gamma effect off ");
		m5mo_i2c_write_8bit(0x01, 0x0B, 0x00); 

		m5mo_i2c_write_8bit( 0x00, 0x11, M5MO_INT_MODE);
		/* change to monitor mode */
		m5mo_i2c_write_8bit(0x00, 0x0B, M5MO_MONITOR_MODE);
		//msleep(10);
		m5mo_i2c_verify(0x00, 0x0B, M5MO_MONITOR_MODE);
		
		m5mo_i2c_read(1, 0x00, 0x0B, &value);
		//CAM_DEBUG("sensor mode : %d\n", value);
	
		m5mo_i2c_read(1, 0x00, 0x10, &value);
		//CAM_DEBUG("interrupt state : %d\n", value);
	}

	switch(effect)
	{
		case M5MO_EFFECT_OFF:
			CAM_DEBUG(" OFF \n");
			m5mo_i2c_write_8bit(0x02, 0x0B, 0x00);
			break;
		case M5MO_EFFECT_SEPIA:
			CAM_DEBUG(" SEPIA \n");
			m5mo_i2c_write_8bit(0x02, 0x0B, 0x01); 
			m5mo_i2c_write_8bit(0x02, 0x09, 0xD8);
			m5mo_i2c_write_8bit(0x02, 0x0A, 0x18);
			break;
		case M5MO_EFFECT_MONO:
			CAM_DEBUG(" MONO \n");
			m5mo_i2c_write_8bit(0x02, 0x0B, 0x01); 
			m5mo_i2c_write_8bit(0x02, 0x09, 0x00);
			m5mo_i2c_write_8bit(0x02, 0x0A, 0x00);
			break;
/*		case M5MO_EFFECT_RED:
			m5mo_i2c_write_8bit(0x02, 0x0B, 0x01); 
			m5mo_i2c_write_8bit(0x02, 0x09, 0x00);
			m5mo_i2c_write_8bit(0x02, 0x0A, 0x6B);
			break;
		case M5MO_EFFECT_GREEN:
			m5mo_i2c_write_8bit(0x02, 0x0B, 0x01); 
			m5mo_i2c_write_8bit(0x02, 0x09, 0xE0);
			m5mo_i2c_write_8bit(0x02, 0x0A, 0xE0);
			break;
		case M5MO_EFFECT_BLUE:
			m5mo_i2c_write_8bit(0x02, 0x0B, 0x01); 
			m5mo_i2c_write_8bit(0x02, 0x09, 0x40);
			m5mo_i2c_write_8bit(0x02, 0x0A, 0x00);
			break;
		case M5MO_EFFECT_PINK:
			m5mo_i2c_write_8bit(0x02, 0x0B, 0x01); 
			m5mo_i2c_write_8bit(0x02, 0x09, 0x20);
			m5mo_i2c_write_8bit(0x02, 0x0A, 0x40);
			break;
		case M5MO_EFFECT_YELLOW:
			m5mo_i2c_write_8bit(0x02, 0x0B, 0x01); 
			m5mo_i2c_write_8bit(0x02, 0x09, 0x80);
			m5mo_i2c_write_8bit(0x02, 0x0A, 0x00);
			break;
		case M5MO_EFFECT_PURPLE:
			m5mo_i2c_write_8bit(0x02, 0x0B, 0x01); 
			m5mo_i2c_write_8bit(0x02, 0x09, 0x50);
			m5mo_i2c_write_8bit(0x02, 0x0A, 0x20);
			break;
*/		case M5MO_EFFECT_ANTIQUE:
			CAM_DEBUG(" ANTIQUE \n");
			m5mo_i2c_write_8bit(0x02, 0x0B, 0x01); 
			m5mo_i2c_write_8bit(0x02, 0x09, 0xD0);
			m5mo_i2c_write_8bit(0x02, 0x0A, 0x30);
			break;	
		default:
			break;
	}

	return 0;
}

static int 
m5mo_set_effect_gamma(
	int8_t effect)
{
	unsigned int value= 0;

	CAM_DEBUG(" effect : %d - ",effect);
	
	m5mo_i2c_read(1, 0x02, 0x0B, &value);
	if(value)
	{
		/* color effect off */
		CAM_DEBUG("color effect off ");
		m5mo_i2c_write_8bit(0x02, 0x0B, 0x00); 	
	}	

	/* change to paramset mode */
	m5mo_i2c_write_8bit(0x00, 0x0B, M5MO_PARMSET_MODE);
	m5mo_i2c_verify(0x00, 0x0B, M5MO_PARMSET_MODE);
	//msleep(10);
	
	switch(effect)
	{
		case M5MO_EFFECT_NEGATIVE:
			CAM_DEBUG(" NEGATIVE \n");
			m5mo_i2c_write_8bit( 0x01, 0x0B, 0x01);
			break;
#if 0
		case M5MO_EFFECT_SOLARIZATION1:
			m5mo_i2c_write_8bit( 0x01, 0x0B, 0x02);
			break;
		case M5MO_EFFECT_SOLARIZATION2:
			m5mo_i2c_write_8bit( 0x01, 0x0B, 0x03);
			break;
#endif
		case M5MO_EFFECT_SOLARIZATION3:
			CAM_DEBUG(" SOLARIZATION3 \n");
			m5mo_i2c_write_8bit( 0x01, 0x0B, 0x04);
			break;
#if 0
		case M5MO_EFFECT_SOLARIZATION4:
			m5mo_i2c_write_8bit( 0x01, 0x0B, 0x05);
			break;
#endif
		case M5MO_EFFECT_EMBOSS:
			CAM_DEBUG(" EMBOSS \n");
			m5mo_i2c_write_8bit( 0x01, 0x0B, 0x06);
			break;
		case M5MO_EFFECT_OUTLINE:
			CAM_DEBUG(" OUTLINE \n");
			m5mo_i2c_write_8bit( 0x01, 0x0B, 0x07);
			break;
		case M5MO_EFFECT_AQUA:
			CAM_DEBUG(" AQUA \n");
			m5mo_i2c_write_8bit( 0x01, 0x0B, 0x08);
			break;
		default:
			break;	
	}

	/* change to monitor mode */
	m5mo_i2c_write_8bit( 0x00, 0x11, M5MO_INT_MODE);
	
	m5mo_i2c_write_8bit(0x00, 0x0B, M5MO_MONITOR_MODE);
	m5mo_i2c_verify(0x00, 0x0B, M5MO_MONITOR_MODE);
	
	wait_interrupt_mode(M5MO_INT_MODE);

	return 0;
}

static long m5mo_set_effect(
	int8_t effect
)
{
	long rc = 0;

//	CAM_DEBUG("Do not Set Effect(%d)\n", effect);
//	return 0;
	
	switch(effect)
	{
		case M5MO_EFFECT_OFF:
		case M5MO_EFFECT_SEPIA:
		case M5MO_EFFECT_MONO:
/*		case M5MO_EFFECT_RED:
		case M5MO_EFFECT_GREEN:
		case M5MO_EFFECT_BLUE:
		case M5MO_EFFECT_PINK:
		case M5MO_EFFECT_YELLOW:
		case M5MO_EFFECT_PURPLE:
*/		case M5MO_EFFECT_ANTIQUE:
			rc = m5mo_set_effect_color(effect);
			break;
		case M5MO_EFFECT_NEGATIVE:
		//case M5MO_EFFECT_SOLARIZATION1:
		//case M5MO_EFFECT_SOLARIZATION2:
		case M5MO_EFFECT_SOLARIZATION3:
		//case M5MO_EFFECT_SOLARIZATION4:
		case M5MO_EFFECT_EMBOSS:
		case M5MO_EFFECT_OUTLINE:
		case M5MO_EFFECT_AQUA:
			rc = m5mo_set_effect_gamma(effect);
			break;
		default:
			printk("[Effect]Invalid Effect !!!\n");
			return -EINVAL;
	}

	m5mo_ctrl->settings.effect = effect;
	return rc;
}


static long m5mo_set_antishake_mode(
	u32 onoff
)
{
	int rc = 0;
	unsigned int value;
	CAM_DEBUG(" onoff: %d \n",onoff);	

	m5mo_i2c_read(1, 0x01, 0x01, &value);
	CAM_DEBUG(" current preview size : 0x%x",value);
	if(onoff)
	{
		/* Start Anti Shake */
		m5mo_i2c_write_8bit(0x03, 0x0A, 0x0E); 
		m5mo_i2c_write_8bit(0x03, 0x0A, 0x0E); 
		//m5mo_i2c_write_8bit(0x02, 0x00, 0x01); 
		//rc = m5mo_i2c_verify(0x02, 0x26, 0x01); 
	}
	else
	{
		/* Stop Anti Shake */
		//m5mo_i2c_write_8bit(0x02, 0x00, 0x00); 
		m5mo_i2c_write_8bit(0x03, 0x0A, 0x00); 
		m5mo_i2c_write_8bit(0x03, 0x0A, 0x00); 
	}

	m5mo_ctrl->settings.antishake = onoff;
	return rc;
}


static long m5mo_set_zoom(
	int8_t level
)
{
	int rc = 0;

	if(level < 0 || level > 30 )
	{
		printk("[Zoom] Invalid Zoom Level !!!\n");
		return -EINVAL;
	}
	
	CAM_DEBUG(" level : %d (reg value : %d)\n",level,m5mo_zoom_table[level]);
	
	/* Start Digital Zoom */
	m5mo_i2c_write_8bit(0x02, 0x01, m5mo_zoom_table[level]); 
	msleep(30);

	m5mo_ctrl->settings.zoom = level;
	return rc;
}


static int
m5mo_get_af_status(
	int* status)
{
	unsigned int af_status;
	unsigned int af_result;
	unsigned int value;

	// if it's not AF mode, then just return
	if(m5mo_ctrl->settings.focus_status == 0)
		return 0;
	
	m5mo_i2c_read(1, 0x00, 0x10, &value);
	if(!(value & M5MO_INT_AF)) {
		CAM_DEBUG("AF is on going ...(value = 0x%x)",value);
		*status = AF_NONE;
	}
	else {
	m5mo_i2c_read(1, 0x0A, 0x03, &af_result);
	m5mo_i2c_read(1, 0x0A, 0x04, &af_status);
		if(af_result == 2) {
		*status = AF_SUCCEED;	
			m5mo_ctrl->settings.focus_status = 0;
		}
		else if( (af_status != 1) && (af_result == 0)) {
		*status = AF_FAILED;	
			m5mo_ctrl->settings.focus_status = 0;			
			m5mo_i2c_write_8bit(0x03, 0x00, 0x00); 
			m5mo_i2c_write_8bit(0x06, 0x00, 0x00);
		}
		else {
		*status = AF_NONE;
			m5mo_ctrl->settings.focus_status = 0;
			m5mo_i2c_write_8bit(0x03, 0x00, 0x00); 
			m5mo_i2c_write_8bit(0x06, 0x00, 0x00);
		}
	}
	CAM_DEBUG(" status : %d \n",*status);
	
	return 0;
}
static int
m5mo_set_af_status(
	int status)
{
	unsigned int value;
	int wait_count = 30;
	static int af_stopped;
	
	CAM_DEBUG(" status : %d \n",status);
	if(status)	// start
	{
		CAM_DEBUG(" AF start !");
		af_stopped = 0;

		m5mo_i2c_read(1, 0x00, 0x10, &value);
		/* AE/AWB Lock */
		m5mo_i2c_write_8bit(0x03, 0x00, 0x01); 
		m5mo_i2c_write_8bit(0x06, 0x00, 0x01); 
		/* AF interrupt enable */
		m5mo_i2c_write_8bit(0x00, 0x11, M5MO_INT_AF); 
		/* AF operation start */
		m5mo_i2c_write_8bit(0x0A, 0x02, 0x01); 
		//msleep(100);
		//wait_interrupt_mode(M5MO_INT_AF);
		
#ifdef	CHECK_AFSTATUS_IN_KERNEL		
		do
		{
			msleep(50);
			m5mo_i2c_read(1, 0x00, 0x10, &value);
			CAM_DEBUG(" (%d) Wait interrupt mode : %d (0x10 regs value = 0x%x)",wait_count,M5MO_INT_AF,value);
		}while((--wait_count) > 0 && !(value & M5MO_INT_AF) && !af_stopped);

		m5mo_i2c_read(1, 0x0A, 0x04, &value);
		CAM_DEBUG("AF status : %s",(value == 1)?"Free":"Busy");
		
		m5mo_i2c_read(1, 0x0A, 0x03, &value);
		CAM_DEBUG("AF result : %s",(value == 2)?"Success":((value==1)?"None":"Fail"));

		/* AE/AWB unlock when AF result is Failed */
		if(value == 0)
		{
			m5mo_i2c_write_8bit(0x03, 0x00, 0x00); 
			m5mo_i2c_write_8bit(0x06, 0x00, 0x00);
		}
#endif
	}
	else			// stop
	{
		CAM_DEBUG(" AF stop !");
		af_stopped = 1;
		/* Stop AF */
		m5mo_i2c_write_8bit(0x0A, 0x02, 0x00); 
//		m5mo_i2c_write_8bit(0x09, 0x00, 0x00);
		/* AE/AWB Unlock */
		m5mo_i2c_write_8bit(0x03, 0x00, 0x00); 
		m5mo_i2c_write_8bit(0x06, 0x00, 0x00); 
//		msleep(5);
	}

	m5mo_ctrl->settings.focus_status = status;
	return 0;
}


static int
m5mo_set_continuous_af(
	int onoff)
{
	CAM_DEBUG(" onoff : %d \n",onoff);
	
	if(m5mo_ctrl->settings.continuous_af == onoff)
		return 0;
	
	switch(onoff)
	{
		case M5MO_AF_CONTINUOUS_OFF:
			m5mo_i2c_write_8bit(0x0A, 0x05, 0x00);
			break;
		case M5MO_AF_CONTINUOUS_ON:
			m5mo_i2c_write_8bit(0x0A, 0x05, 0x01);
			break;
		default:
			printk("[CONTINUOUS AF]Invalid CAF value!!!\n");
			return -EINVAL;
	}

	m5mo_ctrl->settings.continuous_af= onoff;
	return 0;
}



static int
m5mo_set_touchaf_pos(
	u32 screen_x, u32 screen_y)
{
//	int err=0;
	int sensor_width, sensor_height;
	short sensor_x, sensor_y;
	
	sensor_width = m5mo_ctrl->settings.preview_size.width;
	sensor_height = m5mo_ctrl->settings.preview_size.height;

	sensor_x = (short)screen_x * sensor_width / SCREEN_SIZE_WIDTH;
	sensor_y = (short)screen_y * sensor_height / SCREEN_SIZE_HEIGHT;
	
	CAM_DEBUG(" screenX : %d, screenY : %d \n",screen_x,screen_y);
	CAM_DEBUG(" sensorX : %d, sensorY : %d \n",sensor_x,sensor_y);
	
	m5mo_i2c_write_16bit( 0x0A, 0x30, sensor_x); 
	m5mo_i2c_write_16bit( 0x0A, 0x32, sensor_y); 
	
	return 0;
}


static int
m5mo_set_af_mode(
	int mode)
{
//	int err=0;
	CAM_DEBUG(" mode : %d \n",mode);

	m5mo_i2c_write_8bit(0x09, 0x00, 0x00);	// face detection disable
	
	switch(mode)
	{
	case M5MO_AF_MODE_NORMAL:
		CAM_DEBUG("Normal AF mode");
		/* Set Normal AF Mode */		
		m5mo_i2c_write_8bit( 0x0A, 0x01, 0x00); 
		/* Set base position */
		m5mo_i2c_write_8bit( 0x0A, 0x00, 0x01);
		break;

	case M5MO_AF_MODE_MACRO:
		CAM_DEBUG("Macro AF mode");
		/* Set Macro AF Mode */ 
		m5mo_i2c_write_8bit( 0x0A, 0x01, 0x01); 
		/* Set base position */
		m5mo_i2c_write_8bit( 0x0A, 0x00, 0x01);
		break;
#if 0
	case M5MO_AF_START:
		/* AE/AWB Lock */
		m5mo_i2c_write_8bit(0x03, 0x00, 0x01); 
		m5mo_i2c_write_8bit(0x06, 0x00, 0x01); 
		/* AF operation start */
		m5mo_i2c_write_8bit(0x0A, 0x02, 0x01); 
		break;

	case M5MO_AF_STOP:
		break;

	case M5MO_AF_RELEASE:
		break;
#endif
	case M5MO_AF_MODE_CAF:
		CAM_DEBUG("Continuous mode");
		/* Set Normal AF Mode */		
		m5mo_i2c_write_8bit( 0x0A, 0x01, 0x08); 
		break;

	case M5MO_AF_MODE_FACEDETECT:
		CAM_DEBUG("Face detection mode : max 11 ");
/*		m5mo_i2c_write_8bit( 0x09, 0x01, 0x01); 
		m5mo_i2c_write_8bit( 0x09, 0x02, 0x0B); */
		
//		m5mo_i2c_write_8bit( 0x09, 0x00, 0x11); 	// enable face detection 
		m5mo_i2c_write_8bit( 0x0A, 0x01, 0x03); 
		//msleep(10);
		//m5mo_set_af_status(1);
		break;
		
	case M5MO_AF_MODE_TOUCHAF:
		CAM_DEBUG("Touch AF mode");
		/* Set Normal AF Mode */		
		//m5mo_i2c_write_8bit( 0x0A, 0x01, 0x04); 	// from Seine
		m5mo_i2c_write_8bit( 0x0A, 0x01, 0x0B); 
		break;

	case M5MO_AF_MODE_NIGHTSHOT:
		CAM_DEBUG("Face detection mode");
		/* Set Normal AF Mode */		
		//m5mo_i2c_write_8bit( 0x0A, 0x01, 0x05); 	// from Seine
		m5mo_i2c_write_8bit( 0x0A, 0x01, 0x02); 
		break;
	
	default:
			printk("[AF] Invalid AutoFocus mode!!!\n");
			return -EINVAL;
	}

	//m5mo_set_af_status(1);		// START AF

	m5mo_ctrl->settings.focus_mode = mode;
	return 0;
}

static int 
m5mo_set_scene(
	int mode)
{
	int err=0;
	CAM_DEBUG(" mode : %d \n",mode);
	switch(mode)
	{
		case M5MO_SCENE_NORMAL:
			/* Wide dynamic range */
			err = m5mo_set_wdr(M5MO_WDR_OFF);	// check default value
			/* EV-P */
			m5mo_i2c_write_8bit(0x03, 0x0A, 0x00);
			m5mo_i2c_write_8bit(0x03, 0x0B, 0x00);
			/* EV */
			err = m5mo_set_ev(M5MO_EV_DEFAULT);
			/* AWB */
			err = m5mo_set_whitebalance(M5MO_WB_AUTO);
			/* AF mode / Lens Position */
			err = m5mo_set_af_mode( M5MO_AF_MODE_NORMAL);
			/* Face Detection */
			//err = m5mo_set_face_detection( M5MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m5mo_set_metering(M5MO_METERING_CENTER);
			/* Chroma Saturation */
			err = m5mo_set_saturation(M5MO_SATURATION_DEFAULT);
			/* Sharpness */
			err = m5mo_set_sharpness(M5MO_SHARPNESS_DEFAULT);	// check default value
			/* Flash */
			err = m5mo_set_flash(M5MO_FLASH_CAPTURE_OFF);
			/* Emotional Color */
			m5mo_i2c_write_8bit( 0x0B, 0x1D, 0x01);
			/* ISO*/
			err = m5mo_set_iso(M5MO_ISO_AUTO);
			/* Contrast */
			err = m5mo_set_contrast(M5MO_CONTRAST_DEFAULT);
			/* Image Stabilization */
			//err = m5mo_set_isc(M5MO_ISC_STILL_OFF);
			
			break;
			
		case M5MO_SCENE_PORTRAIT:
#if 0
			printk("#############################\n");
			printk("####### Firmware Update START ######\n");
			printk("#############################\n");
			m5mo_reset_for_update();
			m5mo_load_fw(1);

			printk("#############################\n");
			printk("####### Firmware Update END ########\n");
			printk("#############################\n");
			break;
#endif
			/* Wide dynamic range */
			err = m5mo_set_wdr(M5MO_WDR_OFF);
			/* EV-P */
			m5mo_i2c_write_8bit( 0x03, 0x0A, 0x01);
			m5mo_i2c_write_8bit( 0x03, 0x0B, 0x01);
			/* EV */
			err = m5mo_set_ev(M5MO_EV_DEFAULT);
			/* AWB */
			err = m5mo_set_whitebalance(M5MO_WB_AUTO);
			/* AF mode / Lens Position */
			err = m5mo_set_af_mode(M5MO_AF_MODE_FACEDETECT);
			/* Face Detection */
			//err = m5mo_set_face_detection(M5MO_FACE_DETECTION_ON);
			/*Photometry */
			err = m5mo_set_metering(M5MO_METERING_AVERAGE);
			/* Chroma Saturation */
			err = m5mo_set_saturation(M5MO_SATURATION_DEFAULT);
			/* Sharpness */
			err = m5mo_set_sharpness(M5MO_SHARPNESS_MINUS_1);
			m5mo_i2c_write_8bit( 0x02, 0x11, 0x05);
			/* Flash */
			err = m5mo_set_flash(M5MO_FLASH_CAPTURE_OFF);
			/* Emotional Color */
			m5mo_i2c_write_8bit( 0x0B, 0x1D, 0x00);
			/* ISO*/
			err = m5mo_set_iso(M5MO_ISO_AUTO);
			/* Contrast */
			err = m5mo_set_contrast(M5MO_CONTRAST_DEFAULT);
			/* Image Stabilization */
			//err = m5mo_set_isc(M5MO_ISC_STILL_OFF);
			break;
			
		case M5MO_SCENE_LANDSCAPE:
			/* Wide dynamic range */
			err = m5mo_set_wdr(M5MO_WDR_OFF);
			/* EV-P */
			m5mo_i2c_write_8bit( 0x03, 0x0A, 0x02);
			m5mo_i2c_write_8bit( 0x03, 0x0B, 0x02);
			/* EV */
			err = m5mo_set_ev(M5MO_EV_DEFAULT);
			/* AWB */
			err = m5mo_set_whitebalance(M5MO_WB_AUTO);
			/* AF mode / Lens Position */
			err = m5mo_set_af_mode(M5MO_AF_MODE_NORMAL);
			/* Face Detection */
			//err = m5mo_set_face_detection(M5MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m5mo_set_metering(M5MO_METERING_CENTER);
			/* Chroma Saturation */
			err = m5mo_set_saturation(M5MO_SATURATION_PLUS_2);
			/* Sharpness */
			err = m5mo_set_sharpness(M5MO_SHARPNESS_DEFAULT);
			/* Flash */
			err = m5mo_set_flash(M5MO_FLASH_CAPTURE_OFF);
			/* Emotional Color */
			m5mo_i2c_write_8bit(0x0B, 0x1D, 0x00);
			/* ISO*/
			err = m5mo_set_iso(M5MO_ISO_AUTO);
			/* Contrast */
			err = m5mo_set_contrast(M5MO_CONTRAST_DEFAULT);
			/* Image Stabilization */
			//err = m5mo_set_isc(M5MO_ISC_STILL_OFF);
			break;
			
		case M5MO_SCENE_SPORTS:
			/* Wide dynamic range */
			err = m5mo_set_wdr(M5MO_WDR_OFF);
			/* EV-P */
			m5mo_i2c_write_8bit(0x03, 0x0A, 0x03);
			m5mo_i2c_write_8bit( 0x03, 0x0B, 0x03);
			/* EV */
			err = m5mo_set_ev(M5MO_EV_DEFAULT);
			/* AWB */
			err = m5mo_set_whitebalance(M5MO_WB_AUTO);
			/* AF mode / Lens Position */
			err = m5mo_set_af_mode(M5MO_AF_MODE_NORMAL);
			/* Face Detection */
			//err = m5mo_set_face_detection(M5MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m5mo_set_metering(M5MO_METERING_CENTER);
			/* Chroma Saturation */
			err = m5mo_set_saturation(M5MO_SATURATION_DEFAULT);
			/* Sharpness */
			err = m5mo_set_sharpness(M5MO_SHARPNESS_DEFAULT);
			/* Flash */
			err = m5mo_set_flash(M5MO_FLASH_CAPTURE_OFF);
			/* Emotional Color */
			m5mo_i2c_write_8bit( 0x0B, 0x1D, 0x00);
			/* ISO*/
			err = m5mo_set_iso(M5MO_ISO_AUTO);
			/* Contrast */
			err = m5mo_set_contrast(M5MO_CONTRAST_DEFAULT);
			/* Image Stabilization */
			//err = m5mo_set_isc(M5MO_ISC_STILL_OFF);
			break;
			
		case M5MO_SCENE_PARTY_INDOOR:
			/* Wide dynamic range */
			err = m5mo_set_wdr(M5MO_WDR_OFF);
			/* EV-P */
			m5mo_i2c_write_8bit( 0x03, 0x0A, 0x04);
			m5mo_i2c_write_8bit( 0x03, 0x0B, 0x04);
			/* EV */
			err = m5mo_set_ev(M5MO_EV_DEFAULT);
			/* AWB */
			err = m5mo_set_whitebalance(M5MO_WB_AUTO);
			/* AF mode / Lens Position */
			err = m5mo_set_af_mode(M5MO_AF_MODE_NORMAL);
			/* Face Detection */
			//err = m5mo_set_face_detection(M5MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m5mo_set_metering(M5MO_METERING_AVERAGE);
			/* Chroma Saturation */
			err = m5mo_set_saturation(M5MO_SATURATION_PLUS_1);
			/* Sharpness */
			err = m5mo_set_sharpness(M5MO_SHARPNESS_DEFAULT);
			/* Flash */
			err = m5mo_set_flash(M5MO_FLASH_CAPTURE_OFF);
			/* Emotional Color */
			m5mo_i2c_write_8bit( 0x0B, 0x1D, 0x00);
			/* ISO*/
			err = m5mo_set_iso(M5MO_ISO_200);
			/* Contrast */
			err = m5mo_set_contrast(M5MO_CONTRAST_DEFAULT);
			/* Image Stabilization */
			//err = m5mo_set_isc(M5MO_ISC_STILL_OFF);
			break;

		case M5MO_SCENE_BEACH_SNOW:
			/* Wide dynamic range */
			err = m5mo_set_wdr(M5MO_WDR_OFF);
			/* EV-P */
			m5mo_i2c_write_8bit( 0x03, 0x0A, 0x05);
			m5mo_i2c_write_8bit( 0x03, 0x0B, 0x05);
			/* EV */
			err = m5mo_set_ev(M5MO_EV_PLUS_2);
			/* AWB */
			err = m5mo_set_whitebalance(M5MO_WB_AUTO);
			/* AF mode / Lens Position */
			err = m5mo_set_af_mode(M5MO_AF_MODE_NORMAL);
			/* Face Detection */
			//err = m5mo_set_face_detection(M5MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m5mo_set_metering(M5MO_METERING_CENTER);
			/* Chroma Saturation */
			err = m5mo_set_saturation(M5MO_SATURATION_PLUS_1);
			/* Sharpness */
			err = m5mo_set_sharpness(M5MO_SHARPNESS_DEFAULT);
			/* Flash */
			err = m5mo_set_flash(M5MO_FLASH_CAPTURE_OFF);
			/* Emotional Color */
			m5mo_i2c_write_8bit( 0x0B, 0x1D, 0x00);
			/* ISO*/
			err = m5mo_set_iso(M5MO_ISO_50);
			/* Contrast */
			err = m5mo_set_contrast(M5MO_CONTRAST_DEFAULT);
			/* Image Stabilization */
			//err = m5mo_set_isc(M5MO_ISC_STILL_OFF);
			break;

		case M5MO_SCENE_SUNSET:
			/* Wide dynamic range */
			err = m5mo_set_wdr(M5MO_WDR_OFF);
			/* EV-P */
			m5mo_i2c_write_8bit( 0x03, 0x0A, 0x06);
			m5mo_i2c_write_8bit( 0x03, 0x0B, 0x06);
			/* EV */
			err = m5mo_set_ev(M5MO_EV_DEFAULT);
			/* AWB */
			err = m5mo_set_whitebalance(M5MO_WB_DAYLIGHT);
			/* AF mode / Lens Position */
			err = m5mo_set_af_mode(M5MO_AF_MODE_NORMAL);
			/* Face Detection */
			//err = m5mo_set_face_detection(M5MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m5mo_set_metering(M5MO_METERING_CENTER);			
			/* Chroma Saturation */
			err = m5mo_set_saturation(M5MO_SATURATION_DEFAULT);
			/* Sharpness */
			err = m5mo_set_sharpness(M5MO_SHARPNESS_DEFAULT);
			/* Flash */
			err = m5mo_set_flash(M5MO_FLASH_CAPTURE_OFF);
			/* Emotional Color */
			m5mo_i2c_write_8bit( 0x0B, 0x1D, 0x00);
			/* ISO*/
			err = m5mo_set_iso(M5MO_ISO_AUTO);
			/* Contrast */
			err = m5mo_set_contrast(M5MO_CONTRAST_DEFAULT);
			/* Image Stabilization */
			//err = m5mo_set_isc(M5MO_ISC_STILL_OFF);
			break;

		case M5MO_SCENE_DUSK_DAWN:
			/* Wide dynamic range */
			err = m5mo_set_wdr(M5MO_WDR_OFF);
			/* EV-P */
			m5mo_i2c_write_8bit( 0x03, 0x0A, 0x07);
			m5mo_i2c_write_8bit( 0x03, 0x0B, 0x07);
			/* EV */
			err = m5mo_set_ev(M5MO_EV_DEFAULT);
			/* AWB */
			err = m5mo_set_whitebalance(M5MO_WB_FLUORESCENT);
			/* AF mode / Lens Position */
			err = m5mo_set_af_mode(M5MO_AF_MODE_NORMAL);
			/* Face Detection */
			//err = m5mo_set_face_detection(M5MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m5mo_set_metering(M5MO_METERING_CENTER);		
			/* Chroma Saturation */
			err = m5mo_set_saturation(M5MO_SATURATION_DEFAULT);
			/* Sharpness */
			err = m5mo_set_sharpness(M5MO_SHARPNESS_DEFAULT);
			/* Flash */
			err = m5mo_set_flash(M5MO_FLASH_CAPTURE_OFF);
			/* Emotional Color */
			m5mo_i2c_write_8bit( 0x0B, 0x1D, 0x00);
			/* ISO*/
			err = m5mo_set_iso(M5MO_ISO_AUTO);
			/* Contrast */
			err = m5mo_set_contrast(M5MO_CONTRAST_DEFAULT);
			/* Image Stabilization */
			//err = m5mo_set_isc(M5MO_ISC_STILL_OFF);
			break;
			
		case M5MO_SCENE_FALL_COLOR:
			/* Wide dynamic range */
			err = m5mo_set_wdr(M5MO_WDR_OFF);
			/* EV-P */
			m5mo_i2c_write_8bit( 0x03, 0x0A, 0x08);
			m5mo_i2c_write_8bit( 0x03, 0x0B, 0x08);
			/* EV */
			err = m5mo_set_ev(M5MO_EV_DEFAULT);
			/* AWB */
			err = m5mo_set_whitebalance(M5MO_WB_AUTO);
			/* AF mode / Lens Position */
			err = m5mo_set_af_mode(M5MO_AF_MODE_NORMAL);
			/* Face Detection */
			//err = m5mo_set_face_detection(M5MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m5mo_set_metering(M5MO_METERING_AVERAGE);			
			/* Chroma Saturation */
			err = m5mo_set_saturation(M5MO_SATURATION_PLUS_2);
			/* Sharpness */
			err = m5mo_set_sharpness(M5MO_SHARPNESS_DEFAULT);
			/* Flash */
			err = m5mo_set_flash(M5MO_FLASH_CAPTURE_OFF);
			/* Emotional Color */
			m5mo_i2c_write_8bit( 0x0B, 0x1D, 0x00);
			/* ISO*/
			err = m5mo_set_iso(M5MO_ISO_AUTO);
			/* Contrast */
			err = m5mo_set_contrast(M5MO_CONTRAST_DEFAULT);
			/* Image Stabilization */
			//err = m5mo_set_isc(M5MO_ISC_STILL_OFF);
			break;


		case M5MO_SCENE_NIGHT:
			/* Wide dynamic range */
			err = m5mo_set_wdr(M5MO_WDR_OFF);
			/* EV-P */
			m5mo_i2c_write_8bit( 0x03, 0x0A, 0x09);
			m5mo_i2c_write_8bit( 0x03, 0x0B, 0x09);
			/* EV */
			err = m5mo_set_ev(M5MO_EV_DEFAULT);
			/* AWB */
			err = m5mo_set_whitebalance(M5MO_WB_AUTO);
			/* AF mode / Lens Position */
			err = m5mo_set_af_mode(M5MO_AF_MODE_NORMAL);
			/* Face Detection */
			//err = m5mo_set_face_detection(M5MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m5mo_set_metering(M5MO_METERING_CENTER);	
			/* Chroma Saturation */
			err = m5mo_set_saturation(M5MO_SATURATION_DEFAULT);
			/* Sharpness */
			err = m5mo_set_sharpness(M5MO_SHARPNESS_DEFAULT);
			/* Flash */
			err = m5mo_set_flash(M5MO_FLASH_CAPTURE_OFF);
			/* Emotional Color */
			m5mo_i2c_write_8bit( 0x0B, 0x1D, 0x00);
			/* ISO*/
			err = m5mo_set_iso(M5MO_ISO_AUTO);
			/* Contrast */
			err = m5mo_set_contrast(M5MO_CONTRAST_DEFAULT);
			/* Image Stabilization */
			//err = m5mo_set_isc(M5MO_ISC_STILL_OFF);
			break;
			
		case M5MO_SCENE_FIREWORK:
			/* Wide dynamic range */
			err = m5mo_set_wdr(M5MO_WDR_OFF);
			/* EV-P */
			m5mo_i2c_write_8bit( 0x03, 0x0A, 0x0A);
			m5mo_i2c_write_8bit( 0x03, 0x0B, 0x0A);
			/* EV */
			err = m5mo_set_ev(M5MO_EV_DEFAULT);
			/* AWB */
			err = m5mo_set_whitebalance(M5MO_WB_AUTO);
			/* AF mode / Lens Position */
			err = m5mo_set_af_mode(M5MO_AF_MODE_NORMAL);
			/* Face Detection */
			//err = m5mo_set_face_detection(M5MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m5mo_set_metering(M5MO_METERING_CENTER);		
			/* Chroma Saturation */
			err = m5mo_set_saturation(M5MO_SATURATION_DEFAULT);
			/* Sharpness */
			err = m5mo_set_sharpness(M5MO_SHARPNESS_DEFAULT);
			/* Flash */
			err = m5mo_set_flash(M5MO_FLASH_CAPTURE_OFF);
			/* Emotional Color */
			m5mo_i2c_write_8bit( 0x0B, 0x1D, 0x00);
			/* ISO*/
			err = m5mo_set_iso(M5MO_ISO_50);
			/* Contrast */
			err = m5mo_set_contrast(M5MO_CONTRAST_DEFAULT);
			/* Image Stabilization */
			//err = m5mo_set_isc(M5MO_ISC_STILL_OFF);
			break;

		case M5MO_SCENE_TEXT:
			/* Wide dynamic range */
			err = m5mo_set_wdr(M5MO_WDR_OFF);
			/* EV-P */
			m5mo_i2c_write_8bit( 0x03, 0x0A, 0x0B);
			m5mo_i2c_write_8bit( 0x03, 0x0B, 0x0B);
			/* EV */
			err = m5mo_set_ev(M5MO_EV_DEFAULT);
			/* AWB */
			err = m5mo_set_whitebalance(M5MO_WB_AUTO);
			/* AF mode / Lens Position */
			err = m5mo_set_af_mode(M5MO_AF_MODE_MACRO);
			/* Face Detection */
			//err = m5mo_set_face_detection(M5MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m5mo_set_metering(M5MO_METERING_CENTER);		
			/* Chroma Saturation */
			err = m5mo_set_saturation(M5MO_SATURATION_DEFAULT);
			/* Sharpness */
			err = m5mo_set_sharpness(M5MO_SHARPNESS_PLUS_2);
			m5mo_i2c_write_8bit( 0x02, 0x11, 0x08);
			/* Flash */
			err = m5mo_set_flash(M5MO_FLASH_CAPTURE_OFF);
			/* Emotional Color */
			m5mo_i2c_write_8bit( 0x0B, 0x1D, 0x00);
			/* ISO*/
			err = m5mo_set_iso(M5MO_ISO_AUTO);
			/* Contrast */
			err = m5mo_set_contrast(M5MO_CONTRAST_DEFAULT);
			/* Image Stabilization */
			//err = m5mo_set_isc(M5MO_ISC_STILL_OFF);
			break;	
			
		case M5MO_SCENE_SHOWWINDOW:
			/* Wide dynamic range */
			err = m5mo_set_wdr(M5MO_WDR_OFF);
			/* EV-P */
			m5mo_i2c_write_8bit( 0x03, 0x0A, 0x0C);
			m5mo_i2c_write_8bit( 0x03, 0x0B, 0x0C);
			/* EV */
			err = m5mo_set_ev(M5MO_EV_DEFAULT);
			/* AWB */
			err = m5mo_set_whitebalance(M5MO_WB_AUTO);
			/* AF mode / Lens Position */
			err = m5mo_set_af_mode(M5MO_AF_MODE_NORMAL);
			/* Face Detection */
			//err = m5mo_set_face_detection(M5MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m5mo_set_metering(M5MO_METERING_AVERAGE);		
			/* Chroma Saturation */
			err = m5mo_set_saturation(M5MO_SATURATION_DEFAULT);
			/* Sharpness */
			err = m5mo_set_sharpness(M5MO_SHARPNESS_DEFAULT);
			/* Flash */
			err = m5mo_set_flash(M5MO_FLASH_CAPTURE_OFF);
			/* Emotional Color */
			m5mo_i2c_write_8bit( 0x0B, 0x1D, 0x00);
			/* ISO*/
			err = m5mo_set_iso(M5MO_ISO_AUTO);
			/* Contrast */
			err = m5mo_set_contrast(M5MO_CONTRAST_DEFAULT);
			/* Image Stabilization */
			//err = m5mo_set_isc(M5MO_ISC_STILL_OFF);
			break;	

		case M5MO_SCENE_CANDLELIGHT:
			/* Wide dynamic range */
			err = m5mo_set_wdr(M5MO_WDR_OFF);
			/* EV-P */
			m5mo_i2c_write_8bit( 0x03, 0x0A, 0x0D);
			m5mo_i2c_write_8bit( 0x03, 0x0B, 0x0D);
			/* EV */
			err = m5mo_set_ev(M5MO_EV_DEFAULT);
			/* AWB */
			err = m5mo_set_whitebalance(M5MO_WB_DAYLIGHT);
			/* AF mode / Lens Position */
			err = m5mo_set_af_mode(M5MO_AF_MODE_NORMAL);
			/* Face Detection */
			//err = m5mo_set_face_detection(M5MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m5mo_set_metering(M5MO_METERING_AVERAGE);		
			/* Chroma Saturation */
			err = m5mo_set_saturation(M5MO_SATURATION_DEFAULT);
			/* Sharpness */
			err = m5mo_set_sharpness(M5MO_SHARPNESS_DEFAULT);
			/* Flash */
			err = m5mo_set_flash(M5MO_FLASH_CAPTURE_OFF);
			/* Emotional Color */
			m5mo_i2c_write_8bit( 0x0B, 0x1D, 0x00);
			/* ISO*/
			err = m5mo_set_iso(M5MO_ISO_AUTO);
			/* Contrast */
			err = m5mo_set_contrast(M5MO_CONTRAST_DEFAULT);
			/* Image Stabilization */
			//err = m5mo_set_isc(M5MO_ISC_STILL_OFF);
			break;
			
		case M5MO_SCENE_BACKLIGHT:
			/* Wide dynamic range */
			err = m5mo_set_wdr(M5MO_WDR_OFF);
			/* EV-P */
			m5mo_i2c_write_8bit( 0x03, 0x0A, 0x0E);
			m5mo_i2c_write_8bit( 0x03, 0x0B, 0x0E);
			/* EV */
			err = m5mo_set_ev( M5MO_EV_DEFAULT);
			/* AWB */
			err = m5mo_set_whitebalance(M5MO_WB_AUTO);
			/* AF mode / Lens Position */
			err = m5mo_set_af_mode(M5MO_AF_MODE_NORMAL);
			/* Face Detection */
			//err = m5mo_set_face_detection(M5MO_FACE_DETECTION_OFF);
			/*Photometry */
			err = m5mo_set_metering(M5MO_METERING_SPOT);		
			/* Chroma Saturation */
			err = m5mo_set_saturation(M5MO_SATURATION_DEFAULT);
			/* Sharpness */
			err = m5mo_set_sharpness(M5MO_SHARPNESS_DEFAULT);
			/* Flash */
			err = m5mo_set_flash(M5MO_FLASH_CAPTURE_ON);
			/* Emotional Color */
			m5mo_i2c_write_8bit( 0x0B, 0x1D, 0x00);
			/* ISO*/
			err = m5mo_set_iso(M5MO_ISO_AUTO);
			/* Contrast */
			err = m5mo_set_contrast(M5MO_CONTRAST_DEFAULT);
			/* Image Stabilization */
			//err = m5mo_set_isc(M5MO_ISC_STILL_OFF);			
			break;
			
		default:
			printk("[Scene]Invalid Scene value !!!\n");
			return -EINVAL;
	}

	m5mo_ctrl->settings.scenemode = mode;
	return err;
}



int get_camera_fw_id(int * fw_version)
{
	int rc = 0;
	unsigned int fw_ver = 0;
	unsigned int param_ver = 0;
	unsigned int ver;

	int i=0;
///////////////// test
#if 0
	sensor_version_info		info_data;
	char version_info[M5MO_VERSION_INFO_SIZE+1] = {0,};

	m5mo_mem_read(M5MO_VERSION_INFO_SIZE,M5MO_VERSION_INFO_ADDR,version_info);
	CAM_DEBUG("Readed version info : %s",version_info);

	m5mo_i2c_read(1, 0x00, 0x0a, &ver);
	while((ver != 0x00) &&(i <M5MO_VERSION_INFO_SIZE)  )
	{
		version_info[i++] = (char)ver;
		CAM_DEBUG("ver[%d] = %c",i,(char)ver);
		m5mo_i2c_read(1, 0x00, 0x0a, &ver);
	}
	CAM_DEBUG("Readed version info : %s",version_info);
	memcpy(&info_data, version_info,6);

	CAM_DEBUG("info_data.company : %c",info_data.company);
	CAM_DEBUG("info_data.module_vesion : %c",info_data.module_vesion);
	CAM_DEBUG("info_data.year : %c",info_data.year);
	CAM_DEBUG("info_data.month : %c",info_data.month);
#endif
////////////////

	
	if( m5mo_i2c_read(2, 0x00, 0x02, &fw_ver) < 0 )
	{

		return -1;
	}
	printk("<=PCAM=> M5MO fw_id : %x\n", fw_ver);
	(*fw_version) = fw_ver;


	/* parameter version - 5055 */
	m5mo_i2c_read(2, 0x00, 0x06, &param_ver);

	printk("#################################\n");
	printk("#### Firmware version : %x \t####\n", fw_ver);
	printk("#### Parameter version : %x \t####\n", param_ver);
	printk("#################################\n");
	
	return rc;
}


static int m5mo_start(void)
{
	int rc=0;
	unsigned int value;
	u32 ver;
	
	//gpio_set_value(2,0);

	/* start camera */
	m5mo_i2c_write_8bit(0x0F, 0x12, 0x01);
	//msleep(1000);		// minimum 6ms
	msleep(100);		// minimum 6ms 


	/* change to paramset mode */
/*	m5mo_i2c_write_8bit(0x00, 0x0B, 0x01);
	m5mo_i2c_verify(0x00, 0x0B, 0x01);	
*/	
	/* read firmware version */
	rc = get_camera_fw_id(&ver);
	
	/* reverse and mirrorring */
/*	m5mo_i2c_write_8bit(0x02, 0x05, 0x01);
	m5mo_i2c_write_8bit(0x02, 0x06, 0x01);
	m5mo_i2c_write_8bit(0x02, 0x07, 0x01);
	m5mo_i2c_write_8bit(0x02, 0x08, 0x01);
*/	

	/* AE/AWB Unlock */
//	m5mo_i2c_write_8bit(0x03, 0x00, 0x00); 
//	m5mo_i2c_write_8bit(0x06, 0x00, 0x00);
	
	/* set auto flicker - 60Hz */
	m5mo_i2c_write_8bit(0x03, 0x06, 0x02);

	// set Thumbnail JPEG size
	m5mo_i2c_write_32bit(0x0B, 0x3C, 0xF000);
/*	m5mo_i2c_write_8bit(0x0B, 0x3F, 0x00);	
	m5mo_i2c_write_8bit(0x0B, 0x3E, 0xF0);	
	m5mo_i2c_write_8bit(0x0B, 0x3D, 0x00);	
	m5mo_i2c_write_8bit(0x0B, 0x3C, 0x00);	*/
#if 0
	/* set monitor(preview) size - QVGA*/
	CAM_DEBUG("set Preview size as QVGA");
	m5mo_i2c_write_8bit(0x01, 0x01, 0x09);	// 320x240
	CAM_DEBUG("set Preview size as 320x240");
	m5mo_ctrl->settings.preview_size.width = 320;
	m5mo_ctrl->settings.preview_size.height = 240;
	m5mo_ctrl->settings.preview_size_idx = 0;
#endif

#if 1
	/* set monitor(preview) size - VGA*/
	CAM_DEBUG("set Preview size as VGA");
	m5mo_i2c_write_8bit(0x01, 0x01, 0x17);	// 640x480
	CAM_DEBUG("set Preview size as 640x480");
	m5mo_ctrl->settings.preview_size.width = 640;
	m5mo_ctrl->settings.preview_size.height = 480;
	m5mo_ctrl->settings.preview_size_idx = 0;
#endif
	
#if 0	
	//CAM_DEBUG("set Preview size as 720P");
	//m5mo_i2c_write_8bit(0x01, 0x01, 0x21);	// 1280x720	: 720P HD mode
	//m5mo_i2c_write_8bit(0x01, 0x01, 0x25);	// 1920x1080
	m5mo_ctrl->settings.preview_size.width = 1280;
	m5mo_ctrl->settings.preview_size.height = 720;
#endif	
#if 0
	CAM_DEBUG("set Preview size as 800x600");
	m5mo_i2c_write_8bit(0x01, 0x01, 0x1F);	// 800x600
	m5mo_ctrl->settings.preview_size.width = 800;
	m5mo_ctrl->settings.preview_size.height = 600;
	m5mo_ctrl->settings.preview_size_idx = 0;
#endif
	/* clear interrupt */
	m5mo_i2c_read(1,  0x00, 0x10, &value);		// added 20101126
	/* start YUV output */
	m5mo_i2c_write_8bit( 0x00, 0x11, M5MO_INT_MODE);

	m5mo_i2c_write_8bit(0x00, 0x0B, M5MO_MONITOR_MODE);
	m5mo_i2c_verify(0x00, 0x0B, M5MO_MONITOR_MODE);
	
	wait_interrupt_mode(M5MO_INT_MODE);

	return rc;
}


void m5mo_dump_registers(int category, int start_addr,int end_addr)
{
	int i=start_addr;
	unsigned int value=0;
	
	for(; i< end_addr; i++) {
		m5mo_i2c_read(1, category, i, &value);
		printk("0x%x : 0x%x : 0x%x \n", category, i, value);
	}
}
//static long m5mo_set_sensor_mode(enum sensor_mode_t mode)
static long m5mo_set_sensor_mode(int mode)
{
//	int i=0;
	unsigned int value=0;
	unsigned int size=0;
//	int cnt = 0;
//	int wait_count=5;

/*
	if(m5mo_ctrl->settings.started == 0)
	{
		m5mo_start();
		m5mo_ctrl->settings.started = 1;
	}
*/
	switch (mode) 
	{
	case SENSOR_PREVIEW_MODE:
		printk("SENSOR_PREVIEW_MODE START\n");
		
		/* Flash Test */
		//m5mo_set_flash(M5MO_FLASH_MOVIE_ON);
	

		m5mo_i2c_read(1, 0x00, 0x0B, &value);

		if(value != M5MO_MONITOR_MODE)
		{
			/* Interrupt Clear */
			m5mo_i2c_read(1,  0x00, 0x10, &value);
			CAM_DEBUG("INT STATUS(0x10 ) = 0x%x\n",value);

			/* Enable YUV-Output Interrupt */
			m5mo_i2c_write_8bit( 0x00, 0x11, M5MO_INT_MODE);
			
			/* Enable Interrupt Control */
			//m5mo_i2c_write_8bit( 0x00, 0x12, 0x01);

			/* Set Monitor Mode */
			m5mo_i2c_write_8bit(0x00, 0x0B, M5MO_MONITOR_MODE);
			//msleep(100);			

			/* AE/AWB Unlock */
			m5mo_i2c_write_8bit(0x03, 0x00, 0x00); 
			m5mo_i2c_write_8bit(0x06, 0x00, 0x00);

			/* wait interrupt - INT_STATUS_MODE */
			wait_interrupt_mode(M5MO_INT_MODE);

		}
		break;

	case SENSOR_SNAPSHOT_MODE:
		CAM_DEBUG("SENSOR_SNAPSHOT_MODE START\n");

		/* temporary code for DV - always Flash On when capture starts */
//		m5mo_set_flash(M5MO_FLASH_CAPTURE_ON);
		
		
		/* set Thumbnail size - 160x120 */
		//printk("<=PCAM=> Thumbnail size~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
		//m5mo_i2c_write_8bit(0x0B, 0x0B, 0x02);
		
		/* set format - Main JPEG + Thumbnail YUV422 */
		CAM_DEBUG(" Capture Format : main + Thumbnail JPEG + Thumbnail YUV");
		m5mo_i2c_write_8bit(0x0B, 0x00, 0x20);	// main + Thumbnail JPEG + Thumbnail YUV
		//m5mo_i2c_write_8bit(0x0B, 0x00, 0x10);	// main + Thumbnail JPEG
		//m5mo_i2c_write_8bit(0x0B, 0x00, 0x02);	// test - main JPEG only
		//m5mo_i2c_write_8bit(0x0B, 0x00, 0x00);	// test - YUV format


		/* lock AE/AWB */
		m5mo_i2c_write_8bit(0x03, 0x00, 0x01);
		m5mo_i2c_write_8bit(0x06, 0x00, 0x01);

		/* set JPEG size */
//		m5mo_i2c_write_32bit(0x0B, 0x0F, 0x3A0000);
//		m5mo_i2c_write_32bit(0x0B, 0x3C, 0x20000);

		/* interrupt clear */
		m5mo_i2c_read(1, 0x00, 0x10, &value);
		
		/* enable capture  interrupt */
		m5mo_i2c_write_8bit(0x00, 0x11, M5MO_INT_CAPTURE);
		
		/* set still capture mode */
		m5mo_i2c_write_8bit(0x00, 0x0B, M5MO_STILLCAP_MODE);
		msleep(200);

		/* dummy code from GT-i8330 */
		//m5mo_i2c_write_8bit(0x00, 0x11, 0x00);
		
		/* read back 0x00,0x0B register */
		m5mo_i2c_verify(0x00, 0x0B, M5MO_STILLCAP_MODE);

		wait_interrupt_mode(M5MO_INT_CAPTURE);
		
		
		break;

	
	case SENSOR_SNAPSHOT_TRANSFER:
		CAM_DEBUG("SENSOR_SNAPSHOT_TRANSFER START (200ms delay added) ~\n");

//		m5mo_dump_registers(0xA,0x00,0x37);
//		m5mo_dump_registers(0x9,0x00,0x16);
		
					
		/* select main image */
		m5mo_i2c_write_8bit(0x0C, 0x06, 0x01);		
		
		/* interrupt clear */
		m5mo_i2c_read(1, 0x00, 0x10, &value);
		
		/* enable capture transfer interrupt */
		m5mo_i2c_write_8bit(0x00, 0x11, M5MO_INT_CAPTURE);
		
		/* read back 0x00,0x0B register */
		m5mo_i2c_verify(0x00, 0x0B, M5MO_STILLCAP_MODE);
		
		/* start trasnfer */
		m5mo_i2c_write_8bit(0x0C, 0x09, 0x01);

		msleep(200);
		wait_interrupt_mode(M5MO_INT_CAPTURE);

		/* get exif data */
		m5mo_set_exif_data();

#if 1
		/* read main JPEG size */
		value=0;
		m5mo_i2c_read(1, 0x0B, 0x12, &value);	
		size |= value;value=0;
		m5mo_i2c_read(1, 0x0B, 0x11, &value);	
		size |= (value<<8);value=0;
		m5mo_i2c_read(1, 0x0B, 0x10, &value);	
		size |= (value<<16);value=0;
		m5mo_i2c_read(1, 0x0B, 0x0F, &value);			
		size |= (value<<24);value=0;
		CAM_DEBUG("Main JPEG size : 0x%x",size);
		
		/* read thumbnail size */
		size =0; value=0;
		m5mo_i2c_read(1, 0x0B, 0x3F, &value);	
		size |= value;value=0;
		m5mo_i2c_read(1, 0x0B, 0x3E, &value);	
		size |= (value<<8);value=0;
		m5mo_i2c_read(1, 0x0B, 0x3D, &value);	
		size |= (value<<16);value=0;
		m5mo_i2c_read(1, 0x0B, 0x3C, &value);			
		size |= (value<<24);value=0;
		CAM_DEBUG("Thumnail size : 0x%x",size);
#endif

		//m5mo_i2c_write_8bit(0x00, 0x11, M5MO_INT_MODE);

#ifdef EARLY_PREVIEW_AFTER_CAPTURE
		CAM_DEBUG("Early preivew after capture ");
		msleep(50);
			
			/* start YUV output */
		m5mo_i2c_write_8bit( 0x00, 0x11, M5MO_INT_MODE);

		m5mo_i2c_write_8bit(0x00, 0x0B, M5MO_MONITOR_MODE);
		m5mo_i2c_verify(0x00, 0x0B, M5MO_MONITOR_MODE);
		
		/* AE/AWB Unlock */
		m5mo_i2c_write_8bit(0x03, 0x00, 0x00); 
		m5mo_i2c_write_8bit(0x06, 0x00, 0x00);
		
		wait_interrupt_mode(M5MO_INT_MODE);
#endif
		

		
		break;

	default:
		return -EFAULT;
	}

	return 0;
}


int cam_pmic_onoff(int onoff)
{
/*	static int last_state = -1;

	if(last_state == onoff)
	{
		CAM_DEBUG("%s : CAM PMIC already %d\n",__FUNCTION__,onoff);
		return 0;
	}
*/	
	if(onoff)		// ON
	{
		CAM_DEBUG(" : ON\n");

		//gpio_direction_output(CAM_PMIC_STBY, LOW);		// set PMIC to STBY mode
		gpio_set_value(CAM_PMIC_STBY, 0);
		msleep(2);

		if(system_rev >= 4)
		{
			CAM_DEBUG(" ----------- New Camera Power Sequence -----------\n");
			#if 0	// before 20101111
			cam_pm_lp8720_i2c_write(0x06, 0x09);
			cam_pm_lp8720_i2c_write(0x07, 0x09); // BUCKS2: ISP_CORE 1.2V, no delay
			cam_pm_lp8720_i2c_write(0x04, 0x45); // LDO4:	   SENSOR_CORE 1.2V, 2*ts delay
			cam_pm_lp8720_i2c_write(0x02, 0x46); // LDO2 :   CAM_DVDD 1.5V, 2*ts delay
			cam_pm_lp8720_i2c_write(0x01, 0x6C); // LDO1 :   CAM_HOST 1.8V, 3*ts delay
			cam_pm_lp8720_i2c_write(0x05, 0x6C); // LDO5 :   SENSOR_IO 1.8V, 3*ts delay
			#else
			cam_pm_lp8720_i2c_write(0x00, 0x04); // set Time Step as 50us
			cam_pm_lp8720_i2c_write(0x06, 0x09);
			cam_pm_lp8720_i2c_write(0x07, 0x09); // BUCKS2: ISP_CORE 1.2V, no delay
			cam_pm_lp8720_i2c_write(0x04, 0x45); // LDO4:	   SENSOR_CORE 1.2V, 2*ts delay
			cam_pm_lp8720_i2c_write(0x02, 0xA6); // LDO2 :   CAM_DVDD 1.5V, 5*ts delay
			cam_pm_lp8720_i2c_write(0x01, 0xCC); // LDO1 :   CAM_HOST 1.8V, 6*ts delay
			cam_pm_lp8720_i2c_write(0x05, 0xCC); // LDO5 :   SENSOR_IO 1.8V, 6*ts delay
			#endif
			
		}
		else
		{
			/*
			cam_pm_lp8720_i2c_write(0x07, 0x09); // BUCKS2:1.2V, no delay
			cam_pm_lp8720_i2c_write(0x04, 0x05); // LDO4:1.2V, no delay
			cam_pm_lp8720_i2c_write(0x02, 0x19); // LDO2 :2.8V, no delay
			cam_pm_lp8720_i2c_write(0x05, 0x0C); // LDO5 :1.8V, no delay
			cam_pm_lp8720_i2c_write(0x01, 0x19); // LDO1 :2.8V, no delay
			cam_pm_lp8720_i2c_write(0x08, 0xBB); // Enable all power without LDO3
			*/
			cam_pm_lp8720_i2c_write(0x06, 0x09);
			cam_pm_lp8720_i2c_write(0x07, 0x09); // BUCKS2: ISP_CORE 1.2V, no delay
			cam_pm_lp8720_i2c_write(0x04, 0x65); // LDO4:	   SENSOR_CORE 1.2V, 3*ts delay
			cam_pm_lp8720_i2c_write(0x02, 0x59); // LDO2 :   SENSOR_A 2.8V, 2*ts delay
			cam_pm_lp8720_i2c_write(0x05, 0x6C); // LDO5 :   SENSOR_IO 1.8V, 3*ts delay
			cam_pm_lp8720_i2c_write(0x01, 0x79); // LDO1 :   CAM_AF 2.8V, 3*ts delay
			cam_pm_lp8720_i2c_write(0x08, 0xBB); // Enable all power without LDO3
		}		
		
		
		gpio_set_value(CAM_PMIC_STBY, 1);

		if(system_rev >= 4)
		{
			usleep(200);
			/* CAM_SENSOR_A2.8V out */
			gpio_set_value_cansleep(CAM_SENSOR_A_EN, 1);
			gpio_set_value_cansleep(CAM_SENSOR_A_EN_ALTER, 1);		// beyond Rev07

			/* CAM_AF_2.8V out */
			vreg_CAM_AF28 = vreg_get(NULL, "gp10");
			if (IS_ERR(vreg_CAM_AF28)) {
				pr_err("%s: VREG GP10 get failed %ld\n", __func__,
					PTR_ERR(vreg_CAM_AF28));
				vreg_CAM_AF28 = NULL;
			}
			if (vreg_set_level(vreg_CAM_AF28, 2800)) {
				pr_err("%s: VREG GP10 set failed\n", __func__);
			}

			if (vreg_enable(vreg_CAM_AF28)) {
				pr_err("%s: VREG GP10 enable failed\n", __func__);
			}
		}
		msleep(5);
	}
	else
	{
		CAM_DEBUG(" : OFF\n",__FUNCTION__);

		gpio_set_value(CAM_PMIC_STBY, 0);
		if(system_rev >= 4)
		{
			gpio_set_value_cansleep(CAM_SENSOR_A_EN, 0);
			mdelay(1);
			gpio_set_value_cansleep(CAM_SENSOR_A_EN_ALTER, 0);
			mdelay(1);
			vreg_disable(vreg_CAM_AF28);
		}
	}

	return 0;
}

static int m5mo_init_regs(void)
{
	return 0;
}

static int m5mo_sensor_init_probe(struct msm_camera_sensor_info *data)
{
	int rc = 0;
//	int read_value=-1;
//	unsigned short read_value_1 = 0;
//	int i; //for loop
//	int cnt = 0;
//	int fd=0;

#ifdef	FORCE_FIRMWARE_UPDATE
	struct file* fp;
#endif
	CDBG("init entry \n");


	CAM_DEBUG("m5mo_sensor_init_probe start");

	
	CAM_DEBUG(" 1. CAM_8M_RST = 0 ");
	gpio_set_value_cansleep(CAM_8M_RST, LOW);

	CAM_DEBUG(" 2. CAM_MEGA_EN = 0 ");
	if (system_rev >= 4) 
		gpio_set_value(CAM_MEGA_EN_REV04, LOW);
	else
		gpio_set_value(CAM_MEGA_EN, LOW);

	////////////////////////// test
	CAM_DEBUG(" 2-1. CAM_VGA_EN = 1 ");
	gpio_set_value_cansleep(CAM_VGA_EN, HIGH);
	msleep(1);
	CAM_DEBUG(" 2-2. CAM_VGA_RST = 1 ");
	gpio_set_value_cansleep(CAM_VGA_RST, HIGH);
	msleep(1);
	CAM_DEBUG(" 2-3. CAM_VGA_EN = 0 ");
	gpio_set_value_cansleep(CAM_VGA_EN, LOW);
	////////////////////////// 
	CAM_DEBUG(" 3. PMIC ON ");
	//cam_pmic_onoff(ON);

#if 0
////////////////////////////////////////////////////
#if 0//Mclk_timing for M4Mo spec.		// -Jeonhk clock was enabled in vfe31_init
	msm_camio_clk_enable(CAMIO_VFE_CLK);
	msm_camio_clk_enable(CAMIO_MDC_CLK);
	msm_camio_clk_enable(CAMIO_VFE_MDC_CLK);
#endif	

	CAM_DEBUG("START MCLK:24Mhz~~~~~");
//	msm_camio_clk_rate_set(24000000);
	msleep(5);
	msm_camio_camif_pad_reg_reset();		// this is not
	msleep(10);
////////////////////////////////////////////////////
#endif
	CAM_DEBUG(" 4. CAM_MEGA_EN = 1 ");
	if (system_rev >= 4) 
		gpio_set_value(CAM_MEGA_EN_REV04, HIGH);
	else
		gpio_set_value(CAM_MEGA_EN, HIGH);		
	msleep(1);

	CAM_DEBUG(" 5. CAM_8M_RST = 1 ");
	gpio_set_value_cansleep(CAM_8M_RST, HIGH);
	//msleep(30); // min 350ns
	msleep(5);

#ifdef FORCE_FIRMWARE_UPDATE
	if((firmware_update_mode == 0 ) && (fp = filp_open(M5MO_FW_PATH_SDCARD, O_RDONLY, S_IRUSR)) > 0)
	{
		printk(" #### FIRMWARE UPDATE MODE ####\n");
		firmware_update_mode = 1;
		filp_close(fp, current->files);

		m5mo_load_fw(2);
	}
	else
	{
#endif
		m5mo_start();
#ifdef FORCE_FIRMWARE_UPDATE
	}
#endif

	return rc;
}




int m5mo_sensor_init(struct msm_camera_sensor_info *data)
{
	int rc = 0;

	printk("[PGH] %s 1111111\n", __func__);

	
	m5mo_ctrl = kzalloc(sizeof(struct m5mo_ctrl_t), GFP_KERNEL);
	if (!m5mo_ctrl) {
		CDBG("m5mo_sensor_init failed!\n");
		rc = -ENOMEM;
		goto init_done;
	}	
	
	if (data)
		m5mo_ctrl->sensordata = data;

	m5mo_exif = kzalloc(sizeof(struct m5mo_exif_data), GFP_KERNEL);
	if (!m5mo_exif) {
		CDBG("allocate m5mo_exif_data failed!\n");
		rc = -ENOMEM;
		goto init_done;
	}

#if 1//PGH
	/* Input MCLK = 24MHz */
	CAM_DEBUG("set MCLK 24MHz");
	msm_camio_clk_rate_set(24000000);
	msleep(5);

	msm_camio_camif_pad_reg_reset();
#endif//PGH

	printk("[PGH] %s 222222\n", __func__);
  rc = m5mo_sensor_init_probe(data);
	if (rc < 0) {
		CDBG("m5mo_sensor_init failed!\n");
		goto init_fail;
	}

	printk("[PGH] %s 3333  rc:%d\n", __func__, rc);
init_done:
	return rc;

init_fail:
	kfree(m5mo_ctrl);
	kfree(m5mo_exif);
	return rc;
}

static int m5mo_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&m5mo_wait_queue);
	return 0;
}

int m5mo_sensor_ext_config(void __user *argp)
{
	sensor_ext_cfg_data		cfg_data;
	int rc=0;

	if(copy_from_user((void *)&cfg_data, (const void *)argp, sizeof(cfg_data)))
	{
		printk("<=PCAM=> %s fail copy_from_user!\n", __func__);
	}

	CDBG("m5mo_sensor_ext_config, cmd = %d ",cfg_data.cmd);
	CAM_DEBUG("cmd = %d, param1 = %d",cfg_data.cmd,cfg_data.value_1);
	switch(cfg_data.cmd) {
	case EXT_CFG_SET_FLASH:
		rc = m5mo_set_flash(cfg_data.value_1);
		break;
		
	case EXT_CFG_SET_SCENE:
		rc = m5mo_set_scene(cfg_data.value_1);
		break;
		
	case EXT_CFG_SET_SHARPNESS:
		rc = m5mo_set_sharpness(cfg_data.value_1);
		break;

	case EXT_CFG_SET_EFFECT:
		rc = m5mo_set_effect(cfg_data.value_1);
		break;
		
	case EXT_CFG_SET_SATURATION:
		rc = m5mo_set_saturation(cfg_data.value_1);
		break;

	case EXT_CFG_SET_ISO:
		rc = m5mo_set_iso(cfg_data.value_1);
		break;

	case EXT_CFG_SET_WB:
		rc = m5mo_set_whitebalance(cfg_data.value_1);
		break;

	case EXT_CFG_SET_CONTRAST:
		rc = m5mo_set_contrast(cfg_data.value_1);
		break;

	case EXT_CFG_SET_BRIGHTNESS:
		rc = m5mo_set_ev(cfg_data.value_1);
		break;

	case EXT_CFG_SET_ZOOM:
		rc = m5mo_set_zoom(cfg_data.value_1);
		break;

	case EXT_CFG_SET_FPS:
		rc = m5mo_set_fps(cfg_data.value_1,cfg_data.value_2);
		break;

	case EXT_CFG_SET_AF_MODE:
		rc = m5mo_set_af_mode(cfg_data.value_1);
		break;

	case EXT_CFG_SET_AF_START:
		rc = m5mo_set_af_status(1);
		break;
		
	case EXT_CFG_SET_AF_STOP:
		rc = m5mo_set_af_status(0);
		break;

	case EXT_CFG_GET_AF_STATUS:
		rc = m5mo_get_af_status(&cfg_data.value_1);
		break;
		
	case EXT_CFG_SET_FACE_DETECT:
		rc = m5mo_set_face_detection(cfg_data.value_1);
		break;

	case EXT_CFG_SET_METERING:	// auto exposure mode
		rc = m5mo_set_metering(cfg_data.value_1);
		break;
		
	case EXT_CFG_SET_CONTINUOUS_AF:	
		rc = m5mo_set_continuous_af(cfg_data.value_1);
		break;

	case EXT_CFG_SET_PREVIEW_SIZE:	
		rc = m5mo_set_preview_size(cfg_data.value_1);
		break;

	case EXT_CFG_SET_PICTURE_SIZE:	
		rc = m5mo_set_picture_size(cfg_data.value_1,cfg_data.value_2);
		break;

	case EXT_CFG_SET_JPEG_QUALITY:	
		rc = m5mo_set_jpeg_quality(cfg_data.value_1);
		break;
		
	case EXT_CFG_SET_TOUCHAF_POS:	
		rc = m5mo_set_touchaf_pos(cfg_data.value_1,cfg_data.value_2);
		break;

	case EXT_CFG_SET_ANTISHAKE:
		rc = m5mo_set_antishake_mode(cfg_data.value_1);
		break;
		
	case EXT_CFG_SET_WDR:
		rc = m5mo_set_wdr(cfg_data.value_1);
		break;


	case EXT_CFG_SET_EXIF:
			{
				printk("<=PCAM=> EXT_CFG_SET_EXIF~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
		rc = m5mo_get_exif(&cfg_data.value_1, &cfg_data.value_2);

		printk("<=PCAM=> cfg_data.value_1 : %x cfg_data.value_2 : %x\n", cfg_data.value_1, cfg_data.value_2);
		}
		break;
		
	case EXT_CFG_SET_DTP:
		rc = m5mo_set_dtp(&cfg_data.value_1);
		break;

	case EXT_CFG_SET_AE_AWB:
		rc = m5mo_set_ae_awb(cfg_data.value_1);
		break;

	default:
		break;
	}

	if(copy_to_user((void *)argp, (const void *)&cfg_data, sizeof(cfg_data)))
	{
		printk(" %s : copy_to_user Failed \n", __func__);
	}
	
	return rc;	
}

int m5mo_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cfg_data;
	long   rc = 0;

	if (copy_from_user(
				&cfg_data,
				(void *)argp,
				sizeof(struct sensor_cfg_data)))
		return -EFAULT;

	/* down(&m5mo_sem); */

	CDBG("m5mo_sensor_config, cfgtype = %d, mode = %d\n",
		cfg_data.cfgtype, cfg_data.mode);

	switch (cfg_data.cfgtype) {
	case CFG_SET_MODE:
		rc = m5mo_set_sensor_mode(cfg_data.mode);
		break;

	case CFG_SET_EFFECT:
		//rc = m5mo_set_effect(cfg_data.mode, cfg_data.cfg.effect);
		rc = m5mo_set_effect(cfg_data.cfg.effect);
		break;
	case CFG_SET_BRIGHTNESS:		
		//rc = m5mo_set_brightness(cfg_data.cfg.brightness);
		rc = m5mo_set_ev(cfg_data.cfg.brightness);
		break;

	case CFG_SET_WB:
		//rc = m5mo_set_whitebalance(cfg_data.mode, cfg_data.cfg.whitebalance);
		rc = m5mo_set_whitebalance(cfg_data.cfg.whitebalance);
		break;

	case CFG_SET_ISO:
		//rc = m5mo_set_iso(cfg_data.mode, cfg_data.cfg.iso);
		rc = m5mo_set_iso(cfg_data.cfg.iso);
		break;
		
	case CFG_SET_CONTRAST:
		//rc = m5mo_set_contrast(cfg_data.mode, cfg_data.cfg.contrast);
		rc = m5mo_set_contrast(cfg_data.cfg.contrast);
		break;

	case CFG_SET_SATURATION:
		//rc = m5mo_set_saturation(cfg_data.mode, cfg_data.cfg.saturation);
		rc = m5mo_set_saturation(cfg_data.cfg.saturation);
		break;

	
	case CFG_SET_EXPOSURE_MODE:	
		//rc = m5mo_set_metering(cfg_data.mode, cfg_data.cfg.metering);
		rc = m5mo_set_metering(cfg_data.cfg.metering);
		break;

	case CFG_SET_ZOOM:
		//rc = m5mo_set_zoom(cfg_data.mode, cfg_data.cfg.zoom);
		break;
		
	default:
		rc = -EFAULT;
		break;
	}

	/* up(&m5mo_sem); */

	return rc;
}

int m5mo_sensor_update_firmware(void __user *argp)
{
	sensor_ext_cfg_data		cfg_data;
	long   rc = 0;

	if (copy_from_user(
				&cfg_data,
				(void *)argp,
				sizeof(cfg_data)))
		return -EFAULT;

	firmware_update_mode = cfg_data.cmd;
	printk("%s : cmd = %d, from_sdcadr = %d\n",__FUNCTION__, cfg_data.cmd,cfg_data.value_1);
	switch(firmware_update_mode)
	{
	case UPDATE_M5MO_FW:
		printk("#### FIRMWARE UPDATE START ! ####\n");
		m5mo_reset_for_update();
		
		rc = m5mo_load_fw(cfg_data.value_1);
		if(rc == 0)
		{
			printk("#### FIRMWARE UPDATE SUCCEEDED ! ####\n");
			firmware_update_mode = 0;
		}
		else
		{
			printk("#### FIRMWARE UPDATE FAILED ! ####\n");
		}
		break;
		
	case READ_FW_VERSION:
		get_camera_fw_id(&cfg_data.value_1);
		break;
	}


	if(copy_to_user((void *)argp, (const void *)&cfg_data, sizeof(cfg_data)))
	{
		printk(" %s : copy_to_user Failed \n", __func__);
	}
	return 0;
}

int m5mo_sensor_read_version_info(void __user *argp)
{
	sensor_version_info		info_data;
	long   rc = 0;
	char version_info[M5MO_VERSION_INFO_SIZE+1] = {0,};
	unsigned int ver;
	int i=0;
	
	if (copy_from_user(
				&info_data,
				(void *)argp,
				sizeof(info_data)))
		return -EFAULT;

	/*
	m5mo_mem_read(M5MO_VERSION_INFO_SIZE,M5MO_VERSION_INFO_ADDR,version_info);
	CAM_DEBUG("Readed version info : %s",version_info);
	*/
	m5mo_i2c_read(1, 0x00, 0x0a, &ver);
	while((ver != 0x00) &&(i <M5MO_VERSION_INFO_SIZE)  )
	{
		version_info[i] = (char)ver;
		CAM_DEBUG("ver[%d] = %c",i,(char)ver);
		i++;
		m5mo_i2c_read(1, 0x00, 0x0a, &ver);
	}
	CAM_DEBUG("! Readed version info : %s",version_info);
	memcpy(&info_data, version_info,6);

	CAM_DEBUG("info_data.company : %c",info_data.company);
	CAM_DEBUG("info_data.module_vesion : %c",info_data.module_vesion);
	CAM_DEBUG("info_data.year : %c",info_data.year);
	CAM_DEBUG("info_data.month : %c",info_data.month);

	if(copy_to_user((void *)argp, (const void *)&info_data, sizeof(info_data)))
	{
		printk(" %s : copy_to_user Failed \n", __func__);
	}
	return 0;
}

int m5mo_sensor_release(void)
{
	int rc = 0;

	/* down(&m5mo_sem); */
	m5mo_ctrl->settings.started = 0;
	
	CAM_DEBUG("POWER OFF");
	printk("camera turn off\n");

	CAM_DEBUG(" 1. CAM_8M_RST = 0 ");
	gpio_set_value_cansleep(CAM_8M_RST, LOW);
	msleep(3);
	////////////////////// test
	CAM_DEBUG(" 2-2. CAM_VGA_RST = 0 ");
	gpio_set_value_cansleep(CAM_VGA_RST, LOW);
	//////////////////////

	
	CAM_DEBUG(" 2. CAM_MEGA_EN = 0 ");
	gpio_set_value(CAM_MEGA_EN, LOW);

	CAM_DEBUG(" 2. CAM_PMIC_STBY = 0 ");
	gpio_set_value(CAM_PMIC_STBY, 0);

/*
	if(system_rev >= 4)
	{
		CAM_DEBUG(" 2. CAM_SENSOR_A_EN = 0 ");
		gpio_set_value_cansleep(CAM_SENSOR_A_EN, 0);

		CAM_DEBUG(" 2. CAM_SENSOR_A_EN_ALTER = 0 ");
		gpio_set_value_cansleep(CAM_SENSOR_A_EN_ALTER, 0);

		vreg_disable(vreg_CAM_AF28);
	}
*/
	cam_pmic_onoff(0);
	
	kfree(m5mo_ctrl);
	kfree(m5mo_exif);
	/* up(&m5mo_sem); */

	return rc;
}






//#if defined(CONFIG_SAMSUNG_TARGET)
#if 1
static int cam_pm_lp8720_init_client(struct i2c_client *client)
{
	/* Initialize the cam_pm_lp8720 Chip */
	init_waitqueue_head(&cam_pm_lp8720_wait_queue);
	return 0;
}
#endif 




//PGH:KERNEL2.6.25static int m5mo_probe(struct i2c_client *client)
static int m5mo_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int rc = 0;

	CAM_DEBUG("%s E\n",__FUNCTION__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		goto probe_failure;
	}

	m5mo_sensorw =
		kzalloc(sizeof(struct m5mo_work_t), GFP_KERNEL);

	if (!m5mo_sensorw) {
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, m5mo_sensorw);
	m5mo_init_client(client);
	m5mo_client = client;

	CDBG("m5mo_i2c_probe successed!\n");
	CAM_DEBUG("%s X\n",__FUNCTION__);


	return 0;

probe_failure:
	kfree(m5mo_sensorw);
	m5mo_sensorw = NULL;
	CDBG("m5mo_i2c_probe failed!\n");
	CAM_DEBUG("m5mo_i2c_probe failed!\n");
	return rc;
}


static int __exit m5mo_i2c_remove(struct i2c_client *client)
{

	struct m5mo_work_t *sensorw = i2c_get_clientdata(client);
	free_irq(client->irq, sensorw);
//	i2c_detach_client(client);
	m5mo_client = NULL;
	m5mo_sensorw = NULL;
	kfree(sensorw);
	return 0;

}


static const struct i2c_device_id m5mo_id[] = {
    { "m5mo_i2c", 0 },
    { }
};

//PGH MODULE_DEVICE_TABLE(i2c, m5mo);

static struct i2c_driver m5mo_i2c_driver = {
	.id_table	= m5mo_id,
	.probe  	= m5mo_i2c_probe,
	.remove 	= __exit_p(m5mo_i2c_remove),
	.driver 	= {
		.name = "m5mo",
	},
};



//#if defined(CONFIG_SAMSUNG_TARGET)
#if 1
static int cam_pm_lp8720_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct cam_pm_lp8720_data *mt;
	int err = 0;

	CAM_DEBUG("%s E\n",__FUNCTION__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENOTSUPP;
		goto cam_pm_lp8720_probe_failure;
	}

	if(!(mt = kzalloc( sizeof(struct cam_pm_lp8720_data), GFP_KERNEL))) {
		err = -ENOMEM;
		goto cam_pm_lp8720_alloc_data_failed;
	}
	
	i2c_set_clientdata(client, mt);
	cam_pm_lp8720_init_client(client);
	cam_pm_lp8720_pclient = client;	

	CAM_DEBUG("%s X \n",__FUNCTION__);
	return 0;

cam_pm_lp8720_probe_failure:
cam_pm_lp8720_alloc_data_failed:
	CAM_DEBUG("m5mo_i2c_probe failed!\n");
	return err;
	
}

static int cam_pm_lp8720_remove(struct i2c_client *client)
{
	struct cam_pm_lp8720_data *mt = i2c_get_clientdata(client);

// free_irq(client->irq, mt);
//	i2c_detach_client(client);
	cam_pm_lp8720_pclient = NULL;
//	misc_deregister(&m5mo_device);
	kfree(mt);
	return 0;
}

static const struct i2c_device_id cam_pm_lp8720_id[] = {
    { "cam_pm_lp8720_i2c", 0 },
    { }
};

//PGH MODULE_DEVICE_TABLE(i2c, cam_pm_lp8720_id);


static struct i2c_driver cam_pm_lp8720_driver = {
	.id_table 	= cam_pm_lp8720_id,
	.probe  	= cam_pm_lp8720_probe,
	.remove 	= cam_pm_lp8720_remove,
	.driver 	= {
		.name = "cam_pm_lp8720_i2c",
	},
};
#endif//PGH FOR CAM PM LP8720 ////////////////////////////////////////







int32_t m5mo_i2c_init(void)
{
	int32_t rc = 0;

	CAM_DEBUG("%s E\n",__FUNCTION__);

	rc = i2c_add_driver(&m5mo_i2c_driver);

	if (IS_ERR_VALUE(rc))
		goto init_failure;

	rc = i2c_add_driver(&cam_pm_lp8720_driver);

	if (IS_ERR_VALUE(rc))
		goto init_failure_cam_pm;


	return rc;



init_failure:
	CDBG("failed to m5mo_i2c_init, rc = %d\n", rc);
	return rc;

init_failure_cam_pm:
	CDBG("failed to cam_pm_i2c_init, rc = %d\n", rc);
	return rc;
}


void m5mo_exit(void)
{
	i2c_del_driver(&m5mo_i2c_driver);
//PGH CHECK 	i2c_del_driver(&cam_pm_lp8720_driver);
	
}


//int m5mo_sensor_probe(void *dev, void *ctrl)
int m5mo_sensor_probe(const struct msm_camera_sensor_info *info,
				struct msm_sensor_ctrl *s)
{
	int rc = 0;

	CAM_DEBUG("%s E\n",__FUNCTION__);
/*	struct msm_camera_sensor_info *info =
		(struct msm_camera_sensor_info *)dev; 

	struct msm_sensor_ctrl *s =
		(struct msm_sensor_ctrl *)ctrl;
*/

#if 0//PGH
	CAM_DEBUG("START 5M SWI2C VER0.2 : %d", rc);

	gpio_direction_output(CAM_FLASH_EN, LOW); //CAM_FLASH_EN
	gpio_direction_output(CAM_FLASH_SET, LOW); //CAM_FLASH_SET
#endif//PGH

	rc = m5mo_i2c_init();
	if (rc < 0)
		goto probe_done;

	/* Input MCLK = 24MHz */
#if 1//PGH
	CAM_DEBUG("set MCLK 24MHz");
	msm_camio_clk_rate_set(24000000);
//	msleep(5);
#endif//PGH

#if 0
	rc = m5mo_sensor_init_probe(info);
	if (rc < 0)
		goto probe_done;
#endif
	s->s_init		= m5mo_sensor_init;
	s->s_release	= m5mo_sensor_release;
	s->s_config	= m5mo_sensor_config;

probe_done:
	CDBG("%s %s:%d\n", __FILE__, __func__, __LINE__);
	return rc;
	
}


static int m5mo_mem_write(u16 len, u32 addr, u8 *val)
{
	struct i2c_msg msg;
	unsigned char data[len + 8];
	int i, err = 0;

	if (!m5mo_client->adapter)
		return -ENODEV;

	msg.addr = m5mo_client->addr;
	msg.flags = 0;
	msg.len = sizeof(data);
	msg.buf = data;

	/* high byte goes out first */
	data[0] = 0x00;
	data[1] = 0x04;
	data[2] = (addr >> 24) & 0xFF;
	data[3] = (addr >> 16) & 0xFF;
	data[4] = (addr >> 8) & 0xFF;
	data[5] = addr & 0xFF;
	data[6] = (len >> 8) & 0xFF;
	data[7] = len & 0xFF;
	memcpy(data + 2 + sizeof(addr) + sizeof(len), val, len);

	for(i = M5MO_I2C_VERIFY_RETRY; i; i--) {
		err = i2c_transfer(m5mo_client->adapter, &msg, 1);
		if (err == 1)
			break;
		msleep(20);
	}

	return err;
}

static int m5mo_mem_read(u16 len, u32 addr, u8 *val)
{

	struct i2c_msg msg[1];
	unsigned char data[8];
	unsigned char recv_data[len + 3];
	int err;

	if (!m5mo_client->adapter)
		return -ENODEV;
	
	if (len < 1)
	{
		printk("Data Length to read is out of range !\n");
		return -EINVAL;
	}
	
	msg->addr = m5mo_client->addr;
	msg->flags = 0;
	msg->len = 8;
	msg->buf = data;

	/* high byte goes out first */
	data[0] = 0x00;
	data[1] = 0x03;
	data[2] = (u8) (addr >> 24);
	data[3] = (u8) ( (addr >> 16) & 0xff );
	data[4] = (u8) ( (addr >> 8) & 0xff );
	data[5] = (u8) (addr & 0xff);
	data[6] = (u8) (len >> 8);
	data[7] = (u8) (len & 0xff);
	
	err = i2c_transfer(m5mo_client->adapter, msg, 1);
	if (err >= 0) 
	{
		msg->flags = I2C_M_RD;
		msg->len = len + 3;
		msg->buf = recv_data; 

		err = i2c_transfer(m5mo_client->adapter, msg, 1);
	}
	if (err >= 0) 
	{
		memcpy(val, recv_data+3, len);
		return 0;
	}

	printk( "read from offset 0x%x error %d", addr, err);
	return err;
	
#if 0	// from Seine
	struct i2c_msg msg;
	unsigned char data[8];
	unsigned char recv_data[len + 3];
	int i, err = 0;
	u16 recv_len;

	if (len <= 0)
		return -EINVAL;	
		
	msg.addr = m5mo_client->addr;
	msg.flags = 0;
	msg.len = sizeof(data);
	msg.buf = data;

	/* high byte goes out first */
	data[0] = 0x00;
	data[1] = 0x03;
	data[2] = (addr >> 24) & 0xFF;
	data[3] = (addr >> 16) & 0xFF;
	data[4] = (addr >> 8) & 0xFF;
	data[5] = addr & 0xFF;
	data[6] = (len >> 8) & 0xFF;
	data[7] = len & 0xFF;

	for (i = M5MO_I2C_VERIFY_RETRY; i; i--) {
		err = i2c_transfer(m5mo_client->adapter, &msg, 1);
		if (err == 1)
			break;
		msleep(20);
	}

	if (err != 1)
		return err;

	msg.flags = I2C_M_RD;
	msg.len = sizeof(recv_data);
	msg.buf = recv_data;
	for(i = M5MO_I2C_VERIFY_RETRY; i; i--) {
		err = i2c_transfer(m5mo_client->adapter, &msg, 1);
		if (err == 1)
			break;
		msleep(20);
	}

	if (err != 1)
		return err;

	if (recv_len != (recv_data[0] << 8 | recv_data[1]))
		printk("expected length %d, but return length %d\n", 
				 len, recv_len);

	memcpy(val, recv_data + 1 + sizeof(recv_len), len);	

	return err;
#endif
}






static int 
m5mo_program_fw(u8* buf, u32 addr, u32 unit, u32 count)
{
	//u32 val;
	u16 val;
	u32 intram_unit = 0x1000;
	int i, j, retries, err = 0;

	CAM_DEBUG("-- start --");
	for (i = 0; i < count; i++) {
		/* Set Flash ROM memory address */
		//err = m5mo_writel(c, M5MO_CATEGORY_FLASH, M5MO_FLASH_ADDR, addr);
		CAM_DEBUG("set addr : 0x%x (index = %d)", addr,i);	
		err = m5mo_i2c_write_32bit(M5MO_CATEGORY_FLASH, M5MO_FLASH_ADDR, addr);
		CHECK_ERR(err);		

		/* Erase FLASH ROM entire memory */
		err = m5mo_i2c_write_8bit(M5MO_CATEGORY_FLASH, M5MO_FLASH_ERASE, 0x01);
		CHECK_ERR(err);
		
		CAM_DEBUG("erase Flash ROM");
		/* Response while sector-erase is operating */
		m5mo_i2c_verify(M5MO_CATEGORY_FLASH, M5MO_FLASH_ERASE, 0x00);

		/* Set FLASH ROM programming size */
		//err = m5mo_writew(c, M5MO_CATEGORY_FLASH, M5MO_FLASH_BYTE, unit == SZ_64K ? 0 : unit);
		err = m5mo_i2c_write_16bit(M5MO_CATEGORY_FLASH, M5MO_FLASH_BYTE, unit == SZ_64K ? 0 : unit);
		CHECK_ERR(err);
		msleep(10);
		/* Clear M-5MoLS internal RAM */
		err = m5mo_i2c_write_8bit(M5MO_CATEGORY_FLASH, M5MO_FLASH_RAM_CLEAR, 0x01);
		CHECK_ERR(err);
		msleep(10);
		/* Set Flash ROM programming address */
		//err = m5mo_writel(c, M5MO_CATEGORY_FLASH, M5MO_FLASH_ADDR, addr);
		err = m5mo_i2c_write_32bit(M5MO_CATEGORY_FLASH, M5MO_FLASH_ADDR, addr);
		CHECK_ERR(err);
	
		/* Send programmed firmware */
		for (j = 0; j < unit; j += intram_unit) {
			err = m5mo_mem_write(intram_unit, M5MO_INT_RAM_BASE_ADDR + j, buf + (i * unit) + j);
			//err = m5mo_i2c_write_memory(M5MO_INT_RAM_BASE_ADDR + j,intram_unit, buf + (i * unit) + j);
			CHECK_ERR(err);
			msleep(10);
		}

		/* Start Programming */
		err = m5mo_i2c_write_8bit(M5MO_CATEGORY_FLASH, M5MO_FLASH_WR, 0x01);
		CHECK_ERR(err);
		CAM_DEBUG("start programming");
		msleep(100);
		
		/* Confirm programming has been completed */
		m5mo_i2c_verify(M5MO_CATEGORY_FLASH, M5MO_FLASH_WR, 0x00);			
				
		/* Increase Flash ROM memory address */
		addr += unit;
	}

	CAM_DEBUG("-- end --");
	return 0;
}

static int m5mo_reset_for_update(void)
{
	CAM_DEBUG("");
	
	CAM_DEBUG(" 1. CAM_8M_RST = 0 ");
	gpio_set_value_cansleep(CAM_8M_RST, LOW);

	CAM_DEBUG(" 2. CAM_MEGA_EN = 0 ");
	if (system_rev >= 4) 
		gpio_set_value(CAM_MEGA_EN_REV04, LOW);
	else
		gpio_set_value(CAM_MEGA_EN, LOW);
	msleep(20);
	

	CAM_DEBUG(" 4. CAM_MEGA_EN = 1 ");
	if (system_rev >= 4) 
		gpio_set_value(CAM_MEGA_EN_REV04, HIGH);
	else
		gpio_set_value(CAM_MEGA_EN, HIGH);		
	msleep(1);

	CAM_DEBUG(" 5. CAM_8M_RST = 1 ");
	gpio_set_value_cansleep(CAM_8M_RST, HIGH);
	msleep(30); // min 350ns
}
static int m5mo_load_fw(int location)
{
	//struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct file *fp;
	mm_segment_t old_fs;
	u8 *buf, val;
	long fsize, nread;
	loff_t fpos = 0;
	int err;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	if(location) {	// read from SDCARD
		fp = filp_open(M5MO_FW_PATH_SDCARD, O_RDONLY, S_IRUSR);
		CAM_DEBUG("######## Read Firmware From SDCARD ########");
		if (IS_ERR(fp)) {	// if there is no firmware in SDCARD
			CAM_DEBUG("######## No Firmware in SDCARD ########");
			if(location == 2) {	// Auto select
				fp = filp_open(M5MO_FW_PATH_IMAGE, O_RDONLY, S_IRUSR);
				if(IS_ERR(fp)) {
					printk("failed to open %s\n", M5MO_FW_PATH_IMAGE);
					return -ENOENT;
				}
				CAM_DEBUG("######## Read Firmware From IMAGE (AutoSelect) ########");
			}
			else {
				printk("failed to open %s\n", M5MO_FW_PATH_SDCARD);
				return -ENOENT;
			}
		}
	}
	else		// read from IMG
	{
		fp = filp_open(M5MO_FW_PATH_IMAGE, O_RDONLY, S_IRUSR);		
	if (IS_ERR(fp)) {
			printk("failed to open %s\n", M5MO_FW_PATH_IMAGE);
		return -ENOENT;
	}
		CAM_DEBUG("######## Read Firmware From IMAGE ########");		
	}

	fsize = fp->f_path.dentry->d_inode->i_size;
	//CAM_DEBUG("start, file path %s, size %ld Bytes\n", M5MO_FW_PATH, fsize);
	
	buf = vmalloc(fsize);
	if (!buf) {
		printk("failed to allocate memory\n");
		return -ENOMEM;
	}

	nread = vfs_read(fp, (char __user *)buf, fsize, &fpos);
	if (nread != fsize) {
		printk("failed to read firmware file, nread %ld Bytes\n", nread);
		return -EIO;
	}	

		
	/* set pin */
	val = 0x7E;
	err = m5mo_mem_write(sizeof(val), 0x50000308, &val);
	CHECK_ERR(err);
	
	/* select flash memory */
	err = m5mo_i2c_write_8bit(0x0F, 0x13, 0x01);
	CHECK_ERR(err);
	/* program FLSH ROM */
	err = m5mo_program_fw(buf, M5MO_FLASH_BASE_ADDR, SZ_64K, 31);
	CHECK_ERR(err);
	err = m5mo_program_fw(buf, M5MO_FLASH_BASE_ADDR + SZ_64K * 31, SZ_8K, 4);
	CHECK_ERR(err);
	
	CAM_DEBUG("end\n");

	vfree(buf);

	filp_close(fp, current->files);
	set_fs(old_fs);

	return 0;
}


#if 0
static int msm_camera_remove(struct platform_device *pdev)
{
	return msm_camera_drv_remove(pdev);
}
#endif
static int __init msm_camera_probe(struct platform_device *pdev)
{
	printk("############# M5MO probe ##############\n");
	return msm_camera_drv_start(pdev, m5mo_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = msm_camera_probe,
//	.remove	 = msm_camera_remove,
	.driver = {
		.name = "msm_camera_m5mo",
		.owner = THIS_MODULE,
	},
};

static int __init msm_camera_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

static void __exit msm_camera_exit(void)
{
	platform_driver_unregister(&msm_camera_driver);
}

//chief.boot.temp module_init(msm_camera_init);
module_exit(msm_camera_exit);

