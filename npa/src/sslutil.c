/*
 * Copyright (C) 2011 Frank Morgner
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
#include "sslutil.h"
#include <openssl/err.h>
#include <libopensc/log.h>

void ssl_error(sc_context_t *ctx) {
    unsigned long r;
    ERR_load_crypto_strings();
    for (r = ERR_get_error(); r; r = ERR_get_error()) {
        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, ERR_error_string(r, NULL));
    }
    ERR_free_strings();
}
