/* tz mod in soft663 */
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

#include "soft66.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define DEBUG 0
#if DEBUG
#define dprintf(args...) printf(args)
#else
#define dprintf(args...)
#endif

static int vid_from_env(void)
{
	const char *s;
	s = getenv(SOFT66_VID_ENV);
	return s ? strtoul(s, NULL, 0) : 0;
}

static int pid_from_env(void)
{
	const char *s;
	s = getenv(SOFT66_PID_ENV);
	return s ? strtoul(s, NULL, 0) : 0;
}

// more awful globals

extern int first_time_write;  // this is used to control the reset and phase bits in set_frequency to match the original
								// vb code.  it gets set  ture in list_devices and gets set false in set_frequency



/**
 * Enumerate Soft66 devices found on the system and optionally call
 * a callback function with the serial number, manufacturer and description
 * for each (or NULL where these are not set).
 *
 * Callback function gets called with these arguments:
 *   cb(error, serial, manufacturer, description)
 *   error will be set to errno if getting the device info failed,
 *   usually EPERM.
 * 
 * @arg vid      Vendor ID (or 0 for default)
 * @arg pid      Product ID (or 0 for default)
 * @arg callback Callback function or NULL
 * 
 * Returns the number of devices found, or -1 if an error occurred.
 */
int soft66_list_devices(int vid, int pid, void(*callback)(const struct usb_device *, int, const char *, const char *, const char *))
{
	int r;
	struct ftdi_device_list *list, *e;
	struct ftdi_context *ftdi;
	char man[256];
	char des[256];
	char ser[256];
	int n = 0;

	ftdi = ftdi_new();			// get new ftdi context
	if (!ftdi)
		return -1;

	if (vid == 0)
		vid = vid_from_env();
	if (vid == 0)
		vid = SOFT66_VID_DEFAULT;
	if (pid == 0)
		pid = pid_from_env();
	if (pid == 0)
		pid = SOFT66_PID_DEFAULT;
	
	r = ftdi_usb_find_all(ftdi, &list, vid, pid);
	if (r < 0) {
		n = -1;
		goto out;
	}
	for (e=list; e; e=list->next) {
		n++;
		r = ftdi_usb_get_strings(ftdi, e->dev,
					 man, sizeof(man)-1,
					 des, sizeof(des)-1,
					 ser, sizeof(ser)-1);
		if (callback) {
			if (r < 0) {
				strcpy(man, "Unknown");
				strcpy(ser, "Unknown");
				strcpy(des, "Unknown");
			}
			callback(e->dev, (r<0) ? errno : 0, ser, man, des);
		}
	}

out:
	if (list)
		ftdi_list_free(&list);
	if (ftdi) {
		ftdi_deinit(ftdi);
		ftdi_free(ftdi);
	}
	return n;

}

/**
 * Allocate and initialize a soft66_context
 *
 * Returns NULL on failure or the created context, which can later
 * be released with soft66_free().
 */
struct soft66_context *soft66_new(void)
{
	struct soft66_context *ctx;
	ctx = malloc(sizeof(struct soft66_context));
	if (!ctx)
		return NULL;
	memset(ctx, 0, sizeof(*ctx));
	ctx->ftdi = ftdi_new();
	if (!ctx->ftdi) {
		free(ctx);
		return NULL;
	}
	return ctx;
}

/**
 * Open Soft66 device. If vid and pid are specified as 0, the
 * values of the SOFT66_VID and SOFT66_PID environment variables
 * are used, and if they are also not set then the default values are
 * used. If serial is specified or if SOFT66_SERIAL is set in the
 * environment, then only a device with that serial number will be
 * opened. If it is NULL and not specified in the environment, the
 * first device matching vid and pid will be opened.
 *
 * @arg ctx    Context created by soft66_new()
 * @arg vid    Vendor ID (or 0 for default)
 * @arg pid    Product ID (or 0 for default)
 * @arg serial Serial number or NULL
 */
int soft66_open_device(struct soft66_context *ctx, 
			  int vid, int pid, const char *serial)
{
	int r;
	struct ftdi_device_list *list, *e;
	char ser[256];
	if (!serial)
		serial = getenv(SOFT66_SERIAL_ENV);
	if (vid == 0)
		vid = vid_from_env();
	if (vid == 0)
		vid = SOFT66_VID_DEFAULT;
	if (pid == 0)
		pid = pid_from_env();
	if (pid == 0)
		pid = SOFT66_PID_DEFAULT;
	
	r = ftdi_usb_find_all(ctx->ftdi, &list, vid, pid);
	if (r < 0)
		return r;
	if (!list) {
		errno = ENODEV;
		return -1;
	}
	r = 0;
	for (e=list; e; e=list->next) {
		r = ftdi_usb_get_strings(ctx->ftdi, e->dev,
					 NULL, 0,
					 NULL, 0,
					 ser, sizeof(ser)-1);
		if (r < 0) {
			continue;
		}
		if (serial && strcmp(ser, serial)) {
			r = -1;
			errno = ENOENT;
			continue;
		}
		if (!serial && e->next) {
			/* More than one device, we need to specify the
			 * serial number one way or another */
			r = -1;
			errno = EINVAL;
			break;
		}
		r = ftdi_usb_open_dev(ctx->ftdi, e->dev);
		ctx->model = SOFT66_MODEL_SOFT66ADD;
		
		printf("setting model to soft66add\n");
		
		ctx->freq_min = 1000;		/* these are set to 500 and 70,000,000 in lc2 */
		ctx->freq_max = 70000000;
		break;
	}
	ftdi_list_free(&list);
	return r;
}

int soft66_close_device(struct soft66_context *ctx)
{
	if (!ctx)
		return(0);
	if (ctx->ftdi) {
		ftdi_deinit(ctx->ftdi);
		ftdi_free(ctx->ftdi);
		ctx->ftdi = NULL;
	}
    return(0);
}

void soft66_free(struct soft66_context *ctx)
{
	if (!ctx)
		return;
	soft66_close_device(ctx);
	free(ctx);
	ctx = NULL;
	
}
/* formats a chunk of data

	input is val - a 16 bit unsigned int
	output is a 'binary string'
	
	If you can figure out this function everything else will be easier
*/
static int write_16(unsigned char *buf, unsigned short val)
{
	int c, p = 0;
	int bit;
	for (c=0; c<4; c++)	/* this sets up the 5 high order bits as 55551 */
		buf[p++] = 0x5; /* CLOCK + FSYNC high */
	buf[p++] = 0x1; /* FSYNC low */
	dprintf("w: %04x: ", val);
	for (c=0; c<16; c++) {	/* this loop goes bit by bit through val translating into bitzero string format */
		bit = (val >> (15-c)) & 1;
		dprintf("%d ", bit);
		buf[p++] = (bit << 1) | 0x1; /* SCLK high */ /* same as lc2 32 */
		buf[p++] = (bit << 1); /* SCLK low */		/* 10 */
	}
	buf[p++] = 0x5;				/* tack on another 5 at the end - this is different from lc2 */
	dprintf("\n");
	return p;
}

// this version does the ctrl regs and phase stuff 
// note that all prefixes and suffixes are added manually
// in set_frequency 
int write_16_init(unsigned char *buf, unsigned short val)
{
	int c, p = 0;
	int bit;
//	for (c=0; c<4; c++)	/* this sets up the 5 high order bits as 55551 */
//		buf[p++] = 0x5; /* CLOCK + FSYNC high */
//	buf[p++] = 0x1; /* FSYNC low */
	dprintf("w: %04x: ", val);
	for (c=0; c<16; c++) {	/* this loop goes bit by bit through val translating into bitzero string format */
		bit = (val >> (15-c)) & 1;
		dprintf("%d ", bit);
		buf[p++] = (bit << 1) | 0x1; /* SCLK high */ /* same as lc2 32 */
		buf[p++] = (bit << 1); /* SCLK low */		/* 10 */
	}
//	buf[p++] = 0x5;				/* tack on another 5 at the end - this is different from lc2 */
	dprintf("\n");
	return p;
}
// this actually writes suffixes too
// it just translates the digits in the pfx string from char to int
 int write_prefix(unsigned char *buf, char *pfx)
{
	int i, p = 0;
	int len;
	
	len = strlen(pfx);
	for (i=0; i < len; i++)	{
		buf[p++] = pfx[i] - 48; // convert char to int
		}
		 
	return p;
}



/* this is the entry point for setting frequency with actual frequency in freq
*/
int soft66_set_frequency(struct soft66_context *ctx, int freq, int noreset)
{
	unsigned char buf[2048];
	unsigned char *p, *q;
	int len;
	int c;
	unsigned char filter_bits;
	unsigned int relfreq, ddefreq;
	unsigned short fr_lo, fr_hi;
	unsigned short ctrlreg;
	int x;

/* standard ftdi mode stuff */

	int r = 0;
	r = ftdi_set_bitmode(ctx->ftdi, 0xff, BITMODE_BITBANG);
	if (r<0) {
		errno = EIO;
		return -1;
	}
	r = ftdi_set_baudrate(ctx->ftdi, 30000);
	if (r<0) {
		errno = EIO;
		return -1;
	}

	/* Prepare buffer to write in bitbang mode. */

/* basic range checking - low range is slightly different in lc2 */

	if (freq < ctx->freq_min) {
		errno = EINVAL;
		return -1;
	}
	if (freq > ctx->freq_max) {
		errno = EINVAL;
		return -1;
	}

/* this should have optiion for lc2 to do this right */

	switch (ctx->model) {
	case SOFT66_MODEL_SOFT66ADD:
		ddefreq = freq >> 1;		/* divide freq by 2 - ok */
		break;
	default:
		ddefreq = freq;
	}

/* ok so far we have same setup  as set_datas() 
	but here the Cbt3253 will be set as filterbits in the next section
	and the initialize0(4) bit will be set in ctrlregs just after that
	
*/

	/* Frequency as fraction of MCLK, normalized to 28 bits */
	/* ok relfreq should be equivalent to dstatus
	*/
	relfreq = (unsigned int) (ddefreq / 75000000.0 * (double)(1 << 28));
	fr_hi = 0;
	fr_lo = 0;
	fr_lo = relfreq & 0x3fff;				/* not sure what these are yet.... msb, lsb ???? */
	fr_hi = ( relfreq  >> 14 ) & 0x3fff;
	dprintf("ddefreq: %d relfreq: %d (0x%08x) - hi: 0x%04x lo: 0x%04x\n", ddefreq, relfreq, relfreq, fr_hi, fr_lo);
	p = buf;

/* this looks like the setting for Cbt3253 - there are differences from lc2, noted in comments */

	filter_bits = 0;
	if (freq < 1200000)
		filter_bits = 0x10;
	else if (freq < 5500000)
		filter_bits = 0xB0;			/* B0  instead of 30 */
	else if (freq < 10500000)
		filter_bits = 0xC0;			/* C0 instead of 40 */
	else if (freq < 18500000)
		filter_bits = 0x60;			
	else if (freq < 30000000)
		filter_bits = 0x00;			
	else
		filter_bits = 0xD0;			/* D0 instead of 70 */

/* here is the setting for initialize0(4) bit 
	the ctrlreq corresponds exactly to initialize0 array settings where 15 is high order bit and 0 is low
*/

	if ((freq < 1600000) || ( freq> 19000000))
		ctrlreg = 0x2028;
	else
		ctrlreg = 0x2038;

/* throughout, were assuming noreset is false */
	q = p;
	p += write_prefix(p, "55551" );
	
	p += write_prefix(p, "55551" );


//	if (!noreset)
//        {
		/* Write the control register */
//		p += write_16_init(p, ctrlreg | 0x0100);		/* 0x01000 sets the reset bit */
//	}


	// this code was done to match the vb version where the initialize0 bit doesn't ever 
	// get reset after the first time -
	// so it will only get initialized when context is reopened (see list_devices)
	
	if(first_time_write) {
		p += write_16_init(p, ctrlreg | 0x0100);		/* 0x01000 sets the reset bit */
	}
	else {
		p += write_16_init(p, ctrlreg );		/* leave reset bit at 0 */
	}
	
	p += write_prefix(p, "1313131" ); 
	
	x = p - q;
	show_buf(q, x, "ctrl regs");

	/* Top two bits is the register address: 
	 * 1 for FREQ0
	 * 2 for FREQ1 
	 * 3 for PHASE
	 */
	/* FREQ0 */
	

	q = p;
	
	p += write_16_init(p, 0x4000 | fr_lo);
	


	p += write_prefix(p, "55551" );
	
	p += write_16_init(p, 0x4000 | fr_hi);
	
	p += write_prefix(p, "1313131" ); 
	
	x = p - q;
	show_buf(q, x, "freq0" );
	q = p;
	
	/* FREQ1 */

	
	p += write_16_init(p, 0x8000 | fr_lo);
	
	p += write_prefix(p, "55551" );

	p += write_16_init(p, 0x8000 | fr_hi);
	
	p += write_prefix(p, "1313131" ); 
	
	x = p - q;
	show_buf(q, x, "freq1");
	q = p;
	
	
//	if (!noreset) {
		/* PHASE0 register */
//		p += write_16_init(p, 0xc000);
//	}
	
// this code was done to match the vb version where the phase0 bit doesn't ever 
// get reset after the first time - like the initalize0 thing above
// so it will only get initialized when context is reopened (see list_devices)	

// also reset the first_time_write global to false here
// this is kludgey but it matches the original vb code
	
	if(first_time_write) {
		p += write_16_init(p, 0xc000);
		first_time_write = 0;
	}
	else {
		p += write_16_init(p, 0xe000);
	}

	p += write_prefix(p, "5551" );
	
	x = p - q;
	show_buf(q, x, "phase0");
	q = p;
	
	//	if (!noreset) {
			/* PHASE1 register */
			p += write_16_init(p, 0xe000);
	//	}

		p += write_prefix(p, "5551" );

		x = p - q;
		show_buf(q, x, "phase1");
		q = p;
	

	/* Write control register again with RESET disabled */
	p += write_16_init(p, ctrlreg);
	p += write_prefix(p, "555" );

	x = p - q;
	show_buf(q, x, "ctrl reg2");
	q = p;

	len = p-buf;



	/* Or in the filter control bits */
//	for (c=0; c<len; c++)
//		buf[c] |= filter_bits;
	
	chip_change(buf, len, filter_bits);	
	
	show_buf(buf, len, "filterbits");
	

	dprintf("To write: %d bytes. Filter bits: 0x%02x\n", len, filter_bits);
	dprintf("Bits being written:\n");
	for (c=0; c<len; c++) {
		if ((buf[c] & 1) == 0) { /* Falling SCLK edge */
			dprintf("%d", (buf[c] & 0x2) >> 1);
        }
    }
	//for (c=0; c<len; c++)
	//	printf("%d: 0x%02x\n", c, buf[c]);
	//printf("\n");
	r = ftdi_write_data(ctx->ftdi, (unsigned char *)buf, len);
	return r;
}

// finalize freq data buffer by factoring in filterbits and LED flasher
int chip_change(unsigned char *buf, int len, unsigned char filterbits) {
	
	int i;
	unsigned char Ics672;
	static int ping = 1;
	
	
	ping = ping * -1;
	
	if (ping == 1) {
		Ics672 = 0x8;
	}
	else {
		Ics672 = 0x0;
	}
	
	
	
	for (i = 0 ; i < len; i++) {
		buf[i] += Ics672 + filterbits;
	}

	return(0);
	
}


int soft66_set_filterbits(struct soft66_context *ctx, unsigned char filterbits)
{
	int r = 0;
	r = ftdi_set_bitmode(ctx->ftdi, 0xff, BITMODE_BITBANG);
	if (r<0) {
		errno = EIO;
		return -1;
	}
	r = ftdi_set_baudrate(ctx->ftdi, 30000);
	if (r<0) {
		errno = EIO;
		return -1;
	}

	filterbits &= ~0x7;
	filterbits |= 0x5;
	r = ftdi_write_data(ctx->ftdi, (unsigned char *)&filterbits, 1);

	return r;
}

int soft66_get_filterbits(struct soft66_context *ctx, unsigned char *filterbits)
{
	int r = 0;
	r = ftdi_set_bitmode(ctx->ftdi, 0xff, BITMODE_BITBANG);
	if (r<0) {
		errno = EIO;
		return -1;
	}
	r = ftdi_set_baudrate(ctx->ftdi, 30000);
	if (r<0) {
		errno = EIO;
		return -1;
	}

	r = ftdi_read_data(ctx->ftdi, filterbits, 1);
	return r;
}

int soft66_sleep(struct soft66_context *ctx)
{
	int r = 0;
	int len;
	char unsigned buf[256];

	r = ftdi_set_bitmode(ctx->ftdi, 0xff, BITMODE_BITBANG);
	if (r<0) {
		errno = EIO;
		return -1;
	}
	r = ftdi_set_baudrate(ctx->ftdi, 30000);
	if (r<0) {
		errno = EIO;
		return -1;
	}

	len = write_16(buf, 0x21f8);
	r = ftdi_write_data(ctx->ftdi, (unsigned char *)buf, len);

	return r;
}


/* tz - debug function for displaying buffer contents */
int show_buf(unsigned char *buf, int len, char *desc)
{
	dprintf("%s:buf(%d):[", desc, len);
	int i;
	
	
	for ( i = 0; i < len; i++) {
		dprintf("%d-", buf[i]);
		if(((i + 1) % 8) == 0) {
			dprintf("][");
		}
	}
	dprintf("\n");
	
	return(0);
}

