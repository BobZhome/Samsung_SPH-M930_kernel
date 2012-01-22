/*
 * headers.h
 *
 * Global definitions and fynctions
 */
#ifndef _WIMAX_HEADERS_H
#define _WIMAX_HEADERS_H

#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/proc_fs.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio_func.h>
#include <asm/byteorder.h>
#include <linux/uaccess.h>
#include <linux/wakelock.h>
#include <linux/wimax/samsung/wimax732.h>

#include "buffer.h"
#include "wimax_sdio.h"

#define WIMAXMAC_TXT_PATH	"/efs/WiMAXMAC.txt"
#define WIMAX_IMAGE_PATH	"/system/etc/wimaxfw.bin"
#define WIMAX_LOADER_PATH	"/system/etc/wimaxloader.bin"
#define WIMAX_BOOTIMAGE_PATH	"/system/etc/wimax_boot.bin"

#define STATUS_SUCCESS			((u_long)0x00000000L)
/* The operation that was requested is pending completion */
#define STATUS_PENDING			((u_long)0x00000103L)
#define STATUS_RESOURCES		((u_long)0x00001003L)
#define STATUS_RESET_IN_PROGRESS	((u_long)0xc001000dL)
#define STATUS_DEVICE_FAILED		((u_long)0xc0010008L)
#define STATUS_NOT_ACCEPTED		((u_long)0x00010003L)
#define STATUS_FAILURE			((u_long)0xC0000001L)
/* The requested operation was unsuccessful */
#define STATUS_UNSUCCESSFUL		((u_long)0xC0000002L)
#define STATUS_CANCELLED		((u_long)0xC0000003L)

#ifndef TRUE_FALSE_
#define TRUE_FALSE_
enum BOOL {
	FALSE,
	TRUE
};
#endif

#define HARDWARE_USE_ALIGN_HEADER

/*
* external functions & variables
*/
extern int cmc732_sdio_reset_comm(struct mmc_card *card);

extern u32 system_rev;

/* control.c functions */
u32 control_send(struct net_adapter *adapter, void *buffer, u32 length);
void control_recv(struct net_adapter   *adapter, void *buffer, u32 length);
u32 control_init(struct net_adapter *adapter);
void control_remove(struct net_adapter *adapter);

struct process_descriptor *process_by_id(struct net_adapter *adapter,
		u32 id);
struct process_descriptor *process_by_type(struct net_adapter *adapter,
		u16 type);
void remove_process(struct net_adapter *adapter, u32 id);

u32 buffer_count(struct list_head ListHead);
struct buffer_descriptor *buffer_by_type(struct list_head ListHead,
		u16 type);
void dump_buffer(const char *desc, u8 *buffer, u32 len);

/* hardware.c functions */

u32 sd_send_data(struct net_adapter *adapter,
		struct buffer_descriptor *dsc);
u32 hw_send_data(struct net_adapter *adapter,
		void *buffer, u32 length);
void hw_return_packet(struct net_adapter *adapter, u16 type);

int hw_start(struct net_adapter *adapter);
int hw_stop(struct net_adapter *adapter);
int hw_init(struct net_adapter *adapter);
void hw_remove(struct net_adapter *adapter);
int hw_get_mac_address(void *data);
int con0_poll_thread(void *data);

int cmc732_receive_thread(void *data);
int cmc732_send_thread(void *data);

void adapter_interrupt(struct sdio_func *func);

/* structures for global access */
extern struct net_adapter *g_adapter;
extern struct wimax732_platform_data	*g_pdata;

#endif	/* _WIMAX_HEADERS_H */

