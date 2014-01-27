/*
 * Soft66 Radio Control for Linux
 * Copyright (C) 2010 Thomas Horsten <thomas@horsten.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ftdi.h"

#define SOFT66_SERIAL_ENV "SOFT66_SERIAL"
#define SOFT66_VID_ENV "SOFT66_VID"
#define SOFT66_PID_ENV "SOFT66_PID"

#define SOFT66_VID_DEFAULT 0x0403
#define SOFT66_PID_DEFAULT 0x6001

enum soft66_model {
	SOFT66_MODEL_UNKNOWN = 0x0,
	SOFT66_MODEL_SOFT66ADD = 0x1,
};

struct soft66_context {
	int model;
	unsigned int chipid;
	unsigned int freq_min;
	unsigned int freq_max;
	unsigned int mclk;

	struct ftdi_context *ftdi;
};

int soft66_list_devices(int vid, int pid, void(*callback)(const struct usb_device *, int error, const char *, const char *, const char *));

struct soft66_context *soft66_new(void);

void soft66_free(struct soft66_context *ctx);

int soft66_open_device(struct soft66_context *ctx, 
			 int vid, int pid, const char *serial);

int soft66_close_device(struct soft66_context *ctx);

int soft66_query_device(struct soft66_context *ctx);

int soft66_set_frequency(struct soft66_context *ctx, int freq, int noreset);

int show_buf(unsigned char *buf, int len, char *desc);

int soft66_set_filterbits(struct soft66_context *ctx, unsigned char filterbits);

int soft66_get_filterbits(struct soft66_context *ctx, unsigned char *filterbits);

int soft66_sleep(struct soft66_context *ctx);

int chip_change(unsigned char *buf, int len, unsigned char filterbits);

int write_prefix(unsigned char *buf, char *pfx);

int write_16_init(unsigned char *buf, unsigned short val);


 
