#ifndef __sap_H__
#define __sap_H__

#include <nfc.h>

void send_apdu_response(nfc_se_h handle, unsigned char *resp, unsigned int resp_len);
void install_aids(void *buffer, unsigned int buffer_size);

extern gboolean agent_connected;

#endif /* __tcardemulator_H__ */
