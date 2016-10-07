#ifndef __sap_H__
#define __sap_H__

#include <nfc.h>

void send_apdu_response(nfc_se_h handle, unsigned char *resp, unsigned int resp_len);

#endif /* __tcardemulator_H__ */
