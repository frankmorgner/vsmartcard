/*
 * Copyright (C) 2010 Frank Morgner
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
#include <nfc/nfc.h>
#include <stdlib.h>
#include <string.h>

#include "pcsc-relay.h"

struct lnfc_data {
    /* PN53X only supports short APDUs */
    byte_t abtCapdu[4+1+0xff+1];
    size_t szCapduLen;
    nfc_device_t *pndTarget;
};


static size_t get_historical_bytes(unsigned char *atr, size_t atrlen,
        unsigned char **hb)
{
    size_t i, hblen;
    char tax_present, tbx_present, tcx_present, tdx_present, x;

    if (!atr || !hb)
        return 0;


    /* TS - The Initial Character */
    i = 0;
    if (i >= atrlen)
        return 0;


    /* T0 - The Format Character */
    i++;
    if (i >= atrlen)
        return 0;
    hblen = atr[i] & 0xf;
    /* T0 is a specieal case of TD */
    x = 0;
    tdx_present = 1;


    /* TA1, TB1, TC1, TD1... - The Interface Characters */
    while (x <= 4 && tdx_present) {
        if (i >= atrlen)
            return 0;

        if (atr[i] & 0x80) {
            /* TAX is present */
            i++;
            if (i >= atrlen)
                return 0;
        }
        if (atr[i] & 0x40) {
            /* TBX is present */
            i++;
            if (i >= atrlen)
                return 0;
        }
        if (atr[i] & 0x20) {
            /* TCX is present */
            i++;
            if (i >= atrlen)
                return 0;
        }
        if (!(atr[i] & 0x10)) {
            /* TDX is NOT present */
            tdx_present = 0;
        }

        x++;
        i++;
        if (i >= atrlen)
            return 0;
    }

    if (i + hblen > atrlen)
        return 0;

    *hb = atr + i;

    return hblen;
}


static int lnfc_connect(void **driver_data)
{
    struct lnfc_data *data;
    /* data derived from German (test) identity card issued 2010 */
    nfc_target_t ntEmulatedTarget = {
        .nti.nai.abtAtqa = {0x00, 0x08},
        .nti.nai.btSak = 0x20,
        .nti.nai.szUidLen = 4,
        .nti.nai.abtUid = {0x08, 0xdb, 0x02, 0xfe},
        .nti.nai.szAtsLen = 8,
        .nti.nai.abtAts = {0x78, 0x77, 0xd4, 0x02, 0x00, 0x00, 0x90, 0x00},
        .nm.nmt = NMT_ISO14443A,
        .nm.nbr = NBR_106,
    };

    if (!driver_data)
        return 0;


    data = realloc(*driver_data, sizeof *data);
    if (!data)
        return 0;
    *driver_data = data;


    // Try to open the NFC emulator device
    data->pndTarget = nfc_connect (NULL);
    if (data->pndTarget == NULL) {
        ERROR("Error connecting NFC emulator device\n");
        return 0;
    }

    INFO("Connected to the NFC emulator device: %s\n", data->pndTarget->acName);

    if (!nfc_target_init (data->pndTarget, &ntEmulatedTarget, data->abtCapdu, &data->szCapduLen)) {
        ERROR("Initialization of NFC emulator failed");
        nfc_disconnect (data->pndTarget);
        return 0;
    }
    DEBUG("%s\n", "Done, relaying frames now!");


    return 1;
}

static int lnfc_disconnect(void *driver_data)
{
    struct lnfc_data *data = driver_data;


    nfc_disconnect (data->pndTarget);


    return 1;
}

static int lnfc_receive_capdu(void *driver_data,
        unsigned char **capdu, size_t *len)
{
    struct lnfc_data *data = driver_data;

    if (!data || !capdu || !len)
        return 0;


    // Receive external reader command through target
    if (!nfc_target_receive_bytes(data->pndTarget, data->abtCapdu, &data->szCapduLen)) {
        if (verbose >= 0)
            nfc_perror (data->pndTarget, "nfc_target_receive_bytes");
        return 0;
    }


    return 1;
}

static int lnfc_send_rapdu(void *driver_data,
        const unsigned char *rapdu, size_t len)
{
    struct lnfc_data *data = driver_data;

    if (!data || !rapdu)
        return 0;


    if (!nfc_target_send_bytes(data->pndTarget, rapdu, len)) {
        if (verbose >= 0)
            nfc_perror (data->pndTarget, "nfc_target_send_bytes");
        return 0;
    }


    return 1;
}


struct rf_driver driver_libnfc = {
    .data = NULL,
    .connect = lnfc_connect,
    .disconnect = lnfc_disconnect,
    .receive_capdu = lnfc_receive_capdu,
    .send_rapdu = lnfc_send_rapdu,
};