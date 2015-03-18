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



unsigned int vpcdport = VPCDPORT;
char *vpcdhostname = NULL;


static int vpcd_connect(driver_data_t **driver_data)
{
    struct vicc_ctx *ctx;

    int vicc_found = 0;

    if (!driver_data)
        return 0;


    ctx = vicc_init(vpcdhostname, vpcdport);
    if (!ctx) {
        RELAY_ERROR("Could not initialize connection to virtual ICC\n");
        return 0;
    }
    *driver_data = ctx;


    INFO("Waiting for virtual ICC on port %hu\n",
            (unsigned short) vpcdport);
    do {
        switch (vicc_present(ctx)) {
            case 0:
                /* not present */
                sleep(1);
                break;
            case 1:
                vicc_found = 1;
                break;
            default:
                RELAY_ERROR("Could not get ICC state\n");
                return 0;
        }
    } while (!vicc_found);


    if (vicc_poweron(ctx) < 0) {
        RELAY_ERROR("could not powerup\n");
        return 0;
    }

    INFO("Connected to virtual ICC\n");


    return 1;
}

static int vpcd_disconnect(driver_data_t *driver_data)
{
    struct vicc_ctx *ctx = driver_data;

    if (vicc_eject(ctx) != 0)
        DEBUG("Could not eject virtual ICC\n");

    if (vicc_exit(ctx) != 0) {
        RELAY_ERROR("Could not close connection to virtual ICC\n");
        return 0;
    }

    return 1;
}

static int vpcd_transmit(driver_data_t *driver_data,
        const unsigned char *send, size_t send_len,
        unsigned char *recv, size_t *recv_len)
{
    struct vicc_ctx *ctx = driver_data;

    unsigned char *rapdu = NULL;
    int r = 0;
    ssize_t size = vicc_transmit(ctx, send_len, send, &rapdu);

    if (size < 0) {
        RELAY_ERROR("could not send apdu or receive rapdu\n");
        goto err;
    }

    if (*recv_len < size) {
        RELAY_ERROR("Not enough memory for rapdu\n");
        goto err;
    }

    memcpy(recv, rapdu, size);
    *recv_len = size;

    r = 1;

err:
    if (!r)
        *recv_len = 0;

    free(rapdu);

    return r;
}


struct sc_driver driver_vpcd = {
    .connect = vpcd_connect,
    .disconnect = vpcd_disconnect,
    .transmit = vpcd_transmit,
};
