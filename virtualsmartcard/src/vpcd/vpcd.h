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
#ifndef _VPCD_H_
#define _VPCD_H_

#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif


/** Standard port of the virtual smart card reader */
#define VPCDPORT 35963

int vicc_init(unsigned short port);
int vicc_exit(void);
int vicc_eject(void);

int vicc_present(void);
int vicc_poweron(void);
int vicc_poweroff(void);
int vicc_reset(void);

/**
 * @brief Receive ATR from the virtual smart card.
 *
 * @param[in,out] atr ATR received. Memory will be reused (via \a realloc) and
 *                    should be freed by the caller if no longer needed.
 *
 * @return On success, the call returns the number of bytes received.
 *         On error, -1 is returned, and errno is set appropriately.
 */
ssize_t vicc_getatr(unsigned char** atr);

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
ssize_t vicc_transmit(size_t apdu_len, const unsigned char *apdu,
        unsigned char **rapdu);

#ifdef  __cplusplus
}
#endif
#endif
