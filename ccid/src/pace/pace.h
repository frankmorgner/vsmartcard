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

#include <pace/pace_lib.h>
#include <pace/sm.h>
#include <opensc/opensc.h>
#include <openssl/bn.h>
#include <openssl/pace.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PACE_BITMAP_PACE  0x40
#define PACE_BITMAP_EID   0x20
#define PACE_BITMAP_ESIGN 0x10

#define PACE_SUCCESS                            0x00000000
#define PACE_ERROR_LENGTH_INCONSISTENT          0xD0000001
#define PACE_ERROR_UNEXPECTED_DATA              0xD0000002
#define PACE_ERROR_UNEXPECTED_DATA_COMBINATION  0xD0000003
#define PACE_ERROR_CARD_NOT_SUPPORTED           0xE0000001
#define PACE_ERROR_ALGORITH_NOT_SUPPORTED       0xE0000002
#define PACE_ERROR_PINID_NOT_SUPPORTED          0xE0000003
#define PACE_ERROR_SELECT_EF_CARDACCESS         0xF0000000
#define PACE_ERROR_READ_BINARY                  0xF0010000
#define PACE_ERROR_MSE_SET_AT                   0xF0020000
#define PACE_ERROR_GENERAL_AUTHENTICATE_1       0xF0030000
#define PACE_ERROR_GENERAL_AUTHENTICATE_2       0xF0040000
#define PACE_ERROR_GENERAL_AUTHENTICATE_3       0xF0050000
#define PACE_ERROR_GENERAL_AUTHENTICATE_4       0xF0060000
#define PACE_ERROR_COMMUNICATION                0xF0100001
#define PACE_ERROR_NO_CARD                      0xF0100002
#define PACE_ERROR_ABORTED                      0xF0200001
#define PACE_ERROR_TIMEOUT                      0xF0200002

//#define PACE_MRZ 0x01
//#define PACE_CAN 0x02
//#define PACE_PIN 0x03
//#define PACE_PUK 0x04

#define FID_EF_CARDACCESS 0x011C

#define MAX_EF_CARDACCESS 2048
#define MAX_PIN_LEN       6
#define MIN_PIN_LEN       6
#define MAX_MRZ_LEN       128

const char *pace_secret_name(enum s_type pin_id);


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
    unsigned int result;

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

    size_t id_pcd_length;
    unsigned char *id_pcd;

    size_t hash_cert_desc_len;
    unsigned char *hash_cert_desc;
};

#ifdef BUERGERCLIENT_WORKAROUND
int get_ef_card_access(sc_card_t *card,
        u8 **ef_cardaccess, size_t *length_ef_cardaccess);
#endif

int GetReadersPACECapabilities(u8 *bitmap);

int EstablishPACEChannel(struct sm_ctx *oldpacectx, sc_card_t *card,
        struct establish_pace_channel_input pace_input,
        struct establish_pace_channel_output *pace_output,
        struct sm_ctx *sctx);

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
