#ifndef __tcardemulator_H__
#define __tcardemulator_H__

#define LOG_TAG "tcardemulator"

#if !defined(PACKAGE)
#define PACKAGE "com.vsmartcard.tcardemulator"
#endif

#include <stddef.h>
#include <glib.h>
#include <nfc.h>
#include <glib.h>

void     initialize_sap();
gboolean find_peers();
gboolean request_service_connection(void);
gboolean terminate_service_connection(void);
gboolean send_data(nfc_se_h nfc_handle, char* prefix, unsigned int prefix_len, void *message, unsigned int message_len);

#endif /* __tcardemulator_H__ */
