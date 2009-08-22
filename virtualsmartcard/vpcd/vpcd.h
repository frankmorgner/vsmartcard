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
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define PORT 35963
#define LENLEN 3
int server_sock = -1;
int client_sock = -1;

/*
 * Send all size bytes from buffer to sock
 */
int sendall(int sock, size_t size, const char* buffer) {
    /* send a plain message */
    size_t sent = 0;
    int i;
    while (sent < size) {
        i = send(sock, buffer, size-sent, 0);
        if (i < 0) return i;
        sent += i;
    }
    return 0;
}

/*
 * Receive size bytes from sock
 */
char* recvall(int sock, size_t size) {
    char* buffer = (char*) malloc(size);
    if (buffer == NULL) return NULL;

    if (recv(sock, buffer, size, MSG_WAITALL) < size) {
        free(buffer);
        return NULL;
    }
    return buffer;
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

/*
 * First send length of message to the socket on LENLEN bytes, then send the
 * message itself to the socket.
 */
int sendToVICC(size_t size, const char* buffer) {
    /* send size of message on LENLEN bytes */
    char sizebuf[LENLEN];
    size_t i;
    for (i = 0; i < LENLEN; i++)
        sizebuf[i] = ((size>>8*(LENLEN-i-1)) & 0xff);

    i = sendall(client_sock, LENLEN, sizebuf);
    if (i<0) {
        vicc_eject();
        return i;
    }
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
    unsigned char* sizebuffer = (unsigned char*) recvall(client_sock, LENLEN);
    if (sizebuffer == NULL) {
        vicc_eject();
        return -1;
    }

    /* calculate size */
    int size = 0;
    int i;
    for (i = 0; i < LENLEN; i++) {
        size <<= 8;
        size += sizebuffer[i];
    }
    free(sizebuffer);

    /* receive message */
    *buffer = recvall(client_sock, size);
    if (*buffer == NULL) {
        vicc_eject();
        return -1;
    }

    return size;
}

int vicc_init() {
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) return -1;

    struct sockaddr_in server_sockaddr;
    memset(&server_sockaddr, 0, sizeof(server_sockaddr));
    server_sockaddr.sin_family      = PF_INET;
    server_sockaddr.sin_port        = htons(PORT);
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_sock, (struct sockaddr*)&server_sockaddr,
                sizeof(server_sockaddr)) < 0) return -1;

    if (listen(server_sock, 0) < 0) return -1;

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

int vicc_getatr(char** atr) {
    return vicc_transmit(0, "", atr);
}

int vicc_present() {
    if (client_sock > 0) return 1;
    else {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(server_sock, &rfds);

        /* Wait up to one microsecond. */
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 1;

        if (select(server_sock+1, &rfds, NULL, NULL, &tv) < 0) return -1;

        if (FD_ISSET(server_sock, &rfds)) {
            struct sockaddr_in client_sockaddr;
            socklen_t client_socklen = sizeof(client_sockaddr);
            client_sock = accept(server_sock,
                    (struct sockaddr*)&client_sockaddr,
                    &client_socklen);
            if (client_sock == -1) return -1;

            return 1;
        }
    }

    return 0;
}

int vicc_poweron() {
    char on = 1;
    return sendToVICC(1, &on);
}

int vicc_poweroff() {
    char off = 0;
    return sendToVICC(1, &off);
}
