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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


#define VPCDPORT 35963

int vicc_eject(void);
int vicc_init(unsigned short port);
int vicc_exit(void);
int vicc_transmit(int apdu_len, const char *apdu, char **rapdu);
int vicc_getatr(char** atr);
int vicc_present(void);
int vicc_poweron(void);
int vicc_poweroff(void);
int vicc_reset(void);

#ifdef  __cplusplus
}
#endif
#endif
