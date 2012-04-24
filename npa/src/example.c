/*
 * Copyright (C) 2011 Frank Morgner
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

/* This example shows how to use the library functions EstablishPACEChannel to
 * get a secure channel to the nPA. We use the builtin function npa_change_pin
 * to modify the PIN using the secure channel. Then we transmit an arbitrary
 * APDU encrypted and authenticated to the card using sm_transmit_apdu. */
#include <npa/npa.h>
#include <npa/scutil.h>
#include <string.h>

static int reader_num = -1;

static const char *newpin = NULL;
static const char *pin = NULL;

/* SELECT the Master File (MF) */
const unsigned char apdubuf[] = {0x00, 0xA4, 0x00, 0x0C, 0x02, 0x3F, 0x00};

int
main (int argc, char **argv)
{
    /* Set up the environment */
    int r;

    sc_context_t *ctx = NULL;
    sc_card_t *card = NULL;
    sc_reader_t *reader = NULL;

    sc_apdu_t apdu;
    u8 buf[0xffff];

    struct sm_ctx sctx;
    struct establish_pace_channel_input pace_input;
    struct establish_pace_channel_output pace_output;

    memset(&sctx, 0, sizeof sctx);
    memset(&pace_input, 0, sizeof pace_input);
    memset(&pace_output, 0, sizeof pace_output);


    /* Connect to a reader and the nPA */
    r = initialize(reader_num, NULL, 0, &ctx, &reader);
    if (r < 0) {
        fprintf(stderr, "Can't initialize reader\n");
        exit(1);
    }

    if (sc_connect_card(reader, &card) < 0) {
        fprintf(stderr, "Could not connect to card\n");
        sc_release_context(ctx);
        exit(1);
    }


    /* Now we try to change the PIN. Therefor we need to establish a SM channel
     * with PACE.
     *
     * You could set your PIN with pin=“123456”; or just leave it at NULL to be
     * asked for it. The same applies to the new PIN newpin. */
    pace_input.pin_id = PACE_PIN;
    pace_input.pin = (unsigned char *) pin;
    pace_input.pin_length = pin ? strlen(pin) : 0;

    r = EstablishPACEChannel(NULL, card, pace_input, &pace_output,
            &sctx, EAC_TR_VERSION_2_02);
    if (r < 0)
        goto err;
    printf("Established PACE channel with PIN.\n");

    r = npa_change_pin(&sctx, card, newpin, newpin ? strlen(newpin) : 0);
    if (r < 0)
        goto err;
    printf("Changed PIN.\n");


    /* Now we want to transmit additional APDUs in the established SM channel.
     *
     * Here we are parsing the raw apdu buffer apdubuf to be transformed into
     * an sc_apdu_t. Alternatively you could also set CLA, INS, P1, P2, ... by
     * hand in the sc_apdu_t object. */
    r = sc_bytes2apdu(ctx, apdubuf, sizeof apdubuf, &apdu);
    if (r < 0)
        goto err;

    /* write the response data to buf */
    apdu.resp = buf;
    apdu.resplen = sizeof buf;

    /* Transmit the APDU with SM */
    r = sm_transmit_apdu(&sctx, card, &apdu);


err:
    fprintf(r < 0 ? stderr : stdout, "%s\n", sc_strerror(r));

    /* Free up memory and wipe it if necessary (e.g. for keys stored in sm_ctx) */
    sm_ctx_clear_free(&sctx);
    if (pace_output.ef_cardaccess)
        free(pace_output.ef_cardaccess);
    if (pace_output.recent_car)
        free(pace_output.recent_car);
    if (pace_output.previous_car)
        free(pace_output.previous_car);
    if (pace_output.id_icc)
        free(pace_output.id_icc);
    if (pace_output.id_pcd)
        free(pace_output.id_pcd);

    sc_reset(card, 1);
    sc_disconnect_card(card);
    sc_release_context(ctx);

    return -r;
}
