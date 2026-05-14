/**
 * @file ai_host_interface.h
 * @brief Top-level interface for the AI Host service layer.
 */

#ifndef INC_AI_HOST_AI_HOST_INTERFACE_H_
#define INC_AI_HOST_AI_HOST_INTERFACE_H_

#include "ai_host_state.h"
#include "ai_host_events.h"
#include "protocol/packet_parser.h"
#include "protocol/packet_serializer.h"

/**
 * @brief Global handle for the AI Host interface.
 */
typedef struct {
    ai_host_state_t    state;
    haley_parser_t     parser;
    haley_serializer_t serializer;

    ai_host_event_cb_t event_callback;
    void              *event_ctx;
} ai_host_handle_t;

/**
 * @brief Initializes the AI Host interface and protocol stacks.
 * @return HALEY_PROTO_OK on success.
 */
haley_proto_err_t AI_Host_Init(ai_host_handle_t *handle, ai_host_event_cb_t event_cb);

/**
 * @brief Background processing task for the AI Host (heartbeats/watchdogs).
 */
void AI_Host_Process(void);

/**
 * @brief Ingests bytes received from the Ethernet/Socket transport.
 */
void AI_Host_DataReceived(const uint8_t *data, uint32_t len);

/**
 * @brief Returns a pointer to the current internal state.
 */
const ai_host_state_t* AI_Host_GetState(void);

#endif /* INC_AI_HOST_AI_HOST_INTERFACE_H_ */
