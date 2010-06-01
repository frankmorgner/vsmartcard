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
#include <openssl/pace.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int verbose    = 0;
static int doinfo     = 0;
static u8  pin_id = 0;
static u8  dochangepin = 0;
static u8  doresumepin = 0;
static u8  dotranslate = 0;
static const char *newpin = NULL;
static int usb_reader_num = -1;
static const char *pin = NULL;
static const char *puk = NULL;
static const char *can = NULL;
static const char *mrz = NULL;
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
#define OPT_CHANGE_PIN  'N'
#define OPT_RESUME_PIN  'R'
#define OPT_TRANSLATE   't'
#define OPT_VERBOSE     'v'
#define OPT_INFO        'o'
#define OPT_CARD        'c'

static const struct option options[] = {
    { "help", no_argument, NULL, OPT_HELP },
    { "reader",	required_argument, NULL, OPT_READER },
    { "card-driver", required_argument, NULL, OPT_CARD },
    { "pin", required_argument, NULL, OPT_PIN },
    { "puk", required_argument, NULL, OPT_PUK },
    { "can", required_argument, NULL, OPT_CAN },
    { "mrz", required_argument, NULL, OPT_MRZ },
    { "new-pin", optional_argument, NULL, OPT_CHANGE_PIN },
    { "resume-pin", no_argument, NULL, OPT_RESUME_PIN },
    { "translate", no_argument, NULL, OPT_TRANSLATE },
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
    "Run PACE with MRZ (insert MRZ without newlines)",
    "Install a new PIN",
    "Resume PIN (uses CAN to activate last retry)",
    "Send plaintext APDUs through the PACE SM channel",
    "Use (several times) to be more verbose",
    "Print version, available readers and drivers.",
};

int pace_get_channel(struct sm_ctx *oldpacectx, sc_card_t *card,
        enum s_type pin_id, const char *pin, size_t pinlen,
        __u8 **out, size_t *outlen, struct sm_ctx *sctx)
{
    u8 buf[0xff + 5];

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

    return EstablishPACEChannel(oldpacectx, card, buf, out, outlen, sctx);
}

int pace_translate_apdus(struct sm_ctx *sctx, sc_card_t *card)
{
    u8 buf[0xff + 5];
    char *read = NULL;
    size_t readlen = 0, apdulen;
    sc_apdu_t apdu;
    ssize_t linelen;
    int r;

    memset(&apdu, 0, sizeof(apdu));

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
            continue;
        }

        r = build_apdu(card->ctx, buf, apdulen, &apdu);
        if (r < 0) {
            sc_error(card->ctx, "Could not format APDU");
            continue;
        }

        apdu.resp = buf;
        apdu.resplen = sizeof(buf);

        r = pace_transmit_apdu(sctx, card, &apdu);
        if (r < 0) {
            sc_error(card->ctx, "Could not send APDU: %s", sc_strerror(r));
            continue;
        }

        printf("Decrypted APDU sw1=%02x sw2=%02x\n", apdu.sw1, apdu.sw2);
        bin_print(stdout, "Decrypted APDU response data", apdu.resp, apdu.resplen);
    }

    if (read)
        free(read);

    return r;
}

int
main (int argc, char **argv)
{
    int i, oindex = 0;
    __u8 *channeldata = NULL;
    size_t channeldatalen;
    struct sm_ctx sctx, tmpctx;

    memset(&sctx, 0, sizeof(sctx));
    memset(&tmpctx, 0, sizeof(tmpctx));

    while (1) {
        i = getopt_long(argc, argv, "hr:i:u:a:z:N::Rtvoc:", options, &oindex);
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
                puk = optarg;
                break;
            case OPT_PIN:
                pin = optarg;
                break;
            case OPT_CAN:
                can = optarg;
                break;
            case OPT_MRZ:
                mrz = optarg;
                break;
            case OPT_CHANGE_PIN:
                dochangepin = 1;
                newpin = optarg;
                break;
            case OPT_RESUME_PIN:
                doresumepin = 1;
                break;
            case OPT_TRANSLATE:
                dotranslate = 1;
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
        fprintf(stderr, "Can't initialize reader\n");
        exit(1);
    }

    for (i = 0; i < SC_MAX_SLOTS; i++) {
        if (sc_detect_card_presence(reader, 0) & SC_SLOT_CARD_PRESENT) {
            sc_connect_card(reader, i, &card);
            break;
        }
    }
    if (i == SC_MAX_SLOTS) {
        fprintf(stderr, "No card found\n");
        sc_release_context(ctx);
        exit(1);
    }

    if (doresumepin) {
        i = pace_get_channel(NULL, card,
                PACE_CAN, can, can ? strlen(can) : 0,
                &channeldata, &channeldatalen, &tmpctx);
        if (i < 0)
            goto err;
        printf("Established PACE channel with CAN.\n");

        /*i = pace_reset_retry_counter(&tmpctx, card, PACE_PIN, 0, NULL, 0);*/
        /*if (i < 0)*/
            /*goto err;*/
        /*printf("Resumed PIN.\n");*/

        i = pace_get_channel(&tmpctx, card,
                PACE_PIN, pin, pin ? strlen(pin) : 0,
                &channeldata, &channeldatalen, &sctx);
        if (i < 0)
            goto err;
        printf("Established PACE channel with PIN.\n");
    }

    if (dochangepin) {
        i = pace_get_channel(NULL, card,
                PACE_PIN, pin, pin ? strlen(pin) : 0,
                &channeldata, &channeldatalen, &sctx);
        if (i < 0)
            goto err;
        printf("Established PACE channel with PIN.\n");

        i = pace_change_pin(&sctx, card, newpin, newpin ? strlen(newpin) : 0);
        if (i < 0)
            goto err;
        printf("Changed PIN.\n");
    }

    if (dotranslate || (!doresumepin && !dochangepin)) {
        enum s_type id;
        const char *s;
        if (pin) {
            id = PACE_PIN;
            s = pin;
        } else if (can) {
            id = PACE_CAN;
            s = can;
        } else if (mrz) {
            id = PACE_MRZ;
            s = mrz;
        } else if (puk) {
            id = PACE_PUK;
            s = puk;
        } else {
            fprintf(stderr, "Please provide PIN, CAN, MRZ or PUK.");
            exit(1);
        }

        i = pace_get_channel(NULL, card, id, s, s ? strlen(s) : 0,
                &channeldata, &channeldatalen, &sctx);
        if (i < 0)
            goto err;
        printf("Established PACE channel with %s.\n", pace_secret_name(id));

        if (dotranslate) {
            i = pace_translate_apdus(&sctx, card);
            if (i < 0)
                goto err;
        }
    }

err:
    if (channeldata)
        free(channeldata);
    sc_disconnect_card(card, 0);
    sc_release_context(ctx);

    return -i;
}
