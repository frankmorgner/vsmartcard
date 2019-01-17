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
#include <asm/byteorder.h>
#include <libopensc/log.h>
#include <libopensc/opensc.h>
#include <libopensc/reader-boxing.h>
#include <libopensc/sm.h>
#include <sm/sm-eac.h>
#include <openssl/evp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ccid.h"
#include "config.h"
#include "scutil.h"

#ifndef HAVE_BOXING_BUF_TO_PACE_INPUT
#include <libopensc/reader-boxing.c>
#endif

static sc_context_t *ctx = NULL;
static sc_card_t *card = NULL;
static sc_reader_t *reader = NULL;

static int
perform_pseudo_apdu_EstablishPACEChannel(sc_apdu_t *apdu)
{
    struct establish_pace_channel_input  pace_input;
    struct establish_pace_channel_output pace_output;
    int r;

    memset(&pace_input, 0, sizeof pace_input);
    memset(&pace_output, 0, sizeof pace_output);

    r = boxing_buf_to_pace_input(reader->ctx, apdu->data, apdu->datalen,
            &pace_input);
    if (r < 0)
        goto err;

    r = perform_pace(card, pace_input, &pace_output,
            EAC_TR_VERSION_2_02);
    if (r < 0)
        goto err;

    r = boxing_pace_output_to_buf(reader->ctx, &pace_output, &apdu->resp,
            &apdu->resplen);

err:
    free((unsigned char *) pace_input.chat);
    free((unsigned char *) pace_input.certificate_description);
    free((unsigned char *) pace_input.pin);
    free(pace_output.ef_cardaccess);
    free(pace_output.recent_car);
    free(pace_output.previous_car);
    free(pace_output.id_icc);
    free(pace_output.id_pcd);

    return r;
}

static int
perform_pseudo_apdu_GetReaderPACECapabilities(sc_apdu_t *apdu)
{
    unsigned long sc_reader_t_capabilities = SC_READER_CAP_PACE_GENERIC
        | SC_READER_CAP_PACE_EID | SC_READER_CAP_PACE_ESIGN;


    return boxing_pace_capabilities_to_buf(reader->ctx,
            sc_reader_t_capabilities, &apdu->resp, &apdu->resplen);
}

static int
perform_PC_to_RDR_GetSlotStatus(const __u8 *in, size_t inlen, __u8 **out, size_t *outlen);
static int
perform_PC_to_RDR_IccPowerOn(const __u8 *in, size_t inlen, __u8 **out, size_t *outlen);
static int
perform_PC_to_RDR_IccPowerOff(const __u8 *in, size_t inlen, __u8 **out, size_t *outlen);
static int
perform_pseudo_apdu(sc_reader_t *reader, sc_apdu_t *apdu);
static int
perform_PC_to_RDR_XfrBlock(const u8 *in, size_t inlen, __u8** out, size_t *outlen);
static int
perform_PC_to_RDR_GetParamters(const __u8 *in, size_t inlen, __u8** out, size_t *outlen);
static int
perform_PC_to_RDR_Secure_EstablishPACEChannel(sc_card_t *card,
        const __u8 *abData, size_t abDatalen,
        __u8 **abDataOut, size_t *abDataOutLen);
static int
perform_PC_to_RDR_Secure_GetReadersPACECapabilities(__u8 **abDataOut,
        size_t *abDataOutLen);
static int
perform_PC_to_RDR_Secure(const __u8 *in, size_t inlen, __u8** out, size_t *outlen);
static int
perform_unknown(const __u8 *in, size_t inlen, __u8 **out, size_t *outlen);

unsigned int skipfirst = 0;

struct ccid_class_descriptor
ccid_desc = {
    .bLength                = sizeof ccid_desc,
    .bDescriptorType        = 0x21,
    .bcdCCID                = __constant_cpu_to_le16(0x0110),
    .bMaxSlotIndex          = 0,
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
        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, sc_strerror(sc_result)); \
    else \
        sc_debug(ctx, SC_LOG_DEBUG_NORMAL, sc_strerror(sc_result)); \
}

static int
detect_card_presence(void)
{
    int sc_result;

    sc_result = sc_detect_card_presence(reader);

    if (sc_result == 0
            && card) {
		/* FIXME recent OpenSC versions throw an error when disconnecting an
		 * obsolete card handle
        sc_disconnect_card(card); */
		card = NULL;
        sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "Card removed");
    }
    if (sc_result & SC_READER_CARD_CHANGED) {
		/* FIXME recent OpenSC versions throw an error when disconnecting an
		 * obsolete card handle
        sc_disconnect_card(card); */
		card = NULL;
        sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "Card exchanged");
    }
    if (sc_result & SC_READER_CARD_PRESENT
            && !card) {
        sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "Unused card");
    }

    return sc_result;
}


int ccid_initialize(int reader_id, int verbose)
{
    int i;

    i = initialize(reader_id, verbose, &ctx, &reader);
    if (i < 0)
        return i;

    return SC_SUCCESS;
}

void ccid_shutdown(void)
{
    sc_sm_stop(card);

    if (card) {
        sc_disconnect_card(card);
    }
    if (ctx)
        sc_release_context(ctx);
}

static int get_rapdu(sc_apdu_t *apdu, __u8 **buf, size_t *resplen)
{
    int sc_result;

    if (!apdu || !buf || !resplen || !card) {
        sc_result = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }

    apdu->resplen = apdu->le;
    if (0 != apdu->resplen) {
        apdu->resp = realloc(*buf, apdu->resplen);
        if (!apdu->resp) {
            sc_result = SC_ERROR_OUT_OF_MEMORY;
            goto err;
        }
        *buf = apdu->resp;
    }

    if (apdu->cla == 0xff) {
        sc_result = perform_pseudo_apdu(card->reader, apdu);
    } else {
        sc_result = sc_transmit_apdu(card, apdu);
    }
    if (sc_result < 0) {
        goto err;
    }

    if (apdu->sw1 > 0xff || apdu->sw2 > 0xff) {
        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Received invalid status bytes SW1=%d SW2=%d", apdu->sw1, apdu->sw2);
        sc_result = SC_ERROR_INVALID_DATA;
        goto err;
    }

    /* Get two more bytes to use as return buffer including sw1 and sw2 */
    *buf = realloc(apdu->resp, apdu->resplen + sizeof(__u8) + sizeof(__u8));
    if (!*buf) {
        sc_result = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }

    (*buf)[apdu->resplen] = apdu->sw1;
    (*buf)[apdu->resplen + sizeof(__u8)] = apdu->sw2;
    *resplen = apdu->resplen + sizeof(__u8) + sizeof(__u8);

    sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "R-APDU, %d byte%s:\tsw1=%02x sw2=%02x",
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

static __u8 get_bStatus(int sc_result)
{
    int flags;
    __u8 bstatus = 0;

	if (skipfirst > 10)
		flags = detect_card_presence();
	else {
		skipfirst += 1;
		flags = SC_SUCCESS;
	}

    if (flags >= 0) {
        if (sc_result < 0) {
            if (flags & SC_READER_CARD_PRESENT) {
                if (flags & SC_READER_CARD_CHANGED
                        && !card) {
                    sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "error inactive");
                    bstatus = CCID_BSTATUS_ERROR_INACTIVE;
                } else {
                    sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "error active");
                    bstatus = CCID_BSTATUS_ERROR_ACTIVE;
                }
            } else {
                sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "error no icc");
                bstatus = CCID_BSTATUS_ERROR_NOICC;
            }
        } else {
            if (flags & SC_READER_CARD_PRESENT) {
                if (flags & SC_READER_CARD_CHANGED
                        && !card) {
                    sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "ok inactive");
                    bstatus = CCID_BSTATUS_OK_INACTIVE;
                } else {
                    sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "ok active");
                    bstatus = CCID_BSTATUS_OK_ACTIVE;
                }
            } else {
                sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "ok no icc");
                bstatus = CCID_BSTATUS_OK_NOICC;
            }
        }
    } else {
        debug_sc_result(flags);
        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Could not detect card presence."
                " Falling back to default (bStatus=0x%02X).", bstatus);
    }

    return bstatus;
}

static int
get_RDR_to_PC_SlotStatus(__u8 bSeq, int sc_result, __u8 **outbuf, size_t *outlen,
        const __u8 *abProtocolDataStructure, size_t abProtocolDataStructureLen)
{
    if (!outbuf)
        return SC_ERROR_INVALID_ARGUMENTS;
    if (abProtocolDataStructureLen > 0xffff) {
        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "abProtocolDataStructure %u bytes too long",
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
    status->bSlot        = 0;
    status->bSeq         = bSeq;
    status->bStatus      = get_bStatus(sc_result);
    status->bError       = get_bError(sc_result);
    status->bClockStatus = 0;

    /* Flawfinder: ignore */
    memcpy((*outbuf) + sizeof(*status), abProtocolDataStructure, abProtocolDataStructureLen);

    return SC_SUCCESS;
}

static int
get_RDR_to_PC_DataBlock(__u8 bSeq, int sc_result, __u8 **outbuf,
        size_t *outlen, const __u8 *abData, size_t abDataLen)
{
    if (!outbuf)
        return SC_ERROR_INVALID_ARGUMENTS;
    if (abDataLen > 0xffff) {
        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "abProtocolDataStructure %u bytes too long",
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
    data->bSlot           = 0;
    data->bSeq            = bSeq;
    data->bStatus         = get_bStatus(sc_result);
    data->bError          = get_bError(sc_result);
    data->bChainParameter = 0;

    /* Flawfinder: ignore */
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
        SC_FUNC_RETURN(ctx, SC_LOG_DEBUG_VERBOSE, SC_ERROR_INVALID_DATA);

    *outlen = sizeof(RDR_to_PC_SlotStatus_t);

    if (request->bMessageType != 0x65
            || request->dwLength != __constant_cpu_to_le32(0)
            || request->bSlot != 0
            || request->abRFU1 != 0
            || request->abRFU2 != 0)
        sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "warning: malformed PC_to_RDR_GetSlotStatus");

    return get_RDR_to_PC_SlotStatus(request->bSeq, SC_SUCCESS,
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
        SC_FUNC_RETURN(ctx, SC_LOG_DEBUG_VERBOSE, SC_ERROR_INVALID_DATA);

    if (request->bMessageType != 0x62
            || request->dwLength != __constant_cpu_to_le32(0)
            || request->bSlot != 0
            || !( request->bPowerSelect == 0
                || request->bPowerSelect & ccid_desc.bVoltageSupport)
            || request->abRFU != 0)
        sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "warning: malformed PC_to_RDR_IccPowerOn");

    if (card) {
        sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "Card is already powered on.");
        sc_result = SC_SUCCESS;
    } else {
        sc_sm_stop(card);
        sc_result = sc_connect_card(reader, &card);
        card->caps |= SC_CARD_CAP_APDU_EXT;
    }

    if (sc_result >= 0) {
#ifdef WITH_PACE
#ifndef DISABLE_GLOBAL_BOXING_INITIALIZATION
        sc_initialize_boxing_cmds(ctx);
#endif
#endif
        return get_RDR_to_PC_SlotStatus(request->bSeq,
                sc_result, out, outlen, card->atr.value, card->atr.len);
    } else {
        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Returning default status package.");
        return get_RDR_to_PC_SlotStatus(request->bSeq,
                sc_result, out, outlen, NULL, 0);
    }
}

static int
perform_PC_to_RDR_IccPowerOff(const __u8 *in, size_t inlen, __u8 **out, size_t *outlen)
{
    const PC_to_RDR_IccPowerOff_t *request = (PC_to_RDR_IccPowerOff_t *) in;
    int sc_result = SC_SUCCESS;

    if (!in || !out || !outlen)
        return SC_ERROR_INVALID_ARGUMENTS;

    if (inlen < sizeof *request)
        SC_FUNC_RETURN(ctx, SC_LOG_DEBUG_VERBOSE, SC_ERROR_INVALID_DATA);

    if (request->bMessageType != 0x63
            || request->dwLength != __constant_cpu_to_le32(0)
            || request->bSlot != 0
            || request->abRFU1 != 0
            || request->abRFU2 != 0)
        sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "warning: malformed PC_to_RDR_IccPowerOff");

    //sc_reset(card, 1);
    //sc_result = sc_disconnect_card(card);

    return get_RDR_to_PC_SlotStatus(request->bSeq, sc_result,
                out, outlen, NULL, 0);
}

struct sw {
    unsigned char sw1;
    unsigned char sw2;
};
static const struct sw iso_sw_ok = { 0x90, 0x00};
static const struct sw iso_sw_incorrect_p1_p2 = { 0x6A, 0x86};
static const struct sw iso_sw_ref_data_not_found = {0x6A, 0x88};
static const struct sw iso_sw_inconsistent_data = {0x6A, 0x87};
static const struct sw iso_sw_func_not_supported = {0x6A, 0x81};
static const struct sw iso_sw_ins_not_supported = {0x6D, 0x00};

#define min(a,b) (a<b?a:b)
static int
perform_pseudo_apdu(sc_reader_t *reader, sc_apdu_t *apdu)
{
    if (!reader || !apdu || apdu->cla != 0xff)
        return SC_ERROR_INVALID_ARGUMENTS;

    apdu->resplen = 0;
	switch (apdu->ins) {
		case 0x9A:

            switch (apdu->p1) {
                case 0x01:
                    /* GetReaderInfo */
                    if (apdu->datalen != 0) {
                        apdu->sw1 = iso_sw_incorrect_p1_p2.sw1;
                        apdu->sw2 = iso_sw_incorrect_p1_p2.sw2;
                        goto err;
                    }
                    /* TODO Merge this with STRINGID_MFGR, STRINGID_PRODUCT in usb.c */
                    /* Copied from olsc/AusweisApp/Data/siqTerminalsInfo.cfg */
                    char *Herstellername = "REINER SCT";
                    char *Produktname = "cyberJack RFID komfort";
                    char *Firmwareversion = "1.0";
                    char *Treiberversion = "3.99.5";
                    switch (apdu->p2) {
                        case 0x01:
                            apdu->resplen = min(apdu->resplen, strlen(Herstellername));
                            memcpy(apdu->resp, Herstellername, apdu->resplen);
                            break;
                        case 0x03:
                            apdu->resplen = min(apdu->resplen, strlen(Produktname));
                            memcpy(apdu->resp, Produktname, apdu->resplen);
                            break;
                        case 0x06:
                            apdu->resplen = min(apdu->resplen, strlen(Firmwareversion));
                            memcpy(apdu->resp, Firmwareversion, apdu->resplen);
                            break;
                        case 0x07:
                            apdu->resplen = min(apdu->resplen, strlen(Treiberversion));
                            memcpy(apdu->resp, Treiberversion, apdu->resplen);
                            break;
                        default:
                            apdu->sw1 = iso_sw_ref_data_not_found.sw1;
                            apdu->sw2 = iso_sw_ref_data_not_found.sw2;
                            goto err;
                    }
                    break;

                case 0x04:
                    switch (apdu->p2) {
                        case 0x10:
                            /* VerifyPIN/ModifyPIN */
                            LOG_TEST_RET(ctx,
                                    perform_PC_to_RDR_Secure(apdu->data, apdu->datalen, &apdu->resp, &apdu->resplen),
                                    "Could not perform PC_to_RDR_Secure");
                            apdu->sw1 = iso_sw_ok.sw1;
                            apdu->sw2 = iso_sw_ok.sw2;
                            break;
                        case 0x01:
                            /* GetReaderPACECapabilities */
                            LOG_TEST_RET(ctx,
                                    perform_pseudo_apdu_GetReaderPACECapabilities(apdu),
                                    "Could not get reader's PACE Capabilities");
                            apdu->sw1 = iso_sw_ok.sw1;
                            apdu->sw2 = iso_sw_ok.sw2;
                            break;
                        case 0x02:
                            /* EstablishPACEChannel */
                            LOG_TEST_RET(ctx,
                                    perform_pseudo_apdu_EstablishPACEChannel(apdu),
                                    "Could not perform PACE");
                            apdu->sw1 = iso_sw_ok.sw1;
                            apdu->sw2 = iso_sw_ok.sw2;
                            break;
                        case 0x03:
                            /* DestroyPACEChannel */
                        default:
                            apdu->sw1 = iso_sw_func_not_supported.sw1;
                            apdu->sw2 = iso_sw_func_not_supported.sw2;
                            goto err;
                    }
                    break;

                default:
                    apdu->sw1 = iso_sw_func_not_supported.sw1;
                    apdu->sw2 = iso_sw_func_not_supported.sw2;
                    goto err;
            }
            break;

		default:
            apdu->sw1 = iso_sw_ins_not_supported.sw1;
            apdu->sw2 = iso_sw_ins_not_supported.sw2;
            goto err;
	}

err:
    return SC_SUCCESS;
}

static int
perform_PC_to_RDR_XfrBlock(const u8 *in, size_t inlen, __u8** out, size_t *outlen)
{
    const PC_to_RDR_XfrBlock_t *request = (PC_to_RDR_XfrBlock_t *) in;
    const __u8 *abDataIn = in + sizeof *request;
    int sc_result;
    size_t abDataOutLen = 0, apdulen;
    sc_apdu_t apdu;
    __u8 *abDataOut = NULL;

    if (!in || !out || !outlen)
        return SC_ERROR_INVALID_ARGUMENTS;

    if (inlen < sizeof *request)
        SC_FUNC_RETURN(ctx, SC_LOG_DEBUG_VERBOSE, SC_ERROR_INVALID_DATA);

    if (request->bMessageType != 0x6F
            || request->bSlot != 0
            || request->bBWI  != 0)
        sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "malformed PC_to_RDR_XfrBlock, will continue anyway");

	apdulen = __le32_to_cpu(request->dwLength);
	if (inlen < apdulen+sizeof *request) {
        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Not enough Data for APDU");
        SC_FUNC_RETURN(ctx, SC_LOG_DEBUG_VERBOSE, SC_ERROR_INVALID_DATA);
	}

    sc_result = sc_bytes2apdu(ctx, abDataIn, apdulen, &apdu);
    if (sc_result >= 0)
        sc_result = get_rapdu(&apdu, &abDataOut,
                &abDataOutLen);
    else
        bin_log(ctx, SC_LOG_DEBUG_VERBOSE, "Invalid APDU", abDataIn,
                __le32_to_cpu(request->dwLength));

    sc_result = get_RDR_to_PC_DataBlock(request->bSeq, sc_result,
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
        SC_FUNC_RETURN(ctx, SC_LOG_DEBUG_VERBOSE, SC_ERROR_INVALID_DATA);

    if (request->bMessageType != 0x6C
            || request->dwLength != __constant_cpu_to_le32(0)
            || request->bSlot != 0)
        sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "warning: malformed PC_to_RDR_GetParamters");

    switch (reader->active_protocol) {
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

            t1 = (abProtocolDataStructure_T1_t *) (result + sizeof *result);
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
            sc_result = SC_ERROR_INVALID_DATA;
            break;
    }

    result = realloc(*out, sizeof *result);
    if (!result)
        return SC_ERROR_OUT_OF_MEMORY;
    *out = (__u8 *) result;

    result->bMessageType = 0x82;
    result->bSlot = 0;
    result->bSeq = request->bSeq;
    result->bStatus = get_bStatus(sc_result);
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

static size_t
get_datasize_for_pin(const struct sc_pin_cmd_pin *pin, uint8_t length_size)
{
    size_t bytes_for_length, bytes_for_pin;

    if (!pin)
        return 0;

    bytes_for_length = pin->length_offset + (length_size+7)/8;
    if (pin->encoding == CCID_PIN_ENCODING_BCD)
        bytes_for_pin = pin->offset + (pin->len+1)/2;
    else
        bytes_for_pin = pin->offset + pin->len;

    if (bytes_for_length > bytes_for_pin)
        return bytes_for_length;

    return bytes_for_pin;
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
            sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Writing PIN length only if it fits into "
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
            sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "PIN encoding not supported (0x%02x)", encoding);
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
                sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Right aligning BCD encoded PIN only if it fits "
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
    __le32 dword;
    __u8 *p;

    memset(&pace_input, 0, sizeof pace_input);
    memset(&pace_output, 0, sizeof pace_output);


    if (!abDataOut || !abDataOutLen) {
        sc_result = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }

	bin_log(ctx, SC_LOG_DEBUG_VERBOSE, "EstablishPACEChannel InBuffer",
			abData, abDatalen);

    if (abDatalen < parsed+1) {
        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Buffer too small, could not get PinID");
        sc_result = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }
    pace_input.pin_id = abData[parsed];
    parsed++;


    if (abDatalen < parsed+1) {
        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Buffer too small, could not get lengthCHAT");
        sc_result = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }
    pace_input.chat_length = abData[parsed];
    parsed++;

    if (abDatalen < parsed+pace_input.chat_length) {
        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Buffer too small, could not get CHAT");
        sc_result = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }
    pace_input.chat = &abData[parsed];
    parsed += pace_input.chat_length;
    if (pace_input.chat_length) {
        bin_log(ctx, SC_LOG_DEBUG_VERBOSE, "Card holder authorization template",
                pace_input.chat, pace_input.chat_length);
    }


    if (abDatalen < parsed+1) {
        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Buffer too small, could not get lengthPIN");
        sc_result = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }
    pace_input.pin_length = abData[parsed];
    parsed++;

    if (abDatalen < parsed+pace_input.pin_length) {
        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Buffer too small, could not get PIN");
        sc_result = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }
    pace_input.pin = &abData[parsed];
    parsed += pace_input.pin_length;


    if (abDatalen < parsed+sizeof word) {
        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Buffer too small, could not get lengthCertificateDescription");
        sc_result = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }
    /* Flawfinder: ignore */
    memcpy(&word, &abData[parsed], sizeof word);
    pace_input.certificate_description_length = __le16_to_cpu(word);
    parsed += sizeof word;

    if (abDatalen < parsed+pace_input.certificate_description_length) {
        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Buffer too small, could not get CertificateDescription");
        sc_result = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }
    pace_input.certificate_description = &abData[parsed];
    if (pace_input.certificate_description_length) {
        bin_log(ctx, SC_LOG_DEBUG_VERBOSE, "Certificate description",
                pace_input.certificate_description,
                pace_input.certificate_description_length);
    }


    sc_result = perform_pace(card, pace_input, &pace_output,
            EAC_TR_VERSION_2_02);
    if (sc_result < 0)
        goto err;


    p = realloc(*abDataOut,
            4 +                                     /* Result */
            2 +                                     /* length Output data */
            2 +                                     /* Statusbytes */
            2+pace_output.ef_cardaccess_length +    /* EF.CardAccess */
            1+pace_output.recent_car_length +       /* Most recent CAR */
            1+pace_output.previous_car_length +     /* Previous CAR */
            2+pace_output.id_icc_length);           /* identifier of the MRTD chip */
    if (!p) {
        sc_result = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    *abDataOut = p;


    dword = __cpu_to_le32(pace_output.result);
    /* Flawfinder: ignore */
    memcpy(p, &dword, sizeof dword);
    p += sizeof dword;


    word = __cpu_to_le16(
            2 +
            2+pace_output.ef_cardaccess_length +
            1+pace_output.recent_car_length +
            1+pace_output.previous_car_length +
            2+pace_output.id_icc_length);
    /* Flawfinder: ignore */
    memcpy(p, &word, sizeof word);
    p += sizeof word;


    *p = pace_output.mse_set_at_sw1;
    p += 1;

    *p = pace_output.mse_set_at_sw2;
    p += 1;


    if (pace_output.ef_cardaccess_length > 0xffff) {
        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "EF.CardAcces %u bytes too long",
                pace_output.ef_cardaccess_length-0xffff);
        sc_result = SC_ERROR_INVALID_DATA;
        goto err;
    }
    word = __cpu_to_le16(pace_output.ef_cardaccess_length);
    /* Flawfinder: ignore */
    memcpy(p, &word, sizeof word);
    p += sizeof word;

    /* Flawfinder: ignore */
    memcpy(p, pace_output.ef_cardaccess,
            pace_output.ef_cardaccess_length);
    p += pace_output.ef_cardaccess_length;


    if (pace_output.recent_car_length > 0xff) {
        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Most recent CAR %u bytes too long",
                pace_output.recent_car_length-0xff);
        sc_result = SC_ERROR_INVALID_DATA;
        goto err;
    }
    *p = pace_output.recent_car_length;
    p += 1;

    /* Flawfinder: ignore */
    memcpy(p, pace_output.recent_car,
            pace_output.recent_car_length);
    p += pace_output.recent_car_length;


    if (pace_output.previous_car_length > 0xff) {
        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Previous CAR %u bytes too long",
                pace_output.previous_car_length-0xff);
        sc_result = SC_ERROR_INVALID_DATA;
        goto err;
    }
    *p = pace_output.previous_car_length;
    p += 1;

    /* Flawfinder: ignore */
    memcpy(p, pace_output.previous_car,
            pace_output.previous_car_length);
    p += pace_output.previous_car_length;


    if (pace_output.id_icc_length > 0xffff) {
        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "ID ICC %u bytes too long",
                pace_output.id_icc_length-0xffff);
        sc_result = SC_ERROR_INVALID_DATA;
        goto err;
    }
    word = __cpu_to_le16(pace_output.id_icc_length);
    /* Flawfinder: ignore */
    memcpy(p, &word, sizeof word);
    p += sizeof word;

    /* Flawfinder: ignore */
    memcpy(p, pace_output.id_icc,
            pace_output.id_icc_length);


    *abDataOutLen =
            4 +                                     /* Result */
            2 +                                     /* length Output data */
            2 +                                     /* Statusbytes */
            2+pace_output.ef_cardaccess_length +    /* EF.CardAccess */
            1+pace_output.recent_car_length +       /* Most recent CAR */
            1+pace_output.previous_car_length +     /* Previous CAR */
            2+pace_output.id_icc_length ;           /* identifier of the MRTD chip */

    sc_result = SC_SUCCESS;

err:
    free(pace_output.ef_cardaccess);
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
    u8 *BitMap;

    if (!abDataOut || !abDataOutLen)
        return SC_ERROR_INVALID_ARGUMENTS;

    BitMap = realloc(*abDataOut, sizeof *BitMap);
    if (!BitMap)
        return SC_ERROR_OUT_OF_MEMORY;
    *abDataOut = BitMap;

    sc_result = get_pace_capabilities(BitMap);
    if (sc_result < 0)
        return sc_result;

    *abDataOutLen = sizeof *BitMap;

    return SC_SUCCESS;
}

static int
perform_PC_to_RDR_Secure(const __u8 *in, size_t inlen, __u8** out, size_t *outlen)
{
    int sc_result;
	u8 curr_pin_data[0xff], new_pin_data[0xff], apdu_data[0xf];
    struct sc_pin_cmd_pin curr_pin, new_pin;
    sc_apdu_t apdu;
    const PC_to_RDR_Secure_t *request = (PC_to_RDR_Secure_t *) in;
    const __u8* abData = in + sizeof *request;
    size_t abDatalen = inlen - sizeof *request;
    u8 *abDataOut = NULL;
    size_t abDataOutLen = 0;

    memset(&curr_pin, 0, sizeof(curr_pin));
    memset(&new_pin, 0, sizeof(new_pin));
    memset(curr_pin_data, 0, sizeof curr_pin_data);
    memset(new_pin_data, 0, sizeof new_pin_data);
	curr_pin.data = curr_pin_data;
	curr_pin.len = sizeof curr_pin_data;
	new_pin.data = new_pin_data;
	new_pin.len = sizeof new_pin_data;


    if (!in || !out || !outlen)
        return SC_ERROR_INVALID_ARGUMENTS;

    if (inlen < sizeof *request + 1)
        SC_FUNC_RETURN(ctx, SC_LOG_DEBUG_VERBOSE, SC_ERROR_INVALID_DATA);

    if (request->bMessageType != 0x69
            || request->bSlot != 0)
        sc_debug(ctx, SC_LOG_DEBUG_NORMAL,  "warning: malformed PC_to_RDR_Secure");

    if (request->wLevelParameter != CCID_WLEVEL_DIRECT) {
        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Received request with unsupported wLevelParameter (0x%04x)",
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
                sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Not enough data for abPINDataStucture_Verification_t");
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

			/* bTeoPrologue adds another 3 bytes */
            if (abDatalen < sizeof *modify + 3*(sizeof(__u8))) {
                sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Not enough data for abPINDataStucture_Modification_t");
                sc_result = SC_ERROR_INVALID_DATA;
                goto err;
            }

            wPINMaxExtraDigit = modify->wPINMaxExtraDigit;
            bmPINLengthFormat = modify->bmPINLengthFormat;
            bmPINBlockString = modify->bmPINBlockString;
            bmFormatString = modify->bmFormatString;
            bNumberMessage = modify->bNumberMessage;
			/* bTeoPrologue adds another 3 bytes */
            abPINApdu = (__u8*) modify + sizeof *modify + 3*(sizeof(__u8));
            apdulen = __le32_to_cpu(request->dwLength) - sizeof *modify - 4*sizeof(__u8);
            switch (bNumberMessage) {
                case 0x03:
                    /* bMsgIndex3 is present */
                    abPINApdu++;
                    apdulen--;
                    /* fall through */
                case 0x02:
                    /* bMsgIndex2 is present */
                    abPINApdu++;
                    apdulen--;
                    break;
            }
            break;
        case 0x10:

            sc_result = perform_PC_to_RDR_Secure_GetReadersPACECapabilities(
                    &abDataOut, &abDataOutLen);

            if (card)
                bin_log(card->ctx, SC_LOG_DEBUG_VERBOSE, "PACE Capabilities", abDataOut, abDataOutLen);

            goto err;
            break;
        case 0x20:

            sc_result = perform_PC_to_RDR_Secure_EstablishPACEChannel(card,
                    abData+1, abDatalen-1, &abDataOut, &abDataOutLen);
            goto err;
            break;
        case 0x04:
            // Cancel PIN function
        default:
            sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Received request with unsupported PIN operation (bPINOperation=0x%02x)",
                    *abData);
            sc_result = SC_ERROR_NOT_SUPPORTED;
            goto err;
    }

	if (inlen - (abData - abPINApdu) < apdulen) {
        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Not enough Data for APDU");
		sc_result = SC_ERROR_INVALID_DATA;
		goto err;
	}
    sc_result = sc_bytes2apdu(ctx, abPINApdu, apdulen, &apdu);
    if (sc_result < 0) {
        bin_log(ctx, SC_LOG_DEBUG_VERBOSE, "Invalid APDU", abPINApdu, apdulen);
        goto err;
    }

    new_pin.min_length = curr_pin.min_length = wPINMaxExtraDigit >> 8;
    new_pin.max_length = curr_pin.max_length = wPINMaxExtraDigit & 0x00ff;
	if (new_pin.min_length > new_pin.max_length) {
		/* If maximum length is smaller than minimum length, suppose minimum
		 * length defines the exact length of the pin. */
		curr_pin.max_length = curr_pin.min_length;
		new_pin.max_length = new_pin.min_length;
	}
    uint8_t system_units = bmFormatString & CCID_PIN_UNITS_BYTES;
    uint8_t pin_offset = (bmFormatString >> 3) & 0xf;
    uint8_t length_offset = bmPINLengthFormat & 0xf;
    uint8_t length_size = bmPINBlockString >> 4;
    uint8_t justify_right = bmFormatString & CCID_PIN_JUSTIFY_RIGHT;
    uint8_t encoding = bmFormatString & 2;
    uint8_t blocksize = bmPINBlockString & 0xf;

    sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "PIN %s block (%d bytes) proberties:\n"
            "\tminimum %d, maximum %d PIN digits\n"
            "\t%s PIN encoding, %s justification\n"
            "\tsystem units are %s\n"
            "\twrite PIN length on %d bits with %d system units offset\n"
            "\tcurrent PIN offset is %d %s\n",
            modify ? "modification" : "verification",
            blocksize, modify ? (unsigned int) new_pin.min_length :
            (unsigned int) curr_pin.min_length, modify ?
            (unsigned int) new_pin.max_length :
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
            sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Could not read PIN.\n");
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
                sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Could not read current PIN.\n");
                goto err;
            }
        }

        if (0 > EVP_read_pw_string_min((char *) new_pin.data,
                    new_pin.min_length, new_pin.max_length,
                    "Please enter your new PIN for modification: ",
                    modify->bConfirmPIN & CCID_PIN_CONFIRM_NEW)) {
            sc_result = SC_ERROR_INTERNAL;
            sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Could not read new PIN.\n");
            goto err;
        }
    }

    /* set length of PIN */
    curr_pin.len = strnlen((char *) curr_pin.data, curr_pin.max_length);
    if (modify) {
        new_pin.len = strnlen((char *) new_pin.data, new_pin.max_length);
    }

    if (!apdu.datalen || !apdu.data) {
        /* Host did not provide any data in the APDU, so we have to assign
         * something for the pin block */
        if (verify) {
            apdu.lc = get_datasize_for_pin(&curr_pin, length_size);
            blocksize = apdu.lc;
        } else {
            apdu.lc = get_datasize_for_pin(&new_pin, length_size);
            blocksize = apdu.lc;
            if (modify->bConfirmPIN & CCID_PIN_INSERT_OLD) {
                if (get_datasize_for_pin(&curr_pin, length_size) > apdu.lc)
                    apdu.lc = get_datasize_for_pin(&curr_pin, length_size);
            }
        }

        if (apdu.lc > sizeof apdu_data) {
            sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Not enough memory for PIN block.\n");
            sc_result = SC_ERROR_INTERNAL;
            goto err;
        }

        apdu.data = apdu_data;
        apdu.datalen = apdu.lc;

        switch (apdu.cse) {
            case SC_APDU_CASE_1:
                apdu.cse = SC_APDU_CASE_3_SHORT;
                break;
            case SC_APDU_CASE_2_SHORT:
                apdu.cse = SC_APDU_CASE_4_SHORT;
                break;
            case SC_APDU_CASE_2_EXT:
                apdu.cse = SC_APDU_CASE_4_EXT;
                break;
            default:
                /* Don't change APDU Case */
                break;
        }
    }

    /* Note: pin.offset and pin.length_offset are relative to the first
     * databyte */
    if (verify || (modify->bConfirmPIN & CCID_PIN_INSERT_OLD)) {
        if (!get_effective_offset(system_units, pin_offset, &curr_pin.offset,
                    &sc_result)
                || !write_pin(&apdu, &curr_pin, blocksize, justify_right, encoding,
                    &sc_result)
                || !get_effective_offset(system_units, length_offset,
                    &curr_pin.length_offset, &sc_result)
                || !write_pin_length(&apdu, &curr_pin, system_units, length_size,
                    &sc_result)) {
            sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Writing current PIN to PIN block failed.\n");
            goto err;
        }
    }
    if (modify) {
        if (modify->bConfirmPIN & CCID_PIN_INSERT_OLD
                && modify->bInsertionOffsetOld != curr_pin.offset) {
            sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Inconsistent PIN block proberties.\n");
            sc_result = SC_ERROR_INVALID_DATA;
            goto err;
        }
        new_pin.offset = modify->bInsertionOffsetNew;
        if (!write_pin(&apdu, &new_pin, blocksize, justify_right, encoding,
                    &sc_result))
            goto err;
    }

    sc_result = get_rapdu(&apdu, &abDataOut, &abDataOutLen);

    /* clear PINs that have been transmitted */
    sc_mem_clear((u8 *) apdu.data, apdu.datalen);

err:
    sc_mem_clear((u8 *) curr_pin.data, curr_pin.len);
    sc_mem_clear((u8 *) new_pin.data, new_pin.len);

    sc_result = get_RDR_to_PC_DataBlock(request->bSeq, sc_result, out,
            outlen, abDataOut, abDataOutLen);

    free(abDataOut);

    return sc_result;
}

/* XXX calling sc_wait_for_event blocks all other threads, thats why it
 * can't be used here... */
static int
get_RDR_to_PC_NotifySlotChange(RDR_to_PC_NotifySlotChange_t **out)
{
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

    sc_result = detect_card_presence();
    if (sc_result < 0) {
        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Could not detect card presence.");
        debug_sc_result(sc_result);
    }

    if (sc_result & SC_READER_CARD_PRESENT)
        oldmask |= present[0];
    if (sc_result & SC_READER_CARD_CHANGED) {
        sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "Card status changed.");
        result->bmSlotICCState |= changed[0];
    }

    sleep(10);

    sc_result = detect_card_presence();
    if (sc_result < 0) {
        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Could not detect card presence.");
        debug_sc_result(sc_result);
    }

    if (sc_result & SC_READER_CARD_PRESENT)
        result->bmSlotICCState |= present[0];
    if (sc_result & SC_READER_CARD_CHANGED) {
        sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "Card status changed.");
        result->bmSlotICCState |= changed[0];
    }

    if ((oldmask & present[0]) != (result->bmSlotICCState & present[0])) {
        sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "Card status changed.");
        result->bmSlotICCState |= changed[0];
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
        SC_FUNC_RETURN(ctx, SC_LOG_DEBUG_VERBOSE, SC_ERROR_INVALID_DATA);

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
            sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "Unknown message type in request (0x%02x). "
                    "Using bMessageType=0x%02x for output.",
                    request->bMessageType, 0);
            result->bMessageType = 0;
    }
    result->dwLength     = __constant_cpu_to_le32(0);
    result->bSlot        = 0,
    result->bSeq         = request->bSeq;
    result->bStatus      = get_bStatus(SC_ERROR_UNKNOWN_DATA_RECEIVED);
    result->bError       = 0;
    result->bClockStatus = 0;

    *outlen = sizeof *result;

    return SC_SUCCESS;
}

int ccid_parse_bulkout(const __u8* inbuf, size_t inlen, __u8** outbuf)
{
    int sc_result;
    size_t outlen;

    if (!inbuf)
        return 0;

	bin_log(ctx, SC_LOG_DEBUG_VERBOSE, "CCID input", inbuf, inlen);

    switch (*inbuf) {
        case 0x62: 
                sc_debug(ctx, SC_LOG_DEBUG_NORMAL,  "PC_to_RDR_IccPowerOn");
                sc_result = perform_PC_to_RDR_IccPowerOn(inbuf, inlen, outbuf, &outlen);
                break;

        case 0x63:
                sc_debug(ctx, SC_LOG_DEBUG_NORMAL,  "PC_to_RDR_IccPowerOff");
                sc_result = perform_PC_to_RDR_IccPowerOff(inbuf, inlen, outbuf, &outlen);
                break;

        case 0x65:
                sc_debug(ctx, SC_LOG_DEBUG_NORMAL,  "PC_to_RDR_GetSlotStatus");
                sc_result = perform_PC_to_RDR_GetSlotStatus(inbuf, inlen, outbuf, &outlen);
                break;

        case 0x6F:
                sc_debug(ctx, SC_LOG_DEBUG_NORMAL,  "PC_to_RDR_XfrBlock");
                sc_result = perform_PC_to_RDR_XfrBlock(inbuf, inlen, outbuf, &outlen);
                break;

        case 0x6C:
                sc_debug(ctx, SC_LOG_DEBUG_NORMAL,  "PC_to_RDR_GetParameters");
                sc_result = perform_PC_to_RDR_GetParamters(inbuf, inlen, outbuf, &outlen);
                break;

        case 0x69:
                sc_debug(ctx, SC_LOG_DEBUG_NORMAL,  "PC_to_RDR_Secure");
                sc_result = perform_PC_to_RDR_Secure(inbuf, inlen, outbuf, &outlen);
                break;

        default:
                sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Unknown ccid bulk-in message. "
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
                sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "ABORT");
                if (length != 0x00) {
                    sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "warning: malformed ABORT");
                }

                r = 0;
                break;

            case CCID_CONTROL_GET_CLOCK_FREQUENCIES:
                sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "GET_CLOCK_FREQUENCIES");
                if (value != 0x00) {
                    sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "warning: malformed GET_CLOCK_FREQUENCIES");
                }

                r = sizeof(__le32);
                tmp = realloc(*outbuf, r);
                if (!tmp) {
                    r = SC_ERROR_OUT_OF_MEMORY;
                    break;
                }
                *outbuf = tmp;
                __le32 clock  = ccid_desc.dwDefaultClock;
                /* Flawfinder: ignore */
                memcpy(*outbuf, &clock,  sizeof (__le32));
                break;

            case CCID_CONTROL_GET_DATA_RATES:
                sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "GET_DATA_RATES");
                if (value != 0x00) {
                    sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "warning: malformed GET_DATA_RATES");
                }

                r = sizeof (__le32);
                tmp = realloc(*outbuf, r);
                if (tmp == NULL) {
                    r = -1;
                    break;
                }
                *outbuf = tmp;
                __le32 drate  = ccid_desc.dwDataRate;
                /* Flawfinder: ignore */
                memcpy(*outbuf, &drate,  sizeof (__le32));
                break;

            default:
                sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "Unknown ccid control command.");

                r = SC_ERROR_NOT_SUPPORTED;
        }
    } else {
        r = SC_ERROR_INVALID_ARGUMENTS;
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
