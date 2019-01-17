/*
 * Copyright (C) 2010-2012 Frank Morgner <frankmorgner@gmail.com>.
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
#include "config.h"
#include "pcsc-relay.h"
#include <stdio.h>

#if HAVE_TCGETATTR
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>


struct picc_data {
    char *e_rapdu;
    char *line;
    size_t linemax;
    FILE *fd;
};
static int picc_encode_rapdu(const unsigned char *inbuf, size_t inlen,
        char **outbuf, size_t *outlen);
static int picc_decode_apdu(const char *inbuf, size_t inlen,
        unsigned char **outbuf, size_t *outlen);
static void un_braindead_ify_device(int fd);


int picc_encode_rapdu(const unsigned char *inbuf, size_t inlen,
        char **outbuf, size_t *outlen)
{
    char *p;
    const unsigned char *next;
    size_t length;

    if (!inbuf || inlen > 0xffff || !outbuf)
        return 0;

    /* length with ':' + for each byte ' ' with hex + '\0' */
    length = 5+inlen*3+1;
    p = realloc(*outbuf, length);
    if (!p) {
        RELAY_ERROR("Error allocating memory for encoded R-APDU\n");
        return 0;
    }
    *outbuf = p;
    *outlen = length;

    /* write length of R-APDU */
    sprintf(p, "%0lX:", (unsigned long) inlen);

    /* next points to the next byte to encode */
    next = inbuf;
    /* let p point behind ':' to write hex encoded bytes */
    p += 5;
    while (inbuf+inlen > next) {
        sprintf(p, " %02X", *next);
        next++;
        p += 3;
    }

    return 1;
}

int picc_decode_apdu(const char *inbuf, size_t inlen,
        unsigned char **outbuf, size_t *outlen)
{
    size_t pos, length;
    char *end;
    unsigned char *p;
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
    if (inbuf+inlen < end+1 || end[0] != ':') {
        *outlen = 0;
        return 1;
    }
    end++;

    if (length != 0) {
        p = realloc(*outbuf, length);
        if (!p) {
            RELAY_ERROR("Error allocating memory for decoded C-APDU\n");
            return 0;
        }
        *outbuf = p;
    }

    pos = 0;
    while(inbuf+inlen > end && length > pos) {
        b = strtoul(end, &end, 16);
        if (b > 0xff) {
            RELAY_ERROR("Error decoding C-APDU\n");
            return 0;
        }

        p[pos++] = b;
    }

    *outlen = length;

    return 1;
}

void un_braindead_ify_device(int fd)
{
    /* For some stupid reason the default setting for a serial console is to
     * use XON/XOFF. This means that some of the bytes will be dropped, making
     * the device completely unusable for a binary protocol.  Remove that
     * setting */
    struct termios options;

    tcgetattr (fd, &options);

    options.c_lflag = 0;
    options.c_iflag &= IGNPAR | IGNBRK;
    options.c_oflag &= IGNPAR | IGNBRK;

    cfsetispeed (&options, B115200);
    cfsetospeed (&options, B115200);

    if (tcsetattr (fd, TCSANOW, &options))
        RELAY_ERROR("Can't set device attributes");
}


static int picc_connect(driver_data_t **driver_data)
{
    struct picc_data *data;

    if (!driver_data)
        return 0;


    data = realloc(*driver_data, sizeof *data);
    if (!data)
        return 0;
    *driver_data = data;

    data->e_rapdu = NULL;
    data->line = NULL;
    data->linemax = 0;

    data->fd = fopen(PICCDEV, "a+"); /*O_NOCTTY ?*/
    if (!data->fd) {
        RELAY_ERROR("Error opening %s: %s\n", PICCDEV, strerror(errno));
        return 0;
    }
    un_braindead_ify_device(fileno(data->fd));


    PRINTF("Connected to %s\n", PICCDEV);

    return 1;
}

static int picc_disconnect(driver_data_t *driver_data)
{
    struct picc_data *data = driver_data;


    if (data) {
        if (data->fd)
            fclose(data->fd); 
        free(data->e_rapdu);
        free(data->line);
        free(data);
    }


    return 1;
}

static int picc_receive_capdu(driver_data_t *driver_data,
        unsigned char **capdu, size_t *len)
{
    ssize_t linelen;
    struct picc_data *data = driver_data;

    if (!data || !capdu || !len)
        return 0;


    /* read C-APDU */
    linelen = getline(&data->line, &data->linemax, data->fd);
    if (linelen <= 0) {
        if (linelen < 0) {
            RELAY_ERROR("Error reading from %s: %s\n", PICCDEV, strerror(errno));
            return 0;
        }
        if (linelen == 0) {
            *len = 0;
            return 1;
        }
    }
    if (fflush(data->fd) != 0)
        RELAY_ERROR("Warning, fflush failed: %s\n", strerror(errno));

    DEBUG("%s", data->line);


    /* decode C-APDU */
    return picc_decode_apdu(data->line, linelen, capdu, len);
}

static int picc_send_rapdu(driver_data_t *driver_data,
        const unsigned char *rapdu, size_t len)
{
    struct picc_data *data = driver_data;
    size_t buflen;

    if (!data || !rapdu)
        return 0;


    /* encode R-APDU */
    if (!picc_encode_rapdu(rapdu, len, &data->e_rapdu, &buflen))
        return 0;


    /* write R-APDU */
    DEBUG("INF: Writing R-APDU\r\n%s\r\n", data->e_rapdu);

    if (fprintf(data->fd,"%s\r\n", data->e_rapdu) < 0) {
        RELAY_ERROR("Error writing to %s: %s\n", PICCDEV, strerror(errno));
        return 0;
    }
    if (fflush(data->fd) != 0)
        RELAY_ERROR("Warning, fflush failed: %s\n", strerror(errno));


    return 1;
}


#else
/* If tcgetattr() is not available, OpenPICC backend will not be supported. I
 * don't want to hassle with any workarounds. */
#define OPICCERR RELAY_ERROR("OpenPICC backend currently not supported on your system.\n")
static int picc_send_rapdu(driver_data_t *driver_data,
        const unsigned char *rapdu, size_t len)
{ OPICCERR; return 0; }
static int picc_receive_capdu(driver_data_t *driver_data,
        unsigned char **capdu, size_t *len)
{ OPICCERR; return 0; }
static int picc_disconnect(driver_data_t *driver_data)
{ OPICCERR; return 0; }
static int picc_connect(driver_data_t **driver_data)
{ OPICCERR; return 0; }


#endif
struct rf_driver driver_openpicc = {
    .connect = picc_connect,
    .disconnect = picc_disconnect,
    .receive_capdu = picc_receive_capdu,
    .send_rapdu = picc_send_rapdu,
};
