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
#include <asm/byteorder.h>
#include <opensc/log.h>
#include <openssl/pace.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int verbose    = 0;
static int doinfo     = 0;
static u8  dobreak = 0;
static u8  dochangepin = 0;
static u8  doresumepin = 0;
static u8  dounblock = 0;
static u8  dotranslate = 0;
static const char *newpin = NULL;
static int usb_reader_num = -1;
static const char *pin = NULL;
static u8 usepin = 0;
static const char *puk = NULL;
static u8 usepuk = 0;
static const char *can = NULL;
static u8 usecan = 0;
static const char *mrz = NULL;
static u8 usemrz = 0;
static u8 chat[0xff];
static u8 desc[0xffff];
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
#define OPT_BREAK       'b'
#define OPT_CHAT        'C'
#define OPT_CERTDESC    'D'
#define OPT_CHANGE_PIN  'N'
#define OPT_RESUME_PIN  'R'
#define OPT_UNBLOCK_PIN 'U'
#define OPT_TRANSLATE   't'
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
    { "break", no_argument, NULL, OPT_BREAK },
    { "chat", required_argument, NULL, OPT_CHAT },
    { "cert-desc", required_argument, NULL, OPT_CERTDESC },
    { "new-pin", optional_argument, NULL, OPT_CHANGE_PIN },
    { "resume-pin", no_argument, NULL, OPT_RESUME_PIN },
    { "unblock-pin", no_argument, NULL, OPT_UNBLOCK_PIN },
    { "translate", no_argument, NULL, OPT_TRANSLATE },
    { "verbose", no_argument, NULL, OPT_VERBOSE },
    { "info", no_argument, NULL, OPT_INFO },
    { NULL, 0, NULL, 0 }
};
static const char *option_help[] = {
    "Print help and exit",
    "Number of reader to use  (default: auto-detect)",
    "Which card driver to use (default: auto-detect)",
    "Run PACE with (transport) PIN",
    "Run PACE with PUK",
    "Run PACE with CAN",
    "Run PACE with MRZ (insert MRZ without newlines)",
    "Brute force the secret (only for PIN, CAN, PUK)",
    "Card holder authorization template to use (hex string)",
    "Certificate description to use (hex string)",
    "Install a new PIN",
    "Resume PIN (uses CAN to activate last retry)",
    "Unblock PIN (uses PUK to activate three more retries)",
    "Send plaintext APDUs through the PACE SM channel",
    "Use (several times) to be more verbose",
    "Print version, available readers and drivers.",
};

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

        apdulen = sizeof buf;
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

        r = sm_transmit_apdu(sctx, card, &apdu);
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
    size_t channeldatalen;
    struct sm_ctx sctx, tmpctx;
    struct establish_pace_channel_input pace_input;
    struct establish_pace_channel_output pace_output;
    time_t t_start, t_end;
    size_t outlen;

    memset(&sctx, 0, sizeof(sctx));
    memset(&tmpctx, 0, sizeof(tmpctx));
    memset(&pace_input, 0, sizeof(pace_input));
    memset(&pace_output, 0, sizeof(pace_output));

    while (1) {
        i = getopt_long(argc, argv, "hr:i::u::a::z::bC:D:N::RUtvoc:", options, &oindex);
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
                usepuk = 1;
                puk = optarg;
                if (!puk)
                    pin = getenv("PUK");
                break;
            case OPT_PIN:
                usepin = 1;
                pin = optarg;
                if (!pin)
                    pin = getenv("PIN");
                break;
            case OPT_CAN:
                usecan = 1;
                can = optarg;
                if (!can)
                    can = getenv("CAN");
                break;
            case OPT_MRZ:
                usemrz = 1;
                mrz = optarg;
                if (!mrz)
                    can = getenv("MRZ");
                break;
            case OPT_BREAK:
                dobreak = 1;
                break;
            case OPT_CHAT:
                pace_input.chat = chat;
                pace_input.chat_length = sizeof chat;
                if (sc_hex_to_bin(optarg, (u8 *) pace_input.chat,
                            &pace_input.chat_length) < 0) {
                    parse_error(argv[0], options, option_help, optarg, oindex);
                    exit(2);
                }
                break;
            case OPT_CERTDESC:
                pace_input.certificate_description = desc;
                pace_input.certificate_description_length = sizeof desc;
                if (sc_hex_to_bin(optarg, (u8 *) pace_input.certificate_description,
                            &pace_input.certificate_description_length) < 0) {
                    parse_error(argv[0], options, option_help, optarg, oindex);
                    exit(2);
                }
                break;
            case OPT_CHANGE_PIN:
                dochangepin = 1;
                newpin = optarg;
                if (!newpin)
                    pin = getenv("NEWPIN");
                break;
            case OPT_RESUME_PIN:
                doresumepin = 1;
                break;
            case OPT_UNBLOCK_PIN:
                dounblock = 1;
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

    if (dobreak) {
        char can_ch[7];
        unsigned int can_nb = 0;

        if (usepin) {
            pace_input.pin_id = PACE_PIN;
            if (pin) {
                if (sscanf(pin, "%u", &can_nb) != 1) {
                    fprintf(stderr, "PIN is not an unsigned integer.");
                    exit(2);
                }
            }
        } else if (usecan) {
            pace_input.pin_id = PACE_CAN;
            if (can) {
                if (sscanf(can, "%u", &can_nb) != 1) {
                    fprintf(stderr, "CAN is not an unsigned integer.");
                    exit(2);
                }
            }
        } else if (usepuk) {
            pace_input.pin_id = PACE_PUK;
            if (puk) {
                if (sscanf(puk, "%u", &can_nb) != 1) {
                    fprintf(stderr, "PUK is not an unsigned integer.");
                    exit(2);
                }
            }
        } else {
            fprintf(stderr, "Please specify whether to do PACE with "
                    "PIN, CAN or PUK.");
            exit(1);
        }

        sprintf(can_ch, "%06u", can_nb);
        pace_input.pin = can_ch;
        pace_input.pin_length = strlen(can_ch);

        t_start = time(NULL);
        while (0 > EstablishPACEChannel(NULL, card, pace_input, &pace_output,
                    &sctx)) {
            printf("Failed trying %s=%s\n",
                    pace_secret_name(pace_input.pin_id), pace_input.pin);
            can_nb++;
            sprintf(can_ch, "%06u", can_nb);
            if (can_nb > 999999)
                break;
        }
        t_end = time(NULL);
        if (can_nb > 999999) {
            printf("Tried breaking %s for %.0fs without success.\n",
                    pace_secret_name(pace_input.pin_id), difftime(t_end, t_start));
            goto err;
        } else {
            printf("Tried breaking %s for %.0fs with success.\n",
                    pace_secret_name(pace_input.pin_id), difftime(t_end, t_start));
            printf("%s=%s\n", pace_secret_name(pace_input.pin_id), pace_input.pin);
        }
    }

    if (doresumepin) {
        pace_input.pin_id = PACE_CAN;
        if (can) {
            pace_input.pin = can;
            pace_input.pin_length = strlen(can);
        } else {
            pace_input.pin = NULL;
            pace_input.pin_length = 0;
        }
        t_start = time(NULL);
        i = EstablishPACEChannel(NULL, card, pace_input, &pace_output,
            &tmpctx);
        t_end = time(NULL);
        if (i < 0)
            goto err;
        printf("Established PACE channel with CAN in %.0fs.\n",
                difftime(t_end, t_start));

        pace_input.pin_id = PACE_PIN;
        if (pin) {
            pace_input.pin = pin;
            pace_input.pin_length = strlen(pin);
        } else {
            pace_input.pin = NULL;
            pace_input.pin_length = 0;
        }
        t_start = time(NULL);
        i = EstablishPACEChannel(&tmpctx, card, pace_input, &pace_output,
            &sctx);
        t_end = time(NULL);
        if (i < 0)
            goto err;
        printf("Established PACE channel with PIN in %.0fs. PIN resumed.\n",
                difftime(t_end, t_start));
    }

    if (dounblock) {
        pace_input.pin_id = PACE_PUK;
        if (puk) {
            pace_input.pin = puk;
            pace_input.pin_length = strlen(puk);
        } else {
            pace_input.pin = NULL;
            pace_input.pin_length = 0;
        }
        t_start = time(NULL);
        i = EstablishPACEChannel(NULL, card, pace_input, &pace_output,
            &sctx);
        t_end = time(NULL);
        if (i < 0)
            goto err;
        printf("Established PACE channel with PUK in %.0fs.\n",
                difftime(t_end, t_start));

        i = pace_unblock_pin(&sctx, card);
        if (i < 0)
            goto err;
        printf("Unblocked PIN.\n");
    }

    if (dochangepin) {
        pace_input.pin_id = PACE_PIN;
        if (pin) {
            pace_input.pin = pin;
            pace_input.pin_length = strlen(pin);
        } else {
            pace_input.pin = NULL;
            pace_input.pin_length = 0;
        }
        t_start = time(NULL);
        i = EstablishPACEChannel(NULL, card, pace_input, &pace_output,
            &sctx);
        t_end = time(NULL);
        if (i < 0)
            goto err;
        printf("Established PACE channel with PIN in %.0fs.\n",
                difftime(t_end, t_start));

        i = pace_change_pin(&sctx, card, newpin, newpin ? strlen(newpin) : 0);
        if (i < 0)
            goto err;
        printf("Changed PIN.\n");
    }

    if (dotranslate || (!doresumepin && !dochangepin && !dounblock && !dobreak)) {
        pace_input.pin = NULL;
        pace_input.pin_length = 0;
        if (usepin) {
            pace_input.pin_id = PACE_PIN;
            if (pin) {
                pace_input.pin = pin;
                pace_input.pin_length = strlen(pin);
            }
        } else if (usecan) {
            pace_input.pin_id = PACE_CAN;
            if (can) {
                pace_input.pin = can;
                pace_input.pin_length = strlen(can);
            }
        } else if (usemrz) {
            pace_input.pin_id = PACE_MRZ;
            if (mrz) {
                pace_input.pin = mrz;
                pace_input.pin_length = strlen(mrz);
            }
        } else if (usepuk) {
            pace_input.pin_id = PACE_PUK;
            if (puk) {
                pace_input.pin = puk;
                pace_input.pin_length = strlen(puk);
            }
        } else {
            fprintf(stderr, "Please specify whether to do PACE with "
                    "PIN, CAN, MRZ or PUK.");
            exit(1);
        }

        t_start = time(NULL);
        i = EstablishPACEChannel(NULL, card, pace_input, &pace_output,
            &sctx);
        t_end = time(NULL);
        if (i < 0)
            goto err;
        printf("Established PACE channel with %s in %.0fs.\n",
                pace_secret_name(pace_input.pin_id), difftime(t_end, t_start));

        if (dotranslate) {
            i = pace_translate_apdus(&sctx, card);
            if (i < 0)
                goto err;
        }
    }

err:
    pace_sm_ctx_clear_free(sctx.cipher_ctx);
    pace_sm_ctx_clear_free(tmpctx.cipher_ctx);
    if (pace_output.ef_cardaccess)
        free(pace_output.ef_cardaccess);
    if (pace_output.recent_car)
        free(pace_output.recent_car);
    if (pace_output.previous_car)
        free(pace_output.previous_car);
    if (pace_output.id_icc)
        free(pace_output.id_icc);
    if (pace_output.id_pcd)
        free(pace_output.id_pcd);

    sc_disconnect_card(card, 0);
    sc_release_context(ctx);

    return -i;
}
