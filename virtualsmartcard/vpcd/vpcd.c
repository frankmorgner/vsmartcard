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
#include <arpa/inet.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


#include <stdlib.h>

#include "vpcd.h"

static int server_sock = -1;
static int client_sock = -1;

/*
 * Send all size bytes from buffer to sock
 */
static int sendall(int sock, size_t size, const char* buffer);
/*
 * Receive size bytes from sock
 */
static char* recvall(int sock, size_t size);
/*
 * Open a TCP socket and listen.
 */
static int opensock(unsigned short port);


int sendall(int sock, size_t size, const char* buffer) {
    size_t sent = 0;
    int i;
    while (sent < size) {
        i = send(sock, buffer, size-sent, 0);
        if (i < 0) return i;
        sent += i;
    }
    return 0;
}

char* recvall(int sock, size_t size) {
    char* buffer = (char*) malloc(size);
    if (buffer == NULL) return NULL;

    if (recv(sock, buffer, size, MSG_WAITALL) < size) {
        free(buffer);
        return NULL;
    }
    return buffer;
}

int opensock(unsigned short port)
{
    int sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in server_sockaddr;
    memset(&server_sockaddr, 0, sizeof(server_sockaddr));
    server_sockaddr.sin_family      = PF_INET;
    server_sockaddr.sin_port        = htons(VPCDPORT);
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr*)&server_sockaddr,
                sizeof(server_sockaddr)) < 0) return -1;

    if (listen(sock, 0) < 0) return -1;

    return sock;
}

int waitforclient(int server, long int secs, long int usecs) {
    int sock;

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(server, &rfds);

    /* Wait up to one microsecond. */
    struct timeval tv;
    tv.tv_sec = secs;
    tv.tv_usec = usecs;

    if (select(server+1, &rfds, NULL, NULL, &tv) < 0) return -1;

    if (FD_ISSET(server, &rfds)) {
        struct sockaddr_in client_sockaddr;
        socklen_t client_socklen = sizeof(client_sockaddr);
        sock = accept(server,
                (struct sockaddr*)&client_sockaddr,
                &client_socklen);
    }

    return sock;
}

int sendToVICC(uint16_t size, const char* buffer) {
    /* send size of message */
    uint16_t i = htons(size);
    i = sendall(client_sock, sizeof i, (char *) &i);
    if (i<0) {
        vicc_eject();
        return i;
    }
    /* send message */
    i = sendall(client_sock, size, buffer);
    if (i<0) {
        vicc_eject();
        return i;
    }

    return 0;
}

/*
 * Receive a message from icc
 */
int recvFromVICC(char** buffer) {
    /* receive size of message on LENLEN bytes */
    uint16_t *p = (uint16_t *) recvall(client_sock, sizeof *p);
    if (p == NULL) {
        vicc_eject();
        return -1;
    }
    uint16_t size = ntohs(*p);
    free(p);

    /* receive message */
    *buffer = recvall(client_sock, size);
    if (*buffer == NULL) {
        vicc_eject();
        return -1;
    }

    return size;
}

int vicc_eject() {
    if (client_sock > 0) {
        client_sock = close(client_sock);
        if (client_sock < 0) {
            return -1;
        }
    }
    return 0;
}

int vicc_init() {
    server_sock = opensock(VPCDPORT);
    if (server_sock < 0)
        return -1;

    return 0;
}

int vicc_exit() {
    if (server_sock > 0) {
        server_sock = close(server_sock);
        if (server_sock < 0) return -1;
    }

    return 0;
}

int vicc_transmit(int apdu_len, const char *apdu, char **rapdu) {
    if (sendToVICC(apdu_len, apdu) < 0) return -1;

    return recvFromVICC(rapdu);
}

int vicc_present() {
    if (client_sock > 0) return 1;
    else {
        /* Wait up to one microsecond. */
        client_sock = waitforclient(server_sock, 0, 1);
        if (client_sock < 0)
            return -1;
    }

    return 0;
}

int vicc_getatr(char** atr) {
    char i = VPCD_CTRL_ATR;
    return vicc_transmit(VPCD_CTRL_LEN, &i, atr);
}

int vicc_poweron() {
    char i = VPCD_CTRL_ON;
    return sendToVICC(VPCD_CTRL_LEN, &i);
}

int vicc_poweroff() {
    char i = VPCD_CTRL_OFF;
    return sendToVICC(VPCD_CTRL_LEN, &i);
}

int vicc_reset() {
    char i = VPCD_CTRL_RESET;
    return sendToVICC(VPCD_CTRL_LEN, &i);
}
