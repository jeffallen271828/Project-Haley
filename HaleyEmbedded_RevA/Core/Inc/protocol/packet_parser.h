/*
 * packet_parser.h
 *
 * Byte-stream receive state machine for the Haley binary protocol.
 *
 * Responsibility:
 *   Accepts a raw byte stream (one or more bytes per call) and reassembles
 *   complete, CRC-validated Haley packets. When a packet is ready, it is
 *   delivered to an application callback.
 *
 * State machine overview:
 *
 *   WAIT_MAGIC_0  ──(saw 0x41)──▶  WAIT_MAGIC_1
 *   WAIT_MAGIC_1  ──(saw 0x48)──▶  RECV_HEADER
 *   RECV_HEADER   ──(16 more bytes)─▶  RECV_PAYLOAD
 *   RECV_PAYLOAD  ──(payload_length bytes)─▶  VALIDATE_CRC
 *   VALIDATE_CRC  ──(CRC OK)──▶  callback + reset to WAIT_MAGIC_0
 *                 ──(CRC fail)─▶  error callback + reset
 *
 * Wire magic on the byte stream:
 *   The 18-byte header starts with magic 0x4841 stored in little-endian
 *   order, so the wire bytes are [0x41, 0x48].  The parser waits for
 *   byte 0x41 first, then 0x48.
 *
 * Thread safety:
 *   A haley_parser_t instance is NOT thread-safe. Each RTOS task that
 *   receives data should own its own parser instance, or protect access
 *   with a mutex.
 *
 * Layer: Protocol / Service Layer
 * Dependencies: protocol_types.h, protocol_constants.h, packet_protocol.h
 *
 *  Created on: May 14, 2026
 *      Author: jeffreya181
 */

#ifndef INC_PROTOCOL_PACKET_PARSER_H_
#define INC_PROTOCOL_PACKET_PARSER_H_

#include <stdint.h>
#include <stddef.h>
#include "protocol/protocol_types.h"
#include "protocol/protocol_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------
 * Callbacks
 * ----------------------------------------------------------------------- */

/**
 * @brief  Application callback invoked for every valid received packet.
 *
 *         The pointer @p pkt is only valid for the duration of the callback.
 *         The application must copy any data it needs to retain.
 *
 * @param  pkt       Pointer to the validated, fully reassembled packet.
 * @param  user_ctx  Opaque pointer passed to PARSER_Init().
 */
typedef void (*haley_parser_rx_cb_t)(const haley_packet_t *pkt,
                                      void                 *user_ctx);

/**
 * @brief  Application callback invoked when a framing or CRC error occurs.
 *
 * @param  err       Error code describing the failure.
 * @param  user_ctx  Opaque pointer passed to PARSER_Init().
 */
typedef void (*haley_parser_err_cb_t)(haley_proto_err_t err,
                                       void             *user_ctx);

/* -----------------------------------------------------------------------
 * Internal parser states (exposed for diagnostics only)
 * ----------------------------------------------------------------------- */

typedef enum {
    PARSER_STATE_WAIT_MAGIC_0  = 0, /**< Waiting for first magic byte (0x41) */
    PARSER_STATE_WAIT_MAGIC_1  = 1, /**< Waiting for second magic byte (0x48) */
    PARSER_STATE_RECV_HEADER   = 2, /**< Accumulating remaining 16 header bytes */
    PARSER_STATE_RECV_PAYLOAD  = 3, /**< Accumulating payload bytes            */
    PARSER_STATE_VALIDATE_CRC  = 4, /**< All bytes received; CRC check pending */
} haley_parser_state_t;

/* -----------------------------------------------------------------------
 * Parser instance
 * ----------------------------------------------------------------------- */

/**
 * @brief  Parser context. Allocate one instance per receive channel.
 *
 *         Initialise with PARSER_Init() before first use. The internals
 *         should be treated as opaque; use the accessor functions below.
 */
typedef struct {
    /* State machine */
    haley_parser_state_t state;

    /* Header accumulation buffer (18 bytes) */
    uint8_t  hdr_buf[HALEY_PROTO_HEADER_SIZE];
    uint16_t hdr_bytes;          /**< Bytes accumulated so far */

    /* Payload accumulation buffer */
    uint8_t  payload_buf[HALEY_PROTO_MAX_PAYLOAD];
    uint32_t payload_bytes;      /**< Bytes accumulated so far */
    uint32_t payload_expected;   /**< Total bytes to accumulate (from header) */

    /* Decoded header for the in-progress packet */
    haley_hdr_t current_hdr;

    /* Callbacks */
    haley_parser_rx_cb_t  on_packet;
    haley_parser_err_cb_t on_error;
    void                 *user_ctx;

    /* Statistics */
    uint32_t stat_packets_ok;    /**< Total successfully parsed packets */
    uint32_t stat_crc_errors;    /**< Total CRC validation failures     */
    uint32_t stat_framing_errors;/**< Total magic / version mismatches  */
} haley_parser_t;

/* -----------------------------------------------------------------------
 * API
 * ----------------------------------------------------------------------- */

/**
 * @brief  Initialise a parser instance.
 *
 *         Must be called before the first PARSER_Feed() call.
 *         Safe to call again to fully reset a parser (e.g. after a
 *         transport reconnect).
 *
 * @param  parser     Parser instance to initialise.
 * @param  on_packet  Callback for valid packets. Must not be NULL.
 * @param  on_error   Callback for errors. May be NULL.
 * @param  user_ctx   Opaque pointer forwarded to both callbacks.
 * @return HALEY_PROTO_OK, or HALEY_PROTO_ERR_NULL_PTR if on_packet is NULL.
 */
haley_proto_err_t PARSER_Init(haley_parser_t       *parser,
                                haley_parser_rx_cb_t  on_packet,
                                haley_parser_err_cb_t on_error,
                                void                 *user_ctx);

/**
 * @brief  Reset a parser to its initial state without changing callbacks.
 *
 *         Useful after a transport reconnect or when a framing error is
 *         detected outside the parser (e.g. by a higher layer).
 *
 * @param  parser  Parser instance to reset.
 */
void PARSER_Reset(haley_parser_t *parser);

/**
 * @brief  Feed received bytes into the parser.
 *
 *         Processes @p length bytes from @p data through the state machine.
 *         For each complete, valid packet found, on_packet is called.
 *         For each error, on_error is called and the state machine resets
 *         to resync framing.
 *
 *         This function is designed to be called from ethernetRxTask's loop
 *         after each raw receive from the transport driver.
 *
 * @param  parser  Parser instance.
 * @param  data    Pointer to received bytes.
 * @param  length  Number of bytes to process.
 */
void PARSER_Feed(haley_parser_t *parser,
                  const uint8_t  *data,
                  size_t          length);

/**
 * @brief  Feed a single byte into the parser.
 *
 *         Convenience wrapper around PARSER_Feed() for byte-at-a-time sources.
 *
 * @param  parser  Parser instance.
 * @param  byte    The next received byte.
 */
void PARSER_FeedByte(haley_parser_t *parser, uint8_t byte);

/* -----------------------------------------------------------------------
 * Diagnostics
 * ----------------------------------------------------------------------- */

/**
 * @brief  Query the current internal state of the parser.
 *
 * @param  parser  Parser instance.
 * @return Current haley_parser_state_t value.
 */
haley_parser_state_t PARSER_GetState(const haley_parser_t *parser);

/**
 * @brief  Return the total number of successfully parsed packets.
 */
uint32_t PARSER_GetPacketCount(const haley_parser_t *parser);

/**
 * @brief  Return the total number of CRC errors since last init/reset.
 */
uint32_t PARSER_GetCRCErrorCount(const haley_parser_t *parser);

/**
 * @brief  Return the total number of framing errors since last init/reset.
 */
uint32_t PARSER_GetFramingErrorCount(const haley_parser_t *parser);

#ifdef __cplusplus
}
#endif

#endif /* INC_PROTOCOL_PACKET_PARSER_H_ */
