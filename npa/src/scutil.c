/*
 * Copyright (C) 2010 Frank Morgner
 *
 * This file is part of libnpa.
 *
 * libnpa is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * libnpa is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * libnpa.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "libopensc/internal.h"

#ifndef HAVE__SC_MATCH_ATR
int sc_dlclose(void *handle) {}
const char *sc_dlerror() {}
void *sc_dlsym(void *handle, const char *symbol) {}
size_t strlcpy(char *dst, const char *src, size_t siz) {}
int sc_mutex_create(const sc_context_t *ctx, void **mutex) {}
int sc_mutex_destroy(const sc_context_t *ctx, void *mutex) {}
int _sc_parse_atr(sc_reader_t *reader) {}
void *sc_dlopen(const char *filename) {}
int sc_mutex_lock(const sc_context_t *ctx, void *mutex) {}
int sc_mutex_unlock(const sc_context_t *ctx, void *mutex) {}
#include "libopensc/card.c"
#endif

#if !defined(HAVE_SC_APDU_GET_OCTETS) || !defined(HAVE_SC_APDU_SET_RESP)
#ifdef HAVE__SC_MATCH_ATR
size_t sc_get_max_send_size(const sc_card_t *card) {}
#endif
#include "libopensc/apdu.c"
#endif

#include <libopensc/log.h>
#include <npa/iso-sm.h>
#include <npa/scutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int initialize(int reader_id, int verbose,
        sc_context_t **ctx, sc_reader_t **reader)
{
    unsigned int i, reader_count;

    if (!ctx || !reader)
        return SC_ERROR_INVALID_ARGUMENTS;

    int r = sc_establish_context(ctx, "");
    if (r < 0 || !*ctx) {
        fprintf(stderr, "Failed to create initial context: %s", sc_strerror(r));
        return r;
    }

    (*ctx)->debug = verbose;
	(*ctx)->flags |= SC_CTX_FLAG_ENABLE_DEFAULT_DRIVER;

    reader_count = sc_ctx_get_reader_count(*ctx);

    if (reader_count == 0) {
        sc_debug(*ctx, SC_LOG_DEBUG_NORMAL, "No reader not found.\n");
        return SC_ERROR_NO_READERS_FOUND;
    }

    if (reader_id < 0) {
        /* Automatically try to skip to a reader with a card if reader not specified */
        for (i = 0; i < reader_count; i++) {
            *reader = sc_ctx_get_reader(*ctx, i);
            if (sc_detect_card_presence(*reader) & SC_READER_CARD_PRESENT) {
                reader_id = i;
                sc_debug(*ctx, SC_LOG_DEBUG_NORMAL, "Using the first reader"
                        " with a card: %s", (*reader)->name);
                break;
            }
        }
        if (reader_id >= reader_count) {
            sc_debug(*ctx, SC_LOG_DEBUG_NORMAL, "No card found, using the first reader.");
            reader_id = 0;
        }
    }

    if (reader_id >= reader_count) {
        sc_debug(*ctx, SC_LOG_DEBUG_NORMAL, "Invalid reader number "
                "(%d), only %d available.\n", reader_id, reader_count);
        return SC_ERROR_NO_READERS_FOUND;
    }

    *reader = sc_ctx_get_reader(*ctx, reader_id);

    return SC_SUCCESS;
}

void _bin_log(sc_context_t *ctx, int type, const char *file, int line,
        const char *func, const char *label, const u8 *data, size_t len,
        FILE *f)
{
    if (!f) {
        char buf[1800];
        if (data)
            sc_hex_dump(ctx, SC_LOG_DEBUG_NORMAL, data, len, buf, sizeof buf);
        else
            buf[0] = 0;
        sc_do_log(ctx, type, file, line, func,
                "\n%s (%u byte%s)%s%s",
                label, (unsigned int) len, len==1?"":"s", len==0?"":":\n", buf);
    } else {
        fprintf(f, "%s (%u byte%s)%s%s\n",
                label, (unsigned int) len, len==1?"":"s", len==0?"":":\n", sc_dump_hex(data, len));
    }
}

static int list_readers(sc_context_t *ctx)
{
    char card_atr[0x3e];
    sc_card_t *card;
    sc_reader_t *reader;
	size_t i, rcount = sc_ctx_get_reader_count(ctx);
	
	if (rcount == 0) {
		printf("No smart card readers found.\n");
		return 0;
	}
    printf("%-4s %-7s %s\n", "Nr.", "Driver", "Smart Card Reader");
	for (i = 0; i < rcount; i++) {
		reader = sc_ctx_get_reader(ctx, i);
        memset(card_atr, '\0', sizeof card_atr);
        if (sc_detect_card_presence(reader) & SC_READER_CARD_PRESENT) {
            if (sc_connect_card(reader, &card) == SC_SUCCESS) {
                sc_bin_to_hex(card->atr.value, card->atr.len,
                        card_atr, sizeof card_atr, ':');
            }
            sc_disconnect_card(card);
        } else {
            strncpy(card_atr, "[no card present]", sizeof card_atr);
        }
        printf("%-4d %-7s %s\n", i, reader->driver->short_name, reader->name);
        printf("             ATR: %s\n", card_atr);
	}

	return 0;
}

int print_avail(int verbose)
{
    sc_context_t *ctx = NULL;

    int r;
    r = sc_establish_context(&ctx, "");
    if (r) {
        fprintf(stderr, "Failed to establish context: %s\n", sc_strerror(r));
        return 1;
    }
    ctx->debug = verbose;
	ctx->flags |= SC_CTX_FLAG_ENABLE_DEFAULT_DRIVER;

    r = list_readers(ctx);

    sc_release_context(ctx);

    return r;
}

#define ISO_READ_BINARY  0xB0
#define ISO_P1_FLAG_SFID 0x80
int read_binary_rec(sc_card_t *card, unsigned char sfid,
        u8 **ef, size_t *ef_len)
{
    int r;
    size_t read = MAX_SM_APDU_RESP_SIZE;
    sc_apdu_t apdu;
    u8 *p;

    if (!card || !ef || !ef_len) {
        r = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }
    *ef_len = 0;

    if (read > 0xff+1)
        sc_format_apdu(card, &apdu, SC_APDU_CASE_2_EXT,
                ISO_READ_BINARY, ISO_P1_FLAG_SFID|sfid, 0);
    else
        sc_format_apdu(card, &apdu, SC_APDU_CASE_2_SHORT,
                ISO_READ_BINARY, ISO_P1_FLAG_SFID|sfid, 0);

    p = realloc(*ef, read);
    if (!p) {
        r = SC_ERROR_OUT_OF_MEMORY;
        goto err;
    }
    *ef = p;
    apdu.resp = *ef;
    apdu.resplen = read;
    apdu.le = read;

    r = sc_transmit_apdu(card, &apdu);
    /* emulate the behaviour of sc_read_binary */
    if (r >= 0)
        r = apdu.resplen;

    while(1) {
        if (r >= 0 && r != read) {
            *ef_len += r;
            break;
        }
        if (r < 0) {
            sc_debug(card->ctx, SC_LOG_DEBUG_VERBOSE, "Could not read EF.");
            goto err;
        }
        *ef_len += r;

        p = realloc(*ef, *ef_len + read);
        if (!p) {
            r = SC_ERROR_OUT_OF_MEMORY;
            goto err;
        }
        *ef = p;

        r = sc_read_binary(card, *ef_len,
                *ef + *ef_len, read, 0);
    }

    r = SC_SUCCESS;

err:
    return r;
}

#define ISO_WRITE_BINARY  0xD0
int write_binary_rec(sc_card_t *card, unsigned char sfid,
        u8 *ef, size_t ef_len)
{
    int r;
    size_t write = MAX_SM_APDU_DATA_SIZE, wrote = 0;
    sc_apdu_t apdu;
#ifdef ENABLE_SM
    struct iso_sm_ctx *iso_sm_ctx;
#endif

    if (!card) {
        r = SC_ERROR_INVALID_ARGUMENTS;
        goto err;
    }

#ifdef ENABLE_SM
    iso_sm_ctx = card->sm_ctx.info.cmd_data;
    if (write > SC_MAX_APDU_BUFFER_SIZE-2
            || (card->sm_ctx.sm_mode == SM_MODE_TRANSMIT
                && write > (((SC_MAX_APDU_BUFFER_SIZE-2
                    /* for encrypted APDUs we usually get authenticated status
                     * bytes (4B), a MAC (11B) and a cryptogram with padding
                     * indicator (3B without data).  The cryptogram is always
                     * padded to the block size. */
                    -18) / iso_sm_ctx->block_length)
                    * iso_sm_ctx->block_length - 1)))
        sc_format_apdu(card, &apdu, SC_APDU_CASE_3_EXT,
                ISO_WRITE_BINARY, ISO_P1_FLAG_SFID|sfid, 0);
    else
#endif
        sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT,
                ISO_WRITE_BINARY, ISO_P1_FLAG_SFID|sfid, 0);

    if (write > ef_len) {
        apdu.datalen = ef_len;
        apdu.lc = ef_len;
    } else {
        apdu.datalen = write;
        apdu.lc = write;
    }
    apdu.data = ef;


    r = sc_transmit_apdu(card, &apdu);
    /* emulate the behaviour of sc_write_binary */
    if (r >= 0)
        r = apdu.datalen;

    while (1) {
        if (r < 0 || r > ef_len) {
            sc_debug(card->ctx, SC_LOG_DEBUG_VERBOSE, "Could not write EF.");
            goto err;
        }
        wrote += r;
        apdu.data += r;
        if (wrote >= ef_len)
            break;

        r = sc_write_binary(card, wrote, ef, write, 0);
    }

    r = SC_SUCCESS;

err:
    return r;
}

int fread_to_eof(const char *file, unsigned char **buf, size_t *buflen)
{
    FILE *input = NULL;
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
