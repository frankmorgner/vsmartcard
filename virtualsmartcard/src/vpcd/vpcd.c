/*
 * Copyright (C) 2009-2014 Frank Morgner
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
#include "lock.h"

#if HAVE_CONFIG_H
#include "config.h"
#endif

#if (!defined HAVE_DECL_MSG_NOSIGNAL) || !HAVE_DECL_MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define close(s) closesocket(s)
#define snprintf _snprintf
#ifndef AI_NUMERICSERV
#define AI_NUMERICSERV 0
#endif
typedef WORD uint16_t;
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define VPCD_CTRL_LEN 	1

#define VPCD_CTRL_OFF   0
#define VPCD_CTRL_ON    1
#define VPCD_CTRL_RESET 2
#define VPCD_CTRL_ATR	4

static ssize_t sendToVICC(struct vicc_ctx *ctx, size_t size, const unsigned char *buffer);
static ssize_t recvFromVICC(struct vicc_ctx *ctx, unsigned char **buffer);

static ssize_t sendall(int sock, const void *buffer, size_t size);
static ssize_t recvall(int sock, void *buffer, size_t size);

static int opensock(unsigned short port);
static int connectsock(const char *hostname, unsigned short port);

ssize_t sendall(int sock, const void *buffer, size_t size)
{
    size_t sent;
    ssize_t r;

	for (sent = 0; sent < size; sent += r) {
		r = send(sock, (void *) (((unsigned char *) buffer)+sent), size-sent, MSG_NOSIGNAL);

		if (r < 0)
			return r;
	}

    return sent;
}

ssize_t recvall(int sock, void *buffer, size_t size) {
    return recv(sock, buffer, size, MSG_WAITALL|MSG_NOSIGNAL);
}

static int opensock(unsigned short port)
{
    int sock;
    socklen_t yes = 1;
    struct sockaddr_in server_sockaddr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return -1;

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *) &yes, sizeof yes) != 0)
		return -1;

#if HAVE_DECL_SO_NOSIGPIPE
    if (setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, (void *) &yes, sizeof yes) != 0)
		return -1;
#endif

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

static int connectsock(const char *hostname, unsigned short port)
{
	struct addrinfo hints, *res = NULL, *cur;
	int sock = -1;
    char _port[10];

    if (snprintf(_port, sizeof _port, "%hu", port) < 0)
        goto err;
    _port[(sizeof _port) -1] = '\0';

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;

	if (getaddrinfo(hostname, _port, &hints, &res) != 0)
		goto err;

	for (cur = res; cur; cur = cur->ai_next) {
		sock = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
		if (sock < 0)
			continue;

		if (connect(sock, cur->ai_addr, cur->ai_addrlen) != -1)
			break;

		close(sock);
	}

err:
	freeaddrinfo(res);
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

static ssize_t sendToVICC(struct vicc_ctx *ctx, size_t length, const unsigned char* buffer)
{
    ssize_t r;
    uint16_t size;

    if (!ctx || length > 0xFFFF) {
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

static ssize_t recvFromVICC(struct vicc_ctx *ctx, unsigned char **buffer)
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
    int r = 0;
    if (ctx && ctx->client_sock > 0) {
        if (close(ctx->client_sock) < 0) {
            r -= 1;
        }
        ctx->client_sock = -1;
    }
    return r;
}

struct vicc_ctx * vicc_init(const char *hostname, unsigned short port)
{
    struct vicc_ctx *r = NULL;

    struct vicc_ctx *ctx = malloc(sizeof *ctx);
    if (!ctx) {
        goto err;
    }

    ctx->hostname = NULL;
    ctx->io_lock = NULL;
    ctx->server_sock = -1;
    ctx->client_sock = -1;
    ctx->port = port;

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    ctx->io_lock = create_lock();
    if (!ctx->io_lock) {
        goto err;
    }

    if (hostname) {
        ctx->hostname = strdup(hostname);
        if (!ctx->hostname) {
            goto err;
        }
        ctx->client_sock = connectsock(hostname, port);
    } else {
        ctx->server_sock = opensock(port);
        if (ctx->server_sock < 0) {
            goto err;
        }
    }
    r = ctx;

err:
    if (!r) {
        vicc_exit(ctx);
    }

    return r;
}

int vicc_exit(struct vicc_ctx *ctx)
{
    int r = vicc_eject(ctx);
    if (ctx) {
        free_lock(ctx->io_lock);
        free(ctx->hostname);
        if (ctx->server_sock > 0) {
            ctx->server_sock = close(ctx->server_sock);
            if (ctx->server_sock < 0) {
                r -= 1;
            }
        }
#ifdef _WIN32
        WSACleanup();
#endif
    }

    return r;
}

ssize_t vicc_transmit(struct vicc_ctx *ctx,
        size_t apdu_len, const unsigned char *apdu,
        unsigned char **rapdu)
{
    ssize_t r = -1;

    if (ctx && lock(ctx->io_lock)) {
        r = sendToVICC(ctx, apdu_len, apdu);

        if (r > 0)
            r = recvFromVICC(ctx, rapdu);

        unlock(ctx->io_lock);
    }

    if (r <= 0)
        vicc_eject(ctx);

    return r;
}

int vicc_present(struct vicc_ctx *ctx) {
    unsigned char *atr = NULL;

    if (!ctx)
        return 0;

    if (ctx->client_sock < 0) {
        if (ctx->server_sock) {
            /* server mode, try to accept a client */
            ctx->client_sock = waitforclient(ctx->server_sock, 0, 0);
            if (!ctx->client_sock) {
                ctx->client_sock = -1;
            }
        } else {
            /* client mode, try to connect (again) */
            ctx->client_sock = connectsock(ctx->hostname, ctx->port);
        }
    }

    if (ctx->client_sock < 0)
        /* not connected */
        return 0;

    /* get the atr to check if the card is still alive */
    if (vicc_getatr(ctx, &atr) <= 0)
        return 0;

    free(atr);

    return 1;
}

ssize_t vicc_getatr(struct vicc_ctx *ctx, unsigned char **atr) {
    unsigned char i = VPCD_CTRL_ATR;
    return vicc_transmit(ctx, VPCD_CTRL_LEN, &i, atr);
}

int vicc_poweron(struct vicc_ctx *ctx) {
    unsigned char i = VPCD_CTRL_ON;
    int r = 0;

    if (ctx && lock(ctx->io_lock)) {
        r = sendToVICC(ctx, VPCD_CTRL_LEN, &i);
        unlock(ctx->io_lock);
    }

    return r;
}

int vicc_poweroff(struct vicc_ctx *ctx) {
    unsigned char i = VPCD_CTRL_OFF;
    int r = 0;

    if (ctx && lock(ctx->io_lock)) {
        r = sendToVICC(ctx, VPCD_CTRL_LEN, &i);
        unlock(ctx->io_lock);
    }

    return r;
}

int vicc_reset(struct vicc_ctx *ctx) {
    unsigned char i = VPCD_CTRL_RESET;
    int r = 0;

    if (ctx && lock(ctx->io_lock)) {
        r = sendToVICC(ctx, VPCD_CTRL_LEN, &i);
        unlock(ctx->io_lock);
    }

    return r;
}
