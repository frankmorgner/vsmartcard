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
#include <opensc/ui.h>
#include <opensc/log.h>

#include "ccid.h"
#include "pace.h"

static sc_context_t *ctx = NULL;
static sc_card_t *card_in_slot[SC_MAX_SLOTS];
static sc_reader_t *reader;

struct ccid_class_descriptor
ccid_desc = {
    .bLength                = sizeof ccid_desc,
    .bDescriptorType        = 0x21,
    .bcdCCID                = __constant_cpu_to_le16(0x0110),
    .bMaxSlotIndex          = SC_MAX_SLOTS,
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

static void
debug_sc_result(int sc_result)
{
    sc_debug(ctx, sc_strerror(sc_result));
}

static int
detect_card_presence(int slot)
{
    int sc_result;

    if (slot >= sizeof *card_in_slot)
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_VERBOSE, SC_ERROR_INVALID_ARGUMENTS);

    if (card_in_slot[slot] && sc_card_valid(card_in_slot[slot])) {
        sc_result = SC_SLOT_CARD_PRESENT;
    } else {
        sc_result = sc_detect_card_presence(reader, slot);
    }

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

    SC_FUNC_RETURN(ctx, SC_LOG_TYPE_VERBOSE, sc_result);
}


int ccid_initialize(int reader_id, const char *cdriver, int verbose)
{
    unsigned int i, reader_count;

    int r = sc_context_create(&ctx, NULL);
    if (r < 0) {
        printf("Failed to create initial context: %s", sc_strerror(r));
        return r;
    }

    if (cdriver != NULL) {
        r = sc_set_card_driver(ctx, cdriver);
        if (r < 0) {
            sc_error(ctx, "Card driver '%s' not found!\n", cdriver);
            return r;
        }
    }

    ctx->debug = verbose;

    for (i = 0; i < sizeof *card_in_slot; i++) {
        card_in_slot[i] = NULL;
    }

    reader_count = sc_ctx_get_reader_count(ctx);

    if (reader_count == 0)
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_ERROR_NO_READERS_FOUND);

    if (reader_id < 0) {
        /* Automatically try to skip to a reader with a card if reader not specified */
        for (i = 0; i < reader_count; i++) {
            reader = sc_ctx_get_reader(ctx, i);
            if (sc_detect_card_presence(reader, 0) & SC_SLOT_CARD_PRESENT) {
                reader_id = i;
                sc_debug(ctx, "Using reader with a card: %s", reader->name);
                break;
            }
        }
        if (reader_id >= reader_count) {
            /* no reader found, use the first */
            reader_id = 0;
        }
    }

    if (reader_id >= reader_count)
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_ERROR_NO_READERS_FOUND);

    reader = sc_ctx_get_reader(ctx, reader_id);
    ccid_desc.bMaxSlotIndex = reader->slot_count - 1;

    SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_SUCCESS);
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
}

int build_apdu(const __u8 *buf, size_t len, sc_apdu_t *apdu)
{
	const __u8 *p;
	size_t len0;

        if (!buf || !apdu)
            SC_FUNC_RETURN(ctx, SC_LOG_TYPE_VERBOSE, SC_ERROR_INVALID_ARGUMENTS);

	len0 = len;
	if (len < 4) {
                sc_error(ctx, "APDU too short (must be at least 4 bytes)");
                SC_FUNC_RETURN(ctx, SC_LOG_TYPE_VERBOSE, SC_ERROR_INVALID_DATA);
	}

	memset(apdu, 0, sizeof(*apdu));
	p = buf;
	apdu->cla = *p++;
	apdu->ins = *p++;
	apdu->p1 = *p++;
	apdu->p2 = *p++;
	len -= 4;
	if (len > 1) {
		apdu->lc = *p++;
		len--;
		apdu->data = p;
		apdu->datalen = apdu->lc;
		if (len < apdu->lc) {
                        sc_error(ctx, "APDU too short (need %lu bytes)\n",
                                (unsigned long) apdu->lc - len);
                        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_VERBOSE, SC_ERROR_INVALID_DATA);
		}
		len -= apdu->lc;
		p += apdu->lc;
		if (len) {
			apdu->le = *p++;
			if (apdu->le == 0)
				apdu->le = 256;
			len--;
			apdu->cse = SC_APDU_CASE_4_SHORT;
		} else {
			apdu->cse = SC_APDU_CASE_3_SHORT;
		}
		if (len) {
			sc_error(ctx, "APDU too long (%lu bytes extra)\n",
				(unsigned long) len);
                        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_VERBOSE, SC_ERROR_INVALID_DATA);
		}
	} else if (len == 1) {
		apdu->le = *p++;
		if (apdu->le == 0)
			apdu->le = 256;
		len--;
		apdu->cse = SC_APDU_CASE_2_SHORT;
	} else {
		apdu->cse = SC_APDU_CASE_1;
	}

        apdu->flags = SC_APDU_FLAGS_NO_GET_RESP|SC_APDU_FLAGS_NO_RETRY_WL;

        sc_debug(ctx, "APDU, %d bytes:\tins=%02x p1=%02x p2=%02x",
                (unsigned int) len0, apdu->ins, apdu->p1, apdu->p2);

        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_VERBOSE, SC_SUCCESS);
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
    apdu->resp = malloc(apdu->resplen + sizeof(__u8) + sizeof(__u8));
    if (!apdu->resp) {
        sc_result = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }

    sc_result = sc_transmit_apdu(card_in_slot[slot], apdu);
    if (sc_result < 0) {
        goto err;
    }

    if (apdu->sw1 > 0xff || apdu->sw2 > 0xff) {
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

    SC_FUNC_RETURN(ctx, SC_LOG_TYPE_VERBOSE, SC_SUCCESS);

err:
    if (apdu->resp) {
        free(apdu->resp);
    }

    SC_FUNC_RETURN(ctx, SC_LOG_TYPE_VERBOSE, sc_result);
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
    __u8 result = 0;

    flags = detect_card_presence(bSlot);

    if (flags >= 0) {
        if (sc_result < 0) {
            if (flags & SC_SLOT_CARD_PRESENT) {
                if (flags & SC_SLOT_CARD_CHANGED || (
                            card_in_slot[bSlot]
                            && !sc_card_valid(card_in_slot[bSlot]))) {
                    /*sc_debug(ctx, "error inactive");*/
                    result = CCID_BSTATUS_ERROR_INACTIVE;
                } else {
                    /*sc_debug(ctx, "error active");*/
                    result = CCID_BSTATUS_ERROR_ACTIVE;
                }
            } else {
                /*sc_debug(ctx, "error no icc");*/
                result = CCID_BSTATUS_ERROR_NOICC;
            }
        } else {
            if (flags & SC_SLOT_CARD_PRESENT) {
                if (flags & SC_SLOT_CARD_CHANGED || (
                            card_in_slot[bSlot]
                            && !sc_card_valid(card_in_slot[bSlot]))) {
                    /*sc_debug(ctx, "ok inactive");*/
                    result = CCID_BSTATUS_OK_INACTIVE;
                } else {
                    sc_debug(ctx, "ok active");
                    result = CCID_BSTATUS_OK_ACTIVE;
                }
            } else {
                /*sc_debug(ctx, "ok no icc");*/
                result = CCID_BSTATUS_OK_NOICC;
            }
        }
    } else {
        debug_sc_result(flags);
        sc_error(ctx, "Could not detect card presence."
                " Falling back to default (bStatus=0x%02X).", result);
    }

    return result;
}

static int
get_RDR_to_PC_SlotStatus(__u8 bSlot, __u8 bSeq, int sc_result, RDR_to_PC_SlotStatus_t **out)
{
    if (!out)
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_DEBUG, SC_ERROR_INVALID_ARGUMENTS);

    RDR_to_PC_SlotStatus_t *result = realloc(*out, sizeof *result);
    if (!result)
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_DEBUG, SC_ERROR_OUT_OF_MEMORY);
    *out = result;

    result->bMessageType = 0x81;
    result->dwLength     = __constant_cpu_to_le32(0);
    result->bSlot        = bSlot;
    result->bSeq         = bSeq;
    result->bStatus      = get_bStatus(sc_result, bSlot);
    result->bError       = get_bError(sc_result);
    result->bClockStatus = 0;

    SC_FUNC_RETURN(ctx, SC_LOG_TYPE_DEBUG, SC_SUCCESS);
}

static int
get_RDR_to_PC_DataBlock(__u8 bSlot, __u8 bSeq, int sc_result, RDR_to_PC_DataBlock_t **out)
{
    if (!out)
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_DEBUG, SC_ERROR_INVALID_ARGUMENTS);

    RDR_to_PC_DataBlock_t *result = realloc(*out, sizeof *result);
    if (!result)
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_DEBUG, SC_ERROR_OUT_OF_MEMORY);
    *out = result;

    result->bMessageType    = 0x80;
    result->dwLength        = __constant_cpu_to_le32(0);
    result->bSlot           = bSlot;
    result->bSeq            = bSeq;
    result->bStatus         = get_bStatus(sc_result, bSlot);
    result->bError          = get_bError(sc_result);
    result->bChainParameter = 0;

    SC_FUNC_RETURN(ctx, SC_LOG_TYPE_DEBUG, SC_SUCCESS);
}

static int
perform_PC_to_RDR_GetSlotStatus(const __u8 *in, __u8 **out, size_t *outlen)
{
    const PC_to_RDR_GetSlotStatus_t *request = (PC_to_RDR_GetSlotStatus_t *) in;

    if (!out || !outlen || !in)
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_ERROR_INVALID_ARGUMENTS);

    *outlen = sizeof(RDR_to_PC_SlotStatus_t);

    if (    request->bMessageType != 0x65 ||
            request->dwLength     != __constant_cpu_to_le32(0) ||
            request->abRFU1       != 0 ||
            request->abRFU2       != 0)
        sc_debug(ctx, "warning: malformed PC_to_RDR_GetSlotStatus");

    SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR,
            get_RDR_to_PC_SlotStatus(request->bSlot, request->bSeq, SC_SUCCESS,
                (RDR_to_PC_SlotStatus_t **) out));
}

static int
perform_PC_to_RDR_IccPowerOn(const __u8 *in, __u8 **out, size_t *outlen)
{
    const PC_to_RDR_IccPowerOn_t *request = (PC_to_RDR_IccPowerOn_t *) in;
    int sc_result;
    RDR_to_PC_SlotStatus_t *result;

    if (!out || !outlen || !in)
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_ERROR_INVALID_ARGUMENTS);

    if (    request->bMessageType != 0x62 ||
            request->dwLength     != __constant_cpu_to_le32(0) ||
            !( request->bPowerSelect == 0 ||
                request->bPowerSelect & ccid_desc.bVoltageSupport ) ||
            request->abRFU        != 0)
        sc_debug(ctx, "warning: malformed PC_to_RDR_IccPowerOn");

    if (request->bSlot < sizeof *card_in_slot) {
        if (card_in_slot[request->bSlot]
                && sc_card_valid(card_in_slot[request->bSlot])) {
            sc_disconnect_card(card_in_slot[request->bSlot], 0);
        }
        sc_result = sc_connect_card(reader, request->bSlot,
                &card_in_slot[request->bSlot]);
    } else {
        sc_result = SC_ERROR_INVALID_DATA;
    }

    SC_TEST_RET(ctx,
            get_RDR_to_PC_SlotStatus(request->bSlot, request->bSeq, sc_result,
                (RDR_to_PC_SlotStatus_t **) out),
            "Could not get RDR_to_PC_SlotStatus object");
    result = (RDR_to_PC_SlotStatus_t *) *out;

    if (sc_result >= 0) {
        *outlen = sizeof *result + card_in_slot[request->bSlot]->atr_len;
        result = realloc(*out, *outlen);
        if (result) {
            *out = (__u8 *) result;
            memcpy(*out + sizeof *result, card_in_slot[request->bSlot]->atr,
                    card_in_slot[request->bSlot]->atr_len);
            result->dwLength = __cpu_to_le32(card_in_slot[request->bSlot]->atr_len);
        } else {
            debug_sc_result(SC_ERROR_OUT_OF_MEMORY);
            sc_error(ctx, "Could not get memory for ATR."
                    " Returning package without payload.");
            *outlen = sizeof *result;
        }
    } else {
        debug_sc_result(sc_result);
        sc_error(ctx, "Could not connect to card in slot %d."
                " Returning default status package.", request->bSlot);
        *outlen = sizeof *result;
    }

    SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_SUCCESS);
}

static int
perform_PC_to_RDR_IccPowerOff(const __u8 *in, __u8 **out, size_t *outlen)
{
    const PC_to_RDR_IccPowerOff_t *request = (PC_to_RDR_IccPowerOff_t *) in;
    int sc_result;

    if (!in || !out || !outlen)
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_ERROR_INVALID_ARGUMENTS);

    if (    request->bMessageType != 0x63 ||
            request->dwLength     != __constant_cpu_to_le32(0) ||
            request->abRFU1       != 0 ||
            request->abRFU2       != 0)
        sc_debug(ctx, "warning: malformed PC_to_RDR_IccPowerOff");

    if (request->bSlot > sizeof *card_in_slot)
        sc_result = SC_ERROR_INVALID_DATA;
    else
        sc_result = sc_disconnect_card(card_in_slot[request->bSlot], 0);

    SC_TEST_RET(ctx,
            get_RDR_to_PC_SlotStatus(request->bSlot, request->bSeq, sc_result,
                (RDR_to_PC_SlotStatus_t **) out),
            "Could not get RDR_to_PC_SlotStatus object");

    *outlen = sizeof(RDR_to_PC_SlotStatus_t);

    SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_SUCCESS);
}

static int
perform_PC_to_RDR_XfrBlock(const u8 *in,  __u8** out, size_t *outlen)
{
    const PC_to_RDR_XfrBlock_t *request = (PC_to_RDR_XfrBlock_t *) in;
    const __u8 *abDataIn = in + sizeof *request;
    int sc_result, r;
    size_t resplen = 0;
    sc_apdu_t apdu;
    __u8 *abDataOut;
    RDR_to_PC_DataBlock_t *result;

    if (!in || !out || !outlen)
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_ERROR_INVALID_ARGUMENTS);

    if (    request->bMessageType != 0x6F ||
            request->bBWI  != 0)
        sc_debug(ctx, "malformed PC_to_RDR_XfrBlock, will continue anyway");

    if (request->bSlot > sizeof *card_in_slot)
        sc_result = SC_ERROR_INVALID_DATA;
    else {
        sc_result = build_apdu(abDataIn, request->dwLength, &apdu);
        if (sc_result >= 0)
            sc_result = get_rapdu(&apdu, request->bSlot, &abDataOut, &resplen);
    }

    r = get_RDR_to_PC_DataBlock(request->bSlot, request->bSeq, sc_result,
            (RDR_to_PC_DataBlock_t **) out);
    result = (RDR_to_PC_DataBlock_t *) *out;
    if (r < 0) {
        if (sc_result >= 0)
            free(abDataOut);
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, r);
    }

    if (sc_result >= 0) {
        *outlen = sizeof *result + resplen;
        result = realloc(*out, *outlen);
        if (!result) {
            *outlen = sizeof *result;
            free(abDataOut);
            sc_result = SC_ERROR_OUT_OF_MEMORY;
        } else {
            *out = (__u8 *) result;
            result->dwLength =  __cpu_to_le32(resplen);
            memcpy(*out + sizeof *result, abDataOut, resplen);
        }
    } else {
        *outlen = sizeof *result;
        debug_sc_result(sc_result);
        sc_error(ctx, "Could not get memory for response apdu."
                " Returning package without payload.");
    }
    result->bError = get_bError(sc_result);
    result->bStatus = get_bStatus(sc_result, request->bSlot);

    SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_SUCCESS);
}

static int
perform_PC_to_RDR_GetParamters(const __u8 *in, __u8** out, size_t *outlen)
{
    const PC_to_RDR_GetParameters_t *request = (PC_to_RDR_GetParameters_t *) in;
    RDR_to_PC_Parameters_t *result;
    abProtocolDataStructure_T1_t *t1;
    abProtocolDataStructure_T0_t *t0;
    int sc_result;

    if (!in || !out || !outlen)
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_ERROR_INVALID_ARGUMENTS);

    if (    request->bMessageType != 0x6C ||
            request->dwLength != __constant_cpu_to_le32(0))
        sc_debug(ctx, "warning: malformed PC_to_RDR_GetParamters");

    if (request->bSlot < sizeof *card_in_slot) {
        switch (reader->slot[request->bSlot].active_protocol) {
            case SC_PROTO_T0:
                result = realloc(*out, sizeof *result + sizeof *t0);
                if (!result)
                    SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_ERROR_OUT_OF_MEMORY);
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
                    SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_ERROR_OUT_OF_MEMORY);
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
            SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_ERROR_OUT_OF_MEMORY);
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

    SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_SUCCESS);
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
            /* only write pin length if it fits into a full byte */
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
                /* only do right aligned BCD which fits into full bytes */
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

static int
perform_PC_to_RDR_Secure(const __u8 *in, __u8** out, size_t *outlen)
{
    int sc_result, r;
    struct sc_pin_cmd_pin curr_pin, new_pin;
    sc_apdu_t apdu;
    sc_ui_hints_t hints;
    const PC_to_RDR_Secure_t *request = (PC_to_RDR_Secure_t *) in;
    const __u8* abData = in + sizeof *request;
    u8 *abDataOut;
    size_t resplen = 0;
    RDR_to_PC_DataBlock_t *result;

    if (!in || !out || !outlen)
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_ERROR_INVALID_ARGUMENTS);

    if (request->bMessageType != 0x69)
        sc_debug(ctx,  "warning: malformed PC_to_RDR_Secure");

    memset(&hints, 0, sizeof(hints));
    memset(&curr_pin, 0, sizeof(curr_pin));
    memset(&new_pin, 0, sizeof(new_pin));

    if (request->bSlot > sizeof *card_in_slot)
        goto err;

    if (request->wLevelParameter != CCID_WLEVEL_DIRECT) {
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
            wPINMaxExtraDigit = modify->wPINMaxExtraDigit;
            bmPINLengthFormat = modify->bmPINLengthFormat;
            bmPINBlockString = modify->bmPINBlockString;
            bmFormatString = modify->bmFormatString;
            bNumberMessage = modify->bNumberMessage;
            abPINApdu = (__u8*) modify + sizeof(*modify);
            apdulen = __le32_to_cpu(request->dwLength) - sizeof(*modify) - sizeof(__u8);
            break;
        case 0x04:
            // Cancel PIN function
        default:
            sc_result = SC_ERROR_NOT_SUPPORTED;
            goto err;
    }

    sc_result = build_apdu(abPINApdu, apdulen, &apdu);
    if (sc_result < 0)
        goto err;
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

    sc_debug(ctx,   "PIN %s block (%d bytes) proberties:\n"
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
    hints.dialog_name = "ccid.PC_to_RDR_Secure";
    hints.card = card_in_slot[request->bSlot];
    if (bNumberMessage == CCID_PIN_MSG_DEFAULT
            || bNumberMessage == CCID_PIN_MSG_REF
            || bNumberMessage == CCID_PIN_NO_MSG) {
        /* don't interprete bMsgIndex*, just use defaults */
        if (verify)
            hints.usage = SC_UI_USAGE_OTHER;
        else
            hints.usage = SC_UI_USAGE_CHANGE_PIN;
    } else if (bNumberMessage == CCID_PIN_MSG1)
        hints.usage = SC_UI_USAGE_OTHER;
    else if (bNumberMessage == CCID_PIN_MSG2)
        hints.usage = SC_UI_USAGE_NEW_PIN;
    else {
        sc_result = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }
    if (verify) {
        hints.prompt = "PIN Verification";
        sc_result = sc_ui_get_pin(&hints, (char **) &curr_pin.data);
    } else {
        hints.prompt = "PIN Modification";
        if (modify->bConfirmPIN & CCID_PIN_CONFIRM_NEW)
            hints.flags |= SC_UI_PIN_RETYPE;
        if (modify->bConfirmPIN & CCID_PIN_INSERT_OLD) {
            sc_result = sc_ui_get_pin_pair(&hints, (char **) &curr_pin.data,
                    (char **) &new_pin.data);
        } else {
            /* if only the new pin is requested, it is stored in curr_pin */
            sc_result = sc_ui_get_pin(&hints, (char **) &curr_pin.data);
        }
    }
    if (sc_result < 0)
        goto err;

    /* set and check length of PIN */
    curr_pin.len = strlen((char *) curr_pin.data);
    if ((curr_pin.max_length && curr_pin.len > curr_pin.max_length)
            || curr_pin.len < curr_pin.min_length) {
        sc_result = SC_ERROR_PIN_CODE_INCORRECT;
        goto err;
    }
    if (modify) {
        new_pin.len = strlen((char *) new_pin.data);
        if ((new_pin.max_length && new_pin.len > new_pin.max_length)
                || new_pin.len < new_pin.min_length) {
            sc_result = SC_ERROR_PIN_CODE_INCORRECT;
            goto err;
        }
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

    sc_result = get_rapdu(&apdu, request->bSlot, &abDataOut, &resplen);

    sc_mem_clear(abPINApdu, apdulen);
err:
    if (curr_pin.data) {
        sc_mem_clear((u8 *) curr_pin.data, curr_pin.len);
        free((u8 *) curr_pin.data);
    }
    if (new_pin.data) {
        sc_mem_clear((u8 *) new_pin.data, new_pin.len);
        free((u8 *) new_pin.data);
    }

    r = get_RDR_to_PC_DataBlock(request->bSlot, request->bSeq, sc_result, (RDR_to_PC_DataBlock_t **) out);
    result = (RDR_to_PC_DataBlock_t *) *out;
    if (r < 0) {
        if (sc_result >= 0)
            free(abDataOut);
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, r);
    }

    if (sc_result >= 0) {
        *outlen = sizeof *result + resplen;
        result = realloc(*out, *outlen);
        if (!result) {
            *outlen = sizeof *result;
            free(abDataOut);
            sc_result = SC_ERROR_OUT_OF_MEMORY;
        } else {
            *out = (__u8 *) result;
            result->dwLength =  __cpu_to_le32(resplen);
            memcpy(*out + sizeof *result, abDataOut, resplen);
        }
    } else {
        *outlen = sizeof *result;
        debug_sc_result(sc_result);
        sc_error(ctx, "Could not get memory for response apdu."
                " Returning package without payload.");
    }
    result->bError = get_bError(sc_result);
    result->bStatus = get_bStatus(sc_result, request->bSlot);

    SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, r);
}

static int
get_RDR_to_PC_NotifySlotChange(RDR_to_PC_NotifySlotChange_t **out)
{
    int i;
    int sc_result;
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
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_DEBUG, SC_ERROR_INVALID_ARGUMENTS);

    RDR_to_PC_NotifySlotChange_t *result = realloc(*out, sizeof *result);
    if (!result)
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_DEBUG, SC_ERROR_OUT_OF_MEMORY);
    *out = result;

    result->bMessageType = 0x50;
    result->bmSlotICCState = CCID_SLOTS_UNCHANGED;

    for (i = 0; i < reader->slot_count; i++) {
        sc_result = detect_card_presence(i);
        if (sc_result < 0) {
            debug_sc_result(sc_result);
            sc_error(ctx, "Could not detect card presence, skipping slot %d.",
                    i);
            continue;
        }

        if (sc_result & SC_SLOT_CARD_PRESENT)
            result->bmSlotICCState |= present[i];
        if (sc_result & SC_SLOT_CARD_CHANGED) {
            result->bmSlotICCState |= changed[i];
        }
    }

    SC_FUNC_RETURN(ctx, SC_LOG_TYPE_DEBUG, SC_SUCCESS);
}

static int
perform_unknown(const __u8 *in, __u8 **out, size_t *outlen)
{
    const PC_to_RDR_GetSlotStatus_t *request = (PC_to_RDR_GetSlotStatus_t *) in;
    RDR_to_PC_SlotStatus_t *result;

    if (!in || !out || !outlen)
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_ERROR_INVALID_ARGUMENTS);

    result = realloc(*out, sizeof *result);
    if (!result)
        SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_ERROR_OUT_OF_MEMORY);
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
            sc_debug(ctx, "Unknown message type in request. "
                    "Using bMessageType=0x%2x for output.", 0);
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

    SC_FUNC_RETURN(ctx, SC_LOG_TYPE_ERROR, SC_SUCCESS);
}

int ccid_parse_bulkin(const __u8* inbuf, __u8** outbuf)
{
    int sc_result;
    size_t outlen;

    if (!inbuf)
        return 0;

    switch (*inbuf) {
        case 0x62: 
                sc_debug(ctx,  "PC_to_RDR_IccPowerOn");
                sc_result = perform_PC_to_RDR_IccPowerOn(inbuf, outbuf, &outlen);
                break;

        case 0x63:
                sc_debug(ctx,  "PC_to_RDR_IccPowerOff");
                sc_result = perform_PC_to_RDR_IccPowerOff(inbuf, outbuf, &outlen);
                break;

        case 0x65:
                sc_debug(ctx,  "PC_to_RDR_GetSlotStatus");
                sc_result = perform_PC_to_RDR_GetSlotStatus(inbuf, outbuf, &outlen);
                break;

        case 0x6F:
                sc_debug(ctx,  "PC_to_RDR_XfrBlock");
                sc_result = perform_PC_to_RDR_XfrBlock(inbuf, outbuf, &outlen);
                break;

        case 0x6C:
                sc_debug(ctx,  "PC_to_RDR_GetParameters");
                sc_result = perform_PC_to_RDR_GetParamters(inbuf, outbuf, &outlen);
                break;

        case 0x69:
                sc_debug(ctx,  "PC_to_RDR_Secure");
                sc_result = perform_PC_to_RDR_Secure(inbuf, outbuf, &outlen);
                break;

        default:
                sc_debug(ctx, "Unknown ccid bulk-in message. "
                        "Starting default handler.");
                sc_result = perform_unknown(inbuf, outbuf, &outlen);
    }

    if (sc_result < 0) {
        debug_sc_result(sc_result);
        return -1;
    }

    return outlen;
}

int ccid_parse_control(struct usb_ctrlrequest *setup, __u8 **outbuf)
{
    int result;
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

                result = 0;
                break;

            case CCID_CONTROL_GET_CLOCK_FREQUENCIES:
                sc_debug(ctx, "GET_CLOCK_FREQUENCIES");
                if (value != 0x00) {
                    sc_debug(ctx, "warning: malformed GET_CLOCK_FREQUENCIES");
                }

                result = sizeof(__le32);
                tmp = realloc(*outbuf, result);
                if (!tmp) {
                    result = SC_ERROR_OUT_OF_MEMORY;
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

                result = sizeof (__le32);
                tmp = realloc(*outbuf, result);
                if (tmp == NULL) {
                    result = -1;
                    break;
                }
                *outbuf = tmp;
                __le32 drate  = ccid_desc.dwDataRate;
                memcpy(*outbuf, &drate,  sizeof (__le32));
                break;

            default:
                sc_error(ctx, "Unknown ccid control command.");

                result = SC_ERROR_NOT_SUPPORTED;
        }
    }

    if (result < 0)
        debug_sc_result(result);

    return result;
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

int ccid_testpace(u8 pin_id, const char *pin, size_t pinlen)
{
    int i;
    for (i = 0; i < sizeof *card_in_slot; i++) {
        if (sc_detect_card_presence(reader, 0) & SC_SLOT_CARD_PRESENT) {
            if (!card_in_slot[i] || !sc_card_valid(card_in_slot[i])) {
                sc_connect_card(reader, i, &card_in_slot[i]);
            }
            return pace_test(card_in_slot[i], pin_id, pin, pinlen);
        }
    }

    sc_error(ctx, "No card found.");

    return SC_ERROR_SLOT_NOT_FOUND;
}

static int ccid_list_readers(sc_context_t *ctx)
{
	unsigned int i, rcount = sc_ctx_get_reader_count(ctx);
	
	if (rcount == 0) {
		printf("No smart card readers found.\n");
		return 0;
	}
	printf("Readers known about:\n");
	printf("Nr.    Driver     Name\n");
	for (i = 0; i < rcount; i++) {
		sc_reader_t *screader = sc_ctx_get_reader(ctx, i);
		printf("%-7d%-11s%s\n", i, screader->driver->short_name,
		       screader->name);
	}

	return 0;
}

static int ccid_list_drivers(sc_context_t *ctx)
{
	int i;
	
	if (ctx->card_drivers[0] == NULL) {
		printf("No card drivers installed!\n");
		return 0;
	}
	printf("Configured card drivers:\n");
	for (i = 0; ctx->card_drivers[i] != NULL; i++) {
		printf("  %-16s %s\n", ctx->card_drivers[i]->short_name,
		       ctx->card_drivers[i]->name);
	}

	return 0;
}

int ccid_print_avail(int verbose)
{
	sc_context_t *ctx = NULL;

	int r;
	r = sc_context_create(&ctx, NULL);
	if (r) {
		fprintf(stderr, "Failed to establish context: %s\n", sc_strerror(r));
		return 1;
	}
	ctx->debug = verbose;

	r = ccid_list_readers(ctx)|ccid_list_drivers(ctx);

	if (ctx)
		sc_release_context(ctx);

	return r;
}
