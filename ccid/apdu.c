/*
 * apdu.c: basic APDU handling functions
 *
 * Copyright (C) 2005 Nils Larsch <nils@larsch.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "apdu.h"
#include <opensc/log.h>
#include <opensc/types.h>
#include <string.h>


/*********************************************************************/
/*   higher level APDU transfer handling functions                   */
/*********************************************************************/
/*   +------------------+
 *   | sc_transmit_apdu |
 *   +------------------+
 *         |     |
 *         |     |     detect APDU cse               +--------------------+
 *         |     +---------------------------------> | sc_detect_apdu_cse |
 *         |                                         +--------------------+
 *         |                                                               
 *         |                                                               
 *         |                                                               
 *         |           send single APDU              +--------------------+
 *         +---------------------------------------> | do_single_transmit |
 *                        ^                          +--------------------+
 *                        |                               |
 *                        |  re-transmit if wrong length  |
 *                        |       or GET RESPONSE         |
 *                        +-------------------------------+
 *                                                        |
 *                                                        v
 *                                               card->reader->ops->tranmit
 */


#ifndef _WIN32
#include <unistd.h>
#define msleep(t)       usleep((t) * 1000)
#else
#define msleep(t)       Sleep(t)
#define sleep(t)        Sleep((t) * 1000)
#endif


/** Tries to determine the APDU type (short or extended) of the supplied
 *  APDU if one of the SC_APDU_CASE_? types is used. 
 *  @param  apdu  APDU object
 */
static void sc_detect_apdu_cse(const sc_card_t *card, sc_apdu_t *apdu)
{
	if (apdu->cse == SC_APDU_CASE_2 || apdu->cse == SC_APDU_CASE_3 ||
	    apdu->cse == SC_APDU_CASE_4) {
		int btype = apdu->cse & SC_APDU_SHORT_MASK;
		/* if either Lc or Le is bigger than the maximun for
		 * short APDUs and the card supports extended APDUs
		 * use extended APDUs (unless Lc is greater than
		 * 255 and command chaining is activated) */
		if ((apdu->le > 256 || (apdu->lc > 255 && (apdu->flags & SC_APDU_FLAGS_CHAINING) == 0)) &&
		    (card->caps & SC_CARD_CAP_APDU_EXT) != 0)
			btype |= SC_APDU_EXT;
		apdu->cse = btype;
	}
}


/** Sends a single APDU to the card reader and calls 
 *  GET RESPONSE to get the return data if necessary.
 *  @param  card  sc_card_t object for the smartcard
 *  @param  apdu  APDU to be send
 *  @return SC_SUCCESS on success and an error value otherwise
 */
static int do_single_transmit(sc_card_t *card, sc_apdu_t *apdu)
{
	int          r;
	size_t       olen  = apdu->resplen;
	sc_context_t *ctx  = card->ctx;

	/* XXX: insert secure messaging here (?), i.e. something like
	if (card->sm_ctx->use_sm != 0) {
		r = card->ops->sm_transform(...);
		if (r != SC_SUCCESS)
			...
		r = sc_check_apdu(...);
		if (r != SC_SUCCESS)
			...
	}
	*/

	/* send APDU to the reader driver */
	if (card->reader->ops->transmit == NULL)
		return SC_ERROR_NOT_SUPPORTED;
	r = card->reader->ops->transmit(card->reader, card->slot, apdu);
	if (r != 0) {
		sc_error(ctx, "unable to transmit APDU");
		return r;
	}
	/* ok, the APDU was successfully transmitted. Now we have two
	 * special cases:
	 * 1. the card returned 0x6Cxx: in this case we re-trasmit the APDU
	 *    wit hLe set to SW2 (this is course only possible if the
	 *    response buffer size is larger than the new Le = SW2)
	 */
	if (apdu->sw1 == 0x6C && (apdu->flags & SC_APDU_FLAGS_NO_RETRY_WL) == 0) {
		size_t nlen = apdu->sw2 != 0 ? (size_t)apdu->sw2 : 256;
		if (olen >= nlen) {
			/* don't try again if it doesn't work this time */
			apdu->flags  |= SC_APDU_FLAGS_NO_GET_RESP;
			/* set the new expected length */
			apdu->resplen = olen;
			apdu->le      = nlen;
			/* as some reader/smartcards can't handle an immediate
			 * re-transmit so we optionally need to sleep for
			 * a while */
			if (card->wait_resend_apdu != 0)
				msleep(card->wait_resend_apdu);
			/* re-transmit the APDU with new Le length */
			r = card->reader->ops->transmit(card->reader, card->slot, apdu);
			if (r != SC_SUCCESS) {
				sc_error(ctx, "unable to transmit APDU");
				return r;
			}
		} else {
			/* we cannot re-transmit the APDU with the demanded
			 * Le value as the buffer is too small => error */
			sc_debug(ctx, "wrong length: required length exceeds resplen");
			return SC_ERROR_WRONG_LENGTH;
		}
	}

	/* 2. the card returned 0x61xx: more data can be read from the card
	 *    using the GET RESPONSE command (mostly used in the T0 protocol).
	 *    Unless the SC_APDU_FLAGS_NO_GET_RESP is set we try to read as
	 *    much data as possible using GET RESPONSE.
	 */
	if (apdu->sw1 == 0x61 && (apdu->flags & SC_APDU_FLAGS_NO_GET_RESP) == 0) {
		if (apdu->le == 0) {
			/* no data is requested => change return value to
			 * 0x9000 and ignore the remaining data */
			/* FIXME: why not return 0x61xx ? It's not an
			 *        error */
			apdu->sw1 = 0x90;
			apdu->sw2 = 0x00;
			
		} else {
			/* call GET RESPONSE until we have read all data
			 * requested or until the card retuns 0x9000, 
			 * whatever happens first.
			 */
			size_t le, minlen, buflen;
			u8     *buf;

			if (card->ops->get_response == NULL) {
				/* this should _never_ happen */
				sc_error(ctx, "no GET RESPONSE command\n");
                        	return SC_ERROR_NOT_SUPPORTED;
	                }

			/* if the command already returned some data 
			 * append the new data to the end of the buffer
			 */
			buf = apdu->resp + apdu->resplen;

			/* read as much data as fits in apdu->resp (i.e.
			 * max(apdu->resplen, amount of data available)).
			 */
			buflen = olen - apdu->resplen;

			/* 0x6100 means at least 256 more bytes to read */
			le = apdu->sw2 != 0 ? (size_t)apdu->sw2 : 256;
			/* we try to read at least as much as bytes as 
			 * promised in the response bytes */
			minlen = le;

			do {
				u8 tbuf[256];
				/* call GET RESPONSE to get more date from
				 * the card; note: GET RESPONSE returns the
				 * amount of data left (== SW2) */
				r = card->ops->get_response(card, &le, tbuf);
				if (r < 0)
					SC_FUNC_RETURN(ctx, 2, r);

				if (buflen < le)
					return SC_ERROR_WRONG_LENGTH;

				memcpy(buf, tbuf, le);
				buf    += le;
				buflen -= le;

				minlen -= le;
				if (r != 0) 
					le = minlen = (size_t)r;
				else
					/* if the card has returned 0x9000 but
					 * we still expect data ask for more 
					 * until we have read enough bytes */
					le = minlen;
			} while (r != 0 || minlen != 0);
			/* we've read all data, let's return 0x9000 */
			apdu->resplen = buf - apdu->resp;
			apdu->sw1 = 0x90;
			apdu->sw2 = 0x00;
		}
	}

	return SC_SUCCESS;
}

int my_transmit_apdu(sc_card_t *card, sc_apdu_t *apdu)
{
	int r = SC_SUCCESS;

	if (card == NULL || apdu == NULL)
		return SC_ERROR_INVALID_ARGUMENTS;

	SC_FUNC_CALLED(card->ctx, 4);

	/* determine the APDU type if necessary, i.e. to use
	 * short or extended APDUs  */
	sc_detect_apdu_cse(card, apdu);
	/* basic APDU consistency check */
	/*r = sc_check_apdu(card, apdu);*/
	if (r != SC_SUCCESS)
		return SC_ERROR_INVALID_ARGUMENTS;

	r = sc_lock(card);	/* acquire card lock*/
	if (r != SC_SUCCESS) {
		sc_error(card->ctx, "unable to acquire lock");
		return r;
	} 

	if ((apdu->flags & SC_APDU_FLAGS_CHAINING) != 0) {
		/* divide et impera: transmit APDU in chunks with Lc < 255
		 * bytes using command chaining */
		size_t    len  = apdu->datalen;
		const u8  *buf = apdu->data;

		while (len != 0) {
			size_t    plen;
			sc_apdu_t tapdu;
			int       last = 0;

			tapdu = *apdu;
			/* clear chaining flag */
			tapdu.flags &= ~SC_APDU_FLAGS_CHAINING;
			if (len > 255) {
				/* adjust APDU case: in case of CASE 4 APDU
				 * the intermediate APDU are of CASE 3 */
				if ((tapdu.cse & SC_APDU_SHORT_MASK) == SC_APDU_CASE_4_SHORT)
					tapdu.cse--;
				/* XXX: the chunk size must be adjusted when
				 *      secure messaging is used */
				plen          = 255;
				tapdu.cla    |= 0x10;
				tapdu.le      = 0;
				/* the intermediate APDU don't expect data */
				tapdu.lc      = 0;
				tapdu.resplen = 0;
				tapdu.resp    = NULL;
			} else {
				plen = len;
				last = 1;
			}
			tapdu.data    = buf;
			tapdu.datalen = tapdu.lc = plen;

			/*r = sc_check_apdu(card, &tapdu);*/
			if (r != SC_SUCCESS) {
				sc_error(card->ctx, "inconsistent APDU while chaining");
				break;
			}

			r = do_single_transmit(card, &tapdu);
			if (r != SC_SUCCESS)
				break;
			if (last != 0) {
				/* in case of the last APDU set the SW1 
				 * and SW2 bytes in the original APDU */
				apdu->sw1 = tapdu.sw1;
				apdu->sw2 = tapdu.sw2;
				apdu->resplen = tapdu.resplen;
			} else {
				/* otherwise check the status bytes */
				r = sc_check_sw(card, tapdu.sw1, tapdu.sw2);
				if (r != SC_SUCCESS)
					break;
			}
			len -= plen;
			buf += plen;
		}
	} else 
		/* transmit single APDU */
		r = do_single_transmit(card, apdu);
	/* all done => release lock */
	if (sc_unlock(card) != SC_SUCCESS)
		sc_error(card->ctx, "sc_unlock failed");

	return r;
}

