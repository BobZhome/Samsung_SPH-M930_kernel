/**
 * header for dpram driver
 *
 * Copyright (C) 2010 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef __DPRAM_H__
#define __DPRAM_H__

#include <linux/ioport.h>
#include <linux/types.h>

struct dpram_platform_data {
	void (*cfg_gpio)(void);
};

extern int dpram_register_handler(void (*handler)(u32, void *), void *data);
extern int dpram_unregister_handler(void (*handler)(u32, void *));

extern struct resource* dpram_request_region(resource_size_t start,
		resource_size_t size, const char *name);
extern void dpram_release_region(resource_size_t start,
		resource_size_t size);

extern int dpram_read_mailbox(u32 *);
extern int dpram_write_mailbox(u32);

extern int dpram_get_auth(u32 cmd);
extern int dpram_put_auth(int release);

extern int dpram_rel_sem(void);
extern int dpram_read_sem(void);

#define DPRAM_GET_AUTH _IOW('o', 0x20, u32)
#define DPRAM_PUT_AUTH _IO('o', 0x21)
#define DPRAM_REL_SEM _IO('o', 0x22)

#endif /* __DPRAM_H__ */
