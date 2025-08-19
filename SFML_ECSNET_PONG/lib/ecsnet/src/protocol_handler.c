#include "protocol_handler.h"
#include <string.h>
#include <stdio.h>

void protocol_handler_init(protocol_handler_t* handler) {
    if (!handler) return;
    // Clear the entire handler structure to a known zero state.
    memset(handler, 0, sizeof(protocol_handler_t));
}

void protocol_handler_pack_entity_update(protocol_handler_t* handler, entity_t entity_id, const uint8_t* data, uint16_t data_len) {
    if (!handler || data_len > sizeof(handler->out_packet.data)) return;

    handler->out_packet.header.type = PACKET_TYPE_ENTITY_UPDATE;
    // The total size is the header size + entity ID size + the serialized data length.
    handler->out_packet.header.size = sizeof(packet_header_t) + sizeof(entity_t) + data_len;

    // Copy the entity ID into the data payload.
    memcpy(handler->out_packet.data, &entity_id, sizeof(entity_t));

    // Copy the serialized component data immediately after the entity ID.
    memcpy(handler->out_packet.data + sizeof(entity_t), data, data_len);
}

void protocol_handler_pack_client_register(protocol_handler_t* handler, uint16_t udp_port) {
    if (!handler) return;

    handler->out_packet.header.type = PACKET_TYPE_CLIENT_REGISTER;
    // The total size is the header size + the UDP port size.
    handler->out_packet.header.size = sizeof(packet_header_t) + sizeof(uint16_t);

    // Copy the UDP port into the data payload.
    memcpy(handler->out_packet.data, &udp_port, sizeof(uint16_t));
}

void protocol_handler_pack_server_ack(protocol_handler_t* handler) {
    if (!handler) return;

    handler->out_packet.header.type = PACKET_TYPE_SERVER_ACK;
    // The total size is just the header size, as there is no payload.
    handler->out_packet.header.size = sizeof(packet_header_t);
}

void protocol_handler_process_received_data(protocol_handler_t* handler, peer_t* peer, const void* data, int len) {
    if (!handler || !data || len <= sizeof(packet_header_t)) return;

    const network_packet_t* packet = (const network_packet_t*)data;

    printf("[ProtocolHandler] Processing packet of type %d from peer %s\n", packet->header.type, peer->id);

    switch (packet->header.type) {
        case PACKET_TYPE_ENTITY_UPDATE: {
            printf("[ProtocolHandler] Received entity update. Not yet implemented.\n");
            // The logic to deserialize the entity update would go here.
            break;
        }
        case PACKET_TYPE_CLIENT_REGISTER: {
            if (len >= sizeof(packet_header_t) + sizeof(uint16_t)) {
                uint16_t udp_port;
                // Read the UDP port from the packet's data payload.
                memcpy(&udp_port, packet->data, sizeof(uint16_t));

                // Log the extracted information. The actual logic is handled elsewhere.
                printf("[ProtocolHandler] Client %s sent UDP port %hu. Waiting for Network_cs to handle the logic.\n", peer->id, udp_port);
            }
            break;
        }
        case PACKET_TYPE_SERVER_ACK: {
            printf("[ProtocolHandler] Server ACK received. Connection is fully established.\n");
            break;
        }
        default:
            printf("[ProtocolHandler] Unknown packet type %d.\n", packet->header.type);
            break;
    }
}
void protocol_handler_send_packet(connection_manager_t* cm, const char* peer_id, protocol_handler_t* handler) {
    // Delegate the actual sending to the connection manager.
    connection_manager_send_to_peer(cm, peer_id, &handler->out_packet, handler->out_packet.header.size);
}