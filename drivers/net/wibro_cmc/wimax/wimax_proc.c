/**
* wimax_proc.c
*
* App read wimax mode and power state by proc file system.
* Some proc file used for debug.
*/
#include "headers.h"
#include "wimax_proc.h"
#include "wimax_i2c.h"

#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/delay.h>

/* proc entry */
static struct proc_dir_entry *g_proc_wmxmode_dir;
static struct proc_dir_entry *g_proc_wmxuart;
static struct proc_dir_entry *g_proc_eeprom;
static struct proc_dir_entry *g_proc_onoff;
static struct proc_dir_entry *g_proc_dump;
static struct proc_dir_entry *g_proc_sleepmode;

static int wimax_proc_mode(char *buf, char **start, off_t offset,
		int count, int *eof, void *data)
{
	int	len;

	len = sprintf(buf, "%c\n", (g_pdata->g_cfg->wimax_mode));
	if (len > 0)
		*eof = 1;

	return len;
}

static int wimax_proc_power(char *buf, char **start, off_t offset,
		int count, int *eof, void *data)
{
	return count;
}

static int proc_read_wmxuart(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	g_pdata->display_gpios();

	return 0;
}

static int proc_write_wmxuart(struct file *foke, const char *buffer,
		unsigned long count, void *data)
{
	if (buffer == NULL)
		return 0;

	if (buffer[0] == '0')
		g_pdata->switch_uart_ap();
	else if (buffer[0] == '1')
		g_pdata->switch_uart_wimax();

	return count - 1;
}

static int proc_read_eeprom(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	dump_debug("Write EEPROM!!");

	eeprom_write_boot();
	eeprom_write_rev();

	return 0;
}

static int proc_write_eeprom(struct file *foke, const char *buffer,
		unsigned long count, void *data)
{
	if (count != 5)
		return count;

	if (strncmp(buffer, "wb00", 4) == 0) {
		dump_debug("Write EEPROM!!");
		eeprom_write_boot();
	} else if (strncmp(buffer, "rb00", 4) == 0) {
		dump_debug("Read Boot!!");
		eeprom_read_boot();
	} else if (strncmp(buffer, "re00", 4) == 0) {
		dump_debug("Read EEPROM!!");
		eeprom_read_all();
	} else if (strncmp(buffer, "ee00", 4) == 0) {
		dump_debug("Erase EEPROM!!");
		eeprom_erase_all();
	} else if (strncmp(buffer, "rcal", 4) == 0) {
		dump_debug("Check Cal!!");
		eeprom_check_cal();
	} else if (strncmp(buffer, "ecer", 4) == 0) {
		dump_debug("Erase Cert!!");
		eeprom_erase_cert();
	} else if (strncmp(buffer, "rcer", 4) == 0) {
		dump_debug("Check Cert!!");
		eeprom_check_cert();
	} else if (strncmp(buffer, "wrev", 4) == 0) {
		dump_debug("Write Rev!!");
		eeprom_write_rev();
	} else if (strncmp(buffer, "ons0", 4) == 0) {
		dump_debug("Power On - SDIO MODE!!");
		g_pdata->power(0);
		g_pdata->g_cfg->wimax_mode = SDIO_MODE;
		g_pdata->power(1);
	} else if (strncmp(buffer, "off0", 4) == 0) {
		dump_debug("Power Off!!");
		g_pdata->power(0);
	} else if (strncmp(buffer, "wu00", 4) == 0) {
		dump_debug("WiMAX UART!!");
		g_pdata->switch_uart_wimax();
	} else if (strncmp(buffer, "au00", 4) == 0) {
		dump_debug("AP UART!!");
		g_pdata->switch_uart_ap();
	} else if (strncmp(buffer, "don0", 4) == 0) {
		dump_debug("Enable Dump!!");
		g_pdata->g_cfg->enable_dump_msg = 1;
	} else if (strncmp(buffer, "doff", 4) == 0) {
		dump_debug("Disable Dump!!");
		g_pdata->g_cfg->enable_dump_msg = 0;
	} else if (strncmp(buffer, "gpio", 4) == 0) {
		dump_debug("Display GPIOs!!");
		g_pdata->display_gpios();
	} else if (strncmp(buffer, "wake", 4) == 0) {
		dump_debug("g_pdata->wimax_wakeup!!");
		g_pdata->wakeup_assert(1);
		msleep(10);
		g_pdata->wakeup_assert(0);
	}

	return count - 1;
}

static int proc_read_onoff(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	g_pdata->power(0);
	return 0;
}

static int proc_write_onoff(struct file *foke,
		const char *buffer, unsigned long count, void *data)
{
	if (buffer[0] == 's') {
		if (g_pdata->g_cfg->wimax_mode != SDIO_MODE
				) {
			g_pdata->g_cfg->wimax_mode = SDIO_MODE;
			g_pdata->power(1);
		}
	} else if (buffer[0] == 'w') {
		if (g_pdata->g_cfg->wimax_mode != WTM_MODE
				) {
			g_pdata->g_cfg->wimax_mode = WTM_MODE;
			g_pdata->power(1);
		}
	} else if (buffer[0] == 'u') {
		if (g_pdata->g_cfg->wimax_mode != USB_MODE
				) {
			g_pdata->g_cfg->wimax_mode = USB_MODE;
			g_pdata->power(1);
		}
	} else if (buffer[0] == 'a') {
		if (g_pdata->g_cfg->wimax_mode != AUTH_MODE
				) {
			g_pdata->g_cfg->wimax_mode = AUTH_MODE;
			g_pdata->power(1);
		}
	}

	return count - 1;
}

static int proc_read_dump(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	g_pdata->power(0);
	eeprom_check_cal();

	return 0;
}

static int proc_write_dump(struct file *foke,
		const char *buffer, unsigned long count, void *data)
{
	if (buffer[0] == '0') {
		dump_debug("Control Dump Disabled.");
		g_pdata->g_cfg->enable_dump_msg = 0;
	} else if (buffer[0] == '1') {
		dump_debug("Control Dump Enabled.");
		g_pdata->g_cfg->enable_dump_msg = 1;
	}

	return count - 1;
}

static int proc_read_sleepmode(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	return 0;
}

static int proc_write_sleepmode(struct file *foke, const char *buffer,
		unsigned long count, void *data)
{
	if (buffer[0] == '0') {
		dump_debug("WiMAX Sleep Mode: VI");
		g_pdata->g_cfg->sleep_mode = 0;
	} else if (buffer[0] == '1') {
		dump_debug("WiMAX Sleep Mode: IDLE");
		g_pdata->g_cfg->sleep_mode = 1;
	}

	return count - 1;
}

void proc_init(void)
{
	/* create procfs directory & entry */
	g_proc_wmxmode_dir = proc_mkdir("wmxmode", NULL);
	create_proc_read_entry("mode", 0644, g_proc_wmxmode_dir,
			wimax_proc_mode, NULL);

	/* wimax power status */
	create_proc_read_entry("power", 0644, g_proc_wmxmode_dir,
			wimax_proc_power, NULL);

	/* switch UART path */
	g_proc_wmxuart = create_proc_entry("wmxuart", 0600,
			g_proc_wmxmode_dir);
	g_proc_wmxuart->data = NULL;
	g_proc_wmxuart->read_proc = proc_read_wmxuart;
	g_proc_wmxuart->write_proc = proc_write_wmxuart;

	/* write EEPROM */
	g_proc_eeprom = create_proc_entry("eeprom", 0666,
			g_proc_wmxmode_dir);
	g_proc_eeprom->data = NULL;
	g_proc_eeprom->read_proc = proc_read_eeprom;
	g_proc_eeprom->write_proc = proc_write_eeprom;

	/* on/off test */
	g_proc_onoff = create_proc_entry("onoff", 0600,
			g_proc_wmxmode_dir);
	g_proc_onoff->data = NULL;
	g_proc_onoff->read_proc = proc_read_onoff;
	g_proc_onoff->write_proc = proc_write_onoff;

	/* dump control message */
	g_proc_dump = create_proc_entry("dump", 0600,
			g_proc_wmxmode_dir);
	g_proc_dump->data = NULL;
	g_proc_dump->read_proc = proc_read_dump;
	g_proc_dump->write_proc = proc_write_dump;

	/* sleep Mode */
	g_proc_sleepmode = create_proc_entry("sleepmode", 0600,
			g_proc_wmxmode_dir);
	g_proc_sleepmode->data = NULL;
	g_proc_sleepmode->read_proc = proc_read_sleepmode;
	g_proc_sleepmode->write_proc = proc_write_sleepmode;
}

void proc_deinit(void)
{
	remove_proc_entry("sleepmode", g_proc_wmxmode_dir);
	remove_proc_entry("dump", g_proc_wmxmode_dir);
	remove_proc_entry("onoff", g_proc_wmxmode_dir);
	remove_proc_entry("eeprom", g_proc_wmxmode_dir);
	remove_proc_entry("wmxuart", g_proc_wmxmode_dir);
	remove_proc_entry("power", g_proc_wmxmode_dir);
	remove_proc_entry("mode", g_proc_wmxmode_dir);
	remove_proc_entry("wmxmode", NULL);
}
