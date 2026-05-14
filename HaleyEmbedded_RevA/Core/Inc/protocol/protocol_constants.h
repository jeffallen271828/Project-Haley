/*
 * protocol_constants.h
 *
 * Numeric constants for the Haley binary protocol.
 *
 * These values govern timing, buffer sizing, retry behaviour, and
 * key-value payload limits. They mirror the defaults found in
 * HaleyLinux_RevA/services/stm32_interface/stm32_config.py (STM32Config).
 *
 * Layer: Protocol / Service Layer
 * Dependencies: protocol_types.h
 *
 *  Created on: May 14, 2026
 *      Author: jeffreya181
 */

#ifndef INC_PROTOCOL_PROTOCOL_CONSTANTS_H_
#define INC_PROTOCOL_PROTOCOL_CONSTANTS_H_

#include "protocol/protocol_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------
 * Identity
 * ----------------------------------------------------------------------- */

/** Human-readable name transmitted during version negotiation */
#define HALEY_PROTO_DEVICE_NAME        "haley-stm32-controller"

/* -----------------------------------------------------------------------
 * Buffer / packet sizing
 * ----------------------------------------------------------------------- */

/**
 * @brief Maximum recommended MTU payload size (bytes).
 *
 * Mirrors STM32Config.mtu_payload_bytes = 1400.
 * Packets larger than this may be fragmented at the transport layer.
 */
#define HALEY_PROTO_MTU_PAYLOAD        1400U

/**
 * @brief Maximum key-value text payload (bytes).
 *
 * Text payloads (commands, telemetry, version negotiation) must fit within
 * this limit. Chosen to stay well inside the MTU.
 */
#define HALEY_PROTO_MAX_KV_PAYLOAD     512U

/**
 * @brief Maximum number of key-value pairs that can be held in one payload.
 */
#define HALEY_PROTO_MAX_KV_PAIRS       32U

/**
 * @brief Maximum length of a single key or value string (excluding NUL).
 */
#define HALEY_PROTO_MAX_KV_ITEM_LEN    127U

/**
 * @brief Scratch transmit buffer size (must be >= HALEY_PROTO_MAX_PACKET_SIZE).
 */
#define HALEY_PROTO_TX_BUF_SIZE        (HALEY_PROTO_HEADER_SIZE + HALEY_PROTO_MTU_PAYLOAD)

/**
 * @brief Internal receive reassembly buffer size.
 */
#define HALEY_PROTO_RX_BUF_SIZE        (HALEY_PROTO_HEADER_SIZE + HALEY_PROTO_MAX_PAYLOAD)

/**
 * @brief Maximum queue depth for outbound packets.
 *
 * Mirrors STM32Config.max_queue_depth = 64.
 */
#define HALEY_PROTO_MAX_QUEUE_DEPTH    64U

/* -----------------------------------------------------------------------
 * Timing (milliseconds unless otherwise noted)
 * ----------------------------------------------------------------------- */

/**
 * @brief Heartbeat transmission interval (ms).
 *
 * Mirrors STM32Config.heartbeat_interval_s = 2.0 s.
 */
#define HALEY_PROTO_HEARTBEAT_INTERVAL_MS   2000U

/**
 * @brief Heartbeat timeout — mark link as degraded if no heartbeat
 *        received within this window (ms).
 *
 * Mirrors STM32Config.heartbeat_timeout_s = 6.0 s.
 */
#define HALEY_PROTO_HEARTBEAT_TIMEOUT_MS    6000U

/**
 * @brief Reconnect attempt delay after a link drop (ms).
 *
 * Mirrors STM32Config.reconnect_delay_s = 2.0 s.
 */
#define HALEY_PROTO_RECONNECT_DELAY_MS      2000U

/**
 * @brief Maximum reconnect back-off delay (ms).
 *
 * Mirrors STM32Config.max_reconnect_delay_s = 15.0 s.
 */
#define HALEY_PROTO_RECONNECT_MAX_DELAY_MS  15000U

/**
 * @brief Telemetry polling interval (ms).
 *
 * Mirrors STM32Config.telemetry_poll_interval_s = 5.0 s.
 */
#define HALEY_PROTO_TELEMETRY_INTERVAL_MS   5000U

/**
 * @brief Connection setup timeout (ms).
 *
 * Mirrors STM32Config.connect_timeout_s = 5.0 s.
 */
#define HALEY_PROTO_CONNECT_TIMEOUT_MS      5000U

/**
 * @brief Per-read socket timeout (ms).
 *
 * Mirrors STM32Config.read_timeout_s = 1.0 s.
 */
#define HALEY_PROTO_READ_TIMEOUT_MS         1000U

/* -----------------------------------------------------------------------
 * Audio configuration defaults
 * ----------------------------------------------------------------------- */

/**
 * @brief Default audio sample rate (Hz).
 *
 * Mirrors STM32Config.audio_sample_rate_hz = 16000.
 * Must match SAI1 and I2S1 initialisation in main.c.
 */
#define HALEY_PROTO_AUDIO_SAMPLE_RATE_HZ    16000U

/**
 * @brief Default audio channel count.
 *
 * Mirrors STM32Config.audio_channels = 1 (mono).
 */
#define HALEY_PROTO_AUDIO_CHANNELS          1U

/**
 * @brief Default audio sample width (bytes per sample).
 *
 * Mirrors STM32Config.audio_sample_width_bytes = 2 (16-bit PCM).
 */
#define HALEY_PROTO_AUDIO_SAMPLE_WIDTH      2U

/**
 * @brief Default audio frame size in samples.
 *
 * Mirrors STM32Config.audio_frame_samples = 512.
 */
#define HALEY_PROTO_AUDIO_FRAME_SAMPLES     512U

/** Audio frame size in bytes (derived from above constants) */
#define HALEY_PROTO_AUDIO_FRAME_BYTES \
    (HALEY_PROTO_AUDIO_FRAME_SAMPLES * \
     HALEY_PROTO_AUDIO_CHANNELS      * \
     HALEY_PROTO_AUDIO_SAMPLE_WIDTH)

/* -----------------------------------------------------------------------
 * Protocol behaviour flags
 * ----------------------------------------------------------------------- */

/**
 * @brief Enable CRC32 validation on receive.
 *
 * Set to 0 during early bring-up to ignore CRC errors.
 * Must be 1 in production.
 * Mirrors STM32Config.use_crc32 = True.
 */
#define HALEY_PROTO_USE_CRC32               1U

/**
 * @brief Enable periodic heartbeat transmission.
 *
 * Mirrors STM32Config.enable_heartbeat = True.
 */
#define HALEY_PROTO_ENABLE_HEARTBEAT        1U

/**
 * @brief Enable automatic reconnect on link drop.
 *
 * Mirrors STM32Config.enable_reconnect = True.
 */
#define HALEY_PROTO_ENABLE_RECONNECT        1U

/**
 * @brief Enable packet-level logging (UART debug output).
 *
 * Mirrors STM32Config.log_packets = True.
 * Disable in performance-critical code paths.
 */
#define HALEY_PROTO_LOG_PACKETS             1U

/* -----------------------------------------------------------------------
 * Sequence number management
 * ----------------------------------------------------------------------- */

/**
 * @brief Sequence number wrap value (2^32, implicit in uint32_t arithmetic).
 *
 * Sequence numbers count from 1; value 0 is reserved for
 * "unsequenced" or "initial" packets.
 */
#define HALEY_PROTO_SEQ_INIT                1U
#define HALEY_PROTO_SEQ_RESERVED            0U

/* -----------------------------------------------------------------------
 * CRC32 polynomial (IEEE 802.3 reflected, same as zlib.crc32)
 * ----------------------------------------------------------------------- */

/** Reflected CRC32 polynomial used throughout the protocol */
#define HALEY_PROTO_CRC32_POLY              0xEDB88320UL

/** CRC32 initial value */
#define HALEY_PROTO_CRC32_INIT              0xFFFFFFFFUL

/** CRC32 final XOR value */
#define HALEY_PROTO_CRC32_XOR_OUT          0xFFFFFFFFUL

/* -----------------------------------------------------------------------
 * Known command strings (sent in HALEY_MSG_COMMAND payloads)
 *
 * These string literals are used as the value for the "command" key
 * in key-value encoded command payloads. Both sides must agree on naming.
 * ----------------------------------------------------------------------- */

#define HALEY_CMD_LED_SET               "led_set"
#define HALEY_CMD_LED_BLINK             "led_blink"
#define HALEY_CMD_GPIO_SET              "gpio_set"
#define HALEY_CMD_RELAY_SET             "relay_set"
#define HALEY_CMD_AUDIO_START           "audio_start"
#define HALEY_CMD_AUDIO_STOP            "audio_stop"
#define HALEY_CMD_STATE_SET             "state_set"
#define HALEY_CMD_RESET                 "reset"
#define HALEY_CMD_TELEMETRY_REQUEST     "telemetry_request"

/* -----------------------------------------------------------------------
 * Fault code bit positions
 * ----------------------------------------------------------------------- */

#define HALEY_FAULT_AUDIO_OVERRUN       (1U << 0)
#define HALEY_FAULT_AUDIO_UNDERRUN      (1U << 1)
#define HALEY_FAULT_NETWORK_LOST        (1U << 2)
#define HALEY_FAULT_WATCHDOG_NEAR       (1U << 3)
#define HALEY_FAULT_HEAP_LOW            (1U << 4)
#define HALEY_FAULT_SENSOR_ERR          (1U << 5)
#define HALEY_FAULT_UNKNOWN             (1U << 7)

#ifdef __cplusplus
}
#endif

#endif /* INC_PROTOCOL_PROTOCOL_CONSTANTS_H_ */
