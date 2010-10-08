#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <ifdhandler.h>
#include <debuglog.h>

#include "vpcd.h"

RESPONSECODE
IFDHCreateChannel (DWORD Lun, DWORD Channel)
{
    if (vicc_init() < 0) {
        Log1(PCSC_LOG_ERROR, "Could not initialize connection to virtual ICC");
        return IFD_COMMUNICATION_ERROR;
    }

    return IFD_SUCCESS;
}

RESPONSECODE
IFDHCreateChannelByName (DWORD Lun, LPSTR DeviceName)
{
    return IFDHCreateChannel (Lun, 0);
}

RESPONSECODE
IFDHCloseChannel (DWORD Lun)
{
    Log1(PCSC_LOG_DEBUG, "");
    RESPONSECODE r = vicc_eject();
    if (vicc_exit() < 0) {
        Log1(PCSC_LOG_ERROR, "Could not close connection to virtual ICC");
        return IFD_COMMUNICATION_ERROR;
    }

    return r;
}

RESPONSECODE
IFDHGetCapabilities (DWORD Lun, DWORD Tag, PDWORD Length, PUCHAR Value)
{
    char *atr;
    int size;
    switch (Tag) {
        case TAG_IFD_ATR:

            size = vicc_getatr(&atr);
            if (size < 0) {
                Log1(PCSC_LOG_ERROR, "could not get ATR");
                return IFD_COMMUNICATION_ERROR;
            }
            Log2(PCSC_LOG_DEBUG, "Got ATR (%d bytes)", size);

            if (*Length < size)
                return IFD_COMMUNICATION_ERROR;

            memcpy(Value, atr, size);
            *Length = size;
            free(atr);

            break;
        case TAG_IFD_SLOTS_NUMBER:
            if (*Length < 1)
                return IFD_COMMUNICATION_ERROR;

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
    return IFD_NOT_SUPPORTED;
}

RESPONSECODE
IFDHSetProtocolParameters (DWORD Lun, DWORD Protocol, UCHAR Flags, UCHAR PTS1,
        UCHAR PTS2, UCHAR PTS3)
{
    Log1(PCSC_LOG_DEBUG, "");
    return IFD_SUCCESS;
}

RESPONSECODE
IFDHPowerICC (DWORD Lun, DWORD Action, PUCHAR Atr, PDWORD AtrLength)
{
    switch (Action) {
        case IFD_POWER_DOWN:
            if (vicc_poweroff() < 0) {
                Log1(PCSC_LOG_ERROR, "could not powerdown");
                return IFD_COMMUNICATION_ERROR;
            }

            *AtrLength = 0;

            return IFD_SUCCESS;
        case IFD_POWER_UP:
            if (vicc_poweron() < 0) {
                Log1(PCSC_LOG_ERROR, "could not powerup");
                return IFD_COMMUNICATION_ERROR;
            }
            break;
        case IFD_RESET:
            if (vicc_reset() < 0) {
                Log1(PCSC_LOG_ERROR, "could not reset");
                return IFD_COMMUNICATION_ERROR;
            }
            break;
        default:
            Log2(PCSC_LOG_ERROR, "%d not supported", Action);
            return IFD_NOT_SUPPORTED;
    }

    return IFDHGetCapabilities (Lun, TAG_IFD_ATR, AtrLength, Atr);
}

RESPONSECODE
IFDHTransmitToICC (DWORD Lun, SCARD_IO_HEADER SendPci, PUCHAR TxBuffer,
        DWORD TxLength, PUCHAR RxBuffer, PDWORD RxLength,
        PSCARD_IO_HEADER RecvPci)
{
    if (!RxLength || !RecvPci)
        return IFD_COMMUNICATION_ERROR;

    char *rapdu;
    int size = vicc_transmit(TxLength, (char *) TxBuffer, &rapdu);

    if (size < 0) {
        Log1(PCSC_LOG_ERROR, "could not send apdu or receive rapdu");
        *RxLength = 0;
        return IFD_COMMUNICATION_ERROR;
    }

    (*RxLength) = size;
    RxBuffer = memcpy(RxBuffer, rapdu, size);
    free(rapdu);
    RecvPci->Protocol = 1;

    return IFD_SUCCESS;
}

RESPONSECODE
IFDHICCPresence (DWORD Lun)
{
    switch (vicc_present()) {
        case 0:
            return IFD_ICC_NOT_PRESENT;
        case 1:
            return IFD_ICC_PRESENT;
        default:
            Log1(PCSC_LOG_DEBUG, "Could not get ICC state");
            return IFD_COMMUNICATION_ERROR;
    }
}

RESPONSECODE
IFDHControl (DWORD Lun, DWORD dwControlCode, PUCHAR TxBuffer, DWORD TxLength,
        PUCHAR RxBuffer, DWORD RxLength, LPDWORD pdwBytesReturned)
{
    return IFD_ERROR_NOT_SUPPORTED;
}
