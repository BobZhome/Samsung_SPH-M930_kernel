
#include <linux/module.h>
#include <linux/completion.h>
#include <linux/spinlock.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <mach/parameters.h>
#include <asm/uaccess.h>

#define PARAM_READ 0
#define PARAM_WRITE 1

#define BOOT_PARAM_OFFSET 0
#define KERNEL_PARAM_OFFSET (3*1024*1024)

union param_union {
	struct BOOT_PARAM boot;
	struct samsung_parameter kernel;
};

struct param_work_data_type {
	struct work_struct work;
	int rw;
	int param_type;
	union param_union param_data;
	int result;
};

static struct workqueue_struct *param_workqueue;
struct param_work_data_type param_work_data;
static DEFINE_MUTEX(param_lock);
static DECLARE_COMPLETION(param_completion);

static int param_debug;
module_param_named(debug, param_debug, int, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(debug, "dump param data when reading param block.");

static void param_work_func(struct work_struct *work)
{
	struct param_work_data_type* workdata = container_of(work, struct param_work_data_type, work);
	struct file* filp;
	mm_segment_t fs = get_fs();
	loff_t pos = 0;
	ssize_t retsize;
	ssize_t size;
	int result = 0;

	pr_info("%s: rw=%d type=%d\n", __func__, workdata->rw, workdata->param_type);
	
	filp = filp_open("/dev/block/mmcblk0p14", O_RDWR, 0);
	
	if(IS_ERR(filp)) {
		result = PTR_ERR(filp);
		pr_err("%s: file open failed %d", __func__, result);
		goto out;
	}

	switch(workdata->param_type) {
		case PARAM_TYPE_BOOT:
			pos = BOOT_PARAM_OFFSET;
			size = sizeof(struct BOOT_PARAM);
			break;
		case PARAM_TYPE_KERNEL:
			pos = KERNEL_PARAM_OFFSET;
			size = sizeof(struct samsung_parameter);
			break;
		default:
			pr_err("%s: param_type invalid %d", __func__, workdata->param_type);
			result = -EINVAL;
			goto file_close_out;
			break;
	}
	
	if(workdata->rw == PARAM_READ) {
		/* 
		 * param_data is a union so it doesn't matter if we use .boot or 
		 * .kernel 
		 */
		retsize = kernel_read(filp, pos, (char __user *)&workdata->param_data.boot, size);
	} else if(workdata->rw == PARAM_WRITE) {
		set_fs(get_ds());
		retsize = vfs_write(filp, (char __user *)&workdata->param_data.boot, size, &pos);
		set_fs(fs);
	}

	if(retsize != size) {
		pr_err("%s: return size(%d) does not match request size(%d)", __func__, retsize, size);
		result = -EAGAIN;
	}
	
file_close_out:
	filp_close(filp, NULL);
out:
	workdata->result = result;
	pr_info("%s: complete!\n", __func__);
	/* we should always call complete, otherwise the caller will be deadlocked */
	complete(&param_completion);
}

static int param_read_write(int rw, int type, void *param_data, int param_size)
{
	int result;
	
	/* sanity check */
	if(type == PARAM_TYPE_BOOT && param_size != sizeof(struct BOOT_PARAM)) {
		pr_err("%s: invalid param size %d for boot param\n", __func__, param_size);
		return -EINVAL;
	}
	if (type == PARAM_TYPE_KERNEL && param_size != sizeof(struct samsung_parameter)) {
		pr_err("%s: invalid param size %d for kernel param\n", __func__, param_size);
		return -EINVAL;
	}

	/* need to acquire lock to serialize the param operations */
	mutex_lock(&param_lock);
	
	param_work_data.rw = rw;
	param_work_data.param_type = type;
	if(rw == PARAM_WRITE) {
		/* we did a size check so we can safely copy (param_size) of data */
		memcpy(&param_work_data.param_data.boot, param_data, param_size);
	}
	result = queue_work(param_workqueue, &param_work_data.work);
	if(result == 0) {
		/* work is already in queue. this should not happen */
		pr_err("%s: this should not happen", __func__);
	}

	/* for synchronous param operations, wait until the param work is complete */
	wait_for_completion(&param_completion);

	if(rw == PARAM_READ) {
		memcpy(param_data, &param_work_data.param_data.boot, param_size);
		if(param_debug) {
			pr_info("=========================================================\n");
			pr_info(" %s \n", (type==PARAM_TYPE_BOOT)?"BOOT PARAM":"KERNEL PARAM");
			pr_info("=========================================================\n");
			print_hex_dump(KERN_DEBUG, "", DUMP_PREFIX_OFFSET, 16, 1,
		       param_data, param_size, false);
			pr_info("=========================================================\n");
		}
	}
	
	result = param_work_data.result;
	mutex_unlock(&param_lock);

	return result;
}


int param_read(int type, void *param_data, int param_size)
{
	return param_read_write(PARAM_READ, type, param_data, param_size);
}
EXPORT_SYMBOL(param_read);

int param_write(int type, void *param_data, int param_size)
{
	return param_read_write(PARAM_WRITE, type, param_data, param_size);
}
EXPORT_SYMBOL(param_write);

int param_update(int type, int offset, int size)
{
	//read
	//update
	//write
	return 0;
}
EXPORT_SYMBOL(param_update);

static int __init param_init(void)
{	
	pr_info("%s: init", __func__);

#ifndef PRODUCT_SHIP
	param_debug = 1;
#endif
	param_workqueue = create_singlethread_workqueue("param_workqueue");
	if (!param_workqueue)
		return -ENOMEM;
	
	INIT_WORK(&param_work_data.work, param_work_func);

	return 0;
}

static void __exit param_exit(void)
{
	pr_info("%s: exit", __func__);
}

module_init(param_init);
module_exit(param_exit);

