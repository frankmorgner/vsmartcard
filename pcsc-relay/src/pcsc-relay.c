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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <winscard.h>
#include <string.h>
#include <errno.h>

#include "cmdline.h"
#include "pcsc-relay.h"
#include "pcscutil.h"

#ifndef MAX_BUFFER_SIZE
#define MAX_BUFFER_SIZE         264 /**< Maximum Tx/Rx Buffer for short APDU */
#endif

static int doinfo = 0;
static int dodaemon = 1;
int verbose = 0;
static struct rf_driver *driver = &driver_openpicc;
static driver_data_t *driver_data = NULL;


static LPSTR readers = NULL;
static SCARDCONTEXT hContext = 0;
static SCARDHANDLE hCard = 0;

/* Forward declaration */
static void daemonize(void);
static void cleanup_exit(int signo);
static void cleanup(void);


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
    driver->disconnect(driver_data);
    driver_data = NULL;
    pcsc_disconnect(hContext, hCard, readers);
}

int main (int argc, char **argv)
{
    /*printf("%s:%d\n", __FILE__, __LINE__);*/
    unsigned char *buf = NULL;
    size_t buflen;
    int i, oindex;

    BYTE outputBuffer[MAX_BUFFER_SIZE];
    DWORD outputLength;

    LONG r = SCARD_S_SUCCESS;
    DWORD ctl, protocol;

    struct gengetopt_args_info args_info;


    /* Parse command line */
    if (cmdline_parser (argc, argv, &args_info) != 0)
        exit(1) ;
    switch (args_info.emulator_arg) {
        case emulator_arg_openpicc:
            driver = &driver_openpicc;
            break;
        case emulator_arg_libnfc:
            driver = &driver_libnfc;
            break;
        default:
            exit(2);
    }


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


    /* Open the device */
    if (!driver->connect(&driver_data))
        goto err;


    /* connect to reader and card */
    r = pcsc_connect(args_info.reader_arg, SCARD_SHARE_EXCLUSIVE, SCARD_PROTOCOL_ANY,
            &hContext, &readers, &hCard, &protocol);
    if (r != SCARD_S_SUCCESS)
        goto err;


    if (!args_info.foreground_flag) {
        INFO("Forking to background...\n");
        verbose = -1;
        daemonize();
    }


    while(1) {
        /* get C-APDU */
        if (!driver->receive_capdu(driver_data, &buf, &buflen))
            goto err;
        if (!buflen || !buf)
            continue;

        if (verbose >= 0)
            printb("C-APDU:\n", buf, buflen);


        /* transmit APDU to card */
        outputLength = sizeof outputBuffer;
        r = pcsc_transmit(protocol, hCard, buf, buflen, outputBuffer, &outputLength);
        if (r != SCARD_S_SUCCESS)
            goto err;


        /* send R-APDU */
        if (verbose >= 0)
            printb("R-APDU:\n", outputBuffer, outputLength);

        if (!driver->send_rapdu(driver_data, outputBuffer, outputLength))
            goto err;
    }


err:
    cleanup();

    if (r != SCARD_S_SUCCESS) {
        RELAY_ERROR("%s\n", stringify_error(r));
        exit(1);
    }

    exit(0);
}
