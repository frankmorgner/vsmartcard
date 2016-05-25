#include "tcardemulator.h"
#include <service_app.h>
#include <nfc.h>
#include <dlog.h>
#include <unistd.h>
#include <stdlib.h>

unsigned char *rapdu = NULL;
size_t rapdu_length = 0;
gboolean rapdu_received = FALSE;

	static void
hce_event_cb(nfc_se_h handle, nfc_hce_event_type_e event,
		unsigned char *apdu, unsigned int apdu_len, void *user_data)
{
	switch (event) {
		case NFC_HCE_EVENT_DEACTIVATED:
			// Do something when NFC_HCE_EVENT_DEACTIVATED event arrives
			// When the event arrives, apdu and apdu len is NULL and 0
			terminate_service_connection();
			break;

		case NFC_HCE_EVENT_ACTIVATED:
			// Do something when NFC_HCE_EVENT_ACTIVATED event arrives
			// When the event arrives, apdu and apdu len is NULL and 0
			find_peers();
			break;

		case NFC_HCE_EVENT_APDU_RECEIVED:
			rapdu_received = FALSE;
			send_data(apdu, apdu_len);
			size_t count = 0;
			while (!rapdu_received && count < 10) {
				dlog_print(DLOG_INFO, LOG_TAG, "waiting for response");
				usleep(100);
				count++;
			}
			nfc_hce_send_apdu_response(handle, rapdu, rapdu_length);
			rapdu_received = FALSE;
			break;

		default:
			// Error handling
			break;
	}
}

	bool
service_app_create(void *data)
{
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
	}

	initialize_sap();

	r = true;

err:
	return r;
}

	void
service_app_terminate(void *data)
{
	int ret = NFC_ERROR_NONE;

	nfc_manager_unset_hce_event_cb();

	ret = nfc_manager_deinitialize();
	if (ret != NFC_ERROR_NONE)
	{
		dlog_print(DLOG_ERROR, LOG_TAG, "nfc_manager_deinitialize failed : %d", ret);
	}

	terminate_service_connection();

	free(rapdu);
	rapdu = NULL;
	rapdu_length = 0;
	rapdu_received = FALSE;

	return;
}

	void
service_app_control(app_control_h app_control, void *data)
{
	// Todo: add your code here

	return;
}

	void
service_app_low_memory_callback(void *data)
{
	// Todo: add your code here
	service_app_exit();

	return;
}

	void
service_app_low_battery_callback(void *data)
{
	// Todo: add your code here
	service_app_exit();

	return;
}

	int
main(int argc, char* argv[])
{
	char ad[50] = {0,};

	service_app_event_callback_s event_callback;

	event_callback.create = service_app_create;
	event_callback.terminate = service_app_terminate;
	event_callback.app_control = service_app_control;
	event_callback.low_memory = service_app_low_memory_callback;
	event_callback.low_battery = service_app_low_battery_callback;

#if 1
	initialize_sap();
	find_peers();
	rapdu_received = FALSE;
	unsigned char apdu[] = {0x00, 0xa4, 0x00, 0x00};
	send_data(apdu, sizeof apdu);
	size_t count = 0;
	while (!rapdu_received && count < 10) {
		dlog_print(DLOG_INFO, LOG_TAG, "waiting for response");
		usleep(100);
		count++;
	}
#endif

	return svc_app_main(argc, argv, &event_callback, ad);
}
