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
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <asm/byteorder.h>
#include <opensc/opensc.h>
#include <openssl/evp.h>
#include <opensc/log.h>

#include "ccid.h"
#include "config.h"

#ifdef WITH_PACE
#include <pace/pace.h>
#include <pace/sm.h>
#include <pace/scutil.h>

static struct sm_ctx sctx;
#ifdef BUERGERCLIENT_WORKAROUND
static char *ef_cardaccess = NULL;
static size_t ef_cardaccess_length = 0;
#endif

#else
#include "pace/scutil.h"
#endif

static sc_context_t *ctx = NULL;
#ifdef BUERGERCLIENT_WORKAROUND
static sc_card_t *card_in_slot[1];
#else
static sc_card_t *card_in_slot[SC_MAX_SLOTS];
#endif
static sc_reader_t *reader;

struct ccid_class_descriptor
ccid_desc = {
    .bLength                = sizeof ccid_desc,
    .bDescriptorType        = 0x21,
    .bcdCCID                = __constant_cpu_to_le16(0x0110),
    .bMaxSlotIndex          = SC_MAX_SLOTS,
    .bVoltageSupport        = 0x01,  // 5.0V
    .dwProtocols            = __constant_cpu_to_le32(
                              0x01|  // T=0
                              0x02), // T=1
    .dwDefaultClock         = __constant_cpu_to_le32(0xDFC),
    .dwMaximumClock         = __constant_cpu_to_le32(0xDFC),
    .bNumClockSupport       = 1,
    .dwDataRate             = __constant_cpu_to_le32(0x2580),
    .dwMaxDataRate          = __constant_cpu_to_le32(0x2580),
    .bNumDataRatesSupported = 1,
    .dwMaxIFSD              = __constant_cpu_to_le32(0xFF), // IFSD is handled by the real reader driver
    .dwSynchProtocols       = __constant_cpu_to_le32(0),
    .dwMechanical           = __constant_cpu_to_le32(0),
    .dwFeatures             = __constant_cpu_to_le32(
                              0x00000002|  // Automatic parameter configuration based on ATR data
                              0x00000004|  // Automatic activation of ICC on inserting
                              0x00000008|  // Automatic ICC voltage selection
                              0x00000010|  // Automatic ICC clock frequency change
                              0x00000020|  // Automatic baud rate change
                              0x00000040|  // Automatic parameters negotiation
                              0x00000080|  // Automatic PPS   
                              0x00000400|  // Automatic IFSD exchange as first exchange
                              0x00040000|  // Short and Extended APDU level exchange with CCID
                              0x00100000), // USB Wake up signaling supported
    .dwMaxCCIDMessageLength = __constant_cpu_to_le32(CCID_EXT_APDU_MAX),
    .bClassGetResponse      = 0xFF,
    .bclassEnvelope         = 0xFF,
    .wLcdLayout             = __constant_cpu_to_le16(
                              0xFF00|   // Number of lines for the LCD display
                              0x00FF),  // Number of characters per line
    .bPINSupport            = 0x1|      // PIN Verification supported
                              0x2|      // PIN Modification supported
                              0x10|     // PIN PACE Capabilities supported
                              0x20,     // PIN PACE Verification supported
    .bMaxCCIDBusySlots      = 0x01,
};

#define debug_sc_result(sc_result) \
{ \
    if (sc_result < 0) \
        sc_error(ctx, sc_strerror(sc_result)); \
    else \
        sc_debug(ctx, sc_strerror(sc_result)); \
}

static int
detect_card_presence(int slot)
{
    int sc_result;

    if (slot >= sizeof *card_in_slot)
        return SC_ERROR_INVALID_ARGUMENTS;

    sc_result = sc_detect_card_presence(reader, slot);

    if (sc_result == 0
            && card_in_slot[slot]
            && sc_card_valid(card_in_slot[slot])) {
        sc_disconnect_card(card_in_slot[slot], 0);
        sc_debug(ctx, "Card removed from slot %d", slot);
    }
    if (sc_result & SC_SLOT_CARD_CHANGED) {
        sc_disconnect_card(card_in_slot[slot], 0);
        sc_debug(ctx, "Card exchanged in slot %d", slot);
    }
    if (sc_result & SC_SLOT_CARD_PRESENT
            && (!card_in_slot[slot]
                || !sc_card_valid(card_in_slot[slot]))) {
        sc_debug(ctx, "Unused card in slot %d", slot);
    }

    return sc_result;
}


int ccid_initialize(int reader_id, const char *cdriver, int verbose)
{
    int i;

    for (i = 0; i < sizeof *card_in_slot; i++) {
        card_in_slot[i] = NULL;
    }

    i = initialize(reader_id, cdriver, verbose, &ctx, &reader);
    if (i < 0)
        return i;

    ccid_desc.bMaxSlotIndex = reader->slot_count - 1;

#ifdef WITH_PACE
    memset(&sctx, 0, sizeof(sctx));
#ifdef BUERGERCLIENT_WORKAROUND
    free(ef_cardaccess);
    ef_cardaccess = NULL;
    ef_cardaccess_length = 0;
#endif
#endif

    return SC_SUCCESS;
}

void ccid_shutdown()
{
    int i;
    for (i = 0; i < sizeof *card_in_slot; i++) {
        if (card_in_slot[i] && sc_card_valid(card_in_slot[i])) {
            sc_disconnect_card(card_in_slot[i], 0);
        }
    }
    if (ctx)
        sc_release_context(ctx);

#ifdef WITH_PACE
    pace_sm_ctx_clear_free(sctx.cipher_ctx);
    memset(&sctx, 0, sizeof(sctx));
#ifdef BUERGERCLIENT_WORKAROUND
    free(ef_cardaccess);
    ef_cardaccess = NULL;
    ef_cardaccess_length = 0;
#endif
#endif
}

static int get_rapdu(sc_apdu_t *apdu, size_t slot, __u8 **buf, size_t *resplen)
{
    int sc_result;

    if (!apdu || !buf || !resplen || slot >= sizeof *card_in_slot) {
        sc_result = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }

    apdu->resplen = apdu->le;
    /* Get two more bytes to later use as return buffer including sw1 and sw2 */
    apdu->resp = realloc(*buf, apdu->resplen + sizeof(__u8) + sizeof(__u8));
    if (!apdu->resp) {
        sc_result = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    *buf = apdu->resp;

#ifdef WITH_PACE
    sc_result = sm_transmit_apdu(&sctx, card_in_slot[slot], apdu);
#else
    sc_result = sc_transmit_apdu(&sctx, card_in_slot[slot], apdu);
#endif
    if (sc_result < 0) {
        goto err;
    }

    if (apdu->sw1 > 0xff || apdu->sw2 > 0xff || apdu->sw1 < 0 || apdu->sw2 < 0) {
        sc_error(ctx, "Received invalid status bytes SW1=%d SW2=%d", apdu->sw1, apdu->sw2);
        sc_result = SC_ERROR_INVALID_DATA;
        goto err;
    }

    apdu->resp[apdu->resplen] = apdu->sw1;
    apdu->resp[apdu->resplen + sizeof(__u8)] = apdu->sw2;

    *buf = apdu->resp;
    *resplen = apdu->resplen + sizeof(__u8) + sizeof(__u8);

    sc_debug(ctx, "R-APDU, %d byte%s:\tsw1=%02x sw2=%02x",
            (unsigned int) *resplen, !*resplen ? "" : "s",
            apdu->sw1, apdu->sw2);

    sc_result = SC_SUCCESS;

err:
    return sc_result;
}

static __u8 get_bError(int sc_result)
{
    if (sc_result < 0) {
        switch (sc_result) {
            case SC_SUCCESS:
                return CCID_BERROR_OK;

            case SC_ERROR_SLOT_ALREADY_CONNECTED:
                return CCID_BERROR_CMD_SLOT_BUSY;

            case SC_ERROR_KEYPAD_TIMEOUT:
                return CCID_BERROR_PIN_TIMEOUT;

            case SC_ERROR_KEYPAD_CANCELLED:
                return CCID_BERROR_PIN_CANCELLED;

            case SC_ERROR_EVENT_TIMEOUT:
            case SC_ERROR_CARD_UNRESPONSIVE:
                return CCID_BERROR_ICC_MUTE;

            default:
                return CCID_BERROR_HW_ERROR;
        }
    } else
        return CCID_BERROR_OK;
}

static __u8 get_bStatus(int sc_result, __u8 bSlot)
{
    int flags;
    __u8 bstatus = 0;

    flags = detect_card_presence(bSlot);

    if (flags >= 0) {
        if (sc_result < 0) {
            if (flags & SC_SLOT_CARD_PRESENT) {
                if (flags & SC_SLOT_CARD_CHANGED
                        || (card_in_slot[bSlot]
                            && !sc_card_valid(card_in_slot[bSlot]))) {
                    /*sc_debug(ctx, "error inactive");*/
                    bstatus = CCID_BSTATUS_ERROR_INACTIVE;
                } else {
                    /*sc_debug(ctx, "error active");*/
                    bstatus = CCID_BSTATUS_ERROR_ACTIVE;
                }
            } else {
                /*sc_debug(ctx, "error no icc");*/
                bstatus = CCID_BSTATUS_ERROR_NOICC;
            }
        } else {
            if (flags & SC_SLOT_CARD_PRESENT) {
                if (flags & SC_SLOT_CARD_CHANGED || (
                            card_in_slot[bSlot]
                            && !sc_card_valid(card_in_slot[bSlot]))) {
                    /*sc_debug(ctx, "ok inactive");*/
                    bstatus = CCID_BSTATUS_OK_INACTIVE;
                } else {
                    /*sc_debug(ctx, "ok active");*/
                    bstatus = CCID_BSTATUS_OK_ACTIVE;
                }
            } else {
                /*sc_debug(ctx, "ok no icc");*/
                bstatus = CCID_BSTATUS_OK_NOICC;
            }
        }
    } else {
        debug_sc_result(flags);
        sc_error(ctx, "Could not detect card presence."
                " Falling back to default (bStatus=0x%02X).", bstatus);
    }

    return bstatus;
}

static int
get_RDR_to_PC_SlotStatus(__u8 bSlot, __u8 bSeq, int sc_result, __u8 **outbuf, size_t *outlen,
        const __u8 *abProtocolDataStructure, size_t abProtocolDataStructureLen)
{
    if (!outbuf)
        return SC_ERROR_INVALID_ARGUMENTS;
    if (abProtocolDataStructureLen > 0xffff) {
        sc_error(ctx, "abProtocolDataStructure %u bytes too long",
                abProtocolDataStructureLen-0xffff);
        return SC_ERROR_INVALID_DATA;
    }

    RDR_to_PC_SlotStatus_t *status = realloc(*outbuf, sizeof(*status) + abProtocolDataStructureLen);
    if (!status)
        return SC_ERROR_OUT_OF_MEMORY;
    *outbuf = (__u8 *) status;
    *outlen = sizeof(*status) + abProtocolDataStructureLen;

    status->bMessageType = 0x81;
    status->dwLength     = __constant_cpu_to_le32(abProtocolDataStructureLen);
    status->bSlot        = bSlot;
    status->bSeq         = bSeq;
    status->bStatus      = get_bStatus(sc_result, bSlot);
    status->bError       = get_bError(sc_result);
    status->bClockStatus = 0;

    memcpy((*outbuf) + sizeof(*status), abProtocolDataStructure, abProtocolDataStructureLen);

    return SC_SUCCESS;
}

static int
get_RDR_to_PC_DataBlock(__u8 bSlot, __u8 bSeq, int sc_result, __u8 **outbuf,
        size_t *outlen, const __u8 *abData, size_t abDataLen)
{
    if (!outbuf)
        return SC_ERROR_INVALID_ARGUMENTS;
    if (abDataLen > 0xffff) {
        sc_error(ctx, "abProtocolDataStructure %u bytes too long",
                abDataLen-0xffff);
        return SC_ERROR_INVALID_DATA;
    }

    RDR_to_PC_DataBlock_t *data = realloc(*outbuf, sizeof(*data) + abDataLen);
    if (!data)
        return SC_ERROR_OUT_OF_MEMORY;
    *outbuf = (__u8 *) data;
    *outlen = sizeof(*data) + abDataLen;

    data->bMessageType    = 0x80;
    data->dwLength        = __constant_cpu_to_le32(abDataLen);
    data->bSlot           = bSlot;
    data->bSeq            = bSeq;
    data->bStatus         = get_bStatus(sc_result, bSlot);
    data->bError          = get_bError(sc_result);
    data->bChainParameter = 0;

    memcpy((*outbuf) + sizeof(*data), abData, abDataLen);

    return SC_SUCCESS;
}

static int
perform_PC_to_RDR_GetSlotStatus(const __u8 *in, size_t inlen, __u8 **out, size_t *outlen)
{
    const PC_to_RDR_GetSlotStatus_t *request = (PC_to_RDR_GetSlotStatus_t *) in;

    if (!out || !outlen || !in)
        return SC_ERROR_INVALID_ARGUMENTS;

    if (inlen < sizeof *request)
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_ERROR_INVALID_DATA);

    *outlen = sizeof(RDR_to_PC_SlotStatus_t);

    if (    request->bMessageType != 0x65 ||
            request->dwLength     != __constant_cpu_to_le32(0) ||
            request->abRFU1       != 0 ||
            request->abRFU2       != 0)
        sc_debug(ctx, "warning: malformed PC_to_RDR_GetSlotStatus");

    return get_RDR_to_PC_SlotStatus(request->bSlot, request->bSeq, SC_SUCCESS,
            out, outlen, NULL, 0);
}

static int
perform_PC_to_RDR_IccPowerOn(const __u8 *in, size_t inlen, __u8 **out, size_t *outlen)
{
    const PC_to_RDR_IccPowerOn_t *request = (PC_to_RDR_IccPowerOn_t *) in;
    int sc_result;

    if (!out || !outlen || !in)
        return SC_ERROR_INVALID_ARGUMENTS;

    if (inlen < sizeof *request)
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_ERROR_INVALID_DATA);

    if (    request->bMessageType != 0x62 ||
            request->dwLength     != __constant_cpu_to_le32(0) ||
            !( request->bPowerSelect == 0 ||
                request->bPowerSelect & ccid_desc.bVoltageSupport ) ||
            request->abRFU        != 0)
        sc_debug(ctx, "warning: malformed PC_to_RDR_IccPowerOn");

    if (request->bSlot < sizeof *card_in_slot) {
        if (card_in_slot[request->bSlot]
                && sc_card_valid(card_in_slot[request->bSlot])) {
            sc_debug(ctx, "Card is already powered on.");
            /*sc_reset(card_in_slot[request->bSlot]);*/
            /*sc_disconnect_card(card_in_slot[request->bSlot], 0);*/
        } else {
            sc_result = sc_connect_card(reader, request->bSlot,
                    &card_in_slot[request->bSlot]);
#ifdef BUERGERCLIENT_WORKAROUND
            if (sc_result >= 0) {
                if (get_ef_card_access(card_in_slot[request->bSlot],
                            (u8 **) &ef_cardaccess, &ef_cardaccess_length) < 0) {
                    sc_error(ctx, "Could not get EF.CardAccess.");
                }
            }
#endif
        }
    } else {
        sc_result = SC_ERROR_INVALID_DATA;
    }

    if (sc_result >= 0) {
        return get_RDR_to_PC_SlotStatus(request->bSlot, request->bSeq,
                sc_result, out, outlen, card_in_slot[request->bSlot]->atr,
                card_in_slot[request->bSlot]->atr_len);
    } else {
        sc_error(ctx, "Returning default status package.");
        return get_RDR_to_PC_SlotStatus(request->bSlot, request->bSeq,
                sc_result, out, outlen, NULL, 0);
    }
}

static int
perform_PC_to_RDR_IccPowerOff(const __u8 *in, size_t inlen, __u8 **out, size_t *outlen)
{
    const PC_to_RDR_IccPowerOff_t *request = (PC_to_RDR_IccPowerOff_t *) in;
    int sc_result;

    if (!in || !out || !outlen)
        return SC_ERROR_INVALID_ARGUMENTS;

    if (inlen < sizeof *request)
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_ERROR_INVALID_DATA);

    if (    request->bMessageType != 0x63 ||
            request->dwLength     != __constant_cpu_to_le32(0) ||
            request->abRFU1       != 0 ||
            request->abRFU2       != 0)
        sc_debug(ctx, "warning: malformed PC_to_RDR_IccPowerOff");

    if (request->bSlot > sizeof *card_in_slot)
        sc_result = SC_ERROR_INVALID_DATA;
    else {
        sc_reset(card_in_slot[request->bSlot]);
        sc_result = sc_disconnect_card(card_in_slot[request->bSlot], 0);
    }

    return get_RDR_to_PC_SlotStatus(request->bSlot, request->bSeq, sc_result,
                out, outlen, NULL, 0);
}

static int
perform_PC_to_RDR_XfrBlock(const u8 *in, size_t inlen, __u8** out, size_t *outlen)
{
    const PC_to_RDR_XfrBlock_t *request = (PC_to_RDR_XfrBlock_t *) in;
    const __u8 *abDataIn = in + sizeof *request;
    int sc_result;
    size_t abDataOutLen = 0;
    sc_apdu_t apdu;
    __u8 *abDataOut = NULL;

    if (!in || !out || !outlen)
        return SC_ERROR_INVALID_ARGUMENTS;

    if (inlen < sizeof *request)
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_ERROR_INVALID_DATA);

    if (    request->bMessageType != 0x6F ||
            request->bBWI  != 0)
        sc_debug(ctx, "malformed PC_to_RDR_XfrBlock, will continue anyway");

    if (request->bSlot > sizeof *card_in_slot)
        sc_result = SC_ERROR_INVALID_DATA;
    else {
        sc_result = build_apdu(ctx, abDataIn, request->dwLength, &apdu);
        if (sc_result >= 0)
            sc_result = get_rapdu(&apdu, request->bSlot, &abDataOut, &abDataOutLen);
        else
            bin_log(ctx, "Invalid APDU", abDataIn, request->dwLength);
    }

    sc_result = get_RDR_to_PC_DataBlock(request->bSlot, request->bSeq, sc_result,
            out, outlen, abDataOut, abDataOutLen);

    free(abDataOut);

    return sc_result;
}

static int
perform_PC_to_RDR_GetParamters(const __u8 *in, size_t inlen, __u8** out, size_t *outlen)
{
    const PC_to_RDR_GetParameters_t *request = (PC_to_RDR_GetParameters_t *) in;
    RDR_to_PC_Parameters_t *result;
    abProtocolDataStructure_T1_t *t1;
    abProtocolDataStructure_T0_t *t0;
    int sc_result;

    if (!in || !out || !outlen)
        return SC_ERROR_INVALID_ARGUMENTS;

    if (inlen < sizeof *request)
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_ERROR_INVALID_DATA);

    if (    request->bMessageType != 0x6C ||
            request->dwLength != __constant_cpu_to_le32(0))
        sc_debug(ctx, "warning: malformed PC_to_RDR_GetParamters");

    if (request->bSlot < sizeof *card_in_slot) {
        switch (reader->slot[request->bSlot].active_protocol) {
            case SC_PROTO_T0:
                result = realloc(*out, sizeof *result + sizeof *t0);
                if (!result)
                    return SC_ERROR_OUT_OF_MEMORY;
                *out = (__u8 *) result;

                result->bProtocolNum = 0;
                result->dwLength = __constant_cpu_to_le32(sizeof *t0);

                t0 = (abProtocolDataStructure_T0_t *) result + sizeof *result;
                /* values taken from ISO 7816-3 defaults
                 * FIXME analyze ATR to get values */
                t0->bmFindexDindex    =
                    1<<4|   // index to table 7 ISO 7816-3 (Fi)
                    1;      // index to table 8 ISO 7816-3 (Di)
                t0->bmTCCKST0         = 0<<1;   // convention (direct)
                t0->bGuardTimeT0      = 0xFF;
                t0->bWaitingIntegerT0 = 0x10;
                t0->bClockStop        = 0;      // (not allowed)

                sc_result = SC_SUCCESS;
                break;

            case SC_PROTO_T1:
                result = realloc(*out, sizeof *result + sizeof *t1);
                if (!result)
                    return SC_ERROR_OUT_OF_MEMORY;
                *out = (__u8 *) result;

                result->bProtocolNum = 1;
                result->dwLength = __constant_cpu_to_le32(sizeof *t1);

                t1 = (abProtocolDataStructure_T1_t *) result + sizeof *result;
                /* values taken from OpenPGP-card
                 * FIXME analyze ATR to get values */
                t1->bmFindexDindex     =
                    1<<4|   // index to table 7 ISO 7816-3 (Fi)
                    3;      // index to table 8 ISO 7816-3 (Di)
                t1->bmTCCKST1          =
                    0|      // checksum type (CRC)
                    0<<1|   // convention (direct)
                    0x10;
                t1->bGuardTimeT1       = 0xFF;
                t1->bWaitingIntegersT1 =
                    4<<4|   // BWI
                    5;      // CWI
                t1->bClockStop         = 0;      // (not allowed)
                t1->bIFSC              = 0x80;
                t1->bNadValue          = 0;      // see 7816-3 9.4.2.1 (only default value)

                sc_result = SC_SUCCESS;
                break;

            default:
                goto invaliddata;
        }
    } else {
invaliddata:
        result = realloc(*out, sizeof *result);
        if (!result)
            return SC_ERROR_OUT_OF_MEMORY;
        *out = (__u8 *) result;

        sc_result = SC_ERROR_INVALID_DATA;
    }

    result->bMessageType = 0x82;
    result->bSlot = request->bSlot;
    result->bSeq = request->bSeq;
    result->bStatus = get_bStatus(sc_result, request->bSlot);
    result->bError  = get_bError(sc_result);

    if (sc_result < 0)
        debug_sc_result(sc_result);

    return SC_SUCCESS;
}

static int
get_effective_offset(uint8_t system_units, uint8_t off, size_t *eff_off,
        int *sc_result)
{
    if (!eff_off || !sc_result) {
        if (sc_result)
            *sc_result = SC_ERROR_INVALID_ARGUMENTS;
        return 0;
    }

    if (system_units)
        *eff_off = off;
    else
        if (off == 0)
            *eff_off = 0;
        else if (off == 8)
            *eff_off = 1;

    *sc_result = SC_SUCCESS;

    return 1;
}

static int
write_pin_length(sc_apdu_t *apdu, const struct sc_pin_cmd_pin *pin,
        uint8_t system_units, uint8_t length_size, int *sc_result)
{
    u8 *p;

    if (!apdu || !apdu->data || !pin || apdu->datalen <= pin->length_offset ||
            !sc_result) {
        if (sc_result)
            *sc_result = SC_ERROR_INVALID_ARGUMENTS;
        return 0;
    }

    if (length_size) {
        if (length_size != 8) {
            sc_error(ctx, "Writing PIN length only if it fits into "
                    "a full byte (length of PIN size was %u bits)",
                    length_size);
            *sc_result = SC_ERROR_NOT_SUPPORTED;
            return 0;
        }
        p = (u8 *) apdu->data;
        p[pin->length_offset] = pin->len;
    }

    *sc_result = SC_SUCCESS;

    return 1;
}

static int
encode_pin(u8 *buf, size_t buf_len, struct sc_pin_cmd_pin *pin,
        uint8_t encoding, int *sc_result)
{
    const u8 *p;

    if (!pin || !buf || !sc_result) {
        if (sc_result)
            *sc_result = SC_ERROR_INVALID_ARGUMENTS;
        return 0;
    }

    if (encoding == CCID_PIN_ENCODING_BIN) {
        for (p = pin->data; *p && buf_len>0; buf++, p++, buf_len--) {
            switch (*p) {
                case '0':
                    *buf = 0x00;
                    break;
                case '1':
                    *buf = 0x01;
                    break;
                case '2':
                    *buf = 0x02;
                    break;
                case '3':
                    *buf = 0x03;
                    break;
                case '4':
                    *buf = 0x04;
                    break;
                case '5':
                    *buf = 0x05;
                    break;
                case '6':
                    *buf = 0x06;
                    break;
                case '7':
                    *buf = 0x07;
                    break;
                case '8':
                    *buf = 0x08;
                    break;
                case '9':
                    *buf = 0x09;
                    break;
                default:
                    *sc_result = SC_ERROR_INVALID_ARGUMENTS;
                    return 0;
            }
        }
        if (!buf_len && *p) {
            /* pin is longer than buf_len */
            *sc_result = SC_ERROR_OUT_OF_MEMORY;
            return 0;
        }

        *sc_result = SC_SUCCESS;
    } else {
        if (encoding == CCID_PIN_ENCODING_BCD)
            pin->encoding = SC_PIN_ENCODING_BCD;
        else if (encoding == CCID_PIN_ENCODING_ASCII)
            pin->encoding = SC_PIN_ENCODING_ASCII;
        else {
            sc_error(ctx, "PIN encoding not supported (0x%02x)", encoding);
            *sc_result = SC_ERROR_NOT_SUPPORTED;
            return 0;
        }

        *sc_result = sc_build_pin(buf, buf_len, pin, 0);

        if (*sc_result < 0)
            return 0;
    }

    return 1;
}

static int
write_pin(sc_apdu_t *apdu, struct sc_pin_cmd_pin *pin, uint8_t blocksize,
        uint8_t justify_right, uint8_t encoding, int *sc_result)
{
    /* offset due to right alignment */
    uint8_t justify_offset;

    if (!apdu || !pin || !sc_result) {
        if (sc_result)
        *sc_result = SC_ERROR_INVALID_ARGUMENTS;
        return 0;
    }

    if (justify_right) {
        if (encoding == CCID_PIN_ENCODING_BCD) {
            if (pin->len % 2) {
                sc_error(ctx, "Right aligning BCD encoded PIN only if it fits "
                        "into a full byte (length of encoded PIN was %u bits)",
                        (pin->len)*4);
                *sc_result = SC_ERROR_NOT_SUPPORTED;
                return 0;
            } else
                justify_offset = blocksize - pin->len/2;
        } else
            justify_offset = blocksize - pin->len;
    }
    else
        justify_offset = 0;

    return encode_pin((u8 *) apdu->data + justify_offset,
            blocksize - justify_offset, pin, encoding, sc_result);
}

#ifdef WITH_PACE
static int
perform_PC_to_RDR_Secure_EstablishPACEChannel(sc_card_t *card,
        const __u8 *abData, size_t abDatalen,
        __u8 **abDataOut, size_t *abDataOutLen)
{
    struct establish_pace_channel_input pace_input;
    struct establish_pace_channel_output pace_output;
    size_t parsed = 0;
    int sc_result;
    __le16 word;
    __u8 *p;

    memset(&pace_input, 0, sizeof(pace_input));
    memset(&pace_output, 0, sizeof(pace_output));


    if (!abDataOut || !abDataOutLen) {
        sc_result = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }

    if (abDatalen < parsed+1) {
        sc_error(ctx, "Buffer too small, could not get PinID");
        sc_result = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }
    pace_input.pin_id = abData[parsed];
    parsed++;


    if (abDatalen < parsed+1) {
        sc_error(ctx, "Buffer too small, could not get lengthCHAT");
        sc_result = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }
    pace_input.chat_length = abData[parsed];
    parsed++;

    if (abDatalen < parsed+pace_input.chat_length) {
        sc_error(ctx, "Buffer too small, could not get CHAT");
        sc_result = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }
    pace_input.chat = &abData[parsed];
    parsed += pace_input.chat_length;
    /* XXX make this human readable */
    if (pace_input.chat_length)
        bin_log(ctx, "Card holder authorization template",
                pace_input.chat, pace_input.chat_length);


    if (abDatalen < parsed+1) {
        sc_error(ctx, "Buffer too small, could not get lengthPIN");
        sc_result = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }
    pace_input.pin_length = abData[parsed];
    parsed++;

    if (abDatalen < parsed+pace_input.pin_length) {
        sc_error(ctx, "Buffer too small, could not get PIN");
        sc_result = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }
    pace_input.pin = &abData[parsed];
    parsed += pace_input.pin_length;


    if (abDatalen < parsed+sizeof(word)) {
        sc_error(ctx, "Buffer too small, could not get lengthCertificateDescription");
        sc_result = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }
    memcpy(&word, &abData[parsed], sizeof word);
    pace_input.certificate_description_length = __le16_to_cpu(word);
    parsed += sizeof word;

    if (abDatalen < parsed+pace_input.certificate_description_length) {
        sc_error(ctx, "Buffer too small, could not get CertificateDescription");
        sc_result = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }
    pace_input.certificate_description = &abData[parsed];
    parsed += pace_input.certificate_description_length;
    /* XXX make this human readable */
    if (pace_input.certificate_description_length)
        bin_log(ctx, "Certificate description",
                pace_input.certificate_description,
                pace_input.certificate_description_length);


#ifdef BUERGERCLIENT_WORKAROUND
    pace_output.ef_cardaccess_length = ef_cardaccess_length;
    pace_output.ef_cardaccess = ef_cardaccess;
#endif

    sc_result = EstablishPACEChannel(NULL, card, pace_input, &pace_output,
            &sctx);
    if (sc_result < 0)
        goto err;


    p = realloc(*abDataOut, 2 +                     /* Statusbytes */
            2+pace_output.ef_cardaccess_length +    /* EF.CardAccess */
            1+pace_output.recent_car_length +       /* Most recent CAR */
            1+pace_output.previous_car_length +     /* Previous CAR */
            2+pace_output.id_icc_length);           /* identifier of the MRTD chip */
    if (!p) {
        sc_result = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    *abDataOut = p;


    *p = pace_output.mse_set_at_sw1;
    p += 1;

    *p = pace_output.mse_set_at_sw2;
    p += 1;


    if (pace_output.ef_cardaccess_length > 0xffff) {
        sc_error(ctx, "EF.CardAcces %u bytes too long",
                pace_output.ef_cardaccess_length-0xffff);
        sc_result = SC_ERROR_INVALID_DATA;
        goto err;
    }
    word = __cpu_to_le16(pace_output.ef_cardaccess_length);
    memcpy(p, &word, sizeof word);
    p += sizeof word;

    memcpy(p, pace_output.ef_cardaccess,
            pace_output.ef_cardaccess_length);
    p += pace_output.ef_cardaccess_length;


    if (pace_output.recent_car_length > 0xff) {
        sc_error(ctx, "Most recent CAR %u bytes too long",
                pace_output.recent_car_length-0xff);
        sc_result = SC_ERROR_INVALID_DATA;
        goto err;
    }
    *p = pace_output.recent_car_length;
    p += 1;

    memcpy(p, pace_output.recent_car,
            pace_output.recent_car_length);
    p += pace_output.recent_car_length;


    if (pace_output.previous_car_length > 0xff) {
        sc_error(ctx, "Most previous CAR %u bytes too long",
                pace_output.previous_car_length-0xff);
        sc_result = SC_ERROR_INVALID_DATA;
        goto err;
    }
    *p = pace_output.previous_car_length;
    p += 1;

    memcpy(p, pace_output.previous_car,
            pace_output.previous_car_length);
    p += pace_output.previous_car_length;


    if (pace_output.id_icc_length > 0xffff) {
        sc_error(ctx, "ID ICC %u bytes too long",
                pace_output.id_icc_length-0xffff);
        sc_result = SC_ERROR_INVALID_DATA;
        goto err;
    }
    word = __cpu_to_le16(pace_output.id_icc_length);
    memcpy(p, &word, sizeof word);
    p += sizeof word;

    memcpy(p, pace_output.id_icc,
            pace_output.id_icc_length);
    p += pace_output.id_icc_length;


    *abDataOutLen = 2 +
            2+pace_output.ef_cardaccess_length +
            1+pace_output.recent_car_length +
            1+pace_output.previous_car_length +
            2+pace_output.id_icc_length;

    sc_result = SC_SUCCESS;

err:
#ifdef BUERGERCLIENT_WORKAROUND
    ef_cardaccess = pace_output.ef_cardaccess;
    ef_cardaccess_length = pace_output.ef_cardaccess_length;
#else
    free(pace_output.ef_cardaccess;
#endif
    free(pace_output.recent_car);
    free(pace_output.previous_car);
    free(pace_output.id_icc);
    free(pace_output.id_pcd);

    return sc_result;
}

static int
perform_PC_to_RDR_Secure_GetReadersPACECapabilities(__u8 **abDataOut,
        size_t *abDataOutLen)
{
    int sc_result;
    __u8 *result;

    if (!abDataOut || !abDataOutLen)
        return SC_ERROR_INVALID_ARGUMENTS;

    result = realloc(*abDataOut, 2);
    if (!result)
        return SC_ERROR_OUT_OF_MEMORY;
    *abDataOut = result;

    /* lengthBitMap */
    *result = 1;
    result++;

    sc_result = GetReadersPACECapabilities(result);
    if (sc_result < 0)
        return sc_result;

    *abDataOutLen = 2;

    return SC_SUCCESS;
}
#else
static int
perform_PC_to_RDR_Secure_EstablishPACEChannel(sc_card_t *card,
        const __u8 *abData, size_t abDatalen,
        __u8 **abDataOut, size_t *abDataOutLen)
{
    sc_error(ctx, "ccid compiled without PACE support.");
    return SC_ERROR_NOT_SUPPORTED;
}
static int
perform_PC_to_RDR_Secure_GetReadersPACECapabilities(__u8 **abDataOut,
        size_t *abDataOutLen)
{
    sc_error(ctx, "ccid compiled without PACE support.");
    return SC_ERROR_NOT_SUPPORTED;
}
#endif

static int
perform_PC_to_RDR_Secure(const __u8 *in, size_t inlen, __u8** out, size_t *outlen)
{
    int sc_result;
    struct sc_pin_cmd_pin curr_pin, new_pin;
    sc_apdu_t apdu;
    const PC_to_RDR_Secure_t *request = (PC_to_RDR_Secure_t *) in;
    const __u8* abData = in + sizeof *request;
    size_t abDatalen = inlen - sizeof *request;
    u8 *abDataOut = NULL;
    size_t abDataOutLen = 0;
    RDR_to_PC_DataBlock_t *result;

    memset(&curr_pin, 0, sizeof(curr_pin));
    memset(&new_pin, 0, sizeof(new_pin));


    if (!in || !out || !outlen)
        return SC_ERROR_INVALID_ARGUMENTS;

    if (inlen < sizeof *request + 1)
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_ERROR_INVALID_DATA);

    if (request->bMessageType != 0x69)
        sc_debug(ctx,  "warning: malformed PC_to_RDR_Secure");

    if (request->bSlot > sizeof *card_in_slot) {
        sc_error(ctx, "Received request to invalid slot (bSlot=0x%02x)", request->bSlot);
        goto err;
    }

    if (request->wLevelParameter != CCID_WLEVEL_DIRECT) {
        sc_error(ctx, "Received request with unsupported wLevelParameter (0x%04x)",
                __le16_to_cpu(request->wLevelParameter));
        sc_result = SC_ERROR_NOT_SUPPORTED;
        goto err;
    }

    __u8 bmPINLengthFormat, bmPINBlockString, bmFormatString, bNumberMessage;
    __u8 *abPINApdu;
    uint32_t apdulen;
    uint16_t wPINMaxExtraDigit;
    abPINDataStucture_Verification_t *verify = NULL;
    abPINDataStucture_Modification_t *modify = NULL;
    switch (*abData) { // first byte of abData is bPINOperation
        case 0x00:
            // PIN Verification
            verify = (abPINDataStucture_Verification_t *)
                (abData + sizeof(__u8));

            if (abDatalen < sizeof *verify) {
                sc_result = SC_ERROR_INVALID_DATA;
                goto err;
            }

            wPINMaxExtraDigit = verify->wPINMaxExtraDigit;
            bmPINLengthFormat = verify->bmPINLengthFormat;
            bmPINBlockString = verify->bmPINBlockString;
            bmFormatString = verify->bmFormatString;
            bNumberMessage = verify->bNumberMessage;
            abPINApdu = (__u8*) verify + sizeof(*verify);
            apdulen = __le32_to_cpu(request->dwLength) - sizeof(*verify) - sizeof(__u8);
            break;
        case 0x01:
            // PIN Modification
            modify = (abPINDataStucture_Modification_t *)
                (abData + sizeof(__u8));

            if (abDatalen < sizeof *modify) {
                sc_error(ctx, "Not enough data for abPINDataStucture_Modification_t");
                sc_result = SC_ERROR_INVALID_DATA;
                goto err;
            }

            wPINMaxExtraDigit = modify->wPINMaxExtraDigit;
            bmPINLengthFormat = modify->bmPINLengthFormat;
            bmPINBlockString = modify->bmPINBlockString;
            bmFormatString = modify->bmFormatString;
            bNumberMessage = modify->bNumberMessage;
            abPINApdu = (__u8*) modify + sizeof(*modify);
            apdulen = __le32_to_cpu(request->dwLength) - sizeof(*modify) - sizeof(__u8);
            break;
        case 0x10:

            sc_result = perform_PC_to_RDR_Secure_GetReadersPACECapabilities(
                    &abDataOut, &abDataOutLen);
            goto err;
            break;
        case 0x20:

            sc_result = perform_PC_to_RDR_Secure_EstablishPACEChannel(
                    card_in_slot[request->bSlot], abData+1, abDatalen-1,
                    &abDataOut, &abDataOutLen);
            goto err;
            break;
        case 0x04:
            // Cancel PIN function
        default:
            sc_error(ctx, "Received request with unsupported PIN operation (bPINOperation=0x%02x)",
                    *abData);
            sc_result = SC_ERROR_NOT_SUPPORTED;
            goto err;
    }

    sc_result = build_apdu(ctx, abPINApdu, apdulen, &apdu);
    if (sc_result < 0) {
        bin_log(ctx, "Invalid APDU", abPINApdu, apdulen);
        goto err;
    }
    apdu.sensitive = 1;

    new_pin.min_length = curr_pin.min_length = wPINMaxExtraDigit >> 8;
    new_pin.min_length = curr_pin.max_length = wPINMaxExtraDigit & 0x00ff;
    uint8_t system_units = bmFormatString & CCID_PIN_UNITS_BYTES;
    uint8_t pin_offset = (bmFormatString >> 3) & 0xf;
    uint8_t length_offset = bmPINLengthFormat & 0xf;
    uint8_t length_size = bmPINBlockString >> 4;
    uint8_t justify_right = bmFormatString & CCID_PIN_JUSTIFY_RIGHT;
    uint8_t encoding = bmFormatString & 2;
    uint8_t blocksize = bmPINBlockString & 0xf;

    sc_debug(ctx, "PIN %s block (%d bytes) proberties:\n"
            "\tminimum %d, maximum %d PIN digits\n"
            "\t%s PIN encoding, %s justification\n"
            "\tsystem units are %s\n"
            "\twrite PIN length on %d bits with %d system units offset\n"
            "\tcurrent PIN offset is %d %s\n",
            modify ? "modification" : "verification",
            blocksize, (unsigned int) curr_pin.min_length,
            (unsigned int) curr_pin.max_length,
            encoding == CCID_PIN_ENCODING_BIN ? "binary" :
            encoding == CCID_PIN_ENCODING_BCD ? "BCD" :
            encoding == CCID_PIN_ENCODING_ASCII ? "ASCII" :"unknown",
            justify_right ? "right" : "left",
            system_units ? "bytes" : "bits",
            length_size, length_offset,
            modify ? modify->bInsertionOffsetOld : pin_offset,
            modify ? "bytes" : "system units");

    /* get the PIN */
    if (verify) {
        if (0 > EVP_read_pw_string_min((char *) curr_pin.data,
                    curr_pin.min_length, curr_pin.max_length,
                    "Please enter your PIN for verification: ",
                0)) {
            sc_result = SC_ERROR_INTERNAL;
            sc_error(ctx, "Could not read PIN.\n");
            goto err;
        }
    } else {
        if (modify->bConfirmPIN & CCID_PIN_INSERT_OLD) {
            /* if only the new pin is requested, it is stored in curr_pin */
            if (0 > EVP_read_pw_string_min((char *) curr_pin.data,
                        curr_pin.min_length, curr_pin.max_length,
                        "Please enter your current PIN for modification: ",
                        0)) {
                sc_result = SC_ERROR_INTERNAL;
                sc_error(ctx, "Could not read current PIN.\n");
                goto err;
            }
        }

        if (0 > EVP_read_pw_string_min((char *) new_pin.data,
                    new_pin.min_length, new_pin.max_length,
                    "Please enter your new PIN for modification: ",
                    modify->bConfirmPIN & CCID_PIN_CONFIRM_NEW)) {
            sc_result = SC_ERROR_INTERNAL;
            sc_error(ctx, "Could not read new PIN.\n");
            goto err;
        }
    }

    /* set length of PIN */
    curr_pin.len = strlen((char *) curr_pin.data);
    if (modify) {
        new_pin.len = strlen((char *) new_pin.data);
    }

    /* Note: pin.offset and pin.length_offset are relative to the first
     * databyte */
    if (verify) {
        if (!get_effective_offset(system_units, pin_offset, &curr_pin.offset,
                    &sc_result))
            goto err;
    } else {
        if (modify->bConfirmPIN & CCID_PIN_INSERT_OLD) {
            curr_pin.offset = modify->bInsertionOffsetOld;
            new_pin.offset = modify->bInsertionOffsetNew;
            if (!write_pin(&apdu, &new_pin, blocksize, justify_right, encoding,
                        &sc_result))
                goto err;
        } else {
            curr_pin.offset = modify->bInsertionOffsetNew;
        }
    }
    if (!get_effective_offset(system_units, length_offset,
                &curr_pin.length_offset, &sc_result)
            || !write_pin_length(&apdu, &curr_pin, system_units, length_size,
                &sc_result)
            || !write_pin(&apdu, &curr_pin, blocksize, justify_right, encoding,
                &sc_result))
        goto err;

    sc_result = get_rapdu(&apdu, request->bSlot, &abDataOut, &abDataOutLen);

    /* clear PINs that have been transmitted */
    sc_mem_clear((u8 *) apdu.data, apdu.datalen);

err:
    if (curr_pin.data) {
        sc_mem_clear((u8 *) curr_pin.data, curr_pin.len);
        free((u8 *) curr_pin.data);
    }
    if (new_pin.data) {
        sc_mem_clear((u8 *) new_pin.data, new_pin.len);
        free((u8 *) new_pin.data);
    }

    sc_result = get_RDR_to_PC_DataBlock(request->bSlot, request->bSeq, sc_result, out,
            outlen, abDataOut, abDataOutLen);

    free(abDataOut);

    return sc_result;
}

/* XXX calling sc_wait_for_event blocks all other threads, thats why it
 * can't be used here... */
static int
get_RDR_to_PC_NotifySlotChange(RDR_to_PC_NotifySlotChange_t **out)
{
    int i;
    int sc_result;
    uint8_t oldmask;
    uint8_t changed [] = {
            CCID_SLOT1_CHANGED,
            CCID_SLOT2_CHANGED,
            CCID_SLOT3_CHANGED,
            CCID_SLOT4_CHANGED,
    };
    uint8_t present [] = {
            CCID_SLOT1_CARD_PRESENT,
            CCID_SLOT2_CARD_PRESENT,
            CCID_SLOT3_CARD_PRESENT,
            CCID_SLOT4_CARD_PRESENT,
    };

    if (!out)
        return SC_ERROR_INVALID_ARGUMENTS;

    RDR_to_PC_NotifySlotChange_t *result = realloc(*out, sizeof *result);
    if (!result)
        return SC_ERROR_OUT_OF_MEMORY;
    *out = result;

    result->bMessageType = 0x50;
    result->bmSlotICCState = CCID_SLOTS_UNCHANGED;
    oldmask = CCID_SLOTS_UNCHANGED;

    for (i = 0; i < reader->slot_count; i++) {
        sc_result = detect_card_presence(i);
        if (sc_result < 0) {
            sc_error(ctx, "Could not detect card presence, skipping slot %d.",
                    i);
            debug_sc_result(sc_result);
            continue;
        }

        if (sc_result & SC_SLOT_CARD_PRESENT)
            oldmask |= present[i];
        if (sc_result & SC_SLOT_CARD_CHANGED) {
            sc_debug(ctx, "Card status changed in slot %d.", i);
            result->bmSlotICCState |= changed[i];
        }

    }

    sleep(10);

    for (i = 0; i < reader->slot_count; i++) {
        sc_result = detect_card_presence(i);
        if (sc_result < 0) {
            sc_error(ctx, "Could not detect card presence, skipping slot %d.",
                    i);
            debug_sc_result(sc_result);
            continue;
        }

        if (sc_result & SC_SLOT_CARD_PRESENT)
            result->bmSlotICCState |= present[i];
        if (sc_result & SC_SLOT_CARD_CHANGED) {
            sc_debug(ctx, "Card status changed in slot %d.", i);
            result->bmSlotICCState |= changed[i];
        }

        if ((oldmask & present[i]) != (result->bmSlotICCState & present[i])) {
            sc_debug(ctx, "Card status changed in slot %d.", i);
            result->bmSlotICCState |= changed[i];
        }
    }

    return SC_SUCCESS;
}

static int
perform_unknown(const __u8 *in, size_t inlen, __u8 **out, size_t *outlen)
{
    const PC_to_RDR_GetSlotStatus_t *request = (PC_to_RDR_GetSlotStatus_t *) in;
    RDR_to_PC_SlotStatus_t *result;

    if (!in || !out || !outlen)
        return SC_ERROR_INVALID_ARGUMENTS;

    if (inlen < sizeof *request)
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_ERROR_INVALID_DATA);

    result = realloc(*out, sizeof *result);
    if (!result)
        return SC_ERROR_OUT_OF_MEMORY;
    *out = (__u8 *) result;

    switch (request->bMessageType) {
        case 0x62:
        case 0x6F:
        case 0x69:
            result->bMessageType = 0x80;
            break;
        case 0x63:
        case 0x65:
        case 0x6E:
        case 0x6A:
        case 0x71:
        case 0x72:
            result->bMessageType = 0x81;
            break;
        case 0x61:
        case 0x6C:
        case 0x6D:
            result->bMessageType = 0x82;
            break;
        case 0x6B:
            result->bMessageType = 0x83;
            break;
        case 0x73:
            result->bMessageType = 0x84;
            break;
        default:
            sc_debug(ctx, "Unknown message type in request (0x%02x). "
                    "Using bMessageType=0x%02x for output.",
                    request->bMessageType, 0);
            result->bMessageType = 0;
    }
    result->dwLength     = __constant_cpu_to_le32(0);
    result->bSlot        = request->bSlot,
    result->bSeq         = request->bSeq;
    result->bStatus      = get_bStatus(SC_ERROR_UNKNOWN_DATA_RECEIVED,
            request->bSlot);
    result->bError       = 0;
    result->bClockStatus = 0;

    *outlen = sizeof *result;

    return SC_SUCCESS;
}

int ccid_parse_bulkin(const __u8* inbuf, size_t inlen, __u8** outbuf)
{
    int sc_result;
    size_t outlen;

    if (!inbuf)
        return 0;

    switch (*inbuf) {
        case 0x62: 
                sc_debug(ctx,  "PC_to_RDR_IccPowerOn");
                sc_result = perform_PC_to_RDR_IccPowerOn(inbuf, inlen, outbuf, &outlen);
                break;

        case 0x63:
                sc_debug(ctx,  "PC_to_RDR_IccPowerOff");
                sc_result = perform_PC_to_RDR_IccPowerOff(inbuf, inlen, outbuf, &outlen);
                break;

        case 0x65:
                sc_debug(ctx,  "PC_to_RDR_GetSlotStatus");
                sc_result = perform_PC_to_RDR_GetSlotStatus(inbuf, inlen, outbuf, &outlen);
                break;

        case 0x6F:
                sc_debug(ctx,  "PC_to_RDR_XfrBlock");
                sc_result = perform_PC_to_RDR_XfrBlock(inbuf, inlen, outbuf, &outlen);
                break;

        case 0x6C:
                sc_debug(ctx,  "PC_to_RDR_GetParameters");
                sc_result = perform_PC_to_RDR_GetParamters(inbuf, inlen, outbuf, &outlen);
                break;

        case 0x69:
                sc_debug(ctx,  "PC_to_RDR_Secure");
                sc_result = perform_PC_to_RDR_Secure(inbuf, inlen, outbuf, &outlen);
                break;

        default:
                sc_error(ctx, "Unknown ccid bulk-in message. "
                        "Starting default handler...");
                sc_result = perform_unknown(inbuf, inlen, outbuf, &outlen);
    }

    if (sc_result < 0) {
        debug_sc_result(sc_result);
        return -1;
    }

    return outlen;
}

int ccid_parse_control(struct usb_ctrlrequest *setup, __u8 **outbuf)
{
    int r;
    __u16 value, index, length;
    __u8 *tmp;

    if (!setup || !outbuf)
        return -1;

    value = __le16_to_cpu(setup->wValue);
    index = __le16_to_cpu(setup->wIndex);
    length = __le16_to_cpu(setup->wLength);

    if (setup->bRequestType == USB_REQ_CCID) {
        switch(setup->bRequest) {
            case CCID_CONTROL_ABORT:
                sc_debug(ctx, "ABORT");
                if (length != 0x00) {
                    sc_debug(ctx, "warning: malformed ABORT");
                }

                r = 0;
                break;

            case CCID_CONTROL_GET_CLOCK_FREQUENCIES:
                sc_debug(ctx, "GET_CLOCK_FREQUENCIES");
                if (value != 0x00) {
                    sc_debug(ctx, "warning: malformed GET_CLOCK_FREQUENCIES");
                }

                r = sizeof(__le32);
                tmp = realloc(*outbuf, r);
                if (!tmp) {
                    r = SC_ERROR_OUT_OF_MEMORY;
                    break;
                }
                *outbuf = tmp;
                __le32 clock  = ccid_desc.dwDefaultClock;
                memcpy(*outbuf, &clock,  sizeof (__le32));
                break;

            case CCID_CONTROL_GET_DATA_RATES:
                sc_debug(ctx, "GET_DATA_RATES");
                if (value != 0x00) {
                    sc_debug(ctx, "warning: malformed GET_DATA_RATES");
                }

                r = sizeof (__le32);
                tmp = realloc(*outbuf, r);
                if (tmp == NULL) {
                    r = -1;
                    break;
                }
                *outbuf = tmp;
                __le32 drate  = ccid_desc.dwDataRate;
                memcpy(*outbuf, &drate,  sizeof (__le32));
                break;

            default:
                sc_error(ctx, "Unknown ccid control command.");

                r = SC_ERROR_NOT_SUPPORTED;
        }
    }

    if (r < 0)
        debug_sc_result(r);

    return r;
}

int ccid_state_changed(RDR_to_PC_NotifySlotChange_t **slotchange, int timeout)
{
    int sc_result;

    if (!slotchange)
        return 0;

    sc_result = get_RDR_to_PC_NotifySlotChange(slotchange);

    if (sc_result < 0) {
        debug_sc_result(sc_result);
    }

    if ((*slotchange)->bmSlotICCState)
        return 1;

    return 0;
}
