/*
 * Copyright (C) 2010 Frank Morgner
 *
 * This file is part of ccid.
 *
 * ccid is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * ccid is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ccid.  If not, see <http://www.gnu.org/licenses/>.
 */
/**
 * @file
 */
#ifndef _PCSC_RELAY_H
#define _PCSC_RELAY_H


#ifdef __cplusplus
extern "C" {
#endif

struct rf_driver {
    void *data;
    int (*connect) (void **driver_data);
    int (*disconnect) (void *driver_data);
    int (*receive_capdu) (void *driver_data,
            unsigned char **capdu, size_t *len);
    int (*send_rapdu) (void *driver_data,
            const unsigned char *rapdu, size_t len);
};

extern int verbose, debug;

extern struct rf_driver driver_openpicc;
extern struct rf_driver driver_libnfc;


#ifdef  __cplusplus
}
#endif
#endif
