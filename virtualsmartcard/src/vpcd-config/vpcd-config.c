/*
 * Copyright (C) 2014 Frank Morgner
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
#include "config.h"
#endif
#include <stdio.h>
#include "vpcd.h"

extern const char *local_ip (void);

#ifdef _WIN32
#define VICC_MAX_SLOTS 1
#include <process.h>
#include <string.h>
#else
#define VICC_MAX_SLOTS VPCDSLOTS
#endif

#define ERROR_STRING "Unable to guess local IP address"



#ifdef HAVE_QRENCODE

#include "qransi.c"

void print_qrcode(const char *uri)
{
    qransi (uri);
}

#else

#define QR_SERVICE_URL "https://api.qrserver.com/v1/create-qr-code/?data="

#ifdef _WIN32

#define IE_PATH "\"C:\\Program Files\\Internet Explorer\\IExplore.exe\" "
void print_qrcode(const char *uri)
{
    char command[200];
    memset(command, 0, sizeof command);
    strcpy(command, IE_PATH);
    strcat(command, QR_SERVICE_URL);
    strcat(command, uri);
    system(command);
}

#else

void print_qrcode(const char *uri)
{
    printf("%s%s\n", QR_SERVICE_URL, uri);
}

#endif

#endif


int main ( int argc , char *argv[] )
{
    char slot;
    char uri[60];
    const char *ip = NULL;
    int fail = 1, port;

    ip = local_ip();
    if (!ip)
        goto err;

    for (slot = 0; slot < VICC_MAX_SLOTS; slot++) {
        port = VPCDPORT+slot;
        printf("VPCD hostname:  %s\n", ip);
        printf("VPCD port:      %d\n", port);
        printf("On your NFC phone with the Remote Smart Card Reader app scan this code:\n");
        sprintf(uri, "vpcd://%s:%d", ip, port);
        print_qrcode(uri);
        if (slot < VICC_MAX_SLOTS-1)
            puts("");
    }

    fail = 0;

err:
    return fail;
}
