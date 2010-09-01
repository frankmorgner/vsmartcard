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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_ARPA_INIT_H
#include <arpa/inet.h>
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
        fprintf(stderr, "Overrun by %d bytes\n", output_length - parsed);
        return SCARD_F_INTERNAL_ERROR;
    }

    return SCARD_S_SUCCESS;
}

int
main(int argc, char *argv[])
{
    LONG r;
    SCARDCONTEXT hContext;
    SCARDHANDLE hCard;
    LPSTR readers = NULL;
    char *reader;
    BYTE sendbuf[16], recvbuf[1024];
    DWORD ctl, recvlen, readerslen;
    time_t t_start, t_end;
    size_t l, pinlen = 0;
    char *pin = NULL;
    unsigned int readernum = 0, i;


    if (argc > 1) {
        if (argc > 2 && sscanf(argv[1], "%u", &readernum) != 1) {
            fprintf(stderr, "Could not get number of reader\n");
            exit(2);
        }
        if (argc == 2) {
            pin = argv[2];
            pinlen = strlen(pin);
            if (pinlen < 5 || pinlen > 6) {
                fprintf(stderr, "PIN too long\n");
                exit(2);
            }
        }
        if (argc > 3) {
            fprintf(stderr, "Usage:  "
                    "%s [reader number] [PIN]\n", argv[0]);
        }
    }


    r = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);
    if (r != SCARD_S_SUCCESS) {
        fprintf(stderr, "Could not connect to PC/SC Service\n");
        goto err;
    }


    readerslen = SCARD_AUTOALLOCATE;
    r = SCardListReaders(hContext, NULL, (LPSTR) &readers, &readerslen);
    if (r != SCARD_S_SUCCESS) {
        fprintf(stderr, "Could not get readers\n");
        goto err;
    }

    for (reader = readers, i = 0; readerslen > 0;
            l = strlen(reader) + 1, readerslen -= l, reader += l, i++) {

        if (i == readernum)
            break;
    }
    if (readerslen <= 0) {
        fprintf(stderr, "Could not find reader number %d\n", readernum);
        r = SCARD_E_UNKNOWN_READER;
        goto err;
    }


    r = SCardConnect(hContext, reader, SCARD_SHARE_DIRECT, 0, &hCard, &ctl); 
    if (r != SCARD_S_SUCCESS) {
        fprintf(stderr, "Could not connect to %s\n", reader);
        goto err;
    }
    printf("Connected to %s\n", reader);


    /* does the reader support PACE? */
    r = SCardControl(hCard, CM_IOCTL_GET_FEATURE_REQUEST, NULL, 0,
            recvbuf, sizeof(recvbuf), &recvlen);
    if (r != SCARD_S_SUCCESS) {
        fprintf(stderr, "Could not get the reader's features\n");
        goto err;
    }

    ctl = 0;
    for (i = 0; i <= recvlen-PCSC_TLV_ELEMENT_SIZE; i += PCSC_TLV_ELEMENT_SIZE) {
        if (recvbuf[i] == FEATURE_EXECUTE_PACE) {
            memcpy(&ctl, recvbuf+i+2, 4);
            break;
        }
    }
    if (0 == ctl) {
        printf("Reader does not support PACE\n");
        goto err;
    }
    /* convert to host byte order to use for SCardControl */
    ctl = ntohl(ctl);

    sendbuf[0] = 0x01;              /* idxFunction = GetReadersPACECapabilities */
    sendbuf[1] = 0x00;              /* lengthInputData */
    sendbuf[2] = 0x00;              /* lengthInputData */
    r = SCardControl(hCard, ctl,
            sendbuf, 2,
            recvbuf, sizeof(recvbuf), &recvlen);
    if (r != SCARD_S_SUCCESS) {
        fprintf(stderr, "Could not get reader's PACE capabilities\n");
        goto err;
    }
    printb("ReadersPACECapabilities ", recvbuf, recvlen);


    sendbuf[0] = 0x02;              /* idxFunction = EstabishPACEChannel */
    sendbuf[1] = (8+pinlen)>>8;     /* lengthInputData */
    sendbuf[2] = (8+pinlen)&0xff;   /* lengthInputData */
    sendbuf[3] = 0x03;              /* PACE with PIN */
    sendbuf[4] = 0x00,              /* length CHAT */
    sendbuf[5] = 0x00,              /* length PIN */
    sendbuf[6] = pinlen;            /* length PIN */
    memcpy(sendbuf+7, pin, pinlen); /* PIN */
    sendbuf[7+pinlen] = 0x00;       /* length certificate description */
    sendbuf[8+pinlen] = 0x00;       /* length certificate description */
    t_start = time(NULL);
    r = SCardControl(hCard, ctl,
            sendbuf, 8+pinlen,
            recvbuf, sizeof(recvbuf), &recvlen);
    t_end = time(NULL);
    if (r != SCARD_S_SUCCESS) {
        fprintf(stderr, "Could not establish PACE channel\n");
        goto err;
    }
    printf("EstablishPACEChannel successfull, received %d bytes\n",
            recvlen);
    printb("EstablishPACEChannel\n", recvbuf, recvlen);

    r = parse_EstablishPACEChannel_OutputData(recvbuf, recvlen);
    if (r != SCARD_S_SUCCESS)
        goto err;

    printf("Established PACE channel returned in %.0fs.\n",
            difftime(t_end, t_start));


    r = SCardDisconnect(hCard, SCARD_LEAVE_CARD);
    if (r != SCARD_S_SUCCESS)
        goto err;

    r = SCardFreeMemory(hContext, readers);
    if (r != SCARD_S_SUCCESS)
        goto err;


    exit(0);

err:
#ifdef HAVE_PCSCLITE_H
    puts(pcsc_stringify_error(r));
#endif
    if (readers)
        SCardFreeMemory(hContext, readers);


    exit(1);
}
