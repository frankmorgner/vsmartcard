/*
 * Copyright (C) 2012 Frank Morgner
 *
 * This file is part of pcsc-relay.
 *
 * pcsc-relay is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * pcsc-relay is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * pcsc-relay.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pcsc-relay.h"
#include "vpcd.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



unsigned int viccport = VPCDPORT;
char *vicchostname = NULL;
char *viccatr = "3B80800101";
unsigned char atr[256];
size_t atr_len = 0;



static int _vicc_connect(driver_data_t **driver_data)
{
    struct vicc_ctx *ctx;
    const long secs = 300;

    if (!driver_data)
        return 0;

    if (viccatr) {
        size_t i;
        unsigned char *hex = viccatr;
        unsigned char *bin = atr;
        atr_len = strlen(viccatr);
        if (atr_len % 2 != 0) {
            RELAY_ERROR("Length of ATR needs to be even\n");
            return 0;
        }
        atr_len /= 2;
        if (atr_len > sizeof atr) {
            RELAY_ERROR("ATR too long\n");
            return 0;
        }
        while (*hex) {
            if (sscanf(hex, "%2hhX", bin) != 1) {
                RELAY_ERROR("bad ATR\n");
                return 0;
            }
            hex += 2;
            bin += 1;
        }
    } else {
        atr_len = 0;
    }


    ctx = vicc_init(vicchostname, viccport);
    if (!ctx) {
        RELAY_ERROR("Could not initialize connection to VPCD\n");
        return 0;
    }
    *driver_data = ctx;

    INFO("Waiting for VPCD on port %hu for %ld seconds\n",
            (unsigned short) viccport, secs);
    if (vicc_connect(ctx, secs, 0))
        return 1;

    return 0;
}

static int vicc_disconnect(driver_data_t *driver_data)
{
    struct vicc_ctx *ctx = driver_data;

    vicc_eject(ctx);
    if (vicc_exit(ctx) != 0) {
        RELAY_ERROR("Could not close connection to virtual ICC\n");
        return 0;
    }

    return 1;
}

static int vicc_receive_capdu(driver_data_t *driver_data,
        unsigned char **capdu, size_t *len)
{
    struct vicc_ctx *ctx = driver_data;

    int r = 0;
    ssize_t size;

    if (!len)
        goto err;

    do {
        size = vicc_transmit(ctx, 0, NULL, capdu);

        if (size < 0) {
            RELAY_ERROR("could not receive request\n");
            goto err;
        }

        if (size == VPCD_CTRL_LEN) {
            switch (*capdu[0]) {
                case VPCD_CTRL_OFF:
                case VPCD_CTRL_ON:
                case VPCD_CTRL_RESET:
                    // ignore reset, power on, power off
                    break;
                case VPCD_CTRL_ATR:
                    if (vicc_transmit(ctx, atr_len, atr, NULL) < 0) {
                        RELAY_ERROR("could not send ATR\n");
                        goto err;
                    }
                    break;
                default:
                    RELAY_ERROR("Unknown request: 0x%0X\n", *capdu[0]);
                    goto err;
            }
        } else {
            // finaly we got the capdu
            *len = size;
            r = 1;
        }
    } while (!r);

err:
    return r;
}

static int vicc_send_rapdu(driver_data_t *driver_data,
        const unsigned char *rapdu, size_t len)
{
    struct vicc_ctx *ctx = driver_data;

    if (!ctx || !rapdu)
        return 0;

    if (vicc_transmit(ctx, len, rapdu, NULL) < 0) {
        RELAY_ERROR("could not send R-APDU\n");
        return 0;
    }

    return 1;
}


struct rf_driver driver_vicc = {
    .connect = _vicc_connect,
    .disconnect = vicc_disconnect,
    .receive_capdu = vicc_receive_capdu,
    .send_rapdu = vicc_send_rapdu,
};
