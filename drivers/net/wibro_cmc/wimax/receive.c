/*
 * receive.c
 *
 * handle download packet, private cmd and control/data packet
 */
#include "headers.h"
#include "firmware.h"
#include "download.h"

//extern int s3c_bat_use_wimax(int onoff);

void process_indicate_packet(struct net_adapter *adapter, u8 * buffer)
{
	struct wimax_msg_header	*packet;
	char			*tmp_byte;

	packet = (struct wimax_msg_header *)buffer;

	if (packet->type == be16_to_cpu(ETHERTYPE_DL)) {
		switch (be16_to_cpu(packet->id)) {
		case MSG_DRIVER_OK_RESP:
			dump_debug("process_indicate_packet:	\
					MSG_DRIVER_OK_RESP");
			send_image_info_packet(adapter, MSG_IMAGE_INFO_REQ);
			break;
		case MSG_IMAGE_INFO_RESP:
			dump_debug("process_indicate_packet:	\
					MSG_IMAGE_INFO_RESP");
			send_image_data_packet(adapter, MSG_IMAGE_DATA_REQ);
			break;
		case MSG_IMAGE_DATA_RESP:
			if (g_wimax_image.offset == g_wimax_image.size) {
				dump_debug("process_indicate_packet:	\
						Image Download Complete");
				send_cmd_packet(adapter, MSG_RUN_REQ);
			} else {
				send_image_data_packet(adapter,
						MSG_IMAGE_DATA_REQ);
			}
			break;
		case MSG_RUN_RESP:
			tmp_byte = (char *)(buffer +
					sizeof(struct wimax_msg_header));

			if (*tmp_byte == 0x01) {
				dump_debug("process_indicate_packet:	\
						MSG_RUN_RESP");

				adapter->download_complete = TRUE;
				wake_up_interruptible(&adapter->download_event);

				if (g_pdata->g_cfg->wimax_mode == SDIO_MODE
						|| g_pdata->g_cfg->wimax_mode == DM_MODE
						|| g_pdata->g_cfg->wimax_mode == USB_MODE
						|| g_pdata->g_cfg->wimax_mode	==
						USIM_RELAY_MODE) {

					adapter->mac_task = kthread_create(
					hw_get_mac_address,	adapter, "%s",
					"mac_request_thread");
					if (adapter->mac_task)
						wake_up_process(
							adapter->mac_task);
				} else if (g_pdata->g_cfg->wimax_mode == WTM_MODE) {
					adapter->wtm_task = kthread_create(
					con0_poll_thread,	adapter, "%s",
					"wimax_con0_poll_thread");
					if (adapter->wtm_task)
						wake_up_process(
							adapter->wtm_task);
				}
			}
			break;
		default:
			dump_debug("process_indicate_packet: Unkown type");
			break;
		}
	} else
		dump_debug("process_indicate_packet - is not download pakcet");
}

u32 process_private_cmd(struct net_adapter *adapter, void *buffer)
{
	struct hw_private_packet	*cmd;
	u8				*bufp;
	int					ret;

	cmd = (struct hw_private_packet *)buffer;

	switch (cmd->code) {
	case HwCodeMacResponse: {
		u8 mac_addr[ETHERNET_ADDRESS_LENGTH];
		bufp = (u8 *)buffer;

		/* processing for mac_req request */
		dump_debug("MAC address = %02x:%02x:%02x:%02x:%02x:%02x",
				bufp[3], bufp[4], bufp[5],
				bufp[6], bufp[7], bufp[8]);
		memcpy(mac_addr, bufp + 3,
				ETHERNET_ADDRESS_LENGTH);

		/* create ethernet header */
		memcpy(adapter->hw.eth_header,
				mac_addr, ETHERNET_ADDRESS_LENGTH);
		memcpy(adapter->hw.eth_header + ETHERNET_ADDRESS_LENGTH,
				mac_addr, ETHERNET_ADDRESS_LENGTH);
		adapter->hw.eth_header[(ETHERNET_ADDRESS_LENGTH * 2) - 1] += 1;

		memcpy(adapter->net->dev_addr, bufp + 3,
				ETHERNET_ADDRESS_LENGTH);
		adapter->mac_ready = TRUE;

		ret = sizeof(struct hw_private_packet)
			+ ETHERNET_ADDRESS_LENGTH - sizeof(u8);
		return ret;
	}
	case HwCodeLinkIndication: {
		if ((cmd->value == HW_PROT_VALUE_LINK_DOWN)
			&& (adapter->media_state != MEDIA_DISCONNECTED)) {
			dump_debug("LINK_DOWN_INDICATION");

			/* set values */
			adapter->media_state = MEDIA_DISCONNECTED;

			/* indicate link down */
			netif_stop_queue(adapter->net);
			netif_carrier_off(adapter->net);
		} else if ((cmd->value == HW_PROT_VALUE_LINK_UP)
			&& (adapter->media_state != MEDIA_CONNECTED)) {
			dump_debug("LINK_UP_INDICATION");

			/* set values */
			adapter->media_state = MEDIA_CONNECTED;
			adapter->net->mtu = WIMAX_MTU_SIZE;

			/* indicate link up */
			netif_start_queue(adapter->net);
			netif_carrier_on(adapter->net);
		}
		break;
	}
	case HwCodeHaltedIndication: {
		dump_debug("Device is about to reset, stop driver");
		adapter->halted = TRUE;
		break;
	}
	case HwCodeRxReadyIndication: {
		dump_debug("Device RxReady");
		/*
		* to start the data packet send
		* queue again after stopping in xmit
		*/
		if (adapter->media_state == MEDIA_CONNECTED)
			netif_wake_queue(adapter->net);
		break;
	}
	case HwCodeIdleNtfy: {
		/* set idle / vi */
		dump_debug("process_private_cmd: HwCodeIdleNtfy");
//		s3c_bat_use_wimax(0);
		break;
	}
	case HwCodeWakeUpNtfy: {
		/*
		* IMPORTANT!! at least 4 sec
		* is required after modem waked up
		*/
		wake_lock_timeout(&g_pdata->g_cfg->wimax_wake_lock, 4 * HZ);

		dump_debug("process_private_cmd: HwCodeWakeUpNtfy");
//		s3c_bat_use_wimax(1);
		break;
	}
	default:
		dump_debug("Command = %04x", cmd->code);
		break;
	}

	return sizeof(struct hw_private_packet);
}


void adapter_sdio_rx_packet(struct net_adapter *adapter, int len)
{
	struct hw_packet_header	*hdr;
	int							rlen;
	u32			type;
	u8						*ofs;
	struct sk_buff				*rx_skb;

	rlen = len;
	ofs = (u8 *)adapter->hw.receive_buffer;

	while (rlen > 0) {
		hdr = (struct hw_packet_header *)ofs;
		type = HwPktTypeNone;

		/* "WD", "WC", "WP" or "WE" */
		if (unlikely(hdr->id0 != 'W')) {
			if (rlen > 4) {
				dump_debug("Wrong packet	\
				ID (%02x %02x)", hdr->id0, hdr->id1);
				dump_buffer("Wrong packet",
						(u8 *)ofs, rlen);
			}
			/* skip rest of packets */
			break;
		}

		/* check packet type */
		switch (hdr->id1) {
		case 'P': {
			u32 l = 0;
			type = HwPktTypePrivate;

			/* process packet */
			l = process_private_cmd(adapter, ofs);

			/* shift */
			ofs += l;
			rlen -= l;

			/* process next packet */
			continue;
		}
		case 'C':
			type = HwPktTypeControl;
			break;
		case 'D':
			type = HwPktTypeData;
			break;
		case 'E':
			/* skip rest of buffer */
			break;
		default:
			dump_debug("hwParseReceivedData :	\
					Wrong packet ID [%02x %02x]",
						hdr->id0, hdr->id1);
			/* skip rest of buffer */
			break;
		}

		if (type == HwPktTypeNone)
			break;

		if (likely(!adapter->downloading)) {
			if (unlikely(hdr->length > WIMAX_MAX_TOTAL_SIZE
					|| ((hdr->length +
				sizeof(struct hw_packet_header)) > rlen))) {
				dump_debug("Packet length is	\
						too big (%d)", hdr->length);
				/* skip rest of packets */
				break;
			}
		}

		/* change offset */
		ofs += sizeof(struct hw_packet_header);
		rlen -= sizeof(struct hw_packet_header);

		/* process download packet, data and control packet */
		if (likely(!adapter->downloading)) {
#ifdef HARDWARE_USE_ALIGN_HEADER
			ofs += 2;
			rlen -= 2;
#endif

			if (unlikely(type == HwPktTypeControl))
				control_recv(adapter, (u8 *)ofs,
							hdr->length);
			else {
				if (hdr->length > BUFFER_DATA_SIZE) {
					dump_debug("Data packet too large");
					adapter->netstats.rx_dropped++;
					break;
				}

				rx_skb = dev_alloc_skb(hdr->length +
						(ETHERNET_ADDRESS_LENGTH * 2));
				if (!rx_skb) {
					dump_debug("MEMORY PINCH:	\
						unable to allocate skb");
					break;
					}
				skb_reserve(rx_skb,
						(ETHERNET_ADDRESS_LENGTH * 2));

				memcpy(skb_push(rx_skb,
						(ETHERNET_ADDRESS_LENGTH * 2)),
						adapter->hw.eth_header,
						(ETHERNET_ADDRESS_LENGTH * 2));

				memcpy(skb_put(rx_skb, hdr->length),
							(u8 *)ofs,
							hdr->length);

				rx_skb->dev = adapter->net;
				rx_skb->protocol =
					eth_type_trans(rx_skb, adapter->net);
				rx_skb->ip_summed = CHECKSUM_UNNECESSARY;

				if (netif_rx(rx_skb) == NET_RX_DROP) {
					dump_debug("packet dropped!");
					adapter->netstats.rx_dropped++;
				}
				adapter->netstats.rx_packets++;
				adapter->netstats.rx_bytes +=
					(hdr->length +
					 (ETHERNET_ADDRESS_LENGTH * 2));

			}
		} else {
			hdr->length -= sizeof(struct hw_packet_header);
			process_indicate_packet(adapter, ofs);
		}
		/*
		* If the packet is unreasonably long,
		* quietly drop it rather than
		* kernel panicing by calling skb_put.
		*
		* shift
		*/
		ofs += hdr->length;
		rlen -= hdr->length;
	}

	return;
}

int cmc732_receive_thread(void *data)
{
	struct net_adapter *adapter = (struct net_adapter *)data;
	int							err = 1;
	int							nReadIdx;
	u32						len = 0;
	u32						t_len;
	u32						t_index;
	u32						t_size;
	u8						*t_buff;

	do {

		wait_event_interruptible(adapter->receive_event,
				adapter->rx_data_available
				|| (!g_adapter) || adapter->halted);
		adapter->rx_data_available = FALSE;
		if ((!g_adapter) || adapter->halted)
			break;

		sdio_claim_host(adapter->func);
		hwSdioReadBankIndex(adapter, &nReadIdx, &err);

		if (err) {
			dump_debug("adapter_sdio_rx_packet :	\
				error occurred during fetch bank	\
				index!! err = %d, nReadIdx = %d",	\
				err, nReadIdx);
			sdio_release_host(adapter->func);
			continue;
		}
		if (nReadIdx < 0) {
			sdio_release_host(adapter->func);
			continue;
			}
		hwSdioReadCounter(adapter, &len, &nReadIdx, &err);
		if (unlikely(err || !len)) {
			dump_debug("adapter_sdio_rx_packet :	\
					error in reading bank length!!	\
					err = %d, len = %d", err, len);
			sdio_release_host(adapter->func);
			continue;
		}

		if (unlikely(len > SDIO_BUFFER_SIZE)) {
			dump_debug("ERROR RECV length (%d) >	\
					SDIO_BUFFER_SIZE", len);
			len = SDIO_BUFFER_SIZE;
		}

		sdio_writeb(adapter->func, (nReadIdx + 1) % 16,
				SDIO_C2H_RP_REG, NULL);

		/*Read Data in Single Blocks*/

		t_len = len;
		t_index = (SDIO_RX_BANK_ADDR + (SDIO_BANK_SIZE * nReadIdx) + 4);
		t_buff = (u8 *)adapter->hw.receive_buffer;
		
		while (t_len){
			t_size = (t_len > CMC_BLOCK_SIZE) ? (CMC_BLOCK_SIZE) : t_len;
			err = sdio_memcpy_fromio(adapter->func, (void *)t_buff, t_index, t_size);
			t_len -= t_size;
			t_buff += t_size;
			t_index += t_size;
			}
		
		if (unlikely(!len))
			dump_debug("Packet length information zero!");
		if (unlikely(err)) {
			dump_debug("adapter_sdio_rx_packet :	\
				error in receiving packet!	\
				errt = %d, len = %d", err, len);


			/*Restart CMC732 SDIO*/

			sdio_release_irq(adapter->func);
			err = cmc732_sdio_reset_comm(adapter->func->card);
			if (err < 0) {
				dump_debug("cmc732_sdio_reset_comm error = %d", err);
				}
			err = sdio_enable_func(adapter->func);
			if (err < 0) {
				dump_debug("sdio_enable func error = %d", err);
				}
			err = sdio_claim_irq(adapter->func, adapter_interrupt);
			if (err < 0) {
				dump_debug("sdio_claim_irq = %d", err);
				}
			err = sdio_set_block_size(adapter->func, 512);
			if (err < 0) {
				dump_debug("sdio_claim_irq = %d", err);
				}

			/*Now, retry the block read again.
			* the reset does not seem to succeed 
			* without this block read below..
			* Most likely, this has something to do
			* with the host controller state because
			* we get a ADMA error for the below
			* attempt on C110 BSP. But from there on,
			* everything works just fine*/
			
			err = sdio_memcpy_fromio(adapter->func,
					adapter->hw.receive_buffer,
					(SDIO_RX_BANK_ADDR + (SDIO_BANK_SIZE * nReadIdx) + 4),
					len);
			if (err) {
				dump_debug("adapter_sdio_rx_packet :	\
				drop	the packet  errt = %d, len = %d", err, len);

				sdio_release_host(adapter->func);
	
				adapter->netstats.rx_dropped++;
	
				continue;
				}

		}
		sdio_release_host(adapter->func);
		adapter_sdio_rx_packet(adapter, len);
	} while (g_adapter);

	adapter->halted = TRUE;

	dump_debug("cmc732_receive_thread exiting");

	do_exit(0);

return 0;
}
