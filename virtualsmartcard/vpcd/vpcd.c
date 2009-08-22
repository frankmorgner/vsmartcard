/*
 * Copyright (C) 2009 Frank Morgner
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ifdhandler.h>
#include <debuglog.h>

#include "vpcd.h"

/*
 * List of Defined Functions Available to IFD_Handler 1.0
 */
RESPONSECODE
IO_Create_Channel(DWORD channelid)
{
    if (vicc_init() < 0) return IFD_COMMUNICATION_ERROR;

    return IFD_SUCCESS;
}

RESPONSECODE
IO_Close_Channel()
{
    RESPONSECODE r = IFD_Eject_ICC();
    if (vicc_exit() < 0) return IFD_COMMUNICATION_ERROR;

    return r;
}

RESPONSECODE
IFD_Get_Capabilities(DWORD tag, PUCHAR value)
{
    char * atr;
    int size;
    switch (tag) {
        case TAG_IFD_ATR:

            size = vicc_getatr(&atr);
            if (size < 0) {
                Log1(PCSC_LOG_ERROR, "could not get ATR");
                return IFD_COMMUNICATION_ERROR;
            }

            value = memcpy(value, atr, size);
            free(atr);
            return IFD_SUCCESS;

        case TAG_IFD_SLOTS_NUMBER:
            (*value) = 0;
            return IFD_SUCCESS;

        //case TAG_IFD_POLLING_THREAD_KILLABLE:
            //return IFD_SUCCESS;

        default:
            Log2(PCSC_LOG_DEBUG, "unknown tag %d", (int)tag);
    }

    return IFD_ERROR_TAG;
}

RESPONSECODE
IFD_Set_Capabilities(DWORD tag, PUCHAR value)
{
    return IFD_NOT_SUPPORTED;
}

RESPONSECODE
IFD_Set_Protocol_Parameters(DWORD protocoltype, UCHAR selectionflags, UCHAR
        pts1, UCHAR pts2, UCHAR pts3)
{
    return IFD_SUCCESS;
}

RESPONSECODE
IFD_Power_ICC(DWORD a)
{

    if ((a == IFD_POWER_DOWN) || (a == IFD_RESET)) {
        if (vicc_poweroff() < 0) {
            Log1(PCSC_LOG_ERROR, "could not powerdown");
            return IFD_COMMUNICATION_ERROR;
        }
    }

    if ((a == IFD_POWER_UP) || (a == IFD_RESET)) {
        if (vicc_poweron() < 0) {
            Log1(PCSC_LOG_ERROR, "could not powerup");
            return IFD_COMMUNICATION_ERROR;
        }
    }

    return IFD_SUCCESS;
}

RESPONSECODE
IFD_Swallow_ICC()
{
    Log1(PCSC_LOG_DEBUG, "");
    return IFD_NOT_SUPPORTED;
}

RESPONSECODE
IFD_Eject_ICC()
{
    if (vicc_eject() < 0) {
        return IFD_COMMUNICATION_ERROR;
    }
    return IFD_SUCCESS;
}

RESPONSECODE
IFD_Confiscate_ICC()
{
    Log1(PCSC_LOG_DEBUG, "");
    return IFD_NOT_SUPPORTED;
}

RESPONSECODE
IFD_Transmit_to_ICC(SCARD_IO_HEADER SendPci, PUCHAR TxBuffer, DWORD TxLength,
        PUCHAR RxBuffer, PDWORD RxLength, PSCARD_IO_HEADER RecvPci)
{
    (*RxLength) = 0;
    char *rapdu;

    int size = vicc_transmit(TxLength, (char *) TxBuffer, &rapdu);
    if (size < 0) {
        Log1(PCSC_LOG_ERROR, "could not send apdu or receive rapdu");
        return IFD_COMMUNICATION_ERROR;
    }

    (*RxLength) = size;
    RxBuffer = memcpy(RxBuffer, rapdu, size);
    free(rapdu);

    return IFD_SUCCESS;
}

RESPONSECODE
IFD_Is_ICC_Present()
{
    switch (vicc_present()) {
        case 0:  return IFD_ICC_NOT_PRESENT;
        case 1:  return IFD_ICC_PRESENT;
        default: return IFD_COMMUNICATION_ERROR;
    }
    return IFD_COMMUNICATION_ERROR;
}

RESPONSECODE
IFD_Is_ICC_Absent()
{
    return IFD_Is_ICC_Present();
}
