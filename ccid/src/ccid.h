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
/**
 * @file
 */
#ifndef _CCID_H
#define _CCID_H

#include <linux/usb/ch9.h>
#include <libopensc/opensc.h>
#include "ccid-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes reader for relaying
 * 
 * @param[in] reader_id (optional) Index to the reader to be used. Set to -1 to use the first reader with a card or the first reader if no card is available.
 * @param[in] verbose   Verbosity level passed to \c sc_context_t
 * 
 * @return \c SC_SUCCESS or error code if an error occurred
 */
int ccid_initialize(int reader_id, int verbose);

/** 
 * @brief Disconnects from card, reader and releases allocated memory
 */
void ccid_shutdown(void);


/**
 * @brief Parses input from PC and generates the appropriate RDR response
 *
 * Parses command pipe bulk-OUT messages and generates resoponse pipe bulk-IN
 * messages according to CCID Rev 1.1 section 6.1, 6.2
 * 
 * @param[in]     inbuf  input buffer (command pipe bulk-OUT message)
 * @param[in]     inlen  length of \a inbuf
 * @param[in,out] outbuf where to save the output buffer (resoponse pipe bulk-IN message), memory is reused via \c realloc()
 * 
 * @return length of \a outbuf or -1 if an error occurred
 */
int ccid_parse_bulkout(const __u8* inbuf, size_t inlen, __u8** outbuf);

/** 
 * @brief Parses input from control pipe and generates the appropriate response
 *
 * Parses CCID class-specific requests according to CCID Rev 1.1 section 5.3
 * 
 * @param[in]     setup  input from control pipe
 * @param[in,out] outbuf where to save the output buffer, memory is reused via \c realloc()
 * 
 * @return length of \a outbuf or -1 if an error occurred
 */
int ccid_parse_control(struct usb_ctrlrequest *setup, __u8 **outbuf);

/** 
 * @brief Generates event messages
 * 
 * @param[in,out] slotchange where to save the output
 * @param[in]     timeout    currently not used
 * @note ccid_state_changed() must be called periodically. Because the OpenSC implementation of \c sc_wait_for_event() blocks all other operations with the reader, it can't be used for slot state detection.
 * 
 * @return 1 if a card is present and/or the state is changed or 0
 */
int ccid_state_changed(RDR_to_PC_NotifySlotChange_t **slotchange, int timeout);

#ifdef  __cplusplus
}
#endif
#endif
