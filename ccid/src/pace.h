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
#ifndef _CCID_PACE_H
#define _CCID_PACE_H

#include "pace_lib.h"
#include "sm.h"
#include <linux/usb/ch9.h>
#include <opensc/opensc.h>
#include <openssl/bn.h>
#include <openssl/pace.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PACE_BITMAP_PACE  0x40
#define PACE_BITMAP_EID   0x20
#define PACE_BITMAP_ESIGN 0x10

//#define PACE_MRZ 0x01
//#define PACE_CAN 0x02
//#define PACE_PIN 0x03
//#define PACE_PUK 0x04

#define FID_EF_CARDACCESS 0x011C

#define MAX_EF_CARDACCESS 2048
#define MAX_PIN_LEN       6
#define MIN_PIN_LEN       6
#define MAX_MRZ_LEN       128

int increment_ssc(struct pace_sm_ctx *psmctx);
int decrement_ssc(struct pace_sm_ctx *psmctx);
int reset_ssc(struct pace_sm_ctx *psmctx);

const char *pace_secret_name(enum s_type pin_id);

int pace_sm_encrypt(sc_card_t *card, const struct sm_ctx *ctx,
        const u8 *data, size_t datalen, u8 **enc);
int pace_sm_decrypt(sc_card_t *card, const struct sm_ctx *ctx,
        const u8 *enc, size_t enclen, u8 **data);
int pace_sm_authenticate(sc_card_t *card, const struct sm_ctx *ctx,
        const u8 *data, size_t datalen, u8 **outdata);
int pace_sm_verify_authentication(sc_card_t *card, const struct sm_ctx *ctx,
        const u8 *mac, size_t maclen,
        const u8 *macdata, size_t macdatalen);
int pace_sm_pre_transmit(sc_card_t *card, const struct sm_ctx *ctx,
        sc_apdu_t *apdu);
int pace_sm_post_transmit(sc_card_t *card, const struct sm_ctx *ctx,
        sc_apdu_t *sm_apdu);


struct establish_pace_channel_input {
    unsigned char pin_id;

    size_t chat_length;
    const unsigned char *chat;

    size_t pin_length;
    const unsigned char *pin;

    size_t certificate_description_length;
    const unsigned char *certificate_description;
};

struct establish_pace_channel_output {
    unsigned char mse_set_at_sw1;
    unsigned char mse_set_at_sw2;

    size_t ef_cardaccess_length;
    unsigned char *ef_cardaccess;

    size_t recent_car_length;
    unsigned char *recent_car;

    size_t previous_car_length;
    unsigned char *previous_car;

    size_t id_icc_length;
    unsigned char *id_icc;
};

int GetReadersPACECapabilities(sc_card_t *card, const unsigned char *in,
        size_t inlen, unsigned char **out, size_t *outlen);
int EstablishPACEChannel(const struct sm_ctx *oldpacectx, sc_card_t *card,
        struct establish_pace_channel_input pace_input,
        unsigned char **out, size_t *outlen, struct sm_ctx *sctx);

int pace_reset_retry_counter(struct sm_ctx *ctx, sc_card_t *card,
        enum s_type pin_id, int ask_for_secret,
        const char *new, size_t new_len);
#define pace_unblock_pin(ctx, card) \
    pace_reset_retry_counter(ctx, card, PACE_PIN, 0, NULL, 0)
#define pace_change_pin(ctx, card, newp, newplen) \
    pace_reset_retry_counter(ctx, card, PACE_PIN, 1, newp, newplen)

#ifdef  __cplusplus
}
#endif
#endif
