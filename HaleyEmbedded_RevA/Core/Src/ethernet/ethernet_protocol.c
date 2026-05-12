/*
 * ethernet_protocol.c
 *
 * Haley binary protocol — receive state machine and outbound send helpers.
 *
 * Receive state machine overview:
 *
 *   WAIT_MAGIC_0  → WAIT_MAGIC_1  (saw 0x48)
 *   WAIT_MAGIC_1  → RECV_HEADER   (saw 0x4C)
 *   RECV_HEADER   → RECV_PAYLOAD  (accumulated HALEY_HEADER_SIZE bytes, CRC OK)
 *   RECV_PAYLOAD  → RECV_PAY_CRC  (accumulated payload_len bytes)
 *   RECV_PAY_CRC  → dispatch      (accumulated 4 CRC bytes, CRC OK)
 *
 * Any CRC failure or bad byte causes a NACK and reset back to WAIT_MAGIC_0.
 *
 *  Created on: May 12, 2026
 *      Author: jeffreya181
 */

#include <string.h>
#include "cmsis_os.h"
#include "ethernet/ethernet_protocol.h"
#include "ethernet/ethernet_driver.h"
#include "ethernet/ethernet_packets.h"
#include "ethernet/ethernet_commands.h"

/* -----------------------------------------------------------------------
 * Receive state machine
 * ----------------------------------------------------------------------- */

typedef enum {
    RX_STATE_WAIT_MAGIC_0 = 0,
    RX_STATE_WAIT_MAGIC_1,
    RX_STATE_RECV_HEADER,
    RX_STATE_RECV_PAYLOAD,
    RX_STATE_RECV_PAY_CRC,
} rx_state_t;

/* -----------------------------------------------------------------------
 * Module-private state
 * ----------------------------------------------------------------------- */

static eth_proto_rx_handler_t s_rx_handler = NULL;

/* Transmit sequence counter (wraps at 0xFFFF) */
static uint16_t s_tx_seq = 0;

/* Statistics */
static uint32_t s_rx_pkt_count  = 0;
static uint32_t s_crc_err_count = 0;

/* Receive state machine */
static rx_state_t s_rx_state = RX_STATE_WAIT_MAGIC_0;
static uint8_t    s_rx_raw_hdr[HALEY_HEADER_SIZE];
static uint16_t   s_rx_hdr_bytes;       /**< Bytes accumulated in raw header  */
static uint8_t    s_rx_payload[HALEY_MAX_PAYLOAD];
static uint16_t   s_rx_payload_bytes;   /**< Bytes accumulated in payload buf */
static uint8_t    s_rx_crc_buf[4];
static uint8_t    s_rx_crc_bytes;       /**< Bytes accumulated in CRC field   */

/* Decoded header for the packet currently being assembled */
static haley_pkt_header_t s_rx_current_hdr;

/* Raw receive scratch buffer (receives from driver, then fed byte-by-byte) */
#define PROTO_RAW_RX_SIZE 512U
static uint8_t  s_raw_rx_buf[PROTO_RAW_RX_SIZE];

/* -----------------------------------------------------------------------
 * Internal helpers
 * ----------------------------------------------------------------------- */

/** Advance the Tx sequence counter and return the new value */
static inline uint16_t next_tx_seq(void)
{
    s_tx_seq++;
    if (s_tx_seq == 0) {
        s_tx_seq = 1;  /* Skip 0; reserve for "unsequenced" */
    }
    return s_tx_seq;
}

/** Reset the receive state machine to the initial state */
static void reset_rx_state(void)
{
    s_rx_state         = RX_STATE_WAIT_MAGIC_0;
    s_rx_hdr_bytes     = 0;
    s_rx_payload_bytes = 0;
    s_rx_crc_bytes     = 0;
    memset(&s_rx_current_hdr, 0, sizeof(s_rx_current_hdr));
}

/**
 * @brief  Dispatch a fully received and validated packet.
 *         Invokes the registered rx_handler and then ETH_CMD_Dispatch().
 */
static void dispatch_packet(const haley_packet_t *pkt)
{
    s_rx_pkt_count++;

    /* Invoke upper-layer handler */
    if (s_rx_handler != NULL) {
        s_rx_handler(pkt);
    }

    /* Route to command dispatcher */
    ETH_CMD_Dispatch(pkt);
}

/**
 * @brief  Process a single received byte through the state machine.
 */
static void process_byte(uint8_t byte)
{
    switch (s_rx_state) {

    /* ------------------------------------------------------------------ */
    case RX_STATE_WAIT_MAGIC_0:
        if (byte == HALEY_MAGIC_0) {
            s_rx_raw_hdr[0] = byte;
            s_rx_hdr_bytes  = 1;
            s_rx_state      = RX_STATE_WAIT_MAGIC_1;
        }
        /* Silently discard mismatched bytes — common during framing sync */
        break;

    /* ------------------------------------------------------------------ */
    case RX_STATE_WAIT_MAGIC_1:
        if (byte == HALEY_MAGIC_1) {
            s_rx_raw_hdr[1] = byte;
            s_rx_hdr_bytes  = 2;
            s_rx_state      = RX_STATE_RECV_HEADER;
        } else {
            /* Second byte doesn't match — re-check if this byte is MAGIC_0 */
            reset_rx_state();
            if (byte == HALEY_MAGIC_0) {
                s_rx_raw_hdr[0] = byte;
                s_rx_hdr_bytes  = 1;
                s_rx_state      = RX_STATE_WAIT_MAGIC_1;
            }
        }
        break;

    /* ------------------------------------------------------------------ */
    case RX_STATE_RECV_HEADER:
        s_rx_raw_hdr[s_rx_hdr_bytes++] = byte;

        if (s_rx_hdr_bytes < HALEY_HEADER_SIZE) {
            break;  /* Still accumulating */
        }

        /* Full header received — parse and validate */
        {
            haley_err_t parse_err = ETH_PKT_ParseHeader(s_rx_raw_hdr,
                                                         &s_rx_current_hdr);
            if (parse_err != HALEY_ERR_NONE) {
                /* Send NACK and resync */
                s_crc_err_count++;
                ETH_PROTO_SendNACK(0, parse_err);
                reset_rx_state();
                break;
            }

            /* Header OK */
            s_rx_payload_bytes = 0;

            if (s_rx_current_hdr.payload_len == 0) {
                /* No payload — skip directly to CRC state */
                s_rx_crc_bytes = 0;
                s_rx_state     = RX_STATE_RECV_PAY_CRC;
            } else {
                s_rx_state = RX_STATE_RECV_PAYLOAD;
            }
        }
        break;

    /* ------------------------------------------------------------------ */
    case RX_STATE_RECV_PAYLOAD:
        if (s_rx_payload_bytes < s_rx_current_hdr.payload_len) {
            s_rx_payload[s_rx_payload_bytes++] = byte;
        }

        if (s_rx_payload_bytes >= s_rx_current_hdr.payload_len) {
            /* Payload complete — move to CRC state */
            s_rx_crc_bytes = 0;
            s_rx_state     = RX_STATE_RECV_PAY_CRC;
        }
        break;

    /* ------------------------------------------------------------------ */
    case RX_STATE_RECV_PAY_CRC:
        s_rx_crc_buf[s_rx_crc_bytes++] = byte;

        if (s_rx_crc_bytes < HALEY_FOOTER_SIZE) {
            break;  /* Still accumulating CRC bytes */
        }

        /* Full CRC received — validate payload */
        {
            haley_err_t crc_err = ETH_PKT_ValidatePayloadCRC(
                s_rx_payload,
                s_rx_current_hdr.payload_len,
                s_rx_crc_buf);

            if (crc_err != HALEY_ERR_NONE) {
                s_crc_err_count++;
                ETH_PROTO_SendNACK(s_rx_current_hdr.sequence, crc_err);
                reset_rx_state();
                break;
            }

            /* Packet is complete and valid — assemble and dispatch */
            haley_packet_t pkt;
            pkt.header      = s_rx_current_hdr;
            pkt.payload_crc = ((uint32_t)s_rx_crc_buf[0] << 24)
                            | ((uint32_t)s_rx_crc_buf[1] << 16)
                            | ((uint32_t)s_rx_crc_buf[2] << 8)
                            |  (uint32_t)s_rx_crc_buf[3];
            pkt.valid       = 1;

            if (s_rx_current_hdr.payload_len > 0) {
                memcpy(pkt.payload,
                       s_rx_payload,
                       s_rx_current_hdr.payload_len);
            }

            dispatch_packet(&pkt);
            reset_rx_state();
        }
        break;

    /* ------------------------------------------------------------------ */
    default:
        reset_rx_state();
        break;
    }
}

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

eth_proto_result_t ETH_PROTO_Init(eth_proto_rx_handler_t rx_handler)
{
    if (rx_handler == NULL) {
        return ETH_PROTO_ERR_NO_CONN;  /* Handler is mandatory */
    }

    s_rx_handler   = rx_handler;
    s_tx_seq       = 0;
    s_rx_pkt_count  = 0;
    s_crc_err_count = 0;

    reset_rx_state();
    return ETH_PROTO_OK;
}

/* ---------------------------------------------------------------------- */

eth_proto_result_t ETH_PROTO_ProcessReceive(void)
{
    size_t bytes_recvd = 0;

    eth_drv_result_t drv_res = ETH_DRV_Receive(s_raw_rx_buf,
                                                sizeof(s_raw_rx_buf),
                                                &bytes_recvd);

    if (drv_res == ETH_DRV_ERR_TIMEOUT) {
        return ETH_PROTO_ERR_TIMEOUT;
    }

    if (drv_res == ETH_DRV_ERR_NO_CONN || drv_res != ETH_DRV_OK) {
        reset_rx_state();
        return ETH_PROTO_ERR_NO_CONN;
    }

    /* Feed received bytes one at a time through the state machine */
    for (size_t i = 0; i < bytes_recvd; i++) {
        process_byte(s_raw_rx_buf[i]);
    }

    return ETH_PROTO_OK;
}

/* ---------------------------------------------------------------------- */

eth_proto_result_t ETH_PROTO_SendHeartbeat(uint32_t uptime_ms,
                                            uint8_t  system_state)
{
    haley_pay_heartbeat_t hb;
    /* Serialize to big-endian manually for cross-platform safety */
    uint8_t payload[5];
    payload[0] = (uint8_t)(uptime_ms >> 24);
    payload[1] = (uint8_t)(uptime_ms >> 16);
    payload[2] = (uint8_t)(uptime_ms >> 8);
    payload[3] = (uint8_t)(uptime_ms & 0xFF);
    payload[4] = system_state;

    uint8_t pkt_buf[HALEY_OVERHEAD_SIZE + sizeof(payload)];
    size_t pkt_len = ETH_PKT_Build(pkt_buf, sizeof(pkt_buf),
                                   HALEY_PKT_HEARTBEAT,
                                   next_tx_seq(),
                                   payload, sizeof(payload));
    if (pkt_len == 0) {
        return ETH_PROTO_ERR_SEND;
    }

    return (ETH_DRV_SendPacket(pkt_buf, pkt_len) == ETH_DRV_OK)
           ? ETH_PROTO_OK
           : ETH_PROTO_ERR_SEND;
}

/* ---------------------------------------------------------------------- */

eth_proto_result_t ETH_PROTO_SendACK(uint16_t acked_seq)
{
    uint8_t payload[2];
    payload[0] = (uint8_t)(acked_seq >> 8);
    payload[1] = (uint8_t)(acked_seq & 0xFF);

    uint8_t pkt_buf[HALEY_OVERHEAD_SIZE + sizeof(payload)];
    size_t pkt_len = ETH_PKT_Build(pkt_buf, sizeof(pkt_buf),
                                   HALEY_PKT_ACK,
                                   next_tx_seq(),
                                   payload, sizeof(payload));
    if (pkt_len == 0) {
        return ETH_PROTO_ERR_SEND;
    }

    return (ETH_DRV_SendPacket(pkt_buf, pkt_len) == ETH_DRV_OK)
           ? ETH_PROTO_OK
           : ETH_PROTO_ERR_SEND;
}

/* ---------------------------------------------------------------------- */

eth_proto_result_t ETH_PROTO_SendNACK(uint16_t    rejected_seq,
                                       haley_err_t error_code)
{
    uint8_t pkt_buf[HALEY_OVERHEAD_SIZE + 3];
    size_t pkt_len = ETH_PKT_BuildNACK(pkt_buf, sizeof(pkt_buf),
                                        next_tx_seq(),
                                        rejected_seq,
                                        error_code);
    if (pkt_len == 0) {
        return ETH_PROTO_ERR_SEND;
    }

    return (ETH_DRV_SendPacket(pkt_buf, pkt_len) == ETH_DRV_OK)
           ? ETH_PROTO_OK
           : ETH_PROTO_ERR_SEND;
}

/* ---------------------------------------------------------------------- */

eth_proto_result_t ETH_PROTO_SendTelemetrySensor(
    const haley_pay_telem_sensor_t *telem)
{
    if (telem == NULL) {
        return ETH_PROTO_ERR_SEND;
    }

    /* Serialize big-endian */
    uint8_t payload[sizeof(haley_pay_telem_sensor_t)];
    payload[0]  = (uint8_t)(telem->temperature_cdeg >> 8);
    payload[1]  = (uint8_t)(telem->temperature_cdeg & 0xFF);
    payload[2]  = (uint8_t)(telem->humidity_cpct >> 8);
    payload[3]  = (uint8_t)(telem->humidity_cpct & 0xFF);
    payload[4]  = (uint8_t)(telem->pressure_pa >> 24);
    payload[5]  = (uint8_t)(telem->pressure_pa >> 16);
    payload[6]  = (uint8_t)(telem->pressure_pa >> 8);
    payload[7]  = (uint8_t)(telem->pressure_pa & 0xFF);
    payload[8]  = (uint8_t)(telem->timestamp_ms >> 24);
    payload[9]  = (uint8_t)(telem->timestamp_ms >> 16);
    payload[10] = (uint8_t)(telem->timestamp_ms >> 8);
    payload[11] = (uint8_t)(telem->timestamp_ms & 0xFF);

    uint8_t pkt_buf[HALEY_OVERHEAD_SIZE + sizeof(payload)];
    size_t pkt_len = ETH_PKT_Build(pkt_buf, sizeof(pkt_buf),
                                   HALEY_PKT_TELEM_SENSOR,
                                   next_tx_seq(),
                                   payload, sizeof(payload));
    if (pkt_len == 0) {
        return ETH_PROTO_ERR_SEND;
    }

    return (ETH_DRV_SendPacket(pkt_buf, pkt_len) == ETH_DRV_OK)
           ? ETH_PROTO_OK
           : ETH_PROTO_ERR_SEND;
}

/* ---------------------------------------------------------------------- */

eth_proto_result_t ETH_PROTO_SendSystemStatus(
    const haley_pay_status_sys_t *status)
{
    if (status == NULL) {
        return ETH_PROTO_ERR_SEND;
    }

    uint8_t payload[sizeof(haley_pay_status_sys_t)];
    payload[0] = status->system_state;
    payload[1] = status->fault_flags;
    payload[2] = (uint8_t)(status->free_heap_words >> 8);
    payload[3] = (uint8_t)(status->free_heap_words & 0xFF);
    payload[4] = (uint8_t)(status->uptime_ms >> 24);
    payload[5] = (uint8_t)(status->uptime_ms >> 16);
    payload[6] = (uint8_t)(status->uptime_ms >> 8);
    payload[7] = (uint8_t)(status->uptime_ms & 0xFF);

    uint8_t pkt_buf[HALEY_OVERHEAD_SIZE + sizeof(payload)];
    size_t pkt_len = ETH_PKT_Build(pkt_buf, sizeof(pkt_buf),
                                   HALEY_PKT_STATUS_SYSTEM,
                                   next_tx_seq(),
                                   payload, sizeof(payload));
    if (pkt_len == 0) {
        return ETH_PROTO_ERR_SEND;
    }

    return (ETH_DRV_SendPacket(pkt_buf, pkt_len) == ETH_DRV_OK)
           ? ETH_PROTO_OK
           : ETH_PROTO_ERR_SEND;
}

/* ---------------------------------------------------------------------- */

eth_proto_result_t ETH_PROTO_SendAudioData(const uint8_t *audio_data,
                                            uint16_t       data_len)
{
    if (audio_data == NULL || data_len == 0 || data_len > HALEY_MAX_PAYLOAD) {
        return ETH_PROTO_ERR_SEND;
    }

    uint8_t pkt_buf[HALEY_OVERHEAD_SIZE + HALEY_MAX_PAYLOAD];
    size_t pkt_len = ETH_PKT_Build(pkt_buf, sizeof(pkt_buf),
                                   HALEY_PKT_AUDIO_DATA,
                                   next_tx_seq(),
                                   audio_data, data_len);
    if (pkt_len == 0) {
        return ETH_PROTO_ERR_SEND;
    }

    return (ETH_DRV_SendPacket(pkt_buf, pkt_len) == ETH_DRV_OK)
           ? ETH_PROTO_OK
           : ETH_PROTO_ERR_SEND;
}

/* ---------------------------------------------------------------------- */

eth_proto_result_t ETH_PROTO_SendFaultNotification(uint8_t  fault_flags,
                                                    uint32_t uptime_ms)
{
    uint8_t payload[5];
    payload[0] = fault_flags;
    payload[1] = (uint8_t)(uptime_ms >> 24);
    payload[2] = (uint8_t)(uptime_ms >> 16);
    payload[3] = (uint8_t)(uptime_ms >> 8);
    payload[4] = (uint8_t)(uptime_ms & 0xFF);

    uint8_t pkt_buf[HALEY_OVERHEAD_SIZE + sizeof(payload)];
    size_t pkt_len = ETH_PKT_Build(pkt_buf, sizeof(pkt_buf),
                                   HALEY_PKT_FAULT_NOTIF,
                                   next_tx_seq(),
                                   payload, sizeof(payload));
    if (pkt_len == 0) {
        return ETH_PROTO_ERR_SEND;
    }

    return (ETH_DRV_SendPacket(pkt_buf, pkt_len) == ETH_DRV_OK)
           ? ETH_PROTO_OK
           : ETH_PROTO_ERR_SEND;
}

/* ---------------------------------------------------------------------- */

uint16_t ETH_PROTO_GetTxSeq(void)      { return s_tx_seq; }
uint32_t ETH_PROTO_GetRxPacketCount(void) { return s_rx_pkt_count; }
uint32_t ETH_PROTO_GetCRCErrorCount(void) { return s_crc_err_count; }
