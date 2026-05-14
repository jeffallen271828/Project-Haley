/*
 * packet_protocol.h
 *
 * Core Haley packet build and parse API.
 *
 * This module provides:
 *   - Header serialization / deserialization
 *   - Full packet build helpers (heartbeat, ACK, telemetry, command, fault)
 *   - CRC32 computation and verification
 *   - Key-value text payload encoding and lookup
 *
 * Wire compatibility:
 *   Matches HaleyLinux_RevA/services/stm32_interface/stm32_packets.py.
 *   Little-endian byte order. 18-byte header. CRC32 over header[0:14]+payload.
 *
 * CRC note:
 *   The CRC32 field (header bytes 14-17) is set to zero before computing
 *   the checksum. The checksum covers exactly: header[0:14] + payload[0:N].
 *   This differs from the raw[:-4] approach in the Python reference, which
 *   contains a framing bug for payloads shorter than 4 bytes. The C
 *   implementation uses the correct scheme; the Python code should be updated
 *   to match (zero the crc32 header field, compute over header[0:14]+payload).
 *
 * Layer: Protocol / Service Layer
 * Dependencies: protocol_types.h, protocol_constants.h
 *
 *  Created on: May 14, 2026
 *      Author: jeffreya181
 */

#ifndef INC_PROTOCOL_PACKET_PROTOCOL_H_
#define INC_PROTOCOL_PACKET_PROTOCOL_H_

#include <stdint.h>
#include <stddef.h>
#include "protocol/protocol_types.h"
#include "protocol/protocol_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------
 * CRC32 API
 *
 * IEEE 802.3 reflected polynomial (0xEDB88320), identical to zlib.crc32.
 * The table is initialised lazily on first call to PKT_CRC32_Compute().
 * ----------------------------------------------------------------------- */

/**
 * @brief  Compute CRC32 over a data buffer.
 *
 *         Uses the IEEE 802.3 reflected polynomial (0xEDB88320), identical
 *         to Python's zlib.crc32() output.
 *
 * @param  data    Pointer to input bytes.
 * @param  length  Number of bytes to process.
 * @return 32-bit CRC value.
 */
uint32_t PKT_CRC32_Compute(const uint8_t *data, size_t length);

/**
 * @brief  Continue a multi-segment CRC32 computation.
 *
 * @param  running_crc  Current CRC state. Pass 0xFFFFFFFF to begin.
 * @param  data         Next data segment.
 * @param  length       Bytes in this segment.
 * @return Updated running CRC. Call PKT_CRC32_Finalize() when done.
 */
uint32_t PKT_CRC32_Update(uint32_t running_crc,
                           const uint8_t *data,
                           size_t length);

/**
 * @brief  Finalize a multi-segment CRC32.
 *
 * @param  running_crc  Value returned by the last PKT_CRC32_Update() call.
 * @return Final CRC32 value (post-XOR applied).
 */
uint32_t PKT_CRC32_Finalize(uint32_t running_crc);

/* -----------------------------------------------------------------------
 * Header serialization / deserialization
 * ----------------------------------------------------------------------- */

/**
 * @brief  Serialize a header into a 18-byte wire buffer (little-endian).
 *
 *         The crc32 field in @p hdr is written as-is; call this after
 *         PKT_ComputeHeaderCRC() has set hdr->crc32 to the correct value.
 *
 * @param  out_buf  Destination buffer. Must be at least HALEY_PROTO_HEADER_SIZE bytes.
 * @param  hdr      Pointer to the host-order header.
 */
void PKT_SerializeHeader(uint8_t *out_buf, const haley_hdr_t *hdr);

/**
 * @brief  Deserialize an 18-byte wire buffer into a host-order header.
 *
 *         Validates magic bytes and protocol version but does NOT verify CRC
 *         (call PKT_VerifyCRC() separately after reading the payload).
 *
 * @param  raw_buf  Source buffer. Must be at least HALEY_PROTO_HEADER_SIZE bytes.
 * @param  out_hdr  Destination for decoded header.
 * @return HALEY_PROTO_OK, HALEY_PROTO_ERR_BAD_MAGIC, or HALEY_PROTO_ERR_BAD_VERSION.
 */
haley_proto_err_t PKT_ParseHeader(const uint8_t *raw_buf, haley_hdr_t *out_hdr);

/**
 * @brief  Compute the CRC32 for a packet (header + payload).
 *
 *         Computes CRC32 over header bytes [0..13] (with crc32 field = 0)
 *         concatenated with the full payload. The result should be written
 *         into hdr->crc32 before calling PKT_SerializeHeader().
 *
 * @param  hdr          Pointer to host-order header (crc32 field ignored).
 * @param  payload      Pointer to payload bytes (may be NULL if payload_length == 0).
 * @param  payload_len  Number of payload bytes.
 * @return Computed CRC32 value.
 */
uint32_t PKT_ComputeCRC(const haley_hdr_t *hdr,
                         const uint8_t     *payload,
                         uint32_t           payload_len);

/**
 * @brief  Verify the CRC32 of a received packet.
 *
 *         Recomputes the CRC and compares it to hdr->crc32.
 *
 * @param  hdr          Decoded header (crc32 field contains received value).
 * @param  payload      Pointer to received payload bytes.
 * @param  payload_len  Number of payload bytes.
 * @return HALEY_PROTO_OK if CRC matches, HALEY_PROTO_ERR_BAD_CRC otherwise.
 */
haley_proto_err_t PKT_VerifyCRC(const haley_hdr_t *hdr,
                                 const uint8_t     *payload,
                                 uint32_t           payload_len);

/* -----------------------------------------------------------------------
 * Packet build helpers
 *
 * Each function writes a fully serialized, CRC-stamped packet into an
 * output buffer. Returns the total number of bytes written, or 0 on error.
 *
 * Caller manages the transmit sequence counter; pass the next value as @p seq.
 * ----------------------------------------------------------------------- */

/**
 * @brief  Build a heartbeat packet.
 *
 *         Heartbeat packets carry no key-value payload (empty body).
 *         When @p is_request is non-zero, HALEY_FLAG_REQUEST is set.
 *
 * @param  out_buf    Output buffer (>= HALEY_PROTO_HEADER_SIZE bytes).
 * @param  buf_size   Size of out_buf.
 * @param  seq        Sequence number to embed.
 * @param  is_request Non-zero to set REQUEST flag; zero to set RESPONSE flag.
 * @return Bytes written, or 0 on error.
 */
size_t PKT_BuildHeartbeat(uint8_t  *out_buf,
                           size_t    buf_size,
                           uint32_t  seq,
                           int       is_request);

/**
 * @brief  Build an ACK packet.
 *
 *         ACK carries no payload. The sequence number embedded is the ACK's
 *         own TX sequence, not the sequence being acknowledged.
 *         Put the acknowledged sequence in the key-value payload if needed.
 *
 * @param  out_buf   Output buffer.
 * @param  buf_size  Size of out_buf.
 * @param  seq       ACK's own sequence number.
 * @return Bytes written, or 0 on error.
 */
size_t PKT_BuildACK(uint8_t *out_buf, size_t buf_size, uint32_t seq);

/**
 * @brief  Build a command packet with a key-value text payload.
 *
 *         The payload must already be formatted as "key=value\n" lines and
 *         NUL-terminated. Use the PKT_KV_* helpers to build it.
 *
 * @param  out_buf      Output buffer.
 * @param  buf_size     Size of out_buf.
 * @param  seq          Sequence number.
 * @param  kv_payload   Pointer to NUL-terminated key-value text.
 * @param  kv_len       Length of kv_payload in bytes (not including NUL).
 * @param  flags        Flags to set (e.g. HALEY_FLAG_ACK_REQUIRED).
 * @return Bytes written, or 0 on error.
 */
size_t PKT_BuildCommand(uint8_t           *out_buf,
                         size_t             buf_size,
                         uint32_t           seq,
                         const uint8_t     *kv_payload,
                         uint32_t           kv_len,
                         haley_pkt_flags_t  flags);

/**
 * @brief  Build a telemetry packet with a key-value text payload.
 *
 * @param  out_buf    Output buffer.
 * @param  buf_size   Size of out_buf.
 * @param  seq        Sequence number.
 * @param  kv_payload Key-value text payload.
 * @param  kv_len     Length of kv_payload in bytes.
 * @return Bytes written, or 0 on error.
 */
size_t PKT_BuildTelemetry(uint8_t       *out_buf,
                           size_t         buf_size,
                           uint32_t       seq,
                           const uint8_t *kv_payload,
                           uint32_t       kv_len);

/**
 * @brief  Build a fault notification packet.
 *
 * @param  out_buf    Output buffer.
 * @param  buf_size   Size of out_buf.
 * @param  seq        Sequence number.
 * @param  kv_payload Key-value text payload describing the fault.
 * @param  kv_len     Length of kv_payload in bytes.
 * @return Bytes written, or 0 on error.
 */
size_t PKT_BuildFault(uint8_t       *out_buf,
                       size_t         buf_size,
                       uint32_t       seq,
                       const uint8_t *kv_payload,
                       uint32_t       kv_len);

/**
 * @brief  Build a version-negotiation packet.
 *
 *         Payload encodes device name, protocol version, and audio config
 *         as key-value pairs. Mirrors STM32Protocol.build_version_negotiation().
 *
 * @param  out_buf    Output buffer.
 * @param  buf_size   Size of out_buf.
 * @param  seq        Sequence number.
 * @return Bytes written, or 0 on error.
 */
size_t PKT_BuildVersionNegotiation(uint8_t *out_buf,
                                    size_t   buf_size,
                                    uint32_t seq);

/**
 * @brief  Build a state packet with a key-value payload.
 *
 * @param  out_buf    Output buffer.
 * @param  buf_size   Size of out_buf.
 * @param  seq        Sequence number.
 * @param  kv_payload Key-value payload describing the new state.
 * @param  kv_len     Length of kv_payload in bytes.
 * @return Bytes written, or 0 on error.
 */
size_t PKT_BuildState(uint8_t       *out_buf,
                       size_t         buf_size,
                       uint32_t       seq,
                       const uint8_t *kv_payload,
                       uint32_t       kv_len);

/**
 * @brief  Build a raw-payload packet of any type.
 *
 *         Low-level builder used by the higher-level helpers above.
 *         CRC is computed and embedded automatically.
 *
 * @param  out_buf      Output buffer.
 * @param  buf_size     Size of out_buf.
 * @param  msg_type     Message type.
 * @param  seq          Sequence number.
 * @param  flags        Flags bitmask.
 * @param  payload      Payload bytes (may be NULL when payload_len == 0).
 * @param  payload_len  Number of payload bytes.
 * @return Total bytes written (header + payload), or 0 on error.
 */
size_t PKT_Build(uint8_t           *out_buf,
                  size_t             buf_size,
                  haley_msg_type_t   msg_type,
                  uint32_t           seq,
                  haley_pkt_flags_t  flags,
                  const uint8_t     *payload,
                  uint32_t           payload_len);

/* -----------------------------------------------------------------------
 * Key-value text payload helpers
 *
 * Payload encoding format (UTF-8 text, matches Python encode_key_values):
 *   "key1=value1\nkey2=value2\n..."
 *
 * Each key and value is a printable ASCII string.
 * Keys must not contain '=' or '\n'. Values must not contain '\n'.
 * ----------------------------------------------------------------------- */

/**
 * @brief  Initialize a key-value builder context.
 *
 *         Must be called before the first PKT_KV_Add() call.
 *
 * @param  buf       Output character buffer.
 * @param  buf_size  Size of buf in bytes.
 */
void PKT_KV_Init(char *buf, size_t buf_size);

/**
 * @brief  Append a key-value pair to the builder buffer.
 *
 *         Writes "key=value\n" to the buffer. Returns HALEY_PROTO_ERR_BUF_SMALL
 *         if the pair does not fit.
 *
 * @param  buf       Buffer previously passed to PKT_KV_Init().
 * @param  buf_size  Size of buf in bytes.
 * @param  key       NUL-terminated key string.
 * @param  value     NUL-terminated value string.
 * @return HALEY_PROTO_OK or HALEY_PROTO_ERR_BUF_SMALL.
 */
haley_proto_err_t PKT_KV_Add(char       *buf,
                               size_t      buf_size,
                               const char *key,
                               const char *value);

/**
 * @brief  Convenience wrapper: append a uint32_t value.
 *
 * @param  buf       Builder buffer.
 * @param  buf_size  Size of buf.
 * @param  key       NUL-terminated key string.
 * @param  value     Numeric value to format as decimal string.
 * @return HALEY_PROTO_OK or HALEY_PROTO_ERR_BUF_SMALL.
 */
haley_proto_err_t PKT_KV_AddUInt(char       *buf,
                                   size_t      buf_size,
                                   const char *key,
                                   uint32_t    value);

/**
 * @brief  Convenience wrapper: append an int32_t value.
 */
haley_proto_err_t PKT_KV_AddInt(char       *buf,
                                  size_t      buf_size,
                                  const char *key,
                                  int32_t     value);

/**
 * @brief  Convenience wrapper: append a boolean (writes "true" or "false").
 */
haley_proto_err_t PKT_KV_AddBool(char       *buf,
                                   size_t      buf_size,
                                   const char *key,
                                   int         value);

/**
 * @brief  Return the current length of the accumulated key-value string.
 *
 * @param  buf  Builder buffer.
 * @return Number of bytes written so far (excluding NUL terminator).
 */
size_t PKT_KV_Length(const char *buf);

/**
 * @brief  Look up a value by key in a received key-value payload.
 *
 *         Scans the text payload for a line starting with "key=".
 *         Copies the corresponding value into out_val (NUL-terminated).
 *
 * @param  payload       Raw payload bytes from a received packet.
 * @param  payload_len   Number of payload bytes.
 * @param  key           NUL-terminated key to search for.
 * @param  out_val       Output buffer for the found value.
 * @param  out_val_size  Size of out_val in bytes (including room for NUL).
 * @return HALEY_PROTO_OK if found,
 *         HALEY_PROTO_ERR_KV_NOT_FOUND if the key is absent,
 *         HALEY_PROTO_ERR_KV_TRUNCATED if the value does not fit in out_val.
 */
haley_proto_err_t PKT_KV_Find(const uint8_t *payload,
                                uint32_t       payload_len,
                                const char    *key,
                                char          *out_val,
                                size_t         out_val_size);

/**
 * @brief  Look up a uint32_t value by key.
 *
 * @param  payload      Raw payload bytes.
 * @param  payload_len  Number of payload bytes.
 * @param  key          Key to search for.
 * @param  out_val      Output for parsed integer.
 * @return HALEY_PROTO_OK or HALEY_PROTO_ERR_KV_NOT_FOUND.
 */
haley_proto_err_t PKT_KV_FindUInt(const uint8_t *payload,
                                    uint32_t       payload_len,
                                    const char    *key,
                                    uint32_t      *out_val);

/**
 * @brief  Look up a boolean value ("true"/"1"/"yes"/"on" → 1, else → 0).
 */
haley_proto_err_t PKT_KV_FindBool(const uint8_t *payload,
                                    uint32_t       payload_len,
                                    const char    *key,
                                    int           *out_val);

#ifdef __cplusplus
}
#endif

#endif /* INC_PROTOCOL_PACKET_PROTOCOL_H_ */
