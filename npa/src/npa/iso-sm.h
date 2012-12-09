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
 * @defgroup sm Interface to Secure Messaging (SM) defined in ISO 7816
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
struct iso_sm_ctx {
    /** data of the specific crypto implementation */
    void *priv_data;

    /** Padding-content indicator byte (ISO 7816-4 Table 30) */
    u8 padding_indicator;
    /** Pad to this block length */
    size_t block_length;

    /** Call back function for authentication of data */
    int (*authenticate)(sc_card_t *card, const struct iso_sm_ctx *ctx,
            const u8 *data, size_t datalen, u8 **outdata);
    /** Call back function for verifying authentication data */
    int (*verify_authentication)(sc_card_t *card, const struct iso_sm_ctx *ctx,
            const u8 *mac, size_t maclen,
            const u8 *macdata, size_t macdatalen);

    /** Call back function for encryption of data */
    int (*encrypt)(sc_card_t *card, const struct iso_sm_ctx *ctx,
            const u8 *data, size_t datalen, u8 **enc);
    /** Call back function for decryption of data */
    int (*decrypt)(sc_card_t *card, const struct iso_sm_ctx *ctx,
            const u8 *enc, size_t enclen, u8 **data);

    /** Call back function for actions before encoding and encryption of \a apdu */
    int (*pre_transmit)(sc_card_t *card, const struct iso_sm_ctx *ctx,
            sc_apdu_t *apdu);
    /** Call back function for actions before decryption and decoding of \a sm_apdu */
    int (*post_transmit)(sc_card_t *card, const struct iso_sm_ctx *ctx,
            sc_apdu_t *sm_apdu);
    /** Call back function for actions after decrypting SM protected APDU */
    int (*finish)(sc_card_t *card, const struct iso_sm_ctx *ctx,
            sc_apdu_t *apdu);

    /** Clears and frees private data */
    void (*clear_free)(const struct iso_sm_ctx *ctx);
};

/** 
 * @brief Clears and frees the SM context including private data
 *
 * Calls \a sctx->clear_free() if available
 * 
 * @param[in]     sctx (optional)
 */
void iso_sm_ctx_clear_free(struct iso_sm_ctx *sctx);

/**
 * @brief Creates a SM context
 *
 * @return SM context or NULL if an error occurred
 */
struct iso_sm_ctx *iso_sm_ctx_create(void);

/**
 * @brief Initializes a card for usage of the ISO SM driver
 *
 * If a SM module has been assigned previously to the card, it will be cleaned
 * up.
 *
 * @param[in] card
 * @param[in] sctx will NOT be freed automatically. \a sctx should be present
 * for the time of the SM session.
 *
 * @return \c SC_SUCCESS or error code if an error occurred
 */
int iso_sm_start(struct sc_card *card, struct iso_sm_ctx *sctx);

/**
 * @brief Stops SM and frees allocated ressources.
 *
 * @param[in] card
 *
 * @return \c SC_SUCCESS or error code if an error occurred
 */
int sm_stop(struct sc_card *card);

#ifdef  __cplusplus
}
#endif
#endif
/* @} */
