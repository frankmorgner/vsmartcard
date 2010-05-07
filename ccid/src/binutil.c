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
#include <stdio.h>
#include <string.h>
#include <opensc/opensc.h>

void print_usage(const char *app_name, const struct option options[],
	const char *option_help[])
{
    int i = 0;
    printf("Usage: %s [OPTIONS]\nOptions:\n", app_name);

    while (options[i].name) {
        char buf[40], tmp[5];
        const char *arg_str;

        /* Skip "hidden" options */
        if (option_help[i] == NULL) {
            i++;
            continue;
        }

        if (options[i].val > 0 && options[i].val < 128)
            sprintf(tmp, "-%c", options[i].val);
        else
            tmp[0] = 0;
        switch (options[i].has_arg) {
            case 1:
                arg_str = " <arg>";
                break;
            case 2:
                arg_str = " [arg]";
                break;
            default:
                arg_str = "";
                break;
        }
        sprintf(buf, "--%-13s%s%s", options[i].name, tmp, arg_str);
        if (strlen(buf) > 24) {
            printf("  %s\n", buf);
            buf[0] = '\0';
        }
        printf("  %-24s %s\n", buf, option_help[i]);
        i++;
    }
}

void parse_error(const char *app_name, const struct option options[],
        const char *option_help[], const char *optarg, int opt_ind)
{
    printf("Could not parse %s ('%s').\n", options[opt_ind].name, optarg);
    print_usage(app_name , options, option_help);
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
