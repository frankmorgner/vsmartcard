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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pcsc-relay.h"

#include <stdlib.h>
#include <string.h>
#include <winscard.h>

#ifdef HAVE_PCSCLITE_H
#include <pcsclite.h>
#define stringify_error(r) pcsc_stringify_error(r)
#else
#define stringify_error(r) "PC/SC error code %u\n", (unsigned int) r
#define SCARD_PROTOCOL_ANY (SCARD_PROTOCOL_T0|SCARD_PROTOCOL_T1)
#endif

#ifdef HAVE_WINTYPES_H
#include <wintypes.h>
#endif

#ifdef HAVE_READER_H
#include <reader.h>
#endif

#define SHAREMODE SCARD_SHARE_EXCLUSIVE
#define PREFERREDPROTOCOL SCARD_PROTOCOL_ANY


struct pcsc_data {
    LPSTR readers;
    SCARDCONTEXT hContext;
    SCARDHANDLE hCard;
    DWORD dwActiveProtocol;
};



#define READERNUM_AUTODETECT -1
unsigned int readernum = READERNUM_AUTODETECT;


static int pcsc_connect(driver_data_t **driver_data)
{
    struct pcsc_data *data;

    LONG r;
    DWORD readerslen;
    SCARD_READERSTATE state;
    LPSTR readers = NULL;
    char *reader;
    size_t l, i;

    if (!driver_data)
        return 0;


    data = realloc(*driver_data, sizeof *data);
    if (!data)
        return 0;
    data->readers = NULL;
    data->hContext = 0;
    data->hCard = 0;
    *driver_data = data;


    r = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &data->hContext);
    if (r != SCARD_S_SUCCESS) {
        RELAY_ERROR("Could not connect to PC/SC Service\n");
        goto err;
    }


#ifdef SCARD_AUTOALLOCATE
    readerslen = SCARD_AUTOALLOCATE;
    r = SCardListReaders(data->hContext, NULL, (LPSTR) &readers, &readerslen);
#else
    r = SCardListReaders(data->hContext, NULL, NULL, &readerslen);
    if (r != SCARD_S_SUCCESS) {
        RELAY_ERROR("Could not get readers length\n");
        goto err;
    }
    readers = malloc(readerslen);
    if (readers == NULL) {
        RELAY_ERROR("Could not get memory\n");
        goto err;
    }
    r = SCardListReaders(data->hContext, NULL, readers, &readerslen);
#endif
    if (r != SCARD_S_SUCCESS) {
        RELAY_ERROR("Could not get readers\n");
        goto err;
    }
    data->readers = readers;

    for (reader = readers, i = 0; readerslen > 0;
            l = strlen(reader)+1, readerslen -= l, reader += l, i++) {

        if (i == readernum || readernum == READERNUM_AUTODETECT) {
            state.szReader = reader;
            state.dwCurrentState = SCARD_STATE_UNAWARE;

            r = SCardGetStatusChange(data->hContext, 0, &state, 1);
            if (r != SCARD_S_SUCCESS) {
                RELAY_ERROR("Could not get status of %s\n", reader);
                goto err;
            }

            if (state.dwEventState & SCARD_STATE_PRESENT)
                break;

            if (i == readernum) {
                RELAY_ERROR("No card present in %s\n", reader);
                goto err;
            }
        }
    }
    if (readerslen <= 0) {
        if (readernum == READERNUM_AUTODETECT) {
            RELAY_ERROR("Could not find a reader with a card\n");
            r = SCARD_E_NO_SMARTCARD;
        } else {
            RELAY_ERROR("Could not find reader number %u\n", readernum);
            r = SCARD_E_UNKNOWN_READER;
        }
        goto err;
    }


    r = SCardConnect(data->hContext, reader, SHAREMODE, PREFERREDPROTOCOL,
            &data->hCard, &data->dwActiveProtocol); 
    if (r != SCARD_S_SUCCESS) {
        RELAY_ERROR("Could not connect to %s\n", reader);
        goto err;
    }
    INFO("Connected to reader %zu: %s\n", i, reader);
    hexdump("Card's ATR: ", state.rgbAtr, state.cbAtr);

err:
    if (r != SCARD_S_SUCCESS) {
        RELAY_ERROR("%s\n", stringify_error(r));
        return 0;
    }

    return 1;
}

static int pcsc_disconnect(driver_data_t *driver_data)
{
    struct pcsc_data *data = driver_data;

    if (data) {
        SCardDisconnect(data->hCard, SCARD_LEAVE_CARD);
#ifdef SCARD_AUTOALLOCATE
        SCardFreeMemory(data->hContext, data->readers);
#else
        free(data->readers);
#endif
        SCardReleaseContext(data->hContext);
        free(data);
    }

    return 1;
}

static int pcsc_transmit(driver_data_t *driver_data,
        const unsigned char *send, size_t send_len,
        unsigned char *recv, size_t *recv_len)
{
    struct pcsc_data *data = driver_data;
    LPCBYTE pbSendBuffer = send;
    DWORD cbSendLength = send_len;
    LPBYTE pbRecvBuffer = recv;
    DWORD cbRecvLength = *recv_len;

    LONG r;
    SCARD_IO_REQUEST ioRecvPci;

    switch (data->dwActiveProtocol) {
        case SCARD_PROTOCOL_T0:
            r = SCardTransmit(data->hCard, SCARD_PCI_T0, pbSendBuffer, cbSendLength,
                    &ioRecvPci, pbRecvBuffer, &cbRecvLength);
            break;

        case SCARD_PROTOCOL_T1:
            r = SCardTransmit(data->hCard, SCARD_PCI_T1, pbSendBuffer, cbSendLength,
                    &ioRecvPci, pbRecvBuffer, &cbRecvLength);
            break;

        default:
            RELAY_ERROR("Could not transmit with unknown protocol %u\n",
                    (unsigned int) data->dwActiveProtocol);
            r = SCARD_E_PROTO_MISMATCH;
            break;
    }

    if (r != SCARD_S_SUCCESS) {
        RELAY_ERROR("%s\n", stringify_error(r));
        return 0;
    }

    *recv_len = cbRecvLength;

    return 1;
}


struct sc_driver driver_pcsc = {
    .connect = pcsc_connect,
    .disconnect = pcsc_disconnect,
    .transmit = pcsc_transmit,
};
