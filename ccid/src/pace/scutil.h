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
/**
 * @file
 */
#ifndef _CCID_SCUTIL_H
#define _CCID_SCUTIL_H

#include <opensc/opensc.h>

/** 
 * @brief Initializes smart card context and reader
 * 
 * @param[in]     reader_id Index to the reader to be used (optional). Set to -1 to use a reader with a inserted card.
 * @param[in]     cdriver   Card driver to be used (optional)
 * @param[in]     verbose   verbosity level passed to \c sc_context_t
 * @param[in,out] ctx       Where to write the sc context
 * @param[in,out] reader    Where to write the reader context
 * 
 * @return 
 */
int initialize(int reader_id, const char *cdriver, int verbose,
        sc_context_t **ctx, sc_reader_t **reader);

/** 
 * @brief Parse an APDU buffer
 * 
 * @param[in]      ctx  sc context
 * @param[in]      buf  Buffer of APDU
 * @param[in]      len  Length of \a buf
 * @param [in,out] apdu Where to write the initialized APDU
 * 
 * @return \c SC_SUCCESS or error code if an error occurred
 */
int build_apdu(sc_context_t *ctx, const u8 *buf, size_t len, sc_apdu_t *apdu);

/** 
 * @brief Print binary data to a file stream
 * 
 * @param[in] file  File for printing
 * @param[in] label Label to prepend to the buffer
 * @param[in] data  Binary data
 * @param[in] len   Length of \a data
 */
#define bin_print(file, label, data, len) \
    _bin_log(NULL, 0, NULL, 0, NULL, label, data, len, file)
/** 
 * @brief Log binary data to a sc context
 * 
 * @param[in] ctx   Context for logging
 * @param[in] label Label to prepend to the buffer
 * @param[in] data  Binary data
 * @param[in] len   Length of \a data
 */
#define bin_log(ctx, label, data, len) \
    _bin_log(ctx, SC_LOG_TYPE_DEBUG, __FILE__, __LINE__, __FUNCTION__, label, data, len, NULL)
/** 
 * @brief Log binary data
 *
 * Either choose \a ctx or \a file for logging
 * 
 * @param[in] ctx   (optional) Context for logging
 * @param[in] type  Debug level
 * @param[in] file  File name to be prepended
 * @param[in] line  Line to be prepended
 * @param[in] func  Function to be prepended
 * @param[in] label label to prepend to the buffer
 * @param[in] data  binary data
 * @param[in] len   length of \a data
 * @param[in] f     (optional) File for printing
 */
void _bin_log(sc_context_t *ctx, int type, const char *file, int line,
        const char *func, const char *label, const u8 *data, size_t len,
        FILE *f);

int print_avail(int verbose);

#endif
