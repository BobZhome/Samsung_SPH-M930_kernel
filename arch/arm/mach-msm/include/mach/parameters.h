
#define PARAM_TYPE_BOOT 0
#define PARAM_TYPE_KERNEL 1

struct samsung_parameter {
	unsigned int usb_sel;
#if 1 //20101206_inchul
	unsigned int uart_sel;
#endif
	unsigned int usb_mode;
	unsigned int diag_dev_sel;

	unsigned int ram_dump_level_init;
	unsigned int ram_dump_level;
};

#define PARAM_CMDLINE_SIZE 256

/* PARAM STRUCTURE */
struct BOOT_PARAM {
	int booting_now;	// 1:boot   0:enter service
	int fota_mode;	// 5 : FOTA_ENTER_MODE   0:NORMAL_MODE
	int update; // delta update ex) 0x001: KERNEL_UPDATE_SUCCESS , 0x011: KERNEL_UPDATE_SUCCESS + MODEM_UPDATE_SUCCESS
	int gota_mode;
	int recovery_command;
	char str1[32];
	char str2[32];

	char cmdline[PARAM_CMDLINE_SIZE];

	char multi_download[20]; /* rory : samsung multi download solution */
	int multi_download_slot_num;
	unsigned int usb_serial_number_high;
	unsigned int usb_serial_number_low;

	unsigned int first_boot_done;
	unsigned int custom_download_cnt;
	char current_binary[30];
};


extern void msm_read_param(struct samsung_parameter *param_data);
extern void msm_write_param(struct samsung_parameter *param_data);
extern int param_read(int type, void *param_data, int param_size);
extern int param_write(int type, void *param_data, int param_size);

extern void msm_read_boot_param(struct BOOT_PARAM *param_data);
extern void msm_write_boot_param(struct BOOT_PARAM *param_data);
