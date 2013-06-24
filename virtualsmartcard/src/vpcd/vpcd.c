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

ssize_t sendToVICC(struct vicc_ctx *ctx, size_t size, const unsigned char *buffer);
ssize_t recvFromVICC(struct vicc_ctx *ctx, unsigned char **buffer);

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

ssize_t sendToVICC(struct vicc_ctx *ctx, size_t length, const unsigned char* buffer)
{
    ssize_t r;
    uint16_t size;

    if (!ctx) {
        errno = EINVAL;
        return -1;
    }

    /* send size of message on 2 bytes */
    size = htons((uint16_t) length);
    r = sendall(ctx->client_sock, (void *) &size, sizeof size);
    if (r == sizeof size)
        /* send message */
        r = sendall(ctx->client_sock, buffer, length);

    if (r < 0)
        vicc_eject(ctx);

    return r;
}

ssize_t recvFromVICC(struct vicc_ctx *ctx, unsigned char **buffer)
{
    ssize_t r;
    uint16_t size;
    unsigned char *p = NULL;

    if (!buffer || !ctx) {
        errno = EINVAL;
        return -1;
    }

    /* receive size of message on 2 bytes */
    r = recvall(ctx->client_sock, &size, sizeof size);
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
    return recvall(ctx->client_sock, *buffer, size);
}

int vicc_eject(struct vicc_ctx *ctx)
{
    if (ctx && ctx->client_sock > 0) {
        ctx->client_sock = close(ctx->client_sock);
        if (ctx->client_sock < 0) {
            return -1;
        }
    }
    return 0;
}

struct vicc_ctx * vicc_init(unsigned short port)
{
    struct vicc_ctx *ctx = malloc(sizeof *ctx);
    if (!ctx) {
        return NULL;
    }

    ctx->server_sock = opensock(port);
    if (ctx->server_sock < 0) {
        free(ctx);
        return NULL;
    }

    ctx->client_sock = -1;

    return ctx;
}

int vicc_exit(struct vicc_ctx *ctx)
{
    int r = vicc_eject(ctx);
    if (ctx) {
        if (ctx->server_sock > 0) {
            ctx->server_sock = close(ctx->server_sock);
            if (ctx->server_sock < 0) {
                r -= 1;
            }
        }
    }

    return r;
}

ssize_t vicc_transmit(struct vicc_ctx *ctx,
        size_t apdu_len, const unsigned char *apdu,
        unsigned char **rapdu)
{
    ssize_t r;

    r = sendToVICC(ctx, apdu_len, apdu);

    if (r > 0)
        r = recvFromVICC(ctx, rapdu);

    if (r <= 0)
        vicc_eject(ctx);

    return r;
}

int vicc_present(struct vicc_ctx *ctx) {
    unsigned char *atr = NULL;

    if (ctx->client_sock > 0) {
        if (vicc_getatr(ctx, &atr) <= 0)
            return 0;

        free(atr);

        return 1;
    } else {
        /* Wait up to one microsecond. */
        ctx->client_sock = waitforclient(ctx->server_sock, 0, 1);

        if (ctx->client_sock < 0)
            return -1;
    }

    return 0;
}

ssize_t vicc_getatr(struct vicc_ctx *ctx, unsigned char **atr) {
    unsigned char i = VPCD_CTRL_ATR;
    return vicc_transmit(ctx, VPCD_CTRL_LEN, &i, atr);
}

int vicc_poweron(struct vicc_ctx *ctx) {
    unsigned char i = VPCD_CTRL_ON;
    return sendToVICC(ctx, VPCD_CTRL_LEN, &i);
}

int vicc_poweroff(struct vicc_ctx *ctx) {
    unsigned char i = VPCD_CTRL_OFF;
    return sendToVICC(ctx, VPCD_CTRL_LEN, &i);
}

int vicc_reset(struct vicc_ctx *ctx) {
    unsigned char i = VPCD_CTRL_RESET;
    return sendToVICC(ctx, VPCD_CTRL_LEN, &i);
}
