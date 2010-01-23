#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <asm/byteorder.h>
#include <opensc/opensc.h>
#include <opensc/ui.h>

#include "ccid.h"


//static const char *app_name = "ccid";
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


int ccid_initialize(int reader_id, int verbose)
{
    int sc_result;
    int i;

    sc_result = sc_context_create(&ctx, NULL);
    if (sc_result < 0) {
        fprintf(stderr, "Failed to establish context: %s\n", sc_strerror(sc_result));
        return 0;
    }
    if (verbose > 1)
        ctx->debug = verbose-1;

    for (i = 0; i < sizeof card_in_slot; i++) {
        card_in_slot[i] = NULL;
    }

    if (sc_ctx_get_reader_count(ctx) == 0) {
        fprintf(stderr,
                "No smart card readers found.\n");
        return 0;
    }
    if (reader_id < 0) {
        /* Automatically try to skip to a reader with a card if reader not specified */
        for (i = 0; i < sc_ctx_get_reader_count(ctx); i++) {
            reader = sc_ctx_get_reader(ctx, i);
            if (sc_detect_card_presence(reader, 0) & SC_SLOT_CARD_PRESENT) {
                reader_id = i;
                fprintf(stderr, "Using reader with a card: %s\n", reader->name);
                goto autofound;
            }
        }
        reader_id = 0;
    }
autofound:
    if ((unsigned int)reader_id >= sc_ctx_get_reader_count(ctx)) {
        fprintf(stderr,
                "Illegal reader number. "
                "Only %d reader(s) configured.\n",
                sc_ctx_get_reader_count(ctx));
        return 0;
    }

    reader = sc_ctx_get_reader(ctx, reader_id);
    ccid_desc.bMaxSlotIndex = reader->slot_count - 1;

    return 1;
}


int ccid_shutdown()
{
    int i;
    for (i = 0; i < sizeof card_in_slot; i++) {
        if (card_in_slot[i]) {
            sc_unlock(card_in_slot[i]);
            sc_disconnect_card(card_in_slot[i], 0);
        }
    }
    if (ctx)
        sc_release_context(ctx);

    return 1;
}

int build_apdu(const __u8 *buf, size_t len, sc_apdu_t *apdu)
{
	const __u8 *p;
	size_t len0;
        char dbg[40];

	len0 = len;
	if (len < 4) {
		puts("APDU too short (must be at least 4 bytes)");
		return 0;
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
			printf("APDU too short (need %lu bytes)\n",
				(unsigned long) apdu->lc - len);
			return 0;
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
			printf("APDU too long (%lu bytes extra)\n",
				(unsigned long) len);
			return 0;
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

        snprintf(dbg, sizeof dbg, "APDU, %d byte(s):\tins=%02x p1=%02x p2=%02x",
                (unsigned int) len, apdu->ins, apdu->p1, apdu->p2);
        sc_ui_display_debug(ctx, dbg);

        return 1;
}

int get_rapdu(sc_apdu_t *apdu, size_t slot, __u8 **buf, size_t *resplen, int *sc_result)
{
    char dbg[40];

    if (!apdu || !buf || !resplen || slot > sizeof card_in_slot || !sc_result) {
        if (sc_result)
            *sc_result = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }

    apdu->resplen = apdu->le;
    /* Get two more bytes to later use as return buffer including sw1 and sw2 */
    apdu->resp = malloc(apdu->resplen + sizeof(__u8) + sizeof(__u8));
    if (!apdu->resp) {
        *sc_result = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }

    *sc_result = sc_transmit_apdu(card_in_slot[slot], apdu);
    if (*sc_result < 0) {
        goto err;
    }

    if (apdu->sw1 > 0xff || apdu->sw2 > 0xff) {
        *sc_result = SC_ERROR_INVALID_DATA;
        goto err;
    }

    apdu->resp[apdu->resplen] = apdu->sw1;
    apdu->resp[apdu->resplen + sizeof(__u8)] = apdu->sw2;

    *buf = apdu->resp;
    *resplen = apdu->resplen + sizeof(__u8) + sizeof(__u8);

    snprintf(dbg, sizeof dbg, "R-APDU, %d byte(s):\tsw1=%02x sw2=%02x",
            (unsigned int) *resplen, apdu->sw1, apdu->sw2);
    sc_ui_display_debug(ctx, dbg);

    return 1;

err:
    if (apdu->resp)
        free(apdu->resp);

    return 0;
}

__u8 get_bError(int sc_result)
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

__u8 get_bStatus(int sc_result, __u8 bSlot)
{
    __u8 result;
    if (sc_result < 0) {
        if (bSlot < sizeof card_in_slot
                && card_in_slot[bSlot]
                && sc_card_valid(card_in_slot[bSlot])) {
            result = CCID_BSTATUS_ERROR_ACTIVE;
        } else {
            if (bSlot < reader->slot_count
                    && sc_detect_card_presence(reader, bSlot) & SC_SLOT_CARD_PRESENT) {
                result = CCID_BSTATUS_ERROR_INACTIVE;
            } else {
                result = CCID_BSTATUS_ERROR_NOICC;
            }
        }
    } else {
        if (bSlot < sizeof card_in_slot
                && card_in_slot[bSlot]
                && sc_card_valid(card_in_slot[bSlot])) {
            result = CCID_BSTATUS_OK_ACTIVE;
        } else {
            if (bSlot < reader->slot_count
                    && sc_detect_card_presence(reader, bSlot) & SC_SLOT_CARD_PRESENT) {
                result = CCID_BSTATUS_OK_INACTIVE;
            } else {
                result = CCID_BSTATUS_OK_NOICC;
            }
        }
    }

    char buf[30];
    switch (result) {
        case CCID_BSTATUS_OK_ACTIVE:
        case CCID_BSTATUS_ERROR_ACTIVE:
            sprintf(buf, "active card in slot %d", bSlot);
            break;
        case CCID_BSTATUS_OK_INACTIVE:
        case CCID_BSTATUS_ERROR_INACTIVE:
            sprintf(buf, "inactive card in slot %d", bSlot);
            break;
        case CCID_BSTATUS_OK_NOICC:
        case CCID_BSTATUS_ERROR_NOICC:
            sprintf(buf, "no card in slot %d", bSlot);
            break;
    }
    sc_ui_display_debug(ctx, buf);

    return result;
}

RDR_to_PC_SlotStatus_t get_RDR_to_PC_SlotStatus(__u8 bSlot, __u8 bSeq,
        int sc_result)
{
    RDR_to_PC_SlotStatus_t result;

    result.bMessageType = 0x81;
    result.dwLength     = __constant_cpu_to_le32(0);
    result.bSlot        = bSlot;
    result.bSeq         = bSeq;
    result.bStatus      = get_bStatus(sc_result, bSlot);
    result.bError       = get_bError(sc_result);
    result.bClockStatus = 0;

    return result;
}

RDR_to_PC_DataBlock_t get_RDR_to_PC_DataBlock(__u8 bSlot, __u8 bSeq,
        int sc_result, __le32 dwLength)
{
    RDR_to_PC_DataBlock_t result;

    result.bMessageType    = 0x80;
    result.dwLength        = dwLength;
    result.bSlot           = bSlot;
    result.bSeq            = bSeq;
    result.bStatus         = get_bStatus(sc_result, bSlot);
    result.bError          = get_bError(sc_result);
    result.bChainParameter = 0;

    return result;
}

RDR_to_PC_SlotStatus_t
perform_PC_to_RDR_GetSlotStatus(const PC_to_RDR_GetSlotStatus_t request)
{
    if (    request.bMessageType != 0x65 ||
            request.dwLength     != __constant_cpu_to_le32(0) ||
            request.abRFU1       != 0 ||
            request.abRFU2       != 0)
        sc_ui_display_debug(ctx, "warning: malformed PC_to_RDR_GetSlotStatus");

    return get_RDR_to_PC_SlotStatus(request.bSlot, request.bSeq, SC_SUCCESS);
}

RDR_to_PC_SlotStatus_t
perform_PC_to_RDR_IccPowerOn(const PC_to_RDR_IccPowerOn_t request, char ** pATR)
{
    if (    request.bMessageType != 0x62 ||
            request.dwLength     != __constant_cpu_to_le32(0) ||
            !( request.bPowerSelect == 0 ||
                request.bPowerSelect & ccid_desc.bVoltageSupport ) ||
            request.abRFU        != 0)
        sc_ui_display_debug(ctx, "warning: malformed PC_to_RDR_IccPowerOn");

    RDR_to_PC_SlotStatus_t result;
    int sc_result;

    if (!pATR)
        goto err;

    *pATR = NULL;
    result.dwLength = __constant_cpu_to_le32(0);

    if (request.bSlot > sizeof card_in_slot)
        goto err;

    sc_result = sc_connect_card(reader, request.bSlot,
            &card_in_slot[request.bSlot]);

    result = get_RDR_to_PC_SlotStatus(request.bSlot, request.bSeq, sc_result);

    if (sc_result < 0)
        goto err;

    *pATR = (char*) card_in_slot[request.bSlot]->atr;
    result.dwLength = __cpu_to_le32(card_in_slot[request.bSlot]->atr_len);

err:
    return result;
}
RDR_to_PC_SlotStatus_t
perform_PC_to_RDR_IccPowerOff(const PC_to_RDR_IccPowerOff_t request)
{
    if (    request.bMessageType != 0x63 ||
            request.dwLength     != __constant_cpu_to_le32(0) ||
            request.abRFU1       != 0 ||
            request.abRFU2       != 0)
        sc_ui_display_debug(ctx, "warning: malformed PC_to_RDR_IccPowerOff");

    if (request.bSlot > sizeof card_in_slot)
        return get_RDR_to_PC_SlotStatus(request.bSlot, request.bSeq,
                SC_ERROR_INVALID_DATA);

    return get_RDR_to_PC_SlotStatus(request.bSlot, request.bSeq,
            sc_disconnect_card(card_in_slot[request.bSlot], 0));
}

RDR_to_PC_DataBlock_t
perform_PC_to_RDR_XfrBlock(const PC_to_RDR_XfrBlock_t request, const __u8*
        abDataIn, __u8** abDataOut)
{
    int sc_result;
    size_t resplen = 0;
    sc_apdu_t apdu;

    if (    request.bMessageType != 0x6F ||
            request.bBWI  != 0)
        sc_ui_display_error(ctx, "malformed PC_to_RDR_XfrBlock, will continue anyway");

    if (request.bSlot > sizeof card_in_slot)
        goto err;

    if (!build_apdu(abDataIn, request.dwLength, &apdu)) {
        sc_result = SC_ERROR_INVALID_DATA;
        goto err;
    }

    get_rapdu(&apdu, request.bSlot, abDataOut, &resplen, &sc_result);

err:
    if (sc_result < 0)
        sc_ui_display_error(ctx, sc_strerror(sc_result));

    return get_RDR_to_PC_DataBlock(request.bSlot, request.bSeq,
            sc_result, __cpu_to_le32(resplen));
}


RDR_to_PC_Parameters_t
get_RDR_to_PC_Parameters(__u8 bSlot, __u8 bSeq, int sc_result, __u8
        **abProtocolDataStructure)
{
    RDR_to_PC_Parameters_t result;

    result.bMessageType = 0x82;
    result.bSlot = bSlot;
    result.bSeq = bSeq;
    if (sc_result < 0) {
        result.dwLength = __constant_cpu_to_le32(0);
        *abProtocolDataStructure = NULL;
    } else {
        switch (reader->slot[bSlot].active_protocol) {
            case SC_PROTO_T0:
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
                    t0->bmFindexDindex    =
                        1<<4|   // index to table 7 ISO 7816-3 (Fi)
                        1;      // index to table 8 ISO 7816-3 (Di)
                    t0->bmTCCKST0         = 0<<1;   // convention (direct)
                    t0->bGuardTimeT0      = 0xFF;
                    t0->bWaitingIntegerT0 = 0x10;
                    t0->bClockStop        = 0;      // (not allowed)
                } else {
                    // error malloc
                    result.dwLength = __constant_cpu_to_le32(0);
                    *abProtocolDataStructure = NULL;
                    sc_result = SC_ERROR_OUT_OF_MEMORY;
                }
                break;
            case SC_PROTO_T1:
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
                } else {
                    // error malloc
                    result.dwLength = __constant_cpu_to_le32(0);
                    *abProtocolDataStructure = NULL;
                    sc_result = SC_ERROR_OUT_OF_MEMORY;
                }
                break;
            default:
                fprintf (stderr, "unknown protocol\n");
                result.dwLength = __constant_cpu_to_le32(0);
                *abProtocolDataStructure = NULL;
        }
    }
    result.bStatus = get_bStatus(sc_result, bSlot);
    result.bError  = get_bError(sc_result);

    return result;
}

RDR_to_PC_Parameters_t
perform_PC_to_RDR_GetParamters(const PC_to_RDR_GetParameters_t request,
        __u8** abProtocolDataStructure)
{
    if (    request.bMessageType != 0x6C ||
            request.dwLength != __constant_cpu_to_le32(0))
        sc_ui_display_debug(ctx, "warning: malformed PC_to_RDR_GetParamters");

    return get_RDR_to_PC_Parameters(request.bSlot, request.bSeq,
            SC_SUCCESS, abProtocolDataStructure);
}

int
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

int
write_pin_length(sc_apdu_t *apdu, const struct sc_pin_cmd_pin *pin,
        uint8_t system_units, uint8_t length_size, int *sc_result)
{
    u8 *p;

    if (!apdu || !apdu->data || !pin || apdu->datalen <= pin->length_offset || !sc_result) {
        if (sc_result)
            *sc_result = SC_ERROR_INVALID_ARGUMENTS;
        return 0;
    }

    if (length_size) {
        if (length_size != 8) {
            *sc_result = SC_ERROR_NOT_SUPPORTED;
            return 0;
        }
        p = (u8 *) apdu->data;
        p[pin->length_offset] = pin->len;
    }

    *sc_result = SC_SUCCESS;

    return 1;
}

int
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

int
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

RDR_to_PC_DataBlock_t
perform_PC_to_RDR_Secure(const PC_to_RDR_Secure_t request,
        const __u8* abData, __u8** abDataOut)
{
    char dbg[256];
    int sc_result = SC_SUCCESS;
    size_t resplen = 0;
    sc_apdu_t apdu;
    struct sc_pin_cmd_pin curr_pin, new_pin;
    sc_ui_hints_t hints;

    memset(&hints, 0, sizeof(hints));
    memset(&curr_pin, 0, sizeof(curr_pin));
    memset(&new_pin, 0, sizeof(new_pin));

    if (request.bMessageType != 0x69)
        sc_ui_display_debug(ctx, "warning: malformed PC_to_RDR_Secure");

    if (request.bSlot > sizeof card_in_slot)
        goto err;

    if (request.wLevelParameter != CCID_WLEVEL_DIRECT) {
        sc_result = SC_ERROR_NOT_SUPPORTED;
        goto err;
    }

    __u8 bmPINLengthFormat, bmPINBlockString, bmFormatString;
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
            abPINApdu = (__u8*) verify + sizeof(*verify);
            apdulen = __le32_to_cpu(request.dwLength) - sizeof(*verify) - sizeof(__u8);
            break;
        case 0x01:
            // PIN Modification
            modify = (abPINDataStucture_Modification_t *)
                (abData + sizeof(__u8));
            wPINMaxExtraDigit = modify->wPINMaxExtraDigit;
            bmPINLengthFormat = modify->bmPINLengthFormat;
            bmPINBlockString = modify->bmPINBlockString;
            bmFormatString = modify->bmFormatString;
            abPINApdu = (__u8*) modify + sizeof(*modify);
            apdulen = __le32_to_cpu(request.dwLength) - sizeof(*modify) - sizeof(__u8);
            break;
        case 0x04:
            // Cancel PIN function
        default:
            sc_result = SC_ERROR_NOT_SUPPORTED;
            goto err;
    }

    if (!build_apdu(abPINApdu, apdulen, &apdu)) {
        sc_result = SC_ERROR_INVALID_DATA;
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

    snprintf(dbg, sizeof dbg,
            "PIN %s block (%d bytes) proberties:\n"
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
    sc_ui_display_debug(ctx, dbg);

    /* get the PIN */
    hints.dialog_name = "ccid.PC_to_RDR_Secure";
    hints.card = card_in_slot[request.bSlot];
    if (verify) {
        hints.prompt = "PIN Verification";
        hints.usage = SC_UI_USAGE_OTHER;
        sc_result = sc_ui_get_pin(&hints, (char **) &curr_pin.data);
    } else {
        hints.prompt = "PIN Modification";
        hints.usage = SC_UI_USAGE_CHANGE_PIN;
        sc_result = sc_ui_get_pin_pair(&hints, (char **) &curr_pin.data,
                (char **) &new_pin.data);
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
        curr_pin.offset = modify->bInsertionOffsetOld;
        new_pin.offset = modify->bInsertionOffsetNew;
        if (!write_pin(&apdu, &new_pin, blocksize, justify_right, encoding,
                    &sc_result))
            goto err;
    }
    if (!get_effective_offset(system_units, length_offset,
                &curr_pin.length_offset, &sc_result)
            || !write_pin_length(&apdu, &curr_pin, system_units, length_size,
                &sc_result)
            || !write_pin(&apdu, &curr_pin, blocksize, justify_right, encoding,
                &sc_result)
            || !get_rapdu(&apdu, request.bSlot, abDataOut, &resplen,
                &sc_result))
        goto err;

err:
    if (sc_result < 0)
        sc_ui_display_error(ctx, sc_strerror(sc_result));

    if (curr_pin.data) {
        sc_mem_clear((u8 *) curr_pin.data, curr_pin.len);
        free((u8 *) curr_pin.data);
    }
    if (new_pin.data) {
        sc_mem_clear((u8 *) new_pin.data, new_pin.len);
        free((u8 *) new_pin.data);
    }
    sc_mem_clear(abPINApdu, apdulen);

    return get_RDR_to_PC_DataBlock(request.bSlot, request.bSeq, sc_result,
            __constant_cpu_to_le32(resplen));
}

RDR_to_PC_NotifySlotChange_t
get_RDR_to_PC_NotifySlotChange ()
{
    RDR_to_PC_NotifySlotChange_t result;
    result.bMessageType = 0x50;
    result.bmSlotICCState = CCID_SLOTS_UNCHANGED;

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
    for (i = 0; i < reader->slot_count; i++) {
        sc_result = sc_detect_card_presence(reader, i);
        if (sc_result < 0) {
            fprintf (stderr, "error getting slot state\n");
            continue;
        }

        if (sc_result & SC_SLOT_CARD_PRESENT)
            result.bmSlotICCState |= present[i];
        if (sc_result & SC_SLOT_CARD_CHANGED)
            result.bmSlotICCState |= changed[i];
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
            sc_ui_display_debug(ctx, "unknown message type");
            result.bMessageType = 0;
    }
    result.dwLength     = __constant_cpu_to_le32(0);
    result.bSlot        = request.bSlot,
    result.bSeq         = request.bSeq;
    result.bStatus      = get_bStatus(SC_ERROR_UNKNOWN_DATA_RECEIVED, request.bSlot);
    result.bError       = 0;
    result.bClockStatus = 0;

    return result;
}

int ccid_parse_bulkin(const __u8* inbuf, __u8** outbuf)
{
    if (inbuf == NULL)
        return 0;
    int result = -1;
    /*if (SCardIsValidContext(hcontext) != SCARD_S_SUCCESS) {*/
        /*if (ccid_initialize(reader_num) == NULL)*/
            /*goto error;*/
    /*}*/

    switch (*inbuf) {
        case 0x62: 
            {   sc_ui_display_debug(ctx, "PC_to_RDR_IccPowerOn");

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
            {   sc_ui_display_debug(ctx, "PC_to_RDR_IccPowerOff");

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
            {   sc_ui_display_debug(ctx, "PC_to_RDR_GetSlotStatus");

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
            {   sc_ui_display_debug(ctx, "PC_to_RDR_XfrBlock");

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
            {   sc_ui_display_debug(ctx, "PC_to_RDR_GetParameters");

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
            {   sc_ui_display_debug(ctx, "PC_to_RDR_Secure");

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

int ccid_parse_control(struct usb_ctrlrequest *setup, __u8 **outbuf)
{
    int result = -1;
    __u16 value, index, length;

    value = __le16_to_cpu(setup->wValue);
    index = __le16_to_cpu(setup->wIndex);
    length = __le16_to_cpu(setup->wLength);
    if (setup->bRequestType == USB_REQ_CCID) {
        switch(setup->bRequest) {
            case CCID_CONTROL_ABORT:
                {
                    sc_ui_display_debug(ctx, "ABORT");
                    if (length != 0x00) {
                        sc_ui_display_debug(ctx, "warning: malformed ABORT");
                    }
                    result = 0;
                }   break;
            case CCID_CONTROL_GET_CLOCK_FREQUENCIES:
                {
                    sc_ui_display_debug(ctx, "GET_CLOCK_FREQUENCIES");
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
                    sc_ui_display_debug(ctx, "GET_DATA_RATES");
                    if (value != 0x00) {
                        sc_ui_display_debug(ctx, "warning: malformed GET_DATA_RATES");
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
    }

    return result;
}

int ccid_state_changed(RDR_to_PC_NotifySlotChange_t *slotchange)
{
    if (slotchange) {
        *slotchange = get_RDR_to_PC_NotifySlotChange();

        if (slotchange->bmSlotICCState)
            return 1;
    }

    return 0;
}
