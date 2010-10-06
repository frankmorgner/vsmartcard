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

LONG
pcsc_connect(unsigned int readernum, DWORD dwShareMode, DWORD dwPreferredProtocol,
        SCARDCONTEXT *hContext, LPSTR *readers, LPSCARDHANDLE phCard)
{
    LONG r;
    DWORD ctl, readerslen;
    char *reader;
    size_t l, i;

    r = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);
    if (r != SCARD_S_SUCCESS) {
        fprintf(stderr, "Could not connect to PC/SC Service\n");
        goto err;
    }


    readerslen = SCARD_AUTOALLOCATE;
    r = SCardListReaders(hContext, NULL, (LPSTR) readers, &readerslen);
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


    r = SCardConnect(hContext, reader, dwShareMode, dwPreferredProtocol,
            phCard, &ctl); 
    if (r != SCARD_S_SUCCESS) {
        fprintf(stderr, "Could not connect to %s\n", reader);
        goto err;
    }
    printf("Connected to %s\n", reader);

err:
    return r;
}
