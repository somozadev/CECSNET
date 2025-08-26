#include "protocol_handler.h"
#include "network_architecture.h"
#include <string.h>
#include <stdio.h>

#include "ecs_internal.h"
/**
 * @brief Initialize a protocol handler structure.
 *
 * Clears all fields to zero, preparing it for use.
 * @param handler Pointer to the protocol handler to initialize.
 */
ECSNET_API void protocol_handler_init(protocol_handler_t *handler) {
    if (!handler) return;
    // Clear the entire handler structure to a known zero state.
    memset(handler, 0, sizeof(protocol_handler_t));
}
/**
 * @brief Pack an ENTITY_UPDATE packet for a given entity.
 *
 * Format: [entity_t][component data...]
 * @param handler   Protocol handler to fill.
 * @param entity_id Entity being updated.
 * @param data      Serialized component payload.
 * @param data_len  Length of the payload.
 */
ECSNET_API void protocol_handler_pack_entity_update(protocol_handler_t *handler, entity_t entity_id, const uint8_t *data,
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
/**
 * @brief Pack a CLIENT_REGISTER packet containing the client's UDP port.
 *
 * Sent by the client to the server after TCP connect, to advertise
 * which UDP port it listens on.
 */
void protocol_handler_pack_client_register(protocol_handler_t *handler, uint16_t udp_port) {
    if (!handler) return;

    handler->out_packet.header.type = PACKET_TYPE_CLIENT_REGISTER;
    // The total size is the header size + the UDP port size.
    handler->out_packet.header.size = sizeof(packet_header_t) + sizeof(uint16_t);

    // Copy the UDP port into the data payload.
    memcpy(handler->out_packet.data, &udp_port, sizeof(uint16_t));
}
/**
 * @brief Parse a CLIENT_REGISTER packet.
 * @param pkt      The received packet.
 * @param out_port Output pointer for UDP port.
 * @return true if unpacked successfully, false otherwise.
 */
bool protocol_handler_unpack_client_register(const network_packet_t* pkt, uint16_t* out_port) {
    if (!pkt || pkt->header.type != PACKET_TYPE_CLIENT_REGISTER) return false;
    if (pkt->header.size < sizeof(packet_header_t) + sizeof(uint16_t)) return false;
    memcpy(out_port, pkt->data, sizeof(uint16_t));
    return true;
}
/**
 * @brief Pack a SERVER_ACK packet.
 *
 * This packet contains only the header (no payload) and is sent by
 * the server to confirm a successful connection.
 */
void protocol_handler_pack_server_ack(protocol_handler_t *handler) {
    if (!handler) return;

    handler->out_packet.header.type = PACKET_TYPE_SERVER_ACK;
    // The total size is just the header size, as there is no payload.
    handler->out_packet.header.size = sizeof(packet_header_t);
}
/**
 * @brief Pack a CLIENT_INPUT packet.
 *
 * Format: [entity_t][uint8_t cmd][optional payload...]
 * @param handler    Protocol handler to fill.
 * @param entity_id  The entity issuing the command.
 * @param input_cmd  Command identifier.
 * @param extra      Optional extra payload.
 * @param extra_len  Length of extra payload.
 */
ECSNET_API void protocol_handler_pack_client_input(protocol_handler_t *handler, entity_t entity_id, uint8_t input_cmd,
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
/**
 * @brief Process incoming network packet data.
 *
 * Handles ENTITY_UPDATE, MULTI_ENTITY_UPDATE, SERVER_ACK, CLIENT_REGISTER,
 * and forwards unknown packets.
 * Updates ECS state accordingly (create, update, or dirty components).
 *
 * @param handler Protocol handler context.
 * @param ecs     ECS world instance.
 * @param peer    Peer that sent the packet.
 * @param data    Raw packet data.
 * @param len     Packet length.
 */
void protocol_handler_process_received_data(protocol_handler_t *handler,ecs_t *ecs,peer_t *peer,const void *data,int len)
{
    if (!handler || !ecs || !data || len < (int)sizeof(packet_header_t)) return;

    const network_packet_t *packet = (const network_packet_t *)data;
    const uint8_t *cur = packet->data;
    const uint8_t *end = ((const uint8_t*)packet) + packet->header.size;

    printf("[ProtocolHandler] Processing packet of type %d from peer %s\n",
           packet->header.type, peer && peer->id ? peer->id : "(null)");

    switch (packet->header.type) {

    // ========= SNAPSHOT/DELTA: One entity per packet =========
    case PACKET_TYPE_ENTITY_UPDATE: {
        if (end - cur < (ptrdiff_t)sizeof(entity_t)) break;
        entity_t eid; memcpy(&eid, cur, sizeof(eid)); cur += sizeof(eid);

        // Assures entity exists in client side
        ecs_try_create_entity_by_id(ecs, eid);

        // Stream of [component_t][blob]
        while (end - cur >= (ptrdiff_t)sizeof(component_t)) {
            component_t cid;
            memcpy(&cid, cur, sizeof(cid)); cur += sizeof(cid);

            // Validate component index
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
                // Did not exist -> create with received data
                ecs_add_component(ecs, eid, cid, (void*)cur);
            } else {
                // Already exists -> overwrite data
                void* dst = ecs_get_component(ecs, eid, cid);
                if (dst) memcpy(dst, cur, comp_size);
            }
            // Optionally mark dirty if local rendering depends on it
            ecs_mark_component_dirty(ecs, eid, cid);

            cur += comp_size;
        }
        break;
    }

    // ========= SNAPSHOT/DELTA: Packets with a lot of entities =========
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

    default:
        printf("[ProtocolHandler] Unknown packet type %d.\n", packet->header.type);
        break;
    }
}
/**
 * @brief Send the prepared out_packet to a specific peer.
 *
 * Delegates to the connection manager for transport.
 */
ECSNET_API void protocol_handler_send_packet(connection_manager_t *cm, const char *peer_id, protocol_handler_t *handler) {
    connection_manager_send_to_peer(cm, peer_id, &handler->out_packet, handler->out_packet.header.size);
}
