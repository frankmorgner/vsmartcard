/*
 * Copyright (C) 2010 Frank Morgner
 *
 * This file is part of pcsc-relay.
 *
 * pcsc-relay is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * pcsc-relay is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * pcsc-relay.  If not, see <http://www.gnu.org/licenses/>.
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

extern int verbose;

extern struct rf_driver driver_openpicc;
extern struct rf_driver driver_libnfc;

#define LEVEL_INFO    1
#define LEVEL_DEBUG   2
#define DEBUG(...) \
    {if (verbose >= LEVEL_DEBUG) \
        printf (__VA_ARGS__);}
#define INFO(...) \
    {if (verbose >= LEVEL_INFO) \
        printf (__VA_ARGS__);}
#define ERROR(...) \
    {if (verbose >= 0) \
        fprintf (stderr, __VA_ARGS__);}


#ifdef  __cplusplus
}
#endif
#endif
