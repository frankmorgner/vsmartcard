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
#include <openssl/buffer.h>
#include <openssl/pace.h>

struct pace_sm_ctx *
pace_sm_ctx_create(int protocol, const BUF_MEM *key_mac,
        const BUF_MEM *key_enc, PACE_CTX *ctx)
{
    struct pace_sm_ctx *out = malloc(sizeof *out);
    if (!out)
        return NULL;

    out->ssc = BN_new();
    if (!out->ssc) {
        free(out);
        return NULL;
    }

    out->key_enc = key_enc;
    out->key_mac = key_mac;
    out->protocol = protocol;
    out->ctx = ctx;

    return out;
}

void
pace_sm_ctx_free(struct pace_sm_ctx *ctx)
{
    if (ctx) {
        if (ctx->ssc)
            BN_clear_free(ctx->ssc);
        free(ctx);
    }
}

void
pace_sm_ctx_clear_free(struct pace_sm_ctx *ctx)
{
    if (ctx) {
        if (ctx->key_mac) {
            OPENSSL_cleanse(ctx->key_mac->data, ctx->key_mac->max);
            BUF_MEM_free((BUF_MEM *) ctx->key_mac);
        }
        if (ctx->key_enc) {
            OPENSSL_cleanse(ctx->key_enc->data, ctx->key_enc->max);
            BUF_MEM_free((BUF_MEM *) ctx->key_enc);
        }
        PACE_CTX_clear_free(ctx->ctx);
        pace_sm_ctx_free(ctx);
    }
}
