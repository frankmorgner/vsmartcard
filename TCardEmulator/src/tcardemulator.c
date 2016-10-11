#include "tcardemulator.h"
#include "sap_app.h"
#include <service_app.h>
#include <nfc.h>
#include <dlog.h>
#include <unistd.h>
#include <stdlib.h>
#include <glib.h>

typedef struct appdata {

} appdata_s;

static void hce_event_cb(nfc_se_h handle, nfc_hce_event_type_e event, unsigned char *apdu, unsigned int apdu_len, void *user_data) {
	switch (event) {
		case NFC_HCE_EVENT_DEACTIVATED:
			dlog_print(DLOG_DEBUG, LOG_TAG, "received NFC_HCE_EVENT_DEACTIVATED event on NFC handle %d", handle);
			break;

		case NFC_HCE_EVENT_ACTIVATED:
			dlog_print(DLOG_DEBUG, LOG_TAG, "received NFC_HCE_EVENT_ACTIVATED event on NFC handle %d", handle);
			if (!agent_connected) {
				find_peers();
			}
			break;

		case NFC_HCE_EVENT_APDU_RECEIVED:
			dlog_print(DLOG_DEBUG, LOG_TAG, "received NFC_HCE_EVENT_APDU_RECEIVED event on NFC handle %d", handle);
			if (agent_connected) {
				send_data(handle, "d", 1, apdu, apdu_len);
			} else {
				dlog_print(DLOG_INFO, LOG_TAG, "couldn't send message on SAP channel because agent is not connected");
			}
			break;

		default:
			dlog_print(DLOG_DEBUG, LOG_TAG, "received unknown event on NFC handle %d", handle);
			break;
	}
}

void register_aid(gpointer data) {
	int result = nfc_se_register_aid(NFC_SE_TYPE_HCE, NFC_CARD_EMULATION_CATEGORY_OTHER, data);
	if (result != NFC_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "nfc_se_register_aid for aid %s failed: %d", data, result);
	} else {
		dlog_print(DLOG_DEBUG, LOG_TAG, "nfc_se_register_aid for aid %s succeeded.", data);
	}
}

void install_aids(void *buffer, unsigned int buffer_size) {
	gchar aid_string[buffer_size + 1];
	g_strlcpy(aid_string, (gchar *)buffer, buffer_size + 1);

	gchar **aid_buffer = g_strsplit(aid_string, ",", 0);
	int i;
	int aid_len = g_strv_length(aid_buffer);
	for (i = 0; i < aid_len; i++) {
		register_aid((gpointer)aid_buffer[i]);
	}
	g_strfreev(aid_buffer);
}

void send_apdu_response(nfc_se_h handle, unsigned char *resp, unsigned int resp_len) {
	dlog_print(DLOG_DEBUG, LOG_TAG, "sending data to nfc handle: %d", handle);
	int result = nfc_hce_send_apdu_response(handle, resp, resp_len);
	if (result != NFC_ERROR_NONE) {
		dlog_print(DLOG_DEBUG, LOG_TAG, "error sending data to nfc handle %d: ", handle);
	}
}

bool service_app_create(void *data) {
	int ret = NFC_ERROR_NONE;
	bool r = false;
	nfc_se_card_emulation_mode_type_e ce_type;

	ret = nfc_manager_initialize();
	if (ret != NFC_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "nfc_manager_initialize failed : %d", ret);
		goto err;
	}

	ret = nfc_se_get_card_emulation_mode(&ce_type);
	if (ret != NFC_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "nfc_se_get_card_emulation_mode failed : %d", ret);
		goto err;
	}

	if (ce_type == NFC_SE_CARD_EMULATION_MODE_OFF) {
		nfc_se_enable_card_emulation();
		if (ret != NFC_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "nfc_se_enable_card_emulation failed : %d", ret);
			goto err;
		}
	}

	ret = nfc_manager_set_hce_event_cb(hce_event_cb, NULL);
	if (ret != NFC_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "nfc_manager_set_hce_event_cb failed : %d", ret);
		goto err;
	} else {
		dlog_print(DLOG_DEBUG, LOG_TAG, "nfc_manager_set_hce_event_cb succeeded");
	}

	initialize_sap();

	find_peers();

	r = true;

err:
	return r;
}

void service_app_terminate(void *data) {
	int ret = NFC_ERROR_NONE;

	nfc_manager_unset_hce_event_cb();

	ret = nfc_manager_deinitialize();
	if (ret != NFC_ERROR_NONE)
	{
		dlog_print(DLOG_ERROR, LOG_TAG, "nfc_manager_deinitialize failed : %d", ret);
	}

	terminate_service_connection();

	return;
}

void service_app_control(app_control_h app_control, void *data) {
	return;
}

void service_app_low_memory(void *data) {
	service_app_terminate(&data);

	return;
}

void service_app_low_battery(void *data) {
	service_app_terminate(&data);

	return;
}

int main(int argc, char* argv[]) {
	appdata_s ad = {};

	service_app_event_callback_s event_callback;

	event_callback.create = service_app_create;
	event_callback.terminate = service_app_terminate;
	event_callback.app_control = service_app_control;
	event_callback.low_memory = service_app_low_memory;
	event_callback.low_battery = service_app_low_battery;

	return svc_app_main(argc, argv, &event_callback, &ad);
}
