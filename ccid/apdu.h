#ifndef _PACE_H
#define _PACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <opensc.h>

/** Sends a APDU to the card
 *  @param  card  sc_card_t object to which the APDU should be send
 *  @param  apdu  sc_apdu_t object of the APDU to be send
 *  @return SC_SUCCESS on succcess and an error code otherwise
 */
int my_transmit_apdu(sc_card_t *card, sc_apdu_t *apdu);

#ifdef  __cplusplus
}
#endif
#endif
