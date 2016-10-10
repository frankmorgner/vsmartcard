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
 * @addtogroup scutil Wrappers around OpenSC
 * @{
 */
#ifndef _CCID_SCUTIL_H
#define _CCID_SCUTIL_H

#include <libopensc/opensc.h>

/** 
 * @brief Initializes smart card context and reader
 * 
 * @param[in]     reader_id Index to the reader to be used. Set to -1 to use a reader with an inserted card.
 * @param[in]     verbose   verbosity level passed to \c sc_context_t
 * @param[in,out] ctx       Where to write the sc context
 * @param[in,out] reader    Where to write the reader context
 * 
 * @return 
 */
int initialize(int reader_id, int verbose,
        sc_context_t **ctx, sc_reader_t **reader);

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
 * @param[in] level
 * @param[in] label Label to prepend to the buffer
 * @param[in] data  Binary data
 * @param[in] len   Length of \a data
 */
#define bin_log(ctx, level, label, data, len) \
    _bin_log(ctx, level, __FILE__, __LINE__, __FUNCTION__, label, data, len, NULL)
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

/**
 * @brief Prints the available readers to stdout.
 *
 * @param verbose
 *
 * @return \c SC_SUCCESS or error code if an error occurred
 */
int print_avail(int verbose);

/*
 * OPENSC functions that do not get exported (sometimes)
 */

/**
 * Returns the encoded APDU in newly created buffer.
 * @param  ctx     sc_context_t object
 * @param  apdu    sc_apdu_t object with the APDU to encode
 * @param  buf     pointer to the newly allocated buffer
 * @param  len     length of the encoded APDU
 * @param  proto   protocol to be used
 * @return SC_SUCCESS on success and an error code otherwise
 */
int sc_apdu_get_octets(sc_context_t *ctx, const sc_apdu_t *apdu, u8 **buf,
	size_t *len, unsigned int proto);

/**
 * Sets the status bytes and return data in the APDU
 * @param  ctx     sc_context_t object
 * @param  apdu    the apdu to which the data should be written
 * @param  buf     returned data
 * @param  len     length of the returned data
 * @return SC_SUCCESS on success and an error code otherwise
 */
int sc_apdu_set_resp(sc_context_t *ctx, sc_apdu_t *apdu, const u8 *buf,
	size_t len);

/* Returns an index number if a match was found, -1 otherwise. table has to
 * be null terminated. */
int _sc_match_atr(struct sc_card *card, struct sc_atr_table *table, int *type_out);

int fread_to_eof(const char *file, unsigned char **buf, size_t *buflen);

#endif
/* @} */
