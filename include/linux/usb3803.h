#ifndef USB3803_H
#define USB3803_H

#define USB3803_I2C_NAME "usb3803"

enum {
	USB_3803_MODE_HUB = 0,
	USB_3803_MODE_BYPASS = 1,
};

struct usb3803_platform_data {
	bool init_needed;
	bool inital_mode;
	bool es_ver;
	int (*hw_config)(void);
	int (*reset_n)(int);
	int (*bypass_n)(int);
	int (*clock_en)(int);
};

#endif

