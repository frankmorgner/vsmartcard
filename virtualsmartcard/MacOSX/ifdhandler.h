/*
 * MUSCLE SmartCard Development ( http://www.linuxnet.com )
 *
 * Copyright (C) 1999-2004
 *  David Corcoran <corcoran@linuxnet.com>
 *  Damien Sauveron <damien.sauveron@labri.fr>
 *
 * $Id: ifdhandler.h 3029 2008-06-26 13:58:52Z rousseau $
 */
/**
 * @file
 * @brief This provides reader specific low-level calls.
 */
#ifndef _ifd_handler_h_
#define _ifd_handler_h_
#include <pcsclite.h>
#ifdef __cplusplus
extern "C"
{
#endif
        /*
         * List of data structures available to ifdhandler
         */
        typedef struct _DEVICE_CAPABILITIES
        {
                LPSTR Vendor_Name;              /**< Tag 0x0100 */
                LPSTR IFD_Type;                 /**< Tag 0x0101 */
                DWORD IFD_Version;              /**< Tag 0x0102 */
                LPSTR IFD_Serial;               /**< Tag 0x0103 */
                DWORD IFD_Channel_ID;   /**< Tag 0x0110 */
                DWORD Asynch_Supported; /**< Tag 0x0120 */
                DWORD Default_Clock;    /**< Tag 0x0121 */
                DWORD Max_Clock;                /**< Tag 0x0122 */
                DWORD Default_Data_Rate;        /**< Tag 0x0123 */
                DWORD Max_Data_Rate;    /**< Tag 0x0124 */
                DWORD Max_IFSD;                 /**< Tag 0x0125 */
                DWORD Synch_Supported;  /**< Tag 0x0126 */
                DWORD Power_Mgmt;               /**< Tag 0x0131 */
                DWORD Card_Auth_Devices;        /**< Tag 0x0140 */
                DWORD User_Auth_Device; /**< Tag 0x0142 */
                DWORD Mechanics_Supported;      /**< Tag 0x0150 */
                DWORD Vendor_Features;  /**< Tag 0x0180 - 0x01F0 User Defined. */
        }
        DEVICE_CAPABILITIES, *PDEVICE_CAPABILITIES;
        typedef struct _ICC_STATE
        {
                UCHAR ICC_Presence;             /**< Tag 0x0300 */
                UCHAR ICC_Interface_Status;     /**< Tag 0x0301 */
                UCHAR ATR[MAX_ATR_SIZE];        /**< Tag 0x0303 */
                UCHAR ICC_Type;                 /**< Tag 0x0304 */
        }
        ICC_STATE, *PICC_STATE;
        typedef struct _PROTOCOL_OPTIONS
        {
                DWORD Protocol_Type;    /**< Tag 0x0201 */
                DWORD Current_Clock;    /**< Tag 0x0202 */
                DWORD Current_F;                /**< Tag 0x0203 */
                DWORD Current_D;                /**< Tag 0x0204 */
                DWORD Current_N;                /**< Tag 0x0205 */
                DWORD Current_W;                /**< Tag 0x0206 */
                DWORD Current_IFSC;             /**< Tag 0x0207 */
                DWORD Current_IFSD;             /**< Tag 0x0208 */
                DWORD Current_BWT;              /**< Tag 0x0209 */
                DWORD Current_CWT;              /**< Tag 0x020A */
                DWORD Current_EBC;              /**< Tag 0x020B */
        }
        PROTOCOL_OPTIONS, *PPROTOCOL_OPTIONS;
        typedef struct _SCARD_IO_HEADER
        {
                DWORD Protocol;
                DWORD Length;
        }
        SCARD_IO_HEADER, *PSCARD_IO_HEADER;
        /*
         * End of structure list
         */
        /*
         * The list of tags should be alot more but this is all I use in the
         * meantime
         */
#define TAG_IFD_ATR                     0x0303
#define TAG_IFD_SLOTNUM                 0x0180
#define TAG_IFD_SLOT_THREAD_SAFE        0x0FAC
#define TAG_IFD_THREAD_SAFE             0x0FAD
#define TAG_IFD_SLOTS_NUMBER            0x0FAE
#define TAG_IFD_SIMULTANEOUS_ACCESS     0x0FAF
#define TAG_IFD_POLLING_THREAD          0x0FB0
#define TAG_IFD_POLLING_THREAD_KILLABLE 0x0FB1
        /*
         * End of tag list
         */
        /*
         * IFD Handler version number enummerations
         */
#define IFD_HVERSION_1_0               0x00010000
#define IFD_HVERSION_2_0               0x00020000
#define IFD_HVERSION_3_0               0x00030000
        /*
         * End of version number enummerations
         */
        /*
         * List of defines available to ifdhandler
         */
#define IFD_POWER_UP                    500
#define IFD_POWER_DOWN                  501
#define IFD_RESET                       502
#define IFD_NEGOTIATE_PTS1              1
#define IFD_NEGOTIATE_PTS2              2
#define IFD_NEGOTIATE_PTS3              4
#define IFD_SUCCESS                     0
#define IFD_ERROR_TAG                   600
#define IFD_ERROR_SET_FAILURE           601
#define IFD_ERROR_VALUE_READ_ONLY       602
#define IFD_ERROR_PTS_FAILURE           605
#define IFD_ERROR_NOT_SUPPORTED         606
#define IFD_PROTOCOL_NOT_SUPPORTED      607
#define IFD_ERROR_POWER_ACTION          608
#define IFD_ERROR_SWALLOW               609
#define IFD_ERROR_EJECT                 610
#define IFD_ERROR_CONFISCATE            611
#define IFD_COMMUNICATION_ERROR         612
#define IFD_RESPONSE_TIMEOUT            613
#define IFD_NOT_SUPPORTED               614
#define IFD_ICC_PRESENT                 615
#define IFD_ICC_NOT_PRESENT             616
#define IFD_NO_SUCH_DEVICE              617
#ifndef RESPONSECODE_DEFINED_IN_WINTYPES_H
        typedef long RESPONSECODE;
#endif
        /*
         * If you want to compile a V2.0 IFDHandler, define IFDHANDLERv2 before you
         * include this file.
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
        RESPONSECODE IFDHCreateChannelByName(DWORD, LPSTR);
        RESPONSECODE IFDHControl(DWORD, DWORD, PUCHAR, DWORD, PUCHAR,
                DWORD, LPDWORD);
#else
        /*
         * List of Defined Functions Available to IFD_Handler 2.0
         */
        RESPONSECODE IFDHControl(DWORD, PUCHAR, DWORD, PUCHAR, PDWORD);
#endif
        /*
         * common functions in IFD_Handler 2.0 and 3.0
         */
        RESPONSECODE IFDHCreateChannel(DWORD, DWORD);
        RESPONSECODE IFDHCloseChannel(DWORD);
        RESPONSECODE IFDHGetCapabilities(DWORD, DWORD, PDWORD, PUCHAR);
        RESPONSECODE IFDHSetCapabilities(DWORD, DWORD, DWORD, PUCHAR);
        RESPONSECODE IFDHSetProtocolParameters(DWORD, DWORD, UCHAR,
                UCHAR, UCHAR, UCHAR);
        RESPONSECODE IFDHPowerICC(DWORD, DWORD, PUCHAR, PDWORD);
        RESPONSECODE IFDHTransmitToICC(DWORD, SCARD_IO_HEADER, PUCHAR,
                DWORD, PUCHAR, PDWORD, PSCARD_IO_HEADER);
        RESPONSECODE IFDHICCPresence(DWORD);
        /*
         * List of Defined Functions Available to IFD_Handler 1.0
         */
        RESPONSECODE IO_Create_Channel(DWORD);
        RESPONSECODE IO_Close_Channel(void);
        RESPONSECODE IFD_Get_Capabilities(DWORD, PUCHAR);
        RESPONSECODE IFD_Set_Capabilities(DWORD, PUCHAR);
        RESPONSECODE IFD_Set_Protocol_Parameters(DWORD, UCHAR, UCHAR,
                UCHAR, UCHAR);
        RESPONSECODE IFD_Power_ICC(DWORD);
        RESPONSECODE IFD_Swallow_ICC(void);
        RESPONSECODE IFD_Eject_ICC(void);
        RESPONSECODE IFD_Confiscate_ICC(void);
        RESPONSECODE IFD_Transmit_to_ICC(SCARD_IO_HEADER, PUCHAR, DWORD,
                PUCHAR, PDWORD, PSCARD_IO_HEADER);
        RESPONSECODE IFD_Is_ICC_Present(void);
        RESPONSECODE IFD_Is_ICC_Absent(void);
#ifdef __cplusplus
}
#endif
#endif
