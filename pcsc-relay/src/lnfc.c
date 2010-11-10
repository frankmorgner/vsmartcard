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
    nfc_target_t ntEmulatedTarget = {
      .nm.nmt = NMT_ISO14443A,
      .nm.nbr = NBR_106,
    };

    if (!driver_data)
        return 0;


    data = realloc(*driver_data, sizeof *data);
    if (!data)
        return 0;
    memset(data, 0, sizeof *data);
    *driver_data = data;

    /* FIXME
     * ntEmulatedTarget.nti.nai.abtUid
     * ntEmulatedTarget.nti.nai.abtAtqa
     * ntEmulatedTarget.nti.nai.btSak
     * ntEmulatedTarget.nti.nai.abtAts
     */
    // We can only emulate a short UID, so fix length & ATQA bit:
    ntEmulatedTarget.nti.nai.szUidLen = 4;
    ntEmulatedTarget.nti.nai.abtAtqa[1] &= (0xFF-0x40);
    // First byte of UID is always automatically replaced by 0x08 in this mode anyway
    ntEmulatedTarget.nti.nai.abtUid[0] = 0x08;
    // ATS is always automatically replaced by PN532, we've no control on it:
    // ATS = (05) 75 33 92 03
    //       (TL) T0 TA TB TC
    //             |  |  |  +-- CID supported, NAD supported
    //             |  |  +----- FWI=9 SFGI=2 => FWT=154ms, SFGT=1.21ms
    //             |  +-------- DR=2,4 DS=2,4 => supports 106, 212 & 424bps in both directions
    //             +----------- TA,TB,TC, FSCI=5 => FSC=64
    // It seems hazardous to tell we support NAD if the tag doesn't support NAD but I don't know how to disable it
    // PC/SC pseudo-ATR = 3B 80 80 01 01 if there is no historical bytes

    // Creates ATS and copy max 48 bytes of Tk:
    /*
    byte_t * pbtTk;
    size_t szTk;
    pbtTk = iso14443a_locate_historical_bytes (ntEmulatedTarget.nti.nai.abtAts, ntEmulatedTarget.nti.nai.szAtsLen, &szTk);
    szTk = (szTk > 48) ? 48 : szTk;
    byte_t pbtTkt[48];
    memcpy(pbtTkt, pbtTk, szTk);
    ntEmulatedTarget.nti.nai.abtAts[0] = 0x75;
    ntEmulatedTarget.nti.nai.abtAts[1] = 0x33;
    ntEmulatedTarget.nti.nai.abtAts[2] = 0x92;
    ntEmulatedTarget.nti.nai.abtAts[3] = 0x03;
    ntEmulatedTarget.nti.nai.szAtsLen = 4 + szTk;
    memcpy(&(ntEmulatedTarget.nti.nai.abtAts[4]), pbtTkt, szTk);
    */

    // Try to open the NFC emulator device
    data->pndTarget = nfc_connect (NULL);
    if (data->pndTarget == NULL) {
        if (verbose || debug)
            fprintf (stderr, "Error connecting NFC emulator device\n");
        return 0;
    }

    if (verbose)
        printf ("Connected to the NFC emulator device: %s\n", data->pndTarget->acName);

    if (!nfc_target_init (data->pndTarget, &ntEmulatedTarget, data->abtCapdu, &data->szCapduLen)) {
        if (verbose || debug)
            fprintf (stderr, "%s", "Initialization of NFC emulator failed");
        nfc_disconnect (data->pndTarget);
        return 0;
    }
    if (debug)
        printf ("%s\n", "Done, relaying frames now!");


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
        if (verbose || debug)
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


    if (!nfc_target_send_bytes(data->pndTarget, rapdu, len))
        nfc_perror (data->pndTarget, "nfc_target_send_bytes");


    return 1;
}


struct rf_driver driver_libnfc = {
    .data = NULL,
    .connect = lnfc_connect,
    .disconnect = lnfc_disconnect,
    .receive_capdu = lnfc_receive_capdu,
    .send_rapdu = lnfc_send_rapdu,
};
