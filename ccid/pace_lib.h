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
#ifndef _CCID_PACE_LIB_H
#define _CCID_PACE_LIB_H

#include <openssl/pace.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pace_sm_ctx {
    BIGNUM *ssc;
    const BUF_MEM *key_mac;
    const BUF_MEM *key_enc;
    PACE_CTX *ctx;
};


struct pace_sm_ctx * pace_sm_ctx_create(const BUF_MEM *key_mac,
        const BUF_MEM *key_enc, PACE_CTX *ctx);
void pace_sm_ctx_free(struct pace_sm_ctx *ctx);
void pace_sm_ctx_clear_free(struct pace_sm_ctx *ctx);

#ifdef  __cplusplus
}
#endif
#endif
