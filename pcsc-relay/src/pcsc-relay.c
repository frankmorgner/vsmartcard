/*
 * Copyright (C) 2009 by Dominik Oepen <oepen@informatik.hu-berlin.de>.
 * Copyright (C) 2010-2012 by Frank Morgner <morgner@informatik.hu-berlin.de>.
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
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "cmdline.h"
#include "pcsc-relay.h"

#ifndef MAX_BUFFER_SIZE
#define MAX_BUFFER_SIZE         264 /**< Maximum Tx/Rx Buffer for short APDU */
#endif

static int doinfo = 0;
static int dodaemon = 1;
int verbose = 0;
static struct rf_driver *rfdriver = &driver_openpicc;
static driver_data_t *rfdriver_data = NULL;
static struct sc_driver *scdriver = &driver_pcsc;
static driver_data_t *scdriver_data = NULL;

/* Forward declaration */
static void daemonize(void);
static void cleanup_exit(int signo);
static void cleanup(void);
static void hexdump(const char *label, unsigned char *buf, size_t len);


#if HAVE_WORKING_FORK
void daemonize(void) {
    pid_t pid, sid;

    /* Fork and continue as child process */
    pid = fork();
    if (pid < 0) {
        RELAY_ERROR("fork: %s\n", strerror(errno));
        goto err;
    }
    if (pid > 0) /* Exit the parent */
        exit(0);

    umask(0);

    /* Create new session and set the process group ID */
    sid = setsid();
    if (sid < 0) {
        RELAY_ERROR("setsid: %s\n", strerror(errno));
        goto err;
    }

    /* Change the current working directory.  This prevents the current
     * directory from being locked; hence not being able to remove it. */
    if (chdir("/") < 0) {
        RELAY_ERROR("chdir: %s\n", strerror(errno));
        goto err;
    }

    /* Redirect standard files to /dev/null */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    return;

err:
    fprintf(stderr, "Failed to start the daemon. Exiting.\n");
    exit(1);
}


#else
/* If fork() is not available, daemon mode will not be supported. I don't want
 * to hassle with any workarounds. */
void daemonize(void)
{ RELAY_ERROR("Daemon mode currently not supported on your system.\n"); exit(1); }
#endif


void cleanup_exit(int signo){
    cleanup();
    exit(0);
}

void cleanup(void) {
    rfdriver->disconnect(rfdriver_data);
    rfdriver_data = NULL;
    scdriver->disconnect(scdriver_data);
    scdriver_data = NULL;
}

void
hexdump(const char *label, unsigned char *buf, size_t len)
{
    size_t i = 0;
    if (verbose >= LEVEL_NORMAL) {
        printf("%s", label);
        while (i < len) {
            printf("%02X", buf[i]);
            i++;
            if (i%20)
                printf(" ");
            else if (i != len)
                printf("\n");
        }
        printf("\n");
    }
}

int main (int argc, char **argv)
{
    /*printf("%s:%d\n", __FILE__, __LINE__);*/
    unsigned char *buf = NULL;
    size_t buflen;
    int i, oindex;

    unsigned char outputBuffer[MAX_BUFFER_SIZE];
    size_t outputLength;

    /*LONG r = SCARD_S_SUCCESS;*/

    struct gengetopt_args_info args_info;


    /* Parse command line */
    if (cmdline_parser (argc, argv, &args_info) != 0)
        exit(1) ;
    switch (args_info.emulator_arg) {
        case emulator_arg_openpicc:
            rfdriver = &driver_openpicc;
            break;
        case emulator_arg_libnfc:
            rfdriver = &driver_libnfc;
            break;
        default:
            exit(2);
    }
    readernum = args_info.reader_arg;

    switch (args_info.connector_arg) {
        case connector_arg_vicc:
            scdriver = &driver_vpcd;
            break;
        case connector_arg_pcsc:
            scdriver = &driver_pcsc;
            break;
        default:
            exit(2);
    }
    vpcdport = args_info.port_arg;

    verbose = args_info.verbose_given;

#if HAVE_SIGACTION
    struct sigaction new_sig, old_sig;

    /* Register signal handlers */
    new_sig.sa_handler = cleanup_exit;
    sigemptyset(&new_sig.sa_mask);
    new_sig.sa_flags = SA_RESTART;
    if ((sigaction(SIGINT, &new_sig, &old_sig) < 0)
            || (sigaction(SIGTERM, &new_sig, &old_sig) < 0)) {
        RELAY_ERROR("sigaction: %s\n", strerror(errno));
        goto err;
    }
#endif


    /* connect to reader and card */
    if (!scdriver->connect(&scdriver_data))
        goto err;


    /* Open the device */
    if (!rfdriver->connect(&rfdriver_data))
        goto err;


    if (!args_info.foreground_flag) {
        INFO("Forking to background...\n");
        verbose = -1;
        daemonize();
    }


    while(1) {
        /* get C-APDU */
        if (!rfdriver->receive_capdu(rfdriver_data, &buf, &buflen))
            goto err;
        if (!buflen || !buf)
            continue;

        hexdump("C-APDU:\n", buf, buflen);


        /* transmit APDU to card */
        outputLength = sizeof outputBuffer;
        if (!scdriver->transmit(scdriver_data, buf, buflen, outputBuffer,
                    &outputLength))
            goto err;


        /* send R-APDU */
        hexdump("R-APDU:\n", outputBuffer, outputLength);

        if (!rfdriver->send_rapdu(rfdriver_data, outputBuffer, outputLength))
            goto err;
    }


err:
    cleanup();

    /*if (r != SCARD_S_SUCCESS) {*/
        /*RELAY_ERROR("%s\n", stringify_error(r));*/
        /*exit(1);*/
    /*}*/

    exit(0);
}
