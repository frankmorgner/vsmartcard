/*
 * Copyright (C) 2013 Frank Morgner
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
#include <config.h>
#endif

#ifdef _WIN32
#define sleep(s) Sleep((s*1000))
#endif

#include "ifd-vpcd.h"
#include "vpcd.h"
#include <ifdhandler.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winscard.h>

struct card {
    DWORD dwShareMode;
    size_t usage_counter;
};

#define SET_R_TEST(value) { r = value; if (r != SCARD_S_SUCCESS) { goto err; } }

static struct card cards[PCSCLITE_MAX_READERS_CONTEXTS];
static int cancel_status = 0;
static size_t context_count = 0;

static const char reader_format_str[] = "Virtual PCD %02"SCNu32;
static const SCARDHANDLE validhandle = 1;

/* defined as "extern" in pcsclite.h, but not used here */
const SCARD_IO_REQUEST g_rgSCardT0Pci, g_rgSCardT1Pci, g_rgSCardRawPci;

/* part of libvpcd, but not exported */
extern const unsigned char vicc_max_slots;
extern const char *hostname;

static LONG autoallocate(void *buf, LPDWORD len, DWORD max, void **rbuf)
{
    LPSTR *p;

    if (!len || !rbuf)
        return SCARD_E_INVALID_PARAMETER;

    if (buf && (*len == SCARD_AUTOALLOCATE)) {
        p = malloc(max);
        if (!p)
            return SCARD_E_NO_MEMORY;

        /* copy the pointer */
        memcpy(buf, &p, sizeof p);

        *len = max;
        *rbuf = p;
    } else {
        /* we don't have anything to do.
         * Simply tell the caller to use buf with unchanged length */
        *rbuf = buf;
    }

    return SCARD_S_SUCCESS;
}

static void initialize_globals(void)
{
    uint32_t index;
    DWORD Channel = VPCDPORT;
    const char *hostname_old = hostname;

    hostname = VPCDHOST;
    for (index = 0;
            index < PCSCLITE_MAX_READERS_CONTEXTS && index < vicc_max_slots;
            index++) {
        IFDHCreateChannel ((DWORD) index, Channel);
    }
    hostname = hostname_old;
}

static void release_globals(void)
{
    uint32_t index;
    for (index = 0;
            index < PCSCLITE_MAX_READERS_CONTEXTS && index < vicc_max_slots;
            index++) {
        IFDHCloseChannel ((DWORD) index);
    }
    memset(cards, 0, sizeof cards);
}

static LONG handle2card(SCARDHANDLE hCard, struct card **card)
{
    uint32_t index = (uint32_t) hCard;

    if (!card)
        return SCARD_F_INTERNAL_ERROR;

    if (index >= PCSCLITE_MAX_READERS_CONTEXTS)
        return SCARD_E_INVALID_HANDLE;

    *card = &cards[index];

    return SCARD_S_SUCCESS;
}

LONG handle2reader(DWORD Lun, LPSTR mszReaderName, LPDWORD pcchReaderLen)
{
    LONG r;
    char *reader;
    int length;

    if (!pcchReaderLen) {
        r = SCARD_E_INVALID_PARAMETER;
        goto err;
    }

    SET_R_TEST( autoallocate(mszReaderName, pcchReaderLen, MAX_READERNAME, (void **) &reader));

    if (reader) {
        /* caller wants to have the string */
        length = snprintf(reader, *pcchReaderLen, reader_format_str, (uint32_t) Lun);
        if (length > *pcchReaderLen) {
            r = SCARD_E_INSUFFICIENT_BUFFER;
            goto err;
        }
    } else {
        /* caller wants to have the length */
        length = snprintf(reader, 0, reader_format_str, (uint32_t) Lun);
    }

    if (length < 0) {
        r = SCARD_F_INTERNAL_ERROR;
        goto err;
    }

    *pcchReaderLen = length;
    /* r has already been set to SCARD_S_SUCCESS */

err:
    return r;
}

static LONG reader2card(LPCSTR szReader, struct card **card, LPSCARDHANDLE phCard)
{
    uint32_t index;

    if (!card)
        return SCARD_F_INTERNAL_ERROR;

    if (1 != sscanf(szReader, reader_format_str, &index)
            || index >= PCSCLITE_MAX_READERS_CONTEXTS)
        return SCARD_E_READER_UNAVAILABLE;

    *card = &cards[index];
    if (phCard)
        *phCard = (DWORD) index;

    return SCARD_S_SUCCESS;
}

static LONG responsecode2long(RESPONSECODE r)
{
    switch (r) {
        case IFD_ICC_PRESENT:
        case IFD_SUCCESS:
            return SCARD_S_SUCCESS;
        case IFD_ERROR_INSUFFICIENT_BUFFER:
            return SCARD_E_INSUFFICIENT_BUFFER;
        case IFD_ERROR_NOT_SUPPORTED:
        case IFD_NOT_SUPPORTED:
        case IFD_PROTOCOL_NOT_SUPPORTED:
            return SCARD_E_UNSUPPORTED_FEATURE;
        case IFD_ERROR_CONFISCATE:
        case IFD_ERROR_EJECT:
        case IFD_ERROR_POWER_ACTION:
            return SCARD_E_CANT_DISPOSE;
        case IFD_RESPONSE_TIMEOUT:
            return SCARD_E_TIMEOUT;
        case IFD_ICC_NOT_PRESENT:
            return SCARD_E_NO_SMARTCARD;
        case IFD_COMMUNICATION_ERROR:
            return SCARD_F_COMM_ERROR;
        case IFD_NO_SUCH_DEVICE:
            return SCARD_E_READER_UNAVAILABLE;
        case IFD_ERROR_TAG:
        case IFD_ERROR_SET_FAILURE:
        case IFD_ERROR_VALUE_READ_ONLY:
        case IFD_ERROR_PTS_FAILURE:
        case IFD_ERROR_SWALLOW:
        default:
            return SCARD_F_INTERNAL_ERROR;
    }
}

static LONG handle2atr(DWORD Lun, LPBYTE pbAtr, LPDWORD pcbAtrLen)
{
    LONG r;
    void *atr;
    unsigned char _atr[MAX_ATR_SIZE];

    SET_R_TEST( responsecode2long(
                IFDHICCPresence(Lun)));

    SET_R_TEST( autoallocate(pbAtr, pcbAtrLen, MAX_ATR_SIZE, (void **) &atr));

    if (!atr) {
        /* caller wants to have the length */
        *pcbAtrLen = sizeof _atr;
        atr = _atr;
    }

    SET_R_TEST( responsecode2long(
                IFDHGetCapabilities (Lun, TAG_IFD_ATR, pcbAtrLen, atr)));

err:
    return r;
}

PCSC_API LONG SCardEstablishContext(DWORD dwScope, LPCVOID pvReserved1, LPCVOID pvReserved2, LPSCARDCONTEXT phContext)
{
    if (!phContext)
        return SCARD_E_INVALID_PARAMETER;

    if (!context_count)
        initialize_globals();
    context_count++;

    *phContext = validhandle;

    return SCARD_S_SUCCESS;
}

PCSC_API LONG SCardReleaseContext(SCARDCONTEXT hContext)
{
    if (context_count) {
        context_count--;
    }

    if (!context_count) {
        release_globals();
    }

    return SCARD_S_SUCCESS;
}

PCSC_API LONG SCardIsValidContext(SCARDCONTEXT hContext)
{
    if (context_count && hContext == validhandle)
        return SCARD_S_SUCCESS;
    else
        return SCARD_E_INVALID_HANDLE;
}

PCSC_API LONG SCardSetTimeout(SCARDCONTEXT hContext, DWORD dwTimeout)
{
    /* XXX better return an error here? */
    return SCARD_S_SUCCESS;
}

PCSC_API LONG SCardConnect(SCARDCONTEXT hContext, LPCSTR szReader, DWORD dwShareMode, DWORD dwPreferredProtocols, LPSCARDHANDLE phCard, LPDWORD pdwActiveProtocol)
{
    struct card *card;
    LONG r;

    SET_R_TEST( reader2card(szReader, &card, phCard));

    if (card->usage_counter) {
        /* card/reader already in use */
        if (card->dwShareMode == SCARD_SHARE_EXCLUSIVE
                || card->dwShareMode != dwShareMode) {
            /* cannot use the provided mode */
            return SCARD_E_SHARING_VIOLATION;
        }
    }

    card->usage_counter++;
    card->dwShareMode = dwShareMode;

err:
    return r;
}

PCSC_API LONG SCardReconnect(SCARDHANDLE hCard, DWORD dwShareMode, DWORD dwPreferredProtocols, DWORD dwInitialization, LPDWORD pdwActiveProtocol)
{
    struct card *card;
    LONG r;

    SET_R_TEST( handle2card(hCard, &card));

    if (card->usage_counter > 1
            && card->dwShareMode != dwShareMode) {
        /* cannot use the provided mode */
        r = SCARD_E_SHARING_VIOLATION;
        goto err;
    }

    card->dwShareMode = dwShareMode;

err:
    return r;
}

PCSC_API LONG SCardDisconnect(SCARDHANDLE hCard, DWORD dwDisposition)
{
    DWORD Lun = hCard;
    LONG r;
    UCHAR Atr[MAX_ATR_SIZE];
    DWORD AtrLength = sizeof Atr;
    struct card *card;

    SET_R_TEST( handle2card(hCard, &card));

    if (!card->usage_counter) {
        r = SCARD_E_INVALID_HANDLE;
        goto err;
    }
    card->usage_counter--;

    switch (dwDisposition) {
        case SCARD_LEAVE_CARD:
            r = SCARD_S_SUCCESS;
            break;

        case SCARD_RESET_CARD:
            if (card->usage_counter) {
                r = SCARD_E_CANT_DISPOSE;
                goto err;
            }
            SET_R_TEST( responsecode2long(
                        IFDHPowerICC (Lun, IFD_RESET, Atr, &AtrLength)));
            break;

        case SCARD_EJECT_CARD:
            /* fall through */
        case SCARD_UNPOWER_CARD:
            if (card->usage_counter) {
                r = SCARD_E_CANT_DISPOSE;
                goto err;
            }
            SET_R_TEST( responsecode2long(
                        IFDHPowerICC (Lun, IFD_POWER_DOWN, Atr, &AtrLength)));
            break;

        default:
            r = SCARD_E_INVALID_PARAMETER;
            break;
    }

err:
    return r;
}

PCSC_API LONG SCardBeginTransaction(SCARDHANDLE hCard)
{
    struct card *card;
    LONG r;

    SET_R_TEST( handle2card(hCard, &card));

    /* we don't have a strategy if an other card handle is active */
    if (card->usage_counter != 1)
        return SCARD_E_SHARING_VIOLATION;

    card->dwShareMode = SCARD_SHARE_EXCLUSIVE;

err:
    return r;
}

PCSC_API LONG SCardEndTransaction(SCARDHANDLE hCard, DWORD dwDisposition)
{
    struct card *card;
    LONG r;

    SET_R_TEST( handle2card(hCard, &card));

    card->dwShareMode = dwDisposition;

err:
    return r;
}

PCSC_API LONG SCardCancelTransaction(SCARDHANDLE hCard)
{
    return SCARD_S_SUCCESS;
}

PCSC_API LONG SCardStatus(SCARDHANDLE hCard, LPSTR mszReaderName, LPDWORD pcchReaderLen, LPDWORD pdwState, LPDWORD pdwProtocol, LPBYTE pbAtr, LPDWORD pcbAtrLen)
{
    LONG r;

    SET_R_TEST( handle2reader(hCard, mszReaderName, pcchReaderLen));
    SET_R_TEST( handle2atr(hCard, pbAtr, pcbAtrLen));

err:
    return r;
}

PCSC_API LONG SCardGetStatusChange(SCARDCONTEXT hContext, DWORD dwTimeout, LPSCARD_READERSTATE rgReaderStates, DWORD cReaders)
{
    SCARDHANDLE hCard;
    size_t i, seconds, event_count = 0;
    struct card *card;

    cancel_status = 0;

    if (dwTimeout == INFINITE) {
        /* "infinite" timeout */
        seconds = 60;
    } else {
        seconds = dwTimeout/1000;
    }

    do {
        for (i = 0; i < cReaders; i++) {
            if (rgReaderStates[i].dwCurrentState & SCARD_STATE_IGNORE)
                /* this reader should be ignored */
                continue;

            if (strcmp(rgReaderStates[i].szReader, "\\\\?PnP?\\Notification") == 0)
                /* we don't allow readers to be added or removed */
                continue;

            rgReaderStates[i].dwEventState = 0;

            if (SCARD_S_SUCCESS != reader2card(rgReaderStates[i].szReader,
                        &card, &hCard)) {
                /* given reader not recognized */
                rgReaderStates[i].dwEventState |= SCARD_STATE_UNKNOWN
                    | SCARD_STATE_CHANGED |SCARD_STATE_IGNORE;
                event_count++;
                continue;
            }

            if (card->usage_counter) {
                rgReaderStates[i].dwEventState |= SCARD_STATE_INUSE;
                if (card->dwShareMode == SCARD_SHARE_EXCLUSIVE)
                    rgReaderStates[i].dwEventState |= SCARD_STATE_EXCLUSIVE;
            }

            /* normally the application should set cbAtr appropriately.
             * Some application don't mind to do that (e.g., pcsc_scan) */
            rgReaderStates[i].cbAtr = sizeof rgReaderStates[i].rgbAtr;

            if (SCARD_S_SUCCESS != handle2atr(hCard, rgReaderStates[i].rgbAtr,
                        &rgReaderStates[i].cbAtr)) {
                rgReaderStates[i].dwEventState |= SCARD_STATE_EMPTY;
                rgReaderStates[i].cbAtr = 0;
            } else {
                rgReaderStates[i].dwEventState |= SCARD_STATE_PRESENT;
            }

            /* if current and event state differ in the flags SCARD_STATE_EMPTY
             * or SCARD_STATE_PRESENT a state change has occurred */
            if (((rgReaderStates[i].dwCurrentState & SCARD_STATE_EMPTY)
                        != (rgReaderStates[i].dwEventState & SCARD_STATE_EMPTY))
                    || ((rgReaderStates[i].dwCurrentState & SCARD_STATE_PRESENT)
                        != (rgReaderStates[i].dwEventState & SCARD_STATE_PRESENT))) {
                rgReaderStates[i].dwEventState |= SCARD_STATE_CHANGED;
                event_count++;
            }
        }

        if (!seconds || event_count)
            break;

        sleep(1);
        seconds--;

    } while (!cancel_status);

    if (!event_count)
        return SCARD_E_TIMEOUT;

    return SCARD_S_SUCCESS;
}

PCSC_API LONG SCardCancel(SCARDHANDLE hCard)
{
    cancel_status = 1;
    return SCARD_S_SUCCESS;
}

PCSC_API LONG SCardControl(SCARDHANDLE hCard, DWORD dwControlCode, LPCVOID pbSendBuffer, DWORD cbSendLength, LPVOID pbRecvBuffer, DWORD cbRecvLength, LPDWORD lpBytesReturned)
{
    return responsecode2long(
            IFDHControl (hCard, dwControlCode, (PUCHAR) pbSendBuffer,
                cbSendLength, pbRecvBuffer, cbRecvLength, lpBytesReturned));
}

PCSC_API LONG SCardTransmit(SCARDHANDLE hCard, LPCSCARD_IO_REQUEST pioSendPci, LPCBYTE pbSendBuffer, DWORD cbSendLength, LPSCARD_IO_REQUEST pioRecvPci, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength)
{
    DWORD Lun = hCard;
    LONG r;
    /* ignored */
    SCARD_IO_HEADER SendPci, RecvPci;

    /* transceive data */
    SET_R_TEST( responsecode2long(
                IFDHTransmitToICC (Lun, SendPci, (PUCHAR) pbSendBuffer,
                    cbSendLength, pbRecvBuffer, pcbRecvLength, &RecvPci)));

err:
    return r;
}

PCSC_API LONG SCardGetAttrib(SCARDHANDLE hCard, DWORD dwAttrId, LPBYTE pbAttr, LPDWORD pcbAttrLen)
{
    return SCARD_F_INTERNAL_ERROR;
}

PCSC_API LONG SCardSetAttrib(SCARDHANDLE hCard, DWORD dwAttrId, LPCBYTE pbAttr, DWORD cbAttrLen)
{
    return SCARD_F_INTERNAL_ERROR;
}

PCSC_API LONG SCardListReaders(SCARDCONTEXT hContext, LPCSTR mszGroups, LPSTR mszReaders, LPDWORD pcchReaders)
{
    uint32_t index;
    DWORD readerslen = 0, readerlen;
    LONG r;
    char *readers = NULL;

    if (!pcchReaders) {
        r = SCARD_E_INVALID_PARAMETER;
        goto err;
    }


    SET_R_TEST( autoallocate(mszReaders, pcchReaders,
                (MAX_READERNAME+1)*PCSCLITE_MAX_READERS_CONTEXTS+1,
                (void **) &readers));


    /* write reader names */
    for (index = 0; index < PCSCLITE_MAX_READERS_CONTEXTS; index++) {
        /* what memory we have left */
        readerlen = *pcchReaders - readerslen;

        /* get the current readername */
        SET_R_TEST( handle2reader(index, readers ? readers+readerslen : NULL,
                    &readerlen));

        /* readerlen has been set to the correct value */
        readerslen += readerlen;
        /* copy null character */
        readerslen++;
    }

    /* write a null character as final delimiter */
    if (readers) {
        readers[readerslen] = '\0';
    }
    readerslen++;

    *pcchReaders = readerslen;

    if (!readerslen) {
        r = SCARD_E_NO_READERS_AVAILABLE;
    } else {
        r = SCARD_S_SUCCESS;
    }

err:
    return r;
}

PCSC_API LONG SCardListReaderGroups(SCARDCONTEXT hContext, LPSTR mszGroups, LPDWORD pcchGroups)
{
    LONG r;
    unsigned char *groups;

    SET_R_TEST( autoallocate(mszGroups, pcchGroups, 1, (void **) &groups));

    if (groups) {
        /* caller wants to have the string */
        *groups = '\0';
    } else {
        /* caller wants to have the length */
    }

    *pcchGroups = 1;


err:
    return r;
}

PCSC_API LONG SCardFreeMemory(SCARDCONTEXT hContext, LPCVOID pvMem)
{
    free((void *) pvMem);

    return SCARD_S_SUCCESS;
}
