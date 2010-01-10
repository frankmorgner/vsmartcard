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
#include <winscard.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

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


SCARDCONTEXT      hcontext = 0;
SCARDHANDLE       hcard    = 0;
SCARD_READERSTATE rstate;
DWORD             dwActiveProtocol;
char reader_name[MAX_READERNAME];
int  reader_num;

char* perform_initialization(int num)
{
    char *readers, *str;
    DWORD size;
    LONG r;

    reader_num = num;
    r = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hcontext);
    if (r != SCARD_S_SUCCESS) {
        fprintf(stderr, "pc/sc error: %s\n", pcsc_stringify_error(r));
        SCardReleaseContext(hcontext);
	return NULL;
    }
    r = SCardListReaders(hcontext, NULL, NULL, &size);
    if (size == 0)
        r = SCARD_E_UNKNOWN_READER;
    if (r != SCARD_S_SUCCESS) {
        fprintf(stderr, "pc/sc error: %s\n", pcsc_stringify_error(r));
        SCardReleaseContext(hcontext);
	return NULL;
    }

    /* get all readers */
    readers = (char *) malloc(size);
    if (readers == NULL) {
        fprintf(stderr, "pc/sc error: %s\n",
                pcsc_stringify_error(SCARD_E_NO_MEMORY));
        SCardReleaseContext(hcontext);
	return NULL;
    }
    r = SCardListReaders(hcontext, NULL, readers, &size);
    if (r != SCARD_S_SUCCESS) {
        free(readers);
        fprintf(stderr, "pc/sc error: %s\n", pcsc_stringify_error(r));
        SCardReleaseContext(hcontext);
	return NULL;
    }

    /* name of reader number num */
    str = readers;
    for (size = 0; size < num; size++) {
        /* go to the next name */
        str += strlen(str) + 1;

        /* no more readers available? */
        if (strlen(str) == 0) {
            free(readers);
            fprintf(stderr, "pc/sc error: %s\n",
                    pcsc_stringify_error(SCARD_E_UNKNOWN_READER));
            SCardReleaseContext(hcontext);
            return NULL;
        }
    }
    strncpy(reader_name, str, MAX_READERNAME);
    free(readers);

    rstate.dwCurrentState = SCARD_STATE_UNAWARE;
    rstate.dwEventState   = SCARD_STATE_UNAWARE;
    rstate.szReader       = reader_name;

    return reader_name;
}


int perform_shutdown()
{
    SCardDisconnect(hcard, SCARD_UNPOWER_CARD);
    hcard = 0;
    rstate.dwCurrentState = SCARD_STATE_UNAWARE;
    rstate.dwEventState   = SCARD_STATE_UNAWARE;
    return SCardReleaseContext(hcontext);
}

__u8 get_bError(LONG pcsc_result)
{
    switch (pcsc_result) {
        case SCARD_S_SUCCESS              : /**< No error was encountered. */
            // Command not supported
            return 0;
        case SCARD_E_CANCELLED            : /**< The action was cancelled by an SCardCancel request. */
            fprintf(stderr, "CMD_ABORTED\n");
            return 0xFF;
        case SCARD_E_INVALID_HANDLE       : /**< The supplied handle was invalid. */
        case SCARD_E_NO_SMARTCARD         : /**< The operation requires a Smart Card, but no Smart Card is currently in the device. */
        case SCARD_E_UNKNOWN_CARD         : /**< The specified smart card name is not recognized. */
        case SCARD_E_NOT_READY            : /**< The reader or smart card is not ready to accept commands. */
        case SCARD_W_UNRESPONSIVE_CARD    : /**< The smart card is not responding to a reset. */
        case SCARD_W_UNPOWERED_CARD       : /**< Power has been removed from the smart card, so that further communication is not possible. */
        case SCARD_W_REMOVED_CARD         : /**< The smart card has been removed, so further communication is not possible. */
            fprintf(stderr, "ICC_MUTE\n");
            return 0xFE;
        case SCARD_E_SHARING_VIOLATION    : /**< The smart card cannot be accessed because of other connections outstanding. */
            fprintf(stderr, "CMD_SLOT_BUSY\n");
            return 0xE0;
        case SCARD_E_PROTO_MISMATCH       : /**< The requested protocols are incompatible with the protocol currently in use with the smart card. */
            fprintf(stderr, "ICC_PROTOCOL_NOT_SUPPORTED\n");
            return 0xF6;
     // case SCARD_F_INTERNAL_ERROR       : /**< An internal consistency check failed. */
     // case SCARD_E_INVALID_PARAMETER    : /**< One or more of the supplied parameters could not be properly interpreted. */
     // case SCARD_E_INVALID_TARGET       : /**< Registry startup information is missing or invalid. */
     // case SCARD_E_NO_MEMORY            : /**< Not enough memory available to complete this command. */
     // case SCARD_F_WAITED_TOO_LONG      : /**< An internal consistency timer has expired. */
     // case SCARD_E_INSUFFICIENT_BUFFER  : /**< The data buffer to receive returned data is too small for the returned data. */
     // case SCARD_E_UNKNOWN_READER       : /**< The specified reader name is not recognized. */
     // case SCARD_E_TIMEOUT              : /**< The user-specified timeout value has expired. */
     // case SCARD_E_CANT_DISPOSE         : /**< The system could not dispose of the media in the requested manner. */
     // case SCARD_E_INVALID_VALUE        : /**< One or more of the supplied parameters values could not be properly interpreted. */
     // case SCARD_E_SYSTEM_CANCELLED     : /**< The action was cancelled by the system, presumably to log off or shut down. */
     // case SCARD_F_COMM_ERROR           : /**< An internal communications error has been detected. */
     // case SCARD_F_UNKNOWN_ERROR        : /**< An internal error has been detected, but the source is unknown. */
     // case SCARD_E_INVALID_ATR          : /**< An ATR obtained from the registry is not a valid ATR string. */
     // case SCARD_E_NOT_TRANSACTED       : /**< An attempt was made to end a non-existent transaction. */
     // case SCARD_E_READER_UNAVAILABLE   : /**< The specified reader is not currently available for use. */
     // case SCARD_W_UNSUPPORTED_CARD     : /**< The reader cannot communicate with the card, due to ATR string configuration conflicts. */
     // case SCARD_W_RESET_CARD           : /**< The smart card has been reset, so any shared state information is invalid. */
     // case SCARD_E_PCI_TOO_SMALL        : /**< The PCI Receive buffer was too small. */
     // case SCARD_E_READER_UNSUPPORTED   : /**< The reader driver does not meet minimal requirements for support. */
     // case SCARD_E_DUPLICATE_READER     : /**< The reader driver did not produce a unique reader name. */
     // case SCARD_E_CARD_UNSUPPORTED     : /**< The smart card does not meet minimal requirements for support. */
     // case SCARD_E_NO_SERVICE           : /**< The Smart card resource manager is not running. */
     // case SCARD_E_SERVICE_STOPPED      : /**< The Smart card resource manager has shut down. */
     // case SCARD_E_NO_READERS_AVAILABLE : /**< Cannot find a smart card reader. */
        default:
            fprintf(stderr, "HW_ERROR\n");
            return 0xFB;
    }
}

__u8 get_bStatus(LONG pcsc_result)
{
    __u8 bStatus = 0;

    if (rstate.dwEventState & SCARD_STATE_PRESENT) {
        if (rstate.dwEventState & SCARD_STATE_MUTE ||
                rstate.dwEventState & SCARD_STATE_UNPOWERED) {
            // inactive
            fprintf(stderr, "card inactive\n");
            bStatus = 1;
        } else {
            // active
            /*fprintf(stderr, "card active\n");*/
            bStatus = 0;
        }
    } else {
        // absent
        /*fprintf(stderr, "card absent\n");*/
        bStatus = 2;
        if (hcard != 0) {
            pcsc_result = SCardDisconnect(hcard, SCARD_UNPOWER_CARD);
            hcard = 0;
        }
    }

    if (pcsc_result != SCARD_S_SUCCESS) {
        bStatus |= (1<<6);
        fprintf(stderr, "pc/sc error: %s\n", pcsc_stringify_error(pcsc_result));
    }

    return bStatus;
}

RDR_to_PC_SlotStatus_t get_RDR_to_PC_SlotStatus(__u8 bSlot, __u8 bSeq,
        LONG pcsc_result)
{
    RDR_to_PC_SlotStatus_t result;

    result.bMessageType = 0x81;
    result.dwLength     = __constant_cpu_to_le32(0);
    result.bSlot        = bSlot;
    result.bSeq         = bSeq;
    result.bStatus      = get_bStatus(pcsc_result);
    result.bError       = get_bError(pcsc_result);
    result.bClockStatus = 0;

    return result;
}

RDR_to_PC_DataBlock_t get_RDR_to_PC_DataBlock(__u8 bSlot, __u8 bSeq,
        LONG pcsc_result, __le32 dwLength)
{
    RDR_to_PC_DataBlock_t result;

    result.bMessageType    = 0x80;
    result.dwLength        = dwLength;
    result.bSlot           = bSlot;
    result.bSeq            = bSeq;
    result.bStatus         = get_bStatus(pcsc_result);
    result.bError          = get_bError(pcsc_result);
    result.bChainParameter = 0;

    return result;
}

RDR_to_PC_SlotStatus_t
perform_PC_to_RDR_GetSlotStatus(const PC_to_RDR_GetSlotStatus_t request)
{
    if (    request.bMessageType != 0x65 ||
            request.dwLength     != __constant_cpu_to_le32(0) ||
            request.bSlot        != 0 ||
            request.abRFU1       != 0 ||
            request.abRFU2       != 0)
        fprintf(stderr, "warning: malformed PC_to_RDR_GetSlotStatus\n");

    return get_RDR_to_PC_SlotStatus(request.bSlot, request.bSeq,
            SCardGetStatusChange(hcontext, 1, &rstate, 1));
}

RDR_to_PC_SlotStatus_t
perform_PC_to_RDR_IccPowerOn(const PC_to_RDR_IccPowerOn_t request, char ** pATR)
{
    if (    request.bMessageType != 0x62 ||
            request.dwLength     != __constant_cpu_to_le32(0) ||
            request.bSlot        != 0 ||
            !( request.bPowerSelect == 0 ||
                request.bPowerSelect & ccid_desc.bVoltageSupport ) ||
            request.abRFU        != 0)
        fprintf(stderr, "warning: malformed PC_to_RDR_IccPowerOn\n");

    LONG pcsc_result;
    if (hcard) {
        pcsc_result = SCardReconnect(hcard, SCARD_SHARE_EXCLUSIVE,
                SCARD_PROTOCOL_T0|SCARD_PROTOCOL_T1, SCARD_LEAVE_CARD,
                &dwActiveProtocol);
    } else {
        pcsc_result = SCardConnect(hcontext, reader_name,
                SCARD_SHARE_EXCLUSIVE, SCARD_PROTOCOL_T0|SCARD_PROTOCOL_T1,
                &hcard, &dwActiveProtocol);
    }
    if (pcsc_result == SCARD_S_SUCCESS)
        pcsc_result = SCardGetStatusChange(hcontext, 1, &rstate, 1);

    RDR_to_PC_SlotStatus_t result = get_RDR_to_PC_SlotStatus(request.bSlot,
            request.bSeq, pcsc_result);

    if (pcsc_result != SCARD_S_SUCCESS) {
        *pATR = NULL;
        result.dwLength = __constant_cpu_to_le32(0);
    } else {
        *pATR = (char*) rstate.rgbAtr;
        result.dwLength = __cpu_to_le32(rstate.cbAtr);
    }

    return result;
}
RDR_to_PC_SlotStatus_t
perform_PC_to_RDR_IccPowerOff(const PC_to_RDR_IccPowerOff_t request)
{
    if (    request.bMessageType != 0x63 ||
            request.dwLength     != __constant_cpu_to_le32(0) ||
            request.bSlot        != 0 ||
            request.abRFU1       != 0 ||
            request.abRFU2       != 0)
        fprintf(stderr, "warning: malformed PC_to_RDR_IccPowerOff\n");

    LONG result = SCardDisconnect(hcard, SCARD_UNPOWER_CARD);
    hcard = 0;
    if (result == SCARD_E_INVALID_HANDLE) {
        result = SCardGetStatusChange(hcontext, 1, &rstate, 1);
    }

    return get_RDR_to_PC_SlotStatus(request.bSlot, request.bSeq, result);
}

RDR_to_PC_DataBlock_t
perform_PC_to_RDR_XfrBlock(const PC_to_RDR_XfrBlock_t request, const __u8*
        abDataIn, __u8** abDataOut)
{
    if (    request.bMessageType != 0x6F ||
            request.bSlot != 0           ||
            request.bBWI  != 0)
        fprintf(stderr, "warning: malformed PC_to_RDR_XfrBlock\n");

    DWORD dwRecvLength = MAX_BUFFER_SIZE;
    *abDataOut         = (__u8 *) malloc(dwRecvLength);
    if (*abDataOut == NULL) {
        return get_RDR_to_PC_DataBlock(request.bSlot, request.bSeq,
                SCARD_E_NO_MEMORY, __constant_cpu_to_le32(0));
    }

    LPCSCARD_IO_REQUEST pioSendPci;
    if (dwActiveProtocol == SCARD_PROTOCOL_T0)
        pioSendPci = SCARD_PCI_T0;
    else
        pioSendPci = SCARD_PCI_T1;
    int pcsc_result = SCardTransmit(hcard, pioSendPci, abDataIn,
            __le32_to_cpu(request.dwLength), NULL, *abDataOut, &dwRecvLength);

    return get_RDR_to_PC_DataBlock(request.bSlot, request.bSeq,
            pcsc_result, __cpu_to_le32(dwRecvLength));
}


RDR_to_PC_Parameters_t
get_RDR_to_PC_Parameters(__u8 bSlot, __u8 bSeq, LONG pcsc_result, __u8
        **abProtocolDataStructure)
{
    RDR_to_PC_Parameters_t result;

    result.bMessageType = 0x82;
    result.bSlot = bSlot;
    result.bSeq = bSeq;
    if (pcsc_result == SCARD_S_SUCCESS) {
        if (dwActiveProtocol == SCARD_PROTOCOL_T0) {
            result.bProtocolNum = 0;
            *abProtocolDataStructure = (__u8 *) malloc(sizeof
                    (abProtocolDataStructure_T0_t));
            if (*abProtocolDataStructure) {
                fprintf (stderr, "T0\n");
                result.dwLength = __constant_cpu_to_le32(sizeof
                        (abProtocolDataStructure_T0_t));
                abProtocolDataStructure_T0_t * t0 =
                    *(abProtocolDataStructure_T0_t**) abProtocolDataStructure;
                /* values taken from ISO 7816-3 defaults
                 * FIXME analyze ATR to get values */
                t0->bmFindexDindex    = 1<<4|   // index to table 7 ISO 7816-3 (Fi)
                                        1;      // index to table 8 ISO 7816-3 (Di)
                t0->bmTCCKST0         = 0<<1;   // convention (direct)
                t0->bGuardTimeT0      = 0xFF;
                t0->bWaitingIntegerT0 = 0x10;
                t0->bClockStop        = 0;      // (not allowed)
            } else {
                // error malloc
                result.dwLength = __constant_cpu_to_le32(0);
                *abProtocolDataStructure = NULL;
                pcsc_result = SCARD_E_INSUFFICIENT_BUFFER;
            }
        } else {
            result.bProtocolNum = 1;
            *abProtocolDataStructure = (__u8 *) malloc(sizeof
                    (abProtocolDataStructure_T1_t));
            if (*abProtocolDataStructure) {
                fprintf (stderr, "T1\n");
                result.dwLength = __constant_cpu_to_le32(sizeof
                        (abProtocolDataStructure_T1_t));
                abProtocolDataStructure_T1_t * t1 =
                    *(abProtocolDataStructure_T1_t**) abProtocolDataStructure;
                /* values taken from OpenPGP-card
                 * FIXME analyze ATR to get values */
                t1->bmFindexDindex     = 1<<4|   // index to table 7 ISO 7816-3 (Fi)
                                         3;      // index to table 8 ISO 7816-3 (Di)
                t1->bmTCCKST1          = 0|      // checksum type (CRC)
                                         0<<1|   // convention (direct)
                                         0x10;
                t1->bGuardTimeT1       = 0xFF;
                t1->bWaitingIntegersT1 = 4<<4|   // BWI
                                         5;      // CWI
                t1->bClockStop         = 0;      // (not allowed)
                t1->bIFSC              = 0x80;
                t1->bNadValue          = 0;      // see 7816-3 9.4.2.1 (only default value)
            } else {
                // error malloc
                result.dwLength = __constant_cpu_to_le32(0);
                *abProtocolDataStructure = NULL;
                pcsc_result = SCARD_E_INSUFFICIENT_BUFFER;
            }
        }
    } else {
        result.dwLength = __constant_cpu_to_le32(0);
        *abProtocolDataStructure = NULL;
    }
    result.bStatus = get_bStatus(pcsc_result);
    result.bError  = get_bError(pcsc_result);

    return result;
}

RDR_to_PC_Parameters_t
perform_PC_to_RDR_GetParamters(const PC_to_RDR_GetParameters_t request,
        __u8** abProtocolDataStructure)
{
    if (    request.bMessageType != 0x6C ||
            request.dwLength != __constant_cpu_to_le32(0) ||
            request.bSlot != 0)
        fprintf(stderr, "warning: malformed PC_to_RDR_GetParamters\n");

    LONG pcsc_result = SCardReconnect(hcard, SCARD_SHARE_EXCLUSIVE,
            SCARD_PROTOCOL_T0|SCARD_PROTOCOL_T1, SCARD_LEAVE_CARD,
            &dwActiveProtocol);

    return get_RDR_to_PC_Parameters(request.bSlot, request.bSeq,
            pcsc_result, abProtocolDataStructure);
}

RDR_to_PC_DataBlock_t
perform_PC_to_RDR_Secure(const PC_to_RDR_Secure_t request,
        const __u8* abData, __u8** abDataOut)
{
    /* only short APDUs supported so Lc is always the fiths byte */
    if (    request.bMessageType != 0x69 ||
            request.bSlot != 0)
        fprintf(stderr, "warning: malformed PC_to_RDR_Secure\n");

    if (request.wLevelParameter != __constant_cpu_to_le16(0)) {
        fprintf(stderr, "warning: Only APDUs, that begin and end with this command are supported.\n");
        return get_RDR_to_PC_DataBlock(request.bSlot, request.bSeq,
                SCARD_E_READER_UNSUPPORTED, __constant_cpu_to_le32(0));
    }

    __u8 PINMin, PINMax, bmPINLengthFormat, bmPINBlockString, bmFormatString;
    __u8 *abPINApdu;
    uint32_t apdulen;
    abPINDataStucture_Verification_t *verify = NULL;
    abPINDataStucture_Modification_t *modify = NULL;
    switch (*abData) { // first byte of abData is bPINOperation
        case 0x00:
            // PIN Verification
            verify = (abPINDataStucture_Verification_t *)
                (abData + sizeof(__u8));
            PINMin = verify->wPINMaxExtraDigit >> 8;
            PINMax = verify->wPINMaxExtraDigit & 0x00ff;
            bmPINLengthFormat = verify->bmPINLengthFormat;
            bmPINBlockString = verify->bmPINBlockString;
            bmFormatString = verify->bmFormatString;
            abPINApdu = (__u8*) (verify + sizeof(*verify));
            apdulen = __le32_to_cpu(request.dwLength) - sizeof(*verify);
            break;
        case 0x01:
            // PIN Modification
            modify = (abPINDataStucture_Modification_t *)
                (abData + sizeof(__u8));
            PINMin = modify->wPINMaxExtraDigit >> 8;
            PINMax = modify->wPINMaxExtraDigit & 0x00ff;
            bmPINLengthFormat = modify->bmPINLengthFormat;
            bmPINBlockString = modify->bmPINBlockString;
            bmFormatString = modify->bmFormatString;
            abPINApdu = (__u8*) (modify + sizeof(*modify));
            apdulen = __le32_to_cpu(request.dwLength) - sizeof(*modify);
            break;
        case 0x04:
            // Cancel PIN function
        default:
            fprintf(stderr, "warning: unknown pin operation\n");
            return get_RDR_to_PC_DataBlock(request.bSlot, request.bSeq,
                    SCARD_E_READER_UNSUPPORTED, __constant_cpu_to_le32(0));
    }

    printf("verify=%d abPINApdu=%d sizoeof(verify)=%d\n", verify, abPINApdu, sizeof(*verify));
    printf("%x %x %x %x %x %x\n", abPINApdu[0], abPINApdu[1], abPINApdu[2], abPINApdu[3], abPINApdu[4], abPINApdu[5]);

    // copy the apdu
    __u8 *apdu = (__u8*) malloc(apdulen);
    if (!apdu) {
        return get_RDR_to_PC_DataBlock(request.bSlot, request.bSeq,
                SCARD_E_NO_MEMORY, __constant_cpu_to_le32(0));
    }
    memcpy(apdu, abPINApdu, apdulen);

    // TODO
    char *pin = "123456";

    __u8 *p;
    /* if system units are bytes or bits */
    uint8_t bytes = bmFormatString >> 7;
    /* PIN position after format in the APDU command (relative to the first
     * data after Lc). The position is based on the system units’ type
     * indicator (maximum1111 for fifteen system units */
    uint8_t pos = (bmFormatString >> 3) & 0xf;
    /* Right or left justify data */
    uint8_t right = (bmFormatString >> 2) & 1;
    /* Bit wise for the PIN format type */
    uint8_t type = bmFormatString & 2;

    uint8_t pinlen = strnlen(pin, PINMax + 1);
    if (pinlen > PINMax) {
        fprintf(stderr, "warning: PIN was too long, "
                "should be between %d and %d\n", PINMin, PINMax);
        return get_RDR_to_PC_DataBlock(request.bSlot, request.bSeq,
                SCARD_F_INTERNAL_ERROR, __constant_cpu_to_le32(0));
    }

    /* Size in bits of the PIN length inserted in the APDU command. */
    uint8_t lenlen = bmPINBlockString >> 4;
    /* PIN block size in bytes after justification and formatting. */
    uint8_t blocksize = bmPINBlockString & 0xf;
    /* PIN length position in the APDU command */
    uint8_t lenshift = bmPINLengthFormat & 0xf;
    if (lenlen) {
        /* write PIN Length */
        if (lenlen == 8) {
            if (bytes) {
                p = apdu + 5 + lenshift;
            } else {
                if (lenshift == 0)
                    p = apdu + 5;
                if (lenshift == 8)
                    p = apdu + 5 + 1;
                else {
                    fprintf(stderr, "warning: PIN Block too complex, aborting\n");
                    fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
                    return get_RDR_to_PC_DataBlock(request.bSlot, request.bSeq,
                            SCARD_F_INTERNAL_ERROR,
                            __constant_cpu_to_le32(0));
                }
            }
            *p = pinlen;
        }

        fprintf(stderr, "warning: PIN Block too complex, aborting\n");
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        return get_RDR_to_PC_DataBlock(request.bSlot, request.bSeq,
                SCARD_F_INTERNAL_ERROR, __constant_cpu_to_le32(0));
    }

    uint8_t justify;
    if (right)
        justify = blocksize - pinlen;
    else
        justify = 0;

    if (bytes) {
        p = apdu + 5 + pos + justify;
    } else {
        if (pos == 0)
            p = apdu + 5 + justify;
        else if (pos == 8)
            p = apdu + 5 + 1 + justify;
        else {
            fprintf(stderr, "warning: PIN Block too complex, aborting\n");
            fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
            fprintf(stderr, "%x\n", bmFormatString);
            return get_RDR_to_PC_DataBlock(request.bSlot, request.bSeq,
                    SCARD_F_INTERNAL_ERROR,
                    __constant_cpu_to_le32(0));
        }
    }

    while (*pin) {
        uint8_t c;
        switch (type) {
            case 0:
                // binary
                switch (*pin) {
                    case '0':
                        c = 0x00;
                        break;
                    case '1':
                        c = 0x01;
                        break;
                    case '2':
                        c = 0x02;
                        break;
                    case '3':
                        c = 0x03;
                        break;
                    case '4':
                        c = 0x04;
                        break;
                    case '5':
                        c = 0x05;
                        break;
                    case '6':
                        c = 0x06;
                        break;
                    case '7':
                        c = 0x07;
                        break;
                    case '8':
                        c = 0x08;
                        break;
                    case '9':
                        c = 0x09;
                        break;
                    default:
                        fprintf(stderr, "warning: PIN character %c not supported, aborting", *pin);
                        return get_RDR_to_PC_DataBlock(request.bSlot,
                                request.bSeq, SCARD_F_INTERNAL_ERROR,
                                __constant_cpu_to_le32(0));
                }
                break;
            case 1:
                // BCD
                fprintf(stderr, "warning: BCD format not supported, aborting");
                return get_RDR_to_PC_DataBlock(request.bSlot, request.bSeq,
                        SCARD_F_INTERNAL_ERROR, __constant_cpu_to_le32(0));
            case 2:
                // ASCII
                c = *pin;
                break;
            default:
                fprintf(stderr, "warning: unknown formatting, aborting");
                return get_RDR_to_PC_DataBlock(request.bSlot, request.bSeq,
                        SCARD_F_INTERNAL_ERROR, __constant_cpu_to_le32(0));
        }
        *p = c;
        p++;
        pin++;
    }

    printf("%x %x %x %x %x %x\n", apdu[0], apdu[1], apdu[2], apdu[3], apdu[4], apdu[5]);


    DWORD dwRecvLength = MAX_BUFFER_SIZE;
    *abDataOut         = (__u8 *) malloc(dwRecvLength);
    if (*abDataOut == NULL) {
        return get_RDR_to_PC_DataBlock(request.bSlot, request.bSeq,
                SCARD_E_NO_MEMORY, __constant_cpu_to_le32(0));
    }

    LPCSCARD_IO_REQUEST pioSendPci;
    if (dwActiveProtocol == SCARD_PROTOCOL_T0)
        pioSendPci = SCARD_PCI_T0;
    else
        pioSendPci = SCARD_PCI_T1;
    int pcsc_result = SCardTransmit(hcard, pioSendPci, apdu, apdulen, NULL,
            *abDataOut, &dwRecvLength);

    return get_RDR_to_PC_DataBlock(request.bSlot, request.bSeq,
            pcsc_result, __cpu_to_le32(dwRecvLength));
}

RDR_to_PC_NotifySlotChange_t
get_RDR_to_PC_NotifySlotChange ()
{
    RDR_to_PC_NotifySlotChange_t result;
    result.bMessageType = 0x50;
    result.bmSlotICCState = 0; // no change

    DWORD current = rstate.dwEventState;
    if (SCARD_S_SUCCESS != SCardGetStatusChange(hcontext, 1, &rstate, 1)) {
        fprintf(stderr, "state changed: error\n");
        result.bmSlotICCState = 2; // changed (error)
    } else if (!(current & rstate.dwEventState)) {
        fprintf(stderr, "state changed\n");
        result.bmSlotICCState = 2; // changed
    }

    return result;
}

RDR_to_PC_SlotStatus_t
perform_unknown(const PC_to_RDR_GetSlotStatus_t request)
{
    RDR_to_PC_SlotStatus_t result;
    switch (request.bMessageType) {
        case 0x62:
        case 0x6F:
        case 0x69:
            result.bMessageType = 0x80;
            break;
        case 0x63:
        case 0x65:
        case 0x6E:
        case 0x6A:
        case 0x71:
        case 0x72:
            result.bMessageType = 0x81;
            break;
        case 0x61:
        case 0x6C:
        case 0x6D:
            result.bMessageType = 0x82;
            break;
        case 0x6B:
            result.bMessageType = 0x83;
            break;
        case 0x73:
            result.bMessageType = 0x84;
            break;
        default:
            fprintf(stderr, "unknown message type\n");
            result.bMessageType = 0;
    }
    result.dwLength     = __constant_cpu_to_le32(0);
    result.bSlot        = request.bSlot,
    result.bSeq         = request.bSeq;
    result.bStatus      = get_bStatus(SCARD_F_UNKNOWN_ERROR);
    result.bError       = 0;
    result.bClockStatus = 0;

    return result;
}

int parse_ccid(const __u8* inbuf, __u8** outbuf) {
    if (inbuf == NULL)
        return 0;
    int result = -1;
    if (SCardIsValidContext(hcontext) != SCARD_S_SUCCESS) {
        if (perform_initialization(reader_num) == NULL)
            goto error;
    }

    switch (*inbuf) {
        case 0x62: 
            {   fprintf(stderr, "PC_to_RDR_IccPowerOn\n");

                char* atr;
                PC_to_RDR_IccPowerOn_t input  =
                    *(PC_to_RDR_IccPowerOn_t*) inbuf;
                RDR_to_PC_SlotStatus_t output =
                    perform_PC_to_RDR_IccPowerOn(input, &atr);

                result = sizeof output + __le32_to_cpu(output.dwLength);
                *outbuf = realloc(*outbuf, result);
                if (*outbuf == NULL) {
                    result = -1;
                    break;
                }

                memcpy(*outbuf, &output, sizeof output);
                memcpy(*outbuf + sizeof output, atr,
                        __le32_to_cpu(output.dwLength));
            }   break;

        case 0x63:
            {   fprintf(stderr, "PC_to_RDR_IccPowerOff\n");

                PC_to_RDR_IccPowerOff_t input =
                    *(PC_to_RDR_IccPowerOff_t*) inbuf;
                RDR_to_PC_SlotStatus_t output   =
                    perform_PC_to_RDR_IccPowerOff(input);

                result = sizeof output;
                *outbuf = realloc(*outbuf, result);
                if (*outbuf == NULL) {
                    result = -1;
                    break;
                }

                memcpy(*outbuf, &output, sizeof output);
            }   break;

        case 0x65:
            {   /*fprintf(stderr, "PC_to_RDR_GetSlotStatus\n");*/

                PC_to_RDR_GetSlotStatus_t input =
                    *(PC_to_RDR_GetSlotStatus_t*) inbuf;
                RDR_to_PC_SlotStatus_t output   =
                    perform_PC_to_RDR_GetSlotStatus(input);

                result = sizeof output;
                *outbuf = realloc(*outbuf, result);
                if (*outbuf == NULL) {
                    result = -1;
                    break;
                }

                memcpy(*outbuf, &output, sizeof output);
            }   break;

        case 0x6F:
            {   fprintf(stderr, "PC_to_RDR_XfrBlock\n");

                __u8* rapdu;
                PC_to_RDR_XfrBlock_t input   = *(PC_to_RDR_XfrBlock_t*) inbuf;
                RDR_to_PC_DataBlock_t output =
                    perform_PC_to_RDR_XfrBlock(input, inbuf + sizeof input,
                            &rapdu);

                result  = sizeof output + __le32_to_cpu(output.dwLength);
                *outbuf = realloc(*outbuf, result);
                if (*outbuf == NULL) {
                    free(rapdu);
                    result = -1;
                    break;
                }

                memcpy(*outbuf, &output, sizeof output);
                memcpy(*outbuf + sizeof output, rapdu,
                        __le32_to_cpu(output.dwLength));
                free(rapdu);
            }   break;

        case 0x6C:
            {   fprintf(stderr, "PC_to_RDR_GetParameters\n");

                __u8* abProtocolDataStructure;
                PC_to_RDR_GetParameters_t input = *(PC_to_RDR_GetParameters_t*)
                    inbuf;
                RDR_to_PC_Parameters_t output = perform_PC_to_RDR_GetParamters(
                        input, &abProtocolDataStructure );

                result  = sizeof output + __le32_to_cpu(output.dwLength);
                *outbuf = realloc(*outbuf, result);
                if (*outbuf == NULL) {
                    if (abProtocolDataStructure)
                        free(abProtocolDataStructure);
                    result = -1;
                    break;
                }

                memcpy(*outbuf, &output, sizeof output);
                memcpy(*outbuf + sizeof output, abProtocolDataStructure,
                        __le32_to_cpu(output.dwLength));
                if (abProtocolDataStructure)
                    free(abProtocolDataStructure);
            }   break;

        case 0x69:
            {   fprintf(stderr, "PC_to_RDR_Secure\n");

                __u8* rapdu;
                PC_to_RDR_Secure_t input = *(PC_to_RDR_Secure_t *) inbuf;
                RDR_to_PC_DataBlock_t output =
                    perform_PC_to_RDR_Secure(input, inbuf + sizeof input,
                            &rapdu);

                result  = sizeof output + __le32_to_cpu(output.dwLength);
                *outbuf = realloc(*outbuf, result);
                if (*outbuf == NULL) {
                    free(rapdu);
                    result = -1;
                    break;
                }

                memcpy(*outbuf, &output, sizeof output);
                memcpy(*outbuf + sizeof output, rapdu,
                        __le32_to_cpu(output.dwLength));
                free(rapdu);
            }   break;

        default:
error:
            {   fprintf(stderr, "unknown ccid command: 0x%4X\n", *inbuf);

                PC_to_RDR_GetSlotStatus_t input =
                    *(PC_to_RDR_GetSlotStatus_t*) inbuf;
                RDR_to_PC_SlotStatus_t output   =
                    perform_unknown(input);

                result = sizeof output;
                *outbuf = realloc(*outbuf, result);
                if (*outbuf == NULL) {
                    result = -1;
                    break;
                }

                memcpy(*outbuf, &output, sizeof output);
            }
    }

    return result;
}

int parse_ccid_control(struct usb_ctrlrequest *setup, __u8 **outbuf) {
    int result = -1;
    __u16 value, index, length;

    value = __le16_to_cpu(setup->wValue);
    index = __le16_to_cpu(setup->wIndex);
    length = __le16_to_cpu(setup->wLength);
    if (setup->bRequestType == USB_REQ_CCID)
        switch(setup->bRequest) {
            case CCID_CONTROL_ABORT:
                {
                    fprintf(stderr, "ABORT\n");
                    if (length != 0x00) {
                        fprintf(stderr, "warning: malformed ABORT\n");
                    }
                    result = SCardCancel(hcontext);
                    if (result != SCARD_S_SUCCESS)
                        fprintf(stderr, "pc/sc error: %s\n",
                                pcsc_stringify_error(result));
                    result = 0;
                }   break;
            case CCID_CONTROL_GET_CLOCK_FREQUENCIES:
                {
                    fprintf(stderr, "GET_CLOCK_FREQUENCIES\n");
                    if (value != 0x00) {
                        fprintf(stderr,
                                "warning: malformed GET_CLOCK_FREQUENCIES\n");
                    }

                    result = sizeof(__le32);
                    *outbuf = realloc(*outbuf, result);
                    if (*outbuf == NULL) {
                        result = -1;
                        break;
                    }
                    __le32 clock  = ccid_desc.dwDefaultClock;
                    memcpy(*outbuf, &clock,  sizeof (__le32));
                }   break;
            case CCID_CONTROL_GET_DATA_RATES:
                {
                    fprintf(stderr, "GET_DATA_RATES\n");
                    if (value != 0x00) {
                        fprintf(stderr, "warning: malformed GET_DATA_RATES\n");
                    }

                    result = sizeof (__le32);
                    *outbuf = realloc(*outbuf, result);
                    if (*outbuf == NULL) {
                        result = -1;
                        break;
                    }
                    __le32 drate  = ccid_desc.dwDataRate;
                    memcpy(*outbuf, &drate,  sizeof (__le32));
                }   break;
            default:
                printf("unknown status setup->bRequest == %d", setup->bRequest);
        }

    return result;
}

