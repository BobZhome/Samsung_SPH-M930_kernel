#ifndef _KERNEL_SEC_COMMON_H_
#define _KERNEL_SEC_COMMON_H_

#include <asm/io.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/sched.h>

/*****
* Debug Level : It's used for debugging and silent reset, it's meaningful in AP master model 
******/
//#define KERNEL_SEC_DEBUG_LEVEL_LOW	(0x574F4C44)
//#define KERNEL_SEC_DEBUG_LEVEL_MID	(0x44494D44)
//#define KERNEL_SEC_DEBUG_LEVEL_HIGH	(0x47494844)


// klaatu - ISR and task schedule log
// dependency : sched.c, handle.c, kernel_sec_debug.c
//#define SCHED_LOG_MAX 1000

#define _DEBUG_
#ifdef _DEBUG_
#define MSGCRIT "\x1b[1;31m"
#define MSGERR "\x1b[1;33m"
#define MSGWARN "\x1b[1;35m"
#define MSGINFO "\x1b[1;32m"
#define MSGDBG "\x1b[1;37m"

#define MSGEND "\x1b[0m \n"
#else
#define MSGCRIT 
#define MSGERR 
#define MSGWARN 
#define MSGINFO 
#define MSGDBG 
#define MSGEND "\n"
#endif

#ifndef PRODUCT_SHIP
#define CONFIG_SEC_KERNEL_FUNC_TRACE
#endif

#ifdef CONFIG_SMP
#undef CONFIG_SEC_KERNEL_FUNC_TRACE
#endif

#ifdef CONFIG_SEC_KERNEL_FUNC_TRACE
extern void kernel_func_trace(const char* msg, unsigned int data1, unsigned int data2, unsigned int data3);
#define KFUNC_TRACE0() kernel_func_trace(NULL, 0)
#define KFUNC_TRACE(msg,data) kernel_func_trace(msg, (unsigned int)data, 0, 0)
#define KFUNC_TRACE3(msg,data1,data2,data3) kernel_func_trace(msg, data1, data2,data3)
#else
#define KFUNC_TRACE0()
#define KFUNC_TRACE(msg,data)
#define KFUNC_TRACE3(msg,data1,data2,data3)
#endif

typedef struct {
    void * dummy;
    void * fn;
}irq_log_t;

typedef union {
    char task[TASK_COMM_LEN];
    irq_log_t irq;
}task_log_t;

typedef struct {
    unsigned long long time;
    task_log_t log;
}sched_log_t;

typedef struct tag_mmu_info
{	
	int SCTLR;
	int TTBR0;
	int TTBR1;
	int TTBCR;
	int DACR;
	int DFSR;
	int DFAR;
	int IFSR;
	int IFAR;
	int DAFSR;
	int IAFSR;
	int PMRRR;
	int NMRRR;
	int FCSEPID;
	int CONTEXT;
	int URWTPID;
	int UROTPID;
	int POTPIDR;
}t_kernel_sec_mmu_info;

/*ARM CORE regs mapping structure*/
typedef struct
{
	/* COMMON */
	unsigned int r0;
	unsigned int r1;
	unsigned int r2;
	unsigned int r3;
	unsigned int r4;
	unsigned int r5;
	unsigned int r6;
	unsigned int r7;
	unsigned int r8;
	unsigned int r9;
	unsigned int r10;
	unsigned int r11;
	unsigned int r12;

	/* SVC */
	unsigned int r13_svc;
	unsigned int r14_svc;
	unsigned int spsr_svc;

	/* PC & CPSR */
	unsigned int pc;
	unsigned int cpsr;
	
	/* USR/SYS */
	unsigned int r13_usr;
	unsigned int r14_usr;

	/* FIQ */
	unsigned int r8_fiq;
	unsigned int r9_fiq;
	unsigned int r10_fiq;
	unsigned int r11_fiq;
	unsigned int r12_fiq;
	unsigned int r13_fiq;
	unsigned int r14_fiq;
	unsigned int spsr_fiq;

	/* IRQ */
	unsigned int r13_irq;
	unsigned int r14_irq;
	unsigned int spsr_irq;

	/* MON */
	unsigned int r13_mon;
	unsigned int r14_mon;
	unsigned int spsr_mon;

	/* ABT */
	unsigned int r13_abt;
	unsigned int r14_abt;
	unsigned int spsr_abt;

	/* UNDEF */
	unsigned int r13_und;
	unsigned int r14_und;
	unsigned int spsr_und;

}t_kernel_sec_arm_core_regsiters;

typedef enum
{
	UPLOAD_CAUSE_INIT           = 0x00000000,
	UPLOAD_CAUSE_KERNEL_PANIC   = 0x000000C8,
	UPLOAD_CAUSE_FORCED_UPLOAD  = 0x00000022,
	UPLOAD_CAUSE_USER_FAULT 	= 0x0000002F,
}kernel_sec_upload_cause_type;


//extern sched_log_t gExcpTaskLog[SCHED_LOG_MAX];
//extern unsigned int gExcpTaskLogIdx;

extern void kernel_sec_set_build_info(void);
extern void kernel_sec_set_upload_cause(kernel_sec_upload_cause_type uploadType);
extern void kernel_sec_get_core_reg_dump(t_kernel_sec_arm_core_regsiters* regs);
extern int  kernel_sec_get_mmu_reg_dump(t_kernel_sec_mmu_info *mmu_info);
extern void kernel_sec_save_final_context(void);
extern void kernel_sec_reset(bool bSilentReset);

#endif /* _KERNEL_SEC_COMMON_H_ */
