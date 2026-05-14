/*
 * packet_serializer.h
 *
 * Stateful packet serialization and transmission manager for the Haley protocol.
 *
 * Responsibility:
 * Manages monotonic sequence numbers and provides a high-level API for RTOS
 * tasks to generate and transmit packets. Wraps the stateless builders in
 * packet_protocol.h and routes the finalized, CRC-stamped byte stream to a
 * user-provided transmit callback.
 *
 * Thread safety:
 * A haley_serializer_t instance is NOT inherently thread-safe. If multiple
 * tasks (e.g., audioInputTask, sensorTalkTask) share a single serializer
 * instance to write to the Ethernet interface, access to the SRL_Send* * functions must be protected by a mutex.
 *
 * Layer: Protocol / Service Layer [cite: 38, 62]
 * Dependencies: protocol_types.h, protocol_constants.h, packet_protocol.h
 */

#ifndef INC_PROTOCOL_PACKET_SERIALIZER_H_
#define INC_PROTOCOL_PACKET_SERIALIZER_H_

#include <stdint.h>
#include <stddef.h>
#include "protocol/protocol_types.h"
#include "protocol/protocol_constants.h"
#include "protocol/packet_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------
 * Callbacks
 * ----------------------------------------------------------------------- */

/**
 * @brief Application callback invoked when a packet is fully serialized
 * and ready to be transmitted over the physical interface.
 *
 * @param data     Pointer to the fully framed and CRC-stamped raw bytes.
 * @param length   Total size of the packet in bytes (header + payload).
 * @param user_ctx Opaque pointer provided during SRL_Init().
 * @return HALEY_PROTO_OK on successful queue/transmit, or an error code.
 */
typedef haley_proto_err_t (*haley_tx_cb_t)(const uint8_t *data,
                                           size_t length,
                                           void *user_ctx);

/* -----------------------------------------------------------------------
 * Serializer Instance
 * ----------------------------------------------------------------------- */

/**
 * @brief Serializer context. Manages the TX buffer and sequence numbers.
 */
typedef struct {
    /* Sequence number tracking */
    uint32_t next_seq;

    /* Dedicated transmit scratch buffer */
    uint8_t tx_buf[HALEY_PROTO_TX_BUF_SIZE];

    /* Transmission callback */
    haley_tx_cb_t tx_cb;
    void         *user_ctx;

    /* Statistics */
    uint32_t stat_tx_packets;
    uint32_t stat_tx_bytes;
    uint32_t stat_tx_errors;
} haley_serializer_t;

/* -----------------------------------------------------------------------
 * API
 * ----------------------------------------------------------------------- */

/**
 * @brief Initialise a serializer instance.
 *
 * @param srl       Serializer instance to initialise.
 * @param tx_cb     Callback fired to actually send the raw bytes.
 * @param user_ctx  Opaque pointer passed to the callback.
 * @return HALEY_PROTO_OK, or HALEY_PROTO_ERR_NULL_PTR if srl/tx_cb are NULL.
 */
haley_proto_err_t SRL_Init(haley_serializer_t *srl,
                           haley_tx_cb_t       tx_cb,
                           void               *user_ctx);

/**
 * @brief Reset the sequence number counter.
 *
 * Useful after a transport reconnect or version negotiation phase.
 *
 * @param srl Serializer instance.
 */
void SRL_ResetSequence(haley_serializer_t *srl);

/**
 * @brief Build and transmit a heartbeat packet.
 *
 * @param srl        Serializer instance.
 * @param is_request Non-zero to set the REQUEST flag; zero for RESPONSE.
 * @return HALEY_PROTO_OK or underlying transport error.
 */
haley_proto_err_t SRL_SendHeartbeat(haley_serializer_t *srl, int is_request);

/**
 * @brief Build and transmit an ACK packet.
 *
 * @param srl Serializer instance.
 * @return HALEY_PROTO_OK or underlying transport error.
 */
haley_proto_err_t SRL_SendACK(haley_serializer_t *srl);

/**
 * @brief Build and transmit a command packet.
 *
 * @param srl        Serializer instance.
 * @param kv_payload Pre-formatted key-value text payload.
 * @param flags      Flags to embed (e.g., HALEY_FLAG_ACK_REQUIRED).
 * @return HALEY_PROTO_OK or underlying transport error.
 */
haley_proto_err_t SRL_SendCommand(haley_serializer_t *srl,
                                  const char         *kv_payload,
                                  haley_pkt_flags_t   flags);

/**
 * @brief Build and transmit a telemetry packet.
 *
 * @param srl        Serializer instance.
 * @param kv_payload Pre-formatted key-value text payload.
 * @return HALEY_PROTO_OK or underlying transport error.
 */
haley_proto_err_t SRL_SendTelemetry(haley_serializer_t *srl,
                                    const char         *kv_payload);

/**
 * @brief Build and transmit an emergency fault packet.
 *
 * @param srl        Serializer instance.
 * @param kv_payload Pre-formatted key-value describing the fault.
 * @return HALEY_PROTO_OK or underlying transport error.
 */
haley_proto_err_t SRL_SendFault(haley_serializer_t *srl,
                                const char         *kv_payload);

/**
 * @brief Build and transmit a state update packet.
 *
 * @param srl        Serializer instance.
 * @param kv_payload Pre-formatted key-value text payload.
 * @return HALEY_PROTO_OK or underlying transport error.
 */
haley_proto_err_t SRL_SendState(haley_serializer_t *srl,
                                const char         *kv_payload);

/**
 * @brief Build and transmit a protocol version negotiation packet.
 *
 * Automatically populates the device name and configuration limits.
 *
 * @param srl Serializer instance.
 * @return HALEY_PROTO_OK or underlying transport error.
 */
haley_proto_err_t SRL_SendVersionNegotiation(haley_serializer_t *srl);

/**
 * @brief Build and transmit a raw binary payload (e.g., PCM audio).
 *
 * @param srl         Serializer instance.
 * @param msg_type    The message type ID.
 * @param flags       Packet flags.
 * @param payload     Pointer to binary payload.
 * @param payload_len Length of the binary payload.
 * @return HALEY_PROTO_OK or underlying transport error.
 */
haley_proto_err_t SRL_SendRaw(haley_serializer_t *srl,
                              haley_msg_type_t    msg_type,
                              haley_pkt_flags_t   flags,
                              const uint8_t      *payload,
                              uint32_t            payload_len);

#ifdef __cplusplus
}
#endif

#endif /* INC_PROTOCOL_PACKET_SERIALIZER_H_ */
