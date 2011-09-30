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
#include "scutil.h"
#include <stdio.h>
#include <string.h>
#include <libopensc/log.h>

int initialize(int reader_id, const char *cdriver, int verbose,
        sc_context_t **ctx, sc_reader_t **reader)
{
    unsigned int i, reader_count;

    if (!ctx || !reader)
        return SC_ERROR_INVALID_ARGUMENTS;

    int r = sc_establish_context(ctx, "");
    if (r < 0) {
        fprintf(stderr, "Failed to create initial context: %s", sc_strerror(r));
        return r;
    }

    if (cdriver != NULL) {
        r = sc_set_card_driver(*ctx, cdriver);
        if (r < 0) {
            sc_debug(*ctx, SC_LOG_DEBUG_VERBOSE, "Card driver '%s' not found.\n", cdriver);
            return r;
        }
    }

    (*ctx)->debug = verbose;

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
                "\n%s (%u byte%s):\n%s",
                label, len, len==1?"":"s", buf);
    } else {
        fprintf(f, "%s (%u byte%s):\n%s\n",
                label, len, len==1?"":"s", sc_dump_hex(data, len));
    }
}

static int list_drivers(sc_context_t *ctx)
{
	int i;
	
	if (ctx->card_drivers[0] == NULL) {
		printf("No card drivers installed!\n");
		return 0;
	}
	printf("Configured card drivers:\n");
	for (i = 0; ctx->card_drivers[i] != NULL; i++) {
		printf("  %-16s %s\n", ctx->card_drivers[i]->short_name,
		       ctx->card_drivers[i]->name);
	}

	return 0;
}

static int list_readers(sc_context_t *ctx)
{
	unsigned int i, rcount = sc_ctx_get_reader_count(ctx);
	
	if (rcount == 0) {
		printf("No smart card readers found.\n");
		return 0;
	}
	printf("Readers known about:\n");
	printf("Nr.    Driver     Name\n");
	for (i = 0; i < rcount; i++) {
		sc_reader_t *screader = sc_ctx_get_reader(ctx, i);
		printf("%-7d%-11s%s\n", i, screader->driver->short_name,
		       screader->name);
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

    r = list_readers(ctx)|list_drivers(ctx);

    if (ctx)
        sc_release_context(ctx);

    return r;
}
