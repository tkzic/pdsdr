//
//  main.c
//  s66test1
//
//  Created by tom zicarelli on 8/29/11.
//  Copyright 2011 zproject. All rights reserved.
//

// we're opening a global device context in list_radios, but never freeing it.  need to 
// actually set up some max open close messages for this
//

// max stuff

// #include "ext.h"			// you must include this - it contains the external object's link to available Max functions
// #include "ext_obex.h"		// this is required for all objects using the newer style for writing objects.

// pd stuff

#include "m_pd.h"

// end pd

#include "soft66.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#define _GNU_SOURCE
#include <getopt.h>

struct soft66_context *ctx = NULL;

// more awful globals

int first_time_write = 1;  // this is used to control the reset and phase bits in set_frequency to match the original
								// vb code.  it gets set  ture in list_devices and gets set false in set_frequency



#define die(args...) do { fprintf(stderr, ##args); exit(1); } while(0)

/* These defaults can be overridden by command line options */
static const char *serial = NULL;
// static int show_list = 0;
// static int show_info = 0;
// static unsigned long tune_freq = 0;
// static int retune = 0;
static int vid = 0;
static int pid = 0;
// static int get_filterbits = 0;
// static int set_filterbits = 0;
// static int powerdown = 0;
// static unsigned char filterbits = 0;

int list_radios(void);
int set_radio_frequency(int freq);


// callback for the device info function - not really necessary but it works well
static void list_cb(const struct usb_device *dev, int error, const char *ser, const char *man, const char *des)
{
    
    char msg[120];
    

	sprintf(msg,"Bus %s device %s:\n", dev->bus->dirname, dev->filename);
	post(msg, 0 );
    if (error) {
		sprintf(msg,"  Device information incomplete: %s\n",
		       strerror(error));
        post( msg, 0 );
		if (error == EPERM) {
			sprintf(msg, "  You do not have permission to access this device. See the documentation!\n");
            post(msg, 0 );
        }
	}
	sprintf(msg, "  Manufacturer: %s\n"
	       "  Description:  %s\n"
	       "  Serial no.:   %s\n", man, des, ser);
    post(msg, 0 );
}

/*
static void show_device_info(void)
{
	int r;
	unsigned char filterbits;
	r = soft66_get_filterbits(ctx, &filterbits);
	if (r < 0) {
		die("Query device failed: %s\n", strerror(errno));
	}
	printf("Register setting: 0x%02x\n", filterbits);
}
*/

// this will just query the device and close - for now
int list_radios(void)
{
	int r;
    char msg[120];
	    


		if(ctx)	{	// if global context open, free it because we'll reopen it
	 		soft66_free(ctx);
			first_time_write = 1;  // this gets set in set_frequency
		}
		
		
	
		r = soft66_list_devices(vid, pid, list_cb);
		sprintf(msg, "%d device%s found\n", r, r==1?"":"s");
        post(msg, 0);


/*
		if(r == 1)	// if one device, then open the global context and leave it open...
			{

				ctx = soft66_new(); // get context
				if (!ctx) {
			        post("Couldn't create context: ",0 );
			        post(strerror(errno), 0);
			        return(-1);
			    }
			
				post("global device context open", 0);	
			}
			
*/			
		return (r);
	
}

// set frequency in hz
int set_radio_frequency(int freq) {
    
    int r;              // status 

//	first_time_write = true;  // this gets set false in set_frequency
	
	if(first_time_write) {
	
    ctx = soft66_new(); // get context
	if (!ctx) {
        post("Couldn't create context: ",0 );
        post(strerror(errno), 0);
        return(-1);
    }


		
	r = soft66_open_device(ctx, vid, pid, serial);
	if (r < 0) {
		post("Couldn't open Soft66 device: ", 0);
        post(strerror(errno), 0);
        return(-1);
	}

	}
/*
	if (show_info) {
		show_device_info();
	}

*/
    
	r = soft66_set_frequency(ctx, freq, 0);
	if (r < 0) {
        post("Couldn't set frequency: ", 0);
        post(strerror(errno), 0);
		}
	
 //  	soft66_close_device(ctx);

// this is the closing stuff we need to figure out

//	soft66_free(ctx);
    
//    post("freq set ok",0);
    
	return(0);
    
    
}
