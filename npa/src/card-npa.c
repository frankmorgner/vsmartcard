/*
 * card-npa.c: Recognize known German identity cards
 *
 * Copyright (C) 2011-2014 Frank Morgner <morgner@informatik.hu-berlin.de>
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

#include "iso-sm-internal.h"
#include "libopensc/internal.h"
#include "libopensc/pace.h"
#include <npa/boxing.h>
#include <npa/npa.h>
#include <string.h>

#ifndef HAVE_SC_APDU_GET_OCTETS
#include "libopensc/card.c"
#endif

enum {
    SC_CARD_TYPE_NPA = 42000,
    SC_CARD_TYPE_NPA_TEST,
    SC_CARD_TYPE_NPA_ONLINE,
};

static struct sc_atr_table npa_atrs[] = {
    {"3B:8A:80:01:80:31:F8:73:F7:41:E0:82:90:00:75", NULL, "German ID card (neuer Personalausweis, nPA)", SC_CARD_TYPE_NPA, 0, NULL},
    {"3B:84:80:01:00:00:90:00:95", NULL, "German ID card (Test neuer Personalausweis)", SC_CARD_TYPE_NPA_TEST, 0, NULL},
    {"3B:88:80:01:00:E1:F3:5E:13:77:83:00:00", "FF:FF:FF:FF:00:FF:FF:FF:FF:FF:FF:FF:00", "German ID card (Test Online-Ausweisfunktion)", SC_CARD_TYPE_NPA_ONLINE, 0, NULL},
    {NULL, NULL, NULL, 0, 0, NULL}
};

static struct sc_card_operations npa_ops;
static struct sc_card_driver npa_drv = {
    "German ID card (neuer Personalausweis, nPA)",
    "npa",
    &npa_ops,
    NULL, 0, NULL
};

static int npa_match_card(sc_card_t * card)
{
    if (_sc_match_atr(card, npa_atrs, &card->type) < 0)
        return 0;
    return 1;
}

static int npa_init(sc_card_t * card)
{
    card->drv_data = NULL;
    card->caps |= SC_CARD_CAP_APDU_EXT | SC_CARD_CAP_RNG;

#ifdef DISABLE_GLOBAL_BOXING_INITIALIZATION
    sc_detect_boxing_cmds(card->reader);
#endif

    return SC_SUCCESS;
}

static int npa_pin_cmd(struct sc_card *card, struct sc_pin_cmd_data *data, int *tries_left)
{
    struct establish_pace_channel_input pace_input;
    struct establish_pace_channel_output pace_output;
    int r;

    memset(&pace_input, 0, sizeof pace_input);
    memset(&pace_output, 0, sizeof pace_output);

    if (!data) {
        r = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }

    if (data->pin_type != SC_AC_CHV) {
        r = SC_ERROR_NOT_SUPPORTED;
        goto err;
    }

    if (tries_left)
        *tries_left = -1;

    if (data->cmd != SC_PIN_CMD_UNBLOCK
            && data->cmd != SC_PIN_CMD_VERIFY
            && data->cmd != SC_PIN_CMD_CHANGE) {
        /* TODO support SC_PIN_CMD_GET_INFO */
        r = SC_ERROR_NOT_SUPPORTED;
        goto err;
    }

    pace_input.pin_id = data->pin_reference;
    pace_input.pin = data->pin1.data;
    pace_input.pin_length = data->pin1.len;

    if (data->cmd == SC_PIN_CMD_UNBLOCK) {
        /* FIXME SC_PIN_CMD_UNBLOCK is currently overloaded with the resume
         * command. Iff the first secret has the length of a CAN (6) we are
         * resuming the PIN with the CAN. Otherwise we will unblock the PIN
         * with PUK. */
        if (pace_input.pin_length == CAN_LEN) {
            /* Try to resume the PIN with the CAN. */
            pace_input.pin_id = PACE_PIN_ID_CAN;
            r = perform_pace(card, pace_input, &pace_output, 2);
            if (r != SC_SUCCESS)
                goto err;

            /* Initialize pace_input for usage with the intended PIN */
            pace_input.pin_id = data->pin_reference;
            pace_input.pin = data->pin2.data;
            pace_input.pin_length = data->pin2.len;
        } else {
            /* Initialize pace_input for usage with the PUK */
            pace_input.pin_id = PACE_PIN_ID_PUK;
        }
    }

    r = perform_pace(card, pace_input, &pace_output, 2);
    if (r != SC_SUCCESS)
        goto err;

    if (pace_output.mse_set_at_sw1 == 0x63) {
        if ((pace_output.mse_set_at_sw2 & 0xF0) == 0xC0 && tries_left != NULL)
            *tries_left = pace_output.mse_set_at_sw2 & 0x0F;
        if (*tries_left > 0 && r != SC_SUCCESS)
            *tries_left = *tries_left - 1;
    }

    switch (data->cmd) {
        case SC_PIN_CMD_UNBLOCK:
            if (pace_input.pin_length != CAN_LEN) {
                r = npa_unblock_pin(card);
                if (r != SC_SUCCESS)
                    goto err;
            }
            break;
        case SC_PIN_CMD_CHANGE:
            r = npa_reset_retry_counter(card, data->pin_reference, 1,
                    (const char *) data->pin2.data, data->pin2.len);
            if (r != SC_SUCCESS)
                goto err;
            break;
    }

err:
    SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_NORMAL, r);
}

static struct sc_card_driver *npa_get_driver(void)
{
    struct sc_card_driver *iso_drv = sc_get_iso7816_driver();

    npa_ops = *iso_drv->ops;
    npa_ops.match_card = npa_match_card;
    npa_ops.init = npa_init;
    npa_ops.pin_cmd = npa_pin_cmd;

    return &npa_drv;
}

void *sc_module_init(const char *name)
{
    const char npa_name[] = "npa";
    if (name) {
        if (strcmp(npa_name, name) == 0)
            return npa_get_driver;
    }
    return NULL;
}

const char *sc_driver_version(void)
{
    /* Tested with OpenSC 0.12 and 0.13.0, which can't be captured by checking
     * our version info against OpenSC's PACKAGE_VERSION. For this reason we
     * tell OpenSC that everything is fine, here. */
    return sc_get_version();
}
