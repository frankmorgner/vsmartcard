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


static int vpcd_connect(driver_data_t **driver_data)
{
    int vicc_found = 0;


    if (vicc_init(vpcdport) < 0) {
        RELAY_ERROR("Could not initialize connection to virtual ICC\n");
        return 0;
    }
    INFO("Waiting for virtual ICC on port %hu\n",
            (unsigned short) vpcdport);


    do {
        switch (vicc_present()) {
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


    if (vicc_poweron() < 0) {
        RELAY_ERROR("could not powerup\n");
        return 0;
    }

    INFO("Connected to virtual ICC\n");


    return 1;
}

static int vpcd_disconnect(driver_data_t *driver_data)
{
    if (vicc_eject() != 0)
        DEBUG("Could not eject virtual ICC\n");

    if (vicc_exit() != 0) {
        RELAY_ERROR("Could not close connection to virtual ICC\n");
        return 0;
    }

    return 1;
}

static int vpcd_transmit(driver_data_t *driver_data,
        const unsigned char *send, size_t send_len,
        unsigned char *recv, size_t *recv_len)
{
    char *rapdu;
    int size = vicc_transmit(send_len, (char *) send, &rapdu);

    if (size < 0) {
        RELAY_ERROR("could not send apdu or receive rapdu\n");
        *recv_len = 0;
        return 0;
    }

    if (*recv_len < size) {
        RELAY_ERROR("Not enough memory for rapdu\n");
        *recv_len = 0;
        free(rapdu);
        return 0;
    }

    *recv_len = size;
    memcpy(recv, rapdu, size);
    free(rapdu);

    return 1;
}


struct sc_driver driver_vpcd = {
    .connect = vpcd_connect,
    .disconnect = vpcd_disconnect,
    .transmit = vpcd_transmit,
};
