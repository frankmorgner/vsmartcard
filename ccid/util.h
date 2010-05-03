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
#ifndef _CCID_UTIL_H
#define _CCID_UTIL_H

#include <getopt.h>

void print_usage(const char *app_name, const struct option options[],
	const char *option_help[]);
void parse_error(const char *app_name, const struct option options[],
        const char *option_help[], const char *optarg, int opt_ind);

#endif
