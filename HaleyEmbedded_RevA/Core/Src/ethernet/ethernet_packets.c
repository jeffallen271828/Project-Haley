/*
 * ethernet_packets.c
 *
 * Haley binary packet build and parse routines.
 * All multi-byte wire fields are big-endian.
 *
 *  Created on: May 12, 2026
 *      Author: jeffreya181
 */

#include <string.h>
#include "ethernet/ethernet_packets.h"

/* -----------------------------------------------------------------------
 * Internal helpers
 * ----------------------------------------------------------------------- */

/** Write a uint16_t in big-endian order to a buffer */
static inline void write_be16(uint8_t *buf, uint16_t val)
{
    buf[0] = (uint8_t)(val >> 8);
    buf[1] = (uint8_t)(val & 0xFF);
}

/** Write a uint32_t in big-endian order to a buffer */
static inline void write_be32(uint8_t *buf, uint32_t val)
{
    buf[0] = (uint8_t)(val >> 24);
    buf[1] = (uint8_t)(val >> 16);
    buf[2] = (uint8_t)(val >> 8);
    buf[3] = (uint8_t)(val & 0xFF);
}

/** Read a uint16_t in big-endian order from a buffer */
static inline uint16_t read_be16(const uint8_t *buf)
{
    return ((uint16_t)buf[0] << 8) | (uint16_t)buf[1];
}

/** Read a uint32_t in big-endian order from a buffer */
static inline uint32_t read_be32(const uint8_t *buf)
{
    return ((uint32_t)buf[0] << 24)
         | ((uint32_t)buf[1] << 16)
         | ((uint32_t)buf[2] << 8)
         |  (uint32_t)buf[3];
}

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

size_t ETH_PKT_Build(uint8_t          *out_buf,
                     size_t            buf_size,
                     haley_pkt_type_t  type,
                     uint16_t          seq,
                     const uint8_t    *payload,
                     uint16_t          payload_len)
{
    /* Validate arguments */
    if (out_buf == NULL) {
        return 0;
    }
    if (payload_len > HALEY_MAX_PAYLOAD) {
        return 0;
    }
    if (payload == NULL && payload_len > 0) {
        return 0;
    }

    size_t total_size = HALEY_HEADER_SIZE + payload_len + HALEY_FOOTER_SIZE;
    if (buf_size < total_size) {
        return 0;
    }

    /* --- Fill header bytes [0..11] ------------------------------------ */

    /* Bytes 0-1: Magic */
    out_buf[0] = HALEY_MAGIC_0;
    out_buf[1] = HALEY_MAGIC_1;

    /* Byte 2: Version */
    out_buf[2] = HALEY_PROTOCOL_VERSION;

    /* Byte 3: Packet type */
    out_buf[3] = (uint8_t)type;

    /* Bytes 4-5: Sequence number (big-endian) */
    write_be16(&out_buf[4], seq);

    /* Bytes 6-7: Payload length (big-endian) */
    write_be16(&out_buf[6], payload_len);

    /* Bytes 8-11: Header CRC32 (over bytes 0..7) */
    uint32_t hdr_crc = ETH_CRC_Compute(out_buf, HALEY_HDR_CRC_COVER);
    write_be32(&out_buf[HALEY_HDR_CRC_OFFSET], hdr_crc);

    /* --- Copy payload bytes [12 .. 12+payload_len-1] ------------------ */
    if (payload_len > 0) {
        memcpy(&out_buf[HALEY_HEADER_SIZE], payload, payload_len);
    }

    /* --- Payload CRC32 [12+payload_len .. 15+payload_len] ------------- */
    uint32_t pay_crc = (payload_len > 0)
                       ? ETH_CRC_Compute(payload, payload_len)
                       : 0x00000000UL;  /* Zero CRC for empty payloads */

    write_be32(&out_buf[HALEY_HEADER_SIZE + payload_len], pay_crc);

    return total_size;
}

/* ---------------------------------------------------------------------- */

haley_err_t ETH_PKT_ParseHeader(const uint8_t     *raw_buf,
                                 haley_pkt_header_t *out_hdr)
{
    if (raw_buf == NULL || out_hdr == NULL) {
        return HALEY_ERR_BAD_LENGTH;
    }

    /* Check magic bytes */
    if (raw_buf[0] != HALEY_MAGIC_0 || raw_buf[1] != HALEY_MAGIC_1) {
        return HALEY_ERR_BAD_MAGIC;
    }

    /* Check protocol version */
    if (raw_buf[2] != HALEY_PROTOCOL_VERSION) {
        return HALEY_ERR_BAD_VERSION;
    }

    /* Verify header CRC (covers bytes [0..7]) */
    uint32_t expected_hdr_crc = read_be32(&raw_buf[HALEY_HDR_CRC_OFFSET]);
    uint32_t computed_hdr_crc = ETH_CRC_Compute(raw_buf, HALEY_HDR_CRC_COVER);

    if (computed_hdr_crc != expected_hdr_crc) {
        return HALEY_ERR_BAD_HDR_CRC;
    }

    /* Decode fields into host byte order */
    out_hdr->version     = raw_buf[2];
    out_hdr->type        = (haley_pkt_type_t)raw_buf[3];
    out_hdr->sequence    = read_be16(&raw_buf[4]);
    out_hdr->payload_len = read_be16(&raw_buf[6]);
    out_hdr->header_crc  = expected_hdr_crc;

    /* Sanity check payload length */
    if (out_hdr->payload_len > HALEY_MAX_PAYLOAD) {
        return HALEY_ERR_BAD_LENGTH;
    }

    return HALEY_ERR_NONE;
}

/* ---------------------------------------------------------------------- */

haley_err_t ETH_PKT_ValidatePayloadCRC(const uint8_t *payload,
                                        uint16_t       payload_len,
                                        const uint8_t *crc_bytes)
{
    if (crc_bytes == NULL) {
        return HALEY_ERR_BAD_PAY_CRC;
    }

    uint32_t expected_pay_crc = read_be32(crc_bytes);

    if (payload_len == 0) {
        /* Empty payload: CRC must be 0x00000000 */
        return (expected_pay_crc == 0x00000000UL)
               ? HALEY_ERR_NONE
               : HALEY_ERR_BAD_PAY_CRC;
    }

    if (payload == NULL) {
        return HALEY_ERR_BAD_PAY_CRC;
    }

    uint32_t computed_pay_crc = ETH_CRC_Compute(payload, payload_len);

    return (computed_pay_crc == expected_pay_crc)
           ? HALEY_ERR_NONE
           : HALEY_ERR_BAD_PAY_CRC;
}

/* ---------------------------------------------------------------------- */

size_t ETH_PKT_BuildControl(uint8_t          *out_buf,
                             size_t            buf_size,
                             haley_pkt_type_t  type,
                             uint16_t          seq)
{
    return ETH_PKT_Build(out_buf, buf_size, type, seq, NULL, 0);
}

/* ---------------------------------------------------------------------- */

size_t ETH_PKT_BuildNACK(uint8_t    *out_buf,
                          size_t      buf_size,
                          uint16_t    seq,
                          uint16_t    rejected_seq,
                          haley_err_t error_code)
{
    haley_pay_nack_t nack_payload;
    nack_payload.rejected_seq = rejected_seq;
    nack_payload.error_code   = (uint8_t)error_code;

    /* Byte-swap rejected_seq to big-endian in the payload */
    uint8_t payload_buf[3];
    payload_buf[0] = (uint8_t)(rejected_seq >> 8);
    payload_buf[1] = (uint8_t)(rejected_seq & 0xFF);
    payload_buf[2] = (uint8_t)error_code;

    return ETH_PKT_Build(out_buf, buf_size,
                         HALEY_PKT_NACK, seq,
                         payload_buf, sizeof(payload_buf));
}
