/*
 * Copyright (C) 2010 Frank Morgner
 *
 * This file is part of npa.
 *
 * npa is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * npa is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * npa.  If not, see <http://www.gnu.org/licenses/>.
 */
/**
 * @file
 * @defgroup sm Secure Messaging (SM)
 * @{
 */
#ifndef _CCID_SM_H
#define _CCID_SM_H

#include <libopensc/opensc.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Padding indicator: use ISO/IEC 9797-1 padding method 2 */
#define SM_ISO_PADDING 0x01
/** Padding indicator: use no padding */
#define SM_NO_PADDING  0x02

/** Secure messaging context */
struct sm_ctx {
    /** 1 if secure messaging is activated, 0 otherwise */
    unsigned char active;

    /** data of the specific crypto implementation */
    void *priv_data;

    /** Padding-content indicator byte (ISO 7816-4 Table 30) */
    u8 padding_indicator;
    /** Pad to this block length */
    size_t block_length;

    /** Call back function for authentication of data */
    int (*authenticate)(sc_card_t *card, const struct sm_ctx *ctx,
            const u8 *data, size_t datalen, u8 **outdata);
    /** Call back function for verifying authentication data */
    int (*verify_authentication)(sc_card_t *card, const struct sm_ctx *ctx,
            const u8 *mac, size_t maclen,
            const u8 *macdata, size_t macdatalen);

    /** Call back function for encryption of data */
    int (*encrypt)(sc_card_t *card, const struct sm_ctx *ctx,
            const u8 *data, size_t datalen, u8 **enc);
    /** Call back function for decryption of data */
    int (*decrypt)(sc_card_t *card, const struct sm_ctx *ctx,
            const u8 *enc, size_t enclen, u8 **data);

    /** Call back function for actions before encoding and encryption of \a apdu */
    int (*pre_transmit)(sc_card_t *card, const struct sm_ctx *ctx,
            sc_apdu_t *apdu);
    /** Call back function for actions before decryption and decoding of \a sm_apdu */
    int (*post_transmit)(sc_card_t *card, const struct sm_ctx *ctx,
            sc_apdu_t *sm_apdu);
    /** Call back function for actions after decrypting SM protected APDU */
    int (*finish)(sc_card_t *card, const struct sm_ctx *ctx,
            sc_apdu_t *apdu);

    /** Clears and frees private data */
    void (*clear_free)(const struct sm_ctx *ctx);
};

/** 
 * @brief Secure messaging wrapper for sc_transmit_apdu()
 *
 * If secure messaging (SM) is activated in \a sctx and \a apdu is not already
 * SM protected, \a apdu is processed with the following steps:
 * \li call to \a sctx->pre_transmit
 * \li encrypt \a apdu calling \a sctx->encrypt
 * \li authenticate \a apdu calling \a sctx->authenticate
 * \li transmit SM protected \a apdu
 * \li verify SM protected \a apdu calling \a sctx->verify_authentication
 * \li decrypt SM protected \a apdu calling \a sctx->decrypt
 * \li copy decrypted/authenticated data and status bytes to \a apdu
 * 
 * @param[in]     sctx (optional)
 * @param[in]     card
 * @param[in,out] apdu
 * 
 * @return \c SC_SUCCESS or error code if an error occurred
 */
int sm_transmit_apdu(struct sm_ctx *sctx, sc_card_t *card,
        sc_apdu_t *apdu);

/** 
 * @brief Clears and frees private data of the SM context
 *
 * Calls \a sctx->clear_free
 * 
 * @param[in]     sctx (optional)
 */
void sm_ctx_clear_free(const struct sm_ctx *sctx);

#ifdef  __cplusplus
}
#endif
#endif
/* @} */
