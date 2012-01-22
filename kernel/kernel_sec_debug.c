/* kernel_sec_debug.c
 *
 * Exception handling in kernel by SEC
 *
 * Copyright (c) 2010 Samsung Electronics
 *                http://www.samsung.com/
 */

#ifdef CONFIG_KERNEL_DEBUG_SEC

#include <linux/kernel_sec_common.h>
#include <asm/cacheflush.h>           // cacheflush
#include <asm/io.h>
#include <linux/errno.h>
#include <linux/ctype.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#include <linux/file.h>
#include <mach/hardware.h>

#include <mach/msm_iomap.h>

#include "../arch/arm/mach-msm/smd_private.h"
#include "../arch/arm/mach-msm/proc_comm.h"

extern void dump_all_task_info();
extern void dump_cpu_stat();

struct smem_hw_reset_detect
{
	unsigned int magic1;
	unsigned int magic2;
};

/********************************
 *  Variable
 *********************************/

// klaatu
//sched_log_t gExcpTaskLog[SCHED_LOG_MAX];
//unsigned int gExcpTaskLogIdx = 0;

char gkernel_sec_build_info[100];

__used t_kernel_sec_arm_core_regsiters kernel_sec_core_reg_dump;
__used t_kernel_sec_mmu_info           kernel_sec_mmu_reg_dump;
__used kernel_sec_upload_cause_type     gkernel_sec_upload_cause;


const char* gkernel_sec_build_info_date_time[] =
{
	__DATE__,
	__TIME__   
};

#ifdef CONFIG_SEC_KERNEL_FUNC_TRACE
struct kernel_trace_data {
	void* func;
	int line;
	
};

struct kfunc_trace_data {
	void* func;
	unsigned int timestamp;
	const char* msg;
	unsigned int data1;
	unsigned int data2;
	unsigned int data3;
};

#define KFUNC_TRACE_SIZE 8192
struct kfunc_trace {
	int index;
	struct kfunc_trace_data trace[KFUNC_TRACE_SIZE];
};
struct kfunc_trace kfunc_trace;
void kernel_func_trace(const char* msg, unsigned int data1, unsigned int data2, unsigned int data3)
{
	unsigned long flags;
	
	raw_local_irq_save(flags);
	kfunc_trace.trace[kfunc_trace.index].func = (void*)__builtin_return_address(0);
	kfunc_trace.trace[kfunc_trace.index].msg = msg;
	kfunc_trace.trace[kfunc_trace.index].data1 = data1;
	kfunc_trace.trace[kfunc_trace.index].data2 = data2;
	kfunc_trace.trace[kfunc_trace.index].data3 = data3;
	if(++kfunc_trace.index>=KFUNC_TRACE_SIZE)
		kfunc_trace.index = 0;
	raw_local_irq_restore(flags);
}
EXPORT_SYMBOL(kernel_func_trace);
#else
void kernel_func_trace(const char* msg, int data)
{
}
EXPORT_SYMBOL(kernel_func_trace);
#endif

/********************************
 *  Function
 *********************************/

void kernel_sec_set_build_info(void)
{
	char * p = gkernel_sec_build_info;
	sprintf(p,"BUILD_INFO: HWREV: %x",system_rev);
	strcat(p, " Date:");
	strcat(p, gkernel_sec_build_info_date_time[0]);
	strcat(p, " Time:");
	strcat(p, gkernel_sec_build_info_date_time[1]);
}
EXPORT_SYMBOL(kernel_sec_set_build_info);


	
void kernel_sec_set_upload_cause(kernel_sec_upload_cause_type uploadType)
{
	printk(KERN_EMERG"(kernel_sec_set_upload_cause) : upload_cause set %x\n",uploadType);	
	gkernel_sec_upload_cause=uploadType;	//it save the cause and it should be sent to MSM
}
EXPORT_SYMBOL(kernel_sec_set_upload_cause);



/* core reg dump function*/
void kernel_sec_get_core_reg_dump(t_kernel_sec_arm_core_regsiters* regs)
{
	asm(
		// we will be in SVC mode when we enter this function. Collect SVC registers along with cmn registers.
		"str r0, [%0,#0] \n\t"		// R0
		"str r1, [%0,#4] \n\t"		// R1
		"str r2, [%0,#8] \n\t"		// R2
		"str r3, [%0,#12] \n\t"		// R3
		"str r4, [%0,#16] \n\t"		// R4
		"str r5, [%0,#20] \n\t"		// R5
		"str r6, [%0,#24] \n\t"		// R6
		"str r7, [%0,#28] \n\t"		// R7
		"str r8, [%0,#32] \n\t"		// R8
		"str r9, [%0,#36] \n\t"		// R9
		"str r10, [%0,#40] \n\t"	// R10
		"str r11, [%0,#44] \n\t"	// R11
		"str r12, [%0,#48] \n\t"	// R12

		/* SVC */
		"str r13, [%0,#52] \n\t"	// R13_SVC
		"str r14, [%0,#56] \n\t"	// R14_SVC
		"mrs r1, spsr \n\t"			// SPSR_SVC
		"str r1, [%0,#60] \n\t"

		/* PC and CPSR */
		"sub r1, r15, #0x4 \n\t"	// PC
		"str r1, [%0,#64] \n\t"	
		"mrs r1, cpsr \n\t"			// CPSR
		"str r1, [%0,#68] \n\t"

		/* SYS/USR */
		"mrs r1, cpsr \n\t"			// switch to SYS mode
		"and r1, r1, #0xFFFFFFE0\n\t"
		"orr r1, r1, #0x1f \n\t"
		"msr cpsr,r1 \n\t"

		"str r13, [%0,#72] \n\t"	// R13_USR
		"str r14, [%0,#76] \n\t"	// R13_USR

		/*FIQ*/
		"mrs r1, cpsr \n\t"			// switch to FIQ mode
		"and r1,r1,#0xFFFFFFE0\n\t"
		"orr r1,r1,#0x11\n\t"
		"msr cpsr,r1 \n\t"

		"str r8, [%0,#80] \n\t"		// R8_FIQ
		"str r9, [%0,#84] \n\t"		// R9_FIQ
		"str r10, [%0,#88] \n\t"	// R10_FIQ
		"str r11, [%0,#92] \n\t"	// R11_FIQ
		"str r12, [%0,#96] \n\t"	// R12_FIQ
		"str r13, [%0,#100] \n\t"	// R13_FIQ
		"str r14, [%0,#104] \n\t"	// R14_FIQ
		"mrs r1, spsr \n\t"			// SPSR_FIQ
		"str r1, [%0,#108] \n\t"

		/*IRQ*/
		"mrs r1, cpsr \n\t"			// switch to IRQ mode
		"and r1, r1, #0xFFFFFFE0\n\t"
		"orr r1, r1, #0x12\n\t"
		"msr cpsr,r1 \n\t"

		"str r13, [%0,#112] \n\t"	// R13_IRQ
		"str r14, [%0,#116] \n\t"	// R14_IRQ
		"mrs r1, spsr \n\t"			// SPSR_IRQ
		"str r1, [%0,#120] \n\t"

		/*MON*/
		"mrs r1, cpsr \n\t"			// switch to monitor mode
		"and r1, r1, #0xFFFFFFE0\n\t"
		"orr r1, r1, #0x16\n\t"
		"msr cpsr,r1 \n\t"

		"str r13, [%0,#124] \n\t"	// R13_MON
		"str r14, [%0,#128] \n\t"	// R14_MON
		"mrs r1, spsr \n\t"			// SPSR_MON
		"str r1, [%0,#132] \n\t"

		/*ABT*/
		"mrs r1, cpsr \n\t"			// switch to Abort mode
		"and r1, r1, #0xFFFFFFE0\n\t"
		"orr r1, r1, #0x17\n\t"
		"msr cpsr,r1 \n\t"

		"str r13, [%0,#136] \n\t"	// R13_ABT
		"str r14, [%0,#140] \n\t"	// R14_ABT
		"mrs r1, spsr \n\t"			// SPSR_ABT
		"str r1, [%0,#144] \n\t"

		/*UND*/
		"mrs r1, cpsr \n\t"			// switch to undef mode
		"and r1, r1, #0xFFFFFFE0\n\t"
		"orr r1, r1, #0x1B\n\t"
		"msr cpsr,r1 \n\t"

		"str r13, [%0,#148] \n\t"	// R13_UND
		"str r14, [%0,#152] \n\t"	// R14_UND
		"mrs r1, spsr \n\t"			// SPSR_UND
		"str r1, [%0,#156] \n\t"

		/* restore to SVC mode */
		"mrs r1, cpsr \n\t"			// switch to undef mode
		"and r1, r1, #0xFFFFFFE0\n\t"
		"orr r1, r1, #0x13\n\t"
		"msr cpsr,r1 \n\t"
		
		:				/* output */
        :"r"(regs)    	/* input */
        :"%r1"     		/* clobbered register */
        );

	return;	
}
EXPORT_SYMBOL(kernel_sec_get_core_reg_dump);



int kernel_sec_get_mmu_reg_dump(t_kernel_sec_mmu_info *mmu_info)
{
	asm("mrc    p15, 0, r1, c1, c0, 0 \n\t"	//SCTLR
		"str r1, [%0] \n\t"
		"mrc    p15, 0, r1, c2, c0, 0 \n\t"	//TTBR0
		"str r1, [%0,#4] \n\t"
		"mrc    p15, 0, r1, c2, c0,1 \n\t"	//TTBR1
		"str r1, [%0,#8] \n\t"
		"mrc    p15, 0, r1, c2, c0,2 \n\t"	//TTBCR
		"str r1, [%0,#12] \n\t"
		"mrc    p15, 0, r1, c3, c0,0 \n\t"	//DACR
		"str r1, [%0,#16] \n\t"
		"mrc    p15, 0, r1, c5, c0,0 \n\t"	//DFSR
		"str r1, [%0,#20] \n\t"
		"mrc    p15, 0, r1, c6, c0,0 \n\t"	//DFAR
		"str r1, [%0,#24] \n\t"
		"mrc    p15, 0, r1, c5, c0,1 \n\t"	//IFSR
		"str r1, [%0,#28] \n\t"
		"mrc    p15, 0, r1, c6, c0,2 \n\t"	//IFAR
		"str r1, [%0,#32] \n\t"
		/*Dont populate DAFSR and RAFSR*/
		"mrc    p15, 0, r1, c10, c2,0 \n\t"	//PMRRR
		"str r1, [%0,#44] \n\t"
		"mrc    p15, 0, r1, c10, c2,1 \n\t"	//NMRRR
		"str r1, [%0,#48] \n\t"
		"mrc    p15, 0, r1, c13, c0,0 \n\t"	//FCSEPID
		"str r1, [%0,#52] \n\t"
		"mrc    p15, 0, r1, c13, c0,1 \n\t"	//CONTEXT
		"str r1, [%0,#56] \n\t"
		"mrc    p15, 0, r1, c13, c0,2 \n\t"	//URWTPID
		"str r1, [%0,#60] \n\t"
		"mrc    p15, 0, r1, c13, c0,3 \n\t"	//UROTPID
		"str r1, [%0,#64] \n\t"
		"mrc    p15, 0, r1, c13, c0,4 \n\t"	//POTPIDR
		"str r1, [%0,#68] \n\t"
		:					/* output */
        :"r"(mmu_info)    /* input */
        :"%r1","memory"         /* clobbered register */
        ); 
	return 0;
}
EXPORT_SYMBOL(kernel_sec_get_mmu_reg_dump);



void kernel_sec_save_final_context(void)
{
	if(kernel_sec_get_mmu_reg_dump(&kernel_sec_mmu_reg_dump) < 0)
	{
		printk(KERN_EMERG"(kernel_sec_save_final_context) kernel_sec_get_mmu_reg_dump fail.\n");
	}
	kernel_sec_get_core_reg_dump(&kernel_sec_core_reg_dump);

	printk(KERN_EMERG "(kernel_sec_save_final_context) Final context was saved before the system reset.\n");
}
EXPORT_SYMBOL(kernel_sec_save_final_context);

/* 
 *    bSilentReset
 *    TRUE  : Silent reset - clear the magic code.
 *    FALSE : Reset to upload mode - not clear the magic code.
 *    It's meaningful only ap master model. 
 */
#define RAMDUMP_MAGIC_NUM   	0xCCCC	// RAMDUMP MODE
void kernel_sec_reset(bool bSilentReset)
{
	struct smem_hw_reset_detect *smem;
       samsung_vendor1_id* smem_vendor1 = NULL;
       
	printk(KERN_EMERG "(kernel_sec_reset) %s\n", gkernel_sec_build_info);
	if(gkernel_sec_upload_cause == UPLOAD_CAUSE_KERNEL_PANIC)
	printk(KERN_EMERG "(kernel_sec_reset) Kernel panic. The system will be reset !!\n");
	if(gkernel_sec_upload_cause == UPLOAD_CAUSE_FORCED_UPLOAD)
	printk(KERN_EMERG "(kernel_sec_reset) user has forced a reset. The system will be reset !!\n");

	// flush cache back to ram 
	flush_cache_all();

	smem = smem_find(SMEM_HW_RESET_DETECT, sizeof(struct smem_hw_reset_detect));

	if (smem) {
		smem->magic1 = gkernel_sec_upload_cause;
		smem->magic2 = gkernel_sec_upload_cause;
	}		
        writel(RAMDUMP_MAGIC_NUM, MSM_SHARED_RAM_BASE + 0x30); //proc_comm[3].command
	writel(gkernel_sec_upload_cause, MSM_SHARED_RAM_BASE + 0x38); //proc_comm[3].data1

//	smsm_reset_modem(SMSM_SYSTEM_DOWNLOAD);
//	msleep(500);

       //copy mmu data
       smem_vendor1 = (samsung_vendor1_id *)smem_alloc(SMEM_ID_VENDOR1, sizeof(samsung_vendor1_id));
       memcpy(&(smem_vendor1->apps_dump.apps_regs),&kernel_sec_mmu_reg_dump,sizeof(kernel_sec_mmu_reg_dump));

       if(gkernel_sec_upload_cause == UPLOAD_CAUSE_FORCED_UPLOAD ){
          memcpy(&(smem_vendor1->apps_dump.apps_string),"USER_FORCED_UPLOAD",sizeof("USER_FORCED_UPLOAD"));
	}
	#if 0
     	dump_all_task_info();
	dump_cpu_stat();
	#endif
	
      msm_proc_comm_reset_modem_now();
	
       // the master cp or watchdog should reboot us  
	while(1);	
}
EXPORT_SYMBOL(kernel_sec_reset);


#define DEBUG_PANIC_TEST
#if defined(CONFIG_DEBUG_FS) && defined(DEBUG_PANIC_TEST)
#include <linux/debugfs.h>

static struct dentry *debug_panic_dent;
static spinlock_t debug_panic_spinlock;

struct debug_panic_type {
	void (*func)(void);
	char *desc;
};


void debug_panic_dabort(void)
{
	// data abort
	int *p = 0;
	*p = 0;
}

void debug_panic_pabort(void)
{
	// prefetch abort
	void (*p)(void) = 0;
	p();
}
void debug_panic_lockup(void)
{
	unsigned long flags;
	spin_lock_irqsave(&debug_panic_spinlock, flags);
	while(1);
	spin_unlock_irqrestore(&debug_panic_spinlock, flags);
}

void debug_panic_spinlock_bug(void)
{
	spin_lock(&debug_panic_spinlock);
	spin_lock(&debug_panic_spinlock);

	spin_unlock(&debug_panic_spinlock);
}

void debug_panic_sched_while_atomic(void)
{
	spin_lock(&debug_panic_spinlock);
	msleep(1);
	spin_unlock(&debug_panic_spinlock);
}

void debug_panic_sched_with_irqoff(void)
{
	unsigned long flags;
	
	raw_local_irq_save(flags);
	msleep(1);
}

struct debug_panic_type debug_panic_scenario[] = {
	[0] = {
		.func = debug_panic_dabort,
		.desc = "data abort\n"
	},
	[1] = {
		.func = debug_panic_pabort,
		.desc = "data abort\n"
	},
	[2] = {
		.func = debug_panic_lockup,
		.desc = "lockup\n"
	},
	[3] = {
		.func = debug_panic_spinlock_bug,
		.desc = "spinlock bug\n"
	},
	[4] = {
		.func = debug_panic_sched_while_atomic,
		.desc = "schedule while atomic\n"
	},
	[5] = {
		.func = debug_panic_sched_with_irqoff,
		.desc = "schedule with irq disabled\n"
	},
	
};

static int debug_panic_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t debug_panic_read(struct file *file, char __user *buf,
			  size_t count, loff_t *ppos)
{
	struct debug_panic_type *panic_type = file->private_data;
	ssize_t rc;
	extern int oops_in_progress;

	rc = simple_read_from_buffer((void __user *) buf, count,
				     ppos, (void *) panic_type->desc, strlen(panic_type->desc));

	return rc;
}
static ssize_t debug_panic_write(struct file *file, const char __user *buf,
			   size_t count, loff_t *ppos)
{
	struct debug_panic_type *panic_type = file->private_data;

	pr_info("@@ %s %s\n", __func__, panic_type->desc);
	msleep(500);

	panic_type->func();

	return count;
}

static const struct file_operations debug_panic_ops = {
	.open =         debug_panic_open,
	.read =         debug_panic_read,
	.write =        debug_panic_write,
};

#define DEBUG_MAX_FNAME 16
void debug_panic_init(void)
{
	int i;
	char name[DEBUG_MAX_FNAME];

	spin_lock_init(&debug_panic_spinlock);

	debug_panic_dent = debugfs_create_dir("panic", NULL);
	if (IS_ERR(debug_panic_dent)) {
		pr_err("panic debugfs_create_dir fail, error %ld\n",
		       PTR_ERR(debug_panic_dent));
		return;
	}

	for (i = 0; i < ARRAY_SIZE(debug_panic_scenario); i++) {
		snprintf(name, DEBUG_MAX_FNAME-1, "panic-%d", i);
		if (debugfs_create_file(name, 0644, debug_panic_dent,
					&debug_panic_scenario[i], &debug_panic_ops) == NULL) {
			pr_err("pmic8058 debugfs_create_file %s failed\n",
			       name);
		}
	}
}

static void debug_panic_exit(void)
{
	debugfs_remove_recursive(debug_panic_dent);
}

#else
static void debug_panic_init(void) { }
static void debug_panic_exit(void) { }
#endif

#if 1 //sleep.debug
#include "../arch/arm/mach-msm/proc_comm.h"
static int sleep_debug_enabled;
static struct proc_dir_entry *sleep_debug_proc_entry;
#if defined(CONFIG_DEBUG_FS)
extern int sleep_debug_get_smem_log(char *buf, int size);
#endif

int is_sleep_debug_enabled(void)
{
	return (sleep_debug_enabled==0x1ed);
}

int sleep_debug_enable(int enable)
{
	int en = enable;
	if(enable) {
		msm_proc_comm(PCOM_OEM_SANSUNG_SLEEP_DEBUG_CMD, &en, NULL);
		sleep_debug_enabled = 0x1ed;
		pr_info("Sleep debug enabled!");
	} else {
		msm_proc_comm(PCOM_OEM_SANSUNG_SLEEP_DEBUG_CMD, &en, NULL);
		sleep_debug_enabled = 0;
		pr_info("Sleep debug disabled!");
	}
}

static int sleep_debug_proc_read(char *page,
				char **start,
				off_t offset, int count, int *eof, void *data)
{
	char *buf = page;
	
	buf += sprintf(buf,"LED Debug is %s\n", (is_sleep_debug_enabled()==1)?"Enabled":"Disabled");

#if defined(CONFIG_DEBUG_FS)
	do {
		void *smemlog = kmalloc(count, GFP_KERNEL);
		int ret = 0;
		if(buf) {
			ret = sleep_debug_get_smem_log(smemlog, count);
			buf += sprintf(buf,"SMEM LOG \n%s", smemlog);
		}
	} while(0);
#endif

	return buf - page < count ? buf - page : count;
}

static int sleep_debug_proc_write(struct file *file, const char *buf,
					 unsigned long count, void *data)
{
	unsigned int enable;
	if(strncmp("enable", buf, 6) == 0 || buf[0]=='1')
		sleep_debug_enable(1);
	else
		sleep_debug_enable(0);
	
	return count;
}

void sleep_debug_init(void)
{
	pr_info("%s\n", __func__);
	
	sleep_debug_proc_entry = create_proc_entry("sleep_debug",
					      S_IRUGO | S_IWUGO, NULL);
	if (!sleep_debug_proc_entry)
		printk(KERN_ERR "%s: failed creating procfile\n",
		       __func__);
	else {
		sleep_debug_proc_entry->read_proc = sleep_debug_proc_read;
		sleep_debug_proc_entry->write_proc = sleep_debug_proc_write;
		sleep_debug_proc_entry->data = (void *) NULL;
	}
}
void sleep_debug_exit(void)
{
	pr_info("%s\n", __func__);
	return;
}
#endif

static struct proc_dir_entry *crit_kmsg_proc_entry;
extern char* get_crit_kmsg_addr(void);
static int crit_kmsg_proc_read(char *page,
				char **start,
				off_t offset, int count, int *eof, void *data)
{
	char *buf = page;
	char* addr = get_crit_kmsg_addr();
	buf += snprintf(buf, 3*1024, "%s\n", addr?addr:"Not implemented");

	*eof = 1;
	
	return buf - page < count ? buf - page : count;
}
void crit_kmsg_register_proc_entry(void)
{
	pr_info("%s\n", __func__);
	
	crit_kmsg_proc_entry = create_proc_entry("last_kmsg",
					      S_IRUGO | S_IWUGO, NULL);
	if (!crit_kmsg_proc_entry)
		printk(KERN_ERR "%s: failed creating procfile\n",
		       __func__);
	else {
		crit_kmsg_proc_entry->read_proc = crit_kmsg_proc_read;
		crit_kmsg_proc_entry->write_proc = NULL;
		crit_kmsg_proc_entry->data = (void *) NULL;
	}
}

void crit_kmsg_init(void) 
{		
	pr_info("%s\n", __func__);
	crit_kmsg_register_proc_entry();
}

static int __init kernel_sec_debug_init(void)
{
	pr_info("%s\n", __func__);
	crit_kmsg_init();
	sleep_debug_init(); //sleep.debug

	return 0;
}

static void __exit kernel_sec_debug_exit(void)
{
	pr_info("%s\n", __func__);
}

module_init(kernel_sec_debug_init);
module_exit(kernel_sec_debug_exit);
#endif // CONFIG_KERNEL_DEBUG_SEC
