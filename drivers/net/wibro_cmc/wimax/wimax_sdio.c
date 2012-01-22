/**
* wimax_sdio.c
*
* functions for device access
* swmxctl (char device): gpio control
* uwibro (char device): send/recv control packet
* sdio device: sdio functions
*/
#include "headers.h"
#include "wimax_proc.h"
#include "wimax_i2c.h"
#include "ctl_types.h"
#include "download.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/miscdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio_func.h>
#include <asm/byteorder.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/platform_device.h>

/* driver Information */
#define WIMAX_DRIVER_VERSION_STRING "3.0.2"
#define DRIVER_AUTHOR "Samsung"
#define DRIVER_DESC "Samsung WiMAX SDIO Device Driver"

#define UWBRBDEVMINOR	233
#define SWMXGPIOMINOR	234

extern int s3c_bat_use_wimax(int onoff);

struct net_adapter *g_adapter;
static u8 node_id[ETH_ALEN];

/* use ethtool to change the level for any given device */
static int msg_level = -1;
module_param(msg_level, int, 0);

static const struct file_operations swmx_fops = {
owner: THIS_MODULE,
	open :		swmxdev_open,
	release :	swmxdev_release,
	ioctl :		swmxdev_ioctl,
	read :		swmxdev_read,
	write :		swmxdev_write,
};

static struct miscdevice swmxctl_dev = {
	.minor = SWMXGPIOMINOR,
	.name = "swmxctl",
	.fops = &swmx_fops,
};

/*
* swmxctl functions
* (power on/off and factory function test)
*/
int swmxdev_open(struct inode *inode, struct file *file)
{
	dump_debug("Device open by %d", current->tgid);
	return 0;
}

int swmxdev_release(struct inode *inode, struct file *file)
{
	dump_debug("Device close by %d", current->tgid);
	return 0;
}

int swmxdev_ioctl(struct inode *inode, struct file *file,
		u32 cmd, unsigned long arg)
{
	int	ret = 0;
	u8	val = ((u8 *)arg)[0];

	dump_debug("CMD: %x, PID: %d", cmd, current->tgid);

	switch (cmd) {
	case CONTROL_IOCTL_WIMAX_POWER_CTL: {
		dump_debug("CONTROL_IOCTL_WIMAX_POWER_CTL..");
		if (val == 0)
			ret = g_pdata->power(0);
		else
			ret = g_pdata->power(1);
		break;
	}
	case CONTROL_IOCTL_WIMAX_MODE_CHANGE: {
		dump_debug("CONTROL_IOCTL_WIMAX_MODE_CHANGE to 0x%02x..", val);

		if ((val < 0) || (val > AUTH_MODE)) {
			dump_debug("Wrong mode 0x%02x", val);
			return 0;
		}

		g_pdata->power(0);

		g_pdata->g_cfg->wimax_mode = val;

		ret = g_pdata->power(1);
		break;
	}
	case CONTROL_IOCTL_WIMAX_EEPROM_DOWNLOAD: {
		dump_debug("CONTROL_IOCTL_WIMAX_EEPROM_DOWNLOAD");
		g_pdata->power(0);

		ret = eeprom_write_boot();
		break;
	}
	case CONTROL_IOCTL_WIMAX_SLEEP_MODE: {
		if (val == 0) {
			dump_debug("AP SLEEP: WIMAX VI");
			g_pdata->g_cfg->sleep_mode = 0;
		} else {
			dump_debug("AP SLEEP: WIMAX IDLE");
			g_pdata->g_cfg->sleep_mode = 1;
		}

		break;
	}
	case CONTROL_IOCTL_WIMAX_WRITE_REV: {
		dump_debug("CONTROL_IOCTL_WIMAX_WRITE_REV");
		g_pdata->power(0);
		ret = eeprom_write_rev();
		break;
	}
	case CONTROL_IOCTL_WIMAX_CHECK_CERT: {
		dump_debug("CONTROL_IOCTL_WIMAX_CHECK_CERT");
		g_pdata->power(0);
		ret = eeprom_check_cert();
		break;
	}
	case CONTROL_IOCTL_WIMAX_CHECK_CAL: {
		dump_debug("CONTROL_IOCTL_WIMAX_CHECK_CAL");
		g_pdata->power(0);
		ret = eeprom_check_cal();
		break;
	}
	}	/* switch (cmd) */

	return ret;
}

ssize_t swmxdev_read(struct file *file, char *buf,
		size_t count, loff_t *ppos)
{
	return 0;
}

ssize_t swmxdev_write(struct file *file, const char *buf,
		size_t count, loff_t *ppos)
{
	return 0;
}

/*
*	uwibro functions
*	(send and receive control packet with WiMAX modem)
*/
int uwbrdev_open(struct inode *inode, struct file *file)
{
	struct net_adapter		*adapter;
	struct process_descriptor	*process;

	if ((g_adapter == NULL) || g_adapter->halted) {
		dump_debug("can't find adapter or Device Removed");
		return -ENODEV;
	}

	file->private_data = (void *)g_adapter;
	adapter = (struct net_adapter *)(file->private_data);
	dump_debug("open: tgid=%d", current->tgid);

	if (adapter->mac_ready != TRUE || adapter->halted) {
		dump_debug("Device not ready Retry..");
		return -ENXIO;
	}

	process = process_by_id(adapter, current->tgid);
	if (process != NULL) {
		dump_debug("second open attemp from uid %d", current->tgid);
		return -EEXIST;
	} else {
		/* init new process descriptor */
		process = kmalloc(sizeof(struct process_descriptor),
				GFP_KERNEL);
		if (process == NULL) {
			dump_debug("uwbrdev_open: kmalloc fail!!");
			return -ENOMEM;
		} else {
			process->id = current->tgid;
			process->irp = FALSE;
			process->type = 0;
			init_waitqueue_head(&process->read_wait);
			spin_lock(&adapter->ctl.apps.lock);
			queue_put_tail(adapter->ctl.apps.process_list,
					process->node);
			spin_unlock(&adapter->ctl.apps.lock);
		}
	}

	return 0;
}

int uwbrdev_release(struct inode *inode, struct file *file)
{
	struct net_adapter		*adapter;
	struct process_descriptor	*process;
	int				current_tgid = 0;

	dump_debug("release: tgid=%d, pid=%d", current->tgid, current->pid);

	adapter = (struct net_adapter *)(file->private_data);
	if (adapter == NULL) {
		dump_debug("can't find adapter");
		return -ENODEV;
	}

	current_tgid = current->tgid;
	process = process_by_id(adapter, current_tgid);

	/* process is not exist. (open process != close process) */
	if (process == NULL) {
		current_tgid = g_pdata->g_cfg->temp_tgid;
		dump_debug("release: pid changed: %d", current_tgid);
		process = process_by_id(adapter, current_tgid);
	}

	if (process != NULL) {
		/* RELEASE READ THREAD */
		if (process->irp) {
			process->irp = FALSE;
			wake_up_interruptible(&process->read_wait);
		}
		spin_lock(&adapter->ctl.apps.lock);
		remove_process(adapter, current_tgid);
		spin_unlock(&adapter->ctl.apps.lock);
	} else {
		/*not found */
		dump_debug("process %d not found", current_tgid);
		return -ESRCH;
	}

	return 0;
}

static const struct file_operations uwbr_fops = {
owner: THIS_MODULE,
	open :		uwbrdev_open,
	release :	uwbrdev_release,
	ioctl :		uwbrdev_ioctl,
	read :		uwbrdev_read,
	write :		uwbrdev_write,
};

static struct miscdevice uwibro_dev = {
	.minor = UWBRBDEVMINOR,
	.name = "uwibro",
	.fops = &uwbr_fops,
};

/* buffer used in uwbrdev_ioctl routine */
struct control_tx_buffer {
	int	length;
	u8	data[WIMAX_MAX_TOTAL_SIZE];
};

static struct control_tx_buffer g_tx_buffer;

int uwbrdev_ioctl(struct inode *inode, struct file *file, u32 cmd,
		unsigned long arg)
{
	struct net_adapter		*adapter;
	struct process_descriptor	*process;
	int				ret = 0;

	adapter = (struct net_adapter *)(file->private_data);

	if ((adapter == NULL) || adapter->halted) {
		dump_debug("can't find adapter or Device Removed");
		return -ENODEV;
	}

	switch (cmd) {
	case CONTROL_IOCTL_WRITE_REQUEST: {
		struct eth_header *ctlhdr;

		memset(&g_tx_buffer, 0x0, sizeof(struct control_tx_buffer));

		if ((char *)arg == NULL) {
			dump_debug("arg == NULL: return -EFAULT");
			return -EFAULT;
		}

		g_tx_buffer.length = ((struct control_tx_buffer *)arg)->length;

		if (g_tx_buffer.length < WIMAX_MAX_TOTAL_SIZE) {
			if (copy_from_user(g_tx_buffer.data,
					(void *)(arg + sizeof(int)),
					g_tx_buffer.length))
				return -EFAULT;
		} else
			return -EFBIG;

		spin_lock(&adapter->ctl.apps.lock);
		process = process_by_id(adapter, current->tgid);
		if (process == NULL) {
			dump_debug("process %d not found", current->tgid);
			ret = -EFAULT;
			spin_unlock(&adapter->ctl.apps.lock);
			break;
		}
		ctlhdr = (struct eth_header *)g_tx_buffer.data;
		process->type = ctlhdr->type;

		control_send(adapter, g_tx_buffer.data, g_tx_buffer.length);
		spin_unlock(&adapter->ctl.apps.lock);
		break;
	}
	default:
		dump_debug("uwbrdev_ioctl: unknown ioctl cmd: 0x%x", cmd);
		break;
	}	/* switch (cmd) */

	return ret;
}

ssize_t uwbrdev_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	struct buffer_descriptor	*dsc;
	struct net_adapter		*adapter;
	struct process_descriptor	*process;
	int				rlen = 0;

	adapter = (struct net_adapter *)(file->private_data);
	if ((adapter == NULL) || adapter->halted) {
		dump_debug("can't find adapter or Device Removed");
		return -ENODEV;
	}

	if (buf == NULL) {
		dump_debug("BUFFER is NULL");
		return -EFAULT; /* bad address */
	}

	process = process_by_id(adapter, current->tgid);
	if (process == NULL) {
		dump_debug("uwbrdev_read: process %d not exist", current->tgid);
		return -ESRCH;
	}

	if (process->irp == FALSE) {
		dsc = buffer_by_type(adapter->ctl.q_received_ctrl.head,
				process->type);
		if (dsc == NULL) {
			process->irp = TRUE;
			if (wait_event_interruptible(process->read_wait,
					((process->irp == FALSE) ||
					 (adapter->halted == TRUE)))) {
				process->irp = FALSE;
				g_pdata->g_cfg->temp_tgid = current->tgid;
				return -ERESTARTSYS;
			}
			if (adapter->halted == TRUE) {
				dump_debug("uwbrdev_read: Card Removed"
						"Indicated to Appln...");
				process->irp = FALSE;
				g_pdata->g_cfg->temp_tgid = current->tgid;
				return -ENODEV;
			}
		}

		if (count == 1500) {	/* app passes read count as 1500 */
			spin_lock(&adapter->ctl.apps.lock);
			dsc = buffer_by_type(adapter->ctl.q_received_ctrl.head,
					process->type);
			if (!dsc) {
				dump_debug("uwbrdev_read: Fail...node is null");
				spin_unlock(&adapter->ctl.apps.lock);
				return -1;
			}
			spin_unlock(&adapter->ctl.apps.lock);

			if (copy_to_user(buf, dsc->buffer, dsc->length)) {
				dump_debug("uwbrdev_read: copy_to_user	\
					failed len=%u !!", dsc->length);
				return -EFAULT;
			}

			spin_lock(&adapter->ctl.apps.lock);
			rlen = dsc->length;
			hw_return_packet(adapter, dsc->type);
			spin_unlock(&adapter->ctl.apps.lock);
		}
	} else {
		dump_debug("uwbrdev_read: Read was sent"
				"twice by process %d", current->tgid);
		return -EEXIST;
	}

	return rlen;
}

ssize_t uwbrdev_write(struct file *file, const char *buf,
		size_t count, loff_t *ppos)
{
	return 0;
}

static struct net_device_ops wimax_net_ops = {
	.ndo_open =			adapter_open,
	.ndo_stop =			adapter_close,
	.ndo_get_stats =		adapter_netdev_stats,
	.ndo_do_ioctl =			adapter_ioctl,
	.ndo_start_xmit =		adapter_start_xmit,
	.ndo_set_mac_address =		NULL,
	.ndo_set_multicast_list =	adapter_set_multicast
};

static const struct sdio_device_id adapter_table[] = {
	{ SDIO_DEVICE(0x98, 0x1) },
	{ }	/* Terminating entry */
};

static struct sdio_driver adapter_driver = {
	.name		= "C730SDIO",
	.probe		= adapter_probe,
	.remove		= adapter_remove,
	.id_table	= adapter_table,
};

static int wimax_probe(struct platform_device *pdev)
{

	int error = 0;

	g_adapter = NULL;

	dump_debug("SDIO driver installing... " WIMAX_DRIVER_VERSION_STRING);

	/* register gpio control char driver */
	error = misc_register(&swmxctl_dev);

	if (error < 0) {
		dump_debug("misc_register() failed");
		return error;
	}

	/* register SDIO driver */
	error = sdio_register_driver(&adapter_driver);
	if (error < 0) {
		dump_debug("sdio_register_driver() failed");
		return error;
	}

	/* initialize proc file system */
	proc_init();

	if (wmxeeprom_init())
		dump_debug("wmxeeprom_init() failed");

	g_pdata =
		(struct wimax732_platform_data	*)(pdev->dev.platform_data);

	g_pdata->g_cfg->probe_complete = TRUE;
	g_pdata->g_cfg->card_removed = TRUE;

	g_pdata->power(0);

	g_pdata->g_cfg->sleep_mode = 1;	/* default sleep mode IDLE */

	/* initialize wake locks */
	wake_lock_init(&g_pdata->g_cfg->wimax_wake_lock,
			WAKE_LOCK_SUSPEND, "wimax_wakeup");
	wake_lock_init(&g_pdata->g_cfg->wimax_rxtx_lock,
			WAKE_LOCK_SUSPEND, "wimax_rxtx");
	return 0;
}

static int wimax_remove(struct platform_device *pdev)
{
	dump_debug("SDIO driver Uninstall");

	/* destroy wake locks */
	wake_lock_destroy(&g_pdata->g_cfg->wimax_wake_lock);
	wake_lock_destroy(&g_pdata->g_cfg->wimax_rxtx_lock);

	unload_wimax_image();

	sdio_unregister_driver(&adapter_driver);
	misc_deregister(&swmxctl_dev);

	proc_deinit();

	wmxeeprom_exit();
	return 0;
}

/* wimax suspend function: call from mmc host */
int wimax_suspend(void)
{
     if(g_pdata->get_wimax_en() == 1) 
     {
        if(g_pdata->get_wimax_int() == 0)
        {
       	  /* wait wakeup noti for 1 sec otherwise suspend again */
          wake_lock_timeout(&g_pdata->g_cfg->wimax_wake_lock, 1 * HZ);
        }
        /* AP active pin LOW */
        g_pdata->signal_ap_active(0);
        
        /* display WiMAX uart msg during AP suspend */
        if (g_pdata->g_cfg->enable_dump_msg == 1) {
        	msleep(10);
        	g_pdata->switch_uart_wimax();
        }
     }
     
     return 0;

}

/* wimax resume function: call from mmc host */
int wimax_resume(void)
{
    if(g_pdata->get_wimax_en() == 1) 
    {	
    	/* AP active pin HIGH */
    	g_pdata->signal_ap_active(1);
    
    	/* wait wakeup noti for 1 sec otherwise suspend again */
    	wake_lock_timeout(&g_pdata->g_cfg->wimax_wake_lock, 1 * HZ);
    
    	/* restore to AP uart */
    	if (g_pdata->g_cfg->enable_dump_msg == 1) {
    		g_pdata->switch_uart_ap();
    		msleep(10);
    	}
    }
	
    return 0;
}

static struct platform_driver wimax_device = {
	.probe          = wimax_probe,
	.remove         = wimax_remove,
	.suspend        = wimax_suspend,
	.resume         = wimax_resume,
	.driver         = {
	.name   = "cmc732_wimax",
	}
};

static void create_char_name(u8 *str, u32 index)
{
	u8	tempName[] = "uwbrdev";

	sprintf(str, "%s%u", tempName, index);
	return;
}

static int netdev_ethtool_ioctl(struct net_device *dev,
		void *useraddr)
{
	u32	ethcmd;

	if (copy_from_user(&ethcmd, useraddr, sizeof(ethcmd)))
		return -EFAULT;

	switch (ethcmd) {
	case ETHTOOL_GDRVINFO: {
		struct ethtool_drvinfo info = {ETHTOOL_GDRVINFO};
		strncpy(info.driver, "C730USB", sizeof(info.driver) - 1);
		if (copy_to_user(useraddr, &info, sizeof(info)))
			return -EFAULT;

		return 0;
	}
	default:
		break;
	}

	return -EOPNOTSUPP;
}

struct net_device_stats *adapter_netdev_stats(struct net_device *dev)
{
	return &((struct net_adapter *)netdev_priv(dev))->netstats;
}

int adapter_start_xmit(struct sk_buff *skb, struct net_device *net)
{
	struct net_adapter	*adapter = netdev_priv(net);
	int			len;

	if (!adapter->media_state || adapter->halted) {
		dump_debug("Driver already halted. Returning Failure...");
		dev_kfree_skb(skb);
		adapter->netstats.tx_dropped++;
		net->trans_start = jiffies;
		adapter->XmitErr += 1;
		return 0;
	}

	len = ((skb->len) & 0x3f) ? skb->len : skb->len + 1;
	hw_send_data(adapter, skb->data, len);
	dev_kfree_skb(skb);

	if (!adapter->media_state)
		netif_stop_queue(net);

	return 0;
}

void adapter_set_multicast(struct net_device *net)
{
	return;
}

int adapter_open(struct net_device *net)
{
	struct net_adapter	*adapter;
	int			res = 0;

	adapter = netdev_priv(net);

	if (adapter == NULL || adapter->halted) {
		dump_debug("can't find adapter or halted");
		return -ENODEV;
	}

	if (adapter->media_state)
		netif_wake_queue(net);
	else
		netif_stop_queue(net);

	if (netif_msg_ifup(adapter))
		dump_debug("netif msg if up");

	res = 0;

	s3c_bat_use_wimax(1);

	dump_debug("adapter driver open success!!!!!!!");

	return res;
}

int adapter_close(struct net_device *net)
{
	dump_debug("adapter driver close success!!!!!!!");

	s3c_bat_use_wimax(0);
	
	netif_stop_queue(net);
	return 0;
}

int adapter_ioctl(struct net_device *net, struct ifreq *rq, int cmd)
{
	struct net_adapter	*adapter = netdev_priv(net);

	if (adapter->halted) {
		dump_debug("Driver already halted. Returning Failure...");
		return STATUS_UNSUCCESSFUL;
	}

	switch (cmd) {
	case SIOCETHTOOL:
		return netdev_ethtool_ioctl(net, (void *)rq->ifr_data);
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

void adapter_interrupt(struct sdio_func *func)
{
	struct hw_private_packet	hdr;
	struct net_adapter		*adapter = sdio_get_drvdata(func);
	int				err = 1;
	int				intrd = 0;

	wake_lock_timeout(&g_pdata->g_cfg->wimax_rxtx_lock, 0.2 * HZ);

	if (likely(!adapter->halted)) {
		/* read interrupt identification register */
		intrd = sdio_readb(func, SDIO_INT_STATUS_REG, NULL);

		sdio_writeb(func, intrd, SDIO_INT_STATUS_CLR_REG, NULL);

		if (likely(intrd & SDIO_INT_DATA_READY)) {
			adapter->rx_data_available = TRUE;
			wake_up_interruptible(&adapter->receive_event);
		} else if (intrd & SDIO_INT_ERROR) {
			adapter->netstats.rx_errors++;
			dump_debug("adapter_sdio_rx_packet intrd ="
					"SDIO_INT_ERROR occurred!!");
		}
	} else {
		dump_debug("adapter->halted=TRUE in	\
				adapter_interrupt !!!!!!!!!");

		/* send stop message */
		hdr.id0	 = 'W';
		hdr.id1	 = 'P';
		hdr.code  = HwCodeHaltedIndication;
		hdr.value = 0;

		err = sd_send(adapter, (unsigned char *)&hdr,
				sizeof(struct hw_private_packet));
		if (err < 0) {
			dump_debug("adapter->halted=TRUE and send"
				"HaltIndication to FW err = (%d) !!", err);
			return;
		}
	}
}

int adapter_probe(struct sdio_func *func, const struct sdio_device_id *id)
{
	struct net_adapter	*adapter;
	struct net_device	*net;
	u8			charName[32];
	int			nRes = -ENOMEM;
	u32			idx = 0;

	g_pdata->g_cfg->probe_complete = FALSE;
	g_pdata->g_cfg->card_removed = FALSE;

	dump_debug("Probe!!!!!!!!!");

	net = alloc_etherdev(sizeof(struct net_adapter));
	if (!net) {
		dump_debug("adapter_probe: error can't allocate device");
		goto alloceth_fail;
	}

	adapter = netdev_priv(net);
	memset(adapter, 0, sizeof(struct net_adapter));
	g_adapter = adapter;

	/* Initialize control */
	control_init(adapter);

	/* initialize hardware */
	nRes = hw_init(adapter);
	if (nRes) {
		dump_debug("adapter_probe: error can't allocate"
				"receive buffer");
		goto hwInit_fail;
	}

	strcpy(net->name, "uwbr%d");

	adapter->func = func;
	adapter->net = net;
	net->netdev_ops = &wimax_net_ops;
	net->watchdog_timeo = ADAPTER_TIMEOUT;
	net->mtu = WIMAX_MTU_SIZE;
	adapter->msg_enable = netif_msg_init(msg_level, NETIF_MSG_DRV
					| NETIF_MSG_PROBE | NETIF_MSG_LINK);

	ether_setup(net);
	net->flags |= IFF_NOARP;

	adapter->downloading = TRUE;

	sdio_set_drvdata(func, adapter);

	SET_NETDEV_DEV(net, &func->dev);
	nRes = register_netdev(net);
	if (nRes)
		goto regdev_fail;

	netif_carrier_off(net);
	netif_tx_stop_all_queues(net);

	sdio_claim_host(adapter->func);
	nRes = sdio_enable_func(adapter->func);
	if (nRes < 0) {
		dump_debug("sdio_enable func error = %d", nRes);
		goto sdioen_fail;
	}

	nRes = sdio_claim_irq(adapter->func, adapter_interrupt);
	if (nRes < 0) {
		dump_debug("sdio_claim_irq = %d", nRes);
		goto sdioirq_fail;
	}
	sdio_set_block_size(adapter->func, CMC_BLOCK_SIZE);
	sdio_release_host(adapter->func);

	memset(charName, 0x00, sizeof(charName));
	create_char_name(charName, idx);
	if (misc_register(&uwibro_dev) != 0) {
		pr_debug("adapter_probe: misc_register() failed");
		goto regchar_fail;
	}

	/* Dummy value for "ifconfig up" for 2.6.24 */
	random_ether_addr(node_id);
	memcpy(net->dev_addr, node_id, sizeof(node_id));

	nRes = hw_start(adapter);
	if (nRes) {
		/* Free the resources and stop the driver processing */
		misc_deregister(&uwibro_dev);
		pr_debug("wimax_hw_start failed");
		goto regchar_fail;
	}
	g_pdata->g_cfg->probe_complete = TRUE;

	return 0;

regchar_fail:
	adapter->halted = TRUE;
	wake_up_interruptible(&adapter->send_event);
	wake_up_interruptible(&adapter->receive_event);
	sdio_claim_host(adapter->func);
	sdio_release_irq(adapter->func);
sdioirq_fail:
	sdio_disable_func(adapter->func);
sdioen_fail:
	sdio_release_host(adapter->func);
	unregister_netdev(adapter->net);
regdev_fail:
	sdio_set_drvdata(func, NULL);
	hw_remove(adapter);
hwInit_fail:
	free_netdev(net);
alloceth_fail:
	g_pdata->g_cfg->card_removed = TRUE;
	g_pdata->g_cfg->probe_complete = TRUE;
	g_pdata->power(0);
	return nRes;
}

void adapter_remove(struct sdio_func *func)
{
	struct net_adapter	*adapter = sdio_get_drvdata(func);

	/* remove adapter from adapters array */
	g_adapter = NULL;
	
	wake_up_interruptible(&adapter->send_event);
	wake_up_interruptible(&adapter->receive_event);

	if (!adapter) {
		dump_debug("unregistering non-bound device?");
		return;
	}

	if (adapter->media_state == MEDIA_CONNECTED) {
		netif_stop_queue(adapter->net);
		adapter->media_state = MEDIA_DISCONNECTED;
	}

	if (!adapter->removed)
		hw_stop(adapter);	/* free hw in and out buffer */

	if (adapter->downloading) {
		adapter->removed = TRUE;
		adapter->download_complete = TRUE;
		wake_up_interruptible(&adapter->download_event);
	}

	/* remove control process list */
	control_remove(adapter);

	/*remove hardware interface */
	hw_remove(adapter);

	misc_deregister(&uwibro_dev);
	if (adapter->net){
		unregister_netdev(adapter->net);

	free_netdev(adapter->net);
		}

	g_pdata->g_cfg->card_removed = TRUE;

	return;
}

static int __init adapter_init_module(void)
{
	return platform_driver_register(&wimax_device);
}

static void __exit adapter_deinit_module(void)
{
	platform_driver_unregister(&wimax_device);
}
module_init(adapter_init_module);
module_exit(adapter_deinit_module);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_PARM_DESC(msg_level, "Override default message level");
MODULE_DEVICE_TABLE(sdio, adapter_table);
MODULE_VERSION(WIMAX_DRIVER_VERSION_STRING);
MODULE_LICENSE("GPL");
