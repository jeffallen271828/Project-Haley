/*
 * ethernet_packets.h
 *
 * Defines the Haley binary packet wire format for STM32 <-> AI host communication.
 *
 * Wire format (all multi-byte fields are BIG-ENDIAN on the wire):
 *
 *   Offset  Size  Field
 *   ------  ----  -----
 *    0       1    Magic byte 0  (0x48 = 'H')
 *    1       1    Magic byte 1  (0x4C = 'L')
 *    2       1    Protocol version
 *    3       1    Packet type   (haley_pkt_type_t)
 *    4       2    Sequence number (big-endian)
 *    6       2    Payload length  (big-endian, 0..HALEY_MAX_PAYLOAD)
 *    8       4    Header CRC32    (CRC32 of bytes [0..7])
 *   12       N    Payload data    (N = payload_len)
 *   12+N     4    Payload CRC32   (CRC32 of payload bytes only; omitted if len=0)
 *
 * Total overhead: 12 bytes header + 4 bytes payload CRC = 16 bytes minimum.
 * Maximum packet size: HALEY_HEADER_SIZE + HALEY_MAX_PAYLOAD + HALEY_FOOTER_SIZE
 *
 * Layer: Ethernet / Service Layer
 * Dependencies: stdint.h, ethernet_crc.h
 *
 *  Created on: May 12, 2026
 *      Author: jeffreya181
 */

#ifndef INC_ETHERNET_ETHERNET_PACKETS_H_
#define INC_ETHERNET_ETHERNET_PACKETS_H_

#include <stdint.h>
#include <stddef.h>
#include "ethernet/ethernet_crc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------
 * Protocol Constants
 * ----------------------------------------------------------------------- */

#define HALEY_MAGIC_0             0x48U   /**< 'H' */
#define HALEY_MAGIC_1             0x4CU   /**< 'L' */
#define HALEY_PROTOCOL_VERSION    0x01U   /**< Current protocol version */

#define HALEY_HEADER_SIZE         12U     /**< Fixed header length in bytes */
#define HALEY_FOOTER_SIZE         4U      /**< Payload CRC size in bytes */
#define HALEY_OVERHEAD_SIZE       (HALEY_HEADER_SIZE + HALEY_FOOTER_SIZE)

#define HALEY_MAX_PAYLOAD         1024U   /**< Maximum payload bytes per packet */
#define HALEY_MAX_PACKET_SIZE     (HALEY_HEADER_SIZE + HALEY_MAX_PAYLOAD + HALEY_FOOTER_SIZE)

/** Offset of the header CRC field within the raw header buffer */
#define HALEY_HDR_CRC_OFFSET      8U
/** Number of header bytes covered by header CRC (bytes 0..7) */
#define HALEY_HDR_CRC_COVER       8U

/* -----------------------------------------------------------------------
 * Packet Type Identifiers
 * ----------------------------------------------------------------------- */

/**
 * @brief Haley packet type enumeration.
 *
 * Range 0x01..0x0F: Control / handshake
 * Range 0x10..0x1F: Hardware commands (AI host -> STM32)
 * Range 0x20..0x2F: Telemetry (STM32 -> AI host)
 * Range 0x30..0x3F: Audio stream
 * Range 0x40..0x4F: System status
 * Range 0x50..0x5F: Fault / error notifications
 */
typedef enum {
    /* --- Control / Handshake ------------------------------------------ */
    HALEY_PKT_HEARTBEAT        = 0x01,   /**< Keep-alive ping                  */
    HALEY_PKT_HEARTBEAT_ACK    = 0x02,   /**< Keep-alive response               */
    HALEY_PKT_ACK              = 0x03,   /**< Generic acknowledgement           */
    HALEY_PKT_NACK             = 0x04,   /**< Negative acknowledgement / error  */
    HALEY_PKT_HELLO            = 0x05,   /**< Initial handshake                 */
    HALEY_PKT_HELLO_ACK        = 0x06,   /**< Handshake response                */

    /* --- Hardware Commands (AI host -> STM32) -------------------------- */
    HALEY_PKT_CMD_LED          = 0x10,   /**< LED state / animation command     */
    HALEY_PKT_CMD_GPIO         = 0x11,   /**< Generic GPIO output command       */
    HALEY_PKT_CMD_RELAY        = 0x12,   /**< Relay / actuator command          */
    HALEY_PKT_CMD_AUDIO_CTL    = 0x13,   /**< Audio capture start/stop          */
    HALEY_PKT_CMD_STATE        = 0x14,   /**< Set system or mood state          */
    HALEY_PKT_CMD_RESET        = 0x15,   /**< Request MCU soft reset            */

    /* --- Telemetry (STM32 -> AI host) ---------------------------------- */
    HALEY_PKT_TELEM_SENSOR     = 0x20,   /**< Environmental sensor data         */
    HALEY_PKT_TELEM_SYSTEM     = 0x21,   /**< System health / task stats        */
    HALEY_PKT_TELEM_AUDIO_META = 0x22,   /**< Audio stream metadata             */

    /* --- Audio Stream -------------------------------------------------- */
    HALEY_PKT_AUDIO_DATA       = 0x30,   /**< PCM audio frame                   */
    HALEY_PKT_AUDIO_START      = 0x31,   /**< Audio stream start notification   */
    HALEY_PKT_AUDIO_STOP       = 0x32,   /**< Audio stream stop notification    */

    /* --- Status -------------------------------------------------------- */
    HALEY_PKT_STATUS_SYSTEM    = 0x40,   /**< General system status             */
    HALEY_PKT_STATUS_BOOT      = 0x41,   /**< Boot complete notification        */

    /* --- Faults -------------------------------------------------------- */
    HALEY_PKT_FAULT_NOTIF      = 0x50,   /**< Fault / error notification        */
    HALEY_PKT_FAULT_WATCHDOG   = 0x51,   /**< Watchdog near-miss notification   */

    HALEY_PKT_UNKNOWN          = 0xFF,
} haley_pkt_type_t;

/* -----------------------------------------------------------------------
 * Packet Error Codes (used in NACK payload)
 * ----------------------------------------------------------------------- */

typedef enum {
    HALEY_ERR_NONE             = 0x00,
    HALEY_ERR_BAD_MAGIC        = 0x01,
    HALEY_ERR_BAD_VERSION      = 0x02,
    HALEY_ERR_BAD_HDR_CRC      = 0x03,
    HALEY_ERR_BAD_PAY_CRC      = 0x04,
    HALEY_ERR_BAD_LENGTH       = 0x05,
    HALEY_ERR_UNKNOWN_TYPE     = 0x06,
    HALEY_ERR_QUEUE_FULL       = 0x07,
    HALEY_ERR_TIMEOUT          = 0x08,
} haley_err_t;

/* -----------------------------------------------------------------------
 * Raw Header Layout (packed, for wire serialization)
 * ----------------------------------------------------------------------- */

/**
 * @brief Packed representation of the 12-byte Haley packet header.
 *
 * Do NOT read multi-byte fields directly from this struct on little-endian
 * systems without byte-swapping — the wire format is big-endian.
 * Use the ETH_PKT_Build*() and ETH_PKT_Parse*() helpers instead.
 */
typedef struct __attribute__((packed)) {
    uint8_t  magic[2];         /**< Must be {HALEY_MAGIC_0, HALEY_MAGIC_1}  */
    uint8_t  version;          /**< Protocol version                         */
    uint8_t  type;             /**< Packet type (haley_pkt_type_t)           */
    uint8_t  sequence[2];      /**< Sequence number, big-endian              */
    uint8_t  payload_len[2];   /**< Payload length in bytes, big-endian      */
    uint8_t  header_crc[4];    /**< CRC32 of bytes [0..7], big-endian        */
} haley_raw_header_t;

/**
 * @brief Decoded (host-byte-order) representation of a received packet header.
 */
typedef struct {
    uint8_t          version;
    haley_pkt_type_t type;
    uint16_t         sequence;
    uint16_t         payload_len;
    uint32_t         header_crc;
} haley_pkt_header_t;

/**
 * @brief Complete decoded packet, ready for use by protocol/task layer.
 */
typedef struct {
    haley_pkt_header_t header;
    uint8_t            payload[HALEY_MAX_PAYLOAD];
    uint32_t           payload_crc;
    uint8_t            valid;    /**< 1 = CRC checks passed; 0 = corrupted  */
} haley_packet_t;

/* -----------------------------------------------------------------------
 * Common Payload Structs
 * ----------------------------------------------------------------------- */

/** Heartbeat payload (sent both ways) */
typedef struct __attribute__((packed)) {
    uint32_t timestamp_ms;     /**< Sender uptime in milliseconds */
    uint8_t  system_state;     /**< High-level system state byte  */
} haley_pay_heartbeat_t;

/** NACK payload */
typedef struct __attribute__((packed)) {
    uint16_t rejected_seq;     /**< Sequence number of the rejected packet  */
    uint8_t  error_code;       /**< haley_err_t error code                  */
} haley_pay_nack_t;

/** LED command payload */
typedef struct __attribute__((packed)) {
    uint8_t  led_id;           /**< LED identifier (0 = LD1, 1 = LD2, etc.) */
    uint8_t  mode;             /**< 0=off, 1=on, 2=blink, 3=pulse           */
    uint8_t  r, g, b;         /**< RGB if applicable                        */
    uint16_t period_ms;        /**< Blink/pulse period                       */
} haley_pay_cmd_led_t;

/** Audio control command payload */
typedef struct __attribute__((packed)) {
    uint8_t  action;           /**< 0=stop, 1=start                         */
    uint16_t sample_rate;      /**< Audio sample rate in Hz                  */
    uint8_t  bit_depth;        /**< Bits per sample                          */
    uint8_t  channels;         /**< 1=mono, 2=stereo                         */
} haley_pay_audio_ctl_t;

/** Sensor telemetry payload */
typedef struct __attribute__((packed)) {
    int16_t  temperature_cdeg; /**< Temperature in centi-degrees Celsius     */
    uint16_t humidity_cpct;    /**< Relative humidity in centi-percent        */
    uint32_t pressure_pa;      /**< Atmospheric pressure in Pascals           */
    uint32_t timestamp_ms;     /**< MCU uptime at measurement time            */
} haley_pay_telem_sensor_t;

/** System status payload */
typedef struct __attribute__((packed)) {
    uint8_t  system_state;     /**< Current operational state                */
    uint8_t  fault_flags;      /**< Bitmask of active faults                 */
    uint16_t free_heap_words;  /**< FreeRTOS free heap in 4-byte words        */
    uint32_t uptime_ms;        /**< MCU uptime in milliseconds                */
} haley_pay_status_sys_t;

/* -----------------------------------------------------------------------
 * Build / Parse API
 * ----------------------------------------------------------------------- */

/**
 * @brief  Serialize a Haley packet into a flat byte buffer ready for TCP send.
 *
 * @param  out_buf      Destination buffer. Must be at least
 *                      HALEY_HEADER_SIZE + payload_len + HALEY_FOOTER_SIZE bytes.
 * @param  buf_size     Size of out_buf in bytes.
 * @param  type         Packet type.
 * @param  seq          Sequence number (caller manages counter).
 * @param  payload      Pointer to payload data (may be NULL if payload_len == 0).
 * @param  payload_len  Number of payload bytes.
 * @return Total bytes written to out_buf, or 0 on error.
 */
size_t ETH_PKT_Build(uint8_t       *out_buf,
                     size_t         buf_size,
                     haley_pkt_type_t type,
                     uint16_t       seq,
                     const uint8_t *payload,
                     uint16_t       payload_len);

/**
 * @brief  Parse and validate a raw header from a receive buffer.
 *
 * @param  raw_buf   Pointer to at least HALEY_HEADER_SIZE bytes.
 * @param  out_hdr   Destination for decoded header fields.
 * @return HALEY_ERR_NONE if valid, or a haley_err_t error code.
 */
haley_err_t ETH_PKT_ParseHeader(const uint8_t     *raw_buf,
                                 haley_pkt_header_t *out_hdr);

/**
 * @brief  Validate the payload CRC from a receive buffer.
 *
 * @param  payload      Pointer to payload bytes.
 * @param  payload_len  Number of payload bytes.
 * @param  crc_bytes    Pointer to the 4 CRC bytes that follow the payload.
 * @return HALEY_ERR_NONE if valid, HALEY_ERR_BAD_PAY_CRC if mismatch.
 */
haley_err_t ETH_PKT_ValidatePayloadCRC(const uint8_t *payload,
                                        uint16_t       payload_len,
                                        const uint8_t *crc_bytes);

/**
 * @brief  Build a zero-payload control packet (heartbeat, ACK, etc.).
 *
 * @param  out_buf   Destination buffer (must be >= HALEY_OVERHEAD_SIZE bytes).
 * @param  buf_size  Size of out_buf.
 * @param  type      Packet type.
 * @param  seq       Sequence number.
 * @return Total bytes written, or 0 on error.
 */
size_t ETH_PKT_BuildControl(uint8_t          *out_buf,
                             size_t            buf_size,
                             haley_pkt_type_t  type,
                             uint16_t          seq);

/**
 * @brief  Build a NACK packet referencing a rejected sequence number.
 *
 * @param  out_buf       Destination buffer.
 * @param  buf_size      Size of out_buf.
 * @param  seq           Our current sequence number.
 * @param  rejected_seq  Sequence number of the packet being rejected.
 * @param  error_code    haley_err_t reason code.
 * @return Total bytes written, or 0 on error.
 */
size_t ETH_PKT_BuildNACK(uint8_t    *out_buf,
                          size_t      buf_size,
                          uint16_t    seq,
                          uint16_t    rejected_seq,
                          haley_err_t error_code);

#ifdef __cplusplus
}
#endif

#endif /* INC_ETHERNET_ETHERNET_PACKETS_H_ */
