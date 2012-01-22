
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <asm/uaccess.h>

#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
#endif	/* CONFIG_PROC_FS */
#include "dpram.h"
#include "../../arch/arm/mach-msm/smd_private.h"
#include "../../arch/arm/mach-msm/proc_comm.h"
#include <mach/parameters.h>

static struct extcmd_dev extcmd_dev;

#define DPRAM_EXTENDED_CMD_FOTA_UNIT_TEST //test

#ifdef DPRAM_EXTENDED_CMD_FOTA_UNIT_TEST
static struct proc_dir_entry *extcmd_fota_debug_proc_entry;
extern struct proc_dir_entry proc_root;
#endif

#define EXTCMD_SUCCESS 1

extern int smem_do_alloc(unsigned id, unsigned size_in);
extern int emmc_boot_param_read(struct BOOT_PARAM *param_data);
extern int emmc_boot_param_write(struct BOOT_PARAM *param_data);

static int cmd_0_dummy(unsigned int subcmd, int *args, void* data);
static int cmd_fota_delta_write(unsigned int subcmd, int *args, void* data);
static int cmd_fota_update_result(unsigned int subcmd, int *args, void* data);


///////////////////////////////////////////////////////////////////////////////
// Add your commands here
// Command number and array index should match!!
//
static struct extcmd_rx_handler_data extcmd_rx_handlers[] = {
	{0, cmd_0_dummy},
	{1, cmd_fota_delta_write},
	{2, cmd_fota_update_result},
};
//
///////////////////////////////////////////////////////////////////////////////

static int cmd_0_dummy(unsigned int subcmd, int *args, void* data)
{
	pr_info("%s", __func__);

	return 1;
}

static int cmd_fota_delta_write(unsigned int subcmd, int *args, void* data)
{
	struct file* filp;
	loff_t pos = (loff_t)args[0];
	ssize_t size = (ssize_t)args[1];
	ssize_t writtensize = 0;
	int is_last = args[2];
	mm_segment_t fs = get_fs();
	void *pbuf;
	
	pr_info("%s: offset=%lld size=%d is_last=%d data=0x%08x\n", __func__, pos, size, is_last, (unsigned int)data);
	
	filp = filp_open("/dev/block/mmcblk0p22", O_RDWR, 0);
	
	if(IS_ERR(filp)) {
		pr_err("%s: file open failed!!\n", __func__);
		return -1;
	} else {
		pbuf = kmalloc(size, GFP_KERNEL);
		if(!pbuf) {
			pr_err("%s: ERROR: alloc failed", __func__);
		} else {
			memcpy(pbuf, data, size);
			set_fs(get_ds());
			writtensize = vfs_write(filp, (char __user *)pbuf, size, &pos);
			set_fs(fs);
			filp_close(filp, NULL);
			kfree(pbuf);
		}
	}

	if(writtensize != size) {
		// file not written. return fail...
		pr_err("%s: write failed!! written size=%d requested size=%d\n", __func__, writtensize, size);
		return -2;
	}
	
	if(is_last == 1) {
		struct BOOT_PARAM param_data;
		pr_info("%s: last packet received. write to param block\n", __func__);
		if(emmc_boot_param_read(&param_data) == 0) {
			param_data.fota_mode = 5;
			emmc_boot_param_write(&param_data);
			#ifdef DPRAM_EXTENDED_CMD_FOTA_UNIT_TEST
			emmc_boot_param_read(&param_data);
			#endif
		} else
			pr_info("%s: emmc_boot_param_read failed\n", __func__);
	}

	// success
	return EXTCMD_SUCCESS;
	
}

static int cmd_fota_update_result(unsigned int subcmd, int *args, void* data)
{
	struct BOOT_PARAM param_data;
	if(emmc_boot_param_read(&param_data) == 0) {
		param_data.update = 0;
		emmc_boot_param_write(&param_data);
	} else
		pr_info("%s: emmc_boot_param_read failed\n", __func__);

	return EXTCMD_SUCCESS;
}

static void extcmd_rx_work(struct work_struct *work)
{
	struct extcmd_dev * dev = container_of(work, struct extcmd_dev, rx_work);
	int ret;
	
	pr_info("%s: cmd=%d status=%d\n", __func__, dev->extcmd->hdr.cmd, dev->extcmd->hdr.status);

	// acquire mutext
	
	if(dev->extcmd->hdr.cmd >= dev->num_rx_handlers) {
		pr_err("%s: ERROR! invalid command %d\n", __func__, dev->extcmd->hdr.cmd);
		// force it to command 0 and continue so that CP doesn't get locked up 
		//waiting for the status to be non-zero.
		dev->extcmd->hdr.cmd = 0;
	}
	
	pr_info("%s calling %pS\n", __func__, dev->rx_handler_data[dev->extcmd->hdr.cmd].handler);
	ret = dev->rx_handler_data[dev->extcmd->hdr.cmd].handler(dev->extcmd->hdr.subcmd, (int*)dev->extcmd->hdr.args, (int*)dev->extcmd->data);
	if(ret == 0) {
		pr_info("%s: handler return 0... Force it to EXTCMD_SUCCESS\n", __func__);
		ret = EXTCMD_SUCCESS;
	}

	// notify CP that we are done.
	dev->extcmd->hdr.status = ret;

	dsb();

	// release mutex
}
static void extcmd_tx_work(struct work_struct *work)
{
	struct extcmd_dev * dev = container_of(work, struct extcmd_dev, rx_work);
	
	pr_info("%s\n", __func__);

	dev;
	// acquire mutex
	
	// get extcmd smem semaphore

	// fill hdr

	// send dpram command

	// wait for status to become non-zero
	
	// release mutex
}

static volatile struct extcmd * get_smem_extcmd(void)
{
	if(!extcmd_dev.extcmd) {
		pr_info("DPRAM: allocating extcmd in smem\n");
		extcmd_dev.extcmd = (volatile struct extcmd *)(smem_alloc(SMEM_ID_VENDOR2, sizeof(struct extcmd)));
		if(!extcmd_dev.extcmd) {
			pr_err("DPRAM: allocating extcmd in smem FAILED!!\n");
			return 0;
		}
	}

	return extcmd_dev.extcmd;
}

int send_extcmd(unsigned int cmd, unsigned int subcmd, int* args)
{
	pr_info("%s: cmd=%u, subcmd=%u, args[0]=%d args[1]=%d\n", __func__, cmd, subcmd, args[0], args[1]);
	return queue_work(extcmd_dev.wq, &extcmd_dev.tx_work);
}
int cmd_extcmd_handler(void)
{
	struct extcmd * pextcmd;
	
	if((pextcmd = get_smem_extcmd()) == NULL)
		return -1;
	
	return queue_work(extcmd_dev.wq, &extcmd_dev.rx_work);
}

#ifdef DPRAM_EXTENDED_CMD_FOTA_UNIT_TEST
static int __debug_fota_delta_read(void *buf, ssize_t size)
{
	struct file* filp;
	ssize_t readsize = 0;

	pr_info("%s: buf=0x%08x size=%d\n", __func__, buf, size);
	

	filp = filp_open("/dev/block/mmcblk0p22", O_RDWR, 0);

	if(filp) {
		readsize = kernel_read(filp, 0, (char __user *)buf, size);
		pr_info("%s: readsize=%d", __func__, readsize);
		filp_close(filp, NULL);
	} else {
		pr_err("%s: file open failed!!\n", __func__);
	}

	return 0;
}

static int extcmd_fota_debug_proc_write(struct file *file, 
					 const char *buf, unsigned long count, void *data)

{
	int args[5] = {0,};
	int *writedata;
	int i;
    int datasize = simple_strtoul(buf, NULL, 10);
	int ret = 0;
	
	pr_info("%s: requested write size : %d", __func__, datasize);

	if(datasize == 0) {
		// do a command handler test
		volatile struct extcmd *pextcmd;

		pr_info("%s: command handler test start...\n", __func__);
		if((pextcmd = get_smem_extcmd()) != NULL) {
			pextcmd->hdr.cmd = 1;
			pextcmd->hdr.subcmd = 0;
			pextcmd->hdr.args[0] = 0;
			pextcmd->hdr.args[1] = 16;
			pextcmd->hdr.args[2] = 1;

			for(i=0;i<16;i++) {
				pextcmd->data[i] = i;
			}

			pextcmd->hdr.status = 0;

			cmd_extcmd_handler();

			pr_info("%s: check for status to be non-zero. status=%d\n", __func__, pextcmd->hdr.status);

			while(!pextcmd->hdr.status) {
				usleep(100);
			}
			
			pr_err("%s: status=%d \n", __func__, pextcmd->hdr.status);
		}

		pr_info("%s: command handler test done...\n", __func__);
		return count;
	}
	
	writedata = kmalloc(datasize, GFP_KERNEL);
	
	for(i=0;i<datasize/4;i++) {
		// fill with a dummy data
		writedata[i] = i;
	}

	if(datasize < 1024) {
		// do a stress test write
		for(i=0;i<datasize;i+=4) {			
			args[0] = i; //offset
			args[1] = 4; //size
			if(i==datasize-4)
				args[2] = 1; //islast
			else
				args[2] = 0; //islast
			ret = cmd_fota_delta_write(0, args, writedata+(i>>2));
			if(ret < 0) pr_err("%s: cmd_fota_delta_write returned with error : %d\n", __func__, ret);
		}
	} else {
		args[0] = 0; //offset
		args[1] = datasize; //size
		args[2] = 1;
		ret = cmd_fota_delta_write(0, args, writedata);
	}
	
	pr_info("%s: last cmd_fota_delta_write returned : %d\n", __func__, ret);
	
	return count;

}

#include <linux/vmalloc.h>
static int extcmd_fota_debug_proc_read(char *page, char **start,
				off_t offset, int count, int *eof, void *data)
{
	char *readbuf;
	char *buf = page;
	int ret;
	int i;
	
	readbuf = vmalloc(32*1024);

	if(!readbuf) {
		pr_err("%s: malloc failed!!!\n", __func__);
	}
	
	ret = __debug_fota_delta_read(readbuf, 32*1024);

	pr_info("%s: ret=%d", __func__, ret);

	if(ret == 0) {
		// just print some of the data
		for(i=1;i<=1024;i+=8) {
			pr_info("%02x %02x %02x %02x %02x %02x %02x %02x \n",
				readbuf[i-1], readbuf[i], readbuf[i+1], readbuf[i+2],
				readbuf[i+3], readbuf[i+4], readbuf[i+5], readbuf[i+6]);
		}
	}

	buf += sprintf(buf,"%02x %02x %02x %02x %02x %02x %02x %02x \n",
			readbuf[0], readbuf[1], readbuf[2], readbuf[3],
			readbuf[4], readbuf[5], readbuf[6], readbuf[7]);

	return buf - page < count ? buf - page : count;
}

#endif /* DPRAM_EXTENDED_CMD_FOTA_UNIT_TEST */



int dpram_register_extcmd_dev(void)
{
	extcmd_dev.extcmd = (volatile struct extcmd *)(smem_do_alloc(SMEM_ID_VENDOR2, sizeof(struct extcmd)));
	if(!extcmd_dev.extcmd) {
		pr_err("DPRAM: Extended cmd alloc failed!\n");
	}
	pr_info("DPRAM: extcmd address = 0x%08x\n", (unsigned int)extcmd_dev.extcmd);
	extcmd_dev.rx_handler_data = extcmd_rx_handlers;
	extcmd_dev.num_rx_handlers = ARRAY_SIZE(extcmd_rx_handlers);
	extcmd_dev.wq = create_singlethread_workqueue("dpram_extcmd");
	INIT_WORK(&(extcmd_dev.rx_work), extcmd_rx_work);
	INIT_WORK(&(extcmd_dev.tx_work), extcmd_tx_work);

#ifdef DPRAM_EXTENDED_CMD_FOTA_UNIT_TEST
	
	extcmd_fota_debug_proc_entry = create_proc_entry("dpram_fota_debug",
					       S_IRUSR|S_IWUSR,&proc_root);
	if (extcmd_fota_debug_proc_entry) {
		extcmd_fota_debug_proc_entry->write_proc = extcmd_fota_debug_proc_write;
		extcmd_fota_debug_proc_entry->read_proc = extcmd_fota_debug_proc_read;
		extcmd_fota_debug_proc_entry->data = NULL;
	} else
		return -ENOMEM;
#endif

	return 0;
}
