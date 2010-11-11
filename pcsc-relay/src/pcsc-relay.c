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

#include "binutil.h"
#include "config.h"
#include "pcsc-relay.h"
#include "pcscutil.h"

#define OPT_HELP        'h'
#define OPT_READER      'r'
#define OPT_VERBOSE     'v'
#define OPT_FOREGROUND  'f'
#define OPT_EMULATOR    'e'
static const struct option options[] = {
    { "help", no_argument, NULL, OPT_HELP },
    { "reader",	required_argument, NULL, OPT_READER },
    { "foreground", no_argument, NULL, OPT_FOREGROUND },
    { "emulator", required_argument, NULL, OPT_EMULATOR },
    { "verbose", no_argument, NULL, OPT_VERBOSE },
    { NULL, 0, NULL, 0 }
};
static const char *option_help[] = {
    "Print help and exit",
    "Number of reader to use (default: 0)",
    "Stay in foreground",
    "NFC emulator backend    (openpicc [default], libnfc)",
    "Use (several times) to be more verbose",
};
static int doinfo = 0;
static int dodaemon = 1;
int verbose = 0;
static unsigned int readernum = 0;
static struct rf_driver *driver = &driver_openpicc;


static LPSTR readers = NULL;
static SCARDCONTEXT hContext = 0;
static SCARDHANDLE hCard = 0;

/* Forward declaration */
static void daemonize();
static void cleanup_exit(int signo);
static void cleanup(void);


void daemonize() {
    pid_t pid, sid;

    /* Fork and continue as child process */
    pid = fork();
    if (pid < 0) {
        ERROR("fork: %s\n", strerror(errno));
        goto err;
    }
    if (pid > 0) /* Exit the parent */
        exit(0);

    umask(0);

    /* Create new session and set the process group ID */
    sid = setsid();
    if (sid < 0) {
        ERROR("setsid: %s\n", strerror(errno));
        goto err;
    }
         
    /* Change the current working directory.  This prevents the current
     * directory from being locked; hence not being able to remove it. */
    if (chdir("/") < 0) {
        ERROR("chdir: %s\n", strerror(errno));
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
    unsigned char *buf = NULL;
    size_t buflen;
    int i, oindex;

    BYTE outputBuffer[MAX_BUFFER_SIZE];
    DWORD outputLength;

    LONG r = SCARD_S_SUCCESS;
    DWORD ctl, protocol;

    struct sigaction new_sig, old_sig;


    while (1) {
        i = getopt_long(argc, argv, "hr:fe:v", options, &oindex);
        if (i == -1)
            break;
        switch (i) {
            case OPT_HELP:
                print_usage(argv[0] , options, option_help);
                exit(0);
                break;
            case OPT_READER:
                if (sscanf(optarg, "%u", &readernum) != 1) {
                    parse_error(argv[0], options, option_help, optarg, oindex);
                    exit(2);
                }
                break;
            case OPT_VERBOSE:
                verbose++;
                dodaemon = 0;
                break;
            case OPT_FOREGROUND:
                dodaemon = 0;
                break;
            case OPT_EMULATOR:
                if (strncmp(optarg, "openpicc", strlen("openpicc")) == 0)
                    driver = &driver_openpicc;
                else
                    if (strncmp(optarg, "libnfc", strlen("libnfc")) == 0)
                        driver = &driver_libnfc;
                    else {
                        parse_error(argv[0], options, option_help, optarg, oindex);
                        exit(2);
                    }
                break;
            case '?':
                /* fall through */
            default:
                exit(1);
                break;
        }
    }


    if (doinfo) {
        fprintf(stderr, "%s  written by Frank Morgner.\n" ,
                PACKAGE_STRING);
        return 0;
    }


    if (dodaemon) {
        verbose = -1;
        daemonize();
    }


    /* Register signal handlers */
    new_sig.sa_handler = cleanup_exit;
    sigemptyset(&new_sig.sa_mask);
    new_sig.sa_flags = SA_RESTART;
    if ((sigaction(SIGINT, &new_sig, &old_sig) < 0)
            || (sigaction(SIGTERM, &new_sig, &old_sig) < 0)) {
        ERROR("sigaction: %s\n", strerror(errno));
        goto err;
    }


    /* Open the device */
    if (!driver->connect(&driver->data))
        goto err;


    /* connect to reader and card */
    r = pcsc_connect(readernum, SCARD_SHARE_EXCLUSIVE, SCARD_PROTOCOL_ANY,
            &hContext, &readers, &hCard, &protocol);
    if (r != SCARD_S_SUCCESS)
        goto err;


    while(1) {
        /* get C-APDU */
        if (!driver->receive_capdu(driver->data, &buf, &buflen))
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

        if (!driver->send_rapdu(driver->data, outputBuffer, outputLength))
            goto err;
    }

err:
    cleanup();

    if (r != SCARD_S_SUCCESS) {
        ERROR(pcsc_stringify_error(r));
        exit(1);
    }

    exit(0);
}
