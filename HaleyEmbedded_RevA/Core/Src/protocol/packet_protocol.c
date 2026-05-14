/*
 * packet_protocol.c
 *
 * Implementation of the core Haley packet build and parse API.
 */

#include "protocol/packet_protocol.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* -----------------------------------------------------------------------
 * CRC32 Implementation
 * ----------------------------------------------------------------------- */

static uint32_t crc32_table[256];
static int crc_table_initialized = 0;

static void InitCRC32Table(void) {
    if (crc_table_initialized) return;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (uint32_t j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ HALEY_PROTO_CRC32_POLY;
            } else {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }
    crc_table_initialized = 1;
}

uint32_t PKT_CRC32_Update(uint32_t running_crc, const uint8_t *data, size_t length) {
    if (!crc_table_initialized) InitCRC32Table();
    for (size_t i = 0; i < length; i++) {
        running_crc = crc32_table[(running_crc ^ data[i]) & 0xFF] ^ (running_crc >> 8);
    }
    return running_crc;
}

uint32_t PKT_CRC32_Compute(const uint8_t *data, size_t length) {
    uint32_t crc = PKT_CRC32_Update(HALEY_PROTO_CRC32_INIT, data, length);
    return PKT_CRC32_Finalize(crc);
}

uint32_t PKT_CRC32_Finalize(uint32_t running_crc) {
    return running_crc ^ HALEY_PROTO_CRC32_XOR_OUT;
}

/* -----------------------------------------------------------------------
 * Header serialization / deserialization
 * ----------------------------------------------------------------------- */

void PKT_SerializeHeader(uint8_t *out_buf, const haley_hdr_t *hdr) {
    out_buf[0] = (uint8_t)(hdr->magic & 0xFF);
    out_buf[1] = (uint8_t)((hdr->magic >> 8) & 0xFF);
    out_buf[2] = hdr->version;
    out_buf[3] = (uint8_t)hdr->msg_type;
    out_buf[4] = (uint8_t)(hdr->flags & 0xFF);
    out_buf[5] = (uint8_t)((hdr->flags >> 8) & 0xFF);

    out_buf[6] = (uint8_t)(hdr->sequence & 0xFF);
    out_buf[7] = (uint8_t)((hdr->sequence >> 8) & 0xFF);
    out_buf[8] = (uint8_t)((hdr->sequence >> 16) & 0xFF);
    out_buf[9] = (uint8_t)((hdr->sequence >> 24) & 0xFF);

    out_buf[10] = (uint8_t)(hdr->payload_length & 0xFF);
    out_buf[11] = (uint8_t)((hdr->payload_length >> 8) & 0xFF);
    out_buf[12] = (uint8_t)((hdr->payload_length >> 16) & 0xFF);
    out_buf[13] = (uint8_t)((hdr->payload_length >> 24) & 0xFF);

    out_buf[14] = (uint8_t)(hdr->crc32 & 0xFF);
    out_buf[15] = (uint8_t)((hdr->crc32 >> 8) & 0xFF);
    out_buf[16] = (uint8_t)((hdr->crc32 >> 16) & 0xFF);
    out_buf[17] = (uint8_t)((hdr->crc32 >> 24) & 0xFF);
}

haley_proto_err_t PKT_ParseHeader(const uint8_t *raw_buf, haley_hdr_t *out_hdr) {
    out_hdr->magic = (uint16_t)(raw_buf[0] | (raw_buf[1] << 8));
    if (out_hdr->magic != HALEY_PROTO_MAGIC) return HALEY_PROTO_ERR_BAD_MAGIC;

    out_hdr->version = raw_buf[2];
    if (out_hdr->version != HALEY_PROTO_VERSION) return HALEY_PROTO_ERR_BAD_VERSION;

    out_hdr->msg_type = (haley_msg_type_t)raw_buf[3];
    out_hdr->flags = (haley_pkt_flags_t)(raw_buf[4] | (raw_buf[5] << 8));
    out_hdr->sequence = (uint32_t)(raw_buf[6] | (raw_buf[7] << 8) | (raw_buf[8] << 16) | (raw_buf[9] << 24));
    out_hdr->payload_length = (uint32_t)(raw_buf[10] | (raw_buf[11] << 8) | (raw_buf[12] << 16) | (raw_buf[13] << 24));
    out_hdr->crc32 = (uint32_t)(raw_buf[14] | (raw_buf[15] << 8) | (raw_buf[16] << 16) | (raw_buf[17] << 24));

    return HALEY_PROTO_OK;
}

uint32_t PKT_ComputeCRC(const haley_hdr_t *hdr, const uint8_t *payload, uint32_t payload_len) {
    uint8_t temp_hdr[HALEY_PROTO_HEADER_SIZE];
    haley_hdr_t hdr_copy = *hdr;
    hdr_copy.crc32 = 0;

    PKT_SerializeHeader(temp_hdr, &hdr_copy);

    uint32_t crc = PKT_CRC32_Update(HALEY_PROTO_CRC32_INIT, temp_hdr, HALEY_PROTO_CRC_COVER);
    if (payload && payload_len > 0) {
        crc = PKT_CRC32_Update(crc, payload, payload_len);
    }
    return PKT_CRC32_Finalize(crc);
}

haley_proto_err_t PKT_VerifyCRC(const haley_hdr_t *hdr, const uint8_t *payload, uint32_t payload_len) {
#if HALEY_PROTO_USE_CRC32
    uint32_t computed = PKT_ComputeCRC(hdr, payload, payload_len);
    return (computed == hdr->crc32) ? HALEY_PROTO_OK : HALEY_PROTO_ERR_BAD_CRC;
#else
    return HALEY_PROTO_OK;
#endif
}

/* -----------------------------------------------------------------------
 * Packet build helpers
 * ----------------------------------------------------------------------- */

size_t PKT_Build(uint8_t *out_buf, size_t buf_size, haley_msg_type_t msg_type,
                 uint32_t seq, haley_pkt_flags_t flags, const uint8_t *payload, uint32_t payload_len) {

    if (buf_size < (HALEY_PROTO_HEADER_SIZE + payload_len)) return 0;

    haley_hdr_t hdr;
    hdr.magic = HALEY_PROTO_MAGIC;
    hdr.version = HALEY_PROTO_VERSION;
    hdr.msg_type = msg_type;
    hdr.flags = flags;
    hdr.sequence = seq;
    hdr.payload_length = payload_len;
    hdr.crc32 = PKT_ComputeCRC(&hdr, payload, payload_len);

    PKT_SerializeHeader(out_buf, &hdr);
    if (payload && payload_len > 0) {
        memcpy(out_buf + HALEY_PROTO_HEADER_SIZE, payload, payload_len);
    }

    return HALEY_PROTO_HEADER_SIZE + payload_len;
}

size_t PKT_BuildHeartbeat(uint8_t *out_buf, size_t buf_size, uint32_t seq, int is_request) {
    haley_pkt_flags_t flags = is_request ? HALEY_FLAG_REQUEST : HALEY_FLAG_RESPONSE;
    return PKT_Build(out_buf, buf_size, HALEY_MSG_HEARTBEAT, seq, flags, NULL, 0);
}

size_t PKT_BuildACK(uint8_t *out_buf, size_t buf_size, uint32_t seq) {
    return PKT_Build(out_buf, buf_size, HALEY_MSG_ACK, seq, HALEY_FLAG_NONE, NULL, 0);
}

size_t PKT_BuildCommand(uint8_t *out_buf, size_t buf_size, uint32_t seq,
                        const uint8_t *kv_payload, uint32_t kv_len, haley_pkt_flags_t flags) {
    return PKT_Build(out_buf, buf_size, HALEY_MSG_COMMAND, seq, flags, kv_payload, kv_len);
}

size_t PKT_BuildTelemetry(uint8_t *out_buf, size_t buf_size, uint32_t seq,
                          const uint8_t *kv_payload, uint32_t kv_len) {
    return PKT_Build(out_buf, buf_size, HALEY_MSG_TELEMETRY, seq, HALEY_FLAG_NONE, kv_payload, kv_len);
}

size_t PKT_BuildFault(uint8_t *out_buf, size_t buf_size, uint32_t seq,
                      const uint8_t *kv_payload, uint32_t kv_len) {
    return PKT_Build(out_buf, buf_size, HALEY_MSG_FAULT, seq, HALEY_FLAG_EMERGENCY, kv_payload, kv_len);
}

size_t PKT_BuildVersionNegotiation(uint8_t *out_buf, size_t buf_size, uint32_t seq) {
    char payload[HALEY_PROTO_MAX_KV_PAYLOAD];
    PKT_KV_Init(payload, sizeof(payload));
    PKT_KV_Add(payload, sizeof(payload), HALEY_KV_NAME, HALEY_PROTO_DEVICE_NAME);
    PKT_KV_AddUInt(payload, sizeof(payload), HALEY_KV_PROTOCOL_VERSION, HALEY_PROTO_VERSION);
    PKT_KV_AddUInt(payload, sizeof(payload), HALEY_KV_AUDIO_SAMPLE_RATE, HALEY_PROTO_AUDIO_SAMPLE_RATE_HZ);
    PKT_KV_AddUInt(payload, sizeof(payload), HALEY_KV_AUDIO_CHANNELS, HALEY_PROTO_AUDIO_CHANNELS);

    size_t len = PKT_KV_Length(payload);
    return PKT_Build(out_buf, buf_size, HALEY_MSG_VERSION_NEGOTIATION, seq, HALEY_FLAG_REQUEST, (const uint8_t*)payload, len);
}

size_t PKT_BuildState(uint8_t *out_buf, size_t buf_size, uint32_t seq,
                      const uint8_t *kv_payload, uint32_t kv_len) {
    return PKT_Build(out_buf, buf_size, HALEY_MSG_STATE, seq, HALEY_FLAG_NONE, kv_payload, kv_len);
}

/* -----------------------------------------------------------------------
 * Key-value text payload helpers
 * ----------------------------------------------------------------------- */

void PKT_KV_Init(char *buf, size_t buf_size) {
    if (buf_size > 0) buf[0] = '\0';
}

haley_proto_err_t PKT_KV_Add(char *buf, size_t buf_size, const char *key, const char *value) {
    size_t current_len = strlen(buf);
    size_t required = strlen(key) + strlen(value) + 2;

    if (current_len + required >= buf_size) return HALEY_PROTO_ERR_BUF_SMALL;

    snprintf(buf + current_len, buf_size - current_len, "%s=%s\n", key, value);
    return HALEY_PROTO_OK;
}

haley_proto_err_t PKT_KV_AddUInt(char *buf, size_t buf_size, const char *key, uint32_t value) {
    char val_str[16];
    snprintf(val_str, sizeof(val_str), "%lu", (unsigned long)value);
    return PKT_KV_Add(buf, buf_size, key, val_str);
}

haley_proto_err_t PKT_KV_AddInt(char *buf, size_t buf_size, const char *key, int32_t value) {
    char val_str[16];
    snprintf(val_str, sizeof(val_str), "%ld", (long)value);
    return PKT_KV_Add(buf, buf_size, key, val_str);
}

haley_proto_err_t PKT_KV_AddBool(char *buf, size_t buf_size, const char *key, int value) {
    return PKT_KV_Add(buf, buf_size, key, value ? "true" : "false");
}

size_t PKT_KV_Length(const char *buf) {
    return strlen(buf);
}

haley_proto_err_t PKT_KV_Find(const uint8_t *payload, uint32_t payload_len, const char *key, char *out_val, size_t out_val_size) {
    if (!payload || payload_len == 0 || !key || !out_val || out_val_size == 0) return HALEY_PROTO_ERR_NULL_PTR;

    size_t key_len = strlen(key);
    const uint8_t *ptr = payload;
    const uint8_t *end = payload + payload_len;

    while (ptr < end) {
        if ((size_t)(end - ptr) > key_len && memcmp(ptr, key, key_len) == 0 && ptr[key_len] == '=') {
            ptr += key_len + 1;
            const uint8_t *val_start = ptr;
            while (ptr < end && *ptr != '\n') ptr++;

            size_t val_len = ptr - val_start;
            if (val_len >= out_val_size) return HALEY_PROTO_ERR_KV_TRUNCATED;

            memcpy(out_val, val_start, val_len);
            out_val[val_len] = '\0';
            return HALEY_PROTO_OK;
        }
        while (ptr < end && *ptr != '\n') ptr++;
        if (ptr < end && *ptr == '\n') ptr++;
    }
    return HALEY_PROTO_ERR_KV_NOT_FOUND;
}

haley_proto_err_t PKT_KV_FindUInt(const uint8_t *payload, uint32_t payload_len, const char *key, uint32_t *out_val) {
    char val_buf[16];
    haley_proto_err_t err = PKT_KV_Find(payload, payload_len, key, val_buf, sizeof(val_buf));
    if (err != HALEY_PROTO_OK) return err;

    *out_val = (uint32_t)strtoul(val_buf, NULL, 10);
    return HALEY_PROTO_OK;
}

haley_proto_err_t PKT_KV_FindBool(const uint8_t *payload, uint32_t payload_len, const char *key, int *out_val) {
    char val_buf[16];
    haley_proto_err_t err = PKT_KV_Find(payload, payload_len, key, val_buf, sizeof(val_buf));
    if (err != HALEY_PROTO_OK) return err;

    if (strcmp(val_buf, "true") == 0 || strcmp(val_buf, "1") == 0 ||
        strcmp(val_buf, "yes") == 0 || strcmp(val_buf, "on") == 0) {
        *out_val = 1;
    } else {
        *out_val = 0;
    }
    return HALEY_PROTO_OK;
}
