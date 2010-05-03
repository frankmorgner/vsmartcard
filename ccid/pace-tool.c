/*
 * Copyright (C) 2009 Frank Morgner
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
#include "ccid.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

static int verbose    = 0;
static int doinfo     = 0;
static u8  dopacetest = 0;
static u8  dochangepin = 0;
static const char *newpin = NULL;
static int usb_reader_num = -1;
static const char *pin = NULL;
static const char *cdriver = NULL;

#define OPT_HELP        'h'
#define OPT_READER      'r'
#define OPT_PIN         'i'
#define OPT_PUK         'u'
#define OPT_CAN         'a'
#define OPT_MRZ         'z'
#define OPT_CHANGE_PIN  'I'
#define OPT_CHANGE_CAN  'A'
#define OPT_VERBOSE     'v'
#define OPT_INFO        'o'
#define OPT_CARD        'c'

static const struct option options[] = {
    { "help", no_argument, NULL, OPT_HELP },
    { "reader",	required_argument, NULL, OPT_READER },
    { "card-driver", required_argument, NULL, OPT_CARD },
    { "test-pin", optional_argument, NULL, OPT_PIN },
    { "test-puk", optional_argument, NULL, OPT_PUK },
    { "test-can", optional_argument, NULL, OPT_CAN },
    { "test-mrz", optional_argument, NULL, OPT_MRZ },
    { "new-pin", optional_argument, NULL, OPT_CHANGE_PIN },
    { "new-can", optional_argument, NULL, OPT_CHANGE_CAN },
    { "verbose", no_argument, NULL, OPT_VERBOSE },
    { "info", no_argument, NULL, OPT_INFO },
    { NULL, 0, NULL, 0 }
};
static const char *option_help[] = {
    "Print help and exit",
    "Number of reader to use  (default: auto-detect)",
    "Which card driver to use (default: auto-detect)",
    "Run PACE with PIN",
    "Run PACE with PUK",
    "Run PACE with CAN",
    "Run PACE with MRZ",
    "Change PIN when PACE is finished",
    "Change CAN when PACE is finished",
    "Use (several times) to be more verbose",
    "Print version, available readers and drivers.",
};

int
main (int argc, char **argv)
{
    int i, oindex = 0;

    while (1) {
        i = getopt_long(argc, argv, "hr:i::u::a::z::I::A::voc:", options, &oindex);
        if (i == -1)
            break;
        switch (i) {
            case OPT_HELP:
                print_usage(argv[0] , options, option_help);
                exit(0);
                break;
            case OPT_READER:
                if (sscanf(optarg, "%d", &usb_reader_num) != 1) {
                    parse_error(argv[0], options, option_help, optarg, oindex);
                    exit(2);
                }
                break;
            case OPT_CARD:
                cdriver = optarg;
                break;
            case OPT_VERBOSE:
                verbose++;
                break;
            case OPT_INFO:
                doinfo++;
                break;
            case OPT_PUK:
                /* PACE_PIN from openssl/pace.h */
                dopacetest = 4;
                pin = optarg;
                break;
            case OPT_PIN:
                /* PACE_PIN from openssl/pace.h */
                dopacetest = 3;
                pin = optarg;
                break;
            case OPT_CAN:
                /* PACE_PIN from openssl/pace.h */
                dopacetest = 2;
                pin = optarg;
                break;
            case OPT_MRZ:
                /* PACE_MRZ from openssl/pace.h */
                dopacetest = 1;
                pin = optarg;
                break;
            case OPT_CHANGE_CAN:
                dochangepin = 2;
                newpin = optarg;
                break;
            case OPT_CHANGE_PIN:
                dochangepin = 3;
                newpin = optarg;
                break;
            case '?':
                /* fall through */
            default:
                exit(1);
                break;
        }
    }

    if (optind < argc) {
        fprintf (stderr, "Unknown argument%s:", optind+1 == argc ? "" : "s");
        while (optind < argc) {
            fprintf(stderr, " \"%s\"", argv[optind++]);
            fprintf(stderr, "%c", optind == argc ? '\n' : ',');
        }
        exit(1);
    }


    if (doinfo) {
        fprintf(stderr, "%s 0.9  written by Frank Morgner.\n\n" ,
                argv[0]);
        return ccid_print_avail(verbose);
    }

    if (ccid_initialize(usb_reader_num, cdriver, verbose) < 0) {
        perror("Can't initialize ccid");
        return 1;
    }

    i = ccid_testpace(dopacetest, pin, !pin ? 0 : strlen(pin),
            dochangepin, newpin, !newpin ? 0 : strlen(newpin));
    ccid_shutdown();
    return i;
}
