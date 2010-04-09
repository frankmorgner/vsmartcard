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
#ifndef _CCID_SM_H
#define _CCID_SM_H

#include <opensc/opensc.h>
#include <openssl/buffer.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SM_ISO_PADDING 0x01
#define SM_NO_PADDING  0x02

struct sm_ctx {
    u8 padding_indicator;
    size_t block_length;


    void *authentication_ctx;
    int (*authenticate)(sc_card_t *card, const struct sm_ctx *ctx,
            const u8 *data, size_t datalen, u8 **outdata);
    int (*verify_authentication)(sc_card_t *card, const struct sm_ctx *ctx,
            const u8 *mac, size_t maclen,
            const u8 *macdata, size_t macdatalen);

    void *cipher_ctx;
    int (*encrypt)(sc_card_t *card, const struct sm_ctx *ctx,
            const u8 *data, size_t datalen, u8 **enc);
    int (*decrypt)(sc_card_t *card, const struct sm_ctx *ctx,
            const u8 *enc, size_t enclen, u8 **data);
};

int sm_encrypt(const struct sm_ctx *ctx, sc_card_t *card,
        const sc_apdu_t *apdu, sc_apdu_t *sm_apdu);
int sm_decrypt(const struct sm_ctx *ctx, sc_card_t *card,
        const sc_apdu_t *sm_apdu, sc_apdu_t *apdu);

BUF_MEM * add_iso_pad(const BUF_MEM * m, int block_size);
BUF_MEM * add_padding(const struct sm_ctx *ctx, const char *data, size_t datalen);

void bin_log(sc_context_t *ctx, const char *label, const u8 *data, size_t len);

#ifdef  __cplusplus
}
#endif
#endif
