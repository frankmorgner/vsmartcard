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
#include "binutil.h"
#include "pace.h"
#include "scutil.h"
#include <opensc/log.h>
#include <opensc/ui.h>
#include <openssl/pace.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int verbose    = 0;
static int doinfo     = 0;
static u8  pin_id = 0;
static u8  dochangepin = 0;
static const char *newpin = NULL;
static int usb_reader_num = -1;
static const char *pin = NULL;
static const char *cdriver = NULL;

static sc_context_t *ctx = NULL;
static sc_card_t *card = NULL;
static sc_reader_t *reader;

#define OPT_HELP        'h'
#define OPT_READER      'r'
#define OPT_PIN         'i'
#define OPT_PUK         'u'
#define OPT_CAN         'a'
#define OPT_MRZ         'z'
#define OPT_CHANGE_PIN  'I'
#define OPT_VERBOSE     'v'
#define OPT_INFO        'o'
#define OPT_CARD        'c'

static const struct option options[] = {
    { "help", no_argument, NULL, OPT_HELP },
    { "reader",	required_argument, NULL, OPT_READER },
    { "card-driver", required_argument, NULL, OPT_CARD },
    { "pin", optional_argument, NULL, OPT_PIN },
    { "puk", optional_argument, NULL, OPT_PUK },
    { "can", optional_argument, NULL, OPT_CAN },
    { "mrz", optional_argument, NULL, OPT_MRZ },
    { "new-pin", optional_argument, NULL, OPT_CHANGE_PIN },
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
    "Install a new PIN",
    "Use (several times) to be more verbose",
    "Print version, available readers and drivers.",
};

int pace_change_p(struct sm_ctx *ctx, sc_card_t *card, enum s_type pin_id,
        const char *newp, size_t newplen)
{
    sc_ui_hints_t hints;
    char *p = NULL;
    int r;

    if (!newplen || !newp) {
        memset(&hints, 0, sizeof(hints));
        hints.dialog_name = "ccid.PACE";
        hints.card = card;
        hints.prompt = NULL;
        hints.obj_label = pace_secret_name(pin_id);
        hints.usage = SC_UI_USAGE_NEW_PIN;
        r = sc_ui_get_pin(&hints, &p);
        if (r < 0) {
            sc_error(card->ctx, "Could not read new %s (%s).\n",
                    hints.obj_label, sc_strerror(r));
            SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_ERROR, r);
        }
        newplen = strlen(p);
        newp = p;
    }

    r = pace_reset_retry_counter(ctx, card, pin_id, newp, newplen);

    if (p) {
        OPENSSL_cleanse(p, newplen);
        free(p);
    }

    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_DEBUG, r);
}

int pace_test(sc_card_t *card,
        enum s_type pin_id, const char *pin, size_t pinlen,
        enum s_type new_pin_id, const char *new_pin, size_t new_pinlen)
{
    u8 buf[0xff + 5];
    char *read = NULL;
    __u8 *out = NULL;
    size_t outlen, readlen = 0, apdulen;
    ssize_t linelen;
    struct sm_ctx sctx;
    sc_apdu_t apdu;
    int r;

    memset(&sctx, 0, sizeof(sctx));
    memset(&apdu, 0, sizeof(apdu));

    switch (pin_id) {
        case PACE_MRZ:
        case PACE_CAN:
        case PACE_PIN:
        case PACE_PUK:
            break;
        default:
            sc_error(card->ctx, "Type of secret not supported");
            return SC_ERROR_INVALID_ARGUMENTS;
    }

    if (pinlen > sizeof(buf) - 5) {
        sc_error(card->ctx, "%s too long (maximal %u bytes supported)",
                pace_secret_name(pin_id),
                sizeof(buf) - 5);
    }
    buf[0] = pin_id;
    buf[1] = 0;             // length_chat
    buf[2] = pinlen;        // length_pin
    memcpy(&buf[3], pin, pinlen);
    buf[3 + pinlen] = 0;    // length_cert_desc
    buf[4 + pinlen]= 0;     // length_cert_desc

    SC_TEST_RET(card->ctx,
            EstablishPACEChannel(card, buf, &out, &outlen, &sctx),
            "Could not establish PACE channel.");

    printf("Established PACE channel.\n");

    if (new_pin_id) {
        SC_TEST_RET(card->ctx,
                pace_change_p(&sctx, card, new_pin_id, new_pin, new_pinlen),
                "Could not change PACE secret.");
    } else {
        while (1) {
            printf("Enter unencrypted APDU (empty line to exit)\n");

            linelen = getline(&read, &readlen, stdin);
            if (linelen <= 1) {
                if (linelen < 0) {
                    r = SC_ERROR_INTERNAL;
                    sc_error(card->ctx, "Could not read line");
                } else {
                    r = SC_SUCCESS;
                    printf("Thanks for flying with ccid\n");
                }
                break;
            }

            read[linelen - 1] = 0;
            if (sc_hex_to_bin(read, buf, &apdulen) < 0) {
                sc_error(card->ctx, "Could not format binary string");
            }

            r = build_apdu(card->ctx, buf, apdulen, &apdu);
            if (r < 0) {
                sc_error(card->ctx, "Could not format APDU");
                continue;
            }

            apdu.resp = buf;
            apdu.resplen = sizeof(buf);

            r = pace_transmit_apdu(&sctx, card, &apdu);
            if (r < 0) {
                sc_error(card->ctx, "Could not send APDU: %s", sc_strerror(r));
                continue;
            }

            printf("Decrypted APDU sw1=%02x sw2=%02x\n", apdu.sw1, apdu.sw2);
            bin_print(stdout, "Decrypted APDU response data", apdu.resp, apdu.resplen);
        }

        if (read)
            free(read);
    }

    SC_FUNC_RETURN(card->ctx, SC_LOG_TYPE_ERROR, r);
}

int
main (int argc, char **argv)
{
    int i, oindex = 0;

    while (1) {
        i = getopt_long(argc, argv, "hr:i::u::a::z::I::voc:", options, &oindex);
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
                pin_id = PACE_PUK;
                pin = optarg;
                break;
            case OPT_PIN:
                pin_id = PACE_PIN;
                pin = optarg;
                break;
            case OPT_CAN:
                pin_id = PACE_CAN;
                pin = optarg;
                break;
            case OPT_MRZ:
                pin_id = PACE_MRZ;
                pin = optarg;
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
        return print_avail(verbose);
    }

    i = initialize(usb_reader_num, cdriver, verbose, &ctx, &reader);
    if (i < 0) {
        perror("Can't initialize reader");
        return 1;
    }

    for (i = 0; i < SC_MAX_SLOTS; i++) {
        if (sc_detect_card_presence(reader, 0) & SC_SLOT_CARD_PRESENT) {
            sc_connect_card(reader, i, &card);
            break;
        }
    }
    if (i == SC_MAX_SLOTS) {
        perror("No card found");
        sc_disconnect_card(card, 0);
        sc_release_context(ctx);
        return 1;
    }

    i = pace_test(card, pin_id, pin, !pin ? 0 : strlen(pin),
            dochangepin, newpin, !newpin ? 0 : strlen(newpin));

    sc_disconnect_card(card, 0);
    sc_release_context(ctx);

    return i;
}
