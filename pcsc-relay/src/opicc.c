/*
 * Copyright (C) 2010 Frank Morgner
 *
 * This file is part of pcsc-relay.
 *
 * pcsc-relay is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * pcsc-relay is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * pcsc-relay.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "pcsc-relay.h"

struct picc_data {
    char *buf;
    size_t buflen;
    FILE *fd;
};
static size_t picc_encode_rapdu(const unsigned char *inbuf, size_t inlen, char **outbuf);
static size_t picc_decode_apdu(const char *inbuf, size_t inlen, unsigned char **outbuf);


size_t picc_encode_rapdu(const unsigned char *inbuf, size_t inlen, char **outbuf)
{
    char *p;
    const unsigned char *next;
    size_t length;

    if (!inbuf || inlen > 0xffff || !outbuf)
        goto err;

    length = 5+inlen*3+1;
    p = realloc(*outbuf, length);
    if (!p) {
        if (debug || verbose)
            fprintf(stderr, "Error allocating memory for encoded R-APDU\n");
        goto err;
    }
    *outbuf = p;

    sprintf(*outbuf, "%04X:", inlen);

    next = inbuf;
    /* let p point behind ':' */
    p += 5;
    while (inbuf+inlen > next) {
        sprintf(p," %02X",*next);
        next++;
        p += 3;
    }

    return length;

err:
    return 0;
}

size_t picc_decode_apdu(const char *inbuf, size_t inlen, unsigned char **outbuf)
{
    size_t pos, length;
    unsigned char buf[0xffff];
    char *end, *p;
    unsigned long int b;

    if (!outbuf || inbuf == NULL || inlen == 0 || inbuf[0] == '\0') {
        /* Ignore invalid parameters, empty and 'RESET' lines */
        goto noapdu;
    }

    length = strtoul(inbuf, &end, 16);

    /* check for ':' right behind the length */
    if (inbuf+inlen < end+1 || end[0] != ':')
        goto noapdu;
    end++;

    p = realloc(*outbuf, length);
    if (!p) {
        if (debug || verbose)
            fprintf(stderr, "Error allocating memory for decoded C-APDU\n");
        goto noapdu;
    }
    *outbuf = p;

    pos = 0;
    while(inbuf+inlen > end && length > pos) {
        b = strtoul(end, &end, 16);
        if (b > 0xff) {
            if (debug || verbose)
                fprintf(stderr, "%s:%u Error decoding C-APDU\n", __FILE__, __LINE__);
            goto noapdu;
        }

        (*outbuf)[pos++] = b;
    }

    return length;

noapdu:
    return 0;
}


static int picc_connect(void **driver_data)
{
    struct picc_data *data;

    if (!driver_data)
        return 0;

    data = realloc(*driver_data, sizeof *data);
    if (!data)
        return 0;
    memset(data, 0, sizeof *data);
    *driver_data = data;

    data->fd = fopen(PICCDEV, "a+"); /*O_NOCTTY ?*/
    if (!data->fd) {
        if (debug || verbose)
            fprintf(stderr,"Error opening %s\n", PICCDEV);
        return 0;
    }
    if (debug || verbose)
        printf("Connected to %s\n", PICCDEV);

    return 1;
}

static int picc_disconnect(void *driver_data)
{
    struct picc_data *data = driver_data;
    if (data) {
        if (data->fd)
            fclose(data->fd); 
        if (data->buf)
            free(data->buf);
    }

    return 1;
}

static int picc_receive_capdu(void *driver_data,
        unsigned char **capdu, size_t *len)
{
    ssize_t linelen;
    struct picc_data *data = driver_data;

    if (!data || !capdu || !len)
        return 0;


    /* read C-APDU */
    linelen = getline(&data->buf, &data->buflen, data->fd);
    if (linelen < 0) {
        if (linelen < 0) {
            if (debug || verbose)
                fprintf(stderr,"Error reading from %s\n", PICCDEV);
            return 0;
        }
    }
    if (linelen == 0) {
        *len = 0;
        return 1;
    }
    fflush(data->fd);

    if (debug)
        printf("%s\n", data->buf);


    /* decode C-APDU */
    *len = picc_decode_apdu(data->buf, linelen, capdu);

    return 1;
}

static int picc_send_rapdu(void *driver_data,
        const unsigned char *rapdu, size_t len)
{
    size_t buflen;
    struct picc_data *data = driver_data;

    if (!data || !rapdu)
        return 0;


    /* encode R-APDU */
    buflen = picc_encode_rapdu(rapdu, len, &data->buf);
    if (buflen > data->buflen)
        data->buflen = buflen;


    /* write R-APDU */
    if (debug)
        printf("INF: Writing R-APDU\n\n%s\n\n", data->buf);

    if (fprintf(data->fd,"%s\r\n", (char *) data->buf) < 0
            || fflush(data->fd) != 0) {
        return 0;
    }

    return 1;
}


struct rf_driver driver_openpicc = {
    .data = NULL,
    .connect = picc_connect,
    .disconnect = picc_disconnect,
    .receive_capdu = picc_receive_capdu,
    .send_rapdu = picc_send_rapdu,
};
