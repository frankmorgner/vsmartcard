/*
 * Copyright (C) 2010 Frank Morgner
 *
 * This file is part of ifdnfc.
 *
 * ifdnfc is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * ifdnfc is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ifdnfc.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <winscard.h>
#include <time.h>

#ifdef HAVE_ARPA_INIT_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_PCSCLITE_H
#include <pcsclite.h>
#endif

#ifdef HAVE_READER_H
#include <reader.h>
#endif


#ifndef FEATURE_EXECUTE_PACE
#define FEATURE_EXECUTE_PACE 0x20
#endif

#ifndef CM_IOCTL_GET_FEATURE_REQUEST
#define CM_IOCTL_GET_FEATURE_REQUEST SCARD_CTL_CODE(3400)
#endif

#define PCSC_TLV_ELEMENT_SIZE 6


static void
printb(const char *label, unsigned char *buf, size_t len)
{
    size_t i = 0;
    printf(label);
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

static LONG parse_EstablishPACEChannel_OutputData(
        unsigned char output[], unsigned int output_length)
{
    uint8_t lengthCAR, lengthCARprev;
    uint16_t lengthOutputData, lengthEF_CardAccess, length_IDicc;
    uint32_t result;
    size_t parsed = 0;

    if (parsed+4 > output_length) {
        fprintf(stderr, "Malformed Establish PACE Channel output data.\n");
        return SCARD_F_INTERNAL_ERROR;
    }
    memcpy(&result, output+parsed, 4);
    parsed += 4;
    switch (result) {
        case 0x00000000:
            break;
        default:
            fprintf(stderr, "Reader reported some error.\n");
            return SCARD_F_COMM_ERROR;
    }

    if (parsed+2 > output_length) {
        fprintf(stderr, "Malformed Establish PACE Channel output data.\n");
        return SCARD_F_INTERNAL_ERROR;
    }
    memcpy(&lengthOutputData, output+parsed, 2);
    parsed += 2;
    if (lengthOutputData != output_length-parsed) {
        fprintf(stderr, "Malformed Establish PACE Channel output data.\n");
        return SCARD_F_INTERNAL_ERROR;
    }

    if (parsed+2 > output_length) {
        fprintf(stderr, "Malformed Establish PACE Channel output data.\n");
        return SCARD_F_INTERNAL_ERROR;
    }
    printf("MSE:Set AT Statusbytes: %02X %02X\n",
            output[parsed+0], output[parsed+1]);
    parsed += 2;

    if (parsed+2 > output_length) {
        fprintf(stderr, "Malformed Establish PACE Channel output data.\n");
        return SCARD_F_INTERNAL_ERROR;
    }
    memcpy(&lengthEF_CardAccess, output+parsed, 2);
    parsed += 2;

    if (parsed+lengthEF_CardAccess > output_length) {
        fprintf(stderr, "Malformed Establish PACE Channel output data.\n");
        return SCARD_F_INTERNAL_ERROR;
    }
    if (lengthEF_CardAccess)
        printb("EF.CardAccess:\n", &output[parsed], lengthEF_CardAccess);
    parsed += lengthEF_CardAccess;

    if (parsed+1 > output_length) {
        fprintf(stderr, "Malformed Establish PACE Channel output data.\n");
        return SCARD_F_INTERNAL_ERROR;
    }
    lengthCAR = output[parsed];
    parsed += 1;

    if (parsed+lengthCAR > output_length) {
        fprintf(stderr, "Malformed Establish PACE Channel output data.\n");
        return SCARD_F_INTERNAL_ERROR;
    }
    if (lengthCAR)
        printb("Recent Certificate Authority:\n",
                &output[parsed], lengthCAR);
    parsed += lengthCAR;

    if (parsed+1 > output_length) {
        fprintf(stderr, "Malformed Establish PACE Channel output data.\n");
        return SCARD_F_INTERNAL_ERROR;
    }
    lengthCARprev = output[parsed];
    parsed += 1;

    if (parsed+lengthCARprev > output_length) {
        fprintf(stderr, "Malformed Establish PACE Channel output data.\n");
        return SCARD_F_INTERNAL_ERROR;
    }
    if (lengthCARprev)
        printb("Previous Certificate Authority:\n",
                &output[parsed], lengthCARprev);
    parsed += lengthCARprev;

    if (parsed+2 > output_length) {
        fprintf(stderr, "Malformed Establish PACE Channel output data.\n");
        return SCARD_F_INTERNAL_ERROR;
    }
    memcpy(&length_IDicc , output+parsed, 2);
    parsed += 2;

    if (parsed+length_IDicc > output_length) {
        fprintf(stderr, "Malformed Establish PACE Channel output data.\n");
        return SCARD_F_INTERNAL_ERROR;
    }
    if (length_IDicc)
        printb("IDicc:\n", &output[parsed], length_IDicc);
    parsed += length_IDicc;

    if (parsed != output_length) {
        fprintf(stderr, "Overrun by %d bytes", output_length - parsed);
        return SCARD_F_INTERNAL_ERROR;
    }

    return SCARD_S_SUCCESS;
}

int
main(int argc, char *argv[])
{
    LONG rv;
    SCARDCONTEXT hContext;
    SCARDHANDLE hCard;
    LPSTR mszReaders = NULL;
    int num, l;
    char *reader;
    DWORD pace_ioctl = 0;
    BYTE pbSendBufferCapabilities [] = {
        0x01, /* idxFunction = GetReadersPACECapabilities */
        0x00, /* lengthInputData */
        0x00, /* lengthInputData */
    };
    BYTE pbSendBufferEstablish [] = {
        0x02, /* idxFunction = EstabishPACEChannel */
        0x00, /* lengthInputData */
        0x00, /* lengthInputData */
        0x03, /* PACE with PIN */
        0x00, /* length CHAT */
        0x00, /* length PIN */
        /*0x06, [> length PIN <]*/
        /*'2',*/
        /*'6',*/
        /*'3',*/
        /*'4',*/
        /*'1',*/
        /*'1',*/
        0x00, /* length certificate description */
        0x00, /* length certificate description */
    };
    BYTE pbRecvBuffer[1024];
    DWORD dwActiveProtocol, dwRecvLength, dwReaders, i;
    time_t t_start, t_end;

    uint16_t lengthInputData = sizeof(pbSendBufferEstablish) - 3;
    memcpy(pbSendBufferEstablish + 1, &lengthInputData, 2);


    if (argc > 1) {
        if (argc > 2 || sscanf(argv[1], "%d", &num) != 1) {
            fprintf(stderr, "Usage:  %s [reader_num]\n", argv[0]);
            exit(2);
        }
    } else
        num = 0;


    rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);
    if (rv != SCARD_S_SUCCESS) {
        fprintf(stderr, "Could not connect to PC/SC Service");
        goto err;
    }


    dwReaders = SCARD_AUTOALLOCATE;
    rv = SCardListReaders(hContext, NULL, (LPSTR)&mszReaders, &dwReaders);
    if (rv != SCARD_S_SUCCESS) {
        fprintf(stderr, "Could not get readers");
        goto err;
    }

    for (reader = mszReaders, i = 0; dwReaders > 0;
            l = strlen(reader) + 1, dwReaders -= l, reader += l, i++) {

        if (i == num)
            break;
    }
    if (dwReaders <= 0) {
        fprintf(stderr, "Could not find reader number %d\n", num);
        rv = SCARD_E_UNKNOWN_READER;
        goto err;
    }


    rv = SCardConnect(hContext, reader, SCARD_SHARE_DIRECT, 0, &hCard,
            &dwActiveProtocol); 
    if (rv != SCARD_S_SUCCESS) {
        fprintf(stderr, "Could not connect to %s\n", reader);
        goto err;
    }
    printf("Connected to %s\n", reader);


    /* does the reader support PACE? */
    rv = SCardControl(hCard, CM_IOCTL_GET_FEATURE_REQUEST, NULL, 0,
            pbRecvBuffer, sizeof(pbRecvBuffer), &dwRecvLength);
    if (rv != SCARD_S_SUCCESS) {
        fprintf(stderr, "Could not get the reader's features\n");
        goto err;
    }
    printb("reader's features\n", pbRecvBuffer, dwRecvLength);

    for (i = 0; i <= dwRecvLength-PCSC_TLV_ELEMENT_SIZE; i += PCSC_TLV_ELEMENT_SIZE) {
        if (pbRecvBuffer[i] == FEATURE_EXECUTE_PACE) {
            memcpy(&pace_ioctl, pbRecvBuffer+i+2, 4);
            break;
        }
    }
    if (0 == pace_ioctl) {
        printf("Reader does not support PACE\n");
        goto err;
    }
    /* convert to host byte order to use for SCardControl */
    pace_ioctl = ntohl(pace_ioctl);


    rv = SCardControl(hCard, pace_ioctl,
            pbSendBufferCapabilities, sizeof(pbSendBufferCapabilities),
            pbRecvBuffer, sizeof(pbRecvBuffer), &dwRecvLength);
    if (rv != SCARD_S_SUCCESS) {
        fprintf(stderr, "Could not get reader's PACE capabilities\n");
        goto err;
    }
    printb("ReadersPACECapabilities ", pbRecvBuffer, dwRecvLength);


    dwRecvLength = 0;
    t_start = time(NULL);
    rv = SCardControl(hCard, pace_ioctl,
            pbSendBufferEstablish, sizeof(pbSendBufferEstablish),
            pbRecvBuffer, sizeof(pbRecvBuffer), &dwRecvLength);
    t_end = time(NULL);
    if (rv != SCARD_S_SUCCESS) {
        fprintf(stderr, "Could not establish PACE channel\n");
        goto err;
    }
    printf("EstablishPACEChannel successfull, received %d bytes\n",
            dwRecvLength);
    printb("EstablishPACEChannel\n", pbRecvBuffer, dwRecvLength);

    rv = parse_EstablishPACEChannel_OutputData(pbRecvBuffer, dwRecvLength);
    if (rv != SCARD_S_SUCCESS)
        goto err;

    printf("Established PACE channel returned in %.0fs.\n",
            difftime(t_end, t_start));


    rv = SCardDisconnect(hCard, SCARD_LEAVE_CARD);
    if (rv != SCARD_S_SUCCESS)
        goto err;

    rv = SCardFreeMemory(hContext, mszReaders);
    if (rv != SCARD_S_SUCCESS)
        goto err;


    exit(0);

err:
#ifdef HAVE_PCSCLITE_H
    puts(pcsc_stringify_error(rv));
#endif
    if (mszReaders)
        SCardFreeMemory(hContext, mszReaders);


    exit(1);
}
