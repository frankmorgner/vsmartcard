/*
 * boxing.c: implementation related to boxing commands with pseudo APDUs
 *
 * Copyright (C) 2013  Frank Morgner  <morgner@informatik.hu-berlin.de>
 *
 * This file is part of libnpa.
 *
 * libnpa is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * libnpa is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * libnpa.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopensc/asn1.h>
#include <libopensc/log.h>
#include <libopensc/opensc.h>
#include <libopensc/pace.h>
#include <stdlib.h>
#include <string.h>

static const u8 boxing_cla                          = 0xff;
static const u8 boxing_ins                          = 0x9a;
static const u8 boxing_p1                           = 0x04;
static const u8 boxing_p2_GetReaderPACECapabilities = 0x01;
static const u8 boxing_p2_EstablishPACEChannel      = 0x02;
static const u8 boxing_p2_DestroyPACEChannel        = 0x03;
static const u8 boxing_p2_PC_to_RDR_Secure          = 0x10;

static const struct sc_asn1_entry g_EstablishPACEChannelInput_data[] = {
    { "passwordID",
        /* use an OCTET STRING to avoid a conversion to int */
        SC_ASN1_OCTET_STRING, SC_ASN1_CTX|0x01, 0, NULL, NULL },
    { "transmittedPassword",
        SC_ASN1_OCTET_STRING, SC_ASN1_CTX|0x02, SC_ASN1_OPTIONAL|SC_ASN1_ALLOC, NULL, NULL },
    { "cHAT",
        SC_ASN1_OCTET_STRING, SC_ASN1_CTX|0x03, SC_ASN1_OPTIONAL|SC_ASN1_ALLOC, NULL, NULL },
    { "certificateDescription",
        SC_ASN1_OCTET_STRING, SC_ASN1_CTX|0x04, SC_ASN1_OPTIONAL|SC_ASN1_ALLOC, NULL, NULL },
    { "hashOID",
        /* use an OCTET STRING to avoid a conversion to struct sc_object_id */
        SC_ASN1_OCTET_STRING, SC_ASN1_CTX|0x05, SC_ASN1_OPTIONAL|SC_ASN1_ALLOC, NULL, NULL },
    { NULL , 0 , 0 , 0 , NULL , NULL }
};
static const struct sc_asn1_entry g_EstablishPACEChannelOutput_data[] = {
    { "errorCode",
        SC_ASN1_OCTET_STRING, SC_ASN1_CTX|0x01, 0, NULL, NULL },
    { "statusMSESetAT",
        SC_ASN1_OCTET_STRING, SC_ASN1_CTX|0x02, 0, NULL, NULL },
    { "efCardAccess",
        SC_ASN1_OCTET_STRING, SC_ASN1_CTX|0x03, SC_ASN1_ALLOC, NULL, NULL },
    { "idPICC",
        SC_ASN1_OCTET_STRING, SC_ASN1_CTX|0x04, SC_ASN1_OPTIONAL|SC_ASN1_ALLOC, NULL, NULL },
    { "curCAR",
        SC_ASN1_OCTET_STRING, SC_ASN1_CTX|0x05, SC_ASN1_OPTIONAL|SC_ASN1_ALLOC, NULL, NULL },
    { "prevCAR",
        SC_ASN1_OCTET_STRING, SC_ASN1_CTX|0x06, SC_ASN1_OPTIONAL|SC_ASN1_ALLOC, NULL, NULL },
    { NULL , 0 , 0 , 0 , NULL , NULL }
};
static const struct sc_asn1_entry g_EstablishPACEChannel[] = {
    { "EstablishPACEChannel",
        SC_ASN1_STRUCT, SC_ASN1_TAG_SEQUENCE|SC_ASN1_CONS, 0, NULL, NULL },
    { NULL , 0 , 0 , 0 , NULL , NULL }
};

int boxing_pace_input_to_buf(sc_context_t *ctx,
        const struct establish_pace_channel_input *input,
        unsigned char **asn1, size_t *asn1_len)
{
    const size_t count_data =
        sizeof g_EstablishPACEChannelInput_data/
        sizeof *g_EstablishPACEChannelInput_data;
    const size_t count_sequence =
        sizeof g_EstablishPACEChannel/
        sizeof *g_EstablishPACEChannel;
    size_t pin_id_len = sizeof input->pin_id, i;
    struct sc_asn1_entry EstablishPACEChannelInput_data[count_data];
    struct sc_asn1_entry EstablishPACEChannel[count_sequence];

    for (i = 0; i < count_sequence; i++)
        sc_copy_asn1_entry(g_EstablishPACEChannel+i,
                EstablishPACEChannel+i);
    sc_format_asn1_entry(EstablishPACEChannel,
            EstablishPACEChannelInput_data, 0, 1);

    for (i = 0; i < count_data; i++)
        sc_copy_asn1_entry(g_EstablishPACEChannelInput_data+i,
                EstablishPACEChannelInput_data+i);
    sc_format_asn1_entry(EstablishPACEChannelInput_data,
            (unsigned char *) &input->pin_id, &pin_id_len, 1);
    if (input->pin)
        sc_format_asn1_entry(EstablishPACEChannelInput_data,
                (unsigned char *) input->pin,
                (size_t *) &input->pin_length, 1);
    if (input->chat)
        sc_format_asn1_entry(EstablishPACEChannelInput_data,
                (unsigned char *) input->chat,
                (size_t *) &input->chat_length, 1);
    if (input->certificate_description)
        sc_format_asn1_entry(EstablishPACEChannelInput_data,
                (unsigned char *) input->certificate_description,
                (size_t *) &input->certificate_description_length, 1);

    return sc_asn1_encode(ctx, EstablishPACEChannel, asn1, asn1_len);
}

int boxing_buf_to_pace_input(sc_context_t *ctx,
        const unsigned char *asn1, size_t asn1_len,
        struct establish_pace_channel_input *input)
{
    const size_t count_data =
        sizeof g_EstablishPACEChannelInput_data/
        sizeof *g_EstablishPACEChannelInput_data;
    const size_t count_sequence =
        sizeof g_EstablishPACEChannel/
        sizeof *g_EstablishPACEChannel;
    size_t pin_id_len = sizeof input->pin_id, i;
    struct sc_asn1_entry EstablishPACEChannelInput_data[count_data];
    struct sc_asn1_entry EstablishPACEChannel[count_sequence];

    for (i = 0; i < count_sequence; i++)
        sc_copy_asn1_entry(g_EstablishPACEChannel+i,
                EstablishPACEChannel+i);
    sc_format_asn1_entry(EstablishPACEChannel,
            EstablishPACEChannelInput_data, 0, 0);

    for (i = 0; i < count_data; i++)
        sc_copy_asn1_entry(g_EstablishPACEChannelInput_data+i,
                EstablishPACEChannelInput_data+i);
    sc_format_asn1_entry(EstablishPACEChannelInput_data+0,
            &input->pin_id, &pin_id_len, 0);
    sc_format_asn1_entry(EstablishPACEChannelInput_data+1,
            (unsigned char *) input->pin, &input->pin_length, 0);
    sc_format_asn1_entry(EstablishPACEChannelInput_data+2,
            (unsigned char *) input->chat, &input->chat_length, 0);
    sc_format_asn1_entry(EstablishPACEChannelInput_data+3,
            (unsigned char *) input->certificate_description,
            &input->certificate_description_length, 0);

    LOG_TEST_RET(ctx,
            sc_asn1_decode(ctx, EstablishPACEChannel, asn1, asn1_len, NULL, NULL),
            "Error decoding EstablishPACEChannel");

    if (pin_id_len != sizeof input->pin_id)
        return SC_ERROR_UNKNOWN_DATA_RECEIVED;

    return SC_SUCCESS;
}

int boxing_pace_output_to_buf(sc_context_t *ctx,
        const struct establish_pace_channel_output *output,
        unsigned char **asn1, size_t *asn1_len)
{
    const size_t count_data =
        sizeof g_EstablishPACEChannelOutput_data/
        sizeof *g_EstablishPACEChannelOutput_data;
    const size_t count_sequence =
        sizeof g_EstablishPACEChannel/
        sizeof *g_EstablishPACEChannel;
    uint16_t status_mse_set_at = ((output->mse_set_at_sw1 & 0xff) << 8) | output->mse_set_at_sw2;
    size_t i, result_len = sizeof output->result,
           status_mse_set_at_len = sizeof status_mse_set_at;
    struct sc_asn1_entry EstablishPACEChannelOutput_data[count_data];
    struct sc_asn1_entry EstablishPACEChannel[count_sequence];

    for (i = 0; i < count_sequence; i++)
        sc_copy_asn1_entry(g_EstablishPACEChannel+i,
                EstablishPACEChannel+i);
    sc_format_asn1_entry(EstablishPACEChannel,
            EstablishPACEChannelOutput_data, 0, 1);

    for (i = 0; i < count_data; i++)
        sc_copy_asn1_entry(g_EstablishPACEChannelOutput_data+i,
                EstablishPACEChannelOutput_data+i);
    sc_format_asn1_entry(EstablishPACEChannelOutput_data+0,
            (unsigned char *) &output->result, &result_len, 1);
    sc_format_asn1_entry(EstablishPACEChannelOutput_data+1,
            &status_mse_set_at, &status_mse_set_at_len, 1);
    if (output->ef_cardaccess)
        sc_format_asn1_entry(EstablishPACEChannelOutput_data+2, 
                output->ef_cardaccess, (size_t *) &output->ef_cardaccess_length, 1);
    if (output->id_icc)
        sc_format_asn1_entry(EstablishPACEChannelOutput_data+3,
                output->id_icc, (size_t *) &output->id_icc_length, 1);
    if (output->recent_car)
        sc_format_asn1_entry(EstablishPACEChannelOutput_data+4,
                output->recent_car, (size_t *) &output->recent_car_length, 1);
    if (output->previous_car)
        sc_format_asn1_entry(EstablishPACEChannelOutput_data+5,
            output->previous_car, (size_t *) &output->previous_car_length, 1);

    return sc_asn1_encode(ctx, EstablishPACEChannel, asn1, asn1_len);
}

int boxing_buf_to_pace_output(sc_context_t *ctx,
        const unsigned char *asn1, size_t asn1_len,
        struct establish_pace_channel_output *output)
{
    const size_t count_data =
        sizeof g_EstablishPACEChannelOutput_data/
        sizeof *g_EstablishPACEChannelOutput_data;
    const size_t count_sequence =
        sizeof g_EstablishPACEChannel/
        sizeof *g_EstablishPACEChannel;
    uint16_t status_mse_set_at;
    size_t i, result_len = sizeof output->result,
           status_mse_set_at_len = sizeof status_mse_set_at;
    struct sc_asn1_entry EstablishPACEChannelOutput_data[count_data];
    struct sc_asn1_entry EstablishPACEChannel[count_sequence];

    for (i = 0; i < count_sequence; i++)
        sc_copy_asn1_entry(g_EstablishPACEChannel+i,
                EstablishPACEChannel+i);
    sc_format_asn1_entry(EstablishPACEChannel,
            EstablishPACEChannelOutput_data, 0, 0);

    for (i = 0; i < count_data; i++)
        sc_copy_asn1_entry(g_EstablishPACEChannelOutput_data+i,
                EstablishPACEChannelOutput_data+i);
    sc_format_asn1_entry(EstablishPACEChannelOutput_data+0,
            &output->result,       &result_len, 0);
    sc_format_asn1_entry(EstablishPACEChannelOutput_data+1,
            &status_mse_set_at,    &status_mse_set_at_len, 0);
    sc_format_asn1_entry(EstablishPACEChannelOutput_data+2,
            output->ef_cardaccess, &output->ef_cardaccess_length, 0);
    sc_format_asn1_entry(EstablishPACEChannelOutput_data+3,
            output->id_icc,        &output->id_icc_length, 0);
    sc_format_asn1_entry(EstablishPACEChannelOutput_data+4,
            output->recent_car,    &output->recent_car_length, 0);
    sc_format_asn1_entry(EstablishPACEChannelOutput_data+5,
            output->previous_car,  &output->previous_car_length, 0);

    LOG_TEST_RET(ctx,
            sc_asn1_decode(ctx, EstablishPACEChannel,
                asn1, asn1_len, NULL, NULL),
            "Error decoding EstablishPACEChannel");

    if (status_mse_set_at_len != sizeof status_mse_set_at
            || result_len != sizeof output->result)
        return SC_ERROR_UNKNOWN_DATA_RECEIVED;

    output->mse_set_at_sw1 = (status_mse_set_at >> 8) & 0xff;
    output->mse_set_at_sw2 =  status_mse_set_at       & 0xff;

    return SC_SUCCESS;
}

static int boxing_perform_verify(struct sc_reader *reader, struct sc_pin_cmd_data *data)
{
    return SC_ERROR_NOT_SUPPORTED;
}

static int boxing_perform_pace(struct sc_reader *reader,
        void *establish_pace_channel_input,
        void *establish_pace_channel_output)
{
    u8 rbuf[0xffff];
    sc_apdu_t apdu;
    int r;
    struct establish_pace_channel_input  *input  =
        establish_pace_channel_input;
    struct establish_pace_channel_output *output =
        establish_pace_channel_output;

	memset(&apdu, 0, sizeof(apdu));
    apdu.cse     = SC_APDU_CASE_4_EXT;
	apdu.cla     = boxing_cla;
	apdu.ins     = boxing_ins;
	apdu.p1      = boxing_p1;
	apdu.p2      = boxing_p2_EstablishPACEChannel;
    apdu.resp    = rbuf;
    apdu.resplen = sizeof rbuf;
    apdu.le      = sizeof rbuf;

    if (!reader || !reader->ops || !reader->ops->transmit) {
        r = SC_ERROR_NOT_SUPPORTED;
        goto err;
    }

    r = boxing_pace_input_to_buf(reader->ctx, input,
            (unsigned char **) &apdu.data, &apdu.datalen);
    if (r < 0) {
        sc_debug(reader->ctx, SC_LOG_DEBUG_NORMAL,
                "Error encoding EstablishPACEChannel");
        goto err;
    }
    apdu.lc = apdu.datalen;

    r = reader->ops->transmit(reader, &apdu);
    if (r < 0) {
        sc_debug(reader->ctx, SC_LOG_DEBUG_NORMAL,
                "Error performing EstablishPACEChannel");
        goto err;
    }

    if (apdu.sw1 != 0x90 && apdu.sw2 != 0x00) {
        sc_debug(reader->ctx, SC_LOG_DEBUG_NORMAL,
                "Error decoding EstablishPACEChannel");
        r = SC_ERROR_NOT_SUPPORTED;
        goto err;
    }

    r = boxing_buf_to_pace_output(reader->ctx, apdu.resp, apdu.resplen,
            output);

err:
    free((unsigned char *) apdu.data);

    return r;
}

struct sc_asn1_entry g_PACECapabilities_data[] = {
    { "capabilityPACE",
        SC_ASN1_BOOLEAN, SC_ASN1_CTX|0x01, SC_ASN1_ALLOC, NULL, NULL },
    { "capabilityEID",
        SC_ASN1_BOOLEAN, SC_ASN1_CTX|0x02, SC_ASN1_ALLOC, NULL, NULL },
    { "capabilityESign",
        SC_ASN1_BOOLEAN, SC_ASN1_CTX|0x03, SC_ASN1_ALLOC, NULL, NULL },
    { "capabilityDestroy",
        SC_ASN1_BOOLEAN, SC_ASN1_CTX|0x04, SC_ASN1_ALLOC, NULL, NULL },
    { NULL , 0 , 0 , 0 , NULL , NULL }
};
struct sc_asn1_entry g_PACECapabilities[] = {
    { "PACECapabilities",
        SC_ASN1_STRUCT, SC_ASN1_TAG_SEQUENCE|SC_ASN1_CONS, 0, NULL, NULL },
    { NULL , 0 , 0 , 0 , NULL , NULL }
};

int boxing_buf_to_pace_capabilities(sc_context_t *ctx,
        const unsigned char *asn1, size_t asn1_len,
        unsigned long *sc_reader_t_capabilities)
{
    const size_t count_data =
        sizeof g_PACECapabilities_data/
        sizeof *g_PACECapabilities_data;
    const size_t count_sequence =
        sizeof g_PACECapabilities/
        sizeof *g_PACECapabilities;
    int capabilityPACE = 0, capabilityEID = 0, capabilityESign = 0,
        capabilityDestroy = 0;
    size_t i;
    struct sc_asn1_entry PACECapabilities_data[count_data];
    struct sc_asn1_entry PACECapabilities[count_sequence];

    for (i = 0; i < count_sequence; i++)
        sc_copy_asn1_entry(g_PACECapabilities+i,
                PACECapabilities+i);
    sc_format_asn1_entry(PACECapabilities,
            PACECapabilities_data, 0, 1);

    for (i = 0; i < count_data; i++)
        sc_copy_asn1_entry(g_PACECapabilities_data+i,
                PACECapabilities_data+i);
    sc_format_asn1_entry(PACECapabilities_data+0,
            &capabilityPACE, NULL, 0);
    sc_format_asn1_entry(PACECapabilities_data+1,
            &capabilityEID, NULL, 0);
    sc_format_asn1_entry(PACECapabilities_data+2,
            &capabilityESign, NULL, 0);
    sc_format_asn1_entry(PACECapabilities_data+3,
            &capabilityDestroy, NULL, 0);

    LOG_TEST_RET(ctx,
            sc_asn1_decode(ctx, PACECapabilities,
                asn1, asn1_len, NULL, NULL),
            "Error decoding PACECapabilities");

    /* We got a valid PACE Capabilities reply. There is currently no mechanism
     * to determine support PIN verification/modification with a boxing
     * command. Since the reader implements this mechanism it is reasonable to
     * assume that PIN verification/modification is available. */
    *sc_reader_t_capabilities = SC_READER_CAP_PIN_PAD;

    if (capabilityPACE)
        *sc_reader_t_capabilities |= SC_READER_CAP_PACE_GENERIC;
    if (capabilityEID)
        *sc_reader_t_capabilities |= SC_READER_CAP_PACE_EID;
    if (capabilityESign)
        *sc_reader_t_capabilities |= SC_READER_CAP_PACE_ESIGN;
    if (capabilityDestroy)
        *sc_reader_t_capabilities |= SC_READER_CAP_PACE_DESTROY_CHANNEL;

    return SC_SUCCESS;
}

int boxing_pace_capabilities_to_buf(sc_context_t *ctx,
        const unsigned long sc_reader_t_capabilities,
        unsigned char **asn1, size_t *asn1_len)
{
    const size_t count_data =
        sizeof g_PACECapabilities_data/
        sizeof *g_PACECapabilities_data;
    const size_t count_sequence =
        sizeof g_PACECapabilities/
        sizeof *g_PACECapabilities;
    int yes = 1, no = 0;
    size_t i;
    struct sc_asn1_entry PACECapabilities_data[count_data];
    struct sc_asn1_entry PACECapabilities[count_sequence];

    for (i = 0; i < count_sequence; i++)
        sc_copy_asn1_entry(g_EstablishPACEChannel+i,
                PACECapabilities+i);
    sc_format_asn1_entry(PACECapabilities,
            PACECapabilities_data, 0, 1);

    for (i = 0; i < count_data; i++)
        sc_copy_asn1_entry(g_PACECapabilities_data+i,
                PACECapabilities_data+i);
    if (sc_reader_t_capabilities & SC_READER_CAP_PACE_GENERIC)
        sc_format_asn1_entry(PACECapabilities_data+0, 
                &yes, NULL, 1);
    else
        sc_format_asn1_entry(PACECapabilities_data+0, 
                &no, NULL, 1);
    if (sc_reader_t_capabilities & SC_READER_CAP_PACE_EID)
        sc_format_asn1_entry(PACECapabilities_data+1, 
                &yes, NULL, 1);
    else
        sc_format_asn1_entry(PACECapabilities_data+1, 
                &no, NULL, 1);
    if (sc_reader_t_capabilities & SC_READER_CAP_PACE_ESIGN)
        sc_format_asn1_entry(PACECapabilities_data+2, 
                &yes, NULL, 1);
    else
        sc_format_asn1_entry(PACECapabilities_data+2, 
                &no, NULL, 1);
    if (sc_reader_t_capabilities & SC_READER_CAP_PACE_DESTROY_CHANNEL)
        sc_format_asn1_entry(PACECapabilities_data+3, 
                &yes, NULL, 1);
    else
        sc_format_asn1_entry(PACECapabilities_data+3, 
                &no, NULL, 1);

    return sc_asn1_encode(ctx, PACECapabilities, asn1, asn1_len);
}

#ifdef DISABLE_GLOBAL_BOXING_INITIALIZATION
static
#endif
void sc_detect_boxing_cmds(sc_reader_t *reader)
{
    u8 rbuf[0xff];
    sc_apdu_t apdu;
    unsigned long capabilities;

	memset(&apdu, 0, sizeof(apdu));
    apdu.cse     = SC_APDU_CASE_1;
	apdu.cla     = boxing_cla;
	apdu.ins     = boxing_ins;
	apdu.p1      = boxing_p1;
	apdu.p2      = boxing_p2_GetReaderPACECapabilities;
    apdu.resp    = rbuf;
    apdu.resplen = sizeof rbuf;
    apdu.le      = sizeof rbuf;

    if (!reader || !reader->ops || !reader->ops->transmit
            || reader->ops->transmit(reader, &apdu) != SC_SUCCESS
            || apdu.sw1 != 0x90
            || apdu.sw2 != 0x00
            || boxing_buf_to_pace_capabilities(reader->ctx,
                apdu.resp, apdu.resplen, &capabilities) != SC_SUCCESS) {
        sc_debug(reader->ctx, SC_LOG_DEBUG_NORMAL,
                "%s does not support boxing commands", reader->name);
    } else {
        if (capabilities & SC_READER_CAP_PIN_PAD
                && !(reader->capabilities & SC_READER_CAP_PIN_PAD)) {
            ((struct sc_reader_operations *) reader->ops)->perform_verify =
                boxing_perform_verify;
            sc_debug(reader->ctx, SC_LOG_DEBUG_NORMAL,
                    "Added boxing command wrappers for PIN verification/modification to '%s'", reader->name);
        }

        if (capabilities & SC_READER_CAP_PACE_GENERIC
                && !(reader->capabilities & SC_READER_CAP_PACE_GENERIC)) {
            ((struct sc_reader_operations *) reader->ops)->perform_pace =
                boxing_perform_pace;
            sc_debug(reader->ctx, SC_LOG_DEBUG_NORMAL,
                    "Added boxing command wrappers for PACE to '%s'", reader->name);
        }

        reader->capabilities |= capabilities;
    }
}

#ifdef DISABLE_GLOBAL_BOXING_INITIALIZATION
void sc_initialize_boxing_cmds(sc_context_t *ctx)
{
    sc_reader_t *reader;

    if (!ctx)
        return;

    if (!list_iterator_start(&ctx->readers))
        return;

    reader = list_iterator_next(&ctx->readers);
    while (reader) {
        reader = list_iterator_next(&ctx->readers);
        sc_detect_boxing_cmds(reader);
    }
}
#endif
