/*
 * Copyright (C) 2010 Dominik Oepen, Frank Morgner
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

#include "config.h"
#include "pcsc-relay.h"
#include "pcscutil.h"

struct rf_driver *driver = &driver_openpicc;


static LPSTR readers = NULL;
static SCARDCONTEXT hContext = 0;
static SCARDHANDLE hCard = 0;
int verbose = 0, debug = 0;

/* Forward declaration */
static void daemonize();
static void cleanup_exit(int signo);
static void cleanup(void);

void daemonize() {
    pid_t pid, sid;

    /*Fork and continue as child process*/
    pid = fork();
    if (pid < 0)
        goto err;
    if (pid > 0) /* Exit the parent */
        exit(0);

    umask(0);

    /* Create new session and set the process group ID */
    sid = setsid();
    if (sid < 0)
        goto err;
         
    /* Change the current working directory.  This prevents the current
       directory from being locked; hence not being able to remove it. */
    if (chdir("/") < 0)
        goto err;

    /* Redirect standard files to /dev/null */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    return;

err:
    fprintf(stderr, "Failed to start the daemon. Exiting.\n");
    exit(1);
}

void cleanup_exit(int signo){
    cleanup();
    exit(0);
}

void cleanup(void) {
    driver->disconnect(driver->data);
    pcsc_disconnect(hContext, hCard, readers);
}

int main (int argc, char **argv)
{
    /*printf("%s:%d\n", __FILE__, __LINE__);*/
    void *buf = NULL;
    size_t buflen;

    BYTE outputBuffer[MAX_BUFFER_SIZE];
    DWORD outputLength;

    LONG r = SCARD_S_SUCCESS;
    DWORD ctl, protocol;

    char *read = NULL;
    size_t readlen = 0;

    unsigned int readernum = 0;

    struct sigaction new_sig, old_sig;


    if (argc > 1) {
        readernum = strtoul(argv[1], NULL, 10);
        if (argc > 2) {
            if (0 == strcmp(argv[2], "verbose")) 
                verbose++;
            else if (0 == strcmp(argv[2], "debug"))
                debug++;
            else
                goto parse_err;
            if (argc > 3) {
parse_err:                
                fprintf(stderr, "Usage:  "
                        "%s [reader number] [verbose | debug]\n", argv[0]);
                exit(2);
            }
        }
    }

    if (!verbose && !debug)
        daemonize();

    /* Register signal handlers */
    new_sig.sa_handler = cleanup_exit;
    sigemptyset(&new_sig.sa_mask);
    new_sig.sa_flags = SA_RESTART;
    if ((sigaction(SIGINT, &new_sig, &old_sig) < 0)
            || (sigaction(SIGTERM, &new_sig, &old_sig) < 0))
        goto err;


    /* Open the device */
    driver->connect(&driver->data);

    /* connect to reader and card */
    r = pcsc_connect(readernum, SCARD_SHARE_EXCLUSIVE, SCARD_PROTOCOL_ANY,
            &hContext, &readers, &hCard, &protocol);
    if (r != SCARD_S_SUCCESS)
        goto err;


    while(1) {
        driver->receive_capdu(driver->data, (unsigned char **) &buf, &buflen);

        if (!verbose)
            printb("C-APDU: ===================================================\n", buf, buflen);


        /* transmit APDU to card */
        outputLength = MAX_BUFFER_SIZE;
        r = pcsc_transmit(protocol, hCard, buf, buflen, outputBuffer, &outputLength);
        if (r != SCARD_S_SUCCESS)
            goto err;

        if (!verbose)
            printb("R-APDU:\n", outputBuffer, outputLength);


        driver->send_rapdu(driver->data, outputBuffer, outputLength);
    }

err:
    cleanup();

    exit(r == SCARD_S_SUCCESS ? 0 : 1);
}
