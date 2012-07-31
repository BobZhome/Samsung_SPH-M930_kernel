/**
* wimax_sdio.h
*
* functions for device access
* swmxctl (char device): gpio control
* uwibro (char device): send/recv control packet
* sdio device: sdio functions
*/
#ifndef _WIMAX_SDIO_H
#define _WIMAX_SDIO_H

#include "hw_types.h"
#include "ctl_types.h"

#define RX_SKBS					4

/* WiMAX Constants */
#define WIMAX_MTU_SIZE				1400
#define WIMAX_MAX_FRAMESIZE			1500
#define WIMAX_HEADER_SIZE			14
#define WIMAX_MAX_TOTAL_SIZE	(WIMAX_MAX_FRAMESIZE + WIMAX_HEADER_SIZE)
/* maximum allocated data size,  mtu 1400 so 3 blocks max 1536 */
#define BUFFER_DATA_SIZE			1600
#define MINIPORT_VENDOR_DESCRIPTION		"Samsung Electronics"
#define MINIPORT_DRIVER_VERSION			0x00010000
#define MINIPORT_MCAST_LIST_LENGTH		32
#define ADAPTER_TIMEOUT				(HZ * 10)

#define MEDIA_DISCONNECTED			0
#define MEDIA_CONNECTED				1

/* wimax state */
enum {
	WIMAX_STATE_NOT_READY,
	WIMAX_STATE_READY,
	WIMAX_STATE_VIRTUAL_IDLE,
	WIMAX_STATE_NORMAL,
	WIMAX_STATE_IDLE,
	WIMAX_STATE_RESET_REQUESTED,
	WIMAX_STATE_RESET_ACKED,
	WIMAX_STATE_AWAKE_REQUESTED,
};

/* network adapter structure */
struct net_adapter {
	struct sdio_func		*func;
	struct net_device		*net;
	struct net_device_stats		netstats;

	struct task_struct		*tx_task;
	struct task_struct		*rx_task;
	struct task_struct		*mac_task;
	struct task_struct		*wtm_task;
	struct tasklet_struct hostwake_task;

	s32			wake_irq;

	u32			msg_enable;

	u32			XmitErr; /* packet send fails */

	struct hardware_info		hw;
	struct ctl_info			ctl;

	wait_queue_head_t		download_event;
	wait_queue_head_t		receive_event;
	wait_queue_head_t		send_event;
	u8			downloading;	/* firmware downloading */
	u8			download_complete;
	u8			mac_ready;

	u8			media_state;/* mac completion */
	u8			rx_data_available;
	u8			halted;	/* device halt pending flag */
	u8			removed;
};

#define hwSdioReadCounter(Adapter, pLen, pRIdx, Status)\
{\
    *pLen = 0;\
    *pLen = sdio_readl(Adapter->func,	\
			(SDIO_RX_BANK_ADDR + (*pRIdx * SDIO_BANK_SIZE)) ,\
			Status);\
    if (*Status != 0) {\
			\
	*pLen = 0;\
    } \
}

#define hwSdioWriteBankIndex(Adapter, pWIdx, Status)\
{\
    *pWIdx = sdio_readb(Adapter->func, SDIO_H2C_WP_REG, Status);\
    if (*Status == 0) {\
		\
    if (((*pWIdx + 1) % 15) == sdio_readb(Adapter->func,	\
				SDIO_H2C_RP_REG, Status))\
		*pWIdx = -1; } \
}

#define hwSdioReadBankIndex(Adapter, pRIdx, Status)\
{\
   *pRIdx = sdio_readb(Adapter->func, SDIO_C2H_RP_REG, Status);\
    if (*Status == 0) {\
		\
    if (*pRIdx == sdio_readb(Adapter->func, SDIO_C2H_WP_REG, Status))\
		*pRIdx = -1; } \
}


/* swmxctl functions */
int swmxdev_open(struct inode *inode, struct file *file);
int swmxdev_release(struct inode *inode, struct file *file);
int swmxdev_ioctl(struct inode *inode, struct file *file,
		u32 cmd, unsigned long arg);
ssize_t swmxdev_read(struct file *file, char *buf,
		size_t count, loff_t *ppos);
ssize_t swmxdev_write(struct file *file, const char *buf,
		size_t count, loff_t *ppos);

/* uwibro functions */
int uwbrdev_open(struct inode *inode, struct file *file);
int uwbrdev_release(struct inode *inode, struct file *file);
int uwbrdev_ioctl(struct inode *inode, struct file *file,
		u32 cmd, unsigned long arg);
ssize_t uwbrdev_read(struct file *file, char *buf,
		size_t count, loff_t *ppos);
ssize_t uwbrdev_write(struct file *file, const char *buf,
		size_t count, loff_t *ppos);

/* net dev functions */
int adapter_open(struct net_device *net);
int adapter_close(struct net_device *net);
int adapter_ioctl(struct net_device *net, struct ifreq *rq,
		int cmd);
int adapter_probe(struct sdio_func *func,
		const struct sdio_device_id *id);
void adapter_remove(struct sdio_func *func);
struct net_device_stats *adapter_netdev_stats(struct net_device *dev);
int adapter_start_xmit(struct sk_buff *skb, struct net_device *net);
void adapter_set_multicast(struct net_device *net);

/* suspend/resume function */
int wimax_suspend(void);
int wimax_resume(void);

#endif	/* _WIMAX_SDIO_H */
