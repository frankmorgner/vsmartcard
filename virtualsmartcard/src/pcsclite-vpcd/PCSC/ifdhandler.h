/*
 * MUSCLE SmartCard Development ( http://www.linuxnet.com )
 *
 * Copyright (C) 1999-2004
 *  David Corcoran <corcoran@linuxnet.com>
 * Copyright (C) 2003-2004
 *  Damien Sauveron <damien.sauveron@labri.fr>
 * Copyright (C) 2002-2011
 *  Ludovic Rousseau <ludovic.rousseau@free.fr>
 *
 * $Id: ifdhandler.h 6413 2012-08-08 09:35:18Z rousseau $
 */

/**
 * @file
 * @defgroup IFDHandler IFDHandler
 * @brief This provides reader specific low-level calls.

The routines specified hereafter will allow you to write an IFD handler
for the PC/SC Lite resource manager. Please use the complement
developer's kit complete with headers and Makefile at:
http://www.musclecard.com/drivers.html

This gives a common API for communication to most readers in a
homogeneous fashion. This document assumes that the driver developer is
experienced with standards such as ISO-7816-(1, 2, 3, 4), EMV and MCT
specifications. For listings of these specifications please access the
above web site. 

@section UsbReaders USB readers

USB readers use the bundle approach so that the reader can be loaded
and unloaded upon automatic detection of the device. The bundle
approach is simple: the actual library is just embedded in a
directory so additional information can be gathered about the device.

A bundle looks like the following:

@verbatim
GenericReader.bundle/
  Contents/
    Info.plist  - XML file describing the reader
    MacOS/      - Driver directory for OS X
    Solaris/    - Driver directory for Solaris
    Linux/      - Driver directory for Linux
    HPUX/       - Driver directory for HPUX
@endverbatim

The @c Info.plist file describes the driver and gives the loader
all the necessary information. The following must be contained in the
@c Info.plist file:

@subsection ifdVendorID

The vendor ID of the USB device.

Example:

@verbatim
    <key>ifdVendorID</key>
    <string>0x04E6</string>
@endverbatim

You may have an OEM of this reader in which an additional @c <string>
can be used like in the below example:

@verbatim
    <key>ifdVendorID</key>
    <array>
      <string>0x04E6</string>
      <string>0x0973</string>
    </array>
@endverbatim

If multiples exist all the other parameters must have a second value
also. You may chose not to support this feature but it is useful when
reader vendors OEM products so you only distribute one driver.


The CCID driver from Ludovic Rousseau
http://pcsclite.alioth.debian.org/ccid.html uses this feature since the
same driver supports many different readers.

@subsection ifdProductID

   The product id of the USB device.

@verbatim
   <key>ifdProductID</key>
   <string>0x3437</string>
@endverbatim

@subsection ifdFriendlyName

   Example:

@verbatim
   <key>ifdFriendlyName</key>
   <string>SCM Microsystems USB Reader</string>
@endverbatim

@subsection CFBundleExecutable

   The executable name which exists in the particular platform's directory.

   Example:

@verbatim
   <key>CFBundleExecutable</key>
   <string>libccid.so.0.4.2</string>
@endverbatim

@subsection ifdCapabilities

   List of capabilities supported by the driver. This is a bit field. Possible values are:

- 0
  No special capabilities
- 1 IFD_GENERATE_HOTPLUG
  The driver supports the hot plug feature.

Complete sample file:

@verbatim
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN"
    "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>English</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundlePackageType</key>
    <string>BNDL</string>
    <key>CFBundleSignature</key>
    <string>????</string>
    <key>CFBundleVersion</key>
    <string>0.0.1d1</string>
    <key>ifdCapabilities</key>
    <string>0x00000000</string>
    <key>ifdProtocolSupport</key>
    <string>0x00000001</string>
    <key>ifdVersionNumber</key>
    <string>0x00000001</string>

    <key>CFBundleExecutable</key>
    <string>libfoobar.so.x.y</string>

    <key>ifdManufacturerString</key>
    <string>Foo bar inc.</string>

    <key>ifdProductString</key>
    <string>Driver for Foobar reader, version x.y</string>

    <key>ifdVendorID</key>
    <string>0x1234</string>

    <key>ifdProductID</key>
    <string>0x5678</string>

    <key>ifdFriendlyName</key>
    <string>Foobar USB reader</string>
</dict>
</plist>
@endverbatim

As indicated in the XML file the DTD is available at
http://www.apple.com/DTDs/PropertyList-1.0.dtd. 

@section SerialReaders Serial readers

Serial drivers must be configured to operate on a particular port and
respond to a particular name. The @c reader.conf file is used for this
purpose.

It has the following syntax:

@verbatim
# Configuration file for pcsc-lite
# David Corcoran <corcoran@musclecard.com>

FRIENDLYNAME  Generic Reader
DEVICENAME    /dev/ttyS0
LIBPATH       /usr/lib/pcsc/drivers/libgen_ifd.so
CHANNELID     1
@endverbatim

The pound sign # denotes a comment.

@subsection FRIENDLYNAME
The FRIENDLYNAME field is an arbitrary text used to identify the reader.
This text is displayed by commands like @c pcsc_scan
http://ludovic.rousseau.free.fr/softwares/pcsc-tools/ that prints the
names of all the connected and detected readers.

@subsection DEVICENAME
The DEVICENAME field was not used for old drivers (using the IFD handler
version 2.0 or previous). It is now (IFD handler version 3.0) used to
identify the physical port on which the reader is connected.  This is
the device name of this port. It is dependent of the OS kernel. For
example the first serial port device is called @c /dev/ttyS0 under Linux
and @c /dev/cuaa0 under FreeBSD.

If you want to use IFDHCreateChannel() instead of
IFDHCreateChannelByName() then do not use any DEVICENAME line in the
configuration file.  IFDHCreateChannel() will then be called with the
CHANNELID parameter.

@subsection LIBPATH
The LIBPATH field is the filename of the driver code. The driver is a
dynamically loaded piece of code (generally a @c drivername.so* file).

@subsection CHANNELID
The CHANNELID is no more used for recent drivers (IFD handler 3.0) and
has been superseded by DEVICENAME.

If you have an old driver this field is used to indicate the port to
use. You should read your driver documentation to know what information
is needed here. It should be the serial port number for a serial reader.

CHANNELID was the numeric version of the port in which the reader will
be located. This may be done by a symbolic link where @c /dev/pcsc/1 is
the first device which may be a symbolic link to @c /dev/ttyS0 or
whichever location your reader resides.

 */

#ifndef _ifd_handler_h_
#define _ifd_handler_h_

#include <pcsclite.h>

	/*
	 * List of data structures available to ifdhandler
	 */
	typedef struct _DEVICE_CAPABILITIES
	{
		LPSTR Vendor_Name;		/**< Tag 0x0100 */
		LPSTR IFD_Type;			/**< Tag 0x0101 */
		DWORD IFD_Version;		/**< Tag 0x0102 */
		LPSTR IFD_Serial;		/**< Tag 0x0103 */
		DWORD IFD_Channel_ID;	/**< Tag 0x0110 */

		DWORD Asynch_Supported;	/**< Tag 0x0120 */
		DWORD Default_Clock;	/**< Tag 0x0121 */
		DWORD Max_Clock;		/**< Tag 0x0122 */
		DWORD Default_Data_Rate;	/**< Tag 0x0123 */
		DWORD Max_Data_Rate;	/**< Tag 0x0124 */
		DWORD Max_IFSD;			/**< Tag 0x0125 */
		DWORD Synch_Supported;	/**< Tag 0x0126 */
		DWORD Power_Mgmt;		/**< Tag 0x0131 */
		DWORD Card_Auth_Devices;	/**< Tag 0x0140 */
		DWORD User_Auth_Device;	/**< Tag 0x0142 */
		DWORD Mechanics_Supported;	/**< Tag 0x0150 */
		DWORD Vendor_Features;	/**< Tag 0x0180 - 0x01F0 User Defined. */
	}
	DEVICE_CAPABILITIES, *PDEVICE_CAPABILITIES;

	typedef struct _ICC_STATE
	{
		UCHAR ICC_Presence;		/**< Tag 0x0300 */
		UCHAR ICC_Interface_Status;	/**< Tag 0x0301 */
		UCHAR ATR[MAX_ATR_SIZE];	/**< Tag 0x0303 */
		UCHAR ICC_Type;			/**< Tag 0x0304 */
	}
	ICC_STATE, *PICC_STATE;

	typedef struct _PROTOCOL_OPTIONS
	{
		DWORD Protocol_Type;	/**< Tag 0x0201 */
		DWORD Current_Clock;	/**< Tag 0x0202 */
		DWORD Current_F;		/**< Tag 0x0203 */
		DWORD Current_D;		/**< Tag 0x0204 */
		DWORD Current_N;		/**< Tag 0x0205 */
		DWORD Current_W;		/**< Tag 0x0206 */
		DWORD Current_IFSC;		/**< Tag 0x0207 */
		DWORD Current_IFSD;		/**< Tag 0x0208 */
		DWORD Current_BWT;		/**< Tag 0x0209 */
		DWORD Current_CWT;		/**< Tag 0x020A */
		DWORD Current_EBC;		/**< Tag 0x020B */
	}
	PROTOCOL_OPTIONS, *PPROTOCOL_OPTIONS;

	/**
	 * Use by SCardTransmit()
	 */
	typedef struct _SCARD_IO_HEADER
	{
		DWORD Protocol;
		DWORD Length;
	}
	SCARD_IO_HEADER, *PSCARD_IO_HEADER;

	/*
	 * The list of tags should be alot more but this is all I use in the
	 * meantime
	 */
#define TAG_IFD_ATR                     0x0303	/**< ATR */
#define TAG_IFD_SLOTNUM                 0x0180	/**< select a slot */
#define TAG_IFD_SLOT_THREAD_SAFE        0x0FAC	/**< support access to different slots of the reader */
#define TAG_IFD_THREAD_SAFE             0x0FAD	/**< driver is thread safe */
#define TAG_IFD_SLOTS_NUMBER            0x0FAE	/**< number of slots of the reader */
#define TAG_IFD_SIMULTANEOUS_ACCESS     0x0FAF	/**< number of reader the driver can manage */
#define TAG_IFD_POLLING_THREAD          0x0FB0	/**< not used. See TAG_IFD_POLLING_THREAD_WITH_TIMEOUT */
#define TAG_IFD_POLLING_THREAD_KILLABLE 0x0FB1	/**< the polling thread can be killed */
#define TAG_IFD_STOP_POLLING_THREAD     0x0FB2	/**< method used to stop the polling thread (instead of just pthread_kill()) */
#define TAG_IFD_POLLING_THREAD_WITH_TIMEOUT 0x0FB3	/**< driver uses a polling thread with a timeout parameter */

	/*
	 * IFD Handler version number enummerations
	 */
#define IFD_HVERSION_1_0               0x00010000
#define IFD_HVERSION_2_0               0x00020000
#define IFD_HVERSION_3_0               0x00030000

	/*
	 * List of defines available to ifdhandler
	 */
#define IFD_POWER_UP			500 /**< power up the card */
#define IFD_POWER_DOWN			501 /**< power down the card */
#define IFD_RESET			502 /**< warm reset */

#define IFD_NEGOTIATE_PTS1		1   /**< negotiate PTS1 */
#define IFD_NEGOTIATE_PTS2		2   /**< negotiate PTS2 */
#define IFD_NEGOTIATE_PTS3              4   /**< negotiate PTS3 */

#define	IFD_SUCCESS			0   /**< no error */
#define IFD_ERROR_TAG			600 /**< tag unknown */
#define IFD_ERROR_SET_FAILURE		601 /**< set failed */
#define IFD_ERROR_VALUE_READ_ONLY	602 /**< value is read only */
#define IFD_ERROR_PTS_FAILURE		605 /**< failed to negotiate PTS */
#define IFD_ERROR_NOT_SUPPORTED		606
#define IFD_PROTOCOL_NOT_SUPPORTED	607 /**< requested protocol not supported */
#define IFD_ERROR_POWER_ACTION		608 /**< power up failed */
#define IFD_ERROR_SWALLOW		609
#define IFD_ERROR_EJECT			610
#define IFD_ERROR_CONFISCATE		611
#define IFD_COMMUNICATION_ERROR		612 /**< generic error */
#define IFD_RESPONSE_TIMEOUT		613 /**< timeout */
#define IFD_NOT_SUPPORTED		614 /**< request is not supported */
#define IFD_ICC_PRESENT			615 /**< card is present */
#define IFD_ICC_NOT_PRESENT		616 /**< card is absent */
/**
 * The \ref IFD_NO_SUCH_DEVICE error must be returned by the driver when
 * it detects the reader is no more present. This will tell pcscd to
 * remove the reader from the list of available readers.
 */
#define IFD_NO_SUCH_DEVICE		617
#define IFD_ERROR_INSUFFICIENT_BUFFER	618 /**< buffer is too small */

#ifndef RESPONSECODE_DEFINED_IN_WINTYPES_H
	typedef long RESPONSECODE;
#endif

	/*
	 * If you want to compile a V2.0 IFDHandler, define IFDHANDLERv2
	 * before you include this file.
	 *
	 * By default it is setup for for most recent version of the API (V3.0)
	 */

#ifndef IFDHANDLERv2

	/*
	 * List of Defined Functions Available to IFD_Handler 3.0
	 *
	 * All the functions of IFD_Handler 2.0 are available
	 * IFDHCreateChannelByName() is new
	 * IFDHControl() API changed
	 */

/**
This function is required to open a communications channel to the port
listed by @p DeviceName.

Once the channel is opened the reader must be in a state in which it is
possible to query IFDHICCPresence() for card status.

@ingroup IFDHandler
@param[in] Lun Logical Unit Number\n
  Use this for multiple card slots or multiple readers. 0xXXXXYYYY -
  XXXX multiple readers, YYYY multiple slots. The resource manager will
  set these automatically. By default the resource manager loads a new
  instance of the driver so if your reader does not have more than one
  smart card slot then ignore the Lun in all the functions.\n
  \n
  PC/SC supports the loading of multiple readers through one instance of
  the driver in which XXXX is important. XXXX identifies the unique
  reader in which the driver communicates to. The driver should set up
  an array of structures that asociate this XXXX with the underlying
  details of the particular reader.

@param[in] DeviceName Filename to use by the driver.\n
  For drivers configured by @p /etc/reader.conf this is the value of the
  field \ref DEVICENAME.
  \n
  For USB drivers the @p DeviceName must start with @p usb:VID/PID. VID
  is the Vendor ID and PID is the Product ID. Both are a 4-digits hex
  number.

Typically the string is generated by:

@code
printf("usb:%04x/%04x", idVendor, idProduct);
@endcode

The @p DeviceName string may also contain a more specialised
identification string. This additional information is used to
differentiate between two identical readers connected at the same time.
In this case the driver can't differentiate the two readers using VID
and PID and must use some additional information identifying the USB
port used by each reader.

- libusb

  For USB drivers using libusb-1.0 http://libusb.sourceforge.net/ for USB
  abstraction the @p DeviceName the string may be generated by:

  @code
  printf("usb:%04x/%04x:libusb-1.0:%d:%d:%d",
    idVendor, idProduct, bus_number, device_address, interface)
  @endcode

  So it is something like: <tt>usb:08e6/3437:libusb-1.0:7:99:0</tt> under
  GNU/Linux.

- libudev

  If pcscd is compiled with libudev support instead of libusb (default
  since pcsc-lite 1.6.8) the string will look like:

  @code
  printf("usb:%04x/%04x:libudev:%d:%s", idVendor, idProduct,
		bInterfaceNumber, devpath);
  @endcode

  bInterfaceNumber is the number of the interface on the device. It is
  only usefull for devices with more than one CCID interface.

  devpath is the filename of the device on the file system.

  So it is something like:
  <tt>usb:08e6/3437:libudev:0:/dev/bus/usb/008/047</tt>
  under GNU/Linux.

- other

  If the driver does not understand the <tt>:libusb:</tt> or
  <tt>:libudev:</tt> scheme or if a new scheme is used, the driver should
  ignore the part it does not understand instead of failing.

  The driver shall recognize the <tt>usb:VID/PID</tt> part and, only if
  possible, the remaining of the DeviceName field.

  It is the responsibility of the driver to correctly identify the reader.

@return Error codes
@retval IFD_SUCCESS Successful (\ref IFD_SUCCESS)
@retval IFD_COMMUNICATION_ERROR Error has occurred (\ref IFD_COMMUNICATION_ERROR)
@retval IFD_NO_SUCH_DEVICE The reader is no more present (\ref IFD_NO_SUCH_DEVICE)
  */
RESPONSECODE IFDHCreateChannelByName(DWORD Lun, LPSTR DeviceName);

/**
This function performs a data exchange with the reader (not the card)
specified by Lun. It is responsible for abstracting functionality such
as PIN pads, biometrics, LCD panels, etc. You should follow the MCT and
CTBCS specifications for a list of accepted commands to implement. This
function is fully voluntary and does not have to be implemented unless
you want extended functionality.

@ingroup IFDHandler
@param[in] Lun Logical Unit Number
@param[in] dwControlCode Control code for the operation\n
  This value identifies the specific operation to be performed. This
  value is driver specific.
@param[in] TxBuffer Transmit data
@param[in] TxLength Length of this buffer
@param[out] RxBuffer Receive data
@param[in] RxLength Length of the response buffer
@param[out] pdwBytesReturned Length of response\n
  This function will be passed the length of the buffer RxBuffer in
  RxLength and it must set the length of the received data in
  pdwBytesReturned.

@note
  @p *pdwBytesReturned should be set to zero on error. 

@return Error codes
@retval IFD_SUCCESS Successful (\ref IFD_SUCCESS)
@retval IFD_COMMUNICATION_ERROR Error has occurred (\ref IFD_COMMUNICATION_ERROR)
@retval IFD_RESPONSE_TIMEOUT The response timed out (\ref IFD_RESPONSE_TIMEOUT)
@retval IFD_NO_SUCH_DEVICE The reader is no more present (\ref IFD_NO_SUCH_DEVICE)
 */
RESPONSECODE IFDHControl(DWORD Lun, DWORD dwControlCode, PUCHAR
	TxBuffer, DWORD TxLength, PUCHAR RxBuffer, DWORD RxLength,
	LPDWORD pdwBytesReturned);

#else

/**
 * Available in IFD_Handler 2.0
 *
 * @deprecated
 * You should use the new form of IFDHControl()
 */
RESPONSECODE IFDHControl(DWORD Lun, PUCHAR TxBuffer, DWORD TxLength,
	PUCHAR RxBuffer, PDWORD RxLength);

#endif

	/*
	 * common functions in IFD_Handler 2.0 and 3.0
	 */
/**
This function is required to open a communications channel to the port
listed by Channel. For example, the first serial reader on COM1 would
link to @p /dev/pcsc/1 which would be a symbolic link to @p /dev/ttyS0
on some machines This is used to help with inter-machine independence.

On machines with no /dev directory the driver writer may choose to map
their Channel to whatever they feel is appropriate.

Once the channel is opened the reader must be in a state in which it is
possible to query IFDHICCPresence() for card status.

USB readers can ignore the @p Channel parameter and query the USB bus
for the particular reader by manufacturer and product id. 

@ingroup IFDHandler
@param[in] Lun Logical Unit Number\n
  Use this for multiple card slots or multiple readers. 0xXXXXYYYY -
  XXXX multiple readers, YYYY multiple slots. The resource manager will
  set these automatically. By default the resource manager loads a new
  instance of the driver so if your reader does not have more than one
  smart card slot then ignore the Lun in all the functions.\n
  \n
  PC/SC supports the loading of multiple readers through one instance of
  the driver in which XXXX is important. XXXX identifies the unique
  reader in which the driver communicates to. The driver should set up
  an array of structures that associate this XXXX with the underlying
  details of the particular reader.
@param[in] Channel Channel ID
  This is denoted by the following:
  - 0x000001 	@p /dev/pcsc/1
  - 0x000002 	@p /dev/pcsc/2
  - 0x000003 	@p /dev/pcsc/3
  - 0x000004 	@p /dev/pcsc/4

@return Error codes
@retval IFD_SUCCESS Successful (\ref IFD_SUCCESS)
@retval IFD_COMMUNICATION_ERROR Error has occurred (\ref IFD_COMMUNICATION_ERROR)
@retval IFD_NO_SUCH_DEVICE The reader is no more present (\ref IFD_NO_SUCH_DEVICE)

 */
RESPONSECODE IFDHCreateChannel(DWORD Lun, DWORD Channel);

/**
This function should close the reader communication channel for the
particular reader. Prior to closing the communication channel the reader
should make sure the card is powered down and the terminal is also
powered down.

@ingroup IFDHandler
@param[in] Lun Logical Unit Number

@return Error codes
@retval IFD_SUCCESS Successful (\ref IFD_SUCCESS)
@retval IFD_COMMUNICATION_ERROR Error has occurred (\ref IFD_COMMUNICATION_ERROR)
@retval IFD_NO_SUCH_DEVICE The reader is no more present (\ref IFD_NO_SUCH_DEVICE)
  */
RESPONSECODE IFDHCloseChannel(DWORD Lun);

/**
This function should get the slot/card capabilities for a particular
slot/card specified by Lun. Again, if you have only 1 card slot and
don't mind loading a new driver for each reader then ignore Lun.

@ingroup IFDHandler
@param[in] Lun Logical Unit Number
@param[in] Tag Tag of the desired data value
- \ref TAG_IFD_ATR
  Return the ATR and its size (implementation is mandatory).
- \ref TAG_IFD_SLOTNUM
  Unused/deprecated
- \ref SCARD_ATTR_ATR_STRING
  Same as \ref TAG_IFD_ATR but this one is not mandatory. It is defined
  in Microsoft PC/SC SCardGetAttrib().
- \ref TAG_IFD_SIMULTANEOUS_ACCESS
  Return the number of sessions (readers) the driver can handle in
  <tt>Value[0]</tt>.
  This is used for multiple readers sharing the same driver.
- \ref TAG_IFD_THREAD_SAFE
  If the driver supports more than one reader (see
  \ref TAG_IFD_SIMULTANEOUS_ACCESS above) this tag indicates if the
  driver supports access to multiple readers at the same time.\n
  <tt>Value[0] = 1</tt> indicates the driver supports simultaneous accesses.
- \ref TAG_IFD_SLOTS_NUMBER
  Return the number of slots in this reader in <tt>Value[0]</tt>.
- \ref TAG_IFD_SLOT_THREAD_SAFE
  If the reader has more than one slot (see \ref TAG_IFD_SLOTS_NUMBER
  above) this tag indicates if the driver supports access to multiple
  slots of the same reader at the same time.\n
  <tt>Value[0] = 1</tt> indicates the driver supports simultaneous slot
  accesses.
- \ref TAG_IFD_POLLING_THREAD
  Unused/deprecated
- \ref TAG_IFD_POLLING_THREAD_WITH_TIMEOUT
  If the driver provides a polling thread then @p Value is a pointer to
  this function. The function prototype is:
@verbatim
  RESPONSECODE foo(DWORD Lun, int timeout);
@endverbatim
- \ref TAG_IFD_POLLING_THREAD_KILLABLE
  Tell if the polling thread can be killed (pthread_kill()) by pcscd
- \ref TAG_IFD_STOP_POLLING_THREAD
  Returns a pointer in @p Value to the function used to stop the polling
  thread returned by \ref TAG_IFD_POLLING_THREAD_WITH_TIMEOUT. The
  function prototype is:
@verbatim
  RESPONSECODE foo(DWORD Lun);
@endverbatim
@param[in,out] Length Length of the desired data value
@param[out] Value Value of the desired data

@return Error codes
@retval IFD_SUCCESS Successful (\ref IFD_SUCCESS)
@retval IFD_ERROR_TAG Invalid tag given (\ref IFD_ERROR_TAG)
@retval IFD_NO_SUCH_DEVICE The reader is no more present (\ref IFD_NO_SUCH_DEVICE)
 */
RESPONSECODE IFDHGetCapabilities(DWORD Lun, DWORD Tag, PDWORD Length,
	PUCHAR Value);

/**
This function should set the slot/card capabilities for a particular
slot/card specified by @p Lun. Again, if you have only 1 card slot and
don't mind loading a new driver for each reader then ignore @p Lun. 

@ingroup IFDHandler
@param[in] Lun Logical Unit Number
@param[in] Tag Tag of the desired data value
@param[in,out] Length Length of the desired data value
@param[out] Value Value of the desired data

This function is also called when the application uses the PC/SC
SCardGetAttrib() function. The list of supported tags is not limited.

@return Error codes
@retval IFD_SUCCESS Successful (\ref IFD_SUCCESS)
@retval IFD_ERROR_TAG Invalid tag given (\ref IFD_ERROR_TAG)
@retval IFD_ERROR_SET_FAILURE Could not set value (\ref IFD_ERROR_SET_FAILURE)
@retval IFD_ERROR_VALUE_READ_ONLY Trying to set read only value (\ref IFD_ERROR_VALUE_READ_ONLY)
@retval IFD_NO_SUCH_DEVICE The reader is no more present (\ref IFD_NO_SUCH_DEVICE)
 */
RESPONSECODE IFDHSetCapabilities(DWORD Lun, DWORD Tag, DWORD Length, PUCHAR Value);

/**
This function should set the Protocol Type Selection (PTS) of a
particular card/slot using the three PTS parameters sent 

@ingroup IFDHandler
@param[in] Lun Logical Unit Number
@param[in] Protocol Desired protocol
- \ref SCARD_PROTOCOL_T0
  T=0 protocol
- \ref SCARD_PROTOCOL_T1
  T=1 protocol
@param[in] Flags Logical OR of possible values to determine which PTS values
to negotiate 
- \ref IFD_NEGOTIATE_PTS1
- \ref IFD_NEGOTIATE_PTS2
- \ref IFD_NEGOTIATE_PTS3
@param[in] PTS1 1st PTS Value
@param[in] PTS2 2nd PTS Value
@param[in] PTS3 3rd PTS Value\n
  See ISO 7816/EMV documentation.

@return Error codes
@retval IFD_SUCCESS Successful (\ref IFD_SUCCESS)
@retval IFD_ERROR_PTS_FAILURE Could not set PTS value (\ref IFD_ERROR_PTS_FAILURE)
@retval IFD_COMMUNICATION_ERROR Error has occurred (\ref IFD_COMMUNICATION_ERROR)
@retval IFD_PROTOCOL_NOT_SUPPORTED Protocol is not supported (\ref IFD_PROTOCOL_NOT_SUPPORTED)
@retval IFD_NOT_SUPPORTED Action not supported (\ref IFD_NOT_SUPPORTED)
@retval IFD_NO_SUCH_DEVICE The reader is no more present (\ref IFD_NO_SUCH_DEVICE)
 */
RESPONSECODE IFDHSetProtocolParameters(DWORD Lun, DWORD Protocol, UCHAR Flags,
	UCHAR PTS1, UCHAR PTS2, UCHAR PTS3);
/**
This function controls the power and reset signals of the smart card
reader at the particular reader/slot specified by @p Lun.

@ingroup IFDHandler
@param[in] Lun Logical Unit Number
@param[in] Action Action to be taken on the card
- \ref IFD_POWER_UP
  Power up the card (store and return Atr and AtrLength)
- \ref IFD_POWER_DOWN
  Power down the card (Atr and AtrLength should be zeroed)
- \ref IFD_RESET
  Perform a warm reset of the card (no power down). If the card is not powered then power up the card (store and return Atr and AtrLength)
@param[out] Atr Answer to Reset (ATR) of the card\n
  The driver is responsible for caching this value in case
  IFDHGetCapabilities() is called requesting the ATR and its length. The
  ATR length should not exceed \ref MAX_ATR_SIZE.
@param[in,out] AtrLength Length of the ATR\n
  This should not exceed \ref MAX_ATR_SIZE.

@note
Memory cards without an ATR should return \ref IFD_SUCCESS on reset but the
Atr should be zeroed and the length should be zero Reset errors should
return zero for the AtrLength and return \ref IFD_ERROR_POWER_ACTION.

@return Error codes
@retval IFD_SUCCESS Successful (\ref IFD_SUCCESS)
@retval IFD_ERROR_POWER_ACTION Error powering/resetting card (\ref IFD_ERROR_POWER_ACTION)
@retval IFD_COMMUNICATION_ERROR Error has occurred (\ref IFD_COMMUNICATION_ERROR)
@retval IFD_NOT_SUPPORTED Action not supported (\ref IFD_NOT_SUPPORTED)
@retval IFD_NO_SUCH_DEVICE The reader is no more present (\ref IFD_NO_SUCH_DEVICE)
 */
RESPONSECODE IFDHPowerICC(DWORD Lun, DWORD Action, PUCHAR Atr, PDWORD
	AtrLength);

/**
This function performs an APDU exchange with the card/slot specified by
Lun. The driver is responsible for performing any protocol specific
exchanges such as T=0, 1, etc. differences. Calling this function will
abstract all protocol differences.

@ingroup IFDHandler
@param[in] Lun Logical Unit Number
@param[in] SendPci contains two structure members
- Protocol 0, 1, ... 14\n
  T=0 ... T=14
- Length\n
  Not used.
@param[in] TxBuffer Transmit APDU\n
      Example: "\x00\xA4\x00\x00\x02\x3F\x00"
@param[in] TxLength Length of this buffer
@param[out] RxBuffer Receive APDU\n
      Example: "\x61\x14"
@param[in,out] RxLength Length of the received APDU\n
  This function will be passed the size of the buffer of RxBuffer and
  this function is responsible for setting this to the length of the
  received APDU response. This should be ZERO on all errors. The
  resource manager will take responsibility of zeroing out any temporary
  APDU buffers for security reasons.
@param[out] RecvPci contains two structure members
- Protocol - 0, 1, ... 14\n
  T=0 ... T=14
- Length\n
  Not used.

@note
The driver is responsible for knowing what type of card it has. If the
current slot/card contains a memory card then this command should ignore
the Protocol and use the MCT style commands for support for these style
cards and transmit them appropriately. If your reader does not support
memory cards or you don't want to implement this functionality, then
ignore this.
@par
RxLength should be set to zero on error.
@par
The driver is not responsible for doing an automatic Get Response
command for received buffers containing 61 XX.

@return Error codes
@retval IFD_SUCCESS Successful (\ref IFD_SUCCESS)
@retval IFD_COMMUNICATION_ERROR Error has occurred (\ref IFD_COMMUNICATION_ERROR)
@retval IFD_RESPONSE_TIMEOUT The response timed out (\ref IFD_RESPONSE_TIMEOUT)
@retval IFD_ICC_NOT_PRESENT ICC is not present (\ref IFD_ICC_NOT_PRESENT)
@retval IFD_NOT_SUPPORTED Action not supported (\ref IFD_NOT_SUPPORTED)
@retval IFD_NO_SUCH_DEVICE The reader is no more present (\ref IFD_NO_SUCH_DEVICE)
 */
RESPONSECODE IFDHTransmitToICC(DWORD Lun, SCARD_IO_HEADER SendPci,
	PUCHAR TxBuffer, DWORD TxLength, PUCHAR RxBuffer, PDWORD
	RxLength, PSCARD_IO_HEADER RecvPci);

/**
This function returns the status of the card inserted in the reader/slot
specified by @p Lun. In cases where the device supports asynchronous
card insertion/removal detection, it is advised that the driver manages
this through a thread so the driver does not have to send and receive a
command each time this function is called. 

@ingroup IFDHandler
@param[in] Lun Logical Unit Number

@return Error codes
@retval IFD_SUCCESS Successful (\ref IFD_SUCCESS)
@retval IFD_COMMUNICATION_ERROR Error has occurred (\ref IFD_COMMUNICATION_ERROR)
@retval IFD_ICC_NOT_PRESENT ICC is not present (\ref IFD_ICC_NOT_PRESENT)
@retval IFD_NO_SUCH_DEVICE The reader is no more present (\ref IFD_NO_SUCH_DEVICE)
 */
RESPONSECODE IFDHICCPresence(DWORD Lun);

#endif
