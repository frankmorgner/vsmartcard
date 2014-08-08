/*
 * pkcs15-npa.c: PKCS#15 emulation for German ID card
 *
 * Copyright (C) 2014 Frank Morgner <morgner@informatik.hu-berlin.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "libopensc/log.h"
#include "libopensc/opensc.h"
#include "libopensc/pace.h"
#include "libopensc/pkcs15.h"
#include "card-npa.h"
#include <npa/npa.h>
#include <stdlib.h>
#include <string.h>


static int npa_detect_card(sc_pkcs15_card_t *p15card)
{
	int r = SC_ERROR_WRONG_CARD;

	if (p15card && p15card->card
			&& (p15card->card->type == SC_CARD_TYPE_NPA
				|| p15card->card->type == SC_CARD_TYPE_NPA_TEST
				|| p15card->card->type == SC_CARD_TYPE_NPA_ONLINE)) {
		r = SC_SUCCESS;
	}

	return r;
}

static int npa_add_pin(sc_pkcs15_card_t *p15card,
	   	const char *label, int max_tries,
		unsigned int flags, size_t  min_length,
	   	size_t max_length, unsigned char reference,
		unsigned char auth_id, const struct sc_path *path,
		const char *pin, size_t pin_length)
{
	struct sc_pkcs15_auth_info pin_info;
	struct sc_pkcs15_object pin_obj;
	memset(&pin_info, 0, sizeof pin_info);
	memset(&pin_obj, 0, sizeof(pin_obj));

	strncpy(pin_obj.label, label, sizeof pin_obj.label);
	pin_obj.label[(sizeof pin_obj.label) - 1]= '\0';
	if (auth_id) {
		pin_obj.auth_id.len      = sizeof auth_id;
		memcpy(&pin_obj.auth_id.value, &auth_id, sizeof auth_id);
	}

	pin_info.auth_id.len      = sizeof reference;
	memcpy(&pin_info.auth_id.value, &reference, sizeof reference);
	pin_info.auth_type = SC_PKCS15_PIN_AUTH_TYPE_PIN;
	pin_info.auth_method = SC_AC_CHV;
	pin_info.tries_left = -1;
	pin_info.max_tries = max_tries;
	if (path)
		memcpy(&pin_info.path, path, sizeof *path);

	pin_info.attrs.pin.flags = flags;
	pin_info.attrs.pin.type = SC_PKCS15_PIN_TYPE_ASCII_NUMERIC;
	pin_info.attrs.pin.min_length = min_length;
	pin_info.attrs.pin.max_length = max_length;
	pin_info.attrs.pin.reference = reference;

	if (pin && pin_length) {
		sc_pkcs15_pincache_add(p15card, &pin_obj, pin, pin_length);
	}

	return sc_pkcs15emu_add_pin_obj(p15card, &pin_obj, &pin_info);
}

static int npa_add_pins(sc_pkcs15_card_t *p15card, const char *can)
{
	int r;
	const sc_path_t *mf = sc_get_mf_path();
	sc_path_t df_esign;

	r = sc_path_set(&df_esign, SC_PATH_TYPE_PATH,
		   	df_esign_path, sizeof df_esign_path, 0, 0);
	if (r != SC_SUCCESS)
		goto err;

	r = npa_add_pin(p15card, npa_secret_name(PACE_PIN_ID_MRZ), -1,
			SC_PKCS15_PIN_FLAG_CASE_SENSITIVE
			| SC_PKCS15_PIN_FLAG_INITIALIZED
			| SC_PKCS15_PIN_FLAG_SO_PIN
			| SC_PKCS15_PIN_FLAG_UNBLOCK_DISABLED
		   	| SC_PKCS15_PIN_FLAG_CHANGE_DISABLED,
			90, 90, PACE_PIN_ID_MRZ, 0, mf, NULL, 0);
	if (r != SC_SUCCESS)
		goto err;

	r = npa_add_pin(p15card, npa_secret_name(PACE_PIN_ID_CAN), -1,
			SC_PKCS15_PIN_FLAG_CASE_SENSITIVE
			| SC_PKCS15_PIN_FLAG_INITIALIZED
			| SC_PKCS15_PIN_FLAG_SO_PIN
			| SC_PKCS15_PIN_FLAG_UNBLOCK_DISABLED,
			6, 6, PACE_PIN_ID_CAN, 0, mf, can, can ? strlen(can) : 0);
	if (r != SC_SUCCESS)
		goto err;
	/* TODO */
	/*r = sc_pkcs15_pincache_add(p15card, pin_obj, pinbuf, *pinsize);*/

	r = npa_add_pin(p15card, npa_secret_name(PACE_PIN_ID_PIN), 3,
			SC_PKCS15_PIN_FLAG_CASE_SENSITIVE
			| SC_PKCS15_PIN_FLAG_INITIALIZED,
			5, 6, PACE_PIN_ID_PIN, PACE_PIN_ID_PUK, mf, NULL, 0);
	if (r != SC_SUCCESS)
		goto err;

	r = npa_add_pin(p15card, npa_secret_name(PACE_PIN_ID_PUK), -1,
			SC_PKCS15_PIN_FLAG_CASE_SENSITIVE
			| SC_PKCS15_PIN_FLAG_INITIALIZED
			| SC_PKCS15_PIN_FLAG_UNBLOCKING_PIN
			| SC_PKCS15_PIN_FLAG_UNBLOCK_DISABLED
			| SC_PKCS15_PIN_FLAG_UNBLOCKING_PIN
			| SC_PKCS15_PIN_FLAG_CHANGE_DISABLED,
			10, 10, PACE_PIN_ID_PUK, 0, mf, NULL, 0);
	if (r != SC_SUCCESS)
		goto err;

	r = npa_add_pin(p15card, "eSign PIN", 3,
			SC_PKCS15_PIN_FLAG_CASE_SENSITIVE
			| SC_PKCS15_PIN_FLAG_LOCAL
			| SC_PKCS15_PIN_FLAG_INTEGRITY_PROTECTED
			| SC_PKCS15_PIN_FLAG_INITIALIZED,
			6, 6, NPA_PIN_ID_ESIGN_PIN, PACE_PIN_ID_PUK, &df_esign, NULL, 0);
	if (r != SC_SUCCESS)
		goto err;

err:
	return r;
}

static const char npa_manufacturer[] = "Bundesdruckerei GmbH";

static int npa_add_cardlabels(sc_pkcs15_card_t *p15card)
{
	if (!p15card || !p15card->tokeninfo)
		return SC_ERROR_INTERNAL;

	/* manufacturer ID */
	free(p15card->tokeninfo->manufacturer_id);
	p15card->tokeninfo->manufacturer_id = strdup(npa_manufacturer);
	if (!p15card->tokeninfo->manufacturer_id)
		return SC_ERROR_NOT_ENOUGH_MEMORY;

	/* card label */
	free(p15card->tokeninfo->label);
	if (p15card->card && p15card->card->name) {
		p15card->tokeninfo->label = strdup(p15card->card->name);
		if (!p15card->tokeninfo->label)
			return SC_ERROR_NOT_ENOUGH_MEMORY;
	} else {
		p15card->tokeninfo->label = NULL;
	}

	return SC_SUCCESS;
}

static int npa_get_cert(sc_pkcs15_card_t *p15card, const char *can)
{
    struct establish_pace_channel_input pace_input;
    struct establish_pace_channel_output pace_output;
	int r;

	if (!p15card || !p15card->card ||  !p15card->card->reader) {
		r = SC_ERROR_INTERNAL;
		goto err;
	}

	if (!(p15card->card->reader->capabilities & SC_READER_CAP_PACE_ESIGN)) {
		sc_log(p15card->card->ctx, "No comfort reader (CAT-K) found\n");
		r = SC_ERROR_NOT_SUPPORTED;
		goto err;
	}

    memset(&pace_input, 0, sizeof pace_input);
    memset(&pace_output, 0, sizeof pace_output);
	pace_input.chat = esign_chat;
	pace_input.chat_length = sizeof esign_chat;
	pace_input.pin_id = PACE_PIN_ID_CAN;
	if (can) {
		pace_input.pin = (const unsigned char *) can;
		pace_input.pin_length = strlen(can);
	}

    r = perform_pace(p15card->card, pace_input, &pace_output, EAC_TR_VERSION_2_02);
    if (r != SC_SUCCESS)
        goto err;

	/* TODO read certificate */
	r = SC_ERROR_OBJECT_NOT_FOUND;

err:
	return r;
}

int sc_pkcs15emu_npa_init_ex(sc_pkcs15_card_t *p15card,
		sc_pkcs15emu_opt_t *opts)
{
	int r;
	const char *can = NULL;

	if (!p15card || !p15card->card) {
		r = SC_ERROR_INTERNAL;
		goto err;
	}

	if (opts && (opts->flags & SC_PKCS15EMU_FLAGS_NO_CHECK)) {
		/* don't do a card check */
	} else {
		r = npa_detect_card(p15card);
		if (r != SC_SUCCESS)
			goto err;
	}

	r = npa_add_cardlabels(p15card);
	if (r != SC_SUCCESS)
		goto err;

	if (opts) {
		can = scconf_get_str(opts->blk, "can", NULL);
	}

	r = npa_add_pins(p15card, can);
	if (r != SC_SUCCESS)
		goto err;

	r = npa_get_cert(p15card, can);
	if (r != SC_SUCCESS) {
		sc_log(p15card->card->ctx, "No certificate found, will continue anyway\n");
		r = SC_SUCCESS;
	}

err:
	return r;
}

const char *sc_driver_version(void)
{
    /** TODO fix the version check in opensc/src/libopensc/pkcs15-syn.c:271
	 * here we choose 0.9.3 simply to pass the bogus check, see
	 * https://github.com/OpenSC/OpenSC/pull/258 */
    static const char version[] = "0.9.3";
    return version;
}
