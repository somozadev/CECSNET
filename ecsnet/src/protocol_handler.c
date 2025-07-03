#include "protocol_handler.h"
#include <string.h>
#include <stdio.h>

int pack_message(const message_t* message, uint8_t* out_buffer, int max_len) {
    if (!message || !out_buffer || max_len < 3 + message->size) return -1;
    out_buffer[0] = (uint8_t)message->type;
    out_buffer[1] = (message->size >> 8) & 0xFF;
    out_buffer[2] = message->size & 0xFF;
    memcpy(&out_buffer[3], message->payload, message->size);
    return 3 + message->size;
}

int unpack_message(const uint8_t* buffer, int length, message_t* out_message) {
    if (!buffer || length < 3 || !out_message) return -1;
    out_message->type = (message_type_t)buffer[0];
    out_message->size = (buffer[1] << 8) | buffer[2];
    if (out_message->size > MAX_PAYLOAD_SIZE || (3 + out_message->size) > length) return -2;
    memcpy(out_message->payload, &buffer[3], out_message->size);
    return 0;
}

bool validate_message(const message_t* message) {
    if (!message) return false;
    if (message->type < MSG_TYPE_INPUT || message->type > MSG_TYPE_ACK) return false;
    if (message->size > MAX_PAYLOAD_SIZE) return false;
    return true;
}

void handle_message(const message_t* message) {
    if (!message || !validate_message(message)) return;
    switch (message->type) {
        case MSG_TYPE_INPUT:
            printf("[ProtocolHandler] Received input message (%d bytes)\n", message->size);
            break;
        case MSG_TYPE_SNAPSHOT:
            printf("[ProtocolHandler] Received snapshot message (%d bytes)\n", message->size);
            break;
        case MSG_TYPE_ACK:
            printf("[ProtocolHandler] Received ACK\n");
            break;
        default:
            printf("[ProtocolHandler] Unknown message type\n");
            break;
    }
}
