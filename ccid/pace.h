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
#ifndef _PACE_H
#define _PACE_H

#include <linux/usb/ch9.h>
#include <openssl/asn1.h>
#include <openssl/asn1t.h>

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

int GetReadersPACECapabilities(sc_context_t *ctx, sc_card_t *card, const __u8
        *in, __u8 **out, size_t *outlen);
int EstablishPACEChannel(sc_context_t *ctx, sc_card_t *card, const __u8 *in,
        __u8 **out, size_t *outlen);
void pace_test(sc_context_t *ctx, sc_card_t *card);

#ifdef  __cplusplus
}
#endif
#endif
