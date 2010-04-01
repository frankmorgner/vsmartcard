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
#ifndef _SM_H
#define _SM_H

#include <opensc/opensc.h>
#include <openssl/evp.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SM_ISO_PADDING 0x01
#define SM_NO_PADDING  0x02

struct sm_ctx {
    u8 padding_indicator;
    u8 *key_mac;
    size_t key_mac_len;
    u8 *key_enc;
    size_t key_enc_len;

    const EVP_CIPHER *cipher;
    EVP_CIPHER_CTX *cipher_ctx;
    ENGINE *cipher_engine;
    unsigned char *iv;

    const EVP_MD * md;
    EVP_MD_CTX * md_ctx;
    ENGINE *md_engine;
};

int sm_encrypt(const struct sm_ctx *ctx, sc_card_t *card,
        const sc_apdu_t *apdu, sc_apdu_t *sm_apdu);
int sm_decrypt(const struct sm_ctx *ctx, sc_card_t *card,
        const sc_apdu_t *sm_apdu, sc_apdu_t *apdu);

#ifdef  __cplusplus
}
#endif
#endif
