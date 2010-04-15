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

int sm_transmit_apdu(const struct sm_ctx *sctx, sc_card_t *card,
        sc_apdu_t *apdu);

int add_iso_pad(const u8 *data, size_t datalen, int block_size, u8 **padded);
int add_padding(const struct sm_ctx *ctx, const u8 *data, size_t datalen,
        u8 **padded);

#define bin_print(file, label, data, len) \
    _bin_log(NULL, 0, NULL, 0, NULL, label, data, len, file)
#define bin_log(ctx, label, data, len) \
    _bin_log(ctx, SC_LOG_TYPE_DEBUG, __FILE__, __LINE__, __FUNCTION__, label, data, len, NULL)
void _bin_log(sc_context_t *ctx, int type, const char *file, int line,
        const char *func, const char *label, const u8 *data, size_t len,
        FILE *f);

#ifdef  __cplusplus
}
#endif
#endif
