#include "protocol_handler.h"
#include <string.h>
#include <stdio.h>

#include "ecs_internal.h"

void protocol_handler_init(protocol_handler_t *handler) {
    if (!handler) return;
    // Clear the entire handler structure to a known zero state.
    memset(handler, 0, sizeof(protocol_handler_t));
}

void protocol_handler_pack_entity_update(protocol_handler_t *handler, entity_t entity_id, const uint8_t *data,
                                         uint16_t data_len) {
    if (!handler || data_len > sizeof(handler->out_packet.data)) return;

    handler->out_packet.header.type = PACKET_TYPE_ENTITY_UPDATE;
    // The total size is the header size + entity ID size + the serialized data length.
    handler->out_packet.header.size = sizeof(packet_header_t) + sizeof(entity_t) + data_len;

    // Copy the entity ID into the data payload.
    memcpy(handler->out_packet.data, &entity_id, sizeof(entity_t));

    // Copy the serialized component data immediately after the entity ID.
    memcpy(handler->out_packet.data + sizeof(entity_t), data, data_len);
}

void protocol_handler_pack_client_register(protocol_handler_t *handler, uint16_t udp_port) {
    if (!handler) return;

    handler->out_packet.header.type = PACKET_TYPE_CLIENT_REGISTER;
    // The total size is the header size + the UDP port size.
    handler->out_packet.header.size = sizeof(packet_header_t) + sizeof(uint16_t);

    // Copy the UDP port into the data payload.
    memcpy(handler->out_packet.data, &udp_port, sizeof(uint16_t));
}

void protocol_handler_pack_server_ack(protocol_handler_t *handler) {
    if (!handler) return;

    handler->out_packet.header.type = PACKET_TYPE_SERVER_ACK;
    // The total size is just the header size, as there is no payload.
    handler->out_packet.header.size = sizeof(packet_header_t);
}

void protocol_handler_pack_client_input(protocol_handler_t *handler, entity_t entity_id, uint8_t input_cmd,
                                        const void *extra, uint16_t extra_len) {
    if (!handler) return;
    if (extra_len > sizeof(handler->out_packet.data)) extra_len = (uint16_t) sizeof(handler->out_packet.data);

    // [entity_t][uint8_t cmd][extra...]
    uint8_t *p = handler->out_packet.data;
    memcpy(p, &entity_id, sizeof(entity_t));
    p += sizeof(entity_t);
    memcpy(p, &input_cmd, sizeof(uint8_t));
    p += sizeof(uint8_t);

    if (extra && extra_len) {
        if ((size_t) (p - handler->out_packet.data) + extra_len > sizeof(handler->out_packet.data))
            extra_len = (uint16_t) (sizeof(handler->out_packet.data) - (p - handler->out_packet.data));
        memcpy(p, extra, extra_len);
        p += extra_len;
    }

    handler->out_packet.header.type = PACKET_TYPE_CLIENT_INPUT;
    handler->out_packet.header.size = (uint16_t) (sizeof(packet_header_t) + (p - handler->out_packet.data));
}

void protocol_handler_process_received_data(protocol_handler_t *handler, ecs_t *ecs, peer_t *peer, const void *data,
                                            int len) {
    if (!handler || !data || len <= sizeof(packet_header_t)) return;

    const network_packet_t *packet = (const network_packet_t *) data;

    printf("[ProtocolHandler] Processing packet of type %d from peer %s\n", packet->header.type, peer->id);

    switch (packet->header.type) {
        case PACKET_TYPE_ENTITY_UPDATE: {
            printf("[ProtocolHandler] Received entity update. Not yet implemented.\n");
            // The logic to deserialize the entity update would go here.
            const uint8_t *p = packet->data;
            const uint8_t *end = ((const uint8_t *) packet) + packet->header.size;

            entity_t wire_eid;
            memcpy(&wire_eid, p, sizeof(entity_t));
            p += sizeof(entity_t);

            entity_t e = wire_eid; // V1.0 assuming entity id is the same in client and server
            while (p < end) {
                component_t cid;
                memcpy(&cid, p, sizeof(component_t));
                p += sizeof(component_t);


                size_t comp_size = ecs->components[cid].descriptor.size;
                void *dst = ecs_get_component(ecs, e, cid);
                if (dst) {
                    memcpy(dst, p, comp_size);
                    ecs_mark_component_dirty(ecs, e, cid);
                }
                p += comp_size;
            }

            break;
        }
        case PACKET_TYPE_CLIENT_REGISTER: {
            if (len >= sizeof(packet_header_t) + sizeof(uint16_t)) {
                uint16_t udp_port;
                // Read the UDP port from the packet's data payload.
                memcpy(&udp_port, packet->data, sizeof(uint16_t));

                // Log the extracted information. The actual logic is handled elsewhere.
                printf("[ProtocolHandler] Client %s sent UDP port %hu. Waiting for Network_cs to handle the logic.\n",
                       peer->id, udp_port);
            }
            break;
        }
        case PACKET_TYPE_SERVER_ACK: {
            printf("[ProtocolHandler] Server ACK received. Connection is fully established.\n");
            break;
        }

        case PACKET_TYPE_CLIENT_INPUT: {
            const uint8_t* p = packet->data;
            const uint8_t* end = ((const uint8_t*)packet) + packet->header.size;

            if (end - p < (ptrdiff_t)(sizeof(entity_t) + sizeof(uint8_t))) {
                printf("[ProtocolHandler] CLIENT_INPUT too small\n");
                break;
            }

            entity_t eid; uint8_t cmd;
            memcpy(&eid, p, sizeof(entity_t)); p += sizeof(entity_t);
            memcpy(&cmd,  p, sizeof(uint8_t));  p += sizeof(uint8_t);

            printf("[ProtocolHandler] CLIENT_INPUT eid=%u cmd=0x%02X from %s\n",
                   eid, cmd, peer ? peer->id : "(null)");

            if (cmd == INPUT_SPAWN) {
                if (end - p < (ptrdiff_t)sizeof(float)*2) {
                    printf("[ProtocolHandler] SPAWN missing xy\n");
                    break;
                }
                float x,y;
                memcpy(&x, p, sizeof(float)); p += sizeof(float);
                memcpy(&y, p, sizeof(float)); p += sizeof(float);

                position_t pos = { x, y };
                velocity_t vel = { 0.f, 120.f };

                entity_t e = ecs_create_entity(ecs);
                ecs_add_component(ecs, e, COMPONENT_POSITION, &pos);
                ecs_add_component(ecs, e, COMPONENT_VELOCITY, &vel);
                ecs_mark_component_dirty(ecs, e, COMPONENT_POSITION);
                ecs_mark_component_dirty(ecs, e, COMPONENT_VELOCITY);

                printf("[ProtocolHandler] SPAWN ok -> entity %u at (%.1f, %.1f)\n", e, x, y);
            }
            break;
        }
        default:
            printf("[ProtocolHandler] Unknown packet type %d.\n", packet->header.type);
            break;
    }
}

void protocol_handler_send_packet(connection_manager_t *cm, const char *peer_id, protocol_handler_t *handler) {
    // Delegate the actual sending to the connection manager.
    connection_manager_send_to_peer(cm, peer_id, &handler->out_packet, handler->out_packet.header.size);
}
