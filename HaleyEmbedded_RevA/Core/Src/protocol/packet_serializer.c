/*
 * packet_serializer.c
 *
 * Implementation of the stateful Haley packet serializer.
 */

#include "protocol/packet_serializer.h"
#include <string.h>

/* Internal helper to increment sequence and dispatch to the callback */
static haley_proto_err_t dispatch_packet(haley_serializer_t *srl, size_t length) {
    if (length == 0) {
        srl->stat_tx_errors++;
        return HALEY_PROTO_ERR_BUF_SMALL;
    }

    /* Dispatch to the transport layer (e.g., ethernet_driver / lwIP) */
    haley_proto_err_t err = srl->tx_cb(srl->tx_buf, length, srl->user_ctx);

    if (err == HALEY_PROTO_OK) {
        srl->stat_tx_packets++;
        srl->stat_tx_bytes += length;

        /* Increment sequence number, avoiding the reserved 0 value */
        srl->next_seq++;
        if (srl->next_seq == HALEY_PROTO_SEQ_RESERVED) {
            srl->next_seq = HALEY_PROTO_SEQ_INIT;
        }
    } else {
        srl->stat_tx_errors++;
    }

    return err;
}

haley_proto_err_t SRL_Init(haley_serializer_t *srl, haley_tx_cb_t tx_cb, void *user_ctx) {
    if (!srl || !tx_cb) {
        return HALEY_PROTO_ERR_NULL_PTR;
    }

    memset(srl, 0, sizeof(haley_serializer_t));
    srl->tx_cb = tx_cb;
    srl->user_ctx = user_ctx;
    srl->next_seq = HALEY_PROTO_SEQ_INIT;

    return HALEY_PROTO_OK;
}

void SRL_ResetSequence(haley_serializer_t *srl) {
    if (srl) {
        srl->next_seq = HALEY_PROTO_SEQ_INIT;
    }
}

haley_proto_err_t SRL_SendHeartbeat(haley_serializer_t *srl, int is_request) {
    if (!srl) return HALEY_PROTO_ERR_NULL_PTR;

    size_t len = PKT_BuildHeartbeat(srl->tx_buf, sizeof(srl->tx_buf),
                                    srl->next_seq, is_request);
    return dispatch_packet(srl, len);
}

haley_proto_err_t SRL_SendACK(haley_serializer_t *srl) {
    if (!srl) return HALEY_PROTO_ERR_NULL_PTR;

    size_t len = PKT_BuildACK(srl->tx_buf, sizeof(srl->tx_buf), srl->next_seq);
    return dispatch_packet(srl, len);
}

haley_proto_err_t SRL_SendCommand(haley_serializer_t *srl, const char *kv_payload, haley_pkt_flags_t flags) {
    if (!srl || !kv_payload) return HALEY_PROTO_ERR_NULL_PTR;

    uint32_t kv_len = (uint32_t)strlen(kv_payload);
    size_t len = PKT_BuildCommand(srl->tx_buf, sizeof(srl->tx_buf),
                                  srl->next_seq, (const uint8_t *)kv_payload,
                                  kv_len, flags);
    return dispatch_packet(srl, len);
}

haley_proto_err_t SRL_SendTelemetry(haley_serializer_t *srl, const char *kv_payload) {
    if (!srl || !kv_payload) return HALEY_PROTO_ERR_NULL_PTR;

    uint32_t kv_len = (uint32_t)strlen(kv_payload);
    size_t len = PKT_BuildTelemetry(srl->tx_buf, sizeof(srl->tx_buf),
                                    srl->next_seq, (const uint8_t *)kv_payload, kv_len);
    return dispatch_packet(srl, len);
}

haley_proto_err_t SRL_SendFault(haley_serializer_t *srl, const char *kv_payload) {
    if (!srl || !kv_payload) return HALEY_PROTO_ERR_NULL_PTR;

    uint32_t kv_len = (uint32_t)strlen(kv_payload);
    size_t len = PKT_BuildFault(srl->tx_buf, sizeof(srl->tx_buf),
                                srl->next_seq, (const uint8_t *)kv_payload, kv_len);
    return dispatch_packet(srl, len);
}

haley_proto_err_t SRL_SendState(haley_serializer_t *srl, const char *kv_payload) {
    if (!srl || !kv_payload) return HALEY_PROTO_ERR_NULL_PTR;

    uint32_t kv_len = (uint32_t)strlen(kv_payload);
    size_t len = PKT_BuildState(srl->tx_buf, sizeof(srl->tx_buf),
                                srl->next_seq, (const uint8_t *)kv_payload, kv_len);
    return dispatch_packet(srl, len);
}

haley_proto_err_t SRL_SendVersionNegotiation(haley_serializer_t *srl) {
    if (!srl) return HALEY_PROTO_ERR_NULL_PTR;

    size_t len = PKT_BuildVersionNegotiation(srl->tx_buf, sizeof(srl->tx_buf), srl->next_seq);
    return dispatch_packet(srl, len);
}

haley_proto_err_t SRL_SendRaw(haley_serializer_t *srl, haley_msg_type_t msg_type,
                              haley_pkt_flags_t flags, const uint8_t *payload, uint32_t payload_len) {
    if (!srl) return HALEY_PROTO_ERR_NULL_PTR;

    size_t len = PKT_Build(srl->tx_buf, sizeof(srl->tx_buf),
                           msg_type, srl->next_seq, flags, payload, payload_len);
    return dispatch_packet(srl, len);
}
