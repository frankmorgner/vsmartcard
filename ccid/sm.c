#include "sm.h"
#include "utils.h"
#include <opensc/asn1.h>
#include <opensc/log.h>
#include <string.h>

static const struct sc_asn1_entry c_sm_capdu[] = {
    { "Padding-content indicator followed by cryptogram",
        SC_ASN1_INTEGER     , 0x87, SC_ASN1_OPTIONAL|SC_ASN1_UNSIGNED, NULL, NULL },
    { "Protected Le",
        SC_ASN1_INTEGER     , 0x97, SC_ASN1_OPTIONAL|SC_ASN1_UNSIGNED, NULL, NULL },
    { "Cryptographic Checksum",
        SC_ASN1_OCTET_STRING, 0x8E,                                 0, NULL, NULL },
    { NULL , 0 , 0 , 0 , NULL , NULL }
};

static const struct sc_asn1_entry c_sm_rapdu[] = {
    { "Padding-content indicator followed by cryptogram" ,
        SC_ASN1_INTEGER     , 0x87, SC_ASN1_OPTIONAL|SC_ASN1_UNSIGNED, NULL, NULL },
    { "Processing Status",
        SC_ASN1_INTEGER     , 0x99, SC_ASN1_OPTIONAL|SC_ASN1_UNSIGNED, NULL, NULL },
    { "Cryptographic Checksum",
        SC_ASN1_OCTET_STRING, 0x8E,                                 0, NULL, NULL },
    { NULL, 0, 0, 0, NULL, NULL }
};

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
            padded = add_iso_pad(tmp, EVP_CIPHER_block_size(ctx->cipher));
            if (tmp) {
                OPENSSL_cleanse(tmp->data, tmp->max);
                BUF_MEM_free(tmp);
            }
            break;
        default:
            return NULL;
    }

    return padded;
}

int no_padding(u8 padding_indicator, const char *data, size_t datalen,
        size_t *raw_len)
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

    *raw_len = len;

    return SC_SUCCESS;
}

u8 * format_le(size_t le_len, size_t le, struct sc_asn1_entry *le_entry)
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

BUF_MEM * prefix_buf(u8 prefix, BUF_MEM *buf)
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

BUF_MEM * format_data(const struct sm_ctx *ctx, const u8 *data, size_t datalen,
        struct sc_asn1_entry *formatted_encrypted_data_entry)
{

    BUF_MEM *pad_data = add_padding(ctx, (char *) data, datalen);
    BUF_MEM *enc_data = cipher(ctx->cipher_ctx, ctx->cipher,
            ctx->cipher_engine, ctx->key_enc, ctx->iv, 1, pad_data);

    if (pad_data) {
        OPENSSL_cleanse(pad_data->data, pad_data->max);
        BUF_MEM_free(pad_data);
    }

    BUF_MEM *indicator_encdata = prefix_buf(ctx->padding_indicator, enc_data);

    if (enc_data) {
        BUF_MEM_free(enc_data);
    }

    if (indicator_encdata)
        sc_format_asn1_entry(formatted_encrypted_data_entry,
                indicator_encdata->data, &(indicator_encdata->length),
                SC_ASN1_PRESENT);
    return indicator_encdata;
}

BUF_MEM * format_head(const struct sm_ctx *ctx, const sc_apdu_t *apdu)
{
    char head[4];

    head[0] = apdu->cla;
    head[1] = apdu->ins;
    head[2] = apdu->p1;
    head[3] = apdu->p2;
    return add_padding(ctx, head, sizeof head);
}

int sm_encrypt(const struct sm_ctx *ctx, sc_card_t *card,
        const sc_apdu_t *apdu, sc_apdu_t *sm_apdu)
{
    struct sc_asn1_entry sm_capdu[4];
    u8 *le = NULL;
    size_t oldlen;
    BUF_MEM *fdata = NULL, *sm_data = NULL,
            *mac_data = NULL, *mac = NULL, *tmp = NULL;
    int r;

    if (!apdu || !ctx || !card || !card->slot || !sm_apdu) {
        r = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }

    sc_copy_asn1_entry(c_sm_capdu, sm_capdu);

    mac_data = format_head(ctx, apdu);
    if (!mac_data) {
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
                r = SC_ERROR_WRONG_LENGTH;
                goto err;
            }
            break;
	case SC_APDU_CASE_2_EXT:
            if (card->slot->active_protocol == SC_PROTO_T0) {
                /* T0 extended APDUs look just like short APDUs */
                le = format_le(1, apdu->le, sm_capdu + 1);
                if (!le) {
                    r = SC_ERROR_WRONG_LENGTH;
                    goto err;
                }
            } else {
                /* in case of T1 always use 3 bytes for length */
                le = format_le(3, apdu->le, sm_capdu + 1);
                if (!le) {
                    r = SC_ERROR_WRONG_LENGTH;
                    goto err;
                }
            }
            break;
        case SC_APDU_CASE_3_SHORT:
        case SC_APDU_CASE_3_EXT:
            fdata = format_data(ctx, apdu->data, apdu->datalen,
                    sm_capdu + 0);
            if (!fdata) {
                r = SC_ERROR_OUT_OF_MEMORY;
                goto err;
            }
            break;
        case SC_APDU_CASE_4_SHORT:
            /* in case of T0 no Le byte is added */
            if (card->slot->active_protocol != SC_PROTO_T0) {
                le = format_le(1, apdu->le, sm_capdu + 1);
                if (!le) {
                    r = SC_ERROR_WRONG_LENGTH;
                    goto err;
                }
            }

            fdata = format_data(ctx, apdu->data, apdu->datalen,
                    sm_capdu + 0);
            if (!fdata) {
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
                    r = SC_ERROR_WRONG_LENGTH;
                    goto err;
                }
            }

            fdata = format_data(ctx, apdu->data, apdu->datalen,
                    sm_capdu + 0);
            if (!fdata) {
                r = SC_ERROR_OUT_OF_MEMORY;
                goto err;
            }
            break;
        default:
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
    BUF_MEM_free(mac_data);
    mac_data = hash(ctx->md, ctx->md_ctx, ctx->md_engine, tmp);
    mac = cipher(ctx->cipher_ctx, ctx->cipher, ctx->cipher_engine,
            ctx->key_mac, ctx->iv, 1, mac_data);
    if (!mac) {
        r = SC_ERROR_INTERNAL;
        goto err;
    }
    sc_format_asn1_entry(sm_capdu + 2, mac->data, &(mac->length),
            SC_ASN1_PRESENT);


    /* format SM apdu */
    BUF_MEM_free(sm_data);
    sm_data = BUF_MEM_new();
    if (!sm_data) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    r = sc_asn1_encode(card->ctx, sm_capdu, (u8 **) &sm_data->data, &sm_data->length);
    if (r < 0)
        goto err;
    memcpy(sm_apdu, apdu, sizeof *sm_apdu);
    sm_apdu->data = (u8 *) sm_data->data;
    sm_apdu->lc = sm_data->length;
    sm_apdu->le = 0;
    sm_apdu->cse = SC_APDU_CASE_4;


    /* BUF_MEM_free must not free sm_data->data */
    sm_data->data = NULL;
    sm_data->length = 0;
    sm_data->max = 0;

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
    if (sm_data) {
        BUF_MEM_free(sm_data);
    }
    if (le) {
        free(le);
    }

    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_ERROR, r);
}

int sm_decrypt(const struct sm_ctx *ctx, sc_card_t *card,
        const sc_apdu_t *sm_apdu, sc_apdu_t *apdu)
{
    int r;
    struct sc_asn1_entry sm_rapdu[4];
    u8 sw[2], mac[EVP_CIPHER_block_size(ctx->cipher)], fdata[1024];
    size_t sw_len = sizeof sw, mac_len = sizeof mac, fdata_len = sizeof fdata,
           buf_len, oldlen;
    const u8 *buf;
    BUF_MEM *mac_data = NULL, *tmp = NULL, *my_mac = NULL, *data = NULL,
            enc_data;

    sc_copy_asn1_entry(c_sm_rapdu, sm_rapdu);
    sc_format_asn1_entry(sm_rapdu + 0, fdata, &fdata_len, 0);
    sc_format_asn1_entry(sm_rapdu + 1, sw, &sw_len, 0);
    sc_format_asn1_entry(sm_rapdu + 2, mac, &mac_len, 0);

    r = sc_asn1_decode(card->ctx, sm_rapdu, apdu->resp, apdu->resplen,
            &buf, &buf_len);
    if (r < 0)
        goto err;
    if (buf_len > 0) {
        r = SC_ERROR_TOO_MANY_OBJECTS;
        goto err;
    }


    if (sm_rapdu[0].flags & SC_ASN1_PRESENT) {
        if (ctx->padding_indicator != fdata[0]) {
            r = SC_ERROR_DECRYPT_FAILED;
            goto err;
        }
        enc_data.data = (char *) fdata + 1;
        enc_data.length = fdata_len - 1;
        enc_data.max = enc_data.length;
    }


    mac_data = format_head(ctx, sm_apdu);

    tmp = BUF_MEM_new();
    if (!tmp) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    sc_format_asn1_entry(sm_rapdu + 2, NULL, NULL, 0);
    r = sc_asn1_encode(card->ctx, sm_rapdu, (u8 **) &tmp->data, &tmp->length);
    if (r < 0)
        goto err;
    oldlen = mac_data->length;
    BUF_MEM_grow(mac_data, oldlen + tmp->length);
    memcpy(mac_data->data + oldlen, tmp->data, tmp->length);
    free(tmp->data);
    tmp = add_padding(ctx, mac_data->data, mac_data->length);
    BUF_MEM_free(mac_data);
    mac_data = hash(ctx->md, ctx->md_ctx, ctx->md_engine, tmp);
    my_mac = cipher(ctx->cipher_ctx, ctx->cipher, ctx->cipher_engine,
            ctx->key_mac, ctx->iv, 1, mac_data);
    if (!my_mac) {
        r = SC_ERROR_INTERNAL;
        goto err;
    }

    if (my_mac->length != mac_len ||
            memcmp(my_mac->data, mac, mac_len) != 0) {
        r = SC_ERROR_DECRYPT_FAILED;
        goto err;
    }


    data = cipher(ctx->cipher_ctx, ctx->cipher, ctx->cipher_engine,
            ctx->key_enc, ctx->iv, 0, &enc_data);
    r = no_padding(ctx->padding_indicator, data->data, data->length,
            &data->length);
    if (r < 0)
        goto err;


    memcpy(apdu, sm_apdu, sizeof *apdu);
    apdu->resp = (u8 *) data->data;
    apdu->resplen = data->length;


    /* BUF_MEM_free must not free data->data */
    data->data = NULL;
    data->length = 0;
    data->max = 0;

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
    if (data) {
        BUF_MEM_free(data);
    }

    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_ERROR, r);
}
