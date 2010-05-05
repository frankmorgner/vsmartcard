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
#ifndef _CCID_SCUTIL_H
#define _CCID_SCUTIL_H

#include <opensc/opensc.h>

int initialize(int reader_id, const char *cdriver, int verbose,
        sc_context_t **ctx, sc_reader_t **reader);

int build_apdu(sc_context_t *ctx, const u8 *buf, size_t len, sc_apdu_t *apdu);

#endif

