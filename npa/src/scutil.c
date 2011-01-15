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

    int r = sc_context_create(ctx, NULL);
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


int build_apdu(sc_context_t *ctx, const u8 *buf, size_t len, sc_apdu_t *apdu)
{
    const u8 *p;
    size_t len0;

    if (!buf || !apdu)
        return SC_ERROR_INVALID_ARGUMENTS;

    len0 = len;
    if (len < 4) {
        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "APDU too short (must be at least 4 bytes)");
        return SC_ERROR_INVALID_DATA;
    }

    memset(apdu, 0, sizeof *apdu);
    p = buf;
    apdu->cla = *p++;
    apdu->ins = *p++;
    apdu->p1 = *p++;
    apdu->p2 = *p++;
    len -= 4;
    if (!len) {
        apdu->cse = SC_APDU_CASE_1;
    } else {
        if (*p == 0 && len >= 3) {
            /* ...must be an extended APDU */
            p++;
            if (len == 3) {
                apdu->le = (*p++)<<8;
                apdu->le += *p++;
                if (apdu->le == 0)
                    apdu->le = 0xffff+1;
                len -= 3;
                apdu->cse = SC_APDU_CASE_2_EXT;
            } else {
                /* len > 3 */
                apdu->lc = (*p++)<<8;
                apdu->lc += *p++;
                len -= 3;
                if (len < apdu->lc) {
                    sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "APDU too short (need %lu more bytes)\n",
                            (unsigned long) apdu->lc - len);
                    return SC_ERROR_INVALID_DATA;
                }
                apdu->data = p;
                apdu->datalen = apdu->lc;
                len -= apdu->lc;
                p += apdu->lc;
                if (!len) {
                    apdu->cse = SC_APDU_CASE_3_EXT;
                } else {
                    /* at this point the apdu has a Lc, so Le is on 2 bytes */
                    if (len < 2) {
                        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "APDU too short (need 2 more bytes)\n");
                        return SC_ERROR_INVALID_DATA;
                    }
                    apdu->le = (*p++)<<8;
                    apdu->le += *p++;
                    if (apdu->le == 0)
                        apdu->le = 0xffff+1;
                    len -= 2;
                    apdu->cse = SC_APDU_CASE_4_EXT;
                }
            }
        } else {
            /* ...must be a short APDU */
            if (len == 1) {
                apdu->le = *p++;
                if (apdu->le == 0)
                    apdu->le = 0xff+1;
                len--;
                apdu->cse = SC_APDU_CASE_2_SHORT;
            } else {
                apdu->lc = *p++;
                len--;
                if (len < apdu->lc) {
                    sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "APDU too short (need %lu more bytes)\n",
                            (unsigned long) apdu->lc - len);
                    return SC_ERROR_INVALID_DATA;
                }
                apdu->data = p;
                apdu->datalen = apdu->lc;
                len -= apdu->lc;
                p += apdu->lc;
                if (!len) {
                    apdu->cse = SC_APDU_CASE_3_SHORT;
                } else {
                    apdu->le = *p++;
                    if (apdu->le == 0)
                        apdu->le = 0xff+1;
                    len--;
                    apdu->cse = SC_APDU_CASE_4_SHORT;

                }
            }
        }
        if (len) {
            sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, "APDU too long (%lu bytes extra)\n",
                    (unsigned long) len);
            return SC_ERROR_INVALID_DATA;
        }
    }

    apdu->flags = SC_APDU_FLAGS_NO_GET_RESP|SC_APDU_FLAGS_NO_RETRY_WL;

    sc_debug(ctx, SC_LOG_DEBUG_NORMAL, "Case %d %s APDU, %lu bytes:\tins=%02x p1=%02x p2=%02x lc=%04x le=%04x",
            apdu->cse & SC_APDU_SHORT_MASK,
            (apdu->cse & SC_APDU_EXT) != 0 ? "extended" : "short",
            (unsigned long) len0, apdu->ins, apdu->p1, apdu->p2, apdu->lc, apdu->le);

    return SC_SUCCESS;
}

void _bin_log(sc_context_t *ctx, int type, const char *file, int line,
        const char *func, const char *label, const u8 *data, size_t len,
        FILE *f)
{
    /* this is the maximum supported by sc_do_log_va */
    /* Flawfinder: ignore */
    char buf[1800];

    if (data)
        sc_hex_dump(ctx, SC_LOG_DEBUG_NORMAL, data, len, buf, sizeof buf);
    else
        buf[0] = 0;
    if (!f) {
        sc_do_log(ctx, type, file, line, func,
                "\n%s (%u byte%s):\n%s",
                label, len, len==1?"":"s", buf);
    } else {
        fprintf(f, "%s (%u byte%s):\n%s",
                label, len, len==1?"":"s", buf);
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
    r = sc_context_create(&ctx, NULL);
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
