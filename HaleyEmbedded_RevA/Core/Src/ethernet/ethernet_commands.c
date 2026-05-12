/*
 * ethernet_commands.c
 *
 * Routes inbound Haley packets to the appropriate FreeRTOS queues and handlers.
 *
 *  Created on: May 12, 2026
 *      Author: jeffreya181
 */

#include <string.h>
#include "cmsis_os.h"
#include "ethernet/ethernet_commands.h"
#include "ethernet/ethernet_protocol.h"

/* -----------------------------------------------------------------------
 * Module-private state
 * ----------------------------------------------------------------------- */

static osMessageQueueId_t s_cmd_queue = NULL;
static osMessageQueueId_t s_evt_queue = NULL;

/* -----------------------------------------------------------------------
 * Internal helpers
 * ----------------------------------------------------------------------- */

/**
 * @brief  Post a uint32_t message to a queue without blocking.
 *         Drops the message silently if the queue is full (avoids deadlock
 *         from the RX task blocking on a full command queue).
 */
static inline void queue_post(osMessageQueueId_t queue, uint32_t msg)
{
    if (queue == NULL) {
        return;
    }
    /* Use 0 timeout — drop if full rather than blocking the Rx task */
    osMessageQueuePut(queue, &msg, 0, 0);
}

/**
 * @brief  Read a big-endian uint16_t from a byte buffer.
 */
static inline uint16_t be16(const uint8_t *buf)
{
    return ((uint16_t)buf[0] << 8) | (uint16_t)buf[1];
}

/**
 * @brief  Read a big-endian uint32_t from a byte buffer.
 */
static inline uint32_t be32(const uint8_t *buf)
{
    return ((uint32_t)buf[0] << 24)
         | ((uint32_t)buf[1] << 16)
         | ((uint32_t)buf[2] << 8)
         |  (uint32_t)buf[3];
}

/* -----------------------------------------------------------------------
 * Handler implementations
 * ----------------------------------------------------------------------- */

void ETH_CMD_HandleHeartbeat(const haley_packet_t *pkt)
{
    if (pkt == NULL) {
        return;
    }

    /* Respond immediately with a heartbeat ACK */
    uint32_t uptime_ms = osKernelGetTickCount();
    ETH_PROTO_SendHeartbeat(uptime_ms, 0x01 /* TODO: replace with real state */);

    /* Also post a HEARTBEAT event so systemStateTask knows the host is alive */
    uint32_t evt_msg = ETH_CMD_MSG(CMD_CAT_CONTROL,
                                   (uint8_t)HALEY_PKT_HEARTBEAT,
                                   0);
    queue_post(s_evt_queue, evt_msg);
}

/* ---------------------------------------------------------------------- */

void ETH_CMD_HandleLED(const haley_packet_t *pkt)
{
    if (pkt == NULL || pkt->header.payload_len < 6) {
        /* Payload too short for a valid LED command */
        ETH_PROTO_SendNACK(pkt ? pkt->header.sequence : 0,
                           HALEY_ERR_BAD_LENGTH);
        return;
    }

    const uint8_t *p = pkt->payload;

    /*
     * LED command payload layout (from ethernet_packets.h):
     *   [0]   led_id
     *   [1]   mode
     *   [2]   r
     *   [3]   g
     *   [4]   b
     *   [5..6] period_ms (big-endian)
     */
    uint8_t  led_id    = p[0];
    uint8_t  led_mode  = p[1];
    /* RGB and period_ms are available in p[2..6] for future use */

    if (led_id > LED_ID_LD3) {
        ETH_PROTO_SendNACK(pkt->header.sequence, HALEY_ERR_UNKNOWN_TYPE);
        return;
    }

    uint32_t cmd_msg = ETH_CMD_MSG(CMD_CAT_LED,
                                   (uint8_t)HALEY_PKT_CMD_LED,
                                   ETH_CMD_LED_PARAM(led_id, led_mode));
    queue_post(s_cmd_queue, cmd_msg);

    /* ACK the command */
    ETH_PROTO_SendACK(pkt->header.sequence);
}

/* ---------------------------------------------------------------------- */

void ETH_CMD_HandleAudioControl(const haley_packet_t *pkt)
{
    if (pkt == NULL || pkt->header.payload_len < 5) {
        ETH_PROTO_SendNACK(pkt ? pkt->header.sequence : 0,
                           HALEY_ERR_BAD_LENGTH);
        return;
    }

    const uint8_t *p = pkt->payload;

    /*
     * Audio control payload layout:
     *   [0]   action (0=stop, 1=start)
     *   [1..2] sample_rate (big-endian)
     *   [3]   bit_depth
     *   [4]   channels
     */
    uint8_t  action      = p[0];
    uint16_t sample_rate = be16(&p[1]);
    uint8_t  bit_depth   = p[3];
    uint8_t  channels    = p[4];

    (void)sample_rate;  /* Stored in shared config; not encoded in 16-bit param */
    (void)bit_depth;
    (void)channels;

    uint32_t cmd_msg = ETH_CMD_MSG(CMD_CAT_AUDIO,
                                   (uint8_t)HALEY_PKT_CMD_AUDIO_CTL,
                                   (uint16_t)action);
    queue_post(s_cmd_queue, cmd_msg);

    ETH_PROTO_SendACK(pkt->header.sequence);
}

/* ---------------------------------------------------------------------- */

void ETH_CMD_HandleStateChange(const haley_packet_t *pkt)
{
    if (pkt == NULL || pkt->header.payload_len < 1) {
        ETH_PROTO_SendNACK(pkt ? pkt->header.sequence : 0,
                           HALEY_ERR_BAD_LENGTH);
        return;
    }

    uint8_t new_state = pkt->payload[0];

    uint32_t cmd_msg = ETH_CMD_MSG(CMD_CAT_CONTROL,
                                   (uint8_t)HALEY_PKT_CMD_STATE,
                                   (uint16_t)new_state);
    queue_post(s_cmd_queue, cmd_msg);

    ETH_PROTO_SendACK(pkt->header.sequence);
}

/* -----------------------------------------------------------------------
 * Main dispatch table
 * ----------------------------------------------------------------------- */

void ETH_CMD_Dispatch(const haley_packet_t *pkt)
{
    if (pkt == NULL || !pkt->valid) {
        return;
    }

    switch (pkt->header.type) {

    /* --- Control / Handshake ------------------------------------------ */
    case HALEY_PKT_HEARTBEAT:
        ETH_CMD_HandleHeartbeat(pkt);
        break;

    case HALEY_PKT_HEARTBEAT_ACK:
        /* Host acknowledged our heartbeat — post event for state task */
        {
            uint32_t evt = ETH_CMD_MSG(CMD_CAT_CONTROL,
                                       (uint8_t)HALEY_PKT_HEARTBEAT_ACK, 0);
            queue_post(s_evt_queue, evt);
        }
        break;

    case HALEY_PKT_ACK:
        /* Host acknowledged one of our packets. Could track in-flight packets
         * here in the future. For now, just discard. */
        break;

    case HALEY_PKT_NACK:
        /* Host rejected one of our packets. Log for diagnostics. */
        {
            uint32_t evt = ETH_CMD_MSG(CMD_CAT_CONTROL,
                                       (uint8_t)HALEY_PKT_NACK, 0);
            queue_post(s_evt_queue, evt);
        }
        break;

    case HALEY_PKT_HELLO:
        /* Initial handshake from host — respond with HELLO_ACK + boot status */
        {
            uint8_t pkt_buf[HALEY_OVERHEAD_SIZE];
            size_t  pkt_len = ETH_PKT_BuildControl(pkt_buf, sizeof(pkt_buf),
                                                    HALEY_PKT_HELLO_ACK,
                                                    0 /* seq managed by protocol */);
            (void)pkt_len;
            /* Delegate send through protocol layer to use its seq counter */
            ETH_PROTO_SendACK(pkt->header.sequence);

            /* Post HELLO event so systemStateTask can finalize AI host handshake */
            uint32_t evt = ETH_CMD_MSG(CMD_CAT_CONTROL,
                                       (uint8_t)HALEY_PKT_HELLO, 0);
            queue_post(s_evt_queue, evt);
        }
        break;

    /* --- Hardware Commands --------------------------------------------- */
    case HALEY_PKT_CMD_LED:
        ETH_CMD_HandleLED(pkt);
        break;

    case HALEY_PKT_CMD_AUDIO_CTL:
        ETH_CMD_HandleAudioControl(pkt);
        break;

    case HALEY_PKT_CMD_STATE:
        ETH_CMD_HandleStateChange(pkt);
        break;

    case HALEY_PKT_CMD_GPIO:
        /* TODO: implement GPIO command handler */
        ETH_PROTO_SendNACK(pkt->header.sequence, HALEY_ERR_UNKNOWN_TYPE);
        break;

    case HALEY_PKT_CMD_RELAY:
        /* TODO: implement relay command handler */
        ETH_PROTO_SendNACK(pkt->header.sequence, HALEY_ERR_UNKNOWN_TYPE);
        break;

    case HALEY_PKT_CMD_RESET:
        /* Soft reset request — post to command queue for watchdog task to action */
        {
            uint32_t cmd = ETH_CMD_MSG(CMD_CAT_CONTROL,
                                       (uint8_t)HALEY_PKT_CMD_RESET, 0);
            queue_post(s_cmd_queue, cmd);
            ETH_PROTO_SendACK(pkt->header.sequence);
        }
        break;

    /* --- Unrecognized type -------------------------------------------- */
    default:
        ETH_PROTO_SendNACK(pkt->header.sequence, HALEY_ERR_UNKNOWN_TYPE);
        break;
    }
}

/* -----------------------------------------------------------------------
 * Init
 * ----------------------------------------------------------------------- */

void ETH_CMD_Init(osMessageQueueId_t cmd_queue_handle,
                  osMessageQueueId_t evt_queue_handle)
{
    s_cmd_queue = cmd_queue_handle;
    s_evt_queue = evt_queue_handle;
}
