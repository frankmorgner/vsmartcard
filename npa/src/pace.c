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
#include "pace.h"
#include "sm.h"
#include "scutil.h"
#include <opensc/asn1.h>
#include <opensc/log.h>
#include <opensc/opensc.h>
#include <openssl/asn1t.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/cv_cert.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/pace.h>
#include <string.h>


#define ASN1_APP_IMP_OPT(stname, field, type, tag) ASN1_EX_TYPE(ASN1_TFLG_IMPTAG|ASN1_TFLG_APPLICATION|ASN1_TFLG_OPTIONAL, tag, stname, field, type)
#define ASN1_APP_IMP(stname, field, type, tag) ASN1_EX_TYPE(ASN1_TFLG_IMPTAG|ASN1_TFLG_APPLICATION, tag, stname, field, type)

/*
 * MSE:Set AT
 */

typedef struct pace_mse_set_at_cd_st {
    ASN1_OBJECT *cryptographic_mechanism_reference;
    ASN1_INTEGER *key_reference1;
    ASN1_INTEGER *key_reference2;
    ASN1_OCTET_STRING *auxiliary_data;
    ASN1_OCTET_STRING *eph_pub_key;
    CVC_CHAT *chat;
} PACE_MSE_SET_AT_C;
ASN1_SEQUENCE(PACE_MSE_SET_AT_C) = {
    /* 0x80
     * Cryptographic mechanism reference */
    ASN1_IMP_OPT(PACE_MSE_SET_AT_C, cryptographic_mechanism_reference, ASN1_OBJECT, 0),
    /* 0x83
     * Reference of a public key / secret key */
    ASN1_IMP_OPT(PACE_MSE_SET_AT_C, key_reference1, ASN1_INTEGER, 3),
    /* 0x84
     * Reference of a private key / Reference for computing a session key */
    ASN1_IMP_OPT(PACE_MSE_SET_AT_C, key_reference2, ASN1_INTEGER, 4),
    /* 0x67
     * Auxiliary authenticated data */
    ASN1_APP_IMP_OPT(PACE_MSE_SET_AT_C, auxiliary_data, ASN1_OCTET_STRING, 7),
    /* 0x91
     * Ephemeral Public Key */
    ASN1_IMP_OPT(PACE_MSE_SET_AT_C, eph_pub_key, ASN1_OCTET_STRING, 0x11),
    /* Certificate Holder Authorization Template */
    ASN1_OPT(PACE_MSE_SET_AT_C, chat, CVC_CHAT),
} ASN1_SEQUENCE_END(PACE_MSE_SET_AT_C)
IMPLEMENT_ASN1_FUNCTIONS(PACE_MSE_SET_AT_C)


/*
 * General Authenticate
 */

/* Protocol Command Data */
typedef struct pace_gen_auth_cd_st {
    ASN1_OCTET_STRING *mapping_data;
    ASN1_OCTET_STRING *eph_pub_key;
    ASN1_OCTET_STRING *auth_token;
} PACE_GEN_AUTH_C_BODY;
ASN1_SEQUENCE(PACE_GEN_AUTH_C_BODY) = {
    /* 0x81
     * Mapping Data */
    ASN1_IMP_OPT(PACE_GEN_AUTH_C_BODY, mapping_data, ASN1_OCTET_STRING, 1),
    /* 0x83
     * Ephemeral Public Key */
    ASN1_IMP_OPT(PACE_GEN_AUTH_C_BODY, eph_pub_key, ASN1_OCTET_STRING, 3),
    /* 0x85
     * Authentication Token */
    ASN1_IMP_OPT(PACE_GEN_AUTH_C_BODY, auth_token, ASN1_OCTET_STRING, 5),
} ASN1_SEQUENCE_END(PACE_GEN_AUTH_C_BODY)
IMPLEMENT_ASN1_FUNCTIONS(PACE_GEN_AUTH_C_BODY)

typedef PACE_GEN_AUTH_C_BODY PACE_GEN_AUTH_C;
/* 0x7C
 * Dynamic Authentication Data */
ASN1_ITEM_TEMPLATE(PACE_GEN_AUTH_C) =
    ASN1_EX_TEMPLATE_TYPE(
            ASN1_TFLG_IMPTAG|ASN1_TFLG_APPLICATION,
            0x1c, PACE_GEN_AUTH_C, PACE_GEN_AUTH_C_BODY)
ASN1_ITEM_TEMPLATE_END(PACE_GEN_AUTH_C)
IMPLEMENT_ASN1_FUNCTIONS(PACE_GEN_AUTH_C)

/* Protocol Response Data */
typedef struct pace_gen_auth_rapdu_body_st {
    ASN1_OCTET_STRING *enc_nonce;
    ASN1_OCTET_STRING *mapping_data;
    ASN1_OCTET_STRING *eph_pub_key;
    ASN1_OCTET_STRING *auth_token;
    ASN1_OCTET_STRING *cur_car;
    ASN1_OCTET_STRING *prev_car;
} PACE_GEN_AUTH_R_BODY;
ASN1_SEQUENCE(PACE_GEN_AUTH_R_BODY) = {
    /* 0x80
     * Encrypted Nonce */
    ASN1_IMP_OPT(PACE_GEN_AUTH_R_BODY, enc_nonce, ASN1_OCTET_STRING, 0),
    /* 0x82
     * Mapping Data */
    ASN1_IMP_OPT(PACE_GEN_AUTH_R_BODY, mapping_data, ASN1_OCTET_STRING, 2),
    /* 0x84
     * Ephemeral Public Key */
    ASN1_IMP_OPT(PACE_GEN_AUTH_R_BODY, eph_pub_key, ASN1_OCTET_STRING, 4),
    /* 0x86
     * Authentication Token */
    ASN1_IMP_OPT(PACE_GEN_AUTH_R_BODY, auth_token, ASN1_OCTET_STRING, 6),
    /* 0x87
     * Most recent Certification Authority Reference */
    ASN1_IMP_OPT(PACE_GEN_AUTH_R_BODY, cur_car, ASN1_OCTET_STRING, 7),
    /* 0x88
     * Previous Certification Authority Reference */
    ASN1_IMP_OPT(PACE_GEN_AUTH_R_BODY, prev_car, ASN1_OCTET_STRING, 8),
} ASN1_SEQUENCE_END(PACE_GEN_AUTH_R_BODY)
IMPLEMENT_ASN1_FUNCTIONS(PACE_GEN_AUTH_R_BODY)

typedef PACE_GEN_AUTH_R_BODY PACE_GEN_AUTH_R;
/* 0x7C
 * Dynamic Authentication Data */
ASN1_ITEM_TEMPLATE(PACE_GEN_AUTH_R) =
    ASN1_EX_TEMPLATE_TYPE(
            ASN1_TFLG_IMPTAG|ASN1_TFLG_APPLICATION,
            0x1c, PACE_GEN_AUTH_R, PACE_GEN_AUTH_R_BODY)
ASN1_ITEM_TEMPLATE_END(PACE_GEN_AUTH_R)
IMPLEMENT_ASN1_FUNCTIONS(PACE_GEN_AUTH_R)



const size_t maxresp = SC_MAX_APDU_BUFFER_SIZE - 2;

static int pace_sm_encrypt(sc_card_t *card, const struct sm_ctx *ctx,
        const u8 *data, size_t datalen, u8 **enc);
static int pace_sm_decrypt(sc_card_t *card, const struct sm_ctx *ctx,
        const u8 *enc, size_t enclen, u8 **data);
static int pace_sm_authenticate(sc_card_t *card, const struct sm_ctx *ctx,
        const u8 *data, size_t datalen, u8 **outdata);
static int pace_sm_verify_authentication(sc_card_t *card, const struct sm_ctx *ctx,
        const u8 *mac, size_t maclen,
        const u8 *macdata, size_t macdatalen);
static int pace_sm_pre_transmit(sc_card_t *card, const struct sm_ctx *ctx,
        sc_apdu_t *apdu);
static int pace_sm_post_transmit(sc_card_t *card, const struct sm_ctx *ctx,
        sc_apdu_t *sm_apdu);

static int increment_ssc(struct pace_sm_ctx *psmctx);
static int decrement_ssc(struct pace_sm_ctx *psmctx);
static int reset_ssc(struct pace_sm_ctx *psmctx);


int GetReadersPACECapabilities(u8 *bitmap)
{
    if (!bitmap)
        return SC_ERROR_INVALID_ARGUMENTS;

    /* BitMap */
    *bitmap = PACE_BITMAP_PACE|PACE_BITMAP_EID|PACE_BITMAP_ESIGN;

    return SC_SUCCESS;
}

/** select and read EF.CardAccess */
int get_ef_card_access(sc_card_t *card,
        u8 **ef_cardaccess, size_t *length_ef_cardaccess)
{
    int r;
    /* we read less bytes than possible. this is a workaround for acr 122,
     * which only supports apdus of max 250 bytes */
    size_t read = maxresp - 8;
    sc_path_t path;
    sc_file_t *file = NULL;
    u8 *p;

    if (!ef_cardaccess || !length_ef_cardaccess) {
        r = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }

    memset(&path, 0, sizeof path);
    r = sc_append_file_id(&path, FID_EF_CARDACCESS);
    if (r < 0) {
        sc_error(card->ctx, "Could not create path object.");
        goto err;
    }
    r = sc_concatenate_path(&path, sc_get_mf_path(), &path);
    if (r < 0) {
        sc_error(card->ctx, "Could not create path object.");
        goto err;
    }

    r = sc_select_file(card, &path, &file);
    if (r < 0) {
        sc_error(card->ctx, "Could not select EF.CardAccess.");
        goto err;
    }

    *length_ef_cardaccess = 0;
    while(1) {
        p = realloc(*ef_cardaccess, *length_ef_cardaccess + read);
        if (!p) {
            r = SC_ERROR_OUT_OF_MEMORY;
            goto err;
        }
        *ef_cardaccess = p;

        r = sc_read_binary(card, *length_ef_cardaccess,
                *ef_cardaccess + *length_ef_cardaccess, read, 0);

        if (r > 0 && r != read) {
            *length_ef_cardaccess += r;
            break;
        }

        if (r < 0) {
            sc_error(card->ctx, "Could not read EF.CardAccess.");
            goto err;
        }

        *length_ef_cardaccess += r;
    }

    /* test cards only return an empty FCI template,
     * so we can't determine any file proberties */
    if (*length_ef_cardaccess < file->size) {
        sc_error(card->ctx, "Actual filesize differs from the size in file "
                "proberties (%u!=%u).", *length_ef_cardaccess, file->size);
        r = SC_ERROR_FILE_TOO_SMALL;
        goto err;
    }

    /* All cards with PACE support extended length
     * XXX this should better be done by the card driver */
    card->caps |= SC_CARD_CAP_APDU_EXT;

    r = SC_SUCCESS;

err:
    if (file) {
        free(file);
    }

    return r;
}

static int pace_mse_set_at(struct sm_ctx *oldpacectx, sc_card_t *card,
        int protocol, int secret_key, const CVC_CHAT *chat, u8 *sw1, u8 *sw2)
{
    sc_apdu_t apdu;
    unsigned char *d = NULL;
    PACE_MSE_SET_AT_C *data = NULL;
    int r, tries;

    if (!sw1 || !sw2) {
        r = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }

    memset(&apdu, 0, sizeof apdu);
    apdu.ins = 0x22;
    apdu.p1 = 0xc1;
    apdu.p2 = 0xa4;
    apdu.cse = SC_APDU_CASE_3_SHORT;
    apdu.flags = SC_APDU_FLAGS_NO_GET_RESP|SC_APDU_FLAGS_NO_RETRY_WL;


    data = PACE_MSE_SET_AT_C_new();
    if (!data) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }

    data->cryptographic_mechanism_reference = OBJ_nid2obj(protocol);
    data->key_reference1 = ASN1_INTEGER_new();

    if (!data->cryptographic_mechanism_reference
            || !data->key_reference1) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }

    if (!ASN1_INTEGER_set(data->key_reference1, secret_key)) {
        sc_error(card->ctx, "Error setting key reference 1 of MSE:Set AT data");
        r = SC_ERROR_INTERNAL;
        goto err;
    }

    data->chat = (CVC_CHAT *) chat;


    r = i2d_PACE_MSE_SET_AT_C(data, &d);
    if (r < 0) {
        sc_error(card->ctx, "Error encoding MSE:Set AT APDU data");
        r = SC_ERROR_INTERNAL;
        goto err;
    }
    /* The tag/length for the sequence (0x30) is omitted in the command apdu. */
    /* XXX is there a OpenSSL way to get the value only or even a define for
     * the tag? */
    apdu.data = sc_asn1_find_tag(card->ctx, d, r, 0x30, &apdu.datalen);
    apdu.lc = apdu.datalen;

    if (card->ctx->debug > SC_LOG_TYPE_DEBUG)
        bin_log(card->ctx, "MSE:Set AT command data", apdu.data, apdu.datalen);

    if (oldpacectx)
        r = sm_transmit_apdu(oldpacectx, card, &apdu);
    else
        r = sc_transmit_apdu(card, &apdu);
    if (r < 0)
        goto err;

    if (apdu.resplen) {
        sc_error(card->ctx, "MSE:Set AT response data should be empty "
                "(contains %u bytes)", apdu.resplen);
        r = SC_ERROR_UNKNOWN_DATA_RECEIVED;
        goto err;
    }

    *sw1 = apdu.sw1;
    *sw2 = apdu.sw2;

    if (apdu.sw1 == 0x63) {
        if ((apdu.sw2 & 0xc0) == 0xc0) {
            tries = apdu.sw2 & 0x0f;
            if (tries <= 1) {
                /* this is only a warning... */
                sc_error(card->ctx, "Remaining tries: %d (%s must be %s)\n",
                        tries, pace_secret_name(secret_key),
                        tries ? "resumed" : "unblocked");
            }
            r = SC_SUCCESS;
        } else {
            sc_error(card->ctx, "Unknown status bytes: SW1=%02X, SW2=%02X\n",
                    apdu.sw1, apdu.sw2);
            r = SC_ERROR_CARD_CMD_FAILED;
            goto err;
        }
    } else if (apdu.sw1 == 0x62 && apdu.sw2 == 0x83) {
             sc_error(card->ctx, "Password is deactivated\n");
             r = SC_ERROR_AUTH_METHOD_BLOCKED;
             goto err;
    } else {
        r = sc_check_sw(card, apdu.sw1, apdu.sw2);
    }

err:
    if (apdu.resp)
        free(apdu.resp);
    if (data) {
        /* do not free the functions parameter chat */
        data->chat = NULL;
        PACE_MSE_SET_AT_C_free(data);
    }
    if (d)
        free(d);

    return r;
}

static int pace_gen_auth_1_encrypted_nonce(struct sm_ctx *oldpacectx,
        sc_card_t *card, u8 **enc_nonce, size_t *enc_nonce_len)
{
    sc_apdu_t apdu;
    PACE_GEN_AUTH_C *c_data = NULL;
    PACE_GEN_AUTH_R *r_data = NULL;
    unsigned char *d = NULL, *p;
    int r, l;

    memset(&apdu, 0, sizeof apdu);
    apdu.cla = 0x10;
    apdu.ins = 0x86;
    apdu.cse = SC_APDU_CASE_4_SHORT;
    apdu.flags = SC_APDU_FLAGS_NO_GET_RESP|SC_APDU_FLAGS_NO_RETRY_WL;

    c_data = PACE_GEN_AUTH_C_new();
    if (!c_data) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    r = i2d_PACE_GEN_AUTH_C(c_data, &d);
    if (r < 0) {
        r = SC_ERROR_INTERNAL;
        goto err;
    }
    apdu.data = (const u8 *) d;
    apdu.datalen = r;
    apdu.lc = r;

    if (card->ctx->debug > SC_LOG_TYPE_DEBUG)
        bin_log(card->ctx, "General authenticate (Encrypted Nonce) command data", apdu.data, apdu.datalen);

    apdu.resplen = maxresp;
    apdu.resp = malloc(apdu.resplen);
    if (oldpacectx)
        r = sm_transmit_apdu(oldpacectx, card, &apdu);
    else
        r = sc_transmit_apdu(card, &apdu);
    if (r < 0)
        goto err;

    r = sc_check_sw(card, apdu.sw1, apdu.sw2);
    if (r < 0)
        goto err;

    if (card->ctx->debug > SC_LOG_TYPE_DEBUG)
        bin_log(card->ctx, "General authenticate (Encrypted Nonce) response data", apdu.resp, apdu.resplen);

    if (!d2i_PACE_GEN_AUTH_R(&r_data,
                (const unsigned char **) &apdu.resp, apdu.resplen)) {
        sc_error(card->ctx, "Could not parse general authenticate response data.");
        r = SC_ERROR_INTERNAL;
        goto err;
    }

    if (!r_data->enc_nonce
            || r_data->mapping_data
            || r_data->eph_pub_key
            || r_data->auth_token
            || r_data->cur_car
            || r_data->prev_car) {
        sc_error(card->ctx, "Response data of general authenticate for "
                "step 1 should (only) contain the encrypted nonce.");
        r = SC_ERROR_UNKNOWN_DATA_RECEIVED;
        goto err;
    }
    p = r_data->enc_nonce->data;
    l = r_data->enc_nonce->length;

    *enc_nonce = malloc(l);
    if (!*enc_nonce) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    /* Flawfinder: ignore */
    memcpy(*enc_nonce, p, l);
    *enc_nonce_len = l;

err:
    if (c_data) {
        /* XXX
        if (c_data->mapping_data)
            ASN1_OCTET_STRING_free(c_data->mapping_data);
        if (c_data->eph_pub_key)
            ASN1_OCTET_STRING_free(c_data->eph_pub_key);
        if (c_data->auth_token)
            ASN1_OCTET_STRING_free(c_data->auth_token);*/
        PACE_GEN_AUTH_C_free(c_data);
    }
    if (d)
        free(d);
    if (r_data) {
        /* XXX
        if (r_data->mapping_data)
            ASN1_OCTET_STRING_free(r_data->mapping_data);
        if (r_data->eph_pub_key)
            ASN1_OCTET_STRING_free(r_data->eph_pub_key);
        if (r_data->auth_token)
            ASN1_OCTET_STRING_free(r_data->auth_token);*/
        PACE_GEN_AUTH_R_free(r_data);
    }
    /* XXX */
    /*if (apdu.resp)*/
        /*free(apdu.resp);*/

    return r;
}
static int pace_gen_auth_2_map_nonce(struct sm_ctx *oldpacectx,
        sc_card_t *card, const u8 *in, size_t in_len, u8 **map_data_out,
        size_t *map_data_out_len)
{
    sc_apdu_t apdu;
    PACE_GEN_AUTH_C *c_data = NULL;
    PACE_GEN_AUTH_R *r_data = NULL;
    unsigned char *d = NULL, *p;
    int r, l;

    memset(&apdu, 0, sizeof apdu);
    apdu.cla = 0x10;
    apdu.ins = 0x86;
    apdu.cse = SC_APDU_CASE_4_SHORT;
    apdu.flags = SC_APDU_FLAGS_NO_GET_RESP|SC_APDU_FLAGS_NO_RETRY_WL;

    c_data = PACE_GEN_AUTH_C_new();
    if (!c_data) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    c_data->mapping_data = ASN1_OCTET_STRING_new();
    if (!c_data->mapping_data
            || !M_ASN1_OCTET_STRING_set(
                c_data->mapping_data, in, in_len)) {
        r = SC_ERROR_INTERNAL;
        goto err;
    }
    r = i2d_PACE_GEN_AUTH_C(c_data, &d);
    if (r < 0) {
        r = SC_ERROR_INTERNAL;
        goto err;
    }
    apdu.data = (const u8 *) d;
    apdu.datalen = r;
    apdu.lc = r;

    if (card->ctx->debug > SC_LOG_TYPE_DEBUG)
        bin_log(card->ctx, "General authenticate (Map Nonce) command data", apdu.data, apdu.datalen);

    apdu.resplen = maxresp;
    apdu.resp = malloc(apdu.resplen);
    if (oldpacectx)
        r = sm_transmit_apdu(oldpacectx, card, &apdu);
    else
        r = sc_transmit_apdu(card, &apdu);
    if (r < 0)
        goto err;

    r = sc_check_sw(card, apdu.sw1, apdu.sw2);
    if (r < 0)
        goto err;

    if (card->ctx->debug > SC_LOG_TYPE_DEBUG)
        bin_log(card->ctx, "General authenticate (Map Nonce) response data", apdu.resp, apdu.resplen);

    if (!d2i_PACE_GEN_AUTH_R(&r_data,
                (const unsigned char **) &apdu.resp, apdu.resplen)) {
        sc_error(card->ctx, "Could not parse general authenticate response data.");
        r = SC_ERROR_INTERNAL;
        goto err;
    }

    if (r_data->enc_nonce
            || !r_data->mapping_data
            || r_data->eph_pub_key
            || r_data->auth_token
            || r_data->cur_car
            || r_data->prev_car) {
        sc_error(card->ctx, "Response data of general authenticate for "
                "step 2 should (only) contain the mapping data.");
        r = SC_ERROR_UNKNOWN_DATA_RECEIVED;
        goto err;
    }
    p = r_data->mapping_data->data;
    l = r_data->mapping_data->length;

    *map_data_out = malloc(l);
    if (!*map_data_out) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    /* Flawfinder: ignore */
    memcpy(*map_data_out, p, l);
    *map_data_out_len = l;

err:
    if (c_data) {
        /* XXX
        if (c_data->mapping_data)
            ASN1_OCTET_STRING_free(c_data->mapping_data);
        if (c_data->eph_pub_key)
            ASN1_OCTET_STRING_free(c_data->eph_pub_key);
        if (c_data->auth_token)
            ASN1_OCTET_STRING_free(c_data->auth_token);*/
        PACE_GEN_AUTH_C_free(c_data);
    }
    if (d)
        free(d);
    if (r_data) {
        /* XXX
        if (r_data->mapping_data)
            ASN1_OCTET_STRING_free(r_data->mapping_data);
        if (r_data->eph_pub_key)
            ASN1_OCTET_STRING_free(r_data->eph_pub_key);
        if (r_data->auth_token)
            ASN1_OCTET_STRING_free(r_data->auth_token);*/
        PACE_GEN_AUTH_R_free(r_data);
    }
    /* XXX */
    /*if (apdu.resp)*/
        /*free(apdu.resp);*/

    return r;
}
static int pace_gen_auth_3_perform_key_agreement(
        struct sm_ctx *oldpacectx, sc_card_t *card,
        const u8 *in, size_t in_len, u8 **eph_pub_key_out, size_t *eph_pub_key_out_len)
{
    sc_apdu_t apdu;
    PACE_GEN_AUTH_C *c_data = NULL;
    PACE_GEN_AUTH_R *r_data = NULL;
    unsigned char *d = NULL, *p;
    int r, l;

    memset(&apdu, 0, sizeof apdu);
    apdu.cla = 0x10;
    apdu.ins = 0x86;
    apdu.cse = SC_APDU_CASE_4_SHORT;
    apdu.flags = SC_APDU_FLAGS_NO_GET_RESP|SC_APDU_FLAGS_NO_RETRY_WL;

    c_data = PACE_GEN_AUTH_C_new();
    if (!c_data) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    c_data->eph_pub_key = ASN1_OCTET_STRING_new();
    if (!c_data->eph_pub_key
            || !M_ASN1_OCTET_STRING_set(
                c_data->eph_pub_key, in, in_len)) {
        r = SC_ERROR_INTERNAL;
        goto err;
    }
    r = i2d_PACE_GEN_AUTH_C(c_data, &d);
    if (r < 0) {
        r = SC_ERROR_INTERNAL;
        goto err;
    }
    apdu.data = (const u8 *) d;
    apdu.datalen = r;
    apdu.lc = r;

    if (card->ctx->debug > SC_LOG_TYPE_DEBUG)
        bin_log(card->ctx, "General authenticate (Perform Key Agreement) command data", apdu.data, apdu.datalen);

    apdu.resplen = maxresp;
    apdu.resp = malloc(apdu.resplen);
    if (oldpacectx)
        r = sm_transmit_apdu(oldpacectx, card, &apdu);
    else
        r = sc_transmit_apdu(card, &apdu);
    if (r < 0)
        goto err;

    r = sc_check_sw(card, apdu.sw1, apdu.sw2);
    if (r < 0)
        goto err;

    if (card->ctx->debug > SC_LOG_TYPE_DEBUG)
        bin_log(card->ctx, "General authenticate (Perform Key Agreement) response data", apdu.resp, apdu.resplen);

    if (!d2i_PACE_GEN_AUTH_R(&r_data,
                (const unsigned char **) &apdu.resp, apdu.resplen)) {
        sc_error(card->ctx, "Could not parse general authenticate response data.");
        r = SC_ERROR_INTERNAL;
        goto err;
    }

    if (r_data->enc_nonce
            || r_data->mapping_data
            || !r_data->eph_pub_key
            || r_data->auth_token
            || r_data->cur_car
            || r_data->prev_car) {
        sc_error(card->ctx, "Response data of general authenticate for "
                "step 3 should (only) contain the ephemeral public key.");
        r = SC_ERROR_UNKNOWN_DATA_RECEIVED;
        goto err;
    }
    p = r_data->eph_pub_key->data;
    l = r_data->eph_pub_key->length;

    *eph_pub_key_out = malloc(l);
    if (!*eph_pub_key_out) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    /* Flawfinder: ignore */
    memcpy(*eph_pub_key_out, p, l);
    *eph_pub_key_out_len = l;

err:
    if (c_data) {
        /* XXX
        if (c_data->mapping_data)
            ASN1_OCTET_STRING_free(c_data->mapping_data);
        if (c_data->eph_pub_key)
            ASN1_OCTET_STRING_free(c_data->eph_pub_key);
        if (c_data->auth_token)
            ASN1_OCTET_STRING_free(c_data->auth_token);*/
        PACE_GEN_AUTH_C_free(c_data);
    }
    if (d)
        free(d);
    if (r_data) {
        /* XXX
        if (r_data->mapping_data)
            ASN1_OCTET_STRING_free(r_data->mapping_data);
        if (r_data->eph_pub_key)
            ASN1_OCTET_STRING_free(r_data->eph_pub_key);
        if (r_data->auth_token)
            ASN1_OCTET_STRING_free(r_data->auth_token);*/
        PACE_GEN_AUTH_R_free(r_data);
    }
    /* XXX */
    /*if (apdu.resp)*/
        /*free(apdu.resp);*/

    return r;
}
static int pace_gen_auth_4_mutual_authentication(
        struct sm_ctx *oldpacectx, sc_card_t *card,
        const u8 *in, size_t in_len, u8 **auth_token_out,
        size_t *auth_token_out_len, u8 **recent_car, size_t *recent_car_len,
        u8 **prev_car, size_t *prev_car_len)
{
    sc_apdu_t apdu;
    PACE_GEN_AUTH_C *c_data = NULL;
    PACE_GEN_AUTH_R *r_data = NULL;
    unsigned char *d = NULL, *p;
    int r, l;

    memset(&apdu, 0, sizeof apdu);
    apdu.cla = 0x10;
    apdu.ins = 0x86;
    apdu.cse = SC_APDU_CASE_4_SHORT;
    apdu.flags = SC_APDU_FLAGS_NO_GET_RESP|SC_APDU_FLAGS_NO_RETRY_WL;

    c_data = PACE_GEN_AUTH_C_new();
    if (!c_data) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    apdu.cla = 0;
    c_data->auth_token = ASN1_OCTET_STRING_new();
    if (!c_data->auth_token
            || !M_ASN1_OCTET_STRING_set(
                c_data->auth_token, in, in_len)) {
        r = SC_ERROR_INTERNAL;
        goto err;
    }
    r = i2d_PACE_GEN_AUTH_C(c_data, &d);
    if (r < 0) {
        r = SC_ERROR_INTERNAL;
        goto err;
    }
    apdu.data = (const u8 *) d;
    apdu.datalen = r;
    apdu.lc = r;

    if (card->ctx->debug > SC_LOG_TYPE_DEBUG)
        bin_log(card->ctx, "General authenticate (Perform Key Agreement) command data", apdu.data, apdu.datalen);

    apdu.resplen = maxresp;
    apdu.resp = malloc(apdu.resplen);
    if (oldpacectx)
        r = sm_transmit_apdu(oldpacectx, card, &apdu);
    else
        r = sc_transmit_apdu(card, &apdu);
    if (r < 0)
        goto err;

    r = sc_check_sw(card, apdu.sw1, apdu.sw2);
    if (r < 0)
        goto err;

    if (card->ctx->debug > SC_LOG_TYPE_DEBUG)
        bin_log(card->ctx, "General authenticate (Perform Key Agreement) response data", apdu.resp, apdu.resplen);

    if (!d2i_PACE_GEN_AUTH_R(&r_data,
                (const unsigned char **) &apdu.resp, apdu.resplen)) {
        sc_error(card->ctx, "Could not parse general authenticate response data.");
        r = SC_ERROR_INTERNAL;
        goto err;
    }

    if (r_data->enc_nonce
            || r_data->mapping_data
            || r_data->eph_pub_key
            || !r_data->auth_token) {
        sc_error(card->ctx, "Response data of general authenticate for "
                "step 4 should (only) contain the authentication token.");
        r = SC_ERROR_UNKNOWN_DATA_RECEIVED;
        goto err;
    }
    p = r_data->auth_token->data;
    l = r_data->auth_token->length;
    /* XXX CAR sould be returned as result in some way */
    if (r_data->cur_car) {
        bin_log(card->ctx, "Most recent Certificate Authority Reference",
                r_data->cur_car->data, r_data->cur_car->length);
        *recent_car = malloc(r_data->cur_car->length);
        if (!*recent_car) {
            r = SC_ERROR_OUT_OF_MEMORY;
            goto err;
        }
        /* Flawfinder: ignore */
        memcpy(*recent_car, r_data->cur_car->data, r_data->cur_car->length);
        *recent_car_len = r_data->cur_car->length;
    } else
        *recent_car_len = 0;
    if (r_data->prev_car) {
        bin_log(card->ctx, "Previous Certificate Authority Reference",
                r_data->prev_car->data, r_data->prev_car->length);
        *prev_car = malloc(r_data->prev_car->length);
        if (!*prev_car) {
            r = SC_ERROR_OUT_OF_MEMORY;
            goto err;
        }
        /* Flawfinder: ignore */
        memcpy(*prev_car, r_data->prev_car->data, r_data->prev_car->length);
        *prev_car_len = r_data->prev_car->length;
    } else
        *prev_car_len = 0;

    *auth_token_out = malloc(l);
    if (!*auth_token_out) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    /* Flawfinder: ignore */
    memcpy(*auth_token_out, p, l);
    *auth_token_out_len = l;

err:
    if (c_data) {
        /* XXX
        if (c_data->mapping_data)
            ASN1_OCTET_STRING_free(c_data->mapping_data);
        if (c_data->eph_pub_key)
            ASN1_OCTET_STRING_free(c_data->eph_pub_key);
        if (c_data->auth_token)
            ASN1_OCTET_STRING_free(c_data->auth_token);*/
        PACE_GEN_AUTH_C_free(c_data);
    }
    if (d)
        free(d);
    if (r_data) {
        /* XXX
        if (r_data->mapping_data)
            ASN1_OCTET_STRING_free(r_data->mapping_data);
        if (r_data->eph_pub_key)
            ASN1_OCTET_STRING_free(r_data->eph_pub_key);
        if (r_data->auth_token)
            ASN1_OCTET_STRING_free(r_data->auth_token);*/
        PACE_GEN_AUTH_R_free(r_data);
    }
    /* XXX */
    /*if (apdu.resp)*/
        /*free(apdu.resp);*/

    return r;
}

int
pace_reset_retry_counter(struct sm_ctx *ctx, sc_card_t *card,
        enum s_type pin_id, int ask_for_secret,
        const char *new, size_t new_len)
{
    sc_apdu_t apdu;
    char *p = NULL;
    int r;

    if (ask_for_secret && (!new || !new_len)) {
        p = malloc(MAX_PIN_LEN+1);
        if (!p) {
            sc_error(card->ctx, "Not enough memory for new PIN.\n");
            return SC_ERROR_OUT_OF_MEMORY;
        }
        if (0 > EVP_read_pw_string_min(p,
                    MIN_PIN_LEN, MAX_PIN_LEN+1,
                    "Please enter your new PIN: ", 0)) {
            sc_error(card->ctx, "Could not read new PIN.\n");
            free(p);
            return SC_ERROR_INTERNAL;
        }
        new_len = strnlen(p, MAX_PIN_LEN+1);
        new = p;
    }

    memset(&apdu, 0, sizeof apdu);
    apdu.ins = 0x2C;
    apdu.p2 = pin_id;
    apdu.data = (u8 *) new;
    apdu.datalen = new_len;
    apdu.lc = apdu.datalen;
    apdu.flags = SC_APDU_FLAGS_NO_GET_RESP|SC_APDU_FLAGS_NO_RETRY_WL;

    if (new_len) {
        apdu.p1 = 0x02;
        apdu.cse = SC_APDU_CASE_3_SHORT;
    } else {
        apdu.p1 = 0x03;
        apdu.cse = SC_APDU_CASE_1;
    }

    r = sm_transmit_apdu(ctx, card, &apdu);

    if (p) {
        OPENSSL_cleanse(p, new_len);
        free(p);
    }

    return r;
}

static PACE_SEC *
get_psec(sc_card_t *card, const char *pin, size_t length_pin, enum s_type pin_id)
{
    char *p = NULL;
    PACE_SEC *r;
    int sc_result;
    /* Flawfinder: ignore */
    char buf[MAX_MRZ_LEN > 32 ? MAX_MRZ_LEN : 32];

    if (!length_pin || !pin) {
        if (0 > snprintf(buf, sizeof buf, "Please enter your %s: ",
                    pace_secret_name(pin_id))) {
            sc_error(card->ctx, "Could not create password prompt.\n");
            return NULL;
        }
        p = malloc(MAX_MRZ_LEN);
        if (!p) {
            sc_error(card->ctx, "Not enough memory for %s.\n",
                    pace_secret_name(pin_id));
            return NULL;
        }
        if (0 > EVP_read_pw_string_min(p, 0, MAX_MRZ_LEN, buf, 0)) {
            sc_error(card->ctx, "Could not read %s.\n",
                    pace_secret_name(pin_id));
            return NULL;
        }
        length_pin = strnlen(p, MAX_MRZ_LEN);
        pin = p;
    }

    r = PACE_SEC_new(pin, length_pin, pin_id);

    if (p) {
        OPENSSL_cleanse(p, length_pin);
        free(p);
    }

    return r;
}

void ssl_error(sc_context_t *ctx) {
    unsigned long r;
    ERR_load_crypto_strings();
    for (r = ERR_get_error(); r; r = ERR_get_error()) {
        sc_error(ctx, ERR_error_string(r, NULL));
    }
    ERR_free_strings();
}

int EstablishPACEChannel(struct sm_ctx *oldpacectx, sc_card_t *card,
        struct establish_pace_channel_input pace_input,
        struct establish_pace_channel_output *pace_output,
        struct sm_ctx *sctx)
{
    u8 *p = NULL;
    PACEInfo *info = NULL;
    PACEDomainParameterInfo *static_dp = NULL, *eph_dp = NULL;
    BUF_MEM *enc_nonce = NULL, *nonce = NULL, *mdata = NULL, *mdata_opp = NULL,
            *k_enc = NULL, *k_mac = NULL, *token_opp = NULL,
            *token = NULL, *pub = NULL, *pub_opp = NULL, *key = NULL,
            *comp_pub = NULL, *comp_pub_opp = NULL, *hash_cert_desc = NULL;
    PACE_SEC *sec = NULL;
    PACE_CTX *pctx = NULL;
    CVC_CHAT *chat = NULL;
    BIO *bio_stdout = NULL;
    int r;

    if (!card)
        return SC_ERROR_CARD_NOT_PRESENT;
    if (!pace_output || !sctx)
        return SC_ERROR_INVALID_ARGUMENTS;

    /* show description in advance to give the user more time to read it...
     * This behaviour differs from TR-03119 v1.1 p. 44. */
    if (pace_input.certificate_description_length &&
            pace_input.certificate_description) {

        if (!bio_stdout)
            bio_stdout = BIO_new_fp(stdout, BIO_NOCLOSE);
        if (!bio_stdout) {
            sc_error(card->ctx, "Could not create output buffer.");
            ssl_error(card->ctx);
            r = SC_ERROR_INTERNAL;
            goto err;
        }

        printf("Certificate Description\n");
        switch(certificate_description_print(bio_stdout,
                    pace_input.certificate_description,
                    pace_input.certificate_description_length, "\t")) {
            case -1:
                sc_error(card->ctx, "Could not print certificate description.");
                ssl_error(card->ctx);
                r = SC_ERROR_INTERNAL;
                goto err;
            case 0:
                break;
            case 1:
                sc_error(card->ctx, "Certificate description in "
                        "HTML format can not (yet) be handled.");
                r = SC_ERROR_NOT_SUPPORTED;
                goto err;
            case 2:
                sc_error(card->ctx, "Certificate description in "
                        "PDF format can not (yet) be handled.");
                r = SC_ERROR_NOT_SUPPORTED;
                goto err;
            default:
                sc_error(card->ctx, "Certificate description in "
                        "unknown format can not (yet) be handled.");
                r = SC_ERROR_NOT_SUPPORTED;
                goto err;
        }

        hash_cert_desc = PACE_hash_certificate_description(
                pace_input.certificate_description,
                pace_input.certificate_description_length);
        if (!hash_cert_desc) {
            sc_error(card->ctx, "Could not hash certificate description.");
            ssl_error(card->ctx);
            r = SC_ERROR_INTERNAL;
            goto err;
        }

        p = realloc(pace_output->hash_cert_desc, hash_cert_desc->length);
        if (!p) {
            sc_error(card->ctx, "Not enough memory for hash of certificate description.\n");
            r = SC_ERROR_OUT_OF_MEMORY;
            goto err;
        }
        pace_output->hash_cert_desc = p;
        pace_output->hash_cert_desc_len = hash_cert_desc->length;
    }

    /* show chat in advance to give the user more time to read it...
     * This behaviour differs from TR-03119 v1.1 p. 44. */
    if (pace_input.chat_length && pace_input.chat) {

        if (!bio_stdout)
            bio_stdout = BIO_new_fp(stdout, BIO_NOCLOSE);
        if (!bio_stdout) {
            sc_error(card->ctx, "Could not create output buffer.");
            ssl_error(card->ctx);
            r = SC_ERROR_INTERNAL;
            goto err;
        }

        if (!d2i_CVC_CHAT(&chat, (const unsigned char **) &pace_input.chat,
                    pace_input.chat_length)) {
            sc_error(card->ctx, "Could not parse card holder authorization template (CHAT).");
            r = SC_ERROR_INTERNAL;
            goto err;
        }

        printf("Card holder authorization template (CHAT)\n");
        if (!cvc_chat_print(bio_stdout, chat, "\t")) {
            sc_error(card->ctx, "Could not print card holder authorization template (CHAT).");
            ssl_error(card->ctx);
            r = SC_ERROR_INTERNAL;
            goto err;
        }
    }

    if (!pace_output->ef_cardaccess_length || !pace_output->ef_cardaccess) {
        r = get_ef_card_access(card, &pace_output->ef_cardaccess,
                &pace_output->ef_cardaccess_length);
        if (r < 0) {
            sc_error(card->ctx, "Could not get EF.CardAccess.");
            goto err;
        }
    }
    bin_log(card->ctx, "EF.CardAccess", pace_output->ef_cardaccess,
            pace_output->ef_cardaccess_length);

    if (!parse_ef_card_access(pace_output->ef_cardaccess,
                pace_output->ef_cardaccess_length, &info, &static_dp)) {
        sc_error(card->ctx, "Could not parse EF.CardAccess.");
        ssl_error(card->ctx);
        r = SC_ERROR_INTERNAL;
        goto err;
    }

    r = pace_mse_set_at(oldpacectx, card, info->protocol, pace_input.pin_id,
            chat, &pace_output->mse_set_at_sw1, &pace_output->mse_set_at_sw2);
    if (r < 0) {
        sc_error(card->ctx, "Could not select protocol proberties "
                "(MSE: Set AT failed).");
        goto err;
    }
    enc_nonce = BUF_MEM_new();
    if (!enc_nonce) {
        r = SC_ERROR_INTERNAL;
        goto err;
    }
    r = pace_gen_auth_1_encrypted_nonce(oldpacectx, card, (u8 **) &enc_nonce->data,
            &enc_nonce->length);
    if (r < 0) {
        sc_error(card->ctx, "Could not get encrypted nonce from card "
                "(General Authenticate step 1 failed).");
        goto err;
    }
    if (card->ctx->debug >= SC_LOG_TYPE_DEBUG)
        bin_log(card->ctx, "Encrypted nonce from MRTD", (u8 *)enc_nonce->data, enc_nonce->length);
    enc_nonce->max = enc_nonce->length;

    sec = get_psec(card, (char *) pace_input.pin, pace_input.pin_length,
            pace_input.pin_id);
    if (!sec) {
        sc_error(card->ctx, "Could not encode PACE secret.");
        ssl_error(card->ctx);
        r = SC_ERROR_INTERNAL;
        goto err;
    }
    pctx = PACE_CTX_new();
    if (!pctx || !PACE_CTX_init(pctx, info)) {
        r = SC_ERROR_INTERNAL;
        goto err;
    }
    pctx->tr_version = PACE_TR_VERSION_2_01;

    nonce = PACE_STEP2_dec_nonce(sec, enc_nonce, pctx);

    mdata_opp = BUF_MEM_new();
    mdata = PACE_STEP3A_generate_mapping_data(static_dp, pctx);
    if (!nonce || !mdata || !mdata_opp) {
        sc_error(card->ctx, "Could not generate mapping data.");
        ssl_error(card->ctx);
        r = SC_ERROR_INTERNAL;
        goto err;
    }
    r = pace_gen_auth_2_map_nonce(oldpacectx, card, (u8 *) mdata->data, mdata->length,
            (u8 **) &mdata_opp->data, &mdata_opp->length);
    if (r < 0) {
        sc_error(card->ctx, "Could not exchange mapping data with card "
                "(General Authenticate step 2 failed).");
        goto err;
    }
    mdata_opp->max = mdata_opp->length;
    if (card->ctx->debug >= SC_LOG_TYPE_DEBUG)
        bin_log(card->ctx, "Mapping data from MRTD", (u8 *) mdata_opp->data, mdata_opp->length);

    eph_dp = PACE_STEP3A_map_dp(static_dp, pctx, nonce, mdata_opp);
    pub = PACE_STEP3B_generate_ephemeral_key(eph_dp, pctx);
    pub_opp = BUF_MEM_new();
    if (!eph_dp || !pub || !pub_opp) {
        sc_error(card->ctx, "Could not generate ephemeral domain parameter or "
                "ephemeral key pair.");
        ssl_error(card->ctx);
        r = SC_ERROR_INTERNAL;
        goto err;
    }
    r = pace_gen_auth_3_perform_key_agreement(oldpacectx, card, (u8 *) pub->data, pub->length,
            (u8 **) &pub_opp->data, &pub_opp->length);
    if (r < 0) {
        sc_error(card->ctx, "Could not exchange ephemeral public key with card "
                "(General Authenticate step 3 failed).");
        goto err;
    }
    pub_opp->max = pub_opp->length;
    if (card->ctx->debug >= SC_LOG_TYPE_DEBUG)
        bin_log(card->ctx, "Ephemeral public key from MRTD", (u8 *) pub_opp->data, pub_opp->length);

    key = PACE_STEP3B_compute_ephemeral_key(eph_dp, pctx, pub_opp);
    if (!key ||
            !PACE_STEP3C_derive_keys(key, pctx, info, &k_mac, &k_enc)) {
        sc_error(card->ctx, "Could not compute ephemeral shared secret or "
                "derive keys for encryption and authentication.");
        ssl_error(card->ctx);
        r = SC_ERROR_INTERNAL;
        goto err;
    }
    token = PACE_STEP3D_compute_authentication_token(pctx,
            eph_dp, info, pub_opp, k_mac);
    token_opp = BUF_MEM_new();
    if (!token || !token_opp) {
        sc_error(card->ctx, "Could not compute authentication token.");
        ssl_error(card->ctx);
        r = SC_ERROR_INTERNAL;
        goto err;
    }
    r = pace_gen_auth_4_mutual_authentication(oldpacectx, card, (u8 *) token->data, token->length,
            (u8 **) &token_opp->data, &token_opp->length,
            &pace_output->recent_car, &pace_output->recent_car_length,
            &pace_output->previous_car, &pace_output->previous_car_length);

    if (r < 0) {
        sc_error(card->ctx, "Could not exchange authentication token with card "
                "(General Authenticate step 4 failed).");
        goto err;
    }
    token_opp->max = token_opp->length;

    if (!PACE_STEP3D_verify_authentication_token(pctx,
            eph_dp, info, k_mac, token_opp)) {
        sc_error(card->ctx, "Could not verify authentication token.");
        ssl_error(card->ctx);
        r = SC_ERROR_INTERNAL;
        goto err;
    }

    /* Identifier for ICC and PCD */
    comp_pub = PACE_Comp(eph_dp, pctx, pub);
    comp_pub_opp = PACE_Comp(eph_dp, pctx, pub_opp);
    if (!comp_pub || !comp_pub_opp) {
        sc_error(card->ctx, "Could not compress public keys for identification.");
        ssl_error(card->ctx);
        r = SC_ERROR_INTERNAL;
        goto err;
    }
    p = realloc(pace_output->id_icc, comp_pub_opp->length);
    if (!p) {
        sc_error(card->ctx, "Not enough memory for ID ICC.\n");
        return SC_ERROR_OUT_OF_MEMORY;
    }
    pace_output->id_icc = p;
    pace_output->id_icc_length = comp_pub_opp->length;
    /* Flawfinder: ignore */
    memcpy(pace_output->id_icc, comp_pub_opp->data, comp_pub_opp->length);
    bin_log(card->ctx, "ID ICC", pace_output->id_icc,
            pace_output->id_icc_length);
    p = realloc(pace_output->id_pcd, comp_pub->length);
    if (!p) {
        sc_error(card->ctx, "Not enough memory for ID PCD.\n");
        return SC_ERROR_OUT_OF_MEMORY;
    }
    pace_output->id_pcd = p;
    pace_output->id_pcd_length = comp_pub->length;
    /* Flawfinder: ignore */
    memcpy(pace_output->id_pcd, comp_pub->data, comp_pub->length);
    bin_log(card->ctx, "ID PCD", pace_output->id_pcd,
            pace_output->id_pcd_length);

    sctx->authentication_ctx = pace_sm_ctx_create(k_mac,
            k_enc, pctx);
    if (!sctx->authentication_ctx) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    r = reset_ssc(sctx->authentication_ctx);
    if (r < 0)
        goto err;
    sctx->cipher_ctx = sctx->authentication_ctx;
    sctx->authenticate = pace_sm_authenticate;
    sctx->encrypt = pace_sm_encrypt;
    sctx->decrypt = pace_sm_decrypt;
    sctx->verify_authentication = pace_sm_verify_authentication;
    sctx->pre_transmit = pace_sm_pre_transmit;
    sctx->post_transmit = pace_sm_post_transmit;
    sctx->padding_indicator = SM_ISO_PADDING;
    sctx->block_length = EVP_CIPHER_block_size(pctx->cipher);
    sctx->active = 1;

err:
    if (info)
        PACEInfo_free(info);
    if (static_dp)
        PACEDomainParameterInfo_clear_free(static_dp);
    if (eph_dp)
        PACEDomainParameterInfo_clear_free(eph_dp);
    if (enc_nonce)
        BUF_MEM_free(enc_nonce);
    if (nonce) {
        OPENSSL_cleanse(nonce->data, nonce->length);
        BUF_MEM_free(nonce);
    }
    if (mdata)
        BUF_MEM_free(mdata);
    if (mdata_opp)
        BUF_MEM_free(mdata_opp);
    if (token_opp)
        BUF_MEM_free(token_opp);
    if (token)
        BUF_MEM_free(token);
    if (pub)
        BUF_MEM_free(pub);
    if (pub_opp)
        BUF_MEM_free(pub_opp);
    if (comp_pub_opp)
        BUF_MEM_free(comp_pub_opp);
    if (comp_pub)
        BUF_MEM_free(comp_pub);
    if (hash_cert_desc)
        BUF_MEM_free(hash_cert_desc);
    if (key) {
        OPENSSL_cleanse(key->data, key->length);
        BUF_MEM_free(key);
    }
    if (sec)
        PACE_SEC_clean_free(sec);
    if (bio_stdout)
        BIO_free_all(bio_stdout);

    if (r < 0) {
        if (k_enc) {
            OPENSSL_cleanse(k_enc->data, k_enc->length);
            BUF_MEM_free(k_enc);
        }
        if (k_mac) {
            OPENSSL_cleanse(k_mac->data, k_mac->length);
            BUF_MEM_free(k_mac);
        }
        if (pctx)
            PACE_CTX_clear_free(pctx);
        if (sctx->authentication_ctx)
            pace_sm_ctx_free(sctx->authentication_ctx);
    }

    return r;
}

static const char *MRZ_name = "MRZ";
static const char *PIN_name = "PIN";
static const char *PUK_name = "PUK";
static const char *CAN_name = "CAN";
static const char *UNDEF_name = "UNDEF";
const char *pace_secret_name(enum s_type pin_id) {
    switch (pin_id) {
        case PACE_MRZ:
            return MRZ_name;
        case PACE_PUK:
            return PUK_name;
        case PACE_PIN:
            return PIN_name;
        case PACE_CAN:
            return CAN_name;
        default:
            return UNDEF_name;
    }
}

int
increment_ssc(struct pace_sm_ctx *psmctx)
{
    if (!psmctx)
        return SC_ERROR_INVALID_ARGUMENTS;

    BN_add_word(psmctx->ssc, 1);

    return SC_SUCCESS;
}

int
decrement_ssc(struct pace_sm_ctx *psmctx)
{
    if (!psmctx)
        return SC_ERROR_INVALID_ARGUMENTS;

    BN_sub_word(psmctx->ssc, 1);

    return SC_SUCCESS;
}

int
reset_ssc(struct pace_sm_ctx *psmctx)
{
    if (!psmctx)
        return SC_ERROR_INVALID_ARGUMENTS;

    BN_zero(psmctx->ssc);

    return SC_SUCCESS;
}

static int
pace_sm_encrypt(sc_card_t *card, const struct sm_ctx *ctx,
        const u8 *data, size_t datalen, u8 **enc)
{
    BUF_MEM *encbuf = NULL, *databuf = NULL;
    u8 *p = NULL;
    int r;

    if (!ctx || !enc || !ctx->cipher_ctx) {
        r = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }
    struct pace_sm_ctx *psmctx = ctx->cipher_ctx;

    databuf = BUF_MEM_create_init(data, datalen);
    encbuf = PACE_encrypt(psmctx->ctx, psmctx->ssc, psmctx->key_enc, databuf);
    if (!databuf || !encbuf) {
        sc_error(card->ctx, "Could not encrypt data.");
        ssl_error(card->ctx);
        r = SC_ERROR_INTERNAL;
        goto err;
    }

    p = realloc(*enc, encbuf->length);
    if (!p) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    *enc = p;
    /* Flawfinder: ignore */
    memcpy(*enc, encbuf->data, encbuf->length);
    r = encbuf->length;

err:
    if (databuf) {
        OPENSSL_cleanse(databuf->data, databuf->max);
        BUF_MEM_free(databuf);
    }
    if (encbuf)
        BUF_MEM_free(encbuf);

    return r;
}

static int
pace_sm_decrypt(sc_card_t *card, const struct sm_ctx *ctx,
        const u8 *enc, size_t enclen, u8 **data)
{
    BUF_MEM *encbuf = NULL, *databuf = NULL;
    u8 *p = NULL;
    int r;

    if (!ctx || !enc || !ctx->cipher_ctx || !data) {
        r = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }
    struct pace_sm_ctx *psmctx = ctx->cipher_ctx;

    encbuf = BUF_MEM_create_init(enc, enclen);
    databuf = PACE_decrypt(psmctx->ctx, psmctx->ssc, psmctx->key_enc, encbuf);
    if (!encbuf || !databuf) {
        sc_error(card->ctx, "Could not decrypt data.");
        ssl_error(card->ctx);
        r = SC_ERROR_INTERNAL;
        goto err;
    }

    p = realloc(*data, databuf->length);
    if (!p) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    *data = p;
    /* Flawfinder: ignore */
    memcpy(*data, databuf->data, databuf->length);
    r = databuf->length;

err:
    if (databuf) {
        OPENSSL_cleanse(databuf->data, databuf->max);
        BUF_MEM_free(databuf);
    }
    if (encbuf)
        BUF_MEM_free(encbuf);

    return r;
}

static int
pace_sm_authenticate(sc_card_t *card, const struct sm_ctx *ctx,
        const u8 *data, size_t datalen, u8 **macdata)
{
    BUF_MEM *macbuf = NULL;
    u8 *p = NULL, *ssc = NULL;
    int r;

    if (!ctx || !ctx->cipher_ctx || !macdata) {
        r = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }
    struct pace_sm_ctx *psmctx = ctx->cipher_ctx;

    macbuf = PACE_authenticate(psmctx->ctx, psmctx->ssc, psmctx->key_mac,
            data, datalen);
    if (!macbuf) {
        sc_error(card->ctx, "Could not compute message authentication code "
                "(MAC).");
        ssl_error(card->ctx);
        r = SC_ERROR_INTERNAL;
        goto err;
    }

    p = realloc(*macdata, macbuf->length);
    if (!p) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    *macdata = p;
    /* Flawfinder: ignore */
    memcpy(*macdata, macbuf->data, macbuf->length);
    r = macbuf->length;

err:
    if (macbuf)
        BUF_MEM_free(macbuf);
    if (ssc)
        free(ssc);

    return r;
}

static int
pace_sm_verify_authentication(sc_card_t *card, const struct sm_ctx *ctx,
        const u8 *mac, size_t maclen,
        const u8 *macdata, size_t macdatalen)
{
    int r;
    char *p;
    BUF_MEM *my_mac = NULL;

    if (!ctx || !ctx->cipher_ctx) {
        r = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }
    struct pace_sm_ctx *psmctx = ctx->cipher_ctx;

    my_mac = PACE_authenticate(psmctx->ctx, psmctx->ssc, psmctx->key_mac,
            macdata, macdatalen);
    if (!my_mac) {
        sc_error(card->ctx, "Could not compute message authentication code "
                "(MAC) for verification.");
        ssl_error(card->ctx);
        r = SC_ERROR_INTERNAL;
        goto err;
    }

    if (my_mac->length != maclen ||
            memcmp(my_mac->data, mac, maclen) != 0) {
        r = SC_ERROR_OBJECT_NOT_VALID;
        sc_error(card->ctx, "Authentication data not verified");
        goto err;
    }

    if (card->ctx->debug > SC_LOG_TYPE_DEBUG)
        sc_debug(card->ctx, "Authentication data verified");

    r = SC_SUCCESS;

err:
    if (my_mac)
        BUF_MEM_free(my_mac);

    return r;
}

static int
pace_sm_pre_transmit(sc_card_t *card, const struct sm_ctx *ctx,
        sc_apdu_t *apdu)
{
    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_DEBUG,
            increment_ssc(ctx->cipher_ctx));
}

static int pace_sm_post_transmit(sc_card_t *card, const struct sm_ctx *ctx,
        sc_apdu_t *sm_apdu)
{
    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_DEBUG,
            increment_ssc(ctx->cipher_ctx));
}
