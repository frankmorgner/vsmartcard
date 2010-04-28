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

int GetReadersPACECapabilities(sc_card_t *card, const __u8
        *in, __u8 **out, size_t *outlen);
int EstablishPACEChannel(sc_card_t *card, const __u8 *in,
        __u8 **out, size_t *outlen, struct sm_ctx *ctx);
int pace_test(sc_card_t *card,
        enum s_type pin_id, const char *pin, size_t pinlen,
        enum s_type new_pin_id, const char *new_pin, size_t new_pinlen);
int pace_change_p(struct sm_ctx *ctx, sc_card_t *card, enum s_type pin_id,
        const char *newp, size_t newplen);
#define pace_change_pin(ctx, card, newpin, newpinlen) \
        pace_change_p(ctx, card, PACE_PIN, newpin, newpinlen)
#define pace_change_can(ctx, card, newcan, newcanlen) \
        pace_change_p(ctx, card, PACE_CAN, newcan, newcanlen)

int pace_transmit_apdu(struct sm_ctx *sctx, sc_card_t *card,
        sc_apdu_t *apdu);

#ifdef  __cplusplus
}
#endif
#endif
