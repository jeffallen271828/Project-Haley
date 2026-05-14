/**
 * @file ai_host_interface.c
 * @brief Implementation of the AI Host service layer.
 */

#include "ai_host_interface.h"
#include "ethernet/ethernet_protocol.h"
#include "ethernet/ethernet_driver.h"
#include "cmsis_os.h" // For osKernelGetTickCount

/* Configuration constants derived from stm32_config.py and ethernet_protocol.h */
#define AI_HOST_TIMEOUT_MS  6000U // heartbeat_timeout_s: 6.0

static ai_host_handle_t *s_host_instance = NULL;

haley_proto_err_t AI_Host_Init(ai_host_handle_t *handle, ai_host_event_cb_t event_cb) {
    if (handle == NULL) {
        return HALEY_PROTO_ERR_NULL;
    }

    s_host_instance = handle;

    // Initialize high-level state
    s_host_instance->state.conn_state = AI_HOST_DISCONNECTED;
    s_host_instance->state.health = AI_HOST_HEALTH_UNKNOWN;
    s_host_instance->state.last_heartbeat_rx_ms = osKernelGetTickCount();

    // Clear statistics
    s_host_instance->state.stats.rx_count = 0;
    s_host_instance->state.stats.tx_count = 0;
    s_host_instance->state.stats.crc_errors = 0;
    s_host_instance->state.stats.parse_errors = 0;

    // Set callback context
    s_host_instance->event_callback = event_cb;
    s_host_instance->event_ctx = NULL;

    return HALEY_PROTO_OK;
}

void AI_Host_Process(void) {
    if (s_host_instance == NULL) return;

    uint32_t current_time = osKernelGetTickCount();

    // Check connection status from the driver layer
    eth_drv_state_t drv_state = ETH_DRV_GetState();

    if (drv_state == ETH_DRV_CONNECTED) {
        if (s_host_instance->state.conn_state == AI_HOST_DISCONNECTED) {
            s_host_instance->state.conn_state = AI_HOST_CONNECTED;
            s_host_instance->state.last_heartbeat_rx_ms = current_time;
        }

        // Heartbeat Watchdog logic
        if ((current_time - s_host_instance->state.last_heartbeat_rx_ms) > AI_HOST_TIMEOUT_MS) {
            s_host_instance->state.conn_state = AI_HOST_FAULT;
            s_host_instance->state.health = AI_HOST_HEALTH_BAD;

            if (s_host_instance->event_callback) {
                ai_host_event_t event = { .type = AI_EVENT_DISCONNECTED, .timestamp_ms = current_time };
                s_host_instance->event_callback(&event, s_host_instance->event_ctx);
            }
        }
    } else {
        s_host_instance->state.conn_state = AI_HOST_DISCONNECTED;
    }

    // Call the protocol layer processing (handles periodic heartbeat TX)
    ETH_PROTO_Process();
}

void AI_Host_DataReceived(const uint8_t *data, uint32_t len) {
    if (s_host_instance == NULL) return;

    // Ingest raw data into the protocol state machine
    ETH_PROTO_Ingest(data, len);

    // Update local tracking
    s_host_instance->state.last_packet_rx_ms = osKernelGetTickCount();
    s_host_instance->state.stats.rx_count++;
}

const ai_host_state_t* AI_Host_GetState(void) {
    return (s_host_instance != NULL) ? &s_host_instance->state : NULL;
}
