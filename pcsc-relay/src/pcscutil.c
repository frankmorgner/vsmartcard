/*
 * Copyright (C) 2010 Frank Morgner
 *
 * This file is part of ccid.
 *
 * ccid is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * ccid is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ccid.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <string.h>

#include "pcscutil.h"

#pragma comment(lib, "winscard.lib")

LONG
pcsc_connect(unsigned int readernum, DWORD dwShareMode, DWORD dwPreferredProtocols,
        LPSCARDCONTEXT phContext, LPSTR *readers, LPSCARDHANDLE phCard,
        LPDWORD pdwActiveProtocol)
{
    LONG r;
    DWORD ctl, readerslen;
    char *reader;
    size_t l, i;

    r = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, phContext);
    if (r != SCARD_S_SUCCESS) {
        fprintf(stderr, "Could not connect to PC/SC Service\n");
        goto err;
    }


    readerslen = SCARD_AUTOALLOCATE;
    r = SCardListReaders(*phContext, NULL, (LPTSTR) readers, &readerslen);
    if (r != SCARD_S_SUCCESS) {
        fprintf(stderr, "Could not get readers\n");
        goto err;
    }

    for (reader = *readers, i = 0; readerslen > 0;
            l = strlen(reader) + 1, readerslen -= l, reader += l, i++) {

        if (i == readernum)
            break;
    }
    if (readerslen <= 0) {
        fprintf(stderr, "Could not find reader number %u\n", readernum);
        r = SCARD_E_UNKNOWN_READER;
        goto err;
    }


    r = SCardConnect(*phContext, reader, dwShareMode, dwPreferredProtocols,
            phCard, pdwActiveProtocol); 
    if (r != SCARD_S_SUCCESS) {
        fprintf(stderr, "Could not connect to %s\n", reader);
        goto err;
    }
    printf("Connected to %s\n", reader);

err:
    return r;
}

void
pcsc_disconnect(SCARDCONTEXT hContext, SCARDHANDLE hCard, LPSTR readers)
{
    SCardDisconnect(hCard, SCARD_LEAVE_CARD);
    SCardFreeMemory(hContext, readers);
    SCardReleaseContext(hContext);
}

LONG
pcsc_transmit(DWORD dwActiveProtocol, SCARDHANDLE hCard,
        LPCBYTE pbSendBuffer, DWORD cbSendLength,
        LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength)
{
    LONG r;
    SCARD_IO_REQUEST ioRecvPci;

    switch (dwActiveProtocol) {
        case SCARD_PROTOCOL_T0:
            r = SCardTransmit(hCard, SCARD_PCI_T0, pbSendBuffer, cbSendLength,
                    &ioRecvPci, pbRecvBuffer, pcbRecvLength);
            break;

        case SCARD_PROTOCOL_T1:
            r = SCardTransmit(hCard, SCARD_PCI_T1, pbSendBuffer, cbSendLength,
                    &ioRecvPci, pbRecvBuffer, pcbRecvLength);
            break;

        default:
            fprintf(stderr, "Could not transmit with unknown protocol %u\n",
                    (unsigned int) dwActiveProtocol);
            r = SCARD_E_PROTO_MISMATCH;
            break;
    }

    return r;
}

void
printb(const char *label, unsigned char *buf, size_t len)
{
    size_t i = 0;
    printf("%s", label);
    while (i < len) {
        printf("%02X", buf[i]);
        i++;
        if (i%20)
            printf(" ");
        else if (i != len)
            printf("\n");
    }
    printf("\n");
}
