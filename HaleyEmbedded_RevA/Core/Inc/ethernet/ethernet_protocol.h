/*
 * ethernet_protocol.h
 *
 * Packet-level receive state machine for the Haley binary protocol.
 *
 * Owns:
 *   - Byte-stream to packet reassembly
 *   - Header and payload CRC validation
 *   - Inbound packet dispatch (routes to ethernet_commands or audio task)
 *   - Outbound packet helpers (heartbeat, ACK, NACK, status)
 *   - Tx sequence number management
 *
 * This module sits above ethernet_driver (raw bytes) and below
 * ethernet_commands (application-level command routing).
 *
 * Layer: Service / Protocol Layer
 * Dependencies: ethernet_driver.h, ethernet_packets.h, cmsis_os.h
 *
 *  Created on: May 12, 2026
 *      Author: jeffreya181
 */

#ifndef INC_ETHERNET_ETHERNET_PROTOCOL_H_
#define INC_ETHERNET_ETHERNET_PROTOCOL_H_

#include <stdint.h>
#include <stddef.h>
#include "cmsis_os.h"
#include "ethernet/ethernet_packets.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------
 * Configuration
 * ----------------------------------------------------------------------- */

/**
 * @brief  Number of milliseconds between heartbeat transmissions.
 *         Set to 0 to disable periodic heartbeats.
 */
#define HALEY_PROTO_HEARTBEAT_INTERVAL_MS   2000U

/**
 * @brief  Number of consecutive receive errors before the protocol layer
 *         requests a connection reset.
 */
#define HALEY_PROTO_MAX_RECV_ERRORS         5U

/* -----------------------------------------------------------------------
 * Protocol-level result codes
 * ----------------------------------------------------------------------- */

typedef enum {
    ETH_PROTO_OK            =  0,
    ETH_PROTO_ERR_NO_CONN   = -1,  /**< No active driver connection          */
    ETH_PROTO_ERR_HDR_CRC   = -2,  /**< Header CRC check failed              */
    ETH_PROTO_ERR_PAY_CRC   = -3,  /**< Payload CRC check failed             */
    ETH_PROTO_ERR_BAD_MAGIC = -4,  /**< Invalid magic bytes                  */
    ETH_PROTO_ERR_BAD_VER   = -5,  /**< Unsupported protocol version         */
    ETH_PROTO_ERR_BAD_LEN   = -6,  /**< Payload length out of range          */
    ETH_PROTO_ERR_TIMEOUT   = -7,  /**< Receive timed out                    */
    ETH_PROTO_ERR_SEND      = -8,  /**< Send failed                          */
    ETH_PROTO_ERR_OVERFLOW  = -9,  /**< Receive buffer overflow              */
} eth_proto_result_t;

/* -----------------------------------------------------------------------
 * Inbound packet handler callback
 * ----------------------------------------------------------------------- */

/**
 * @brief  Application-level callback invoked for every valid inbound packet.
 *
 * @param  pkt  Pointer to a fully validated, fully reassembled packet.
 *              The pointer is only valid for the duration of the callback;
 *              the caller must copy any data it needs to retain.
 */
typedef void (*eth_proto_rx_handler_t)(const haley_packet_t *pkt);

/* -----------------------------------------------------------------------
 * API
 * ----------------------------------------------------------------------- */

/**
 * @brief  Initialize the protocol layer.
 *         Resets the receive state machine and sequence counter.
 *
 * @param  rx_handler  Callback invoked for every valid inbound packet.
 *                     Must not be NULL.
 * @return ETH_PROTO_OK, or an error code.
 */
eth_proto_result_t ETH_PROTO_Init(eth_proto_rx_handler_t rx_handler);

/**
 * @brief  Main receive processing function.
 *
 *         Calls ETH_DRV_Receive() to read raw bytes from the TCP stream,
 *         feeds them through the reassembly state machine, and invokes
 *         rx_handler for each complete, validated packet.
 *
 *         Intended to be called repeatedly from ethernetRxTask's main loop.
 *         Blocks for up to HALEY_RECV_TIMEOUT_MS waiting for data.
 *
 * @return ETH_PROTO_OK if at least one packet was processed,
 *         ETH_PROTO_ERR_TIMEOUT if no data arrived within the timeout,
 *         ETH_PROTO_ERR_NO_CONN if the connection was dropped.
 */
eth_proto_result_t ETH_PROTO_ProcessReceive(void);

/**
 * @brief  Send a heartbeat packet to the AI host.
 *
 * @param  uptime_ms      Current MCU uptime in milliseconds.
 * @param  system_state   Current system state byte.
 * @return ETH_PROTO_OK or ETH_PROTO_ERR_SEND.
 */
eth_proto_result_t ETH_PROTO_SendHeartbeat(uint32_t uptime_ms,
                                            uint8_t  system_state);

/**
 * @brief  Send an ACK for a received packet sequence number.
 *
 * @param  acked_seq  Sequence number of the packet being acknowledged.
 * @return ETH_PROTO_OK or ETH_PROTO_ERR_SEND.
 */
eth_proto_result_t ETH_PROTO_SendACK(uint16_t acked_seq);

/**
 * @brief  Send a NACK for a received packet.
 *
 * @param  rejected_seq  Sequence number of the rejected packet.
 * @param  error_code    haley_err_t reason code.
 * @return ETH_PROTO_OK or ETH_PROTO_ERR_SEND.
 */
eth_proto_result_t ETH_PROTO_SendNACK(uint16_t    rejected_seq,
                                       haley_err_t error_code);

/**
 * @brief  Send a sensor telemetry packet.
 *
 * @param  telem  Pointer to a populated haley_pay_telem_sensor_t.
 * @return ETH_PROTO_OK or ETH_PROTO_ERR_SEND.
 */
eth_proto_result_t ETH_PROTO_SendTelemetrySensor(
    const haley_pay_telem_sensor_t *telem);

/**
 * @brief  Send a system status packet.
 *
 * @param  status  Pointer to a populated haley_pay_status_sys_t.
 * @return ETH_PROTO_OK or ETH_PROTO_ERR_SEND.
 */
eth_proto_result_t ETH_PROTO_SendSystemStatus(
    const haley_pay_status_sys_t *status);

/**
 * @brief  Send a raw audio data packet.
 *
 * @param  audio_data  Pointer to raw PCM bytes.
 * @param  data_len    Number of PCM bytes.
 * @return ETH_PROTO_OK or ETH_PROTO_ERR_SEND.
 */
eth_proto_result_t ETH_PROTO_SendAudioData(const uint8_t *audio_data,
                                            uint16_t       data_len);

/**
 * @brief  Send a fault notification.
 *
 * @param  fault_flags   Bitmask of active fault conditions.
 * @param  uptime_ms     MCU uptime at the time of the fault.
 * @return ETH_PROTO_OK or ETH_PROTO_ERR_SEND.
 */
eth_proto_result_t ETH_PROTO_SendFaultNotification(uint8_t  fault_flags,
                                                    uint32_t uptime_ms);

/**
 * @brief  Return the current Tx sequence number (for diagnostics).
 */
uint16_t ETH_PROTO_GetTxSeq(void);

/**
 * @brief  Return the total number of valid packets received since init.
 */
uint32_t ETH_PROTO_GetRxPacketCount(void);

/**
 * @brief  Return the total number of CRC errors since init.
 */
uint32_t ETH_PROTO_GetCRCErrorCount(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_ETHERNET_ETHERNET_PROTOCOL_H_ */
