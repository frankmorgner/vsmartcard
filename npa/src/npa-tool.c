/*
 * Copyright (C) 2010-2012 Frank Morgner <morgner@informatik.hu-berlin.de>
 *
 * This file is part of npa.
 *
 * npa is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * npa is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * npa.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cmdline.h"
#include "config.h"
#include <eac/pace.h>
#include <libopensc/log.h>
#include <libopensc/opensc.h>
#include <npa/npa.h>
#include <npa/scutil.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#ifndef HAVE_GETLINE
static ssize_t getline(char **lineptr, size_t *n, FILE *stream)
{
    if (!lineptr)
        return -1;

    char *p = realloc(*lineptr, SC_MAX_EXT_APDU_BUFFER_SIZE*3);
    if (!p)
        return -1;
    *lineptr = p;

    if (fgets(p, SC_MAX_EXT_APDU_BUFFER_SIZE*3, stream) == NULL)
        return -1;

    return strlen(p);
}
#endif

static const char *newpin = NULL;
static const char *pin = NULL;
static const char *puk = NULL;
static const char *can = NULL;
static const char *mrz = NULL;
static u8 chat[0xff];
static u8 desc[0xffff];
size_t *certs_lens = NULL;
static const unsigned char **certs = NULL;
static unsigned char *privkey = NULL;
static size_t privkey_len = 0;
static u8 auxiliary_data[0xff];
static size_t auxiliary_data_len = 0;

static sc_context_t *ctx = NULL;
static sc_card_t *card = NULL;
static sc_reader_t *reader;

int fread_to_eof(const unsigned char *file, unsigned char **buf, size_t *buflen)
{
    FILE *input;
    int r = 0;
    unsigned char *p;

    if (!buflen || !buf)
        goto err;

#define MAX_READ_LEN 0xfff
    p = realloc(*buf, MAX_READ_LEN);
    if (!p)
        goto err;
    *buf = p;

    input = fopen(file, "rb");
    if (!input) {
        fprintf(stderr, "Could not open %s.\n", file);
        goto err;
    }

    *buflen = 0;
    while (feof(input) == 0 && *buflen < MAX_READ_LEN) {
        *buflen += fread(*buf+*buflen, 1, MAX_READ_LEN-*buflen, input);
        if (ferror(input)) {
            fprintf(stderr, "Could not read %s.\n", file);
            goto err;
        }
    }

    r = 1;
err:
    if (input)
        fclose(input);

    return r;
}

int npa_translate_apdus(sc_card_t *card, FILE *input)
{
    u8 buf[4 + 3 + 0xffff + 3];
    char *read = NULL;
    size_t readlen = 0, apdulen;
    sc_apdu_t apdu;
    ssize_t linelen;
    int r;

    memset(&apdu, 0, sizeof apdu);

    while (1) {
        if (input == stdin)
            printf("Enter unencrypted C-APDU (empty line to exit)\n");

        linelen = getline(&read, &readlen, input);
        if (linelen <= 1) {
            if (linelen < 0) {
                r = SC_ERROR_INTERNAL;
                sc_debug(card->ctx, SC_LOG_DEBUG_VERBOSE_TOOL,
                        "Could not read line");
            } else {
                r = SC_SUCCESS;
                printf("Thanks for flying with ccid\n");
            }
            break;
        }
        read[linelen - 1] = 0;

        apdulen = sizeof buf;
        if (sc_hex_to_bin(read, buf, &apdulen) < 0) {
            sc_debug(card->ctx, SC_LOG_DEBUG_VERBOSE_TOOL,
                    "Could not format binary string");
            continue;
        }
        if (input != stdin)
            bin_print(stdout, "Unencrypted C-APDU", buf, apdulen);

        r = sc_bytes2apdu(card->ctx, buf, apdulen, &apdu);
        if (r < 0) {
            bin_log(ctx, SC_LOG_DEBUG_NORMAL, "Invalid C-APDU", buf, apdulen);
            continue;
        }

        apdu.resp = buf;
        apdu.resplen = sizeof buf;

        r = sc_transmit_apdu(card, &apdu);
        if (r < 0) {
            sc_debug(card->ctx, SC_LOG_DEBUG_VERBOSE_TOOL,
                    "Could not send C-APDU: %s", sc_strerror(r));
            continue;
        }

        printf("Decrypted R-APDU sw1=%02x sw2=%02x\n", apdu.sw1, apdu.sw2);
        bin_print(stdout, "Decrypted R-APDU response data", apdu.resp, apdu.resplen);
        printf("======================================================================\n");
    }

    if (read)
        free(read);

    return r;
}

int
main (int argc, char **argv)
{
    int r, oindex = 0, tr_version = EAC_TR_VERSION_2_02;
    size_t channeldatalen;
    struct establish_pace_channel_input pace_input;
    struct establish_pace_channel_output pace_output;
    struct timeval tv;
    size_t i;
    FILE *input = NULL;
    CVC_CERT *cvc_cert = NULL;
    unsigned char *certs_chat = NULL;

    struct gengetopt_args_info cmdline;

    memset(&pace_input, 0, sizeof pace_input);
    memset(&pace_output, 0, sizeof pace_output);


    /* Parse command line */
    if (cmdline_parser (argc, argv, &cmdline) != 0)
        exit(1);
    if (cmdline.env_flag) {
        can = getenv("CAN");
        mrz = getenv("MRZ");
        pin = getenv("PIN");
        puk = getenv("PUK");
        newpin = getenv("NEWPIN");
    }
    can = cmdline.can_arg;
    mrz = cmdline.mrz_arg;
    pin = cmdline.pin_arg;
    puk = cmdline.puk_arg;
    newpin = cmdline.new_pin_arg;
    if (cmdline.chat_given) {
        pace_input.chat = chat;
        pace_input.chat_length = sizeof chat;
        if (sc_hex_to_bin(cmdline.chat_arg, (u8 *) pace_input.chat,
                    &pace_input.chat_length) < 0) {
            fprintf(stderr, "Could not parse CHAT.\n");
            exit(2);
        }
    }
    if (cmdline.cert_desc_given) {
        pace_input.certificate_description = desc;
        pace_input.certificate_description_length = sizeof desc;
        if (sc_hex_to_bin(cmdline.cert_desc_arg,
                    (u8 *) pace_input.certificate_description,
                    &pace_input.certificate_description_length) < 0) {
            fprintf(stderr, "Could not parse certificate description.\n");
            exit(2);
        }
    }
    if (cmdline.tr_03110v201_flag)
        tr_version = EAC_TR_VERSION_2_01;
    if (cmdline.disable_checks_flag)
        npa_default_flags |= NPA_FLAG_DISABLE_CHECKS;


    if (cmdline.info_flag)
        return print_avail(cmdline.verbose_given);


    r = initialize(cmdline.reader_arg, NULL, cmdline.verbose_given, &ctx, &reader);
    if (r < 0) {
        fprintf(stderr, "Can't initialize reader\n");
        exit(1);
    }

    if (sc_connect_card(reader, &card) < 0) {
        fprintf(stderr, "Could not connect to card\n");
        sc_release_context(ctx);
        exit(1);
    }

    if (cmdline.break_flag) {
        /* The biggest buffer sprintf could write with "%llu" */
        char secretbuf[strlen("18446744073709551615")+1];
        unsigned long long secret = 0;
        unsigned long long maxsecret = 0;

        if (cmdline.pin_given) {
            pace_input.pin_id = PACE_PIN;
            pace_input.pin_length = 6;
            maxsecret = 999999;
            if (pin) {
                if (sscanf(pin, "%llu", &secret) != 1) {
                    fprintf(stderr, "%s is not an unsigned long long.\n",
                            npa_secret_name(pace_input.pin_id));
                    exit(2);
                }
                if (strlen(can) > pace_input.pin_length) {
                    fprintf(stderr, "%s too big, only %u digits allowed.\n",
                            npa_secret_name(pace_input.pin_id),
                            (unsigned int) pace_input.pin_length);
                    exit(2);
                }
            }
        } else if (cmdline.can_given) {
            pace_input.pin_id = PACE_CAN;
            pace_input.pin_length = 6;
            maxsecret = 999999;
            if (can) {
                if (sscanf(can, "%llu", &secret) != 1) {
                    fprintf(stderr, "%s is not an unsigned long long.\n",
                            npa_secret_name(pace_input.pin_id));
                    exit(2);
                }
                if (strlen(can) > pace_input.pin_length) {
                    fprintf(stderr, "%s too big, only %u digits allowed.\n",
                            npa_secret_name(pace_input.pin_id),
                            (unsigned int) pace_input.pin_length);
                    exit(2);
                }
            }
        } else if (cmdline.puk_given) {
            pace_input.pin_id = PACE_PUK;
            pace_input.pin_length = 10;
            maxsecret = 9999999999LLU;
            if (puk) {
                if (sscanf(puk, "%llu", &secret) != 1) {
                    fprintf(stderr, "%s is not an unsigned long long.\n",
                            npa_secret_name(pace_input.pin_id));
                    exit(2);
                }
                if (strlen(puk) > pace_input.pin_length) {
                    fprintf(stderr, "%s too big, only %u digits allowed.\n",
                            npa_secret_name(pace_input.pin_id),
                            (unsigned int) pace_input.pin_length);
                    exit(2);
                }
            }
        } else {
            fprintf(stderr, "Please specify whether to do PACE with "
                    "PIN, CAN or PUK.\n");
            exit(1);
        }

        pace_input.pin = (unsigned char *) secretbuf;

        do {
            sprintf(secretbuf, "%0*llu", (unsigned int) pace_input.pin_length, secret);

            gettimeofday(&tv, NULL);
            printf("%u,%06u: Trying %s=%s\n",
                    (unsigned int) tv.tv_sec, (unsigned int) tv.tv_usec,
                    npa_secret_name(pace_input.pin_id), pace_input.pin);

            r = perform_pace(card, pace_input, &pace_output, tr_version);

            secret++;
        } while (0 > r && secret <= maxsecret);

        gettimeofday(&tv, NULL);
        if (0 > r) {
            printf("%u,%06u: Tried breaking %s without success.\n",
                    (unsigned int) tv.tv_sec, (unsigned int) tv.tv_usec,
                    npa_secret_name(pace_input.pin_id));
            goto err;
        } else {
            printf("%u,%06u: Tried breaking %s with success (=%s).\n",
                    (unsigned int) tv.tv_sec, (unsigned int) tv.tv_usec,
                    npa_secret_name(pace_input.pin_id),
                    pace_input.pin);
        }
    }

    if (cmdline.resume_flag) {
        pace_input.pin_id = PACE_CAN;
        if (can) {
            pace_input.pin = (unsigned char *) can;
            pace_input.pin_length = strlen(can);
        } else {
            pace_input.pin = NULL;
            pace_input.pin_length = 0;
        }
        r = perform_pace(card, pace_input, &pace_output, tr_version);
        if (r < 0)
            goto err;
        printf("Established PACE channel with CAN.\n");

        pace_input.pin_id = PACE_PIN;
        if (pin) {
            pace_input.pin = (unsigned char *) pin;
            pace_input.pin_length = strlen(pin);
        } else {
            pace_input.pin = NULL;
            pace_input.pin_length = 0;
        }
        r = perform_pace(card, pace_input, &pace_output, tr_version);
        if (r < 0)
            goto err;
        printf("Established PACE channel with PIN. PIN resumed.\n");
    }

    if (cmdline.unblock_flag) {
        pace_input.pin_id = PACE_PUK;
        if (puk) {
            pace_input.pin = (unsigned char *) puk;
            pace_input.pin_length = strlen(puk);
        } else {
            pace_input.pin = NULL;
            pace_input.pin_length = 0;
        }
        r = perform_pace(card, pace_input, &pace_output, tr_version);
        if (r < 0)
            goto err;
        printf("Established PACE channel with PUK.\n");

        r = npa_unblock_pin(card);
        if (r < 0)
            goto err;
        printf("Unblocked PIN.\n");
    }

    if (cmdline.new_pin_given) {
        pace_input.pin_id = PACE_PIN;
        if (pin) {
            pace_input.pin = (unsigned char *) pin;
            pace_input.pin_length = strlen(pin);
        } else {
            pace_input.pin = NULL;
            pace_input.pin_length = 0;
        }
        r = perform_pace(card, pace_input, &pace_output, tr_version);
        if (r < 0)
            goto err;
        printf("Established PACE channel with PIN.\n");

        r = npa_change_pin(card, newpin, newpin ? strlen(newpin) : 0);
        if (r < 0)
            goto err;
        printf("Changed PIN.\n");
    }

    if (cmdline.translate_given
            || (!cmdline.resume_flag && !cmdline.new_pin_given
                && !cmdline.unblock_flag && !cmdline.break_given)) {

        if (cmdline.cv_certificate_given || cmdline.private_key_given
                || cmdline.auxiliary_data_given) {
            if (!cmdline.cv_certificate_given || !cmdline.private_key_given) {
                fprintf(stderr, "Need at least the terminal's certificate "
                        "and its private key to perform terminal authentication.\n");
                exit(1);
            }

            certs = calloc(sizeof *certs, cmdline.cv_certificate_given + 1);
            certs_lens = calloc(sizeof *certs_lens,
                    cmdline.cv_certificate_given + 1);
            if (!certs || !certs_lens) {
                r = SC_ERROR_OUT_OF_MEMORY;
                goto err;
            }
            for (i = 0; i < cmdline.cv_certificate_given; i++) {
                if (!fread_to_eof(cmdline.cv_certificate_arg[i],
                            (unsigned char **) &certs[i], &certs_lens[i]))
                    goto err;
            }

            if (!pace_input.chat_length) {
                const unsigned char *p = certs[cmdline.cv_certificate_given-1];
                if (!CVC_d2i_CVC_CERT(&cvc_cert, &p, certs_lens[cmdline.cv_certificate_given-1])
                        || !cvc_cert || !cvc_cert->body
                        || !cvc_cert->body->certificate_authority_reference
                        || !cvc_cert->body->chat) {
                    r = SC_ERROR_INVALID_DATA;
                    goto err;
                }
                pace_input.chat_length = i2d_CVC_CHAT(cvc_cert->body->chat, &certs_chat);
                if (0 >= (int) pace_input.chat_length) {
                    r = SC_ERROR_INVALID_DATA;
                    goto err;
                }
                pace_input.chat = certs_chat;
            }

            if (!fread_to_eof(cmdline.private_key_arg,
                        &privkey, &privkey_len))
                goto err;

            if (cmdline.auxiliary_data_given) {
                auxiliary_data_len = sizeof auxiliary_data;
                if (sc_hex_to_bin(cmdline.auxiliary_data_arg, auxiliary_data,
                            &auxiliary_data_len) < 0) {
                    fprintf(stderr, "Could not parse auxiliary data.\n");
                    exit(2);
                }
            }
        }

        pace_input.pin = NULL;
        pace_input.pin_length = 0;
        if (cmdline.pin_given) {
            pace_input.pin_id = PACE_PIN;
            if (pin) {
                pace_input.pin = (unsigned char *) pin;
                pace_input.pin_length = strlen(pin);
            }
        } else if (cmdline.can_given) {
            pace_input.pin_id = PACE_CAN;
            if (can) {
                pace_input.pin = (unsigned char *) can;
                pace_input.pin_length = strlen(can);
            }
        } else if (cmdline.mrz_given) {
            pace_input.pin_id = PACE_MRZ;
            if (mrz) {
                pace_input.pin = (unsigned char *) mrz;
                pace_input.pin_length = strlen(mrz);
            }
        } else if (cmdline.puk_given) {
            pace_input.pin_id = PACE_PUK;
            if (puk) {
                pace_input.pin = (unsigned char *) puk;
                pace_input.pin_length = strlen(puk);
            }
        } else {
            fprintf(stderr, "Please specify whether to do PACE with "
                    "PIN, CAN, MRZ or PUK.\n");
            exit(1);
        }

        r = perform_pace(card, pace_input, &pace_output, tr_version);
        if (r < 0)
            goto err;
        printf("Established PACE channel with %s.\n",
                npa_secret_name(pace_input.pin_id));

        if (cmdline.cv_certificate_given || cmdline.private_key_given) {
            r = perform_terminal_authentication(card, certs, certs_lens,
                    privkey, privkey_len, auxiliary_data, auxiliary_data_len);
            if (r < 0)
                goto err;
            printf("Performed Terminal Authentication.\n");

            r = perform_chip_authentication(card);
            if (r < 0)
                goto err;
            printf("Performed Chip Authentication.\n");
        }

        if (cmdline.translate_given) {
            if (strncmp(cmdline.translate_arg, "stdin", strlen("stdin")) == 0)
                input = stdin;
            else {
                input = fopen(cmdline.translate_arg, "r");
                if (!input) {
                    perror("Opening file with APDUs");
                    goto err;
                }
            }

            r = npa_translate_apdus(card, input);
            if (r < 0)
                goto err;
            fclose(input);
            input = NULL;
        }
    }

err:
    cmdline_parser_free(&cmdline);
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
    if (input)
        fclose(input);
    if (certs) {
        i = 0;
        while (certs[i]) {
            free((unsigned char *) certs[i]);
            i++;
        }
        free(certs);
    }
    free(certs_lens);
    free(certs_chat);
    if (cvc_cert)
        CVC_CERT_free(cvc_cert);

    sm_stop(card);
    sc_reset(card, 1);
    sc_disconnect_card(card);
    sc_release_context(ctx);

    if (r < 0)
        fprintf(stderr, "Error: %s\n", sc_strerror(r));

    return -r;
}
