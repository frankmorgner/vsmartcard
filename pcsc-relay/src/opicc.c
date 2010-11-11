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
#include <errno.h>

#include "config.h"
#include "pcsc-relay.h"

struct picc_data {
    char *buf;
    size_t bufmax;
    FILE *fd;
};
static int picc_encode_rapdu(const unsigned char *inbuf, size_t inlen,
        char **outbuf, size_t *outlen);
static int picc_decode_apdu(const char *inbuf, size_t inlen,
        unsigned char **outbuf, size_t *outlen);


int picc_encode_rapdu(const unsigned char *inbuf, size_t inlen,
        char **outbuf, size_t *outlen)
{
    char *p;
    const unsigned char *next;
    size_t length;

    if (!inbuf || inlen > 0xffff || !outbuf)
        return 0;

    length = 5+inlen*3+1;
    p = realloc(*outbuf, length);
    if (!p) {
        ERROR("Error allocating memory for encoded R-APDU\n");
        return 0;
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

    *outlen = length;
    return 1;
}

int picc_decode_apdu(const char *inbuf, size_t inlen,
        unsigned char **outbuf, size_t *outlen)
{
    size_t pos, length;
    char *end, *p;
    unsigned long int b;

    if (!outbuf || !outlen) {
        return 0;
    }
    if (inbuf == NULL || inlen == 0 || inbuf[0] == '\0') {
        /* Ignore empty and 'RESET' lines */
        *outlen = 0;
        return 1;
    }

    length = strtoul(inbuf, &end, 16);

    /* check for ':' right behind the length */
    if (inbuf+inlen < end+1 || end[0] != ':')
        return 0;
    end++;

    p = realloc(*outbuf, length);
    if (!p) {
        ERROR("Error allocating memory for decoded C-APDU\n");
        return 0;
    }
    *outbuf = p;

    pos = 0;
    while(inbuf+inlen > end && length > pos) {
        b = strtoul(end, &end, 16);
        if (b > 0xff) {
            ERROR( "%s:%u Error decoding C-APDU\n", __FILE__, __LINE__);
            return 0;
        }

        (*outbuf)[pos++] = b;
    }

    *outlen = length;

    return 1;
}


static int picc_connect(void **driver_data)
{
    struct picc_data *data;

    if (!driver_data)
        return 0;


    data = realloc(*driver_data, sizeof *data);
    if (!data)
        return 0;
    *driver_data = data;

    data->fd = fopen(PICCDEV, "a+"); /*O_NOCTTY ?*/
    if (!data->fd) {
        ERROR("Error opening %s: %s\n", PICCDEV, strerror(errno));
        return 0;
    }

    data->buf = NULL;
    data->bufmax = 0;


    if (verbose >= 0)
        printf("Connected to %s\n", PICCDEV);

    return 1;
}

static int picc_disconnect(void *driver_data)
{
    struct picc_data *data = driver_data;


    if (data) {
        if (data->fd)
            fclose(data->fd); 
        data->fd = NULL;
        free(data->buf);
        data->buf = NULL;
        data->bufmax = 0;
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
    linelen = getline(&data->buf, &data->bufmax, data->fd);
    if (linelen < 0) {
        if (linelen < 0) {
            ERROR("Error reading from %s: %s\n", PICCDEV, strerror(errno));
            return 0;
        }
    }
    if (linelen == 0) {
        *len = 0;
        return 1;
    }
    fflush(data->fd);

    DEBUG("%s\n", data->buf);


    /* decode C-APDU */
    return picc_decode_apdu(data->buf, linelen, capdu, len);
}

static int picc_send_rapdu(void *driver_data,
        const unsigned char *rapdu, size_t len)
{
    struct picc_data *data = driver_data;
    size_t buflen;

    if (!data || !rapdu)
        return 0;


    /* encode R-APDU */
    if (!picc_encode_rapdu(rapdu, len, &data->buf, &buflen))
        return 0;
    if (data->bufmax < buflen)
        data->bufmax = buflen;


    /* write R-APDU */
    DEBUG("INF: Writing R-APDU\r\n%s\r\n", data->buf);

    if (fprintf(data->fd,"%s\r\n", (char *) data->buf) < 0) {
        ERROR("fprintf: %s\n", strerror(errno));
        return 0;
    }
    if (fflush(data->fd) != 0) {
        ERROR("fflush: %s\n", strerror(errno));
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
