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
#ifndef _CCID_SSLUTIL_H
#define _CCID_SSLUTIL_H

#include <libopensc/opensc.h>
#include <openssl/err.h>
#include <libopensc/log.h>

#define ssl_error(ctx) { \
    unsigned long _r; \
    ERR_load_crypto_strings(); \
    for (_r = ERR_get_error(); _r; _r = ERR_get_error()) { \
        sc_debug(ctx, SC_LOG_DEBUG_VERBOSE, ERR_error_string(_r, NULL)); \
    } \
    ERR_free_strings(); \
}

#endif
