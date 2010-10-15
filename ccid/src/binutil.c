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

void print_usage(const char *app_name, const struct option options[],
	const char *option_help[])
{
    int i = 0;
    printf("Usage: %s [OPTIONS]\nOptions:\n", app_name);

    while (options[i].name) {
        /* Flawfinder: ignore */
        char buf[40], tmp[5];
        const char *arg_str;

        /* Skip "hidden" options */
        if (option_help[i] == NULL) {
            i++;
            continue;
        }

        if (options[i].val > 0 && options[i].val < 128)
            /* Flawfinder: ignore */
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
        snprintf(buf, sizeof(buf), "--%-13s%s%s", options[i].name, tmp, arg_str);
        if (strnlen(buf, sizeof(buf)) > 24) {
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
