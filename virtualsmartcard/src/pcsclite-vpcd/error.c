/*
 * MUSCLE SmartCard Development ( http://pcsclite.alioth.debian.org/pcsclite.html )
 *
 * Copyright (C) 1999-2002
 *  David Corcoran <corcoran@musclecard.com>
 * Copyright (C) 2006-2009
 *  Ludovic Rousseau <ludovic.rousseau@free.fr>
 *
 * This file is dual licenced:
 * - BSD-like, see the COPYING file
 * - GNU Lesser General Licence 2.1 or (at your option) any later version.
 *
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id$
 */

/**
 * @file
 * @brief This handles pcsc_stringify_error()
 */

#include <stdio.h>
#include <sys/types.h>

#include "config.h"
#include "misc.h"
#include "pcsclite.h"
#include "string.h"

#ifdef NO_LOG
PCSC_API char* pcsc_stringify_error(const LONG pcscError)
{
	static char strError[] = "0x12345678";
	unsigned long error = pcscError;

	snprintf(strError, sizeof(strError), "0x%08lX", error);

	return strError;
}
#else
/**
 * @brief Returns a human readable text for the given PC/SC error code.
 *
 * @ingroup API
 * @param[in] pcscError Error code to be translated to text.
 *
 * @return Text representing the error code passed.
 *
 * @code
 * SCARDCONTEXT hContext;
 * LONG rv;
 * rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);
 * if (rv != SCARD_S_SUCCESS)
 *     printf("SCardEstablishContext: %s (0x%lX)\n",
 *         pcsc_stringify_error(rv), rv);
 * @endcode
 */
PCSC_API char* pcsc_stringify_error(const LONG pcscError)
{
	static char strError[75];
	const char *msg = NULL;

	switch (pcscError)
	{
	case SCARD_S_SUCCESS:
		msg = "Command successful.";
		break;
	case SCARD_F_INTERNAL_ERROR:
		msg = "Internal error.";
		break;
	case SCARD_E_CANCELLED:
		msg = "Command cancelled.";
		break;
	case SCARD_E_INVALID_HANDLE:
		msg = "Invalid handle.";
		break;
	case SCARD_E_INVALID_PARAMETER:
		msg = "Invalid parameter given.";
		break;
	case SCARD_E_INVALID_TARGET:
		msg = "Invalid target given.";
		break;
	case SCARD_E_NO_MEMORY:
		msg = "Not enough memory.";
		break;
	case SCARD_F_WAITED_TOO_LONG:
		msg = "Waited too long.";
		break;
	case SCARD_E_INSUFFICIENT_BUFFER:
		msg = "Insufficient buffer.";
		break;
	case SCARD_E_UNKNOWN_READER:
		msg = "Unknown reader specified.";
		break;
	case SCARD_E_TIMEOUT:
		msg = "Command timeout.";
		break;
	case SCARD_E_SHARING_VIOLATION:
		msg = "Sharing violation.";
		break;
	case SCARD_E_NO_SMARTCARD:
		msg = "No smart card inserted.";
		break;
	case SCARD_E_UNKNOWN_CARD:
		msg = "Unknown card.";
		break;
	case SCARD_E_CANT_DISPOSE:
		msg = "Cannot dispose handle.";
		break;
	case SCARD_E_PROTO_MISMATCH:
		msg = "Card protocol mismatch.";
		break;
	case SCARD_E_NOT_READY:
		msg = "Subsystem not ready.";
		break;
	case SCARD_E_INVALID_VALUE:
		msg = "Invalid value given.";
		break;
	case SCARD_E_SYSTEM_CANCELLED:
		msg = "System cancelled.";
		break;
	case SCARD_F_COMM_ERROR:
		msg = "RPC transport error.";
		break;
	case SCARD_F_UNKNOWN_ERROR:
		msg = "Unknown error.";
		break;
	case SCARD_E_INVALID_ATR:
		msg = "Invalid ATR.";
		break;
	case SCARD_E_NOT_TRANSACTED:
		msg = "Transaction failed.";
		break;
	case SCARD_E_READER_UNAVAILABLE:
		msg = "Reader is unavailable.";
		break;
	/* case SCARD_P_SHUTDOWN: */
	case SCARD_E_PCI_TOO_SMALL:
		msg = "PCI struct too small.";
		break;
	case SCARD_E_READER_UNSUPPORTED:
		msg = "Reader is unsupported.";
		break;
	case SCARD_E_DUPLICATE_READER:
		msg = "Reader already exists.";
		break;
	case SCARD_E_CARD_UNSUPPORTED:
		msg = "Card is unsupported.";
		break;
	case SCARD_E_NO_SERVICE:
		msg = "Service not available.";
		break;
	case SCARD_E_SERVICE_STOPPED:
		msg = "Service was stopped.";
		break;
	/* case SCARD_E_UNEXPECTED: */
	/* case SCARD_E_ICC_CREATEORDER: */
	/* case SCARD_E_UNSUPPORTED_FEATURE: */
	/* case SCARD_E_DIR_NOT_FOUND: */
	/* case SCARD_E_NO_DIR: */
	/* case SCARD_E_NO_FILE: */
	/* case SCARD_E_NO_ACCESS: */
	/* case SCARD_E_WRITE_TOO_MANY: */
	/* case SCARD_E_BAD_SEEK: */
	/* case SCARD_E_INVALID_CHV: */
	/* case SCARD_E_UNKNOWN_RES_MNG: */
	/* case SCARD_E_NO_SUCH_CERTIFICATE: */
	/* case SCARD_E_CERTIFICATE_UNAVAILABLE: */
	case SCARD_E_NO_READERS_AVAILABLE:
		msg = "Cannot find a smart card reader.";
		break;
	/* case SCARD_E_COMM_DATA_LOST: */
	/* case SCARD_E_NO_KEY_CONTAINER: */
	/* case SCARD_E_SERVER_TOO_BUSY: */
	case SCARD_W_UNSUPPORTED_CARD:
		msg = "Card is not supported.";
		break;
	case SCARD_W_UNRESPONSIVE_CARD:
		msg = "Card is unresponsive.";
		break;
	case SCARD_W_UNPOWERED_CARD:
		msg = "Card is unpowered.";
		break;
	case SCARD_W_RESET_CARD:
		msg = "Card was reset.";
		break;
	case SCARD_W_REMOVED_CARD:
		msg = "Card was removed.";
		break;
	/* case SCARD_W_SECURITY_VIOLATION: */
	/* case SCARD_W_WRONG_CHV: */
	/* case SCARD_W_CHV_BLOCKED: */
	/* case SCARD_W_EOF: */
	/* case SCARD_W_CANCELLED_BY_USER: */
	/* case SCARD_W_CARD_NOT_AUTHENTICATED: */

	case SCARD_E_UNSUPPORTED_FEATURE:
		msg = "Feature not supported.";
		break;
	default:
		(void)snprintf(strError, sizeof(strError)-1, "Unknown error: 0x%08lX",
			pcscError);
	};

	if (msg)
		(void)strncpy(strError, msg, sizeof(strError));
	else
		(void)snprintf(strError, sizeof(strError)-1, "Unknown error: 0x%08lX",
			pcscError);

	/* add a null byte */
	strError[sizeof(strError)-1] = '\0';

	return strError;
}
#endif

