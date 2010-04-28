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
#include "ccid.h"
#include <stdio.h>
#include <opensc/log.h>
#include <openssl/evp.h>
#include <openssl/asn1.h>
#include <openssl/asn1t.h>
#include <openssl/buffer.h>


#define ASN1_APP_EXP_OPT(stname, field, type, tag) ASN1_EX_TYPE(ASN1_TFLG_EXPTAG|ASN1_TFLG_APPLICATION|ASN1_TFLG_OPTIONAL, tag, stname, field, type)

/*
 * MSE:Set AT
 */

typedef struct pace_mse_set_at_cd_st {
    ASN1_OBJECT *cryptographic_mechanism_reference;
    ASN1_INTEGER *key_reference1;
    ASN1_INTEGER *key_reference2;
    ASN1_OCTET_STRING *auxiliary_data;
    ASN1_OCTET_STRING *eph_pub_key;
    ASN1_OCTET_STRING *cha_template;
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
    ASN1_APP_EXP_OPT(PACE_MSE_SET_AT_C, auxiliary_data, ASN1_OCTET_STRING, 7),
    /* 0x91
     * Ephemeral Public Key */
    ASN1_IMP_OPT(PACE_MSE_SET_AT_C, eph_pub_key, ASN1_OCTET_STRING, 0x11),
    /* 0x7F4C
     * Certificate Holder Authorization Template */
    ASN1_APP_EXP_OPT(PACE_MSE_SET_AT_C, cha_template, ASN1_OCTET_STRING, 0x4c),
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
    ASN1_OCTET_STRING *cert_auth1;
    ASN1_OCTET_STRING *cert_auth2;
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
     * Certification Authority Reference */
    ASN1_IMP_OPT(PACE_GEN_AUTH_R_BODY, cert_auth1, ASN1_OCTET_STRING, 7),
    /* 0x88
     * Certification Authority Reference */
    ASN1_IMP_OPT(PACE_GEN_AUTH_R_BODY, cert_auth2, ASN1_OCTET_STRING, 8),
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


#ifdef NO_PACE
inline int GetReadersPACECapabilities(sc_card_t *card,
        const __u8 *in, __u8 **out, size_t *outlen)
{
    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_ERROR, SC_ERROR_NOT_SUPPORTED);
}
inline int EstablishPACEChannel(sc_card_t *card, const __u8 *in,
        __u8 **out, size_t *outlen, struct sm_ctx *sm_ctx)
{
    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_ERROR, SC_ERROR_NOT_SUPPORTED);
}
int pace_test(sc_card_t *card,
        enum s_type pin_id, const char *pin, size_t pinlen,
        enum s_type new_pin_id, const char *new_pin, size_t new_pinlen)
{
    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_ERROR, SC_ERROR_NOT_SUPPORTED);
}
int pace_sm_encrypt(sc_card_t *card, struct *sm_ctx,
            const u8 *data, size_t datalen, u8 **enc)
{
    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_ERROR, SC_ERROR_NOT_SUPPORTED);
}
int pace_sm_decrypt(sc_card_t *card, struct *sm_ctx,
            const u8 *enc, size_t enclen, u8 **data)
{
    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_ERROR, SC_ERROR_NOT_SUPPORTED);
}
int pace_sm_authenticate(sc_card_t *card, const struct sm_ctx *ctx,
        const u8 *data, size_t datalen, u8 **macdata)
{
    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_ERROR, SC_ERROR_NOT_SUPPORTED);
}
int pace_sm_verify_authentication(sc_card_t *card, struct sm_ctx *ctx,
        const u8 *mac, size_t maclen,
        const u8 *macdata, size_t macdatalen)
{
    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_ERROR, SC_ERROR_NOT_SUPPORTED);
}
int pace_change_p(struct sm_ctx *ctx, sc_card_t *card, enum s_type pin_id,
        const char *newp, size_t newplen)
{
    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_ERROR, SC_ERROR_NOT_SUPPORTED);
}
#else

#include <asm/byteorder.h>
#include <opensc/asn1.h>
#include <opensc/opensc.h>
#include <opensc/ui.h>
#include <openssl/err.h>
#include <openssl/objects.h>
#include <openssl/pace.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const size_t maxresp = SC_MAX_APDU_BUFFER_SIZE - 2;

int GetReadersPACECapabilities(sc_card_t *card,
        const __u8 *in, __u8 **out, size_t *outlen) {
    if (!out || !outlen)
        SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_DEBUG, SC_ERROR_INVALID_ARGUMENTS);

    __u8 *result = realloc(*out, 2);
    if (!result)
        SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_DEBUG, SC_ERROR_OUT_OF_MEMORY);
    *out = result;
    *outlen = 2;

    /* lengthBitMap */
    *result = 1;
    result++;
    /* BitMap */
    *result = PACE_BITMAP_PACE|PACE_BITMAP_EID;

    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_DEBUG, SC_SUCCESS);
}

/** select and read EF.CardAccess */
static int get_ef_card_access(sc_card_t *card,
        __u8 **ef_cardaccess, size_t *length_ef_cardaccess)
{
    int r;
    sc_path_t path;
    sc_file_t *file = NULL;
    __u8 *p;

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
        p = realloc(*ef_cardaccess, *length_ef_cardaccess + maxresp);
        if (!p) {
            r = SC_ERROR_OUT_OF_MEMORY;
            goto err;
        }
        *ef_cardaccess = p;

        r = sc_read_binary(card, *length_ef_cardaccess,
                *ef_cardaccess + *length_ef_cardaccess, maxresp, 0);

        if (r > 0 && r != maxresp) {
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
        r = SC_ERROR_FILE_TOO_SMALL;
        goto err;
    }

    r = SC_SUCCESS;

err:
    if (file) {
        free(file);
    }

    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_DEBUG, r);
}

static int pace_mse_set_at(sc_card_t *card,
        int protocol, int secret_key, int reference)
{
    sc_apdu_t apdu;
    unsigned char *d = NULL;
    PACE_MSE_SET_AT_C *data = NULL;
    int r;

    memset(&apdu, 0, sizeof apdu);
    apdu.ins = 0x22;
    apdu.p1 = 0xc1;
    apdu.p2 = 0xa4;
    apdu.cse = SC_APDU_CASE_3;
    apdu.flags = SC_APDU_FLAGS_NO_GET_RESP|SC_APDU_FLAGS_NO_RETRY_WL;

    data = PACE_MSE_SET_AT_C_new();
    if (!data) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    data->cryptographic_mechanism_reference = OBJ_nid2obj(protocol);
    data->key_reference1 = ASN1_INTEGER_new();
    //data->key_reference2 = ASN1_INTEGER_new();
    if (!data->cryptographic_mechanism_reference
            || !data->key_reference1
            //|| !data->key_reference2
            ) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    if (!ASN1_INTEGER_set(data->key_reference1, secret_key)
            //|| !ASN1_INTEGER_set(data->key_reference2, reference)
            ) {
        r = SC_ERROR_INTERNAL;
        goto err;
    }
    r = i2d_PACE_MSE_SET_AT_C(data, &d);
    if (r < 0) {
        r = SC_ERROR_INTERNAL;
        goto err;
    }
    /* The tag/length for the sequence (0x30) is omitted in the command apdu. */
    /* FIXME is there a OpenSSL way to get the value only or even a define for
     * the tag? */
    apdu.data = sc_asn1_find_tag(card->ctx, d, r, 0x30, &apdu.datalen);
    apdu.lc = apdu.datalen;

    bin_log(card->ctx, "MSE:Set AT command data", apdu.data, apdu.datalen);

    r = sc_transmit_apdu(card, &apdu);
    if (r < 0)
        goto err;

    if (apdu.resplen) {
        sc_error(card->ctx, "MSE:Set AT response data should be empty");
        r = SC_ERROR_UNKNOWN_DATA_RECEIVED;
        goto err;
    }

    if (apdu.sw1 == 0x63) {
        if ((apdu.sw2 & 0xc0) == 0xc0) {
             sc_error(card->ctx, "Verification failed (remaining tries: %d%s)\n",
                   apdu.sw2 & 0x0f,
                   (apdu.sw2 & 0x0f) == 1? ", password must be resumed": (apdu.sw2 & 0x0f) == 0? ", password must be unblocked":
                   "");
             r = SC_ERROR_PIN_CODE_INCORRECT;
             goto err;
        } else {
            sc_error(card->ctx, "Unknown SWs; SW1=%02X, SW2=%02X\n",
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
        // XXX
        //if (data->cryptographic_mechanism_reference)
            //ASN1_OBJECT_free(data->cryptographic_mechanism_reference);
        //if (data->key_reference1)
            //ASN1_INTEGER_free(data->key_reference1);
        //if (data->key_reference2)
            //ASN1_INTEGER_free(data->key_reference2);
        PACE_MSE_SET_AT_C_free(data);
    }
    if (d)
        free(d);

    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_DEBUG, r);
}

static int pace_gen_auth(sc_card_t *card,
        int step, const u8 *in, size_t in_len, u8 **out, size_t *out_len)
{
    sc_apdu_t apdu;
    PACE_GEN_AUTH_C *c_data = NULL;
    PACE_GEN_AUTH_R *r_data = NULL;
    unsigned char *d = NULL, *p;
    int r, l;

    memset(&apdu, 0, sizeof apdu);
    apdu.cla = 0x10;
    apdu.ins = 0x86;
    apdu.p1 = 0;
    apdu.p2 = 0;
    apdu.cse = SC_APDU_CASE_4;
    apdu.flags = SC_APDU_FLAGS_NO_GET_RESP|SC_APDU_FLAGS_NO_RETRY_WL;

    c_data = PACE_GEN_AUTH_C_new();
    if (!c_data) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    switch (step) {
        case 1:
            break;
        case 2:
            c_data->mapping_data = ASN1_OCTET_STRING_new();
            if (!c_data->mapping_data
                    || !M_ASN1_OCTET_STRING_set(
                        c_data->mapping_data, in, in_len)) {
                r = SC_ERROR_INTERNAL;
                goto err;
            }
            break;
        case 3:
            c_data->eph_pub_key = ASN1_OCTET_STRING_new();
            if (!c_data->eph_pub_key
                    || !M_ASN1_OCTET_STRING_set(
                        c_data->eph_pub_key, in, in_len)) {
                r = SC_ERROR_INTERNAL;
                goto err;
            }
            break;
        case 4:
            apdu.cla = 0;
            c_data->auth_token = ASN1_OCTET_STRING_new();
            if (!c_data->auth_token
                    || !M_ASN1_OCTET_STRING_set(
                        c_data->auth_token, in, in_len)) {
                r = SC_ERROR_INTERNAL;
                goto err;
            }
            break;
        default:
            r = SC_ERROR_INVALID_ARGUMENTS;
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

    bin_log(card->ctx, "General authenticate command data", apdu.data, apdu.datalen);

    /* sanity checks in sc_transmit_apdu forbid case 4 apdus with le == 0 */
    apdu.resplen = maxresp;
    apdu.resp = malloc(apdu.resplen);
    r = sc_transmit_apdu(card, &apdu);
    if (r < 0)
        goto err;

    r = sc_check_sw(card, apdu.sw1, apdu.sw2);
    if (r < 0)
        goto err;

    bin_log(card->ctx, "General authenticate response data", apdu.resp, apdu.resplen);

    if (!d2i_PACE_GEN_AUTH_R(&r_data,
                (const unsigned char **) &apdu.resp, apdu.resplen)) {
        sc_error(card->ctx, "Could not parse general authenticate response data.");
        r = SC_ERROR_INTERNAL;
        goto err;
    }

    switch (step) {
        case 1:
            if (!r_data->enc_nonce
                    || r_data->mapping_data
                    || r_data->eph_pub_key
                    || r_data->auth_token) {
                sc_error(card->ctx, "Response data of general authenticate for "
                        "step %d should (only) contain the "
                        "encrypted nonce.", step);
                r = SC_ERROR_UNKNOWN_DATA_RECEIVED;
                goto err;
            }
            p = r_data->enc_nonce->data;
            l = r_data->enc_nonce->length;
            break;
        case 2:
            if (r_data->enc_nonce
                    || !r_data->mapping_data
                    || r_data->eph_pub_key
                    || r_data->auth_token) {
                sc_error(card->ctx, "Response data of general authenticate for "
                        "step %d should (only) contain the "
                        "mapping data.", step);
                r = SC_ERROR_UNKNOWN_DATA_RECEIVED;
                goto err;
            }
            p = r_data->mapping_data->data;
            l = r_data->mapping_data->length;
            break;
        case 3:
            if (r_data->enc_nonce
                    || r_data->mapping_data
                    || !r_data->eph_pub_key
                    || r_data->auth_token) {
                sc_error(card->ctx, "Response data of general authenticate for "
                        "step %d should (only) contain the "
                        "ephemeral public key.", step);
                r = SC_ERROR_UNKNOWN_DATA_RECEIVED;
                goto err;
            }
            p = r_data->eph_pub_key->data;
            l = r_data->eph_pub_key->length;
            break;
        case 4:
            if (r_data->enc_nonce
                    || r_data->mapping_data
                    || r_data->eph_pub_key
                    || !r_data->auth_token) {
                sc_error(card->ctx, "Response data of general authenticate for "
                        "step %d should (only) contain the "
                        "authentication token.", step);
                r = SC_ERROR_UNKNOWN_DATA_RECEIVED;
                goto err;
            }
            p = r_data->auth_token->data;
            l = r_data->auth_token->length;
            break;
        default:
            r = SC_ERROR_INVALID_ARGUMENTS;
            goto err;
    }

    *out = malloc(l);
    if (!*out) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    memcpy(*out, p, l);
    *out_len = l;

err:
    if (c_data) {
        /* FIXME
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
        /* FIXME
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

    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_DEBUG, r);
}

static int
pace_reset_retry_counter(struct sm_ctx *ctx, sc_card_t *card,
        enum s_type pin_id, const char *new, size_t new_len)
{
    sc_apdu_t apdu;

    apdu.cla = 0;
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

    return pace_transmit_apdu(ctx, card, &apdu);
}

static PACE_SEC *
get_psec(sc_card_t *card, const char *pin, size_t length_pin, enum s_type pin_id)
{
    sc_ui_hints_t hints;
    char *p = NULL;
    PACE_SEC *r;
    int sc_result;

    if (!length_pin || !pin) {
        memset(&hints, 0, sizeof(hints));
        hints.dialog_name = "ccid.PACE";
        hints.card = card;
        hints.prompt = NULL;
        hints.obj_label = pace_secret_name(pin_id);
        hints.usage = SC_UI_USAGE_OTHER;
        sc_result = sc_ui_get_pin(&hints, &p);
        if (sc_result < 0) {
            sc_error(card->ctx, "Could not read PACE secret (%s).\n",
                    sc_strerror(sc_result));
            return NULL;
        }
        length_pin = strlen(p);
        pin = p;
    }

    r = PACE_SEC_new(pin, length_pin, pin_id);

    if (p) {
        OPENSSL_cleanse(p, length_pin);
        free(p);
    }

    return r;
}

void debug_ossl(sc_context_t *ctx) {
    unsigned long r;
    for (r = ERR_get_error(); r; r = ERR_get_error()) {
        sc_error(ctx, ERR_error_string(r, NULL));
    }
}

int EstablishPACEChannel(sc_card_t *card, const __u8 *in,
        __u8 **out, size_t *outlen, struct sm_ctx *sctx)
{
    __u8 pin_id;
    size_t length_chat, length_pin, length_cert_desc, length_ef_cardaccess;
    const __u8 *chat, *pin, *certificate_description;
    __u8 *ef_cardaccess = NULL;
    PACEInfo *info = NULL;
    PACEDomainParameterInfo *static_dp = NULL, *eph_dp = NULL;
    BUF_MEM *enc_nonce, *nonce = NULL, *mdata = NULL, *mdata_opp = NULL,
            *k_enc = NULL, *k_mac = NULL, *token_opp = NULL,
            *token = NULL, *pub = NULL, *pub_opp = NULL, *key = NULL;
    PACE_SEC *sec = NULL;
    PACE_CTX *pctx = NULL;
    int r;

    if (!in || !out || !outlen)
        SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_DEBUG, SC_ERROR_INVALID_ARGUMENTS);

    pin_id = *in;
    in++;

    length_chat = *in;
    in++;
    chat = in;
    in += length_chat;

    length_pin = *in;
    in++;
    pin = in;
    in += length_pin;

    length_cert_desc = (__le16_to_cpu((__le16) *in));
    in += sizeof (__le16);
    certificate_description = in;

    enc_nonce = BUF_MEM_new();
    if (!enc_nonce) {
        r = SC_ERROR_INTERNAL;
        debug_ossl(card->ctx);
        goto err;
    }
    r = get_ef_card_access(card, &ef_cardaccess, &length_ef_cardaccess);
    if (r < 0) {
        sc_error(card->ctx, "Could not get EF.CardAccess.");
        goto err;
    }
    bin_log(card->ctx, "EF.CardAccess", ef_cardaccess, length_ef_cardaccess);
    if (!parse_ef_card_access(ef_cardaccess, length_ef_cardaccess,
                &info, &static_dp)) {
        r = SC_ERROR_INTERNAL;
        debug_ossl(card->ctx);
        sc_error(card->ctx, "Could not parse EF.CardAccess.");
        goto err;
    }
    r = pace_mse_set_at(card, info->protocol, pin_id, 1);
    if (r < 0) {
        sc_error(card->ctx, "Could not select protocol proberties "
                "(MSE: Set AT).");
        goto err;
    }
    r = pace_gen_auth(card, 1, NULL, 0, (u8 **) &enc_nonce->data,
            &enc_nonce->length);
    if (r < 0) {
        sc_error(card->ctx, "Could not get encrypted nonce from card "
                "(General Authenticate step 1 failed).");
        goto err;
    }
    bin_log(card->ctx, "Encrypted nonce from MRTD", (u8 *)enc_nonce->data, enc_nonce->length);
    enc_nonce->max = enc_nonce->length;

    sec = get_psec(card, (char *) pin, length_pin, pin_id);
    if (!sec) {
        r = SC_ERROR_INTERNAL;
        debug_ossl(card->ctx);
        goto err;
    }
    pctx = PACE_CTX_new();
    if (!pctx || !PACE_CTX_init(pctx, info)) {
        r = SC_ERROR_INTERNAL;
        debug_ossl(card->ctx);
        goto err;
    }
    pctx->tr_version = PACE_TR_VERSION_2_01;

    nonce = PACE_STEP2_dec_nonce(sec, enc_nonce, pctx);

    mdata_opp = BUF_MEM_new();
    mdata = PACE_STEP3A_generate_mapping_data(static_dp, pctx);
    if (!nonce || !mdata || !mdata_opp) {
        r = SC_ERROR_INTERNAL;
        debug_ossl(card->ctx);
        goto err;
    }
    r = pace_gen_auth(card, 2, (u8 *) mdata->data, mdata->length,
            (u8 **) &mdata_opp->data, &mdata_opp->length);
    if (r < 0) {
        sc_error(card->ctx, "Could not exchange mapping data with card "
                "(General Authenticate step 2 failed).");
        goto err;
    }
    mdata_opp->max = mdata_opp->length;
    bin_log(card->ctx, "Mapping data from MRTD", (u8 *) mdata_opp->data, mdata_opp->length);

    eph_dp = PACE_STEP3A_map_dp(static_dp, pctx, nonce, mdata_opp);
    pub = PACE_STEP3B_generate_ephemeral_key(eph_dp, pctx);
    pub_opp = BUF_MEM_new();
    if (!eph_dp || !pub || !pub_opp) {
        r = SC_ERROR_INTERNAL;
        debug_ossl(card->ctx);
        goto err;
    }
    r = pace_gen_auth(card, 3, (u8 *) pub->data, pub->length,
            (u8 **) &pub_opp->data, &pub_opp->length);
    if (r < 0) {
        sc_error(card->ctx, "Could not exchange ephemeral public key with card "
                "(General Authenticate step 3 failed).");
        goto err;
    }
    pub_opp->max = pub_opp->length;
    bin_log(card->ctx, "Public key from MRTD", (u8 *) pub_opp->data, pub_opp->length);

    key = PACE_STEP3B_compute_ephemeral_key(eph_dp, pctx, pub_opp);
    if (!key ||
            !PACE_STEP3C_derive_keys(key, pctx, info, &k_mac, &k_enc)) {
        r = SC_ERROR_INTERNAL;
        debug_ossl(card->ctx);
        goto err;
    }
    token = PACE_STEP3D_compute_authentication_token(pctx,
            eph_dp, info, pub_opp, k_mac);
    token_opp = BUF_MEM_new();
    if (!token || !token_opp) {
        r = SC_ERROR_INTERNAL;
        debug_ossl(card->ctx);
        goto err;
    }
    r = pace_gen_auth(card, 4, (u8 *) token->data, token->length,
            (u8 **) &token_opp->data, &token_opp->length);
    if (r < 0) {
        sc_error(card->ctx, "Could not exchange authentication token with card "
                "(General Authenticate step 4 failed).");
        goto err;
    }
    token_opp->max = token_opp->length;

    if (!PACE_STEP3D_verify_authentication_token(pctx,
            eph_dp, info, k_mac, token_opp)) {
        r = SC_ERROR_INTERNAL;
        debug_ossl(card->ctx);
        goto err;
    }

    /* XXX parse CHAT to check role of terminal */

    sctx->authenticate = pace_sm_authenticate;
    sctx->encrypt = pace_sm_encrypt;
    sctx->decrypt = pace_sm_decrypt;
    sctx->verify_authentication = pace_sm_verify_authentication;
    sctx->padding_indicator = SM_ISO_PADDING;
    sctx->block_length = EVP_CIPHER_block_size(pctx->cipher);
    sctx->authentication_ctx = pace_sm_ctx_create(k_mac,
            k_enc, pctx);
    sctx->cipher_ctx = sctx->authentication_ctx;
    if (!sctx->authentication_ctx) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    r = reset_ssc(sctx->authentication_ctx);

err:
    if (ef_cardaccess)
        free(ef_cardaccess);
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
    if (key) {
        OPENSSL_cleanse(key->data, key->length);
        BUF_MEM_free(key);
    }
    if (sec)
        PACE_SEC_clean_free(sec);

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

    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_DEBUG, r);
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

int pace_test(sc_card_t *card,
        enum s_type pin_id, const char *pin, size_t pinlen,
        enum s_type new_pin_id, const char *new_pin, size_t new_pinlen)
{
    u8 buf[0xff + 5];
    char *read = NULL;
    __u8 *out = NULL;
    size_t outlen, readlen = 0, linelen, apdulen;
    struct sm_ctx sctx;
    sc_apdu_t apdu;
    int r;

    memset(&sctx, 0, sizeof(sctx));
    memset(&apdu, 0, sizeof(apdu));

    switch (pin_id) {
        case PACE_MRZ:
        case PACE_CAN:
        case PACE_PIN:
        case PACE_PUK:
            break;
        default:
            sc_error(card->ctx, "Type of secret not supported");
            return SC_ERROR_INVALID_ARGUMENTS;
    }

    if (pinlen > sizeof(buf) - 5) {
        sc_error(card->ctx, "%s too long (maximal %u bytes supported)",
                pace_secret_name(pin_id),
                sizeof(buf) - 5);
    }
    buf[0] = pin_id;
    buf[1] = 0;             // length_chat
    buf[2] = pinlen;        // length_pin
    memcpy(&buf[3], pin, pinlen);
    buf[3 + pinlen] = 0;    // length_cert_desc
    buf[4 + pinlen]= 0;     // length_cert_desc

    SC_TEST_RET(card->ctx,
            EstablishPACEChannel(card, buf, &out, &outlen, &sctx),
            "Could not establish PACE channel.");

    if (new_pin_id) {
        SC_TEST_RET(card->ctx,
                pace_change_p(&sctx, card, new_pin_id, new_pin, new_pinlen),
                "Could not change PACE secret.");
    }

    while (1) {
        printf("Enter unencrypted APDU (empty line to exit)\n");

        linelen = getline(&read, &readlen, stdin);
        if (linelen <= 1) {
            if (linelen < 0) {
                r = SC_ERROR_INTERNAL;
                sc_error(card->ctx, "Could not read line");
            } else {
                r = SC_SUCCESS;
                printf("Thanks for flying with ccid\n");
            }
            break;
        }

        read[linelen - 1] = 0;
        if (sc_hex_to_bin(read, buf, &apdulen) < 0) {
            sc_error(card->ctx, "Could not format binary string");
        }

        r = build_apdu(buf, apdulen, &apdu);
        if (r < 0) {
            sc_error(card->ctx, "Could not format APDU");
            continue;
        }

        apdu.resp = buf;
        apdu.resplen = sizeof(buf);

        r = pace_transmit_apdu(&sctx, card, &apdu);
        if (r < 0) {
            sc_error(card->ctx, "Could not send APDU: %s", sc_strerror(r));
            continue;
        }

        printf("Decrypted APDU sw1=%02x sw2=%02x\n", apdu.sw1, apdu.sw2);
        bin_print(stdout, "Decrypted APDU response data", apdu.resp, apdu.resplen);
    }

    if (read)
        free(read);

    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_ERROR, r);
}

static int
encode_ssc(const BIGNUM *ssc, const PACE_CTX *ctx, u8 **encoded)
{
    u8 *p;
    size_t en_len, bn_len;

    if (!ctx)
        return SC_ERROR_INVALID_ARGUMENTS;

    en_len = EVP_CIPHER_block_size(ctx->cipher);
    p = realloc(*encoded, en_len);
    if (!p)
        return SC_ERROR_OUT_OF_MEMORY;
    *encoded = p;

    bn_len = BN_num_bytes(ssc);

    if (bn_len <= en_len) {
        memset(*encoded, 0, en_len - bn_len);
        BN_bn2bin(ssc, *encoded + en_len - bn_len);
    } else {
        p = malloc(bn_len);
        if (!p)
            return SC_ERROR_OUT_OF_MEMORY;
        BN_bn2bin(ssc, p);
        memcpy(*encoded, p + bn_len - en_len, en_len);
        free(p);
    }

    return en_len;
}

static int
update_iv(struct pace_sm_ctx *psmctx)
{
    BUF_MEM *sscbuf = NULL, *ivbuf = NULL;
    const EVP_CIPHER *ivcipher = NULL, *oldcipher;
    u8 *ssc = NULL;
    unsigned char *p;
    int r;

    if (!psmctx)
        return SC_ERROR_INVALID_ARGUMENTS;

    switch (EVP_CIPHER_nid(psmctx->ctx->cipher)) {
        case NID_aes_128_cbc:
            if (!ivcipher)
                ivcipher = EVP_aes_128_ecb();
            /* fall through */
        case NID_aes_192_cbc:
            if (!ivcipher)
                ivcipher = EVP_aes_192_ecb();
            /* fall through */
        case NID_aes_256_cbc:
            if (!ivcipher)
                ivcipher = EVP_aes_256_ecb();

            /* For AES decryption the IV is not needed,
             * so we always set it to the encryption IV=E(K_Enc, SSC) */
            r = encode_ssc(psmctx->ssc, psmctx->ctx, &ssc);
            if (r < 0)
                goto err;
            sscbuf = BUF_MEM_create_init(ssc, r);
            if (!sscbuf) {
                r = SC_ERROR_OUT_OF_MEMORY;
                goto err;
            }
            oldcipher = psmctx->ctx->cipher;
            psmctx->ctx->cipher = ivcipher;
            ivbuf = PACE_encrypt(psmctx->ctx, psmctx->key_enc, sscbuf);
            psmctx->ctx->cipher = oldcipher;
            if (!ivbuf) {
                r = SC_ERROR_INTERNAL;
                goto err;
            }
            p = realloc(psmctx->ctx->iv, ivbuf->length);
            if (!p) {
                r = SC_ERROR_OUT_OF_MEMORY;
                goto err;
            }
            psmctx->ctx->iv = p;
            memcpy(psmctx->ctx->iv, ivbuf->data, ivbuf->length);
            break;
        case NID_des_ede_cbc:
            /* For 3DES encryption or decryption the IV is always NULL */
            free(psmctx->ctx->iv);
            psmctx->ctx->iv = NULL;
            break;
        default:
            r = SC_ERROR_INVALID_ARGUMENTS;
            goto err;
    }

    r = SC_SUCCESS;

err:
    if (ssc)
        free(ssc);
    if (sscbuf)
        BUF_MEM_free(sscbuf);
    if (ivbuf)
        BUF_MEM_free(ivbuf);

    return r;
}

int
increment_ssc(struct pace_sm_ctx *psmctx)
{
    if (!psmctx)
        return SC_ERROR_INVALID_ARGUMENTS;

    BN_add_word(psmctx->ssc, 1);

    return update_iv(psmctx);
}

int
decrement_ssc(struct pace_sm_ctx *psmctx)
{
    if (!psmctx)
        return SC_ERROR_INVALID_ARGUMENTS;

    BN_sub_word(psmctx->ssc, 1);

    return update_iv(psmctx);
}

int
reset_ssc(struct pace_sm_ctx *psmctx)
{
    if (!psmctx)
        return SC_ERROR_INVALID_ARGUMENTS;

    BN_zero(psmctx->ssc);

    return update_iv(psmctx);
}

int pace_sm_encrypt(sc_card_t *card, const struct sm_ctx *ctx,
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

    /* The send sequence counter is extended to the block length of the cipher,
     * so it is no problem that we get padded data */
    databuf = BUF_MEM_create_init(data, datalen);
    encbuf = PACE_encrypt(psmctx->ctx, psmctx->key_enc, databuf);
    if (!databuf || !encbuf) {
        r = SC_ERROR_INTERNAL;
        goto err;
    }

    if (!encbuf) {
        r = SC_ERROR_INTERNAL;
        goto err;
    }

    p = realloc(*enc, encbuf->length);
    if (!p) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    *enc = p;
    memcpy(*enc, encbuf->data, encbuf->length);
    r = encbuf->length;

err:
    if (databuf) {
        OPENSSL_cleanse(databuf->data, databuf->max);
        BUF_MEM_free(databuf);
    }
    if (encbuf)
        BUF_MEM_free(encbuf);

    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_ERROR, r);
}

int pace_sm_decrypt(sc_card_t *card, const struct sm_ctx *ctx,
        const u8 *enc, size_t enclen, u8 **data)
{
    BUF_MEM *encbuf = NULL, *databuf = NULL;
    u8 *p = NULL;
    int r;

    if (!ctx || !enc || !ctx->cipher_ctx) {
        r = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }
    struct pace_sm_ctx *psmctx = ctx->cipher_ctx;

    encbuf = BUF_MEM_create_init(enc, enclen);
    databuf = PACE_decrypt(psmctx->ctx, psmctx->key_enc, encbuf);
    if (!encbuf || !databuf) {
        r = SC_ERROR_INTERNAL;
        goto err;
    }

    p = realloc(*data, databuf->length);
    if (!p) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    *data = p;
    memcpy(*data, databuf->data, databuf->length);
    r = databuf->length;

err:
    if (databuf) {
        OPENSSL_cleanse(databuf->data, databuf->max);
        BUF_MEM_free(databuf);
    }
    if (encbuf)
        BUF_MEM_free(encbuf);

    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_ERROR, r);
}

int pace_sm_authenticate(sc_card_t *card, const struct sm_ctx *ctx,
        const u8 *data, size_t datalen, u8 **macdata)
{
    BUF_MEM *databuf = NULL, *macbuf = NULL;
    u8 *p = NULL, *ssc = NULL;
    int r;

    if (!ctx || !ctx->cipher_ctx) {
        r = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }
    struct pace_sm_ctx *psmctx = ctx->cipher_ctx;

    r = encode_ssc(psmctx->ssc, psmctx->ctx, &ssc);
    if (r < 0) {
        sc_error(card->ctx, "Could not get send sequence counter\n");
        goto err;
    }
    bin_log(card->ctx, "Encoded Send Sequence Counter",
            ssc, r);

    databuf = BUF_MEM_create(r + datalen);
    if (!databuf) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    memcpy(databuf->data, ssc, r);
    memcpy(databuf->data + r, data, datalen);
    databuf->length = r + datalen;
    bin_log(card->ctx, "Data to authenticate (PACE)",
            (u8 *) databuf->data, databuf->length);

    macbuf = PACE_authenticate(psmctx->ctx, psmctx->key_mac, databuf);
    if (!macbuf) {
        sc_error(card->ctx, "Could not get MAC\n");
        r = SC_ERROR_INTERNAL;
        goto err;
    }

    p = realloc(*macdata, macbuf->length);
    if (!p) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    *macdata = p;
    memcpy(*macdata, macbuf->data, macbuf->length);
    r = macbuf->length;

err:
    if (databuf) {
        OPENSSL_cleanse(databuf->data, databuf->max);
        BUF_MEM_free(databuf);
    }
    if (macbuf)
        BUF_MEM_free(macbuf);
    if (ssc)
        free(ssc);

    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_ERROR, r);
}

int pace_sm_verify_authentication(sc_card_t *card, const struct sm_ctx *ctx,
        const u8 *mac, size_t maclen,
        const u8 *macdata, size_t macdatalen)
{
    int r;
    char *p;
    BUF_MEM authdata, *my_mac = NULL;
    authdata.data = NULL;

    if (!ctx || !ctx->cipher_ctx) {
        r = SC_ERROR_INVALID_ARGUMENTS;
        goto incerr;
    }
    struct pace_sm_ctx *psmctx = ctx->cipher_ctx;

    r = increment_ssc(psmctx);
    if (r < 0)
        goto incerr;

    r = encode_ssc(psmctx->ssc, psmctx->ctx, (u8 **) &authdata.data);
    if (r < 0)
        goto err;
    authdata.length = r;

    p = realloc(authdata.data, authdata.length + macdatalen);
    if (!p) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    authdata.data = p;
    memcpy(authdata.data + authdata.length, macdata, macdatalen);
    authdata.length += macdatalen;
    bin_log(card->ctx, "Authentication data to verify (PACE)",
            (u8 *) authdata.data, authdata.length);

    authdata.max = authdata.length;
    my_mac = PACE_authenticate(psmctx->ctx, psmctx->key_mac, &authdata);
    if (!my_mac) {
        r = SC_ERROR_INTERNAL;
        goto err;
    }

    if (my_mac->length != maclen ||
            memcmp(my_mac->data, mac, maclen) != 0) {
        r = SC_ERROR_OBJECT_NOT_VALID;
        sc_debug(card->ctx, "Authentication data not verified");
        goto err;
    }

    sc_debug(card->ctx, "Authentication data verified");

err:
    if (authdata.data)
        free(authdata.data);
    if (my_mac)
        BUF_MEM_free(my_mac);
    if (r >= 0)
        decrement_ssc(psmctx);
    else
        r = decrement_ssc(psmctx);

incerr:
    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_ERROR, r);
}

int pace_transmit_apdu(struct sm_ctx *ctx, sc_card_t *card,
        sc_apdu_t *apdu)
{
    int r;

    if ((apdu->cla & 0x0C) == 0x0C) {
        sc_debug(card->ctx, "Given APDU is already protected with some secure messaging.");
        SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_DEBUG, sc_transmit_apdu(card, apdu));
    }

    if (!ctx || !ctx->cipher_ctx) {
        SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_DEBUG, SC_ERROR_INVALID_ARGUMENTS);
    }

    /* SW1 and SW2 are used to determine if really something has been sent and
     * received. Only if this is the case, the send sequence counter needs to
     * be incremented. */

    apdu->sw1 = 0;
    apdu->sw2 = 0;

    SC_TEST_RET(card->ctx, increment_ssc(ctx->cipher_ctx),
            "Could not increment send sequence counter");

    r = sm_transmit_apdu(ctx, card, apdu);

    if (apdu->sw1 || apdu->sw2) {
        if (r < 0) {
            if (increment_ssc(ctx->cipher_ctx) < 0)
                sc_error(card->ctx,
                        "Could not increment send sequence counter");
        } else
            r = increment_ssc(ctx->cipher_ctx);
    } else
        if (decrement_ssc(ctx->cipher_ctx) < 0)
            sc_error(card->ctx,
                    "Could not decrement send sequence counter");

    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_DEBUG, r);
}

int pace_change_p(struct sm_ctx *ctx, sc_card_t *card, enum s_type pin_id,
        const char *newp, size_t newplen)
{
    sc_ui_hints_t hints;
    char *p = NULL;
    int r;

    if (!newplen || !newp) {
        memset(&hints, 0, sizeof(hints));
        hints.dialog_name = "ccid.PACE";
        hints.card = card;
        hints.prompt = NULL;
        hints.obj_label = pace_secret_name(pin_id);
        hints.usage = SC_UI_USAGE_NEW_PIN;
        r = sc_ui_get_pin(&hints, &p);
        if (r < 0) {
            sc_error(card->ctx, "Could not read new %s (%s).\n",
                    hints.obj_label, sc_strerror(r));
            SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_ERROR, r);
        }
        newplen = strlen(p);
        newp = p;
    }

    r = pace_reset_retry_counter(ctx, card, pin_id, newp, newplen);

    if (p) {
        OPENSSL_cleanse(p, newplen);
        free(p);
    }

    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_DEBUG, r);
}

#endif
