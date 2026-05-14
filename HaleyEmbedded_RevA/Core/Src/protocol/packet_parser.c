/*
 * packet_parser.c
 *
 * Implementation of the byte-stream receive state machine.
 */

#include "protocol/packet_parser.h"
#include "protocol/packet_protocol.h"
#include <string.h>

haley_proto_err_t PARSER_Init(haley_parser_t *parser, haley_parser_rx_cb_t on_packet,
                              haley_parser_err_cb_t on_error, void *user_ctx) {
    if (!parser || !on_packet) return HALEY_PROTO_ERR_NULL_PTR;

    memset(parser, 0, sizeof(haley_parser_t));
    parser->on_packet = on_packet;
    parser->on_error = on_error;
    parser->user_ctx = user_ctx;
    parser->state = PARSER_STATE_WAIT_MAGIC_0;

    return HALEY_PROTO_OK;
}

void PARSER_Reset(haley_parser_t *parser) {
    if (!parser) return;

    parser->state = PARSER_STATE_WAIT_MAGIC_0;
    parser->hdr_bytes = 0;
    parser->payload_bytes = 0;
    parser->payload_expected = 0;
}

void PARSER_FeedByte(haley_parser_t *parser, uint8_t byte) {
    if (!parser) return;

    switch (parser->state) {
        case PARSER_STATE_WAIT_MAGIC_0:
            if (byte == 0x41) {
                parser->hdr_buf[0] = byte;
                parser->hdr_bytes = 1;
                parser->state = PARSER_STATE_WAIT_MAGIC_1;
            }
            break;

        case PARSER_STATE_WAIT_MAGIC_1:
            if (byte == 0x48) {
                parser->hdr_buf[1] = byte;
                parser->hdr_bytes = 2;
                parser->state = PARSER_STATE_RECV_HEADER;
            } else if (byte == 0x41) {
                parser->hdr_buf[0] = byte;
            } else {
                parser->stat_framing_errors++;
                if (parser->on_error) parser->on_error(HALEY_PROTO_ERR_BAD_MAGIC, parser->user_ctx);
                parser->state = PARSER_STATE_WAIT_MAGIC_0;
            }
            break;

        case PARSER_STATE_RECV_HEADER:
            parser->hdr_buf[parser->hdr_bytes++] = byte;
            if (parser->hdr_bytes == HALEY_PROTO_HEADER_SIZE) {
                haley_proto_err_t err = PKT_ParseHeader(parser->hdr_buf, &parser->current_hdr);
                if (err != HALEY_PROTO_OK) {
                    parser->stat_framing_errors++;
                    if (parser->on_error) parser->on_error(err, parser->user_ctx);
                    PARSER_Reset(parser);
                } else if (parser->current_hdr.payload_length > HALEY_PROTO_MAX_PAYLOAD) {
                    parser->stat_framing_errors++;
                    if (parser->on_error) parser->on_error(HALEY_PROTO_ERR_BAD_LENGTH, parser->user_ctx);
                    PARSER_Reset(parser);
                } else {
                    parser->payload_expected = parser->current_hdr.payload_length;
                    parser->payload_bytes = 0;
                    if (parser->payload_expected == 0) {
                        parser->state = PARSER_STATE_VALIDATE_CRC;
                        PARSER_FeedByte(parser, 0);
                    } else {
                        parser->state = PARSER_STATE_RECV_PAYLOAD;
                    }
                }
            }
            break;

        case PARSER_STATE_RECV_PAYLOAD:
            parser->payload_buf[parser->payload_bytes++] = byte;
            if (parser->payload_bytes == parser->payload_expected) {
                parser->state = PARSER_STATE_VALIDATE_CRC;
                PARSER_FeedByte(parser, 0);
            }
            break;

        case PARSER_STATE_VALIDATE_CRC:
        {
            haley_proto_err_t crc_err = PKT_VerifyCRC(&parser->current_hdr,
                                                      parser->payload_expected ? parser->payload_buf : NULL,
                                                      parser->payload_expected);
            if (crc_err == HALEY_PROTO_OK) {
                parser->stat_packets_ok++;
                haley_packet_t pkt;
                pkt.header = parser->current_hdr;
                if (parser->payload_expected > 0) {
                    memcpy(pkt.payload, parser->payload_buf, parser->payload_expected);
                }
                pkt.valid = 1;
                if (parser->on_packet) parser->on_packet(&pkt, parser->user_ctx);
            } else {
                parser->stat_crc_errors++;
                if (parser->on_error) parser->on_error(HALEY_PROTO_ERR_BAD_CRC, parser->user_ctx);
            }
            PARSER_Reset(parser);
            break;
        }
    }
}

void PARSER_Feed(haley_parser_t *parser, const uint8_t *data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        PARSER_FeedByte(parser, data[i]);
    }
}

haley_parser_state_t PARSER_GetState(const haley_parser_t *parser) {
    return parser ? parser->state : PARSER_STATE_WAIT_MAGIC_0;
}

uint32_t PARSER_GetPacketCount(const haley_parser_t *parser) {
    return parser ? parser->stat_packets_ok : 0;
}

uint32_t PARSER_GetCRCErrorCount(const haley_parser_t *parser) {
    return parser ? parser->stat_crc_errors : 0;
}

uint32_t PARSER_GetFramingErrorCount(const haley_parser_t *parser) {
    return parser ? parser->stat_framing_errors : 0;
}
