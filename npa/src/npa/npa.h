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
#ifndef _CCID_NPA_H
#define _CCID_NPA_H

#include <npa/sm.h>
#include <libopensc/opensc.h>
#include <openssl/bn.h>
#include <openssl/eac.h>
#include <openssl/pace.h>

#ifdef __cplusplus
extern "C" {
#endif

/** NPA capabilities (TR-03119): PACE */
#define NPA_BITMAP_PACE  0x40
/** NPA capabilities (TR-03119): EPA: eID */
#define NPA_BITMAP_EID   0x20
/** NPA capabilities (TR-03119): EPA: eSign */
#define NPA_BITMAP_ESIGN 0x10

/** NPA result (TR-03119): Kein Fehler */
#define NPA_SUCCESS                            0x00000000
/** NPA result (TR-03119): Längen im Input sind inkonsistent */
#define NPA_ERROR_LENGTH_INCONSISTENT          0xD0000001
/** NPA result (TR-03119): Unerwartete Daten im Input */
#define NPA_ERROR_UNEXPECTED_DATA              0xD0000002
/** NPA result (TR-03119): Unerwartete Kombination von Daten im Input */
#define NPA_ERROR_UNEXPECTED_DATA_COMBINATION  0xD0000003
/** NPA result (TR-03119): Die Karte unterstützt das PACE – Verfahren nicht.  (Unerwartete Struktur in Antwortdaten der Karte) */
#define NPA_ERROR_CARD_NOT_SUPPORTED           0xE0000001
/** NPA result (TR-03119): Der Kartenleser unterstützt den angeforderten bzw. den ermittelten Algorithmus nicht.  */
#define NPA_ERROR_ALGORITH_NOT_SUPPORTED       0xE0000002
/** NPA result (TR-03119): Der Kartenleser kennt die PIN – ID nicht. */
#define NPA_ERROR_PINID_NOT_SUPPORTED          0xE0000003
/** NPA result (TR-03119): Negative Antwort der Karte auf Select EF_CardAccess (needs to be OR-ed with SW1|SW2) */
#define NPA_ERROR_SELECT_EF_CARDACCESS         0xF0000000
/** NPA result (TR-03119): Negative Antwort der Karte auf Read Binary (needs to be OR-ed with SW1|SW2) */
#define NPA_ERROR_READ_BINARY                  0xF0010000
/** NPA result (TR-03119): Negative Antwort der Karte auf MSE: Set AT (needs to be OR-ed with SW1|SW2) */
#define NPA_ERROR_MSE_SET_AT                   0xF0020000
/** NPA result (TR-03119): Negative Antwort der Karte auf General Authenticate Step 1 (needs to be OR-ed with SW1|SW2) */
#define NPA_ERROR_GENERAL_AUTHENTICATE_1       0xF0030000
/** NPA result (TR-03119): Negative Antwort der Karte auf General Authenticate Step 2 (needs to be OR-ed with SW1|SW2) */
#define NPA_ERROR_GENERAL_AUTHENTICATE_2       0xF0040000
/** NPA result (TR-03119): Negative Antwort der Karte auf General Authenticate Step 3 (needs to be OR-ed with SW1|SW2) */
#define NPA_ERROR_GENERAL_AUTHENTICATE_3       0xF0050000
/** NPA result (TR-03119): Negative Antwort der Karte auf General Authenticate Step 4 (needs to be OR-ed with SW1|SW2) */
#define NPA_ERROR_GENERAL_AUTHENTICATE_4       0xF0060000
/** NPA result (TR-03119): Kommunikationsabbruch mit Karte. */
#define NPA_ERROR_COMMUNICATION                0xF0100001
/** NPA result (TR-03119): Keine Karte im Feld. */
#define NPA_ERROR_NO_CARD                      0xF0100002
/** NPA result (TR-03119): Benutzerabbruch. */
#define NPA_ERROR_ABORTED                      0xF0200001
/** NPA result (TR-03119): Benutzer – Timeout */
#define NPA_ERROR_TIMEOUT                      0xF0200002

//#define PACE_MRZ 0x01
//#define PACE_CAN 0x02
//#define PACE_PIN 0x03
//#define PACE_PUK 0x04

/** File identifier of EF.CardAccess */
#define FID_EF_CARDACCESS 0x011C

/** Maximum length of EF.CardAccess */
#define MAX_EF_CARDACCESS 2048
/** Maximum length of PIN */
#define MAX_PIN_LEN       6
/** Minimum length of PIN */
#define MIN_PIN_LEN       6
/** Minimum length of MRZ */
#define MAX_MRZ_LEN       128

const char *npa_secret_name(enum s_type pin_id);


/** 
 * Input data for EstablishPACEChannel()
 */
struct establish_pace_channel_input {
    /** Version of TR-03110 to use with PACE */
    enum eac_tr_version tr_version;

    /** Type of secret (CAN, MRZ, PIN or PUK). You may use <tt>enum s_type</tt> from \c <openssl/pace.h> */
    unsigned char pin_id;

    /** Length of \a chat */
    size_t chat_length;
    /** Card holder authorization template */
    const unsigned char *chat;

    /** Length of \a pin */
    size_t pin_length;
    /** Secret */
    const unsigned char *pin;

    /** Length of \a certificate_description */
    size_t certificate_description_length;
    /** Certificate description */
    const unsigned char *certificate_description;
};

/** 
 * Output data for EstablishPACEChannel()
 */
struct establish_pace_channel_output {
    /** PACE result (TR-03119) */
    unsigned int result;

    /** MSE: Set AT status byte */
    unsigned char mse_set_at_sw1;
    /** MSE: Set AT status byte */
    unsigned char mse_set_at_sw2;

    /** Length of \a ef_cardaccess */
    size_t ef_cardaccess_length;
    /** EF.CardAccess */
    unsigned char *ef_cardaccess;

    /** Length of \a recent_car */
    size_t recent_car_length;
    /** Most recent certificate authority reference */
    unsigned char *recent_car;

    /** Length of \a previous_car */
    size_t previous_car_length;
    /** Previous certificate authority reference */
    unsigned char *previous_car;

    /** Length of \a id_icc */
    size_t id_icc_length;
    /** ICC identifier */
    unsigned char *id_icc;

    /** Length of \a id_pcd */
    size_t id_pcd_length;
    /** PCD identifier */
    unsigned char *id_pcd;
};

#ifdef BUERGERCLIENT_WORKAROUND
int get_ef_card_access(sc_card_t *card,
        u8 **ef_cardaccess, size_t *length_ef_cardaccess);
#endif

/** 
 * @brief Get the reader's PACE capabilities
 * 
 * @param[in,out] bitmap where to store capabilities bitmap
 * @note Since this code offers no support for terminal certificate, the bitmap is always \c PACE_BITMAP_PACE|PACE_BITMAP_EID
 * 
 * @return \c SC_SUCCESS or error code if an error occurred
 */
int GetReadersPACECapabilities(u8 *bitmap);

/** 
 * @brief Establish secure messaging using PACE
 *
 * Prints certificate description and card holder authorization template if
 * given in a human readable form to stdout. If no secret is given, the user is
 * asked for it. Only \a pace_input.pin_id is mandatory, the other members of
 * \a pace_input can be set to \c 0 or \c NULL.
 *
 * The buffers in \a pace_output are allocated using \c realloc() and should be
 * set to NULL, if empty. If an EF.CardAccess is already present, this file is
 * reused and not fetched from the card.
 * 
 * @param[in]     oldpacectx  (optional) Old SM context, if PACE is established in an existing SM channel
 * @param[in]     card
 * @param[in]     pace_input
 * @param[in,out] pace_output
 * @param[out]    sctx
 * 
 * @return \c SC_SUCCESS or error code if an error occurred
 */
int EstablishPACEChannel(struct sm_ctx *oldpacectx, sc_card_t *card,
        struct establish_pace_channel_input pace_input,
        struct establish_pace_channel_output *pace_output,
        struct sm_ctx *sctx);

/** 
 * @brief Sends a reset retry counter APDU
 *
 * According to TR-03110 the reset retry counter APDU is used to set a new PIN
 * or to reset the retry counter of the PIN. The standard requires this
 * operation to be authorized either by an established PACE channel or by the
 * effective authorization of the terminal's certificate.
 * 
 * @param[in] ctx            (optional) NPA SM context
 * @param[in] card
 * @param[in] pin_id         Type of secret (usually PIN or CAN). You may use <tt>enum s_type</tt> from \c <openssl/pace.h>.
 * @param[in] ask_for_secret whether to ask the user for the secret (\c 1) or not (\c 0)
 * @param[in] new            (optional) new secret
 * @param[in] new_len        (optional) length of \a new
 * 
 * @return \c SC_SUCCESS or error code if an error occurred
 */
int npa_reset_retry_counter(struct sm_ctx *ctx, sc_card_t *card,
        enum s_type pin_id, int ask_for_secret,
        const char *new, size_t new_len);
/** 
 * @brief Send APDU to unblock the PIN
 *
 * @param[in] ctx            (optional) NPA SM context
 * @param[in] card
 */
#define npa_unblock_pin(ctx, card) \
    npa_reset_retry_counter(ctx, card, PACE_PIN, 0, NULL, 0)
/** Send APDU to set a new PIN
 *
 * @param[in] ctx            (optional) NPA SM context
 * @param[in] card
 * @param[in] new            (optional) new PIN
 * @param[in] new_len        (optional) length of \a new
 */
#define npa_change_pin(ctx, card, newp, newplen) \
    npa_reset_retry_counter(ctx, card, PACE_PIN, 1, newp, newplen)

#ifdef  __cplusplus
}
#endif
#endif
