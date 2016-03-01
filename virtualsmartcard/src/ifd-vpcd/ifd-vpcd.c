/*
 * Copyright (C) 2010-2013 Frank Morgner
 *
 * This file is part of virtualsmartcard.
 *
 * virtualsmartcard is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * virtualsmartcard is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * virtualsmartcard.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ifd-vpcd.h"
#include "vpcd.h"

#include <wintypes.h>

#include <errno.h>
#include <ifdhandler.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* pcscd allows at most 16 readers. Apple's SmartCardServices on OS X 10.10
 * freaks out if more than 8 slots are registered. We want only two slots... */
#define VICC_MAX_SLOTS (VPCDSLOTS <= PCSCLITE_MAX_READERS_CONTEXTS ? VPCDSLOTS : PCSCLITE_MAX_READERS_CONTEXTS)
const unsigned char vicc_max_slots = VICC_MAX_SLOTS;

#ifdef HAVE_DEBUGLOG_H

#include <debuglog.h>

#else

enum {
	PCSC_LOG_DEBUG = 0,
	PCSC_LOG_INFO,
	PCSC_LOG_ERROR,
	PCSC_LOG_CRITICAL
};

#ifdef HAVE_SYSLOG_H

#include <stdarg.h>
#include <syslog.h>

void log_msg(const int priority, const char *fmt, ...)
{
	char debug_buffer[160]; /* up to 2 lines of 80 characters */
	va_list argptr;
	int syslog_level;

	switch(priority) {
		case PCSC_LOG_CRITICAL:
			syslog_level = LOG_CRIT;
			break;
		case PCSC_LOG_ERROR:
			syslog_level = LOG_ERR;
			break;
		case PCSC_LOG_INFO:
			syslog_level = LOG_INFO;
			break;
		default:
			syslog_level = LOG_DEBUG;
	}

	va_start(argptr, fmt);
	(void)vsnprintf(debug_buffer, sizeof debug_buffer, fmt, argptr);
	va_end(argptr);

	syslog(syslog_level, "%s", debug_buffer);
}
#define Log0(priority) log_msg(priority, "%s:%d:%s()", __FILE__, __LINE__, __FUNCTION__)
#define Log1(priority, fmt) log_msg(priority, "%s:%d:%s() " fmt, __FILE__, __LINE__, __FUNCTION__)
#define Log2(priority, fmt, data) log_msg(priority, "%s:%d:%s() " fmt, __FILE__, __LINE__, __FUNCTION__, data)
#define Log3(priority, fmt, data1, data2) log_msg(priority, "%s:%d:%s() " fmt, __FILE__, __LINE__, __FUNCTION__, data1, data2)
#define Log4(priority, fmt, data1, data2, data3) log_msg(priority, "%s:%d:%s() " fmt, __FILE__, __LINE__, __FUNCTION__, data1, data2, data3)
#define Log5(priority, fmt, data1, data2, data3, data4) log_msg(priority, "%s:%d:%s() " fmt, __FILE__, __LINE__, __FUNCTION__, data1, data2, data3, data4)
#define Log9(priority, fmt, data1, data2, data3, data4, data5, data6, data7, data8) log_msg(priority, "%s:%d:%s() " fmt, __FILE__, __LINE__, __FUNCTION__, data1, data2, data3, data4, data5, data6, data7, data8)

#else

#define Log0(priority) do { } while(0)
#define Log1(priority, fmt) do { } while(0)
#define Log2(priority, fmt, data) do { } while(0)
#define Log3(priority, fmt, data1, data2) do { } while(0)
#define Log4(priority, fmt, data1, data2, data3) do { } while(0)
#define Log5(priority, fmt, data1, data2, data3, data4) do { } while(0)
#define Log9(priority, fmt, data1, data2, data3, data4, data5, data6, data7, data8) do { } while(0)

#endif
#endif

static struct vicc_ctx *ctx[VICC_MAX_SLOTS];
const char *hostname = NULL;
static const char openport[] = "/dev/null";

RESPONSECODE
IFDHCreateChannel (DWORD Lun, DWORD Channel)
{
    size_t slot = Lun & 0xffff;
    if (slot >= vicc_max_slots) {
        return IFD_COMMUNICATION_ERROR;
    }
    if (!hostname)
        Log2(PCSC_LOG_INFO, "Waiting for virtual ICC on port %hu",
                (unsigned short) (Channel+slot));
    ctx[slot] = vicc_init(hostname, Channel+slot);
    if (!ctx[slot]) {
        Log1(PCSC_LOG_ERROR, "Could not initialize connection to virtual ICC");
        return IFD_COMMUNICATION_ERROR;
    }
    if (hostname)
        Log3(PCSC_LOG_INFO, "Connected to virtual ICC on %s port %hu",
                hostname, (unsigned short) (Channel+slot));

    return IFD_SUCCESS;
}

RESPONSECODE
IFDHCreateChannelByName (DWORD Lun, LPSTR DeviceName)
{
    RESPONSECODE r = IFD_NOT_SUPPORTED;
    char *dots;
    char _hostname[MAX_READERNAME];
    size_t hostname_len;
    unsigned long int port = VPCDPORT;

    dots = strchr(DeviceName, ':');
    if (dots) {
        /* a port has been specified behind the device name */

        hostname_len = dots - DeviceName;
        if (strlen(openport) != hostname_len
                || strncmp(DeviceName, openport, hostname_len) != 0) {
            /* a hostname other than /dev/null has been specified,
             * so we connect initialize hostname to connect to vicc */
            if (hostname_len < sizeof _hostname)
                memcpy(_hostname, DeviceName, hostname_len);
            else {
                Log3(PCSC_LOG_ERROR, "Not enough memory to hold hostname (have %zu, need %zu)", sizeof _hostname, hostname_len);
                goto err;
            }
            _hostname[hostname_len] = '\0';
            hostname = _hostname;
        }

        /* skip the ':' */
        dots++;

        errno = 0;
        port = strtoul(dots, NULL, 0);
        if (errno) {
            Log2(PCSC_LOG_ERROR, "Could not parse port: %s", dots);
            goto err;
        }
    } else {
        Log1(PCSC_LOG_INFO, "Using default port.");
    }

    r = IFDHCreateChannel (Lun, port);

err:
    /* set hostname back to default in case it has been changed */
    hostname = NULL;

    return r;
}

RESPONSECODE
IFDHControl (DWORD Lun, DWORD dwControlCode, PUCHAR TxBuffer, DWORD TxLength,
        PUCHAR RxBuffer, DWORD RxLength, LPDWORD pdwBytesReturned)
{
    Log9(PCSC_LOG_DEBUG, "IFDHControl not supported (Lun=%u ControlCode=%u TxBuffer=%p TxLength=%u RxBuffer=%p RxLength=%u pBytesReturned=%p)%s",
            (unsigned int) Lun, (unsigned int) dwControlCode,
            (unsigned char *) TxBuffer, (unsigned int) TxLength,
            (unsigned char *) RxBuffer, (unsigned int) RxLength,
            (unsigned int *) pdwBytesReturned, "");
    if (pdwBytesReturned)
        *pdwBytesReturned = 0;
    return IFD_ERROR_NOT_SUPPORTED;
}

RESPONSECODE
IFDHCloseChannel (DWORD Lun)
{
    size_t slot = Lun & 0xffff;
    if (slot >= vicc_max_slots) {
        return IFD_COMMUNICATION_ERROR;
    }
    if (vicc_exit(ctx[slot]) < 0) {
        Log1(PCSC_LOG_ERROR, "Could not close connection to virtual ICC");
        return IFD_COMMUNICATION_ERROR;
    }
    ctx[slot] = NULL;

    return IFD_SUCCESS;
}

RESPONSECODE
IFDHGetCapabilities (DWORD Lun, DWORD Tag, PDWORD Length, PUCHAR Value)
{
    unsigned char *atr = NULL;
    ssize_t size;
    size_t slot = Lun & 0xffff;
    RESPONSECODE r = IFD_COMMUNICATION_ERROR;

    if (slot >= vicc_max_slots)
        goto err;

    if (!Length || !Value)
        goto err;

    switch (Tag) {
        case TAG_IFD_ATR:

            size = vicc_getatr(ctx[slot], &atr);
            if (size < 0) {
                Log1(PCSC_LOG_ERROR, "could not get ATR");
                goto err;
            }
            if (size == 0) {
                Log1(PCSC_LOG_ERROR, "Virtual ICC removed");
                goto err;
            }
            Log2(PCSC_LOG_DEBUG, "Got ATR (%zd bytes)", size);

#ifndef __APPLE__
            if (*Length < size) {
#else
            /* Apple's new SmartCardServices on OS X 10.10 doesn't set the
             * length correctly so we only check for the maximum  */
            if (MAX_ATR_SIZE < size) {
#endif
                free(atr);
                Log1(PCSC_LOG_ERROR, "Not enough memory for ATR");
                goto err;
            }

            memcpy(Value, atr, size);
            *Length = size;
            free(atr);
            break;

        case TAG_IFD_SLOTS_NUMBER:
            if (*Length < 1) {
                Log1(PCSC_LOG_ERROR, "Invalid input data");
                goto err;
            }

            *Value  = vicc_max_slots;
            *Length = 1;
            break;

        case TAG_IFD_THREAD_SAFE:
            if (*Length < 1) {
                Log1(PCSC_LOG_ERROR, "Invalid input data");
                goto err;
            }

            /* We are not thread safe due to
             * the global hostname and ctx */
            *Value  = 0;
            *Length = 1;
            break;

        case TAG_IFD_SLOT_THREAD_SAFE:
            if (*Length < 1) {
                Log1(PCSC_LOG_ERROR, "Invalid input data");
                goto err;
            }

            /* driver supports access to multiple slots of the same reader at
             * the same time */
            *Value  = 1;
            *Length = 1;
            break;

        default:
            Log2(PCSC_LOG_DEBUG, "unknown tag %d", (int)Tag);
            r = IFD_ERROR_TAG;
            goto err;
    }

    r = IFD_SUCCESS;

err:
    if (r != IFD_SUCCESS && Length)
        *Length = 0;

    return r;
}

RESPONSECODE
IFDHSetCapabilities (DWORD Lun, DWORD Tag, DWORD Length, PUCHAR Value)
{
    Log9(PCSC_LOG_DEBUG, "IFDHSetCapabilities not supported (Lun=%u Tag=%u Length=%u Value=%p)%s%s%s%s",
            (unsigned int) Lun, (unsigned int) Tag, (unsigned int) Length,
            (unsigned char *) Value, "", "", "", "");
    return IFD_NOT_SUPPORTED;
}

RESPONSECODE
IFDHSetProtocolParameters (DWORD Lun, DWORD Protocol, UCHAR Flags, UCHAR PTS1,
        UCHAR PTS2, UCHAR PTS3)
{
    Log9(PCSC_LOG_DEBUG, "Ignoring IFDHSetProtocolParameters (Lun=%u Protocol=%u Flags=%u PTS1=%u PTS2=%u PTS3=%u)%s%s",
            (unsigned int) Lun, (unsigned int) Protocol, (unsigned char) Flags,
            (unsigned char) PTS1, (unsigned char) PTS2, (unsigned char) PTS3, "", "");
    return IFD_SUCCESS;
}

RESPONSECODE
IFDHPowerICC (DWORD Lun, DWORD Action, PUCHAR Atr, PDWORD AtrLength)
{
    size_t slot = Lun & 0xffff;
    RESPONSECODE r = IFD_COMMUNICATION_ERROR;

    if (slot >= vicc_max_slots) {
        goto err;
    }

    switch (Action) {
        case IFD_POWER_DOWN:
            if (vicc_poweroff(ctx[slot]) < 0) {
                Log1(PCSC_LOG_ERROR, "could not powerdown");
                goto err;
            }

            /* XXX see bug #312754 on https://alioth.debian.org/projects/pcsclite */
#if 0
            *AtrLength = 0;

#endif
            return IFD_SUCCESS;
        case IFD_POWER_UP:
            if (vicc_poweron(ctx[slot]) < 0) {
                Log1(PCSC_LOG_ERROR, "could not powerup");
                goto err;
            }
            break;
        case IFD_RESET:
            if (vicc_reset(ctx[slot]) < 0) {
                Log1(PCSC_LOG_ERROR, "could not reset");
                goto err;
            }
            break;
        default:
            Log2(PCSC_LOG_ERROR, "%0lX not supported", Action);
            r = IFD_NOT_SUPPORTED;
            goto err;
    }

    r = IFD_SUCCESS;

err:
    if (r != IFD_SUCCESS && AtrLength)
        *AtrLength = 0;
    else
        r = IFDHGetCapabilities (Lun, TAG_IFD_ATR, AtrLength, Atr);

    return r;
}

RESPONSECODE
IFDHTransmitToICC (DWORD Lun, SCARD_IO_HEADER SendPci, PUCHAR TxBuffer,
        DWORD TxLength, PUCHAR RxBuffer, PDWORD RxLength,
        PSCARD_IO_HEADER RecvPci)
{
    unsigned char *rapdu = NULL;
    ssize_t size;
    RESPONSECODE r = IFD_COMMUNICATION_ERROR;
    size_t slot = Lun & 0xffff;

    if (slot >= vicc_max_slots) {
        goto err;
    }

    if (!RxLength || !RecvPci) {
        Log1(PCSC_LOG_ERROR, "Invalid input data");
        goto err;
    }

    size = vicc_transmit(ctx[slot], TxLength, TxBuffer, &rapdu);

    if (size < 0) {
        Log1(PCSC_LOG_ERROR, "could not send apdu or receive rapdu");
        goto err;
    }

    if (*RxLength < size) {
        Log1(PCSC_LOG_ERROR, "Not enough memory for rapdu");
        goto err;
    }

    *RxLength = size;
    memcpy(RxBuffer, rapdu, size);
    RecvPci->Protocol = 1;

    r = IFD_SUCCESS;

err:
    if (r != IFD_SUCCESS && RxLength)
        *RxLength = 0;

    free(rapdu);

    return r;
}

RESPONSECODE
IFDHICCPresence (DWORD Lun)
{
    size_t slot = Lun & 0xffff;
    if (slot >= vicc_max_slots) {
        return IFD_COMMUNICATION_ERROR;
    }
    switch (vicc_present(ctx[slot])) {
        case 0:
            return IFD_ICC_NOT_PRESENT;
        case 1:
            return IFD_ICC_PRESENT;
        default:
            Log1(PCSC_LOG_ERROR, "Could not get ICC state");
            return IFD_COMMUNICATION_ERROR;
    }
}
