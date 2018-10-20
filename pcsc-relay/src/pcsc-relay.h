/*
 * Copyright (C) 2010-2012 Frank Morgner <frankmorgner@gmail.com>.
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


#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef void driver_data_t;
struct rf_driver {
    int (*connect) (driver_data_t **driver_data);
    int (*disconnect) (driver_data_t *driver_data);
    int (*receive_capdu) (driver_data_t *driver_data,
            unsigned char **capdu, size_t *len);
    int (*send_rapdu) (driver_data_t *driver_data,
            const unsigned char *rapdu, size_t len);
};

extern int verbose;

extern struct rf_driver driver_openpicc;
extern struct rf_driver driver_libnfc;
extern struct rf_driver driver_vicc;

struct sc_driver {
    int (*connect) (driver_data_t **driver_data);
    int (*disconnect) (driver_data_t *driver_data);
    int (*transmit) (driver_data_t *driver_data,
        const unsigned char *send, size_t send_len,
        unsigned char *recv, size_t *recv_len);
};

extern struct sc_driver driver_pcsc;
extern unsigned int readernum;
extern struct sc_driver driver_vpcd;
extern unsigned int vpcdport;
extern char *vpcdhostname;
extern unsigned int viccport;
extern char *vicchostname;
extern char *viccatr;

void hexdump(const char *label, unsigned char *buf, size_t len);

#define LEVEL_NORMAL  0
#define LEVEL_INFO    1
#define LEVEL_DEBUG   2
#define PRINTF(...) \
    {if (verbose >= LEVEL_DEBUG) \
        printf (__VA_ARGS__);}
#define DEBUG(...) \
    {if (verbose >= LEVEL_DEBUG) \
        printf (__VA_ARGS__);}
#define INFO(...) \
    {if (verbose >= LEVEL_INFO) \
        printf (__VA_ARGS__);}
#define RELAY_ERROR(...) \
    { \
        if (verbose >= LEVEL_DEBUG) fprintf (stderr, "%s:%u\t", __FILE__, __LINE__); \
        if (verbose >= 0) fprintf (stderr, __VA_ARGS__); \
    }


#ifdef  __cplusplus
}
#endif
#endif
