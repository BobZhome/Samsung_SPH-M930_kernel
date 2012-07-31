/*
 * send.c
 *
 *
 *
 */
#include "headers.h"
#include "download.h"

//extern int s3c_bat_use_wimax(int onoff);

u32 sd_send(struct net_adapter *adapter, u8 *buffer, u32 len)
{
	int	nRet = 0;

	int nWriteIdx;

	int err;

	len += (len & 1) ? 1 : 0;

	if (adapter->halted || adapter->removed) {
		dump_debug("Halted Already");
		return STATUS_UNSUCCESSFUL;
	}
	sdio_claim_host(adapter->func);
	hwSdioWriteBankIndex(adapter, &nWriteIdx, &nRet);

	if (nRet || (nWriteIdx < 0)) {
		dump_debug("sd_send : error occurred during	\
				fetch bank index!! nRet = %d, nWriteIdx = %d",
				nRet, nWriteIdx);
		goto cmc_732_reset;
	}
	sdio_writeb(adapter->func, (nWriteIdx + 1) % 15, SDIO_H2C_WP_REG, NULL);
	nRet = sdio_memcpy_toio(adapter->func,
			SDIO_TX_BANK_ADDR+(SDIO_BANK_SIZE * nWriteIdx)+4,
			buffer, len);

	if (nRet < 0){
		dump_debug("sd_send : error in sending packet!! nRet = %d",
				nRet);
		goto cmc_732_reset;
		}

	nRet = sdio_memcpy_toio(adapter->func,
			SDIO_TX_BANK_ADDR + (SDIO_BANK_SIZE * nWriteIdx),
			&len, 4);
	if (nRet < 0){
		dump_debug("sd_send : error in writing bank length!! nRet = %d",
				nRet);
		goto cmc_732_reset;
		}
	sdio_release_host(adapter->func);

	return nRet;

cmc_732_reset:			/*Restart CMC732 SDIO*/

			nRet = 0;

			sdio_release_irq(adapter->func);
			err = cmc732_sdio_reset_comm(adapter->func->card);
			if (err < 0) {
				dump_debug("cmc732_sdio_reset_comm error = %d", err);
				nRet = err;
				}
			err = sdio_enable_func(adapter->func);
			if (err < 0) {
				dump_debug("sdio_enable func error = %d", err);
				nRet = err;
				}
			err = sdio_claim_irq(adapter->func, adapter_interrupt);
			if (err < 0) {
				dump_debug("sdio_claim_irq = %d", err);
				nRet = err;
				}
			err = sdio_set_block_size(adapter->func, 512);
			if (err < 0) {
				dump_debug("sdio_claim_irq = %d", err);
				nRet = err;
				}

			/*Now, retry the block read again.
			* the reset does not seem to succeed 
			* without this block read below..
			* Most likely, this has something to do
			* with the host controller state because
			* we get a ADMA error for the below
			* attempt on C110 BSP. But from there on,
			* everything works just fine*/
			
			sdio_memcpy_toio(adapter->func,
			SDIO_TX_BANK_ADDR+(SDIO_BANK_SIZE )+4,
			buffer, len);
			sdio_release_host(adapter->func);
			return nRet;


}


/* get MAC address from device */
int hw_get_mac_address(void *data)
{
	struct net_adapter *adapter = (struct net_adapter *)data;
	struct hw_private_packet	req;
	int				nResult = 0;
	int				retry = 4;

	req.id0 = 'W';
	req.id1 = 'P';
	req.code = HwCodeMacRequest;
	req.value = 0;

	dump_debug("wait for SDIO ready..");
	msleep(1700); /*
					*  IMPORTANT! wait for cmc720 can
					* handle mac req packet
					*/
	do {
		if (!g_adapter)
			break;
		nResult = sd_send(adapter, (u8 *)&req,
				sizeof(struct hw_private_packet));

		if (nResult != STATUS_SUCCESS){
			dump_debug("hw_get_mac_address: sd_send fail!!");
			break;
			}
		msleep(300);

		}
	while ((!adapter->mac_ready)&&(g_adapter)&&(retry--));

	do_exit(0);

	return 0;
}

u32 hw_send_data(struct net_adapter *adapter, void *buffer , u32 length)
{
	struct buffer_descriptor	*dsc;
	struct hw_packet_header		*hdr;
	struct net_device		*net = adapter->net;
	u8				*ptr;

	dsc = (struct buffer_descriptor *)
		kmalloc(sizeof(struct buffer_descriptor), GFP_ATOMIC);
	if (dsc == NULL)
		return STATUS_RESOURCES;

	dsc->buffer = kmalloc(BUFFER_DATA_SIZE, GFP_ATOMIC);
	if (dsc->buffer == NULL) {
		kfree(dsc);
		return STATUS_RESOURCES;
	}

	ptr = dsc->buffer;

	/* shift data pointer */
	ptr += sizeof(struct hw_packet_header);
#ifdef HARDWARE_USE_ALIGN_HEADER
	ptr += 2;
#endif
	hdr = (struct hw_packet_header *)dsc->buffer;

	length -= (ETHERNET_ADDRESS_LENGTH * 2);
	buffer += (ETHERNET_ADDRESS_LENGTH * 2);

	memcpy(ptr, buffer, length);

	hdr->id0 = 'W';
	hdr->id1 = 'D';
	hdr->length = (u16)length;

	dsc->length = length + sizeof(struct hw_packet_header);
#ifdef HARDWARE_USE_ALIGN_HEADER
	dsc->length += 2;
#endif

	/* add statistics */
	adapter->netstats.tx_packets++;
	adapter->netstats.tx_bytes += dsc->length;

	/*EC16 hiyo remove spin lock recursion in iperf UL TP*/
	//spin_lock(&adapter->hw.q_send.lock);
	queue_put_tail(adapter->hw.q_send.head, dsc->node);
	//spin_unlock(&adapter->hw.q_send.lock);

	wake_up_interruptible(&adapter->send_event);
	if (!netif_running(net))
		dump_debug("!netif_running");

	return STATUS_SUCCESS;
}

u32 sd_send_data(struct net_adapter *adapter,
		struct buffer_descriptor *dsc)
{

#ifdef HARDWARE_USE_ALIGN_HEADER
	if (dsc->length > SDIO_MAX_BYTE_SIZE)
		dsc->length = (dsc->length + SDIO_MAX_BYTE_SIZE) &
			~(SDIO_MAX_BYTE_SIZE);
#endif

	if (adapter->halted) {
		dump_debug("Halted Already");
		return STATUS_UNSUCCESSFUL;
	}

	return sd_send(adapter, dsc->buffer, dsc->length);

}

/* Return packet for packet buffer freeing */
void hw_return_packet(struct net_adapter *adapter, u16 type)
{
	struct buffer_descriptor	*curElem;
	struct buffer_descriptor	*prevElem = NULL;

	if (queue_empty(adapter->ctl.q_received_ctrl.head))
		return;

	/* first time get head needed to get the dsc nodes */
	curElem = (struct buffer_descriptor *)
		queue_get_head(adapter->ctl.q_received_ctrl.head);

	for ( ; curElem != NULL; prevElem = curElem,
			curElem  = (struct buffer_descriptor *)
						curElem->node.next) {
		if (curElem->type == type) {
			/* process found*/
			if (prevElem == NULL) {
				/* First node or only one node to delete */
				(adapter->ctl.q_received_ctrl.head).next =
					((struct list_head *)curElem)->next;
				if (!((adapter->ctl.q_received_ctrl.head).
							next)) {
					/*
					* rechain list pointer to next link
					* if the list pointer is null, null
					*   out the reverse link
					*/
					(adapter->ctl.q_received_ctrl.head).prev
						= NULL;
				}
			} else if (((struct list_head *)curElem)->next ==
					NULL) {
				/* last node */
				((struct list_head *)prevElem)->next = NULL;
				(adapter->ctl.q_received_ctrl.head).prev =
					(struct list_head *)(&prevElem);
			} else {
				/* middle node */
				((struct list_head *)prevElem)->next =
					((struct list_head *)curElem)->next;
			}

			kfree(curElem->buffer);
			kfree(curElem);
			break;
		}
	}
}

int hw_device_wakeup(struct net_adapter *adapter)
{

	int rc = 0;
	int ret = 0;
	g_pdata->wakeup_assert(1);

	while(!g_pdata->is_modem_awake())
	{
		if(rc==0)
			dump_debug("hw_device_wakeup (CON0 status): waiting for modem awake");
		rc++;
		if (rc > WAKEUP_MAX_TRY)
			{
				dump_debug("hw_device_wakeup (CON0 status): modem wake up time out!!");
				ret = -EIO;
				break;
			}

		/*Toggle modem wake up pin*/

		msleep(WAKEUP_TIMEOUT/2);
		g_pdata->wakeup_assert(0);
		msleep(WAKEUP_TIMEOUT/2);
		g_pdata->wakeup_assert(1);
	}
	if ((rc > 0)&&(rc <= WAKEUP_MAX_TRY))
		dump_debug("hw_device_wakeup (CON0 status): modem awake");
	g_pdata->wakeup_assert(0);
	return ret;
}


int cmc732_send_thread(void *data)
{
	struct net_adapter *adapter = (struct net_adapter *)data;
	struct buffer_descriptor	*dsc;
	int				nRet = 0;
	bool	reset_modem = FALSE;

	do {
			wait_event_interruptible(adapter->send_event,
					(!queue_empty(adapter->hw.q_send.head))
					|| (!g_adapter) || adapter->halted);

			if ((!g_adapter) || adapter->halted)
				break;


			if(hw_device_wakeup(adapter)){
				reset_modem = TRUE;
				break;
				}

			spin_lock(&adapter->hw.q_send.lock);
			dsc = (struct buffer_descriptor *)
				queue_get_head(adapter->hw.q_send.head);
			queue_remove_head(adapter->hw.q_send.head);
			spin_unlock(&adapter->hw.q_send.lock);

			if (!dsc) {
				dump_debug("Fail...node is null");
				continue;
				}
			nRet = sd_send_data(adapter, dsc);
			kfree(dsc->buffer);
			kfree(dsc);
			if (nRet != STATUS_SUCCESS) {
				dump_debug("SendData Fail******");
				reset_modem = TRUE;
				++adapter->XmitErr;
				break;
			}
		}
	while (g_adapter);

	dump_debug("cmc732_send_thread exiting");

	adapter->halted = TRUE;

	if(reset_modem)
		g_pdata->power(0);

	do_exit(0);

	return 0;
}
