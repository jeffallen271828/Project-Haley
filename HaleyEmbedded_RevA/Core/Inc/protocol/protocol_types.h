/*
 * protocol_types.h
 *
 * Core type definitions for the Haley binary protocol.
 *
 * Wire-format compatibility:
 *   This file is the canonical embedded-side definition that mirrors
 *   HaleyLinux_RevA/services/stm32_interface/stm32_packets.py.
 *   All multi-byte wire fields are LITTLE-ENDIAN (STM32H7 native order).
 *
 * Wire frame layout:
 *   Offset  Size  Field
 *   ------  ----  -----
 *    0       2    Magic            (0x4841, little-endian uint16 → bytes [0x41,0x48])
 *    2       1    Protocol version (uint8)
 *    3       1    Message type     (haley_msg_type_t, uint8)
 *    4       2    Flags            (haley_pkt_flags_t, uint16, little-endian)
 *    6       4    Sequence number  (uint32, little-endian)
 *   10       4    Payload length   (uint32, little-endian)
 *   14       4    CRC32            (uint32, little-endian)
 *   18      [N]   Payload          (N = payload_length bytes)
 *
 * Total header size: HALEY_PROTO_HEADER_SIZE (18 bytes).
 * CRC32 computation: IEEE 802.3 reflected polynomial over
 *   header bytes [0..13] (with crc32 field cleared to 0) + full payload.
 *
 * Payload encoding:
 *   Text packets (commands, telemetry, version): UTF-8 "key=value\n" pairs.
 *   Binary packets (audio data): raw bytes.
 *   Empty packets (heartbeat, ACK): zero-length payload.
 *
 * Layer: Protocol / Service Layer
 * Dependencies: <stdint.h>, <stddef.h>
 *
 *  Created on: May 14, 2026
 *      Author: jeffreya181
 */

#ifndef INC_PROTOCOL_PROTOCOL_TYPES_H_
#define INC_PROTOCOL_PROTOCOL_TYPES_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------
 * Wire-format constants (defined here so all modules can use them
 * without pulling in protocol_constants.h in every file)
 * ----------------------------------------------------------------------- */

/** Magic value as a uint16_t (little-endian: wire bytes [0x41, 0x48]) */
#define HALEY_PROTO_MAGIC              0x4841U

/** Current protocol version */
#define HALEY_PROTO_VERSION            1U

/** Fixed header size in bytes */
#define HALEY_PROTO_HEADER_SIZE        18U

/** Byte offset of the CRC32 field within the header */
#define HALEY_PROTO_CRC_OFFSET         14U

/** Number of header bytes covered by the CRC32 (bytes 0..13) */
#define HALEY_PROTO_CRC_COVER          14U

/** Maximum allowed payload bytes per packet */
#define HALEY_PROTO_MAX_PAYLOAD        4096U

/** Maximum total packet size (header + max payload) */
#define HALEY_PROTO_MAX_PACKET_SIZE    (HALEY_PROTO_HEADER_SIZE + HALEY_PROTO_MAX_PAYLOAD)

/* -----------------------------------------------------------------------
 * Message Types
 *
 * Must match Python: class PacketType(IntEnum) in stm32_packets.py
 * ----------------------------------------------------------------------- */

/**
 * @brief Haley protocol message type identifiers.
 *
 * Range 1..7 are defined for the current protocol version.
 * Values 8..255 are reserved for future expansion.
 */
typedef enum {
    HALEY_MSG_HEARTBEAT          = 1,  /**< Keep-alive ping (bidirectional)     */
    HALEY_MSG_COMMAND            = 2,  /**< Hardware command (Linux → STM32)    */
    HALEY_MSG_ACK                = 3,  /**< Generic acknowledgement             */
    HALEY_MSG_TELEMETRY          = 4,  /**< Sensor/system telemetry (STM32 → Linux) */
    HALEY_MSG_FAULT              = 5,  /**< Fault / error notification           */
    HALEY_MSG_AUDIO_META         = 6,  /**< Audio stream metadata               */
    HALEY_MSG_VERSION_NEGOTIATION= 7,  /**< Protocol version handshake          */
    HALEY_MSG_STATE              = 8,  /**< Operational state update            */
    HALEY_MSG_UNKNOWN            = 0xFF,
} haley_msg_type_t;

/* -----------------------------------------------------------------------
 * Packet Flags (bitmask)
 *
 * Must match Python: class PacketFlags(IntFlag) in stm32_packets.py
 * ----------------------------------------------------------------------- */

/**
 * @brief Bitmask flags carried in every packet header.
 *
 * Flags are OR-able and can be combined freely.
 */
typedef enum {
    HALEY_FLAG_NONE              = 0x0000U, /**< No flags                        */
    HALEY_FLAG_REQUEST           = 0x0001U, /**< Packet is a request             */
    HALEY_FLAG_RESPONSE          = 0x0002U, /**< Packet is a response            */
    HALEY_FLAG_ACK_REQUIRED      = 0x0004U, /**< Sender expects an ACK           */
    HALEY_FLAG_PRIORITY_HIGH     = 0x0008U, /**< High-priority delivery hint     */
    HALEY_FLAG_EMERGENCY         = 0x0010U, /**< Emergency / safety-critical msg */
} haley_pkt_flags_t;

/* -----------------------------------------------------------------------
 * Protocol-level error codes
 * ----------------------------------------------------------------------- */

/**
 * @brief Return codes for protocol-layer operations.
 *
 * Negative values indicate errors; zero indicates success.
 */
typedef enum {
    HALEY_PROTO_OK               =  0,
    HALEY_PROTO_ERR_BAD_MAGIC    = -1,  /**< Magic bytes do not match           */
    HALEY_PROTO_ERR_BAD_VERSION  = -2,  /**< Unsupported protocol version       */
    HALEY_PROTO_ERR_BAD_CRC      = -3,  /**< CRC32 validation failed            */
    HALEY_PROTO_ERR_BAD_LENGTH   = -4,  /**< Payload length out of range        */
    HALEY_PROTO_ERR_UNKNOWN_TYPE = -5,  /**< Unrecognised message type          */
    HALEY_PROTO_ERR_BUF_SMALL    = -6,  /**< Output buffer too small            */
    HALEY_PROTO_ERR_NULL_PTR     = -7,  /**< NULL argument where not allowed    */
    HALEY_PROTO_ERR_OVERFLOW     = -8,  /**< Receive buffer overflowed          */
    HALEY_PROTO_ERR_TIMEOUT      = -9,  /**< Operation timed out                */
    HALEY_PROTO_ERR_NO_CONN      = -10, /**< No active transport connection     */
    HALEY_PROTO_ERR_KV_NOT_FOUND = -11, /**< Key not found in KV payload        */
    HALEY_PROTO_ERR_KV_TRUNCATED = -12, /**< Value buffer too small for result  */
} haley_proto_err_t;

/* -----------------------------------------------------------------------
 * Packed header structure (matches wire layout exactly)
 *
 * Do NOT access multi-byte fields directly on big-endian systems.
 * Use haley_hdr_t (host-order) instead after calling PROTO_ParseHeader().
 * ----------------------------------------------------------------------- */

/**
 * @brief Packed representation of the 18-byte on-wire packet header.
 *
 * This struct can be cast directly over a receive buffer to extract fields,
 * provided host byte order is little-endian (STM32H7 Cortex-M7 native).
 * All multi-byte fields are stored in little-endian order.
 */
typedef struct __attribute__((packed)) {
    uint16_t magic;          /**< Must equal HALEY_PROTO_MAGIC (0x4841)       */
    uint8_t  version;        /**< Protocol version (HALEY_PROTO_VERSION)      */
    uint8_t  msg_type;       /**< Message type (haley_msg_type_t)             */
    uint16_t flags;          /**< Flags bitmask (haley_pkt_flags_t)           */
    uint32_t sequence;       /**< Monotonic sequence number                   */
    uint32_t payload_length; /**< Number of payload bytes that follow         */
    uint32_t crc32;          /**< CRC32 over header[0:14]+payload (LE uint32) */
} haley_raw_hdr_t;

/**
 * @brief Host-byte-order decoded packet header.
 *
 * Populated by PROTO_ParseHeader(). All fields are in native host order
 * and can be compared / used directly in C logic.
 */
typedef struct {
    uint16_t         magic;
    uint8_t          version;
    haley_msg_type_t msg_type;
    haley_pkt_flags_t flags;
    uint32_t         sequence;
    uint32_t         payload_length;
    uint32_t         crc32;
} haley_hdr_t;

/**
 * @brief Complete decoded packet ready for dispatch.
 *
 * Payload is a flat byte array. For key-value text packets, call
 * PROTO_KV_Find() to look up individual fields.
 */
typedef struct {
    haley_hdr_t  header;                           /**< Decoded header            */
    uint8_t      payload[HALEY_PROTO_MAX_PAYLOAD]; /**< Raw payload bytes         */
    uint8_t      valid;                            /**< 1 = CRC OK; 0 = corrupted */
} haley_packet_t;

/* -----------------------------------------------------------------------
 * Known telemetry field names (as C string constants)
 *
 * These match the keys emitted / consumed by stm32_protocol.py.
 * ----------------------------------------------------------------------- */

#define HALEY_KV_UPTIME_MS          "uptime_ms"
#define HALEY_KV_TEMPERATURE_C      "temperature_c"
#define HALEY_KV_SUPPLY_VOLTAGE_V   "supply_voltage_v"
#define HALEY_KV_FAULT_CODE         "fault_code"
#define HALEY_KV_MIC_ACTIVE         "mic_active"
#define HALEY_KV_NETWORK_OK         "network_ok"
#define HALEY_KV_AUDIO_OVERRUN      "audio_overrun"
#define HALEY_KV_AUDIO_UNDERRUN     "audio_underrun"

/* Version-negotiation keys */
#define HALEY_KV_NAME               "name"
#define HALEY_KV_PROTOCOL_VERSION   "protocol_version"
#define HALEY_KV_AUDIO_SAMPLE_RATE  "audio_sample_rate_hz"
#define HALEY_KV_AUDIO_CHANNELS     "audio_channels"

/* Command key */
#define HALEY_KV_COMMAND            "command"
#define HALEY_KV_FAULT              "fault"

/* -----------------------------------------------------------------------
 * Convenience macros
 * ----------------------------------------------------------------------- */

/** Return 1 if a flags bitmask contains the given flag */
#define HALEY_FLAG_IS_SET(flags, flag)   (((flags) & (flag)) != 0U)

/** Combine two flags */
#define HALEY_FLAGS(a, b)   ((uint16_t)((a) | (b)))

#ifdef __cplusplus
}
#endif

#endif /* INC_PROTOCOL_PROTOCOL_TYPES_H_ */
