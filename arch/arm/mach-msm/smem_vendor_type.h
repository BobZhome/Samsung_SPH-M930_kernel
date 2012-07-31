#ifndef SMEM_VENDOR_TYPE_H
#define SMEM_VENDOR_TYPE_H
/*
*  SMEM_ID_VENDOR1 type definition
*
*/

//#include "smem.h"

#define SMEM_VENDOR1_CAPTURE_BUF_SIZE (128*1024)

enum {
	reg_ORIG_r0 = 17,
	reg_cpsr    = 16,
	reg_pc      = 15,
	reg_lr      = 14,
	reg_sp      = 13,
	reg_ip      = 12,
	reg_fp      = 11,
	reg_r10     = 10,
	reg_r9      = 9,
	reg_r8      = 8,
	reg_r7      = 7,
	reg_r6      = 6,
	reg_r5      = 5,
	reg_r4      = 4,
	reg_r3      = 3,
	reg_r2      = 2,
	reg_r1      = 1,
	reg_r0      = 0,
};

typedef struct MODEM_RAMDUMP_TYPE
{
	unsigned char modem;
	unsigned int modem_line;
	unsigned char modem_file[40];
	unsigned char modem_format[40];
	unsigned char modem_qxdm_message[80];
} modem_ramdump;


typedef struct APPS_RAMDUMP_TYPE
{
  unsigned char apps;
  size_t apps_regs[18];
  size_t apps_pid;
  size_t mmu_table[4];
  unsigned char apps_process[100];
  unsigned char apps_string[100];
} apps_ramdump;



/* Mark for GetLog */

typedef struct KRNL_LOG_MARK {
	unsigned int special_mark_1;
	unsigned int special_mark_2;
	unsigned int special_mark_3;
	unsigned int special_mark_4;
	void *p__log_buf;
} krnl_log_mark;

typedef struct PLATFORM_LOG_MARK  {
  unsigned int special_mark_1;
  unsigned int special_mark_2;
  unsigned int special_mark_3;
  unsigned int special_mark_4;
  void *p_main;
  void *p_radio;
  void *p_events;
  void *p_system;
} platform_log_mark;


typedef struct VER_LOG_MARK {
	unsigned int special_mark_1;
	unsigned int special_mark_2;
	unsigned int special_mark_3;
	unsigned int special_mark_4;
	unsigned int log_mark_version;
	unsigned int framebuffer_mark_version;
	void * this;
	unsigned int first_size;
	unsigned int first_start_addr;
	unsigned int second_size;
	unsigned int second_start_addr;
} ver_log_mark;

typedef struct FRAME_LOG_MARK {
	unsigned int special_mark_1;
	unsigned int special_mark_2;
	unsigned int special_mark_3;
	unsigned int special_mark_4;
	void *p_fb;
	unsigned int resX;
	unsigned int resY;
	unsigned int bpp; //color depth : 16 or 24
	unsigned int frames; // frame buffer count : 2
} frame_log_mark;


struct dpram_intr_dbg {
	unsigned int sent;
	unsigned int rcvd;
};

typedef struct {
	struct dpram_intr_dbg m2a;
	struct dpram_intr_dbg a2m;
} dpram_dbg_type;

typedef struct SMEM_VENDOR1_ID_TYPE
{
	/* This must be exactly the same with smem_vendor_types.h in CP boot, CP
	 * AP boot and AP ! */
	unsigned int boot_mode;					/* current boot mode */
	unsigned int reboot_reason; 			/* last reboot reason(high 16-bit) lower 16-bit is boot status read from PMIC */
	unsigned char rebuild;			
	unsigned char hw_version;
	unsigned char off_charging_off;
	unsigned char download_mode;
	modem_ramdump modem_dump;
	apps_ramdump  apps_dump;
	unsigned char capture_buffer[SMEM_VENDOR1_CAPTURE_BUF_SIZE];
	krnl_log_mark klog_mark;
	platform_log_mark plog_mark;
	ver_log_mark vlog_mark;
	frame_log_mark flog_mark;
	unsigned int efs_start_block;
	unsigned int efs_length;
	unsigned int factory_reset;
	unsigned int CP_status;					/* Current CP status */
	unsigned int AP_status;					/* Current AP status */
	dpram_dbg_type dpram_debug;
	unsigned int fota_mode;
	unsigned int uart_console;
	unsigned int ram_dump_level;
	unsigned long default_esn;
	unsigned char backup;
	unsigned int fota_update_result;
	unsigned int warning_mode;
	unsigned int lcd_type; /* LcdTypeSMD = 1, LcdTypeDTC = 2 */

	unsigned int AP_reserved[11];			/* Only AP should use this */
	unsigned int CP_reserved[10];			/* Only CP should use this */
}__attribute__((aligned(8))) __attribute__((packed))samsung_vendor1_id;

#endif /* SMEM_VENDOR_TYPE_H */
