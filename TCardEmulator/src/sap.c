#include "tcardemulator.h"
#include "sap_app.h"
#include <dlog.h>
#include <unistd.h>
#include <stdlib.h>
#include <nfc.h>
#include <sap.h>
#include <glib.h>

#define ACCESSORY_PROFILE_ID "/com/vsmcartcard"
#define ACCESSORY_CHANNELID 104

typedef struct priv {
	sap_agent_h agent;
	sap_socket_h socket;
	sap_peer_agent_h peer_agent;
	nfc_se_h nfc_handle;
} priv_s;

gboolean agent_created = FALSE;
gboolean agent_connected = FALSE;

static priv_s priv_data = { 0 };

void on_peer_agent_updated(sap_peer_agent_h peer_agent,
		sap_peer_agent_status_e peer_status,
		sap_peer_agent_found_result_e result,
		void *user_data) {
	switch (result) {
		case SAP_PEER_AGENT_FOUND_RESULT_DEVICE_NOT_CONNECTED:
			dlog_print(DLOG_INFO, LOG_TAG, "device is not connected");
			break;

		case SAP_PEER_AGENT_FOUND_RESULT_FOUND:
			if (peer_status == SAP_PEER_AGENT_STATUS_AVAILABLE) {
				priv_data.peer_agent = peer_agent;
				dlog_print(DLOG_INFO, LOG_TAG, "requesting service connection");
				request_service_connection();
			} else {
				dlog_print(DLOG_INFO, LOG_TAG, "peer agent removed");
				sap_peer_agent_destroy(peer_agent);
			}
			break;

		case SAP_PEER_AGENT_FOUND_RESULT_SERVICE_NOT_FOUND:
			dlog_print(DLOG_INFO, LOG_TAG, "service not found");
			break;

		case SAP_PEER_AGENT_FOUND_RESULT_TIMEDOUT:
			dlog_print(DLOG_INFO, LOG_TAG, "peer agent find timed out");
			break;

		case SAP_PEER_AGENT_FOUND_RESULT_INTERNAL_ERROR:
			dlog_print(DLOG_INFO, LOG_TAG, "peer agent find search failed");
			break;
	}
}


static void on_service_connection_terminated(sap_peer_agent_h peer_agent,
		sap_socket_h socket,
		sap_service_connection_result_e result,
		void *user_data) {
	switch (result) {
		case SAP_CONNECTION_TERMINATED_REASON_PEER_DISCONNECTED:
			dlog_print(DLOG_INFO, LOG_TAG, "disconnected because peer lost");
			break;

		case SAP_CONNECTION_TERMINATED_REASON_DEVICE_DETACHED:
			dlog_print(DLOG_INFO, LOG_TAG, "disconnected because device is detached");
			break;

		default:
			// fall through
		case SAP_CONNECTION_TERMINATED_REASON_UNKNOWN:
			dlog_print(DLOG_INFO, LOG_TAG, "disconnected because of unknown reason");
			break;
	}

	sap_socket_destroy(priv_data.socket);
	priv_data.socket = NULL;
	agent_connected = FALSE;
}


static void on_data_recieved(sap_socket_h socket,
		unsigned short int channel_id,
		unsigned int payload_length,
		void *buffer,
		void *user_data) {
	dlog_print(DLOG_INFO, LOG_TAG, "received data: %p, len:%d", buffer, payload_length);

	char* string_buffer = (char*) buffer;
	if (string_buffer[0] == 'a') {
		install_aids(buffer + sizeof(char), payload_length - 1);
	} else if (string_buffer[0] == 'd') {
		send_apdu_response(priv_data.nfc_handle, buffer + sizeof(char), payload_length - 1);
	}
}

gboolean send_data(nfc_se_h nfc_handle, char* prefix, unsigned int prefix_len, void *message, unsigned int message_len) {
	gboolean result;
	if (nfc_handle != NULL) {
		priv_data.nfc_handle = nfc_handle;
	}
	void *final_message = malloc(prefix_len + message_len);
	memcpy(final_message, prefix, prefix_len);
	memcpy(final_message + prefix_len * sizeof(char), message, message_len);
	result = sap_socket_send_data(priv_data.socket,
				ACCESSORY_CHANNELID, prefix_len + message_len, final_message);

	if (result != SAP_RESULT_SUCCESS) {
		dlog_print(DLOG_INFO, LOG_TAG, "couldn't send sap message: %d", result);
	}

	free(final_message);
	return result;
}

gboolean request_installed_aids() {
	return send_data(NULL, "a", 1, "", 0);
}

static void on_service_connection_created(sap_peer_agent_h peer_agent,
		sap_socket_h socket,
		sap_service_connection_result_e result,
		void *user_data) {
	switch (result) {
		case SAP_CONNECTION_SUCCESS:
			dlog_print(DLOG_INFO, LOG_TAG, "peer agent connection is successful, pa :%u", peer_agent);
			sap_peer_agent_set_service_connection_terminated_cb(priv_data.peer_agent,
					on_service_connection_terminated,
					NULL);

			sap_socket_set_data_received_cb(socket, on_data_recieved, peer_agent);
			priv_data.socket = socket;
			agent_connected = TRUE;

			request_installed_aids();
			break;

		case SAP_CONNECTION_ALREADY_EXIST:
			dlog_print(DLOG_INFO, LOG_TAG, "connection already exists");
			priv_data.socket = socket;
			agent_connected = TRUE;
			break;

		case SAP_CONNECTION_FAILURE_DEVICE_UNREACHABLE:
			dlog_print(DLOG_INFO, LOG_TAG, "device is not unreachable");
			break;

		case SAP_CONNECTION_FAILURE_INVALID_PEERAGENT:
			dlog_print(DLOG_INFO, LOG_TAG, "invalid peer agent");
			break;

		case SAP_CONNECTION_FAILURE_NETWORK:
			dlog_print(DLOG_INFO, LOG_TAG, "network failure");
			break;

		case SAP_CONNECTION_FAILURE_PEERAGENT_NO_RESPONSE:
			dlog_print(DLOG_INFO, LOG_TAG, "peer agent is no response");
			break;

		case SAP_CONNECTION_FAILURE_PEERAGENT_REJECTED:
			dlog_print(DLOG_INFO, LOG_TAG, "peer agent is rejected");
			break;

		case SAP_CONNECTION_FAILURE_UNKNOWN:
			dlog_print(DLOG_INFO, LOG_TAG, "unknown error");
			break;
	}
}

static gboolean _create_service_connection(gpointer user_data) {
	struct priv *priv = NULL;
	sap_result_e result = SAP_RESULT_FAILURE;

	priv = (struct priv *)user_data;
	result = sap_agent_request_service_connection(priv->agent,
			priv->peer_agent,
			on_service_connection_created,
			NULL);

	if (result == SAP_RESULT_SUCCESS) {
		dlog_print(DLOG_DEBUG, LOG_TAG, "req service conn call succeeded");
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "req service conn call is failed (%d)", result);
	}

	return FALSE;
}

gboolean request_service_connection(void) {
	g_idle_add(_create_service_connection, &priv_data);
	return TRUE;
}

static gboolean _terminate_service_connection(gpointer user_data) {
	struct priv *priv = NULL;
	sap_result_e result = SAP_RESULT_FAILURE;

	priv = (struct priv *)user_data;

	if (priv->socket)
		result = sap_peer_agent_terminate_service_connection(priv->peer_agent);
	else {
		return FALSE;
	}

	if (result == SAP_RESULT_SUCCESS) {
		dlog_print(DLOG_DEBUG, LOG_TAG, "terminate service conn call succeeded");

		priv->socket = NULL;
		agent_connected = FALSE;
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "terminate service conn call is failed (%d)", result);
	}

	return FALSE;
}

gboolean terminate_service_connection(void)
{
	g_idle_add(_terminate_service_connection, &priv_data);
	return TRUE;
}

static gboolean _find_peer_agent(gpointer user_data)
{
	struct priv *priv = NULL;
	sap_result_e result = SAP_RESULT_FAILURE;

	priv = (struct priv *)user_data;

	result = sap_agent_find_peer_agent(priv->agent, on_peer_agent_updated, NULL);

	if (result == SAP_RESULT_SUCCESS) {
		dlog_print(DLOG_DEBUG, LOG_TAG, "find peer call succeeded");
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "find peer call failed (%d)", result);
	}

	return FALSE;
}

gboolean find_peers()
{
	g_idle_add(_find_peer_agent, &priv_data);
	dlog_print(DLOG_DEBUG, LOG_TAG, "find peer called");
	return TRUE;
}

static void on_agent_initialized(sap_agent_h agent,
		sap_agent_initialized_result_e result,
		void *user_data)
{
	switch (result) {
		case SAP_AGENT_INITIALIZED_RESULT_SUCCESS:
			dlog_print(DLOG_INFO, LOG_TAG, "agent is initialized");

			priv_data.agent = agent;
			agent_created = TRUE;
			break;

		case SAP_AGENT_INITIALIZED_RESULT_DUPLICATED:
			dlog_print(DLOG_INFO, LOG_TAG, "duplicate registration");
			break;

		case SAP_AGENT_INITIALIZED_RESULT_INVALID_ARGUMENTS:
			dlog_print(DLOG_INFO, LOG_TAG, "invalid arguments");
			break;

		case SAP_AGENT_INITIALIZED_RESULT_INTERNAL_ERROR:
			dlog_print(DLOG_INFO, LOG_TAG, "internal sap error");
			break;

		default:
			dlog_print(DLOG_INFO, LOG_TAG, "unknown status (%d)", result);
			break;
	}
}

static void on_device_status_changed(sap_device_status_e status, sap_transport_type_e transport_type,
		void *user_data)
{
	switch (transport_type) {
		case SAP_TRANSPORT_TYPE_BT:
			dlog_print(DLOG_INFO, LOG_TAG, "connectivity type(%d): bt", transport_type);
			break;

		case SAP_TRANSPORT_TYPE_BLE:
			dlog_print(DLOG_INFO, LOG_TAG, "connectivity type(%d): ble", transport_type);
			break;

		case SAP_TRANSPORT_TYPE_TCP:
			dlog_print(DLOG_INFO, LOG_TAG, "connectivity type(%d): tcp/ip", transport_type);
			break;

		case SAP_TRANSPORT_TYPE_USB:
			dlog_print(DLOG_INFO, LOG_TAG, "connectivity type(%d): usb", transport_type);
			break;

		case SAP_TRANSPORT_TYPE_MOBILE:
			dlog_print(DLOG_INFO, LOG_TAG, "connectivity type(%d): mobile", transport_type);
			break;

		default:
			dlog_print(DLOG_INFO, LOG_TAG, "unknown connectivity type (%d)", transport_type);
			break;
	}

	switch (status) {
		case SAP_DEVICE_STATUS_DETACHED:

			if (priv_data.peer_agent) {
				sap_socket_destroy(priv_data.socket);
				priv_data.socket = NULL;
				sap_peer_agent_destroy(priv_data.peer_agent);
				priv_data.peer_agent = NULL;
				agent_connected = FALSE;
			}
			break;

		case SAP_DEVICE_STATUS_ATTACHED:
			if (agent_created) {
				g_idle_add(_find_peer_agent, &priv_data);
			}
			break;

		default:
			dlog_print(DLOG_INFO, LOG_TAG, "unknown status (%d)", status);
			break;
	}
}

gboolean agent_initialize()
{
	int result = 0, i = 1;
	gboolean agent_initialized = TRUE;

	do {
		if (i > 100) {
			agent_initialized = FALSE;
			break;
		}
		result = sap_agent_initialize(priv_data.agent,
				ACCESSORY_PROFILE_ID, SAP_AGENT_ROLE_CONSUMER,
				on_agent_initialized, NULL);
		dlog_print(DLOG_DEBUG, LOG_TAG, "sap_agent_initialize >>> %d", result);
		i++;
	} while (result != SAP_RESULT_SUCCESS);

	return agent_initialized;
}

void initialize_sap()
{
	sap_agent_h agent = NULL;

	sap_agent_create(&agent);

	if (agent == NULL)
		dlog_print(DLOG_ERROR, LOG_TAG, "ERROR in creating agent");
	else
		dlog_print(DLOG_INFO, LOG_TAG, "SUCCESSFULLY create sap agent");

	priv_data.agent = agent;

	sap_set_device_status_changed_cb(on_device_status_changed, NULL);

	agent_initialize();
}
