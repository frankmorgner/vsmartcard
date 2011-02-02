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
/**
 * @file
 */
#ifndef _CCID_NPA_LIB_H
#define _CCID_NPA_LIB_H

#include <openssl/pace.h>

#ifdef __cplusplus
extern "C" {
#endif

/** NPA secure messaging context */
struct npa_sm_ctx {
    /** Send sequence counter */
    BIGNUM *ssc;
    /** Key for message authentication code */
    const BUF_MEM *key_mac;
    /** Key for encryption and decryption */
    const BUF_MEM *key_enc;
    /** PACE context */
    PACE_CTX *ctx;
};


/** 
 * @brief Creates a NPA SM object
 * 
 * @param[in] key_mac Key for message authentication code
 * @param[in] key_enc Key for encryption and decryption
 * @param[in] ctx     NPA context
 * 
 * @return Initialized NPA SM object or NULL, if an error occurred
 */
struct npa_sm_ctx * npa_sm_ctx_create(const BUF_MEM *key_mac,
        const BUF_MEM *key_enc, PACE_CTX *ctx);

/** 
 * @brief Frees a NPA SM object
 *
 * Frees memory allocated for \a ctx and its send sequence counter
 * 
 * @param[in] ctx object to be freed
 */
void npa_sm_ctx_free(struct npa_sm_ctx *ctx);

/** 
 * @brief Frees a NPA SM object and all its components
 *
 * Frees memory allocated for \a ctx and its send sequence counter, keys and PACE context
 * 
 * @param[in] ctx object to be freed
 */
void npa_sm_ctx_clear_free(struct npa_sm_ctx *ctx);

#ifdef  __cplusplus
}
#endif
#endif
