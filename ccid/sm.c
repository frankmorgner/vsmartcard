/*
 * Copyright (C) 2010 Frank Morgner
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
#include "sm.h"
#include "apdu.h"
#include <arpa/inet.h>
#include <opensc/asn1.h>
#include <opensc/log.h>
#include <stdlib.h>
#include <string.h>

static const struct sc_asn1_entry c_sm_capdu[] = {
    { "Padding-content indicator followed by cryptogram",
        SC_ASN1_OCTET_STRING, SC_ASN1_CTX|0x07, SC_ASN1_OPTIONAL                 , NULL, NULL },
    { "Protected Le",
        SC_ASN1_INTEGER     , SC_ASN1_CTX|0x17, SC_ASN1_OPTIONAL|SC_ASN1_UNSIGNED, NULL, NULL },
    { "Cryptographic Checksum",
        SC_ASN1_OCTET_STRING, SC_ASN1_CTX|0x0E, SC_ASN1_OPTIONAL                 , NULL, NULL },
    { NULL , 0 , 0 , 0 , NULL , NULL }
};

static const struct sc_asn1_entry c_sm_rapdu[] = {
    { "Padding-content indicator followed by cryptogram" ,
        SC_ASN1_OCTET_STRING, SC_ASN1_CTX|0x07, SC_ASN1_OPTIONAL                 , NULL, NULL },
    { "Processing Status",
        SC_ASN1_INTEGER     , SC_ASN1_CTX|0x19, SC_ASN1_OPTIONAL|SC_ASN1_UNSIGNED, NULL, NULL },
    { "Cryptographic Checksum",
        SC_ASN1_OCTET_STRING, SC_ASN1_CTX|0x0E, SC_ASN1_OPTIONAL                 , NULL, NULL },
    { NULL, 0, 0, 0, NULL, NULL }
};

void bin_log(sc_context_t *ctx, const char *label, const u8 *data, size_t len)
{
    char buf[1024];

    sc_hex_dump(ctx, data, len, buf, sizeof buf);
    sc_debug(ctx, "%s (%u bytes):\n%s"
            "======================================================================",
            label, len, buf);
}

BUF_MEM *
add_iso_pad(const BUF_MEM * m, int block_size)
{
    BUF_MEM * out = NULL;
    int p_len;

    if (!m)
        goto err;

    /* calculate length of padded message */
    p_len = (m->length / block_size) * block_size + block_size;

    out = BUF_MEM_create(p_len);
    if (!out)
        goto err;

    memcpy(out->data, m->data, m->length);

    /* now add iso padding */
    memset(out->data + m->length, 0x80, 1);
    memset(out->data + m->length + 1, 0, p_len - m->length - 1);

    return out;

err:
    if (out)
        BUF_MEM_free(out);

    return NULL;
}

BUF_MEM * add_padding(const struct sm_ctx *ctx, const char *data, size_t datalen)
{
    BUF_MEM *tmp = BUF_MEM_create_init(data, datalen);
    BUF_MEM *padded = NULL;

    switch (ctx->padding_indicator) {
        case SM_NO_PADDING:
            padded = tmp;
            tmp = NULL;
            break;
        case SM_ISO_PADDING:
            padded = add_iso_pad(tmp, ctx->block_length);
            /* fall through */
        default:
            if (tmp) {
                sc_mem_clear(tmp->data, tmp->max);
                BUF_MEM_free(tmp);
            }
            break;
    }

    return padded;
}

int no_padding(u8 padding_indicator, const u8 *data, size_t datalen)
{
    if (!datalen || !data)
        return SC_ERROR_INVALID_ARGUMENTS;

    size_t len;

    switch (padding_indicator) {
        case SM_NO_PADDING:
            len = datalen;
            break;
        case SM_ISO_PADDING:
            for (len = datalen;
                    len && (data[len-1] == 0);
                    len--) { };

            if (data[len-1] != 0x80)
                return SC_ERROR_INVALID_DATA;
            break;
        default:
            return SC_ERROR_NOT_SUPPORTED;
    }

    return len;
}

static u8 * format_le(size_t le_len, size_t le, struct sc_asn1_entry *le_entry)
{
    u8 * lebuf = malloc(le_len);
    if (lebuf) {
        switch (le_len) {
            case 1:
                lebuf[0] = le;
                break;
            case 2:
                lebuf[0] = htons(le) >> 8;
                lebuf[1] = htons(le) & 0xff;
                break;
            case 3:
                lebuf[0] = 0x00;
                lebuf[1] = htons(le) >> 8;
                lebuf[2] = htons(le) & 0xff;
                break;
            default:
                free(lebuf);
                return NULL;
        }

        sc_format_asn1_entry(le_entry, lebuf, &le_len, SC_ASN1_PRESENT);
    }

    return lebuf;
}

static BUF_MEM * prefix_buf(u8 prefix, BUF_MEM *buf)
{
    if (!buf) return NULL;
    BUF_MEM *cat = BUF_MEM_create(buf->length + 1);
    if (cat) {
        cat->data[0] = prefix;
        memcpy(cat->data + 1, buf->data, buf->length);
        cat->length = buf->length + 1;
    }
    return cat;
}

static BUF_MEM * format_data(sc_card_t *card, const struct sm_ctx *ctx, const u8 *data, size_t datalen,
        struct sc_asn1_entry *formatted_encrypted_data_entry)
{
    int r;

    if (!ctx)
        return NULL;

    BUF_MEM *pad_data = add_padding(ctx, (char *) data, datalen);
    BUF_MEM *enc_data = BUF_MEM_new();
    if (!pad_data || !enc_data)
        return NULL;

    r = ctx->encrypt(card, ctx, (u8 *) pad_data->data, pad_data->length,
            (u8 **) &enc_data->data);
    if (r < 0)
        return NULL;
    enc_data->length = r;

    BUF_MEM *indicator_encdata = prefix_buf(ctx->padding_indicator, enc_data);

    sc_mem_clear(pad_data->data, pad_data->max);
    BUF_MEM_free(pad_data);
    BUF_MEM_free(enc_data);

    if (indicator_encdata)
        sc_format_asn1_entry(formatted_encrypted_data_entry,
                indicator_encdata->data, &indicator_encdata->length,
                SC_ASN1_PRESENT);

    return indicator_encdata;
}

static BUF_MEM * format_head(const struct sm_ctx *ctx, const sc_apdu_t *apdu)
{
    char head[4];

    head[0] = apdu->cla;
    head[1] = apdu->ins;
    head[2] = apdu->p1;
    head[3] = apdu->p2;
    return add_padding(ctx, head, sizeof head);
}

static int sm_encrypt(const struct sm_ctx *ctx, sc_card_t *card,
        const sc_apdu_t *apdu, sc_apdu_t *sm_apdu)
{
    struct sc_asn1_entry sm_capdu[4];
    u8 *le = NULL, *sm_data = NULL;
    size_t oldlen, sm_data_len;
    BUF_MEM *fdata = NULL, *mac_data = NULL, *mac = NULL, *tmp = NULL;
    int r;

    if (!apdu || !ctx || !card || !card->slot || !sm_apdu) {
        r = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }

    sc_copy_asn1_entry(c_sm_capdu, sm_capdu);

    sm_apdu->sensitive = 0;
    sm_apdu->resp = apdu->resp;
    sm_apdu->resplen = apdu->resplen;
    sm_apdu->control = apdu->control;
    sm_apdu->flags = apdu->flags;
    sm_apdu->cla = 0x0C;
    sm_apdu->ins = apdu->ins;
    sm_apdu->p1 = apdu->p1;
    sm_apdu->p2 = apdu->p2;
    mac_data = format_head(ctx, apdu);
    if (!mac_data) {
        sc_error(card->ctx, "Could not format header of SM apdu.");
        r = SC_ERROR_WRONG_PADDING;
        goto err;
    }

    /* get le and data depending on the case of the unsecure command */
    switch (apdu->cse) {
        case SC_APDU_CASE_1:
            break;
	case SC_APDU_CASE_2_SHORT:
            le = format_le(1, apdu->le, sm_capdu + 1);
            if (!le) {
                sc_error(card->ctx, "Could not format Le of SM apdu.");
                r = SC_ERROR_WRONG_LENGTH;
                goto err;
            }
            break;
	case SC_APDU_CASE_2_EXT:
            if (card->slot->active_protocol == SC_PROTO_T0) {
                /* T0 extended APDUs look just like short APDUs */
                le = format_le(1, apdu->le, sm_capdu + 1);
                if (!le) {
                    sc_error(card->ctx, "Could not format Le of SM apdu.");
                    r = SC_ERROR_WRONG_LENGTH;
                    goto err;
                }
            } else {
                /* in case of T1 always use 3 bytes for length */
                le = format_le(3, apdu->le, sm_capdu + 1);
                if (!le) {
                    sc_error(card->ctx, "Could not format Le of SM apdu.");
                    r = SC_ERROR_WRONG_LENGTH;
                    goto err;
                }
            }
            break;
        case SC_APDU_CASE_3_SHORT:
        case SC_APDU_CASE_3_EXT:
            fdata = format_data(card, ctx, apdu->data, apdu->datalen,
                    sm_capdu + 0);
            if (!fdata) {
                sc_error(card->ctx, "Could not format data of SM apdu.");
                r = SC_ERROR_OUT_OF_MEMORY;
                goto err;
            }
            break;
        case SC_APDU_CASE_4_SHORT:
            /* in case of T0 no Le byte is added */
            if (card->slot->active_protocol != SC_PROTO_T0) {
                le = format_le(1, apdu->le, sm_capdu + 1);
                if (!le) {
                    sc_error(card->ctx, "Could not format Le of SM apdu.");
                    r = SC_ERROR_WRONG_LENGTH;
                    goto err;
                }
            }

            fdata = format_data(card, ctx, apdu->data, apdu->datalen,
                    sm_capdu + 0);
            if (!fdata) {
                sc_error(card->ctx, "Could not format data of SM apdu.");
                r = SC_ERROR_OUT_OF_MEMORY;
                goto err;
            }
            break;
        case SC_APDU_CASE_4_EXT:
            if (card->slot->active_protocol == SC_PROTO_T0) {
                /* again a T0 extended case 4 APDU looks just
                 * like a short APDU, the additional data is
                 * transferred using ENVELOPE and GET RESPONSE */
            } else {
                /* only 2 bytes are use to specify the length of the
                 * expected data */
                le = format_le(2, apdu->le, sm_capdu + 1);
                if (!le) {
                    sc_error(card->ctx, "Could not format Le of SM apdu.");
                    r = SC_ERROR_WRONG_LENGTH;
                    goto err;
                }
            }

            fdata = format_data(card, ctx, apdu->data, apdu->datalen,
                    sm_capdu + 0);
            if (!fdata) {
                sc_error(card->ctx, "Could not format data of SM apdu.");
                r = SC_ERROR_OUT_OF_MEMORY;
                goto err;
            }
            break;
        default:
            sc_error(card->ctx, "Unhandled apdu case.");
            r = SC_ERROR_INVALID_DATA;
            goto err;
    }


    tmp = BUF_MEM_new();
    if (!tmp) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    r = sc_asn1_encode(card->ctx, sm_capdu, (u8 **) &tmp->data, &tmp->length);
    if (r < 0) {
        goto err;
    }
    oldlen = mac_data->length;
    BUF_MEM_grow(mac_data, oldlen + tmp->length);
    memcpy(mac_data->data + oldlen, tmp->data, tmp->length);
    BUF_MEM_free(tmp);
    tmp = add_padding(ctx, mac_data->data, mac_data->length);
    mac = BUF_MEM_new();
    if (!mac) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    r = ctx->authenticate(card, ctx, (u8 *) tmp->data, tmp->length,
            (u8 **) &mac->data);
    if (r < 0) {
        sc_error(card->ctx, "Could not get authentication code");
        goto err;
    }
    mac->length = r;
    bin_log(card->ctx, "mac", (u8 *)mac->data, mac->length);
    sc_format_asn1_entry(sm_capdu + 2, mac->data, &mac->length,
            SC_ASN1_PRESENT);


    /* format SM apdu */
    r = sc_asn1_encode(card->ctx, sm_capdu, (u8 **) &sm_data, &sm_data_len);
    if (r < 0)
        goto err;
    sm_apdu->data = sm_data;
    sm_apdu->datalen = sm_data_len;
    sm_apdu->lc = sm_apdu->datalen;
    sm_apdu->le = 0;
    sm_apdu->cse = SC_APDU_CASE_4;
    bin_log(card->ctx, "sm apdu data", sm_apdu->data, sm_apdu->datalen);

err:
    if (fdata) {
        BUF_MEM_free(fdata);
    }
    if (tmp) {
        BUF_MEM_free(tmp);
    }
    if (mac_data) {
        BUF_MEM_free(mac_data);
    }
    if (mac) {
        BUF_MEM_free(mac);
    }
    if (le) {
        free(le);
    }

    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_ERROR, r);
}

static int sm_decrypt(const struct sm_ctx *ctx, sc_card_t *card,
        const sc_apdu_t *sm_apdu, sc_apdu_t *apdu)
{
    int r;
    struct sc_asn1_entry sm_rapdu[4];
    struct sc_asn1_entry my_sm_rapdu[4];
    u8 sw[2], mac[256], fdata[1024];
    size_t sw_len = sizeof sw, mac_len = sizeof mac, fdata_len = sizeof fdata,
           buf_len;
    const u8 *buf;
    u8 *data;
    BUF_MEM *mac_data = NULL, *tmp = NULL, *my_mac = NULL;

    sc_copy_asn1_entry(c_sm_rapdu, sm_rapdu);
    sc_format_asn1_entry(sm_rapdu + 0, fdata, &fdata_len, 0);
    sc_format_asn1_entry(sm_rapdu + 1, sw, &sw_len, 0);
    sc_format_asn1_entry(sm_rapdu + 2, mac, &mac_len, 0);

    r = sc_asn1_decode(card->ctx, sm_rapdu, apdu->resp, apdu->resplen,
            &buf, &buf_len);
    if (r < 0)
        goto err;
    if (buf_len > 0) {
        r = SC_ERROR_UNKNOWN_DATA_RECEIVED;
        goto err;
    }


    if (sm_rapdu[2].flags & SC_ASN1_PRESENT) {
        sc_copy_asn1_entry(sm_rapdu, my_sm_rapdu);

        tmp = BUF_MEM_new();
        if (!tmp) {
            r = SC_ERROR_OUT_OF_MEMORY;
            goto err;
        }
        sc_format_asn1_entry(my_sm_rapdu + 2, NULL, NULL, 0);
        r = sc_asn1_encode(card->ctx, my_sm_rapdu, (u8 **) &tmp->data, &tmp->length);
        if (r < 0)
            goto err;
        mac_data = add_padding(ctx, tmp->data, tmp->length);
        if (!mac_data) {
            r = SC_ERROR_INTERNAL;
            goto err;
        }
        
        r = ctx->verify_authentication(card, ctx, mac, mac_len,
                (u8 *) mac_data->data, mac_data->length);
        if (r < 0)
            goto err;
    }


    if (sm_rapdu[0].flags & SC_ASN1_PRESENT) {
        if (ctx->padding_indicator != fdata[0]) {
            r = SC_ERROR_UNKNOWN_DATA_RECEIVED;
            goto err;
        }
        r = ctx->decrypt(card, ctx, fdata + 1, fdata_len - 1, &data);
        if (r < 0)
            goto err;
        buf_len = r;

        r = no_padding(ctx->padding_indicator, data, buf_len);
        if (r < 0)
            goto err;

        memcpy(apdu, sm_apdu, sizeof *apdu);
        apdu->resp = data;
        apdu->resplen = r;
    } else {
        apdu->resplen = r;
    }


    r = SC_SUCCESS;

err:
    if (tmp) {
        BUF_MEM_free(tmp);
    }
    if (mac_data) {
        BUF_MEM_free(mac_data);
    }
    if (my_mac) {
        BUF_MEM_free(my_mac);
    }

    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_ERROR, r);
}

int sm_transmit_apdu(const struct sm_ctx *sctx, sc_card_t *card,
        sc_apdu_t *apdu)
{
    sc_apdu_t sm_apdu;
    u8 buf[SC_MAX_APDU_BUFFER_SIZE - 2];

    SC_TEST_RET(card->ctx, sm_encrypt(sctx, card, apdu, &sm_apdu),
            "Could not encrypt APDU.");
    bin_log(card->ctx, "SM APDU data", sm_apdu.data, sm_apdu.datalen);

    sm_apdu.resplen = sizeof *buf;
    sm_apdu.resp = buf;
    SC_TEST_RET(card->ctx, my_transmit_apdu(card, &sm_apdu),
            "Could not send SM APDU.");

    SC_TEST_RET(card->ctx, sc_check_sw(card, sm_apdu.sw1, sm_apdu.sw2),
            "Card returned error.");
    SC_TEST_RET(card->ctx, sm_decrypt(sctx, card, &sm_apdu, apdu),
            "Could not decrypt APDU.");
    bin_log(card->ctx, "SM APDU response", apdu->resp, apdu->resplen);

    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_ERROR, SC_SUCCESS);
}
