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
#include "card-npa.h"
#include <npa/boxing.h>
#include <npa/npa.h>
#include <string.h>

#ifndef HAVE_SC_APDU_GET_OCTETS
#include "libopensc/card.c"
#endif

struct npa_drv_data {
    unsigned char *can;
    size_t can_length;
    unsigned char *ef_cardaccess;
    size_t ef_cardaccess_length;
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
    struct npa_drv_data *drv_data;

    if (card) {
#if 0
        /* we wait for https://github.com/OpenSC/OpenSC/pull/260 to be
         * integrated before switching extended length on, here */
        card->max_recv_size = 0xFFFF+1;
        card->max_send_size = 0xFFFF;
#endif
        drv_data = calloc(1, sizeof *drv_data);
        card->drv_data = drv_data;
        if (drv_data) {
            drv_data->can = NULL;
            drv_data->can_length = 0;
            drv_data->ef_cardaccess = NULL;
            drv_data->ef_cardaccess_length = 0;
        }

        card->caps |= SC_CARD_CAP_APDU_EXT | SC_CARD_CAP_RNG;
        memset(&card->sm_ctx, 0, sizeof card->sm_ctx);
#ifdef DISABLE_GLOBAL_BOXING_INITIALIZATION
        sc_detect_boxing_cmds(card->reader);
#endif
    }

    return SC_SUCCESS;
}

static int npa_finish(sc_card_t * card)
{
    struct npa_drv_data *drv_data;

    if (card) {
        drv_data = card->drv_data;
        if (drv_data) {
            free(drv_data->ef_cardaccess);
            free(drv_data->can);
            free(drv_data);
        }
        card->drv_data = NULL;
    }

    return SC_SUCCESS;
}

static int npa_pin_cmd_get_info(struct sc_card *card,
        struct sc_pin_cmd_data *data, int *tries_left)
{
    int r;
    u8 pin_reference;

    if (!data || data->pin_type != SC_AC_CHV || !tries_left) {
        r = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }

    pin_reference = data->pin_reference;
    switch (data->pin_reference) {
        case PACE_PIN_ID_CAN:
        case PACE_PIN_ID_MRZ:
        case PACE_PIN_ID_PUK:
            /* usually unlimited number of retries */
            *tries_left = -1;
            data->pin1.max_tries = -1;
            data->pin1.tries_left = -1;
            r = SC_SUCCESS;
            break;

        case PACE_PIN_ID_PIN:
            /* usually 3 tries */
            *tries_left = 3;
            data->pin1.max_tries = 3;
            r = npa_pace_get_tries_left(card,
                    pin_reference, tries_left);
            data->pin1.tries_left = *tries_left;
            break;

        default:
            r = SC_ERROR_OBJECT_NOT_FOUND;
            goto err;
    }

err:
    return r;
}

static int npa_pace_verify(struct sc_card *card,
        unsigned char pin_reference, struct sc_pin_cmd_pin *pin,
        const unsigned char *chat, size_t chat_length, int *tries_left)
{
    int r;
    struct establish_pace_channel_input pace_input;
    struct establish_pace_channel_output pace_output;

    memset(&pace_input, 0, sizeof pace_input);
    memset(&pace_output, 0, sizeof pace_output);
    if (chat) {
        pace_input.chat = chat;
        pace_input.chat_length = chat_length;
    }
    pace_input.pin_id = pin_reference;
    if (pin) {
        pace_input.pin = pin->data;
        pace_input.pin_length = pin->len;
    }
    npa_get_cache(card, pace_input.pin_id, &pace_input.pin,
            &pace_input.pin_length, &pace_output.ef_cardaccess,
            &pace_output.ef_cardaccess_length);

    r = perform_pace(card, pace_input, &pace_output, EAC_TR_VERSION_2_02);

    if (tries_left) {
        if (pace_output.mse_set_at_sw1 == 0x63
                && (pace_output.mse_set_at_sw2 & 0xc0) == 0xc0) {
            *tries_left = pace_output.mse_set_at_sw2 & 0x0f;
        } else {
            *tries_left = -1;
        }
    }

    /* resume the PIN if needed */
    if (pin_reference == PACE_PIN_ID_PIN
            && r != SC_SUCCESS
            && pace_output.mse_set_at_sw1 == 0x63
            && (pace_output.mse_set_at_sw2 & 0xc0) == 0xc0
            && (pace_output.mse_set_at_sw2 & 0x0f) <= UC_PIN_SUSPENDED) {
        /* TODO ask for user consent when automatically resuming the PIN */
        sc_log(card->ctx, "%s is suspended. Will try to resume it with %s.\n",
                npa_secret_name(pin_reference), npa_secret_name(PACE_PIN_ID_CAN));

        pace_input.pin_id = PACE_PIN_ID_CAN;
        pace_input.pin = NULL;
        pace_input.pin_length = 0;
        npa_get_cache(card, pace_input.pin_id, &pace_input.pin,
                &pace_input.pin_length, NULL, NULL);

        r = perform_pace(card, pace_input, &pace_output, EAC_TR_VERSION_2_02);

        if (r == SC_SUCCESS) {
            pace_input.pin_id = pin_reference;
            if (pin) {
                pace_input.pin = pin->data;
                pace_input.pin_length = pin->len;
            }

            r = perform_pace(card, pace_input, &pace_output, EAC_TR_VERSION_2_02);

            if (r == SC_SUCCESS) {
                sc_log(card->ctx, "%s resumed.\n");
                if (tries_left) {
                    *tries_left = MAX_PIN_TRIES;
                }
            } else {
                if (tries_left) {
                    if (pace_output.mse_set_at_sw1 == 0x63
                            && (pace_output.mse_set_at_sw2 & 0xc0) == 0xc0) {
                        *tries_left = pace_output.mse_set_at_sw2 & 0x0f;
                    } else {
                        *tries_left = -1;
                    }
                }
            }
        }
    }

    if (pin_reference == PACE_PIN_ID_PIN && tries_left) {
       if (*tries_left == 0) {
           sc_log(card->ctx, "%s is suspended and must be resumed.\n",
                   npa_secret_name(pin_reference));
       } else if (*tries_left == 1) {
           sc_log(card->ctx, "%s is blocked and must be unblocked.\n",
                   npa_secret_name(pin_reference));
       }
    }

    npa_set_cache(card, pace_input.pin_id, pace_input.pin,
            pace_input.pin_length, pace_output.ef_cardaccess,
            pace_output.ef_cardaccess_length);

    free(pace_output.ef_cardaccess);
    free(pace_output.recent_car);
    free(pace_output.previous_car);
    free(pace_output.id_icc);
    free(pace_output.id_pcd);

    return r;
}

static int npa_standard_pin_cmd(struct sc_card *card,
        struct sc_pin_cmd_data *data, int *tries_left)
{
    int r;
    struct sc_card_driver *iso_drv;

    iso_drv = sc_get_iso7816_driver();

    if (!iso_drv || !iso_drv->ops || !iso_drv->ops->pin_cmd) {
        r = SC_ERROR_INTERNAL;
    } else {
        r = iso_drv->ops->pin_cmd(card, data, tries_left);
    }

    return r;
}

static int npa_pin_cmd(struct sc_card *card,
        struct sc_pin_cmd_data *data, int *tries_left)
{
    int r;

    if (!data) {
        r = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }

    if (data->pin_type != SC_AC_CHV) {
        r = SC_ERROR_NOT_SUPPORTED;
        goto err;
    }

    switch (data->cmd) {
        case SC_PIN_CMD_GET_INFO:
            r = npa_pin_cmd_get_info(card, data, tries_left);
            if (r != SC_SUCCESS)
                goto err;
            break;

        case SC_PIN_CMD_UNBLOCK:
            /* opensc-explorer unblocks the PIN by only sending
             * SC_PIN_CMD_UNBLOCK whereas the PKCS#15 framework first verifies
             * the PUK with SC_PIN_CMD_VERIFY and then calls with
             * SC_PIN_CMD_UNBLOCK.
             *
             * Here we determine whether the PUK has been verified or not by
             * checking if an SM channel has been established. */
            if (card->sm_ctx.sm_mode != SM_MODE_TRANSMIT) {
                /* PUK has not yet been verified */
                r = npa_pace_verify(card, PACE_PIN_ID_PUK, &(data->pin1), NULL,
                        0, NULL);
                if (r != SC_SUCCESS)
                    goto err;
            }
            r = npa_reset_retry_counter(card, data->pin_reference, 0,
                    NULL, 0);
            if (r != SC_SUCCESS)
                goto err;
            break;

        case SC_PIN_CMD_CHANGE:
        case SC_PIN_CMD_VERIFY:
            switch (data->pin_reference) {
                case PACE_PIN_ID_CAN:
                case PACE_PIN_ID_PUK:
                case PACE_PIN_ID_MRZ:
                case PACE_PIN_ID_PIN:
                    r = npa_pace_verify(card, data->pin_reference,
                            &(data->pin1), NULL, 0, tries_left);
                    if (r != SC_SUCCESS)
                        goto err;
                    break;

                case NPA_PIN_ID_ESIGN_PIN:
                    if (card->reader->capabilities & SC_READER_CAP_PACE_ESIGN) {
                        sc_log(card->ctx, "Found a comfort reader (CAT-K).\n");
                        sc_log(card->ctx, "Will verify cached CAN first.\n");
                        r = npa_pace_verify(card, PACE_PIN_ID_CAN, NULL,
                                esign_chat, sizeof esign_chat, tries_left);
                        if (r != SC_SUCCESS)
                            goto err;
                    } else {
                        sc_log(card->ctx, "No comfort reader (CAT-K) found.\n");
                        sc_log(card->ctx, "I hope you have already performed EAC as ST...\n");
                    }
                    r = npa_standard_pin_cmd(card, data, tries_left);
                    if (r != SC_SUCCESS)
                        goto err;
                    break;

                default:
                    r = SC_ERROR_OBJECT_NOT_FOUND;
                    goto err;
                    break;
            }

            if (data->cmd == SC_PIN_CMD_CHANGE) {
                r = npa_reset_retry_counter(card, data->pin_reference, 1,
                        (const char *) data->pin2.data, data->pin2.len);
                if (r != SC_SUCCESS)
                    goto err;
            }
            break;

        default:
            r = SC_ERROR_INTERNAL;
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
    npa_ops.finish = npa_finish;
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

void npa_get_cache(struct sc_card *card,
        unsigned char pin_id, const unsigned char **pin, size_t *pin_length,
        unsigned char **ef_cardaccess, size_t *ef_cardaccess_length)
{
    struct npa_drv_data *drv_data;
	if (card && card->drv_data) {
		drv_data = card->drv_data;
        if (pin_id == PACE_PIN_ID_CAN && pin && pin_length && !*pin) {
            /* set CAN if *pin is NULL */
            *pin = drv_data->can;
            *pin_length = drv_data->can_length;
			sc_log(card->ctx, "Using the cached CAN\n");
        }
        if (ef_cardaccess && ef_cardaccess_length && !*ef_cardaccess) {
            /* set EF.CardAccess if *ef_cardaccess is NULL */
            *ef_cardaccess = drv_data->ef_cardaccess;
            *ef_cardaccess_length = drv_data->ef_cardaccess_length;
        }
	}
}

void npa_set_cache(struct sc_card *card,
        unsigned char pin_id, const unsigned char *pin, size_t pin_length,
        const unsigned char *ef_cardaccess, size_t ef_cardaccess_length)
{
    unsigned char *p;
    struct npa_drv_data *drv_data;
    if (card && card->drv_data) {
        drv_data = card->drv_data;
        if (pin_id == PACE_PIN_ID_CAN && pin) {
            p = realloc(drv_data->can, pin_length);
            if (p) {
                memcpy(p, pin, pin_length);
                drv_data->can = p;
                drv_data->can_length = pin_length;
            }
        }
        if (ef_cardaccess) {
            p = realloc(drv_data->ef_cardaccess, ef_cardaccess_length);
            if (p) {
                memcpy(p, ef_cardaccess, ef_cardaccess_length);
                drv_data->ef_cardaccess = p;
                drv_data->ef_cardaccess_length = ef_cardaccess_length;
            }
        }
    }
}
