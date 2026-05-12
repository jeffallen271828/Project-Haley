/*
 * ethernet_commands.h
 *
 * Command dispatcher for inbound Haley packets.
 *
 * Owns:
 *   - Mapping packet types to FreeRTOS queue messages or direct handlers
 *   - Decoding command payloads
 *   - Routing commands to commandQueue, eventQueue, or telemetryQueue
 *   - Sending ACK/NACK responses via ethernet_protocol
 *
 * Architecture:
 *   ETH_CMD_Dispatch() is called by ethernet_protocol.c for every valid
 *   inbound packet. It routes the packet to the appropriate task or queue.
 *
 *   Command packets (0x10..0x1F) → commandQueue
 *   Control packets (heartbeats)  → handled inline (ACK sent immediately)
 *   Telemetry requests            → eventQueue
 *
 * Layer: Service / Protocol Layer (sits above ethernet_protocol)
 * Dependencies: ethernet_packets.h, cmsis_os.h, main.h (queue handles)
 *
 *  Created on: May 12, 2026
 *      Author: jeffreya181
 */

#ifndef INC_ETHERNET_ETHERNET_COMMANDS_H_
#define INC_ETHERNET_ETHERNET_COMMANDS_H_

#include <stdint.h>
#include "cmsis_os.h"
#include "ethernet/ethernet_packets.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------
 * Command Message Format
 *
 * Messages placed on commandQueue are 32-bit words encoded as:
 *
 *   Bits [31:24] — Command category  (haley_cmd_category_t)
 *   Bits [23:16] — Command ID        (haley_pkt_type_t cast to 8 bits)
 *   Bits [15:0]  — Command parameter (type-specific, e.g. LED ID + mode)
 *
 * For commands with large payloads (audio, sensor data), a separate
 * mechanism (semaphore + shared buffer) is used outside commandQueue.
 * ----------------------------------------------------------------------- */

/** Encode a command queue message */
#define ETH_CMD_MSG(category, cmd_id, param)  \
    ( ((uint32_t)(category) << 24) |          \
      ((uint32_t)(cmd_id)   << 16) |          \
      ((uint32_t)(param)    & 0xFFFFU) )

/** Decode the category from a command queue message */
#define ETH_CMD_MSG_CATEGORY(msg)   (((msg) >> 24) & 0xFFU)

/** Decode the command ID from a command queue message */
#define ETH_CMD_MSG_ID(msg)         (((msg) >> 16) & 0xFFU)

/** Decode the parameter from a command queue message */
#define ETH_CMD_MSG_PARAM(msg)      ((msg) & 0xFFFFU)

/* -----------------------------------------------------------------------
 * Command Categories
 * ----------------------------------------------------------------------- */

typedef enum {
    CMD_CAT_CONTROL   = 0x01,  /**< System control (reset, state change)   */
    CMD_CAT_LED       = 0x02,  /**< LED / visual output commands           */
    CMD_CAT_GPIO      = 0x03,  /**< GPIO / digital output commands         */
    CMD_CAT_AUDIO     = 0x04,  /**< Audio capture control                  */
    CMD_CAT_SENSOR    = 0x05,  /**< Sensor / telemetry requests            */
    CMD_CAT_UNKNOWN   = 0xFF,
} haley_cmd_category_t;

/* -----------------------------------------------------------------------
 * LED Command Parameters (Bits [15:8] = LED ID, Bits [7:0] = LED mode)
 * ----------------------------------------------------------------------- */

/** LED IDs matching main.h LD1, LD2, LD3 pin definitions */
typedef enum {
    LED_ID_LD1 = 0,   /**< PB0  — Green LED  */
    LED_ID_LD2 = 1,   /**< PB7  — Blue LED   */
    LED_ID_LD3 = 2,   /**< PB14 — Red LED    */
} haley_led_id_t;

/** LED operating modes */
typedef enum {
    LED_MODE_OFF   = 0,
    LED_MODE_ON    = 1,
    LED_MODE_BLINK = 2,
    LED_MODE_PULSE = 3,
} haley_led_mode_t;

/** Build the 16-bit LED parameter field */
#define ETH_CMD_LED_PARAM(led_id, mode)  \
    ( (((uint16_t)(led_id) & 0xFFU) << 8) | ((uint16_t)(mode) & 0xFFU) )

/** Extract LED ID from parameter */
#define ETH_CMD_LED_ID(param)    (((param) >> 8) & 0xFFU)

/** Extract LED mode from parameter */
#define ETH_CMD_LED_MODE(param)  ((param) & 0xFFU)

/* -----------------------------------------------------------------------
 * Audio Command Parameters (Bits [15:8] = action, Bits [7:0] = reserved)
 * ----------------------------------------------------------------------- */

typedef enum {
    AUDIO_CTL_STOP  = 0,
    AUDIO_CTL_START = 1,
} haley_audio_action_t;

/* -----------------------------------------------------------------------
 * API
 * ----------------------------------------------------------------------- */

/**
 * @brief  Initialize the command dispatcher.
 *
 *         Must be called before ETH_CMD_Dispatch().
 *         Registers the FreeRTOS queue handles that were created in main.c.
 *
 * @param  cmd_queue_handle  Handle to commandQueue  (created in main.c).
 * @param  evt_queue_handle  Handle to eventQueue    (created in main.c).
 */
void ETH_CMD_Init(osMessageQueueId_t cmd_queue_handle,
                  osMessageQueueId_t evt_queue_handle);

/**
 * @brief  Dispatch a validated inbound packet.
 *
 *         Called by ethernet_protocol.c for every complete, CRC-valid packet.
 *         Routes the packet to the appropriate queue or handles it inline.
 *
 *         This function sends ACK for recognized commands and NACK for
 *         unrecognized packet types.
 *
 * @param  pkt  Pointer to a validated haley_packet_t. Valid only during call.
 */
void ETH_CMD_Dispatch(const haley_packet_t *pkt);

/**
 * @brief  Process a single LED command payload.
 *         Decodes the haley_pay_cmd_led_t payload and routes to commandQueue.
 *
 * @param  pkt  Packet containing a HALEY_PKT_CMD_LED payload.
 */
void ETH_CMD_HandleLED(const haley_packet_t *pkt);

/**
 * @brief  Process an audio control command payload.
 *         Decodes haley_pay_audio_ctl_t and routes to commandQueue.
 *
 * @param  pkt  Packet containing a HALEY_PKT_CMD_AUDIO_CTL payload.
 */
void ETH_CMD_HandleAudioControl(const haley_packet_t *pkt);

/**
 * @brief  Process a system state command payload.
 *         Routes to commandQueue with the new state as parameter.
 *
 * @param  pkt  Packet containing a HALEY_PKT_CMD_STATE payload.
 */
void ETH_CMD_HandleStateChange(const haley_packet_t *pkt);

/**
 * @brief  Handle an inbound heartbeat packet.
 *         Responds with HALEY_PKT_HEARTBEAT_ACK.
 *
 * @param  pkt  Packet of type HALEY_PKT_HEARTBEAT.
 */
void ETH_CMD_HandleHeartbeat(const haley_packet_t *pkt);

#ifdef __cplusplus
}
#endif

#endif /* INC_ETHERNET_ETHERNET_COMMANDS_H_ */
