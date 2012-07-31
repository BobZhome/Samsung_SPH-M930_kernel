/*
 * hardware.c
 *
 * gpio control functions (power on/off, init/deinit gpios)
 *
 */
#include "headers.h"
#include "download.h"

static void wimax_hostwake_task(unsigned long data)
{

	wake_lock_timeout(&g_pdata->g_cfg->wimax_wake_lock, 1 * HZ);

   if(g_pdata->is_ap_awake() == 0)
	   g_pdata->signal_ap_active(1);
  	((struct net_adapter *)data)->rx_data_available = TRUE;
	wake_up_interruptible(&((struct net_adapter *)data)->receive_event);

}

static irqreturn_t wimax_hostwake_isr(int irq, void *dev)
{
	struct net_adapter *adapter = (struct net_adapter *)dev;
	tasklet_schedule(&adapter->hostwake_task);
	return IRQ_HANDLED;
}
static int cmc732_setup_wake_irq(struct net_adapter *adapter)
{
	int rc = -EIO;
	int irq;

	tasklet_init(&adapter->hostwake_task, wimax_hostwake_task, (unsigned long)adapter);

	rc = gpio_request(g_pdata->wimax_int, "gpio_wimax_int");
	if (rc < 0) {
		dump_debug("%s: gpio %d request failed (%d)\n",
			__func__, g_pdata->wimax_int, rc);
		return rc;
	}

	rc = gpio_direction_input(g_pdata->wimax_int);
	if (rc < 0) {
		dump_debug("%s: failed to set gpio %d as input (%d)\n",
			__func__, g_pdata->wimax_int, rc);
		goto err_gpio_direction_input;
	}

	irq = gpio_to_irq(g_pdata->wimax_int);

	rc = request_irq(irq, wimax_hostwake_isr, IRQF_TRIGGER_FALLING,
			 "wimax_int", adapter);
	if (rc < 0) {
		dump_debug("%s: request_irq(%d) failed for gpio %d (%d)\n",
			__func__, irq,
			g_pdata->wimax_int, rc);
		goto err_request_irq;
	}

	rc = enable_irq_wake(irq);

	if (rc < 0) {
		dump_debug("%s: enable_irq_wake(%d) failed for gpio %d (%d)\n",
			__func__, irq,
			g_pdata->wimax_int, rc);
		goto err_enable_irq_wake;
	}

	adapter->wake_irq = irq;

	goto done;
err_enable_irq_wake:
	free_irq(irq, adapter);
err_request_irq:
err_gpio_direction_input:
	gpio_free(g_pdata->wimax_int);
	tasklet_kill(&adapter->hostwake_task);
done:
	return rc;

}
void cmc732_release_wake_irq(struct net_adapter *adapter)
{
	if (adapter->wake_irq) {
		disable_irq_wake(adapter->wake_irq);
		free_irq(adapter->wake_irq, adapter);
		gpio_free(g_pdata->wimax_int);
		tasklet_kill(&adapter->hostwake_task);
		}
}

int hw_start(struct net_adapter *adapter)
{
	if (load_wimax_image(g_pdata->g_cfg->wimax_mode))
		return STATUS_UNSUCCESSFUL;

	adapter->download_complete = FALSE;

	adapter->rx_task = kthread_create(
					cmc732_receive_thread,	adapter, "%s",
					"cmc732_receive_thread");

	adapter->tx_task = kthread_create(
					cmc732_send_thread,	adapter, "%s",
					"cmc732_send_thread");

	init_waitqueue_head(&adapter->receive_event);
	init_waitqueue_head(&adapter->send_event);

	if (adapter->rx_task && adapter->tx_task) {
		wake_up_process(adapter->rx_task);
		wake_up_process(adapter->tx_task);
	} else {
		dump_debug("Unable to create send-receive threads");
		return STATUS_UNSUCCESSFUL;
	}

	if (adapter->downloading) {

		/*This command initiates firmware download by the bootloader*/
		send_cmd_packet(adapter, MSG_DRIVER_OK_REQ);

		switch (wait_event_interruptible_timeout(
				adapter->download_event,
				(adapter->download_complete == TRUE),
				HZ*FWDOWNLOAD_TIMEOUT)) {
		case 0:
			/* timeout */
			dump_debug("Error hw_start : \
					F/W Download timeout failed");

			return STATUS_UNSUCCESSFUL;
		case -ERESTARTSYS:
			/* Interrupted by signal */
			dump_debug("Error hw_start :  -ERESTARTSYS retry");
			return STATUS_UNSUCCESSFUL;
		default:
			/* normal condition check */
			if (adapter->removed == TRUE ||
					adapter->halted == TRUE) {
				dump_debug("Error hw_start :	\
						F/W Download surprise removed");
				return STATUS_UNSUCCESSFUL;
			}
			dump_debug("hw_start :  F/W Download Complete");


			/*Setup IRQ so that wimax modem can wakeup
			AP from sleep when it has some packets to give*/

			if (cmc732_setup_wake_irq(adapter) < 0)
				dump_debug("hw_start :"
						"Error setting up wimax_int");

			break;
		}
		adapter->downloading = FALSE;
	}

	return STATUS_SUCCESS;
}

int hw_stop(struct net_adapter *adapter)
{
	adapter->halted = TRUE;

	/*Remove wakeup  interrupt*/
	cmc732_release_wake_irq(adapter);

	/* Stop Sdio Interface */
	sdio_claim_host(adapter->func);
	sdio_release_irq(adapter->func);
	sdio_disable_func(adapter->func);
	sdio_release_host(adapter->func);

	return STATUS_SUCCESS;
}

int hw_init(struct net_adapter *adapter)
{

	/* set g_pdata->wimax_wakeup & g_pdata->wimax_if_mode0 */
	g_pdata->set_mode();

	/* initilize hardware info structure */
	memset(&adapter->hw, 0x0, sizeof(struct hardware_info));

	adapter->hw.receive_buffer = kmalloc(SDIO_BANK_SIZE, GFP_KERNEL);
		if (adapter->hw.receive_buffer == NULL) {
			dump_debug("kmalloc fail!!");
			return -ENOMEM;
		}
	memset(adapter->hw.receive_buffer, 0x0, SDIO_BANK_SIZE);

	/* For sending data and control packets */
	queue_init_list(adapter->hw.q_send.head);
	spin_lock_init(&adapter->hw.q_send.lock);

	init_waitqueue_head(&adapter->download_event);

	return STATUS_SUCCESS;
}

void hw_remove(struct net_adapter *adapter)
{
	struct buffer_descriptor *dsc;

	/* Free the pending data packets and control packets */
	while (!queue_empty(adapter->hw.q_send.head)) {
		dump_debug("Freeing q_send");
		dsc = (struct buffer_descriptor *)
			queue_get_head(adapter->hw.q_send.head);
		if (!dsc) {
			dump_debug("Fail...node is null");
			continue;
		}
		queue_remove_head(adapter->hw.q_send.head);
		kfree(dsc->buffer);
		kfree(dsc);
	}

	if (adapter->hw.receive_buffer != NULL)
		kfree(adapter->hw.receive_buffer);

}

int con0_poll_thread(void *data)
{
	struct net_adapter *adapter = (struct net_adapter *)data;
	int prev_val = 0;
	int curr_val = 0;

	while(g_adapter&&(!adapter->halted)){
		curr_val = g_pdata->is_modem_awake();
		if ((prev_val&&(!curr_val))||(curr_val == GPIO_LEVEL_LOW)){
			g_pdata->restore_uart_path();
			break;
			}
		prev_val = curr_val;
//		dump_debug("con0_poll_thread");
		msleep(40);
		}
	do_exit(0);
	return 0;
}
