/*
 * MUSCLE SmartCard Development ( http://www.linuxnet.com )
 *
 * Copyright (C) 1999-2002
 *  David Corcoran <corcoran@linuxnet.com>
 * Copyright (C) 2006-2009
 *  Ludovic Rousseau <ludovic.rousseau@free.fr>
 *
 * This file is dual licenced:
 * - BSD-like, see the COPYING file
 * - GNU Lesser General Licence 2.1 or (at your option) any later version.
 *
 * $Id: error.c 5964 2011-09-24 09:22:24Z rousseau $
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
#include "strlcpycat.h"

#ifdef NO_LOG
PCSC_API char* pcsc_stringify_error(const long pcscError)
{
	static char strError[] = "0x12345678";

	snprintf(strError, sizeof(strError), "0x%08lX", pcscError);

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

	switch (pcscError)
	{
	case SCARD_S_SUCCESS:
		(void)strlcpy(strError, "Command successful.", sizeof(strError));
		break;
	case SCARD_F_INTERNAL_ERROR:
		(void)strlcpy(strError, "Internal error.", sizeof(strError));
		break;
	case SCARD_E_CANCELLED:
		(void)strlcpy(strError, "Command cancelled.", sizeof(strError));
		break;
	case SCARD_E_INVALID_HANDLE:
		(void)strlcpy(strError, "Invalid handle.", sizeof(strError));
		break;
	case SCARD_E_INVALID_PARAMETER:
		(void)strlcpy(strError, "Invalid parameter given.", sizeof(strError));
		break;
	case SCARD_E_INVALID_TARGET:
		(void)strlcpy(strError, "Invalid target given.", sizeof(strError));
		break;
	case SCARD_E_NO_MEMORY:
		(void)strlcpy(strError, "Not enough memory.", sizeof(strError));
		break;
	case SCARD_F_WAITED_TOO_LONG:
		(void)strlcpy(strError, "Waited too long.", sizeof(strError));
		break;
	case SCARD_E_INSUFFICIENT_BUFFER:
		(void)strlcpy(strError, "Insufficient buffer.", sizeof(strError));
		break;
	case SCARD_E_UNKNOWN_READER:
		(void)strlcpy(strError, "Unknown reader specified.", sizeof(strError));
		break;
	case SCARD_E_TIMEOUT:
		(void)strlcpy(strError, "Command timeout.", sizeof(strError));
		break;
	case SCARD_E_SHARING_VIOLATION:
		(void)strlcpy(strError, "Sharing violation.", sizeof(strError));
		break;
	case SCARD_E_NO_SMARTCARD:
		(void)strlcpy(strError, "No smart card inserted.", sizeof(strError));
		break;
	case SCARD_E_UNKNOWN_CARD:
		(void)strlcpy(strError, "Unknown card.", sizeof(strError));
		break;
	case SCARD_E_CANT_DISPOSE:
		(void)strlcpy(strError, "Cannot dispose handle.", sizeof(strError));
		break;
	case SCARD_E_PROTO_MISMATCH:
		(void)strlcpy(strError, "Card protocol mismatch.", sizeof(strError));
		break;
	case SCARD_E_NOT_READY:
		(void)strlcpy(strError, "Subsystem not ready.", sizeof(strError));
		break;
	case SCARD_E_INVALID_VALUE:
		(void)strlcpy(strError, "Invalid value given.", sizeof(strError));
		break;
	case SCARD_E_SYSTEM_CANCELLED:
		(void)strlcpy(strError, "System cancelled.", sizeof(strError));
		break;
	case SCARD_F_COMM_ERROR:
		(void)strlcpy(strError, "RPC transport error.", sizeof(strError));
		break;
	case SCARD_F_UNKNOWN_ERROR:
		(void)strlcpy(strError, "Unknown error.", sizeof(strError));
		break;
	case SCARD_E_INVALID_ATR:
		(void)strlcpy(strError, "Invalid ATR.", sizeof(strError));
		break;
	case SCARD_E_NOT_TRANSACTED:
		(void)strlcpy(strError, "Transaction failed.", sizeof(strError));
		break;
	case SCARD_E_READER_UNAVAILABLE:
		(void)strlcpy(strError, "Reader is unavailable.", sizeof(strError));
		break;
	/* case SCARD_P_SHUTDOWN: */
	case SCARD_E_PCI_TOO_SMALL:
		(void)strlcpy(strError, "PCI struct too small.", sizeof(strError));
		break;
	case SCARD_E_READER_UNSUPPORTED:
		(void)strlcpy(strError, "Reader is unsupported.", sizeof(strError));
		break;
	case SCARD_E_DUPLICATE_READER:
		(void)strlcpy(strError, "Reader already exists.", sizeof(strError));
		break;
	case SCARD_E_CARD_UNSUPPORTED:
		(void)strlcpy(strError, "Card is unsupported.", sizeof(strError));
		break;
	case SCARD_E_NO_SERVICE:
		(void)strlcpy(strError, "Service not available.", sizeof(strError));
		break;
	case SCARD_E_SERVICE_STOPPED:
		(void)strlcpy(strError, "Service was stopped.", sizeof(strError));
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
		(void)strlcpy(strError, "Cannot find a smart card reader.", sizeof(strError));
		break;
	/* case SCARD_E_COMM_DATA_LOST: */
	/* case SCARD_E_NO_KEY_CONTAINER: */
	/* case SCARD_E_SERVER_TOO_BUSY: */
	case SCARD_W_UNSUPPORTED_CARD:
		(void)strlcpy(strError, "Card is not supported.", sizeof(strError));
		break;
	case SCARD_W_UNRESPONSIVE_CARD:
		(void)strlcpy(strError, "Card is unresponsive.", sizeof(strError));
		break;
	case SCARD_W_UNPOWERED_CARD:
		(void)strlcpy(strError, "Card is unpowered.", sizeof(strError));
		break;
	case SCARD_W_RESET_CARD:
		(void)strlcpy(strError, "Card was reset.", sizeof(strError));
		break;
	case SCARD_W_REMOVED_CARD:
		(void)strlcpy(strError, "Card was removed.", sizeof(strError));
		break;
	/* case SCARD_W_SECURITY_VIOLATION: */
	/* case SCARD_W_WRONG_CHV: */
	/* case SCARD_W_CHV_BLOCKED: */
	/* case SCARD_W_EOF: */
	/* case SCARD_W_CANCELLED_BY_USER: */
	/* case SCARD_W_CARD_NOT_AUTHENTICATED: */

	case SCARD_E_UNSUPPORTED_FEATURE:
		(void)strlcpy(strError, "Feature not supported.", sizeof(strError));
		break;
	default:
		(void)snprintf(strError, sizeof(strError)-1, "Unknown error: 0x%08lX",
			pcscError);
	};

	/* add a null byte */
	strError[sizeof(strError)-1] = '\0';

	return strError;
}
#endif

