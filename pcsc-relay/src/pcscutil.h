/*
 * Copyright (C) 2010-2012 Frank Morgner <morgner@informatik.hu-berlin.de>.
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
#ifndef _PCSCUTIL_H
#define _PCSCUTIL_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <winscard.h>
#ifdef HAVE_PCSCLITE_H
#include <pcsclite.h>
#define stringify_error(r) pcsc_stringify_error(r)
#else
#define stringify_error(r) "PC/SC error code %u\n", (unsigned int) r
#define SCARD_PROTOCOL_ANY (SCARD_PROTOCOL_T0|SCARD_PROTOCOL_T1)
#endif

#ifdef HAVE_READER_H
#include <reader.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

LONG
pcsc_connect(unsigned int readernum, DWORD dwShareMode, DWORD dwPreferredProtocol,
        SCARDCONTEXT *hContext, LPSTR *readers, LPSCARDHANDLE phCard,
        LPDWORD pdwActiveProtocol);

void
pcsc_disconnect(SCARDCONTEXT hContext, SCARDHANDLE hCard, LPSTR readers);

LONG
pcsc_transmit(DWORD dwActiveProtocol, SCARDHANDLE hCard,
        LPCBYTE pbSendBuffer, DWORD cbSendLength,
        LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength);

void
printb(const char *label, unsigned char *buf, size_t len);

#ifdef  __cplusplus
}
#endif
#endif
