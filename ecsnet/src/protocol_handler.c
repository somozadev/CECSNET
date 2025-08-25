#include "protocol_handler.h"
#include "network_architecture.h"
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
bool protocol_handler_unpack_client_register(const network_packet_t* pkt, uint16_t* out_port) {
    if (!pkt || pkt->header.type != PACKET_TYPE_CLIENT_REGISTER) return false;
    if (pkt->header.size < sizeof(packet_header_t) + sizeof(uint16_t)) return false;
    memcpy(out_port, pkt->data, sizeof(uint16_t));
    return true;
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

void protocol_handler_process_received_data(protocol_handler_t *handler,ecs_t *ecs,peer_t *peer,const void *data,int len)
{
    if (!handler || !ecs || !data || len < (int)sizeof(packet_header_t)) return;

    const network_packet_t *packet = (const network_packet_t *)data;
    const uint8_t *cur = packet->data;
    const uint8_t *end = ((const uint8_t*)packet) + packet->header.size;

    printf("[ProtocolHandler] Processing packet of type %d from peer %s\n",
           packet->header.type, peer && peer->id ? peer->id : "(null)");

    switch (packet->header.type) {

    // ========= SNAPSHOT/DELTA: una entidad por paquete =========
    case PACKET_TYPE_ENTITY_UPDATE: {
        if (end - cur < (ptrdiff_t)sizeof(entity_t)) break;
        entity_t eid; memcpy(&eid, cur, sizeof(eid)); cur += sizeof(eid);

        // Asegura existencia de la entidad en el cliente
        ecs_try_create_entity_by_id(ecs, eid);

        // Stream de [component_t][blob]
        while (end - cur >= (ptrdiff_t)sizeof(component_t)) {
            component_t cid;
            memcpy(&cid, cur, sizeof(cid)); cur += sizeof(cid);

            // Blindaje de índice de componente
            if (cid < 0 || cid >= ecs->registered_component_count) {
                printf("[PH] Skip invalid component id %d\n", (int)cid);
                break;
            }

            size_t comp_size = ecs->components[cid].descriptor.size;
            if (end - cur < (ptrdiff_t)comp_size) {
                printf("[PH] Truncated component payload (cid=%d)\n", (int)cid);
                break;
            }

            if (!ecs_has_component(ecs, eid, cid)) {
                // No existía -> créalo con los datos recibidos
                ecs_add_component(ecs, eid, cid, (void*)cur);
            } else {
                // Ya existía -> sobreescribe datos
                void* dst = ecs_get_component(ecs, eid, cid);
                if (dst) memcpy(dst, cur, comp_size);
            }
            // (Opcional) marcar dirty local si tu render depende de ello
            ecs_mark_component_dirty(ecs, eid, cid);

            cur += comp_size;
        }
        break;
    }

    // ========= SNAPSHOT/DELTA: paquete con muchas entidades =========
    case PACKET_TYPE_MULTI_ENTITY_UPDATE: {
        if (end - cur < (ptrdiff_t)sizeof(uint16_t)) break;
        uint16_t entity_count; memcpy(&entity_count, cur, sizeof(entity_count));
        cur += sizeof(entity_count);

        for (uint16_t i = 0; i < entity_count; ++i) {
            if (end - cur < (ptrdiff_t)sizeof(entity_t)) { printf("[PH] Truncated eid\n"); break; }
            entity_t eid; memcpy(&eid, cur, sizeof(eid)); cur += sizeof(eid);

            ecs_try_create_entity_by_id(ecs, eid);

            if (end - cur < (ptrdiff_t)sizeof(uint8_t)) { printf("[PH] Truncated comp_count\n"); break; }
            uint8_t comp_count; memcpy(&comp_count, cur, sizeof(comp_count)); cur += sizeof(comp_count);

            for (uint8_t c = 0; c < comp_count; ++c) {
                if (end - cur < (ptrdiff_t)sizeof(component_t)) { printf("[PH] Truncated cid\n"); break; }
                component_t cid; memcpy(&cid, cur, sizeof(cid)); cur += sizeof(cid);

                if (cid < 0 || cid >= ecs->registered_component_count) {
                    printf("[PH] Skip invalid component id %d\n", (int)cid);
                    return;
                }

                size_t comp_size = ecs->components[cid].descriptor.size;
                if (end - cur < (ptrdiff_t)comp_size) { printf("[PH] Truncated comp payload\n"); return; }

                if (!ecs_has_component(ecs, eid, cid)) {
                    ecs_add_component(ecs, eid, cid, (void*)cur);
                } else {
                    void* dst = ecs_get_component(ecs, eid, cid);
                    if (dst) memcpy(dst, cur, comp_size);
                }
                ecs_mark_component_dirty(ecs, eid, cid);
                cur += comp_size;
            }
        }
        break;
    }

    // ========= Handshake =========
    case PACKET_TYPE_SERVER_ACK: {
        printf("[ProtocolHandler] Server ACK received. Connection is fully established.\n");
        break;
    }
    case PACKET_TYPE_CLIENT_REGISTER: {
    if (len >= (int)(sizeof(packet_header_t) + sizeof(uint16_t))) {
        uint16_t udp_port;
        memcpy(&udp_port, packet->data, sizeof(uint16_t));
        printf("[PH] CLIENT_REGISTER from %s udp=%hu (network_cs will wire UDP)\n",
               peer && peer->id ? peer->id : "(null)", udp_port);
    }
    break;
    }
        // case PACKET_TYPE_CLIENT_INPUT: {
        //
        // const uint8_t* p = packet->data;
        // const uint8_t* end = ((const uint8_t*)packet) + packet->header.size;
        //
        // if (end - p < (ptrdiff_t)(sizeof(entity_t) + sizeof(uint8_t))) {
        //     printf("[PH] CLIENT_INPUT too small\n");
        //     break;
        // }
        // entity_t eid; uint8_t cmd;
        // memcpy(&eid, p, sizeof(entity_t)); p += sizeof(entity_t);
        // memcpy(&cmd,  p, sizeof(uint8_t));  p += sizeof(uint8_t);
        //
        // size_t payload_size = end - p;
        // const uint8_t *payload = p;
        // if (handler->arch && handler->arch->config.on_client_input)
        //     handler->arch->config.on_client_input(handler->arch->config.user_data,peer,eid,cmd,payload,payload_size);
        // else {
        //     printf("[PH] CLIENT_INPUT (cmd=%u) received but no handler set\n", cmd);
        // }break;
        //
        // if (cmd == INPUT_SPAWN) {
        //     if (end - p < (ptrdiff_t)(sizeof(float)*2)) { printf("[PH] SPAWN missing xy\n"); break; }
        //     float x,y; memcpy(&x,p,sizeof(float)); p+=sizeof(float); memcpy(&y,p,sizeof(float)); p+=sizeof(float);
        //
        //     position_t pos = { x, y };
        //     velocity_t vel = { 0.f, 120.f };
        //     entity_t e = ecs_create_entity(ecs);
        //     ecs_add_component(ecs, e, COMPONENT_POSITION, &pos);
        //     ecs_add_component(ecs, e, COMPONENT_VELOCITY, &vel);
        //     ecs_mark_component_dirty(ecs, e, COMPONENT_POSITION);
        //     ecs_mark_component_dirty(ecs, e, COMPONENT_VELOCITY);
        //
        //     printf("[PH] SPAWN ok -> entity %u at (%.1f, %.1f)\n", e, x, y);
        // }
        // break;
        // }

    default:
        printf("[ProtocolHandler] Unknown packet type %d.\n", packet->header.type);
        break;
    }
}

void protocol_handler_send_packet(connection_manager_t *cm, const char *peer_id, protocol_handler_t *handler) {
    // Delegate the actual sending to the connection manager.
    connection_manager_send_to_peer(cm, peer_id, &handler->out_packet, handler->out_packet.header.size);
}
