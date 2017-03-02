/*
 * Copyright (C) 2010-2012 Frank Morgner <frankmorgner@gmail.com>.
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

#ifdef ENABLE_LIBNFC

#include <nfc/nfc.h>
#include <stdlib.h>
#include <string.h>

struct lnfc_data {
    /* PN53X only supports short APDUs */
    uint8_t abtCapdu[4+1+0xff+1];
    int iCapduLen;
    nfc_device *pndTarget;
    nfc_context *context;
};


#if 0
static size_t get_historical_bytes(unsigned char *atr, size_t atrlen,
        unsigned char **hb)
{
    size_t i, hblen;
    char tdx_present, x;

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
#endif

static int lnfc_connect(driver_data_t **driver_data)
{
    struct lnfc_data *data;
    /* data derived from German (test) identity card issued 2010 */
    nfc_target ntEmulatedTarget = {
        .nti.nai.abtAtqa = {0x00, 0x08},
        .nti.nai.btSak = 0x20,
        .nti.nai.szUidLen = 4,
        .nti.nai.abtUid = {0x08, 0xdb, 0x02, 0xfe},
        .nti.nai.szAtsLen = 8,
        .nti.nai.abtAts = {0x78, 0x77, 0xd4, 0x02, 0x00, 0x00, 0x90, 0x00},
        .nm.nmt = NMT_ISO14443A,
        .nm.nbr = NBR_106,
    };

    /* fix emulation issues. Initialisation taken from nfc-relay-picc */
    // We can only emulate a short UID, so fix length & ATQA bit:
    ntEmulatedTarget.nti.nai.szUidLen = 4;
    ntEmulatedTarget.nti.nai.abtAtqa[1] &= (0xFF-0x40);
    // First byte of UID is always automatically replaced by 0x08 in this mode anyway
    ntEmulatedTarget.nti.nai.abtUid[0] = 0x08;
    ntEmulatedTarget.nti.nai.abtAts[0] = 0x75;
    ntEmulatedTarget.nti.nai.abtAts[1] = 0x33;
    ntEmulatedTarget.nti.nai.abtAts[2] = 0x92;
    ntEmulatedTarget.nti.nai.abtAts[3] = 0x03;

    if (!driver_data)
        return 0;


    data = realloc(*driver_data, sizeof *data);
    if (!data)
        return 0;
    *driver_data = data;


    nfc_init(&data->context);
    if (data->context == NULL) {
        RELAY_ERROR("Error initializing libnfc\n");
        return 0;
    }

    data->pndTarget = nfc_open(data->context, NULL);
    if (data->pndTarget == NULL) {
        RELAY_ERROR("Error connecting to NFC emulator device\n");
        return 0;
    }

    PRINTF("Connected to %s\n", nfc_device_get_name(data->pndTarget));

    DEBUG("Waiting for a command that is not part of the anti-collision...\n");
    data->iCapduLen = nfc_target_init(data->pndTarget, &ntEmulatedTarget, data->abtCapdu, sizeof data->abtCapdu, 0);
    if (data->iCapduLen < 0) {
        RELAY_ERROR("nfc_target_init: %s\n", nfc_strerror(data->pndTarget));
        nfc_close (data->pndTarget);
        return 0;
    }
    DEBUG("Initialized NFC emulator\n");


    return 1;
}

static int lnfc_disconnect(driver_data_t *driver_data)
{
    struct lnfc_data *data = driver_data;


    if (data) {
        if (data->pndTarget) {
            nfc_abort_command(data->pndTarget);
            nfc_close(data->pndTarget);
        }
        if (data->context)
            nfc_exit(data->context);
        free(data);
    }


    return 1;
}

static int lnfc_receive_capdu(driver_data_t *driver_data,
        unsigned char **capdu, size_t *len)
{
    struct lnfc_data *data = driver_data;
    unsigned char *p;

    if (!data || !capdu || !len)
        return 0;


    // Receive external reader command through target
    data->iCapduLen = nfc_target_receive_bytes(data->pndTarget, data->abtCapdu, sizeof data->abtCapdu, 0);
    if (data->iCapduLen < 0) {
        RELAY_ERROR ("nfc_target_receive_bytes: %s\n", nfc_strerror(data->pndTarget));
        return 0;
    }


    p = realloc(*capdu, data->iCapduLen);
    if (!p) {
        RELAY_ERROR("Error allocating memory for C-APDU\n");
        return 0;
    }
    memcpy(p, data->abtCapdu, data->iCapduLen);
    *capdu = p;
    *len = data->iCapduLen;


    return 1;
}

static int lnfc_send_rapdu(driver_data_t *driver_data,
        const unsigned char *rapdu, size_t len)
{
    struct lnfc_data *data = driver_data;
    int r;

    if (!data || !rapdu)
        return 0;


    r = nfc_target_send_bytes(data->pndTarget, rapdu, len, -1);
    if (r < 0) {
        RELAY_ERROR ("nfc_target_send_bytes: %s\n", nfc_strerror(data->pndTarget));
        return 0;
    }
    if (r < len)
        INFO ("Transmitted %u less bytes than desired: %s\n", (unsigned int) len-r, nfc_strerror(data->pndTarget));


    return 1;
}

#else

static int error(void)
{
    RELAY_ERROR("Compiled without support for libnfc\n");
    return 0;
}

static int lnfc_connect(driver_data_t **driver_data)
{
    return error();
}

static int lnfc_disconnect(driver_data_t *driver_data)
{
    return error();
}

static int lnfc_receive_capdu(driver_data_t *driver_data,
        unsigned char **capdu, size_t *len)
{
    return error();
}

static int lnfc_send_rapdu(driver_data_t *driver_data,
        const unsigned char *rapdu, size_t len)
{
    return error();
}

#endif

struct rf_driver driver_libnfc = {
    .connect = lnfc_connect,
    .disconnect = lnfc_disconnect,
    .receive_capdu = lnfc_receive_capdu,
    .send_rapdu = lnfc_send_rapdu,
};
