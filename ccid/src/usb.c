/*
 * Copyright (C) 2009-2012 Frank Morgner <frankmorgner@gmail.com>
 *
 * This file is part of ccid-emulator.
 *
 * ccid-emulator is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * ccid-emulator is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ccid-emulator.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <memory.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#include <asm/byteorder.h>

#include <linux/types.h>
#include <linux/usb/gadgetfs.h>
#include <linux/usb/ch9.h>
//#include <usb.h>
//

#ifdef	AIO
/* this aio code works with libaio-0.3.106 */
#include <libaio.h>
#endif

#include "ccid.h"
#include "cmdline.h"
#include "config.h"
#include "scutil.h"
#include "usbstring.h"

#define DRIVER_VENDOR_NUM	0x0D46		/* KOBIL Systems */
#define DRIVER_PRODUCT_NUM	0x3010		/* KOBIL Class 3 Reader */
static int vendorid   = DRIVER_VENDOR_NUM;
static int productid  = DRIVER_PRODUCT_NUM;
static int verbose    = 0;
static int debug      = 0;
static int doint      = 0;
static const char *doiintf = NULL;
static const char *gadgetfs = "/dev/gadget";

/* NOTE:  these IDs don't imply endpoint numbering; host side drivers
 * should use endpoint descriptors, or perhaps bcdDevice, to configure
 * such things.  Other product IDs could have different policies.
 */

/*-------------------------------------------------------------------------*/

/* these descriptors are modified based on what controller we find */

extern struct ccid_class_descriptor ccid_desc;

#define	STRINGID_MFGR		1
#define	STRINGID_PRODUCT	2
#define	STRINGID_SERIAL		3
#define	STRINGID_CONFIG		4
#define	STRINGID_INTERFACE	5

static struct usb_device_descriptor
device_desc = {
	.bLength =		sizeof device_desc,

	//.bcdUSB =		__constant_cpu_to_le16 (0x0110),
        .bDescriptorType =	USB_DT_DEVICE,

	.bcdUSB =		__constant_cpu_to_le16 (0x0200),
	.bDeviceClass =		USB_CLASS_VENDOR_SPEC,
	.bDeviceSubClass =	0,
	.bDeviceProtocol =	0,
	// .bMaxPacketSize0 ... set by gadgetfs
	.idVendor =		__constant_cpu_to_le16 (DRIVER_VENDOR_NUM),
	.idProduct =		__constant_cpu_to_le16 (DRIVER_PRODUCT_NUM),
	.iManufacturer =	STRINGID_MFGR,
	.iProduct =		STRINGID_PRODUCT,
	.iSerialNumber =	STRINGID_SERIAL,
	.bNumConfigurations =	1,
};

#define	MAX_USB_POWER		1

#define	CONFIG_VALUE		3

static struct usb_config_descriptor
config = {
	.bLength =		sizeof config,
	.bDescriptorType =	USB_DT_CONFIG,

	/* must compute wTotalLength ... */
	.bNumInterfaces =	1,
	.bConfigurationValue =	CONFIG_VALUE,
	.iConfiguration =	STRINGID_CONFIG,
	.bmAttributes =		USB_CONFIG_ATT_ONE
					| USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower =		(MAX_USB_POWER + 1) / 2,
};

static struct usb_interface_descriptor
source_sink_intf = {
    .bLength            = sizeof source_sink_intf,
    .bDescriptorType    = USB_DT_INTERFACE,

    .bInterfaceClass    = USB_CLASS_CSCID,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface         = STRINGID_INTERFACE,
};

/* Full speed configurations are used for full-speed only devices as
 * well as dual-speed ones (the only kind with high speed support).
 */

static struct usb_endpoint_descriptor
fs_source_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	/* NOTE some controllers may need FS bulk max packet size
	 * to be smaller.  it would be a chip-specific option.
	 */
	.wMaxPacketSize =	__constant_cpu_to_le16 (64),
};

static struct usb_endpoint_descriptor
fs_sink_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16 (64),
};

/* some devices can handle other result packet sizes */
/*#define STATUS_MAXPACKET	16*/
/*#define	LOG2_STATUS_POLL_MSEC	5*/
#define STATUS_MAXPACKET	8
#define	LOG2_STATUS_POLL_MSEC	3

static struct usb_endpoint_descriptor
fs_status_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	__constant_cpu_to_le16 (STATUS_MAXPACKET),
	.bInterval =		(1 << LOG2_STATUS_POLL_MSEC),
};

static const struct usb_endpoint_descriptor *fs_eps [3] = {
	&fs_source_desc,
	&fs_sink_desc,
	&fs_status_desc,
};


/* High speed configurations are used only in addition to a full-speed
 * ones ... since all high speed devices support full speed configs.
 * Of course, not all hardware supports high speed configurations.
 */

static struct usb_endpoint_descriptor
hs_source_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16 (512),
};

static struct usb_endpoint_descriptor
hs_sink_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16 (512),
	.bInterval =		1,
};

static struct usb_endpoint_descriptor
hs_status_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	__constant_cpu_to_le16 (STATUS_MAXPACKET),
	.bInterval =		LOG2_STATUS_POLL_MSEC + 3,
};

static const struct usb_endpoint_descriptor *hs_eps [] = {
	&hs_source_desc,
	&hs_sink_desc,
	&hs_status_desc,
};


/*-------------------------------------------------------------------------*/

/* 56 is the maximum for the KOBIL Class 3 Reader */
/* Flawfinder: ignore */
static char serial [57];

static const char interrupt_on_string[]  = "Insertion and removal events enabled";
static const char interrupt_off_string[] = "Insertion and removal events disabled";
static struct usb_string stringtab [] = {
	{ STRINGID_MFGR,          "Virtual Smart Card Architecture", },
	{ STRINGID_PRODUCT,       "CCID Emulator",                   },
	{ STRINGID_SERIAL,        serial,                            },
#ifdef WITH_PACE
	{ STRINGID_CONFIG,        "PACE support enabled",            },
#else
	{ STRINGID_CONFIG,        "PACE support disabled",           },
#endif
	{ STRINGID_INTERFACE,     interrupt_on_string,               },
};

static struct usb_gadget_strings strings = {
	.language = 0x0409,		/* "en-us" */
	.strings  = stringtab,
};

/*-------------------------------------------------------------------------*/

/* kernel drivers could autoconfigure like this too ... if
 * they were willing to waste the relevant code/data space.
 */

static int	HIGHSPEED;
static char	*DEVNAME;
static char	*EP_IN_NAME, *EP_OUT_NAME, *EP_STATUS_NAME;

/* gadgetfs currently has no chunking (or O_DIRECT/zerocopy) support
 * to turn big requests into lots of smaller ones; so this is "small".
 */
#define	USB_BUFSIZE	(7 * 1024)

static enum usb_device_speed	current_speed;

static int autoconfig ()
{
	struct stat	statb;

	/* NetChip 2280 PCI device (or dummy_hcd), high/full speed */
	if (stat (DEVNAME = "net2280", &statb) == 0
#ifdef OLD_KERNEL
            || stat (DEVNAME = "dummy_udc", &statb) == 0
#endif
    ) {
		HIGHSPEED = 1;
		device_desc.bcdDevice = __constant_cpu_to_le16 (0x0100),

		fs_source_desc.bEndpointAddress
			= hs_source_desc.bEndpointAddress
			= USB_DIR_IN | 7;
		EP_IN_NAME = "ep-a";
		fs_sink_desc.bEndpointAddress = hs_sink_desc.bEndpointAddress
			= USB_DIR_OUT | 3;
		EP_OUT_NAME = "ep-b";

		source_sink_intf.bNumEndpoints = 3;
		fs_status_desc.bEndpointAddress
			= hs_status_desc.bEndpointAddress
			= USB_DIR_IN | 11;
		EP_STATUS_NAME = "ep-f";

#ifndef OLD_KERNEL
	/* dummy_hcd, high/full speed */
    } else if (stat (DEVNAME = "dummy_udc", &statb) == 0) {
		HIGHSPEED = 1;
		device_desc.bcdDevice = __constant_cpu_to_le16 (0x0100),

		fs_source_desc.bEndpointAddress
			= hs_source_desc.bEndpointAddress
			= USB_DIR_IN | 7;
		EP_IN_NAME = "ep6in-bulk";
		fs_sink_desc.bEndpointAddress = hs_sink_desc.bEndpointAddress
			= USB_DIR_OUT | 3;
		EP_OUT_NAME = "ep7out-bulk";

		source_sink_intf.bNumEndpoints = 3;
		fs_status_desc.bEndpointAddress
			= hs_status_desc.bEndpointAddress
			= USB_DIR_IN | 11;
		EP_STATUS_NAME = "ep11in-bulk";
#endif

	/* Intel PXA 2xx processor, full speed only */
	} else if (stat (DEVNAME = "pxa2xx_udc", &statb) == 0) {
		HIGHSPEED = 0;
		device_desc.bcdDevice = __constant_cpu_to_le16 (0x0101),

		fs_source_desc.bEndpointAddress = USB_DIR_IN | 6;
		EP_IN_NAME = "ep6in-bulk";
		fs_sink_desc.bEndpointAddress = USB_DIR_OUT | 7;
		EP_OUT_NAME = "ep7out-bulk";

		/* using bulk for this since the pxa interrupt endpoints
		 * always use the no-toggle scheme (discouraged).
		 */
		source_sink_intf.bNumEndpoints = 3;
		fs_status_desc.bEndpointAddress = USB_DIR_IN | 11;
		EP_STATUS_NAME = "ep11in-bulk";

#if 0
	/* AMD au1x00 processor, full speed only */
	} else if (stat (DEVNAME = "au1x00_udc", &statb) == 0) {
		HIGHSPEED = 0;
		device_desc.bcdDevice = __constant_cpu_to_le16 (0x0102),

		fs_source_desc.bEndpointAddress = USB_DIR_IN | 2;
		EP_IN_NAME = "ep2in";
		fs_sink_desc.bEndpointAddress = USB_DIR_OUT | 4;
		EP_OUT_NAME = "ep4out";

		source_sink_intf.bNumEndpoints = 3;
		fs_status_desc.bEndpointAddress = USB_DIR_IN | 3;
		EP_STATUS_NAME = "ep3in";

	/* Intel SA-1100 processor, full speed only */
	} else if (stat (DEVNAME = "sa1100", &statb) == 0) {
		HIGHSPEED = 0;
		device_desc.bcdDevice = __constant_cpu_to_le16 (0x0103),

		fs_source_desc.bEndpointAddress = USB_DIR_IN | 2;
		EP_IN_NAME = "ep2in-bulk";
		fs_sink_desc.bEndpointAddress = USB_DIR_OUT | 1;
		EP_OUT_NAME = "ep1out-bulk";

		source_sink_intf.bNumEndpoints = 2;
                ccid_desc.dwFeatures &= ~0x100000; // USB Wake up signaling not supported
		EP_STATUS_NAME = 0;
#endif

        /* Toshiba TC86c001 PCI device, full speed only */
	} else if (stat (DEVNAME = "goku_udc", &statb) == 0) {
		HIGHSPEED = 0;
		device_desc.bcdDevice = __constant_cpu_to_le16 (0x0104),

		fs_source_desc.bEndpointAddress = USB_DIR_IN | 2;
		EP_IN_NAME = "ep2-bulk";
		fs_sink_desc.bEndpointAddress = USB_DIR_OUT | 1;
		EP_OUT_NAME = "ep1-bulk";

        source_sink_intf.bNumEndpoints = 3;

        EP_STATUS_NAME = "ep3-bulk";
        fs_status_desc.bEndpointAddress = USB_DIR_IN | 3;

        /* Samsung S3C24xx series, full speed only */
	} else if (stat (DEVNAME = "s3c2410_udc", &statb) == 0) {
		HIGHSPEED = 0;
		device_desc.bcdDevice = __constant_cpu_to_le16 (0x0110),

		fs_source_desc.bEndpointAddress = USB_DIR_IN | 2;
		EP_IN_NAME = "ep2-bulk";
		fs_sink_desc.bEndpointAddress = USB_DIR_OUT | 1;
		EP_OUT_NAME = "ep1-bulk";

		source_sink_intf.bNumEndpoints = 3;
		fs_status_desc.bEndpointAddress = USB_DIR_IN | 3;
		EP_STATUS_NAME = "ep3-bulk";

	/* Renesas SH77xx processors, full speed only */
	} else if (stat (DEVNAME = "sh_udc", &statb) == 0) {
		HIGHSPEED = 0;
		device_desc.bcdDevice = __constant_cpu_to_le16 (0x0105),

		fs_source_desc.bEndpointAddress = USB_DIR_IN | 2;
		EP_IN_NAME = "ep2in-bulk";
		fs_sink_desc.bEndpointAddress = USB_DIR_OUT | 1;
		EP_OUT_NAME = "ep1out-bulk";

		source_sink_intf.bNumEndpoints = 3;
		fs_status_desc.bEndpointAddress = USB_DIR_IN | 3;
		EP_STATUS_NAME = "ep3in-bulk";

	/* OMAP 1610 and newer devices, full speed only, fifo mode 0 or 3 */
	} else if (stat (DEVNAME = "omap_udc", &statb) == 0) {
		HIGHSPEED = 0;
		device_desc.bcdDevice = __constant_cpu_to_le16 (0x0106),

		fs_source_desc.bEndpointAddress = USB_DIR_IN | 1;
		EP_IN_NAME = "ep1in-bulk";
		fs_sink_desc.bEndpointAddress = USB_DIR_OUT | 2;
		EP_OUT_NAME = "ep2out-bulk";

		source_sink_intf.bNumEndpoints = 3;
		fs_status_desc.bEndpointAddress = USB_DIR_IN | 3;
		EP_STATUS_NAME = "ep3in-int";

	/* Something based on Mentor USB Highspeed Dual-Role Controller */
	} else if (stat (DEVNAME = "musb_hdrc", &statb) == 0) {
		HIGHSPEED = 1;
		device_desc.bcdDevice = __constant_cpu_to_le16 (0x0107),

		fs_source_desc.bEndpointAddress
			= hs_source_desc.bEndpointAddress
			= USB_DIR_IN | 1;
		EP_IN_NAME = "ep1in";
		fs_sink_desc.bEndpointAddress = hs_sink_desc.bEndpointAddress
			= USB_DIR_OUT | 1;
		EP_OUT_NAME = "ep1out";

		source_sink_intf.bNumEndpoints = 3;
		fs_status_desc.bEndpointAddress
			= hs_status_desc.bEndpointAddress
			= USB_DIR_IN | 3;
		EP_STATUS_NAME = "ep3";

	/* Atmel AT91 processors, full speed only */
	} else if (stat (DEVNAME = "at91_udc", &statb) == 0) {
		HIGHSPEED = 0;
		device_desc.bcdDevice = __constant_cpu_to_le16 (0x0106),

		fs_source_desc.bEndpointAddress = USB_DIR_IN | 1;
		EP_IN_NAME = "ep1";
		fs_sink_desc.bEndpointAddress = USB_DIR_OUT | 2;
		EP_OUT_NAME = "ep2";

		source_sink_intf.bNumEndpoints = 3;
		fs_status_desc.bEndpointAddress = USB_DIR_IN | 3;
		EP_STATUS_NAME = "ep3-int";

	/* Sharp LH740x processors, full speed only */
	} else if (stat (DEVNAME = "lh740x_udc", &statb) == 0) {
		HIGHSPEED = 0;
		device_desc.bcdDevice = __constant_cpu_to_le16 (0x0106),

		fs_source_desc.bEndpointAddress = USB_DIR_IN | 1;
		EP_IN_NAME = "ep1in-bulk";
		fs_sink_desc.bEndpointAddress = USB_DIR_OUT | 2;
		EP_OUT_NAME = "ep2out-bulk";

        source_sink_intf.bNumEndpoints = 3;

        fs_status_desc.bEndpointAddress = USB_DIR_IN | 3;
        EP_STATUS_NAME = "ep3in-int";

	/* Atmel AT32AP700x processors, high/full speed */
	} else if (stat (DEVNAME = "atmel_usba_udc", &statb) == 0) {
		HIGHSPEED = 1;
		device_desc.bcdDevice = __constant_cpu_to_le16 (0x0108);

		fs_source_desc.bEndpointAddress
			= hs_source_desc.bEndpointAddress
			= USB_DIR_IN | 1;
		EP_IN_NAME = "ep1in-bulk";
		fs_sink_desc.bEndpointAddress
			= hs_sink_desc.bEndpointAddress
			= USB_DIR_OUT | 2;
		EP_OUT_NAME = "ep2out-bulk";

        source_sink_intf.bNumEndpoints = 3;

        fs_status_desc.bEndpointAddress
            = hs_status_desc.bEndpointAddress
            = USB_DIR_IN | 3;
        EP_STATUS_NAME = "ep3in-int";

	} else {
		DEVNAME = 0;
		return -ENODEV;
	}
        if (!doint) {
            source_sink_intf.bNumEndpoints = 2;
            int i;
            if (!doiintf) {
                for (i=0; i<sizeof stringtab/sizeof(struct usb_string); i++) {
                    if (stringtab[i].id == STRINGID_INTERFACE) {
                        stringtab[i].s = interrupt_off_string;
                        break;
                    }
                }
            }
            // Automatic activation of ICC on inserting not supported
            // USB Wake up signaling not supported
            ccid_desc.dwFeatures &= ~__constant_cpu_to_le32(0x4|0x100000);
        }
	return 0;
}

#ifdef	AIO

static int			iso;
static int			interval;
static unsigned			iosize;
static unsigned			bufsize = USB_BUFSIZE;

/* This is almost the only place where usb needs to know whether we're
 * driving an isochronous stream or a bulk one.
 */
static int iso_autoconfig ()
{
	struct stat	statb;

	/* ISO endpoints "must not be part of a default interface setting".
	 * Never do it like this in "real" code!  This uses the default
	 * setting (alt 0) because it's the only one pxa supports.
	 *
	 * This code doesn't adjust the sample rate based on feedback.
	 */
	device_desc.idProduct = __constant_cpu_to_le16(productid);

	/* NetChip 2280 PCI device or dummy_hcd, high/full speed */
	if (stat (DEVNAME = "net2280", &statb) == 0 ||
			stat (DEVNAME = "dummy_udc", &statb) == 0) {
		unsigned	bInterval, wMaxPacketSize;

		HIGHSPEED = 1;
		device_desc.bcdDevice = __constant_cpu_to_le16 (0x0100);

		/* this code won't use two or four uframe periods */
		if (bufsize > 1024) {
			interval = 0;
			bInterval = 1;
			/* "modprobe net2280 fifo_mode=1" may be needed */
			if (bufsize > (2 * 1024)) {
				wMaxPacketSize = min ((bufsize + 2)/3, 1024);
				bufsize = min (3 * wMaxPacketSize, bufsize);
				wMaxPacketSize |= 2 << 11;
			} else {
				wMaxPacketSize = min ((bufsize + 1)/2, 1024);
				wMaxPacketSize |= 1 << 11;
			}
		} else {
			bInterval = interval + 4;
			wMaxPacketSize = bufsize;
		}

		fs_source_desc.bEndpointAddress
			= hs_source_desc.bEndpointAddress
			= USB_DIR_IN | 7;
		fs_source_desc.bmAttributes
			= hs_source_desc.bmAttributes
			= USB_ENDPOINT_XFER_ISOC;
		fs_source_desc.wMaxPacketSize = min (bufsize, 1023);
		hs_source_desc.wMaxPacketSize = wMaxPacketSize;
		fs_source_desc.bInterval = interval + 1;
		hs_source_desc.bInterval = bInterval;
		EP_IN_NAME = "ep-a";

		fs_sink_desc.bEndpointAddress
			= hs_sink_desc.bEndpointAddress
			= USB_DIR_OUT | 3;
		fs_sink_desc.bmAttributes
			= hs_sink_desc.bmAttributes
			= USB_ENDPOINT_XFER_ISOC;
		fs_sink_desc.wMaxPacketSize = min (bufsize, 1023);
		hs_sink_desc.wMaxPacketSize = wMaxPacketSize;
		fs_sink_desc.bInterval = interval + 1;
		hs_sink_desc.bInterval = bInterval;
		EP_OUT_NAME = "ep-b";

		source_sink_intf.bNumEndpoints = 3;
		fs_status_desc.bEndpointAddress
			= hs_status_desc.bEndpointAddress
			= USB_DIR_IN | 11;
		EP_STATUS_NAME = "ep-f";

	/* Intel PXA 2xx processor, full speed only */
	} else if (stat (DEVNAME = "pxa2xx_udc", &statb) == 0) {
		HIGHSPEED = 0;
		device_desc.bcdDevice = __constant_cpu_to_le16 (0x0101),

		bufsize = min (bufsize, 256);

		fs_source_desc.bEndpointAddress = USB_DIR_IN | 3;
		fs_source_desc.bmAttributes = USB_ENDPOINT_XFER_ISOC;
		fs_source_desc.wMaxPacketSize = bufsize;
		fs_source_desc.bInterval = interval;
		EP_IN_NAME = "ep3in-iso";

		fs_sink_desc.bEndpointAddress = USB_DIR_OUT | 4;
		fs_sink_desc.bmAttributes = USB_ENDPOINT_XFER_ISOC;
		fs_sink_desc.wMaxPacketSize = bufsize;
		fs_sink_desc.bInterval = interval;
		EP_OUT_NAME = "ep4out-iso";

		/* using bulk for this since the pxa interrupt endpoints
		 * always use the no-toggle scheme (discouraged).
		 */
		source_sink_intf.bNumEndpoints = 3;
		fs_status_desc.bEndpointAddress = USB_DIR_IN | 11;
		EP_STATUS_NAME = "ep11in-bulk";

	/* OMAP 1610 and newer devices, full speed only, fifo mode 3 */
	} else if (stat (DEVNAME = "omap_udc", &statb) == 0) {
		HIGHSPEED = 0;
		device_desc.bcdDevice = __constant_cpu_to_le16 (0x0102),

		fs_source_desc.bEndpointAddress = USB_DIR_IN | 7;
		fs_source_desc.bmAttributes = USB_ENDPOINT_XFER_ISOC;
		fs_source_desc.wMaxPacketSize = min (bufsize, 256);
		fs_source_desc.bInterval = interval;
		EP_IN_NAME = "ep7in-iso";

		fs_sink_desc.bEndpointAddress = USB_DIR_OUT | 8;
		fs_sink_desc.bmAttributes = USB_ENDPOINT_XFER_ISOC;
		fs_sink_desc.wMaxPacketSize = min (bufsize, 256);
		fs_sink_desc.bInterval = interval;
		EP_OUT_NAME = "ep8out-iso";

		source_sink_intf.bNumEndpoints = 3;
		fs_status_desc.bEndpointAddress = USB_DIR_IN | 9;
		EP_STATUS_NAME = "ep9in-int";

	/* Something based on Mentor USB Highspeed Dual-Role Controller;
	 * assumes a core that doesn't include high bandwidth support.
	 */
	} else if (stat (DEVNAME = "musb_hdrc", &statb) == 0) {
		unsigned	bInterval, wMaxPacketSize;

		HIGHSPEED = 1;
		device_desc.bcdDevice = __constant_cpu_to_le16 (0x0103);

		bInterval = interval + 4;
		wMaxPacketSize = bufsize;

		fs_source_desc.bEndpointAddress
			= hs_source_desc.bEndpointAddress
			= USB_DIR_IN | 1;
		fs_source_desc.bmAttributes
			= hs_source_desc.bmAttributes
			= USB_ENDPOINT_XFER_ISOC;
		fs_source_desc.wMaxPacketSize = min (bufsize, 1023);
		hs_source_desc.wMaxPacketSize = wMaxPacketSize;
		fs_source_desc.bInterval = interval + 1;
		hs_source_desc.bInterval = bInterval;
		EP_IN_NAME = "ep1in";

		fs_sink_desc.bEndpointAddress
			= hs_sink_desc.bEndpointAddress
			= USB_DIR_OUT | 1;
		fs_sink_desc.bmAttributes
			= hs_sink_desc.bmAttributes
			= USB_ENDPOINT_XFER_ISOC;
		fs_sink_desc.wMaxPacketSize = min (bufsize, 1023);
		hs_sink_desc.wMaxPacketSize = wMaxPacketSize;
		fs_sink_desc.bInterval = interval + 1;
		hs_sink_desc.bInterval = bInterval;
		EP_OUT_NAME = "ep1out";

		source_sink_intf.bNumEndpoints = 3;
		fs_status_desc.bEndpointAddress
			= hs_status_desc.bEndpointAddress
			= USB_DIR_IN | 11;
		EP_STATUS_NAME = "ep3";

	/* Atmel AT91 processors, full speed only */
	} else if (stat (DEVNAME = "at91_udc", &statb) == 0) {
		HIGHSPEED = 0;
		device_desc.bcdDevice = __constant_cpu_to_le16 (0x0104),

		fs_source_desc.bEndpointAddress = USB_DIR_IN | 4;
		fs_source_desc.bmAttributes = USB_ENDPOINT_XFER_ISOC;
		fs_source_desc.wMaxPacketSize = min (bufsize, 256);
		fs_source_desc.bInterval = interval;
		EP_IN_NAME = "ep4";

		fs_sink_desc.bEndpointAddress = USB_DIR_OUT | 2;
		fs_sink_desc.bmAttributes = USB_ENDPOINT_XFER_ISOC;
		fs_sink_desc.wMaxPacketSize = min (bufsize, 256);
		fs_sink_desc.bInterval = interval;
		EP_OUT_NAME = "ep5";

		source_sink_intf.bNumEndpoints = 3;
		fs_status_desc.bEndpointAddress = USB_DIR_IN | 3;
		EP_STATUS_NAME = "ep3-int";

	/* Atmel AT32AP700x processors, high/full speed */
	} else if (stat (DEVNAME = "atmel_usba_udc", &statb) == 0){
		HIGHSPEED = 1;

		device_desc.bcdDevice = __constant_cpu_to_le16 (0x0105);

		fs_source_desc.bEndpointAddress
			= hs_source_desc.bEndpointAddress
			= USB_DIR_IN | 5;
		fs_source_desc.bmAttributes
			= hs_source_desc.bmAttributes
			= USB_ENDPOINT_XFER_ISOC;
		fs_source_desc.wMaxPacketSize
			= hs_source_desc.wMaxPacketSize
			= __cpu_to_le16(min (bufsize, 1024));
		fs_source_desc.bInterval
			= hs_source_desc.bInterval
			= interval;
		EP_IN_NAME = "ep5in-iso";

		fs_sink_desc.bEndpointAddress
			= hs_sink_desc.bEndpointAddress
			= USB_DIR_OUT | 6;
		fs_sink_desc.bmAttributes
			= hs_sink_desc.bmAttributes
			= USB_ENDPOINT_XFER_ISOC;
		fs_sink_desc.wMaxPacketSize
			= hs_sink_desc.wMaxPacketSize
			= __cpu_to_le16(min (bufsize, 1024));
		fs_sink_desc.bInterval
			= hs_sink_desc.bInterval
			= interval;
		EP_OUT_NAME = "ep6out-iso";

		source_sink_intf.bNumEndpoints = 3;
		fs_status_desc.bEndpointAddress
			= hs_status_desc.bEndpointAddress
			= USB_DIR_IN | 3;
		EP_STATUS_NAME = "ep3in-int";

	} else {
		DEVNAME = 0;
		return -ENODEV;
	}
	if (verbose) {
		fprintf (stderr, "iso fs wMaxPacket %04x bInterval %02x\n",
			__le16_to_cpu(fs_sink_desc.wMaxPacketSize),
			fs_sink_desc.bInterval);
		if (HIGHSPEED)
			fprintf (stderr,
				"iso hs wMaxPacket %04x bInterval %02x\n",
				__le16_to_cpu(hs_sink_desc.wMaxPacketSize),
				hs_sink_desc.bInterval);
	}
	return 0;
}

#else
#define	iso	0
#endif	/* AIO */

/*-------------------------------------------------------------------------*/

/* full duplex data, with at least three threads: ep0, sink, and source */

static pthread_t	ep0;

static pthread_t	ccid_thread;
static pthread_t	interrupt_thread;
static int		source_fd = -1;
static int		sink_fd = -1;
static int		status_fd = -1;

// FIXME no result i/o yet

static void close_fd (void *fd_ptr)
{
	int	result, fd;

	fd = *(int *)fd_ptr;
	*(int *)fd_ptr = -1;

	/* test the FIFO ioctls (non-ep0 code paths) */
	if (pthread_self () != ep0) {
		result = ioctl (fd, GADGETFS_FIFO_STATUS);
		if (result < 0) {
			/* ENODEV reported after disconnect */
			if (errno != ENODEV && errno != EOPNOTSUPP)
				perror ("get fifo result");
		} else {
			fprintf (stderr, "fd %d, unclaimed = %d\n",
				fd, result);
			if (result) {
				result = ioctl (fd, GADGETFS_FIFO_FLUSH);
				if (result < 0)
					perror ("fifo flush");
			}
		}
	}

	if (close (fd) < 0)
		perror ("close fd");
        else
		if (debug)
			fprintf (stderr, "closed fd\n");
}

static void close_ccid()
{
    ccid_shutdown();
}


/* you should be able to open and configure endpoints
 * whether or not the host is connected
 */
static int
ep_config (char *name, const char *label,
	struct usb_endpoint_descriptor *fs,
	struct usb_endpoint_descriptor *hs
)
{
	int		fd, result;
	char		buf [USB_BUFSIZE];

	/* open and initialize with endpoint descriptor(s) */
	fd = open (name, O_RDWR);
	if (fd < 0) {
		result = -errno;
		fprintf (stderr, "%s open %s error %d (%s)\n",
			label, name, errno, strerror (errno));
		return result;
	}

	/* one (fs or ls) or two (fs + hs) sets of config descriptors */
	*(__u32 *)buf = 1;	/* tag for this format */
	memcpy (buf + 4, fs, USB_DT_ENDPOINT_SIZE);
	if (HIGHSPEED)
		memcpy (buf + 4 + USB_DT_ENDPOINT_SIZE,
			hs, USB_DT_ENDPOINT_SIZE);
	result = write (fd, buf, 4 + USB_DT_ENDPOINT_SIZE
			+ (HIGHSPEED ? USB_DT_ENDPOINT_SIZE : 0));
	if (result < 0) {
		result = -errno;
		fprintf (stderr, "%s config %s error %d (%s)\n",
			label, name, errno, strerror (errno));
		close (fd);
		return result;
	} else if (verbose > 1) {
		unsigned long	id;

		id = pthread_self ();
		fprintf (stderr, "%s (%ld): fd %d\n", label, id, fd);
	}
	return fd;
}

#define source_open(name) \
	ep_config(name,__FUNCTION__, &fs_source_desc, &hs_source_desc)
#define sink_open(name) \
	ep_config(name,__FUNCTION__, &fs_sink_desc, &hs_sink_desc)
#define status_open(name) \
	ep_config(name,__FUNCTION__, &fs_status_desc, &hs_status_desc)

static void *interrupt (void *param)
{
    status_fd = status_open (((char **) param)[0]);
    if (source_fd < 0) {
        /* an error concerning status endpoint is not fatal for in/out bulk*/
        /* transfer */
        if (verbose > 1)
            perror("status_fd");
        pthread_exit(0);
    }
    pthread_cleanup_push (close_fd, &status_fd);

    int result = 0;
    RDR_to_PC_NotifySlotChange_t *slotchange;
    do {
        /* original LinuxThreads cancelation didn't work right */
        /* so test for it explicitly. */
        pthread_testcancel ();

        if (ccid_state_changed(&slotchange, -1)) {
            /* don't bother host, when nothing changed */
            if (verbose > 1)
                fprintf(stderr, "interrupt loop: writing RDR_to_PC_NotifySlotChange... ");
            result = write(status_fd, slotchange, sizeof *slotchange);
            if (verbose > 1)
                fprintf(stderr, "done (%d written).\n", result);
        }
    } while (result >= 0);

    if (errno != ESHUTDOWN || result < 0) {
        perror ("interrupt loop aborted");
        pthread_cancel(ep0);
    }

    pthread_cleanup_pop (1);

    fflush (stdout);
    fflush (stderr);

    return 0;
}

/* ccid thread, forwards ccid requests to pcsc and returns results  */
static void *ccid (void *param)
{
    char	**names      = (char **) param;
    char	*source_name = names[0];
    char	*sink_name   = names[1];
    int		result;
    size_t      bufsize = sizeof(PC_to_RDR_XfrBlock_t) + CCID_EXT_APDU_MAX;
    __u8        inbuf[bufsize];

    source_fd = source_open (source_name);
    if (source_fd < 0) {
        if (verbose > 1)
            perror("source_fd");
        goto error;
    }
    pthread_cleanup_push (close_fd, &source_fd);

    sink_fd   = sink_open (sink_name);
    if (sink_fd   < 0) {
        if (verbose > 1)
            perror("sink_fd");
        goto error;
    }
    pthread_cleanup_push (close_fd, &sink_fd);

    pthread_cleanup_push (close_ccid, NULL);

    __u8 *outbuf = NULL;
    pthread_cleanup_push (free, outbuf);

    do {

        /* original LinuxThreads cancelation didn't work right
         * so test for it explicitly.
         */
        pthread_testcancel ();

        if (verbose > 1)
            fprintf(stderr, "bulk loop: reading %lu bytes... ", (long unsigned) bufsize);
        result = read(sink_fd, inbuf, bufsize);
        if (result < 0) break;
        if (verbose > 1)
            fprintf(stderr, "bulk loop: got %d, done.\n", result);
        if (!result) break;

        result = ccid_parse_bulkout(inbuf, result, &outbuf);
        if (result < 0) break;

        if (verbose > 1)
            fprintf(stderr, "bulk loop: writing %d bytes... ", result);
        result = write(source_fd, outbuf, result);
        if (verbose > 1)
            fprintf(stderr, "done (%d written).\n", result);
    } while (result >= 0);

    if (errno != ESHUTDOWN || result < 0) {
        perror ("ccid loop aborted");
        pthread_cancel(ep0);
    }

    pthread_cleanup_pop (1);
    pthread_cleanup_pop (1);
    pthread_cleanup_pop (1);
    pthread_cleanup_pop (1);

    fflush (stdout);
    fflush (stderr);

    return 0;

error:
    pthread_cancel(ep0);
    pthread_exit(0);
}

static void start_io (void)
{
    //int tmp;
	sigset_t	allsig, oldsig;

#ifdef	AIO
	/* iso uses the same API as bulk/interrupt.  we queue one
	 * (u)frame's worth of data per i/o request, and the host
	 * polls that queue once per interval.
	 */
	switch (current_speed) {
	case USB_SPEED_FULL:
		if (iso)
			iosize = __le16_to_cpup (&fs_source_desc
					.wMaxPacketSize);
		else
			iosize = bufsize;
		break;
	case USB_SPEED_HIGH:
		/* for iso, we updated bufsize earlier */
		iosize = bufsize;
		break;
	default:
		fprintf (stderr, "bogus link speed %d\n", current_speed);
		return;
	}
#endif	/* AIO */

	sigfillset (&allsig);
	errno = pthread_sigmask (SIG_SETMASK, &allsig, &oldsig);
	if (errno < 0) {
		perror ("set thread signal mask");
		return;
	}

	/* is it true that the LSB requires programs to disconnect
	 * from their controlling tty before pthread_create()?
	 * why?  this clearly doesn't ...
	 */

        static char *names[2];
        names[0] = EP_IN_NAME;
        names[1] = EP_OUT_NAME;
        if (pthread_create (&ccid_thread, NULL, ccid, (void *) names) != 0) {
		perror ("can't create ccid thread");
		goto cleanup;
	}
        if (doint) {
            static char * interruptnames[1];
            interruptnames[0] = EP_STATUS_NAME;
            if (pthread_create (&interrupt_thread, NULL, interrupt, (void *)
                        interruptnames) != 0) {
                perror ("can't create interrupt thread");
                goto cleanup;
            }
        } else if (verbose) {
            fprintf (stderr, "interrupt pipe disabled\n");
        }

	/* give the other threads a chance to run before we report
	 * success to the host.
	 * FIXME better yet, use pthread_cond_timedwait() and
	 * synchronize on ep config success.
	 */
	sched_yield ();

cleanup:
	errno = pthread_sigmask (SIG_SETMASK, &oldsig, 0);
	if (errno != 0) {
		perror ("restore sigmask");
		exit (-1);
	}
}

static void stop_io ()
{
    if (!pthread_equal (ccid_thread, ep0)) {
        pthread_cancel (ccid_thread);
        if (pthread_join (ccid_thread, 0) != 0)
            perror ("can't join ccid thread");
        ccid_thread = ep0;
        fprintf (stderr, "cancled ccid\n");
    }
    if (!pthread_equal (interrupt_thread, ep0)) {
        pthread_cancel (interrupt_thread);
        if (pthread_join (interrupt_thread, 0) != 0)
            perror ("can't join interrupt thread");
        interrupt_thread = ep0;
        fprintf (stderr, "cancled interrupt\n");
    }
}

/*-------------------------------------------------------------------------*/

static char *
build_config (char *cp, const struct usb_endpoint_descriptor **ep)
{
    struct usb_config_descriptor *c;
    int i;

    c = (struct usb_config_descriptor *) cp;

    memcpy (cp, &config, config.bLength);
    cp += config.bLength;

    memcpy (cp, &source_sink_intf, sizeof source_sink_intf);
    cp += sizeof source_sink_intf;

    // Append vendor class specification
    memcpy (cp, &ccid_desc, sizeof ccid_desc);
    cp += sizeof ccid_desc;

    for (i = 0; i < source_sink_intf.bNumEndpoints; i++) {
        memcpy (cp, ep [i], USB_DT_ENDPOINT_SIZE);
        cp += USB_DT_ENDPOINT_SIZE;
    }

    c->wTotalLength = __cpu_to_le16 (cp - (char *) c);
    return cp;
}

static int init_device (void)
{
	char		buf [4096], *cp = &buf [0];
	int		fd;
	int		result;

#ifdef	AIO
	if (iso)
		result = iso_autoconfig ();
	else
#endif
		result = autoconfig ();
	if (result < 0) {
		fprintf (stderr, "?? don't recognize %s %s device\n",
			gadgetfs, iso ? "iso" : "bulk");
		return result;
	}

	fd = open (DEVNAME, O_RDWR);
	if (fd < 0) {
		perror (DEVNAME);
		return -errno;
	}

	*(__u32 *)cp = 0;	/* tag for this format */
	cp += 4;

	/* write full then high speed configs */
	cp = build_config (cp, fs_eps);
	if (HIGHSPEED)
		cp = build_config (cp, hs_eps);

        device_desc.idVendor = __cpu_to_le16 (vendorid);
        device_desc.idProduct = __cpu_to_le16 (productid);
        if (verbose) {
            fprintf(stderr, "idVendor=%04X  idProduct=%04X\n", vendorid,
                    productid);
        }
	/* and device descriptor at the end */
	memcpy (cp, &device_desc, sizeof device_desc);
	cp += sizeof device_desc;

	result = write (fd, &buf [0], cp - &buf [0]);
	if (result < 0) {
		perror ("write dev descriptors");
		close (fd);
		return result;
	} else if (result != (cp - buf)) {
		fprintf (stderr, "dev init, wrote %d expected %ld\n",
				result, (long int) (cp - buf));
		close (fd);
		return -EIO;
	}
	return fd;
}

static void handle_control (int fd, struct usb_ctrlrequest *setup)
{
	int		result, tmp;
	__u8		buf [256];
	__u16		value, index, length;

	value = __le16_to_cpu(setup->wValue);
	index = __le16_to_cpu(setup->wIndex);
	length = __le16_to_cpu(setup->wLength);

        if ((setup->bRequestType & USB_TYPE_MASK) != USB_TYPE_STANDARD)
                goto special;

	switch (setup->bRequest) {	/* usb 2.0 spec ch9 requests */
	case USB_REQ_GET_DESCRIPTOR:
                if (verbose > 1)
                    fprintf(stderr, "USB_REQ_GET_DESCRIPTOR\n");
		if (setup->bRequestType != USB_DIR_IN)
			goto stall;
		switch (value >> 8) {
		case USB_DT_STRING:
			tmp = value & 0x0ff;
			if (verbose > 1)
				fprintf (stderr,
					"... get string %d lang %04x\n",
					tmp, index);
			if (tmp != 0 && index != strings.language) {
				fprintf (stderr, "wrong language, returning defaults\n");
				tmp = 0;
                        }
                        memset (buf, 0, 256);	/* zero all the bytes */
			result = usb_gadget_get_string (&strings, tmp, buf);
			if (result < 0) {
                            perror("usb_gadget_get_string");
                            goto stall;
                        }
			tmp = result;
			if (length < tmp)
				tmp = length;
			result = write (fd, buf, tmp);
			if (result < 0) {
				if (errno == EIDRM)
					fprintf (stderr, "string timeout\n");
				else
					perror ("write string data");
			} else if (result != tmp) {
				fprintf (stderr, "short string write, %d\n",
					result);
			}
			break;
		default:
			goto stall;
		}
		return;
	case USB_REQ_SET_CONFIGURATION:
		if (verbose > 1)
			fprintf (stderr, "USB_REQ_SET_CONFIGURATION #%d\n", value);
		if (setup->bRequestType != USB_DIR_OUT)
			goto stall;

		/* Kernel is normally waiting for us to finish reconfiguring
		 * the device.
		 *
		 * Some hardware can't, notably older PXA2xx hardware.  (With
		 * racey and restrictive config change automagic.  PXA 255 is
		 * OK, most PXA 250s aren't.  If it has a UDC CFR register,
		 * it can handle deferred response for SET_CONFIG.)  To handle
		 * such hardware, don't write code this way ... instead, keep
		 * the endpoints always active and don't rely on seeing any
		 * config change events, either this or SET_INTERFACE.
		 */
		switch (value) {
		case CONFIG_VALUE:
                        if ( source_fd >= 0 || sink_fd >= 0 || status_fd >= 0)
                            stop_io ();
                        start_io ();
			break;
		case 0:
			stop_io ();
			break;
		default:
			/* kernel bug -- "can't happen" */
			fprintf (stderr, "? illegal config\n");
			goto stall;
		}

		/* ... ack (a write would stall) */
		result = read (fd, &result, 0);
		if (result)
			perror ("ack SET_CONFIGURATION");
		return;
	case USB_REQ_GET_INTERFACE:
		if (verbose > 1)
			fprintf (stderr, "USB_REQ_GET_INTERFACE\n");
		if (setup->bRequestType != (USB_DIR_IN|USB_RECIP_INTERFACE)
				|| index != 0
				|| length > 1)
			goto stall;

		/* only one altsetting in this driver */
		buf [0] = 0;
		result = write (fd, buf, length);
		if (result < 0) {
			if (errno == EIDRM)
				fprintf (stderr, "GET_INTERFACE timeout\n");
			else
				perror ("write GET_INTERFACE data");
		} else if (result != length) {
			fprintf (stderr, "short GET_INTERFACE write, %d\n",
				result);
		}
		return;
	case USB_REQ_SET_INTERFACE:
		if (verbose > 1)
			fprintf (stderr, "USB_REQ_SET_INTERFACE\n");
		if (setup->bRequestType != USB_RECIP_INTERFACE
				|| index != 0
				|| value != 0)
			goto stall;

		/* just reset toggle/halt for the interface's endpoints */
		result = 0;
                if (ioctl (source_fd, GADGETFS_CLEAR_HALT) < 0) {
                    result = errno;
                    perror ("reset source fd");
                }
                if (ioctl (sink_fd, GADGETFS_CLEAR_HALT) < 0) {
                    result = errno;
                    perror ("reset sink fd");
                }
                if (status_fd > 0) {
                    if (ioctl (status_fd, GADGETFS_CLEAR_HALT) < 0) {
                        result = errno;
                        perror ("reset status fd");
                    }
                }
		/* FIXME eventually reset the result endpoint too */
		if (result)
			goto stall;

		/* ... and ack (a write would stall) */
		result = read (fd, &result, 0);
		if (result)
			perror ("ack SET_INTERFACE");
                return;
	default:
		goto stall;
	}

special:
        switch (setup->bRequestType) {
            case USB_REQ_CCID: {
                    __u8 *outbuf = NULL;
                    result = ccid_parse_control(setup, &outbuf);
                    if (result < 0 || result > 256)
                        goto stall;
                    if (verbose > 1)
                        fprintf(stderr, "control loop: writing %d bytes... ", result);
                    result = write (fd, outbuf, result);
					/* TODO outbuf should also be freed on error */
					free(outbuf);
                    if (result < 0)
                        goto stall;
                    if (verbose > 1)
                        fprintf(stderr, "done (%d written).\n", result);
                } return;
            default:
		goto stall;
        }

stall:
	if (verbose) {
		fprintf (stderr, "... protocol stall %02x.%02x\n",
			setup->bRequestType, setup->bRequest);
                for (tmp = 0; tmp<5; tmp++) {
                    printf("%d: %s\n", stringtab[tmp].id, stringtab[tmp].s);
                }
        }

	/* non-iso endpoints are stalled by issuing an i/o request
	 * in the "wrong" direction.  ep0 is special only because
	 * the direction isn't fixed.
	 */
	if (setup->bRequestType & USB_DIR_IN)
		result = read (fd, &result, 0);
	else
		result = write (fd, &result, 0);
	if (result != -1)
		fprintf (stderr, "can't stall ep0 for %02x.%02x\n",
			setup->bRequestType, setup->bRequest);
	else if (errno != EL2HLT)
		perror ("ep0 stall");
}

static void signothing (int sig, siginfo_t *info, void *ptr)
{
    unsigned int seconds = 4;
    switch (sig) {
        case SIGQUIT:
            seconds /= 2;
        case SIGINT:
            if (verbose > 1)
                fprintf (stderr, "\nInitializing shutdown, "
                        "waiting %d seconds for threads to terminate\n",
                        seconds);
            pthread_cancel(ep0);
            sleep(seconds);
            fprintf (stderr, "Doing immediate shutdown.\n");
            exit(1);
            break;
        default:
            if (verbose > 1)
                fprintf (stderr, "Received unhandled signal %u\n",
                        sig);
    }
}

static const char *speed (enum usb_device_speed s)
{
	switch (s) {
	case USB_SPEED_LOW:	return "low speed";
	case USB_SPEED_FULL:	return "full speed";
	case USB_SPEED_HIGH:	return "high speed";
	default:		return "UNKNOWN speed";
	}
}

/*-------------------------------------------------------------------------*/

/* control thread, handles main event loop  */

#define	NEVENT		5
#define	LOGDELAY	(15 * 60)	/* seconds before stdout timestamp */

static void *ep0_thread (void *param)
{
	int			fd = *(int*) param;
	struct sigaction	action;
	time_t			now, last;
	struct pollfd		ep0_poll;

	interrupt_thread = ccid_thread = ep0 = pthread_self ();
	pthread_cleanup_push (stop_io, NULL);
	pthread_cleanup_push (close_fd, param);

	/* REVISIT signal handling ... normally one pthread should
	 * be doing sigwait() to handle all async signals.
	 */
	action.sa_sigaction = signothing;
	sigfillset (&action.sa_mask);
	action.sa_flags = SA_SIGINFO;
	if (sigaction (SIGINT, &action, NULL) < 0) {
		perror ("SIGINT");
		return 0;
	}
	if (sigaction (SIGQUIT, &action, NULL) < 0) {
		perror ("SIGQUIT");
		return 0;
	}

	ep0_poll.fd = fd;
	ep0_poll.events = POLLIN | POLLOUT | POLLHUP;

	/* event loop */
	last = 0;
	for (;;) {
		int				tmp;
		struct usb_gadgetfs_event	event [NEVENT];
		int				connected = 0;
		int				i, nevent;

		/* Use poll() to test that mechanism, to generate
		 * activity timestamps, and to make it easier to
		 * tweak this code to work without pthreads.  When
		 * AIO is needed without pthreads, ep0 can be driven
		 * instead using SIGIO.
		 */
		tmp = poll(&ep0_poll, 1, -1);
		if (verbose) {
			time (&now);
			if ((now - last) > LOGDELAY) {
				char		timebuf[26];

				last = now;
				ctime_r (&now, timebuf);
				printf ("\n** %s", timebuf);
			}
		}
		if (tmp < 0) {
			/* exit path includes EINTR exits */
			perror("poll");
			break;
		}

		tmp = read (fd, &event, sizeof event);
		if (tmp < 0) {
			if (errno == EAGAIN) {
				sleep (1);
				continue;
			}
			perror ("ep0 read after poll");
			goto done;
		}
		nevent = tmp / sizeof event [0];
		if (nevent != 1 && verbose > 1)
			fprintf (stderr, "read %d ep0 events\n",
				nevent);

		for (i = 0; i < nevent; i++) {
			switch (event [i].type) {
			case GADGETFS_NOP:
				if (verbose)
					fprintf (stderr, "NOP\n");
				break;
			case GADGETFS_CONNECT:
				connected = 1;
				current_speed = event [i].u.speed;
				if (verbose)
					fprintf (stderr,
						"CONNECT %s\n",
					    speed (event [i].u.speed));
				break;
			case GADGETFS_SETUP:
				connected = 1;
				handle_control (fd, &event [i].u.setup);
				break;
			case GADGETFS_DISCONNECT:
				connected = 0;
				current_speed = USB_SPEED_UNKNOWN;
				if (verbose)
					fprintf(stderr, "DISCONNECT\n");
				stop_io ();
				break;
			case GADGETFS_SUSPEND:
				// connected = 1;
				if (verbose)
					fprintf (stderr, "SUSPEND\n");
				break;
			default:
				fprintf (stderr,
					"* unhandled event %d\n",
					event [i].type);
			}
		}
		continue;
done:
		fflush (stdout);
		if (connected)
			stop_io ();
		break;
	}
	if (verbose)
		fprintf (stderr, "ep0 done.\n");

	pthread_cleanup_pop (1);
	pthread_cleanup_pop (1);

	fflush (stdout);
	fflush (stderr);
	return 0;
}

int
main (int argc, char **argv)
{
    /*printf("%s:%d\n", __FILE__, __LINE__);*/
    int fd, c, i;

    struct gengetopt_args_info cmdline;


    /* Parse command line */
    if (cmdline_parser (argc, argv, &cmdline) != 0)
        exit(1);
    doiintf = cmdline.interface_arg;
    productid = cmdline.product_arg;
    vendorid = cmdline.vendor_arg;
    verbose = cmdline.verbose_given;
    doint = cmdline.interrupt_flag;
    gadgetfs = cmdline.gadgetfs_arg;


    if (cmdline.info_flag)
        return print_avail(verbose);

    if (ccid_initialize(cmdline.reader_arg, verbose) < 0) {
        fprintf (stderr, "Can't initialize ccid\n");
        return 1;
    }

    if (chdir (gadgetfs) < 0) {
        fprintf (stderr, "Error changing directory to %s\n", gadgetfs);
        return 1;
    }

    if (strncmp(cmdline.serial_arg, "random", strlen("random")) == 0) {
        /* random initial serial number */
        srand ((int) time (0));
        for (i = 0; i < sizeof serial - 1; ) {
            c = rand () % 127;
            if ((('a' <= c && c <= 'z') || ('0' <= c && c <= '9')))
                serial [i++] = c;
        }
        if (verbose)
            fprintf (stderr, "serial=\"%s\"\n", serial);
    } else {
        for (i=0; i<sizeof stringtab/sizeof(struct usb_string); i++) {
            if (stringtab[i].id == STRINGID_SERIAL) {
                stringtab[i].s = cmdline.serial_arg;
                break;
            }
        }
        if (verbose)
            fprintf (stderr, "serial=\"%s\"\n", cmdline.serial_arg);
    }

    if (doiintf) {
        for (i=0; i<sizeof stringtab/sizeof(struct usb_string); i++) {
            if (stringtab[i].id == STRINGID_INTERFACE) {
                stringtab[i].s = doiintf;
                break;
            }
        }
    }

    fd = init_device ();
    if (fd < 0)
        return 1;
    if (debug)
        fprintf (stderr, "%s%s ep0 configured\n", gadgetfs, DEVNAME);

    fflush (stdout);
    fflush (stderr);
    (void) ep0_thread (&fd);
    cmdline_parser_free(&cmdline);
    return 0;
}
