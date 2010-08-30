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
//#include <pcsclite.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <winscard.h>

#ifndef FEATURE_EXECUTE_PACE
#define FEATURE_EXECUTE_PACE 0x20
#endif

#ifndef CM_IOCTL_GET_FEATURE_REQUEST
#define CM_IOCTL_GET_FEATURE_REQUEST SCARD_CTL_CODE(3400)
#endif

typedef struct
{
    uint8_t tag;
    uint8_t length;
    uint32_t value; 
} PCSC_TLV_STRUCTURE;


    static void
printb(unsigned char *buf, size_t len)
{
    size_t i = 0;
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
    PCSC_TLV_STRUCTURE *pcsc_tlv;

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

    for (reader = mszReaders, i = 0;
            dwReaders > 0;
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
        fprintf(stderr, "Could not request the reader's features\n");
        goto err;
    }

    /* get the number of elements instead of the complete size */
    dwRecvLength /= sizeof(PCSC_TLV_STRUCTURE);

    pcsc_tlv = (PCSC_TLV_STRUCTURE *)pbRecvBuffer;
    for (i = 0; i < dwRecvLength; i++) {
        if (pcsc_tlv[i].tag == FEATURE_EXECUTE_PACE) {
            pace_ioctl = pcsc_tlv[i].value;
            break;
        }
    }
    if (0 == pace_ioctl) {
        printf("Reader does not support PACE\n");
        goto err;
    }

    rv = SCardControl(hCard, pace_ioctl,
            pbSendBufferCapabilities, sizeof(pbSendBufferCapabilities),
            pbRecvBuffer, sizeof(pbRecvBuffer), &dwRecvLength);
    if (rv != SCARD_S_SUCCESS) {
        fprintf(stderr, "Could not get reader's PACE capabilities\n");
        goto err;
    }
    printf("GetReadersPACECapabilities successfull, received %d bytes\n",
            dwRecvLength);
    printb(pbRecvBuffer, dwRecvLength);

    dwRecvLength = 0;
    rv = SCardControl(hCard, pace_ioctl,
            pbSendBufferEstablish, sizeof(pbSendBufferEstablish),
            pbRecvBuffer, sizeof(pbRecvBuffer), &dwRecvLength);
    if (rv != SCARD_S_SUCCESS) {
        fprintf(stderr, "Could not establish PACE channel\n");
        goto err;
    }
    printf("EstablishPACEChannel successfull, received %d bytes\n",
            dwRecvLength);
    printb(pbRecvBuffer, dwRecvLength);


    rv = SCardDisconnect(hCard, SCARD_LEAVE_CARD);
    if (rv != SCARD_S_SUCCESS)
        goto err;

    rv = SCardFreeMemory(hContext, mszReaders);
    if (rv != SCARD_S_SUCCESS)
        goto err;


    exit(0);

err:
    /*puts(pcsc_stringify_error(rv));*/
    if (mszReaders)
        SCardFreeMemory(hContext, mszReaders);


    exit(1);
}
