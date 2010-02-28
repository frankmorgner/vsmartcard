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
#ifndef _PACE_H
#define _PACE_H

#include <linux/usb/ch9.h>
#include <openssl/asn1.h>
#include <openssl/asn1t.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PACE_BITMAP_PACE  0x40
#define PACE_BITMAP_EID   0x20
#define PACE_BITMAP_ESIGN 0x10

#define PACE_MRZ 0x01
#define PACE_CAN 0x02
#define PACE_PIN 0x03
#define PACE_PUK 0x04

#define FID_EF_CARDACCESS 0x011C

#define ASN1_APP_EXP_OPT(stname, field, type, tag) ASN1_EX_TYPE(ASN1_TFLG_EXPTAG|ASN1_TFLG_APPLICATION|ASN1_TFLG_OPTIONAL, tag, stname, field, type)


/*
 * MSE:Set AT
 */

typedef struct pace_mse_set_at_cd_st {
    ASN1_OBJECT *cryptographic_mechanism_reference;
    ASN1_INTEGER *key_reference1;
    ASN1_INTEGER *key_reference2;
    ASN1_INTEGER *auxiliary_data;
    ASN1_INTEGER *eph_pub_key;
    ASN1_INTEGER *cha_template;
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
    ASN1_APP_EXP_OPT(PACE_MSE_SET_AT_C, auxiliary_data, ASN1_INTEGER, 7),
    /* 0x91
     * Ephemeral Public Key */
    ASN1_IMP_OPT(PACE_MSE_SET_AT_C, eph_pub_key, ASN1_INTEGER, 0x11),
    /* 0x7F4C
     * Certificate Holder Authorization Template */
    ASN1_APP_EXP_OPT(PACE_MSE_SET_AT_C, cha_template, ASN1_INTEGER, 0x4c),
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
} PACE_GEN_AUTH_C;
ASN1_SEQUENCE(PACE_GEN_AUTH_C) = {
    /* 0x81
     * Mapping Data */
    ASN1_IMP_OPT(PACE_GEN_AUTH_C, mapping_data, ASN1_INTEGER, 1),
    /* 0x83
     * Ephemeral Public Key */
    ASN1_IMP_OPT(PACE_GEN_AUTH_C, eph_pub_key, ASN1_INTEGER, 3),
    /* 0x85
     * Authentication Token */
    ASN1_IMP_OPT(PACE_GEN_AUTH_C, auth_token, ASN1_INTEGER, 5),
} ASN1_SEQUENCE_END(PACE_GEN_AUTH_C)
IMPLEMENT_ASN1_FUNCTIONS(PACE_GEN_AUTH_C)

/* Protocol Response Data */
typedef struct pace_gen_auth_rapdu_body_st {
    ASN1_INTEGER *enc_nonce;
    ASN1_INTEGER *mapping_data;
    ASN1_INTEGER *eph_pub_key;
    ASN1_INTEGER *auth_token;
    ASN1_INTEGER *cert_auth1;
    ASN1_INTEGER *cert_auth2;
} PACE_GEN_AUTH_R_BODY;
ASN1_SEQUENCE(PACE_GEN_AUTH_R_BODY) = {
    /* 0x80
     * Encrypted Nonce */
    ASN1_IMP_OPT(PACE_GEN_AUTH_R_BODY, enc_nonce, ASN1_INTEGER, 0),
    /* 0x82
     * Mapping Data */
    ASN1_IMP_OPT(PACE_GEN_AUTH_R_BODY, mapping_data, ASN1_INTEGER, 2),
    /* 0x84
     * Ephemeral Public Key */
    ASN1_IMP_OPT(PACE_GEN_AUTH_R_BODY, eph_pub_key, ASN1_INTEGER, 4),
    /* 0x86
     * Authentication Token */
    ASN1_IMP_OPT(PACE_GEN_AUTH_R_BODY, auth_token, ASN1_INTEGER, 6),
    /* 0x87
     * Certification Authority Reference */
    ASN1_IMP_OPT(PACE_GEN_AUTH_R_BODY, cert_auth1, ASN1_INTEGER, 7),
    /* 0x88
     * Certification Authority Reference */
    ASN1_IMP_OPT(PACE_GEN_AUTH_R_BODY, cert_auth2, ASN1_INTEGER, 8),
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


int GetReadersPACECapabilities(sc_context_t *ctx, sc_card_t *card, const __u8
        *in, __u8 **out, size_t *outlen);
int EstablishPACEChannel(sc_context_t *ctx, sc_card_t *card, const __u8 *in,
        __u8 **out, size_t *outlen);

#ifdef  __cplusplus
}
#endif
#endif
