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
/**
 * @file
 */
#ifndef _CCID_H
#define _CCID_H

#include <linux/usb/ch9.h>
#include <libopensc/opensc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USB_REQ_CCID        0xA1

#define CCID_CONTROL_ABORT                  0x01
#define CCID_CONTROL_GET_CLOCK_FREQUENCIES  0x02
#define CCID_CONTROL_GET_DATA_RATES 0x03

#define CCID_BERROR_CMD_ABORTED 0xff /** Host aborted the current activity */
#define CCID_BERROR_ICC_MUTE 0xfe /** CCID timed out while talking to the ICC */
#define CCID_BERROR_XFR_PARITY_ERROR 0xfd /** Parity error while talking to the ICC */
#define CCID_BERROR_XFR_OVERRUN 0xfc /** Overrun error while talking to the ICC */
#define CCID_BERROR_HW_ERROR 0xfb /** An all inclusive hardware error occurred */
#define CCID_BERROR_BAD_ATR_TS 0xf
#define CCID_BERROR_BAD_ATR_TCK 0xf
#define CCID_BERROR_ICC_PROTOCOL_NOT_SUPPORTED 0xf6
#define CCID_BERROR_ICC_CLASS_NOT_SUPPORTED 0xf5
#define CCID_BERROR_PROCEDURE_BYTE_CONFLICT 0xf4
#define CCID_BERROR_DEACTIVATED_PROTOCOL 0xf3
#define CCID_BERROR_BUSY_WITH_AUTO_SEQUENCE 0xf2 /** Automatic Sequence Ongoing */
#define CCID_BERROR_PIN_TIMEOUT 0xf0
#define CCID_BERROR_PIN_CANCELLED 0xef
#define CCID_BERROR_CMD_SLOT_BUSY 0xe0 /** A second command was sent to a slot which was already processing a command. */
#define CCID_BERROR_CMD_NOT_SUPPORTED 0x00
#define CCID_BERROR_OK 0x00

#define CCID_BSTATUS_OK_ACTIVE 0x00 /** No error. An ICC is present and active */
#define CCID_BSTATUS_OK_INACTIVE 0x01 /** No error. ICC is present and inactive */
#define CCID_BSTATUS_OK_NOICC 0x02 /** No error. No ICC is present */
#define CCID_BSTATUS_ERROR_ACTIVE 0x40 /** Failed. An ICC is present and active */
#define CCID_BSTATUS_ERROR_INACTIVE 0x41 /** Failed. ICC is present and inactive */
#define CCID_BSTATUS_ERROR_NOICC 0x42 /** Failed. No ICC is present */

#define CCID_WLEVEL_DIRECT __constant_cpu_to_le16(0) /** APDU begins and ends with this command */
#define CCID_WLEVEL_CHAIN_NEXT_XFRBLOCK __constant_cpu_to_le16(1) /** APDU begins with this command, and continue in the next PC_to_RDR_XfrBlock */
#define CCID_WLEVEL_CHAIN_END __constant_cpu_to_le16(2) /** abData field continues a command APDU and ends the APDU command */
#define CCID_WLEVEL_CHAIN_CONTINUE __constant_cpu_to_le16(3) /** abData field continues a command APDU and another block is to follow */
#define CCID_WLEVEL_RESPONSE_IN_DATABLOCK __constant_cpu_to_le16(0x10) /** empty abData field, continuation of response APDU is expected in the next RDR_to_PC_DataBlock */

#define CCID_PIN_ENCODING_BIN   0x00
#define CCID_PIN_ENCODING_BCD   0x01
#define CCID_PIN_ENCODING_ASCII 0x02
#define CCID_PIN_UNITS_BYTES    0x80
#define CCID_PIN_JUSTIFY_RIGHT  0x04
#define CCID_PIN_CONFIRM_NEW    0x01
#define CCID_PIN_INSERT_OLD     0x02
#define CCID_PIN_NO_MSG         0x00
#define CCID_PIN_MSG1           0x01
#define CCID_PIN_MSG2           0x02
#define CCID_PIN_MSG_REF        0x03
#define CCID_PIN_MSG_DEFAULT    0xff

#define CCID_SLOTS_UNCHANGED    0x00
#define CCID_SLOT1_CARD_PRESENT 0x01
#define CCID_SLOT1_CHANGED      0x02
#define CCID_SLOT2_CARD_PRESENT 0x04
#define CCID_SLOT2_CHANGED      0x08
#define CCID_SLOT3_CARD_PRESENT 0x10
#define CCID_SLOT3_CHANGED      0x20
#define CCID_SLOT4_CARD_PRESENT 0x40
#define CCID_SLOT4_CHANGED      0x80

#define CCID_EXT_APDU_MAX       (4 + 3 + 0xffff + 3)
#define CCID_SHORT_APDU_MAX     (4 + 1 + 0xff + 1)

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
    __u8   bmSlotICCState; /* we support 1 slots, so we need 2*1 bits = 1 byte */
} __attribute__ ((packed)) RDR_to_PC_NotifySlotChange_t;

struct hid_class_descriptor {
    __u8   bLength;
    __u8   bDescriptorType;
    __le16 bcdHID;
    __le32 bCountryCode;
    __u8   bNumDescriptors;
} __attribute__ ((packed));

/**
 * @brief Initializes reader for relaying
 * 
 * @param[in] reader_id (optional) Index to the reader to be used. Set to -1 to use the first reader with a card or the first reader if no card is available.
 * @param[in] verbose   Verbosity level passed to \c sc_context_t
 * 
 * @return \c SC_SUCCESS or error code if an error occurred
 */
int ccid_initialize(int reader_id, int verbose);

/** 
 * @brief Disconnects from card, reader and releases allocated memory
 */
void ccid_shutdown(void);


/**
 * @brief Parses input from PC and generates the appropriate RDR response
 *
 * Parses command pipe bulk-OUT messages and generates resoponse pipe bulk-IN
 * messages according to CCID Rev 1.1 section 6.1, 6.2
 * 
 * @param[in]     inbuf  input buffer (command pipe bulk-OUT message)
 * @param[in]     inlen  length of \a inbuf
 * @param[in,out] outbuf where to save the output buffer (resoponse pipe bulk-IN message), memory is reused via \c realloc()
 * 
 * @return length of \a outbuf or -1 if an error occurred
 */
int ccid_parse_bulkout(const __u8* inbuf, size_t inlen, __u8** outbuf);

/** 
 * @brief Parses input from control pipe and generates the appropriate response
 *
 * Parses CCID class-specific requests according to CCID Rev 1.1 section 5.3
 * 
 * @param[in]     setup  input from control pipe
 * @param[in,out] outbuf where to save the output buffer, memory is reused via \c realloc()
 * 
 * @return length of \a outbuf or -1 if an error occurred
 */
int ccid_parse_control(struct usb_ctrlrequest *setup, __u8 **outbuf);

/** 
 * @brief Generates event messages
 * 
 * @param[in,out] slotchange where to save the output
 * @param[in]     timeout    currently not used
 * @note ccid_state_changed() must be called periodically. Because the OpenSC implementation of \c sc_wait_for_event() blocks all other operations with the reader, it can't be used for slot state detection.
 * 
 * @return 1 if a card is present and/or the state is changed or 0
 */
int ccid_state_changed(RDR_to_PC_NotifySlotChange_t **slotchange, int timeout);

#ifdef  __cplusplus
}
#endif
#endif
