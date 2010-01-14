/*
 * Copyright (C) 2009 Frank Morgner
 *
 * This file is part of ccid.
 *
 * ccid is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * ccid is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ccid.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _VPCD_H_
#define _VPCD_H_

#include <linux/usb/ch9.h>
#include <winscard.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef USB_REQ_CCID
#define USB_REQ_CCID        0xA1
#define CCID_CONTROL_ABORT                  0x01
#define CCID_CONTROL_GET_CLOCK_FREQUENCIES  0x02
#define CCID_CONTROL_GET_DATA_RATES 0x03
struct ccid_class_descriptor {
    __u8   bLength;
    __u8   bDescriptorType;
    __le16 bcdCCID;
    __u8   bMaxSlotIndex;
    __u8   bVoltageSupport;
    __le32 dwProtocols;
    __le32 dwDefaultClock;
    __le32 dwMaximumClock;
    __u8   bNumClockSupport;
    __le32 dwDataRate;
    __le32 dwMaxDataRate;
    __u8   bNumDataRatesSupported;
    __le32 dwMaxIFSD;
    __le32 dwSynchProtocols;
    __le32 dwMechanical;
    __le32 dwFeatures;
    __le32 dwMaxCCIDMessageLength;
    __u8   bClassGetResponse;
    __u8   bclassEnvelope;
    __le16 wLcdLayout;
    __u8   bPINSupport;
    __u8   bMaxCCIDBusySlots;
} __attribute__ ((packed));

typedef struct {
    __u8 bmFindexDindex;
    __u8 bmTCCKST0;
    __u8 bGuardTimeT0;
    __u8 bWaitingIntegerT0;
    __u8 bClockStop;
} __attribute__ ((packed)) abProtocolDataStructure_T0_t;
typedef struct {
    __u8 bmFindexDindex;
    __u8 bmTCCKST1;
    __u8 bGuardTimeT1;
    __u8 bWaitingIntegersT1;
    __u8 bClockStop;
    __u8 bIFSC;
    __u8 bNadValue;
} __attribute__ ((packed)) abProtocolDataStructure_T1_t;

typedef struct {
    __u8   bTimeOut;
    __u8   bmFormatString;
    __u8   bmPINBlockString;
    __u8   bmPINLengthFormat;
    __le16 wPINMaxExtraDigit;
    __u8   bEntryValidationCondition;
    __u8   bNumberMessage;
    __le16 wLangId;
    __u8   bMsgIndex;
    __u8   bTeoPrologue1;
    __le16 bTeoPrologue2;
} __attribute__ ((packed)) abPINDataStucture_Verification_t;
typedef struct {
    __u8   bTimeOut;
    __u8   bmFormatString;
    __u8   bmPINBlockString;
    __u8   bmPINLengthFormat;
    __u8   bInsertionOffsetOld;
    __u8   bInsertionOffsetNew;
    __le16 wPINMaxExtraDigit;
    __u8   bConfirmPIN;
    __u8   bEntryValidationCondition;
    __u8   bNumberMessage;
    __le16 wLangId;
    __u8   bMsgIndex1;
    __u8   bMsgIndex2;
    __u8   bMsgIndex3;
    __u8   bTeoPrologue1;
    __le16 bTeoPrologue2;
} __attribute__ ((packed)) abPINDataStucture_Modification_t;

typedef struct {
    __u8   bMessageType;
    __le32 dwLength;
    __u8   bSlot;
    __u8   bSeq;
    __u8   bBWI;
    __le16 wLevelParameter;
} __attribute__ ((packed)) PC_to_RDR_XfrBlock_t;
typedef struct {
    __u8   bMessageType;
    __le32 dwLength;
    __u8   bSlot;
    __u8   bSeq;
    __u8   abRFU1;
    __le16 abRFU2;
} __attribute__ ((packed)) PC_to_RDR_IccPowerOff_t;
typedef struct {
    __u8   bMessageType;
    __le32 dwLength;
    __u8   bSlot;
    __u8   bSeq;
    __u8   abRFU1;
    __le16 abRFU2;
} __attribute__ ((packed)) PC_to_RDR_GetSlotStatus_t;
typedef struct {
    __u8   bMessageType;
    __le32 dwLength;
    __u8   bSlot;
    __u8   bSeq;
    __u8   abRFU1;
    __le16 abRFU2;
} __attribute__ ((packed)) PC_to_RDR_GetParameters_t;
typedef struct {
    __u8   bMessageType;
    __le32 dwLength;
    __u8   bSlot;
    __u8   bSeq;
    __u8   abRFU1;
    __le16 abRFU2;
} __attribute__ ((packed)) PC_to_RDR_ResetParameters_t;
typedef struct {
    __u8   bMessageType;
    __le32 dwLength;
    __u8   bSlot;
    __u8   bSeq;
    __u8   bProtocolNum;
    __le16 abRFU;
} __attribute__ ((packed)) PC_to_RDR_SetParameters_t;
typedef struct {
    __u8   bMessageType;
    __le32 dwLength;
    __u8   bSlot;
    __u8   bSeq;
    __u8   bBWI;
    __le16 wLevelParameter;
} __attribute__ ((packed)) PC_to_RDR_Secure_t;
typedef struct {
    __u8   bMessageType;
    __le32 dwLength;
    __u8   bSlot;
    __u8   bSeq;
    __u8   bPowerSelect;
    __le16 abRFU;
} __attribute__ ((packed)) PC_to_RDR_IccPowerOn_t;

typedef struct {
    __u8   bMessageType;
    __le32 dwLength;
    __u8   bSlot;
    __u8   bSeq;
    __u8   bStatus;
    __u8   bError;
    __u8   bClockStatus;
} __attribute__ ((packed)) RDR_to_PC_SlotStatus_t;
typedef struct {
    __u8   bMessageType;
    __le32 dwLength;
    __u8   bSlot;
    __u8   bSeq;
    __u8   bStatus;
    __u8   bError;
    __u8   bChainParameter;
} __attribute__ ((packed)) RDR_to_PC_DataBlock_t;
typedef struct {
    __u8   bMessageType;
    __le32 dwLength;
    __u8   bSlot;
    __u8   bSeq;
    __u8   bStatus;
    __u8   bError;
    __u8   bProtocolNum;
} __attribute__ ((packed)) RDR_to_PC_Parameters_t;
typedef struct {
    __u8   bMessageType;
    __u8   bmSlotICCState;
} __attribute__ ((packed)) RDR_to_PC_NotifySlotChange_t;
#endif

struct hid_class_descriptor {
    __u8   bLength;
    __u8   bDescriptorType;
    __le16 bcdHID;
    __le32 bCountryCode;
    __u8   bNumDescriptors;
} __attribute__ ((packed));


static struct ccid_class_descriptor
ccid_desc = {
    .bLength                = sizeof ccid_desc,
    .bDescriptorType        = 0x21,
    .bcdCCID                = __constant_cpu_to_le16(0x0110),
    .bMaxSlotIndex          = 0,
    .bVoltageSupport        = 0x01,
    .dwProtocols            = __constant_cpu_to_le32(0x01|     // T=0
                              0x02),     // T=1
    .dwDefaultClock         = __constant_cpu_to_le32(0xDFC),
    .dwMaximumClock         = __constant_cpu_to_le32(0xDFC),
    .bNumClockSupport       = 1,
    .dwDataRate             = __constant_cpu_to_le32(0x2580),
    .dwMaxDataRate          = __constant_cpu_to_le32(0x2580),
    .bNumDataRatesSupported = 1,
    .dwMaxIFSD              = __constant_cpu_to_le32(0xFF),     // FIXME
    .dwSynchProtocols       = __constant_cpu_to_le32(0),
    .dwMechanical           = __constant_cpu_to_le32(0),
    .dwFeatures             = __constant_cpu_to_le32(
                              0x2|      // Automatic parameter configuration based on ATR data
                              0x8|      // Automatic ICC voltage selection
                              0x10|     // Automatic ICC clock frequency change
                              0x20|     // Automatic baud rate change
                              0x40|     // Automatic parameters negotiation
                              0x80|     // Automatic PPS   
                              0x20000|  // Short APDU level exchange
                              0x100000),// USB Wake up signaling supported
    .dwMaxCCIDMessageLength = __constant_cpu_to_le32(261+10),
    .bClassGetResponse      = 0xFF,
    .bclassEnvelope         = 0xFF,
    .wLcdLayout             = __constant_cpu_to_le16(
                              //0),
                              0xFF00|   // Number of lines for the LCD display
                              0x00FF),  // Number of characters per line
    //.bPINSupport            = 0,
    .bPINSupport            = 0x1|      // PIN Verification supported
                              0x2,      // PIN Modification supported
    .bMaxCCIDBusySlots      = 0x01,
};
char* perform_initialization(int num);
int perform_shutdown();

__u8 get_bError(LONG pcsc_result);
__u8 get_bStatus(LONG pcsc_result);
RDR_to_PC_SlotStatus_t get_RDR_to_PC_SlotStatus(__u8 bSlot, __u8 bSeq,
        LONG pcsc_result);
RDR_to_PC_DataBlock_t get_RDR_to_PC_DataBlock(__u8 bSlot, __u8 bSeq,
        LONG pcsc_result, __le32 dwLength);
RDR_to_PC_SlotStatus_t perform_PC_to_RDR_GetSlotStatus(
        const PC_to_RDR_GetSlotStatus_t request);
RDR_to_PC_SlotStatus_t perform_PC_to_RDR_GetSlotStatus(
        const PC_to_RDR_GetSlotStatus_t request);
RDR_to_PC_SlotStatus_t perform_PC_to_RDR_IccPowerOn(
        const PC_to_RDR_IccPowerOn_t request, char ** pATR);
RDR_to_PC_SlotStatus_t perform_PC_to_RDR_IccPowerOff(
        const PC_to_RDR_IccPowerOff_t request);
RDR_to_PC_DataBlock_t perform_PC_to_RDR_XfrBlock(
        const PC_to_RDR_XfrBlock_t request, const __u8*
        abDataIn, __u8** abDataOut);
RDR_to_PC_Parameters_t get_RDR_to_PC_Parameters(__u8 bSlot, __u8 bSeq,
        LONG pcsc_result, __u8 **abProtocolDataStructure);
RDR_to_PC_Parameters_t perform_PC_to_RDR_GetParamters(
        const PC_to_RDR_GetParameters_t request,
        __u8** abProtocolDataStructure);
RDR_to_PC_DataBlock_t perform_PC_to_RDR_Secure(
        const PC_to_RDR_Secure_t request, const __u8* abData,
        __u8** abDataOut);
RDR_to_PC_NotifySlotChange_t get_RDR_to_PC_NotifySlotChange ();
RDR_to_PC_SlotStatus_t perform_unknown(
        const PC_to_RDR_GetSlotStatus_t request);

int parse_ccid(const __u8* inbuf, __u8** outbuf);
int parse_ccid_control(struct usb_ctrlrequest *setup, __u8 **outbuf);

#ifdef  __cplusplus
}
#endif
#endif
