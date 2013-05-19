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
#include "vpcd.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define close(s) closesocket(s)
typedef WORD uint16_t;
#else
#include <arpa/inet.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define VPCD_CTRL_LEN 	1

#define VPCD_CTRL_OFF   0
#define VPCD_CTRL_ON    1
#define VPCD_CTRL_RESET 2
#define VPCD_CTRL_ATR	4

static int server_sock = -1;
static int client_sock = -1;

ssize_t sendToVICC(size_t size, const unsigned char *buffer);
ssize_t recvFromVICC(unsigned char **buffer);

static ssize_t sendall(int sock, const void *buffer, size_t size);
static ssize_t recvall(int sock, void *buffer, size_t size);

static int opensock(unsigned short port);


ssize_t sendall(int sock, const void *buffer, size_t size)
{
    size_t sent = 0;
    ssize_t r;
    const unsigned char *p = buffer;

    while (sent < size) {
        r = send(sock, (void *) p, size-sent, 0);
        if (r < 0)
            return r;

        sent += r;
        p += r;
    }

    return sent;
}

ssize_t recvall(int sock, void *buffer, size_t size) {
    return recv(sock, buffer, size, MSG_WAITALL);
}

int opensock(unsigned short port)
{
    int sock;
    socklen_t yes = 1;
    struct sockaddr_in server_sockaddr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return -1;

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *) &yes, sizeof yes) != 0)
		return -1;

    memset(&server_sockaddr, 0, sizeof server_sockaddr);
    server_sockaddr.sin_family      = PF_INET;
    server_sockaddr.sin_port        = htons(port);
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *) &server_sockaddr,
                sizeof server_sockaddr) != 0)
        return -1;

    if (listen(sock, 0) != 0)
        return -1;

    return sock;
}

int waitforclient(int server, long secs, long usecs)
{
    fd_set rfds;
    struct sockaddr_in client_sockaddr;
    socklen_t client_socklen = sizeof client_sockaddr;
    struct timeval tv;

    FD_ZERO(&rfds);
#if _WIN32
    /* work around clumsy define of FD_SET in winsock2.h */
#pragma warning(disable:4127)
    FD_SET((SOCKET) server, &rfds);
#pragma warning(default:4127)
#else
    FD_SET(server, &rfds);
#endif

    tv.tv_sec = secs;
    tv.tv_usec = usecs;

    if (select(server+1, &rfds, NULL, NULL, &tv) == -1)
        return -1;

    if (FD_ISSET(server, &rfds))
        return accept(server, (struct sockaddr *) &client_sockaddr,
                &client_socklen);

    return 0;
}

ssize_t sendToVICC(size_t length, const unsigned char* buffer)
{
    ssize_t r;
    uint16_t size;

    /* send size of message on 2 bytes */
    size = htons((uint16_t) length);
    r = sendall(client_sock, (void *) &size, sizeof size);
    if (r == sizeof size)
        /* send message */
        r = sendall(client_sock, buffer, length);

    if (r < 0)
        vicc_eject();

    return r;
}

ssize_t recvFromVICC(unsigned char **buffer)
{
    ssize_t r;
    uint16_t size;
    unsigned char *p = NULL;

    if (!buffer) {
        errno = EINVAL;
        return -1;
    }

    /* receive size of message on 2 bytes */
    r = recvall(client_sock, &size, sizeof size);
    if (r < sizeof size)
        return r;

    size = ntohs(size);

    p = realloc(*buffer, size);
    if (p == NULL) {
        errno = ENOMEM;
        return -1;
    }
    *buffer = p;

    /* receive message */
    return recvall(client_sock, *buffer, size);
}

int vicc_eject(void) {
    if (client_sock > 0) {
        client_sock = close(client_sock);
        if (client_sock < 0) {
            return -1;
        }
    }
    return 0;
}

int vicc_init(unsigned short port) {
    server_sock = opensock(port);
    if (server_sock < 0)
        return -1;

    return 0;
}

int vicc_exit(void) {
    if (server_sock > 0) {
        server_sock = close(server_sock);
        if (server_sock < 0) return -1;
    }

    return 0;
}

ssize_t vicc_transmit(size_t apdu_len,
        const unsigned char *apdu, unsigned char **rapdu)
{
    ssize_t r;

    r = sendToVICC(apdu_len, apdu);

    if (r > 0)
        r = recvFromVICC(rapdu);

    if (r <= 0)
        vicc_eject();

    return r;
}

int vicc_present(void) {
    unsigned char *atr = NULL;

    if (client_sock > 0) {
        if (vicc_getatr(&atr) <= 0)
            return 0;

        free(atr);

        return 1;
    } else {
        /* Wait up to one microsecond. */
        client_sock = waitforclient(server_sock, 0, 1);

        if (client_sock < 0)
            return -1;
    }

    return 0;
}

ssize_t vicc_getatr(unsigned char **atr) {
    char i = VPCD_CTRL_ATR;
    return vicc_transmit(VPCD_CTRL_LEN, &i, atr);
}

int vicc_poweron(void) {
    char i = VPCD_CTRL_ON;
    return sendToVICC(VPCD_CTRL_LEN, &i);
}

int vicc_poweroff(void) {
    char i = VPCD_CTRL_OFF;
    return sendToVICC(VPCD_CTRL_LEN, &i);
}

int vicc_reset(void) {
    char i = VPCD_CTRL_RESET;
    return sendToVICC(VPCD_CTRL_LEN, &i);
}
