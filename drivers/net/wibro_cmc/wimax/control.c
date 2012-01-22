/*
 * control.c send and receive control packet and handle it
 */
#include "headers.h"

u32 control_init(struct net_adapter *adapter)
{
	queue_init_list(adapter->ctl.q_received_ctrl.head);
	spin_lock_init(&adapter->ctl.q_received_ctrl.lock);

	queue_init_list(adapter->ctl.apps.process_list);
	spin_lock_init(&adapter->ctl.apps.lock);

	adapter->ctl.apps.ready = TRUE;

	return STATUS_SUCCESS;
}

void control_remove(struct net_adapter *adapter)
{
	struct buffer_descriptor	*dsc;
	struct process_descriptor	*process;

	/* Free the received control packets queue */
	while (!queue_empty(adapter->ctl.q_received_ctrl.head)) {
		/* queue is not empty */
		dump_debug("Freeing Control Receive Queue");
		dsc = (struct buffer_descriptor *)
			queue_get_head(adapter->ctl.q_received_ctrl.head);
		if (!dsc) {
			dump_debug("Fail...node is null");
			continue;
		}
		queue_remove_head(adapter->ctl.q_received_ctrl.head);
		kfree(dsc->buffer);
		kfree(dsc);
	}

	/* process list */
	if (adapter->ctl.apps.ready) {
		if (!queue_empty(adapter->ctl.apps.process_list)) {
			/* first time gethead needed to get the dsc nodes */
			process = (struct process_descriptor *)
				queue_get_head(adapter->ctl.apps.process_list);
			while (process != NULL) {
				if (process->irp) {
					process->irp = FALSE;
					wake_up_interruptible
						(&process->read_wait);
				}
				process = (struct process_descriptor *)
					process->node.next;
				dump_debug("sangam dbg : waking processes");
			}
			/* delay for the process release */
			msleep(100);
		}
	}
	adapter->ctl.apps.ready = FALSE;
}

/* add received packet to pending list */
static void control_enqueue_buffer(struct net_adapter *adapter,
					void *buffer, u32 length)
{
	struct buffer_descriptor	*dsc;
	struct process_descriptor	*process;
	struct eth_header			hdr;

	/* get the packet type for the process check */
	memcpy(&hdr.type, (unsigned short *)buffer, sizeof(unsigned short));

	/* Queue and wake read only if process exist. */
	process = process_by_type(adapter, hdr.type);
	if (process) {
		dsc = (struct buffer_descriptor *)
			kmalloc(sizeof(struct buffer_descriptor), GFP_ATOMIC);
		if (dsc == NULL) {
			dump_debug("dsc Memory Alloc Failure *****");
			return;
		}
		dsc->buffer = kmalloc((length + (ETHERNET_ADDRESS_LENGTH * 2))
				, GFP_ATOMIC);
		if (dsc->buffer == NULL) {
			kfree(dsc);
			dump_debug("dsc->buffer Memory Alloc Failure *****");
			return;
		}

		/* add ethernet header to control packet */
		memcpy(dsc->buffer, adapter->hw.eth_header,
				(ETHERNET_ADDRESS_LENGTH * 2));
		memcpy(dsc->buffer + (ETHERNET_ADDRESS_LENGTH * 2),
				buffer, length);

		/* fill out descriptor */
		dsc->length = length + (ETHERNET_ADDRESS_LENGTH * 2);
		dsc->type = hdr.type;

		/* add to pending list */
		queue_put_tail(adapter->ctl.q_received_ctrl.head, dsc->node)

		if (process->irp) {
			process->irp = FALSE;
			wake_up_interruptible(&process->read_wait);
		}
	} else
		dump_debug("Waiting process not found skip the packet");
}

/* receive control data */
void control_recv(struct net_adapter *adapter, void *buffer, u32 length)
{

	/* dump rx control frame */
	if (g_pdata->g_cfg->enable_dump_msg == 1)
		dump_buffer("Control Rx", (u8 *)buffer, length);

	/* check halt flag */
	if (adapter->halted)
		return;

	/* not found, add to pending buffers list */
	spin_lock(&adapter->ctl.q_received_ctrl.lock);
	control_enqueue_buffer(adapter, buffer, length);
	spin_unlock(&adapter->ctl.q_received_ctrl.lock);
}

u32 control_send(struct net_adapter *adapter, void *buffer, u32 length)
{
	struct buffer_descriptor	*dsc;
	struct hw_packet_header		*hdr;
	u8				*ptr;


	if ((length + sizeof(struct hw_packet_header)) >= WIMAX_MAX_TOTAL_SIZE)
		return STATUS_RESOURCES;/* changed from SUCCESS return status */

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
	hdr = (struct hw_packet_header *)dsc->buffer;

	ptr += sizeof(struct hw_packet_header);
#ifdef HARDWARE_USE_ALIGN_HEADER
	ptr += 2;
#endif

	memcpy(ptr, buffer + (ETHERNET_ADDRESS_LENGTH * 2),
					length - (ETHERNET_ADDRESS_LENGTH * 2));

	/* add packet header */
	hdr->id0 = 'W';
	hdr->id1 = 'C';
	hdr->length = (u16)length - (ETHERNET_ADDRESS_LENGTH * 2);

	/* set length */
	dsc->length = (u16)length - (ETHERNET_ADDRESS_LENGTH * 2)
					+ sizeof(struct hw_packet_header);
#ifdef HARDWARE_USE_ALIGN_HEADER
	dsc->length += 2;
#endif

	/* dump control packet for debug */
	if (g_pdata->g_cfg->enable_dump_msg == 1)
		dump_buffer("Control Tx", (u8 *)dsc->buffer + 6,
				dsc->length - 6);

	spin_lock(&adapter->hw.q_send.lock);
	queue_put_tail(adapter->hw.q_send.head, dsc->node);
	spin_unlock(&adapter->hw.q_send.lock);

	wake_up_interruptible(&adapter->send_event);

	return STATUS_SUCCESS;
}

struct process_descriptor *process_by_id(struct net_adapter *adapter, u32 id)
{
	struct process_descriptor	*process;

	if (queue_empty(adapter->ctl.apps.process_list)) {
		dump_debug("process_by_id: Empty process list");
		return NULL;
	}

	/* first time gethead needed to get the dsc nodes */
	process = (struct process_descriptor *)
		queue_get_head(adapter->ctl.apps.process_list);
	while (process != NULL)	{
		if (process->id == id)	/* process found */
			return process;
		process = (struct process_descriptor *)process->node.next;
	}
	dump_debug("process_by_id: process not found");

	return NULL;
}

struct process_descriptor
	*process_by_type(struct net_adapter *adapter, u16 type)
{
	struct process_descriptor	*process;

	if (queue_empty(adapter->ctl.apps.process_list)) {
		dump_debug("process_by_type: Empty process list");
		return NULL;
	}

	/* first time gethead needed to get the dsc nodes */
	process = (struct process_descriptor *)
		queue_get_head(adapter->ctl.apps.process_list);
	while (process != NULL)	{
		if (process->type == type)	/* process found */
			return process;
		process = (struct process_descriptor *)process->node.next;
	}
	dump_debug("process_by_type: process not found");

	return NULL;
}

void remove_process(struct net_adapter *adapter, u32 id)
{
	struct process_descriptor	*curElem;
	struct process_descriptor	*prevElem = NULL;

	if (queue_empty(adapter->ctl.apps.process_list))
		return;

	/* first time get head needed to get the dsc nodes */
	curElem = (struct process_descriptor *)
		queue_get_head(adapter->ctl.apps.process_list);

	for ( ; curElem != NULL;
			prevElem = curElem,
			curElem  =
			(struct process_descriptor *)curElem->node.next) {
		if (curElem->id == id) {	/* process found */
			if (prevElem == NULL) {
				/* only one node present */
				(adapter->ctl.apps.process_list).next =
					((struct list_head *)curElem)->next;
				if (!((adapter->ctl.apps.process_list).next)) {
					/*rechain list pointer to next link*/
					dump_debug("sangam dbg first	\
						and only process delete");
					(adapter->ctl.apps.process_list).prev =
						NULL;
				}
			} else if (((struct list_head *)curElem)->next ==
					NULL) {
				/* last node */
				dump_debug("sangam dbg only last packet");
				((struct list_head *)prevElem)->next = NULL;
				(adapter->ctl.apps.process_list).prev =
					(struct list_head *)(prevElem);
			} else {
				/* middle node */
				dump_debug("sangam dbg middle node");
				((struct list_head *)prevElem)->next =
					((struct list_head *)curElem)->next;
			}

			kfree(curElem);
			break;
		}
	}
}

/* find buffer by buffer type */
struct buffer_descriptor
*buffer_by_type(struct list_head ListHead, u16 type)
{
	struct buffer_descriptor	*dsc;

	if (queue_empty(ListHead))
		return NULL;

	/* first time gethead needed to get the dsc nodes */
	dsc = (struct buffer_descriptor *)queue_get_head(ListHead);
	while (dsc != NULL) {
		if (dsc->type == type)	/* process found */
			return dsc;
		dsc = (struct buffer_descriptor *)dsc->node.next;
	}

	return NULL;
}

void dump_buffer(const char *desc, u8 *buffer, u32 len)
{
	char	print_buf[256] = {0};
	char	chr[8] = {0};
	int	i;

	/* dump packets */
	u8  *b = buffer;
	dump_debug("%s (%d) =>", desc, len);

	for (i = 0; i < len; i++) {
		sprintf(chr, " %02x", b[i]);
		strcat(print_buf, chr);
		if (((i + 1) != len) && (i % 16 == 15)) {
			dump_debug(print_buf);
			memset(print_buf, 0x0, 256);
		}
	}
	dump_debug(print_buf);
}
