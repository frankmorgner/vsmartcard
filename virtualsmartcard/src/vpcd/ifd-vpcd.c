#include "vpcd.h"
#include <debuglog.h>
#include <errno.h>
#include <ifdhandler.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* pcscd allows at most 16 readers. We will use 10.
 * See PCSCLITE_MAX_READERS_CONTEXTS in pcsclite.h */
#define VICC_MAX_SLOTS \
    (PCSCLITE_MAX_READERS_CONTEXTS > 6 ? \
     PCSCLITE_MAX_READERS_CONTEXTS-6 : \
     1)

static struct vicc_ctx *ctx[VICC_MAX_SLOTS];

RESPONSECODE
IFDHCreateChannel (DWORD Lun, DWORD Channel)
{
    size_t slot = Lun & 0xffff;
    if (slot >= VICC_MAX_SLOTS) {
        return IFD_COMMUNICATION_ERROR;
    }
    ctx[slot] = vicc_init(Channel+slot);
    if (!ctx[slot]) {
        Log1(PCSC_LOG_ERROR, "Could not initialize connection to virtual ICC");
        return IFD_COMMUNICATION_ERROR;
    }
    Log2(PCSC_LOG_INFO, "Waiting for virtual ICC on port %hu", (unsigned short) Channel+slot);

    return IFD_SUCCESS;
}

RESPONSECODE
IFDHCreateChannelByName (DWORD Lun, LPSTR DeviceName)
{
    char *dots;
    unsigned long int port = VPCDPORT;

    dots = strchr(DeviceName, ':');
    if (dots) {
        /* a port has been specified behind the device name */

        /* skip '+' */
        dots++;

        errno = 0;
        port = strtoul(dots, NULL, 0);
        if (errno) {
            Log2(PCSC_LOG_ERROR, "Could not parse port: %s", dots);
            return IFD_NO_SUCH_DEVICE;
        }
    } else {
        Log1(PCSC_LOG_INFO, "Using default port.");
    }

    return IFDHCreateChannel (Lun, port);
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
    return IFD_ERROR_NOT_SUPPORTED;
}

RESPONSECODE
IFDHCloseChannel (DWORD Lun)
{
    size_t slot = Lun & 0xffff;
    if (slot >= VICC_MAX_SLOTS) {
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
    if (slot >= VICC_MAX_SLOTS) {
        return IFD_COMMUNICATION_ERROR;
    }

    if (!Length || !Value)
        return IFD_COMMUNICATION_ERROR;

    switch (Tag) {
        case TAG_IFD_ATR:

            size = vicc_getatr(ctx[slot], &atr);
            if (size < 0) {
                Log1(PCSC_LOG_ERROR, "could not get ATR");
                return IFD_COMMUNICATION_ERROR;
            }
            if (size == 0) {
                Log1(PCSC_LOG_ERROR, "Virtual ICC removed");
                return IFD_ICC_NOT_PRESENT;
            }
            Log2(PCSC_LOG_DEBUG, "Got ATR (%d bytes)", size);

            if (*Length < size) {
                free(atr);
                Log1(PCSC_LOG_ERROR, "Not enough memory for ATR");
                return IFD_COMMUNICATION_ERROR;
            }

            memcpy(Value, atr, size);
            *Length = size;
            free(atr);
            break;

        case TAG_IFD_SLOTS_NUMBER:
            if (*Length < 1) {
                Log1(PCSC_LOG_ERROR, "Invalid input data");
                return IFD_COMMUNICATION_ERROR;
            }

            *Value  = VICC_MAX_SLOTS;
            *Length = 1;
            break;

        case TAG_IFD_SLOT_THREAD_SAFE:
            /* driver supports access to multiple slots of the same reader at
             * the same time */
            *Value  = 1;
            *Length = 1;
            break;

        default:
            Log2(PCSC_LOG_DEBUG, "unknown tag %d", (int)Tag);
            return IFD_ERROR_TAG;
    }

    return IFD_SUCCESS;
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
    if (slot >= VICC_MAX_SLOTS) {
        return IFD_COMMUNICATION_ERROR;
    }
    switch (Action) {
        case IFD_POWER_DOWN:
            if (vicc_poweroff(ctx[slot]) < 0) {
                Log1(PCSC_LOG_ERROR, "could not powerdown");
                return IFD_COMMUNICATION_ERROR;
            }

            /* XXX see bug #312754 on https://alioth.debian.org/projects/pcsclite */
#if 0
            *AtrLength = 0;

#endif
            return IFD_SUCCESS;
        case IFD_POWER_UP:
            if (vicc_poweron(ctx[slot]) < 0) {
                Log1(PCSC_LOG_ERROR, "could not powerup");
                return IFD_COMMUNICATION_ERROR;
            }
            break;
        case IFD_RESET:
            if (vicc_reset(ctx[slot]) < 0) {
                Log1(PCSC_LOG_ERROR, "could not reset");
                return IFD_COMMUNICATION_ERROR;
            }
            break;
        default:
            Log2(PCSC_LOG_ERROR, "%0lX not supported", Action);
            return IFD_NOT_SUPPORTED;
    }

    return IFDHGetCapabilities (Lun, TAG_IFD_ATR, AtrLength, Atr);
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
    if (slot >= VICC_MAX_SLOTS) {
        return IFD_COMMUNICATION_ERROR;
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
    if (r != IFD_SUCCESS)
        *RxLength = 0;

    free(rapdu);

    return r;
}

RESPONSECODE
IFDHICCPresence (DWORD Lun)
{
    size_t slot = Lun & 0xffff;
    if (slot >= VICC_MAX_SLOTS) {
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
