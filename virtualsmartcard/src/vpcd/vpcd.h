/*
 * Copyright (C) 2009-2013 Frank Morgner
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
#ifndef _VPCD_H_
#define _VPCD_H_

#if HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _WIN32
#include <stddef.h>
#include <winsock2.h>
#ifndef HAVE_CONFIG_H
/* we assume that ./configure has not defined ssize_t for us */
typedef int ssize_t;
#endif
#else
#define SOCKET int
#include <unistd.h>
#endif

#define VPCD_CTRL_LEN 	1

#define VPCD_CTRL_OFF   0
#define VPCD_CTRL_ON    1
#define VPCD_CTRL_RESET 2
#define VPCD_CTRL_ATR	4

struct vicc_ctx {
        SOCKET server_sock;
        SOCKET client_sock;
        char *hostname;
        unsigned short port;
        void *io_lock;
};

#ifdef __cplusplus
extern "C" {
#endif


/** Standard port of the virtual smart card reader */
#define VPCDPORT 35963

/**
 * @brief Initialize the module
 *
 * @param[in] hostname Set hostname to something different to NULL if you want
 *                     to connect the vpcd to a socket opened by vicc.
 *                     Otherwise (default behavior) the vpcd will open a port
 *                     for vicc
 * @param[in] port     Port to connect to or to open (see \a hostname)
 *
 * @return On success, the call returns the initialized context
 *         On error, NULL is returned.
 */
struct vicc_ctx * vicc_init(const char *hostname, unsigned short port);

int vicc_exit(struct vicc_ctx *ctx);
int vicc_eject(struct vicc_ctx *ctx);

int vicc_connect(struct vicc_ctx *ctx, long secs, long usecs);
int vicc_present(struct vicc_ctx *ctx);
int vicc_poweron(struct vicc_ctx *ctx);
int vicc_poweroff(struct vicc_ctx *ctx);
int vicc_reset(struct vicc_ctx *ctx);

/**
 * @brief Receive ATR from the virtual smart card.
 *
 * @param[in,out] atr ATR received. Memory will be reused (via \a realloc) and
 *                    should be freed by the caller if no longer needed.
 *
 * @return On success, the call returns the number of bytes received.
 *         On error, -1 is returned, and errno is set appropriately.
 */
ssize_t vicc_getatr(struct vicc_ctx *ctx, unsigned char** atr);

/**
 * @brief Send an APDU to the virtual smart card.
 *
 * @param[in]     apdu_len Number of bytes to send
 * @param[in]     apdu     Data to be sent
 * @param[in,out] rapdu    Data received. Memory will be reused (via \a
 *                         realloc) and should be freed by the caller if no
 *                         longer needed.
 *
 * @return On success, the call returns the number of bytes received.
 *         On error, -1 is returned, and errno is set appropriately.
 */
ssize_t vicc_transmit(struct vicc_ctx *ctx,
        size_t apdu_len, const unsigned char *apdu,
        unsigned char **rapdu);

#ifdef  __cplusplus
}
#endif
#endif
