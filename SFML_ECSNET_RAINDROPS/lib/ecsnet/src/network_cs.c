// network_cs.c  — limpio y autocontenido
#include "network_cs.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "ecs.h"
#include "ecs_internal.h"
#include "net_socket.h"
#include "protocol_handler.h"


/*
 * Track which entities have recently been marked dirty.  When any
 * component on an entity is modified on the server, ecsnet_dirty_hook
 * sets the corresponding flag here.  During replication we always
 * process these entities first to reduce perceived lag for players.
 *
 * Note that we do not clear these flags inside the send functions
 * themselves because multiple peers may need to receive the same
 * snapshot of a dirty entity.  Instead, the flags are cleared once
 * per network tick after sending to all peers.
 */
static bool prioritized_entities[MAX_ENTITIES] = { false };

static void ecsnet_dirty_hook(entity_t e) {
    if (e < MAX_ENTITIES) {
        prioritized_entities[e] = true;
    }
}


// ---------- helpers de escritura ----------
static size_t wr_mem(uint8_t* dst, size_t cap, size_t pos, const void* p, size_t n) {
    if (pos + n > cap) return (size_t)-1;
    memcpy(dst + pos, p, n);
    return pos + n;
}
// Encapsula MULTI_ENTITY_UPDATE y envía (UDP si listo, fallback TCP)
static int send_multi_to_peer(connection_manager_t* cm, const char* peer_id, uint8_t* buf, size_t size) {
    if (size > MAX_PACKET_SIZE) return -1;
    packet_header_t* hdr = (packet_header_t*)buf;
    hdr->type = PACKET_TYPE_MULTI_ENTITY_UPDATE;
    hdr->size = (uint16_t)size;

    // Primero UDP si está listo; si falla, TCP
    int s = connection_manager_send_to_peer_udp(cm, peer_id, buf, (int)size);
    if (s < 0) s = connection_manager_send_to_peer(cm, peer_id, buf, (int)size);
    return s;
}

// ---------- snapshot completo por UDP, troceado ----------
/*
 * Send a full snapshot of all networked entities to a single peer.  The
 * payload is chunked into multiple packets that respect a user‑defined
 * soft limit (typically around 1.2 kB).  Each entity is identified by
 * its 32‑bit network_id rather than its local entity index.  Only
 * entities whose NetworkedEntity component intersects the peer's
 * interest_mask are included.  The NetworkedEntity component itself
 * is never serialized, since the network_id implicitly conveys that
 * information to the client.
 */
static void send_full_state_chunked(ecs_t* ecs, connection_manager_t* cm, const char* peer_id, size_t soft_limit) {
    if (!ecs || !cm || !peer_id) return;
    // Clamp the soft limit to stay within the packet buffer.  If the caller
    // passes zero or a value larger than our packet buffer capacity
    // (MAX_PACKET_SIZE), fall back to a value slightly below the maximum
    // capacity to leave room for headers.  Without this clamp, using a
    // larger soft_limit (e.g. 1200) when MAX_PACKET_SIZE is 1024 would
    // cause writes past the end of the buffer and prevent any data from
    // being transmitted.
    if (soft_limit == 0 || soft_limit > MAX_PACKET_SIZE) {
        const size_t margin = 64;
        soft_limit = (MAX_PACKET_SIZE > margin ? MAX_PACKET_SIZE - margin : MAX_PACKET_SIZE);
    }
    peer_t* peer = connection_manager_get_peer(cm, peer_id);
    interest_mask_t mask = peer ? peer->interest_mask : (interest_mask_t)0xFFFFFFFF;

    uint8_t buf[MAX_PACKET_SIZE];
    const size_t cap     = sizeof(buf);
    const size_t hdr_sz  = sizeof(packet_header_t);
    const size_t base_sz = hdr_sz + sizeof(uint16_t); // entity_count

    size_t pos  = base_sz;
    uint16_t ents = 0;

    for (entity_t e = 0; e < MAX_ENTITIES; ++e) {
        // Only consider entities with a NetworkedEntity component
        networked_entity_t* ne = NULL;
        if (ecs_has_component(ecs, e, COMPONENT_NETWORKED_ENTITY)) {
            ne = ecs_get_component(ecs, e, COMPONENT_NETWORKED_ENTITY);
        }
        if (!ne) continue;
        // Filter by interest mask
        if ((ne->interest_groups & mask) == 0) continue;

        // Reserve entity header: write network_id (uint32_t)
        size_t ent_pos = pos;
        uint32_t net_id = ne->network_id;
        pos = wr_mem(buf, cap, pos, &net_id, sizeof(uint32_t));
        if (pos == (size_t)-1) goto flush;
        size_t cc_pos = pos;
        if (pos + 1 > cap) goto flush;
        buf[pos++] = 0;
        uint8_t comp_count = 0;

        // Serialize all components except the NetworkedEntity itself
        for (component_t c = 0; c < ecs->registered_component_count; ++c) {
            if (c == COMPONENT_NETWORKED_ENTITY) continue;
            if (!ecs_has_component(ecs, e, c)) continue;
            const void* comp_data = ecs_get_component(ecs, e, c);
            size_t comp_size      = ecs->components[c].descriptor.size;

            // Would this component exceed the soft limit?  If so, flush now.
            if (pos + sizeof(component_t) + comp_size > soft_limit) {
                if (comp_count == 0) {
                    pos = ent_pos;
                    goto flush;
                }
                buf[cc_pos] = comp_count;
                ents++;
                goto flush;
            }

            pos = wr_mem(buf, cap, pos, &c, sizeof(component_t));
            pos = wr_mem(buf, cap, pos, comp_data, comp_size);
            if (pos == (size_t)-1) { pos = ent_pos; goto flush; }
            comp_count++;
        }

        if (comp_count == 0) {
            // Nothing to send for this entity
            pos = ent_pos;
            continue;
        }
        buf[cc_pos] = comp_count;
        ents++;

        // If not enough space for another entity header and a tiny component, flush
        if (pos + sizeof(uint32_t) + 1 + 8 > soft_limit) {
            flush:
            if (ents > 0) {
                // backfill entity count
                memcpy(buf + hdr_sz, &ents, sizeof(uint16_t));
                send_multi_to_peer(cm, peer_id, buf, pos);
            }
            pos  = base_sz;
            ents = 0;
        }
    }

    // Send any remaining entities
    if (ents > 0) {
        memcpy(buf + hdr_sz, &ents, sizeof(uint16_t));
        send_multi_to_peer(cm, peer_id, buf, pos);
    }
}

static void sd_flush_if_needed(connection_manager_t* cm,
                               const char* peer_id,
                               uint8_t* buf,
                               size_t* pos,
                               uint16_t* ents)
{
    if (*ents == 0) { return; }
    packet_header_t* hdr = (packet_header_t*)buf;
    hdr->type = PACKET_TYPE_MULTI_ENTITY_UPDATE;
    hdr->size = (uint16_t)(*pos);

    // backfill entity_count
    memcpy(buf + sizeof(packet_header_t), ents, sizeof(uint16_t));

    // UDP preferente, TCP fallback
    int s = connection_manager_send_to_peer_udp(cm, peer_id, buf, (int)hdr->size);
    if (s < 0) {
        connection_manager_send_to_peer(cm, peer_id, buf, (int)hdr->size);
    }

    // reset
    *pos  = sizeof(packet_header_t) + sizeof(uint16_t);
    *ents = 0;
}static void send_dirty_chunked_rr(ecs_t* ecs, connection_manager_t* cm,
                                  const char* peer_id, size_t soft_limit) {
    static entity_t rr = 0;
    if (!ecs || !cm) return;
    // Clamp soft_limit so it never exceeds the packet buffer.  Falling back
    // to MAX_PACKET_SIZE - 64 leaves headroom for headers and avoids
    // buffer overflows that would otherwise prevent any data from being
    // transmitted.
    if (soft_limit == 0 || soft_limit > MAX_PACKET_SIZE) {
        const size_t margin = 64;
        soft_limit = (MAX_PACKET_SIZE > margin ? MAX_PACKET_SIZE - margin : MAX_PACKET_SIZE);
    }

    uint8_t buf[MAX_PACKET_SIZE];
    const size_t cap     = sizeof(buf);
    const size_t hdr_sz  = sizeof(packet_header_t);
    const size_t base_sz = hdr_sz + sizeof(uint16_t);

    size_t pos  = base_sz;
    uint16_t ents = 0;

    /* Construir lista ordenada de entidades: primero las que se han marcado como
       dirty recientemente (prioritized_entities[e] == true), luego las demás en
       el orden round‑robin habitual.  El array 'added' evita duplicados. */
    entity_t order[MAX_ENTITIES];
    bool added[MAX_ENTITIES] = { false };
    size_t order_count = 0;
    // Añadir entidades priorizadas
    for (entity_t e = 0; e < MAX_ENTITIES; ++e) {
        if (prioritized_entities[e]) {
            order[order_count++] = e;
            added[e] = true;
            /* Limpiar la marca de prioridad: sólo queremos priorizar la primera
               vez que una entidad queda sucia; las siguientes veces vuelve al
               ciclo normal. */
            prioritized_entities[e] = false;
        }
    }
    // Añadir el resto en orden round‑robin
    for (entity_t step = 0; step < MAX_ENTITIES; ++step) {
        entity_t e = (rr + step) % MAX_ENTITIES;
        if (!added[e]) {
            order[order_count++] = e;
            added[e] = true;
        }
    }

    /* Recorremos la lista ordenada y empaquetamos los componentes sucios.  Si
       al intentar escribir una entidad no cabe ni un componente, se hace flush
       del paquete actual y se reproc esa misma entidad en el siguiente. */
    for (size_t idx = 0; idx < order_count; ++idx) {
        entity_t e = order[idx];

        // Reservar cabecera de entidad
        size_t ent_pos = pos;
        pos = wr_mem(buf, cap, pos, &e, sizeof(entity_t));
        if (pos == (size_t)-1) goto flush;
        size_t cc_pos = pos;
        if (pos + 1 > cap) goto flush;
        buf[pos++] = 0;
        uint8_t comp_count = 0;

        for (component_t c = 0; c < ecs->registered_component_count; ++c) {
            if (!ecs_has_component(ecs, e, c)) continue;
            if (!ecs_is_component_dirty(ecs, e, c)) continue;

            const void* comp_data = ecs_get_component(ecs, e, c);
            size_t comp_size      = ecs->components[c].descriptor.size;

            /* Si añadir este componente supera el soft_limit, decide: si no se ha
               escrito ninguno aún para esta entidad, deja 'pos' en ent_pos y
               envía el paquete actual para empezar uno nuevo (reprocesará
               nuevamente esta entidad).  Si ya se han escrito algunos
               componentes, cierra la entidad, envía el paquete, y reproc. */
            if (pos + sizeof(component_t) + comp_size > soft_limit) {
                if (comp_count == 0) { pos = ent_pos; goto flush; }
                buf[cc_pos] = comp_count;
                ents++;
                goto flush;
            }

            pos = wr_mem(buf, cap, pos, &c, sizeof(component_t));
            pos = wr_mem(buf, cap, pos, comp_data, comp_size);
            if (pos == (size_t)-1) { pos = ent_pos; goto flush; }

            comp_count++;
            /* Limpiamos el flag dirty ahora que se ha escrito este componente.
               Nota: si más tarde hay que reproc. la entidad porque el paquete
               se llenó, los componentes restantes seguirán sucios y por tanto
               se volverán a enviar. */
            ecs_clear_component_dirty(ecs, e, c);
        }

        if (comp_count == 0) {
            // no se escribió nada de esta entidad
            pos = ent_pos;
            continue;
        }

        // Escribir el número de componentes en la cabecera de entidad
        buf[cc_pos] = comp_count;
        ents++;

        /* Si no queda espacio para al menos otra entidad y un componente, envia
           ahora y empieza un paquete nuevo. */
        if (pos + sizeof(entity_t) + 1 + 8 > soft_limit) {
flush:
            if (ents > 0) {
                // Rellenar count e inicializar cabecera
                memcpy(buf + hdr_sz, &ents, sizeof(uint16_t));
                packet_header_t* hdr = (packet_header_t*)buf;
                hdr->type = PACKET_TYPE_MULTI_ENTITY_UPDATE;
                hdr->size = (uint16_t)pos;

                if (peer_id) {
                    // Envío unicast (UDP preferente, TCP fallback)
                    send_multi_to_peer(cm, peer_id, buf, pos);
                } else {
                    // Broadcast: siempre inicializa hdr antes de enviar
                    connection_manager_broadcast(cm, buf, (int)pos);
                }
            }
            // Reiniciar buffer y contadores para nuevo paquete
            pos  = base_sz;
            ents = 0;
            // Decrementa idx para reprocesar la misma entidad si saltamos por flush
            idx--;
        }
    }

    // Enviar cualquier entidad restante en el buffer al final
    if (ents > 0) {
        memcpy(buf + hdr_sz, &ents, sizeof(uint16_t));
        packet_header_t* hdr = (packet_header_t*)buf;
        hdr->type = PACKET_TYPE_MULTI_ENTITY_UPDATE;
        hdr->size = (uint16_t)pos;

        if (peer_id) {
            send_multi_to_peer(cm, peer_id, buf, pos);
        } else {
            connection_manager_broadcast(cm, buf, (int)pos);
        }
    }

    // Avanzar el round‑robin para la próxima llamada
    rr = (rr + 1) % MAX_ENTITIES;
}

/*
 * Send a delta snapshot of all dirty components to a single peer.  This
 * function implements round‑robin ordering with prioritisation of
 * recently dirtied entities, respects the per‑peer interest mask, and
 * serialises the network_id instead of the local entity id.  It never
 * clears dirty flags or the global prioritized_entities; those are
 * cleared once per network tick by the caller after sending to all
 * peers.  The soft_limit parameter bounds the approximate maximum
 * size of each datagram.
 */
static void send_dirty_chunked_rr_single(ecs_t* ecs,
                                         connection_manager_t* cm,
                                         const char* peer_id,
                                         size_t soft_limit,
                                         entity_t rr_base)
{
    if (!ecs || !cm || !peer_id) return;
    if (soft_limit == 0 || soft_limit > MAX_PACKET_SIZE) {
        const size_t margin = 64;
        soft_limit = (MAX_PACKET_SIZE > margin ? MAX_PACKET_SIZE - margin : MAX_PACKET_SIZE);
    }
    peer_t* peer = connection_manager_get_peer(cm, peer_id);
    interest_mask_t mask = peer ? peer->interest_mask : (interest_mask_t)0xFFFFFFFF;

    uint8_t buf[MAX_PACKET_SIZE];
    const size_t cap     = sizeof(buf);
    const size_t hdr_sz  = sizeof(packet_header_t);
    const size_t base_sz = hdr_sz + sizeof(uint16_t);

    size_t pos  = base_sz;
    uint16_t ents = 0;

    // Build ordered list: prioritised entities first, then the rest in round‑robin order.
    entity_t order[MAX_ENTITIES];
    bool added[MAX_ENTITIES] = { false };
    size_t order_count = 0;
    for (entity_t e = 0; e < MAX_ENTITIES; ++e) {
        if (prioritized_entities[e]) {
            order[order_count++] = e;
            added[e] = true;
        }
    }
    for (entity_t step = 0; step < MAX_ENTITIES; ++step) {
        entity_t e = (rr_base + step) % MAX_ENTITIES;
        if (!added[e]) {
            order[order_count++] = e;
            added[e] = true;
        }
    }

    for (size_t idx = 0; idx < order_count; ++idx) {
        entity_t e = order[idx];
        if (!ecs_has_component(ecs, e, COMPONENT_NETWORKED_ENTITY)) continue;
        networked_entity_t* ne = ecs_get_component(ecs, e, COMPONENT_NETWORKED_ENTITY);
        if (!ne) continue;
        if ((ne->interest_groups & mask) == 0) continue;

        size_t ent_pos = pos;
        uint32_t net_id = ne->network_id;
        pos = wr_mem(buf, cap, pos, &net_id, sizeof(uint32_t));
        if (pos == (size_t)-1) goto flush_single;
        size_t cc_pos = pos;
        if (pos + 1 > cap) goto flush_single;
        buf[pos++] = 0;
        uint8_t comp_count = 0;

        for (component_t c = 0; c < ecs->registered_component_count; ++c) {
            if (c == COMPONENT_NETWORKED_ENTITY) continue;
            if (!ecs_has_component(ecs, e, c)) continue;
            if (!ecs_is_component_dirty(ecs, e, c)) continue;
            const void* comp_data = ecs_get_component(ecs, e, c);
            size_t comp_size = ecs->components[c].descriptor.size;
            if (pos + sizeof(component_t) + comp_size > soft_limit) {
                if (comp_count == 0) { pos = ent_pos; goto flush_single; }
                buf[cc_pos] = comp_count;
                ents++;
                goto flush_single;
            }
            pos = wr_mem(buf, cap, pos, &c, sizeof(component_t));
            pos = wr_mem(buf, cap, pos, comp_data, comp_size);
            if (pos == (size_t)-1) { pos = ent_pos; goto flush_single; }
            comp_count++;
        }
        if (comp_count == 0) {
            pos = ent_pos;
            continue;
        }
        buf[cc_pos] = comp_count;
        ents++;
        if (pos + sizeof(uint32_t) + 1 + 8 > soft_limit) {
flush_single:
            if (ents > 0) {
                memcpy(buf + hdr_sz, &ents, sizeof(uint16_t));
                packet_header_t* hdr = (packet_header_t*)buf;
                hdr->type = PACKET_TYPE_MULTI_ENTITY_UPDATE;
                hdr->size = (uint16_t)pos;
                send_multi_to_peer(cm, peer_id, buf, pos);
            }
            pos  = base_sz;
            ents = 0;
            idx--;
        }
    }
    if (ents > 0) {
        memcpy(buf + hdr_sz, &ents, sizeof(uint16_t));
        packet_header_t* hdr = (packet_header_t*)buf;
        hdr->type = PACKET_TYPE_MULTI_ENTITY_UPDATE;
        hdr->size = (uint16_t)pos;
        send_multi_to_peer(cm, peer_id, buf, pos);
    }
}


// ---------- callbacks de connection_manager ----------
void on_packet_received_cs(void *user_data, peer_t *peer, const void *data, int len) {
    network_cs_t *cs = (network_cs_t *) user_data;
    if (!cs || !peer || !data || len < (int)sizeof(packet_header_t)) return;

    const network_packet_t *pkt = (const network_packet_t *) data;

    if (cs->connection_manager.is_server && pkt->header.type == PACKET_TYPE_CLIENT_REGISTER) {
        if (len >= (int)(sizeof(packet_header_t) + sizeof(uint16_t))) {
            uint16_t client_udp_port = 0;
            memcpy(&client_udp_port, pkt->data, sizeof(uint16_t));

            // wire UDP de ese peer
            peer->addr_udp = peer->addr_tcp;
            peer->addr_udp.sin_port = htons(client_udp_port);

            net_socket_t *udp_listen =
                connection_manager_get_listen_socket(&cs->connection_manager, SOCKET_TYPE_UDP);
            if (udp_listen) {
                peer->net_sockets[SOCKET_TYPE_UDP] = *udp_listen;
                peer->udp_ready = 1;
                printf("[network_cs] UDP ready for %s\n", peer->id);
            }

            // Mark all existing networked entities as dirty so that the next
            // network tick will re‑send their components via the delta snapshot
            // system.  Without this, entities created before the client
            // connected might not get replicated if send_full_state_chunked()
            // fails for any reason.
            ecs_t* mark_esc = cs->ecs;
            if (mark_esc) {
                for (entity_t ent = 0; ent < MAX_ENTITIES; ++ent) {
                    if (!ecs_has_component(mark_esc, ent, COMPONENT_NETWORKED_ENTITY)) continue;
                    for (component_t cid = 0; cid < mark_esc->registered_component_count; ++cid) {
                        if (cid == COMPONENT_NETWORKED_ENTITY) continue;
                        if (!ecs_has_component(mark_esc, ent, cid)) continue;
                        ecs_mark_component_dirty(mark_esc, ent, cid);
                    }
                }
            }
            // snapshot inicial por UDP (troceado).  Passing 0 uses the default
            // soft limit (clamped to the buffer size) to avoid overrunning the
            // packet buffer.
            send_full_state_chunked(cs->ecs, &cs->connection_manager, peer->id, 0);
        }
        return; // <- no lo reenvíes a PH ni a la app
    }

    // Si somos un cliente y recibimos un snapshot/delta de múltiples entidades,
    // debemos interpretar los datos usando network_id en lugar de entity_t.
    if (!cs->connection_manager.is_server && pkt->header.type == PACKET_TYPE_MULTI_ENTITY_UPDATE) {
        // Parse the multi-entity update packet.  Layout:
        // [uint16_t entity_count][for each entity: uint32_t network_id][uint8_t comp_count][comp pairs...]
        const uint8_t* ptr = pkt->data;
        const uint8_t* end = ((const uint8_t*)pkt) + pkt->header.size;
        if (end - ptr < (ptrdiff_t)sizeof(uint16_t)) {
            return;
        }
        uint16_t ent_count = 0;
        memcpy(&ent_count, ptr, sizeof(uint16_t));
        ptr += sizeof(uint16_t);
        for (uint16_t i = 0; i < ent_count; ++i) {
            if (end - ptr < (ptrdiff_t)sizeof(uint32_t)) { break; }
            uint32_t net_id = 0;
            memcpy(&net_id, ptr, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
            if (end - ptr < (ptrdiff_t)sizeof(uint8_t)) { break; }
            uint8_t comp_count = *ptr++;
            // Look up or create the local entity for this network_id
            entity_t local_e = network_map_lookup(&cs->network_map, net_id);
            if (local_e == (entity_t)-1) {
                // Create a new local entity and attach a NetworkedEntity component
                local_e = ecs_create_entity(cs->ecs);
                networked_entity_t ne = { .network_id = net_id, .interest_groups = 0 };
                ecs_add_component(cs->ecs, local_e, COMPONENT_NETWORKED_ENTITY, &ne);
                network_map_insert(&cs->network_map, net_id, local_e);
            }
            for (uint8_t j = 0; j < comp_count; ++j) {
                if (end - ptr < (ptrdiff_t)sizeof(component_t)) { break; }
                component_t cid;
                memcpy(&cid, ptr, sizeof(component_t));
                ptr += sizeof(component_t);
                // Validate component ID
                if (cid >= cs->ecs->registered_component_count) {
                    // Skip invalid component id
                    return;
                }
                size_t comp_size = cs->ecs->components[cid].descriptor.size;
                if (end - ptr < (ptrdiff_t)comp_size) {
                    // Truncated component payload
                    return;
                }
                // Skip NetworkedEntity component if ever sent (should never be)
                if (cid == COMPONENT_NETWORKED_ENTITY) {
                    ptr += comp_size;
                    continue;
                }
                if (!ecs_has_component(cs->ecs, local_e, cid)) {
                    ecs_add_component(cs->ecs, local_e, cid, (void*)ptr);
                } else {
                    void* dst = ecs_get_component(cs->ecs, local_e, cid);
                    if (dst) memcpy(dst, ptr, comp_size);
                }
                ecs_mark_component_dirty(cs->ecs, local_e, cid);
                ptr += comp_size;
            }
        }
        return;
    }
    if (cs->connection_manager.is_server && pkt->header.type == PACKET_TYPE_CLIENT_INPUT) {

        const uint8_t* p = pkt->data;
        const uint8_t* end = ((const uint8_t*)pkt) + pkt->header.size;

        if (end - p < (ptrdiff_t)(sizeof(entity_t) + sizeof(uint8_t))) {
            printf("[PH] CLIENT_INPUT too small\n");
        return;
        }
        entity_t eid; uint8_t cmd;
        memcpy(&eid, p, sizeof(entity_t)); p += sizeof(entity_t);
        memcpy(&cmd,  p, sizeof(uint8_t));  p += sizeof(uint8_t);

        size_t payload_size = end - p;
        const uint8_t *payload = p;
        if (cs->config.on_client_input)
            cs->config.on_client_input(cs->config.user_data,peer,eid,cmd,payload,payload_size);
        else {
            printf("[PH] CLIENT_INPUT (cmd=%u) received but no handler set\n", cmd);
        }
        return;
    }
    // resto de paquetes → defer to protocol handler and user callback
    protocol_handler_process_received_data(&cs->protocol_handler, cs->ecs, peer, data, len);
    if (cs->config.on_packet_received)
        cs->config.on_packet_received(cs->config.user_data, peer, data, len);
}

void on_peer_connected_cs(void *user_data, peer_t *peer) {
    network_cs_t *cs = (network_cs_t *) user_data;
    if (!cs || !peer) return;

    printf("[network_cs] Peer %s connected.\n", peer->id);

    if (cs->config.is_server) {
        // SERVER -> manda ACK por TCP (el cliente responderá con CLIENT_REGISTER)

        peer->interest_mask = (interest_mask_t)0xFFFFFFFF;

        protocol_handler_pack_server_ack(&cs->protocol_handler);
        connection_manager_send_to_peer(&cs->connection_manager, peer->id,
                                        &cs->protocol_handler.out_packet,
                                        cs->protocol_handler.out_packet.header.size);
        if (cs->config.on_peer_connected)
            cs->config.on_peer_connected(cs->config.user_data, peer);
        return;
    }

    // ===== CLIENT =====
    // 1) Prepara el mapeo UDP hacia el servidor (misma IP TCP, puerto UDP del server del config)
    peer->addr_udp = peer->addr_tcp;
    peer->addr_udp.sin_port = htons(cs->config.udp_port);

    net_socket_t *udp_listen =
        connection_manager_get_listen_socket(&cs->connection_manager, SOCKET_TYPE_UDP);
    if (udp_listen) {
        peer->net_sockets[SOCKET_TYPE_UDP] = *udp_listen;
        peer->udp_ready = 1;
    }

    // 2) Envía CLIENT_REGISTER por TCP con tu puerto UDP local efímero
    uint16_t local_udp = connection_manager_get_udp_local_port(&cs->connection_manager);
    protocol_handler_pack_client_register(&cs->protocol_handler, local_udp);
    connection_manager_send_to_peer(&cs->connection_manager, peer->id,
                                    &cs->protocol_handler.out_packet,
                                    cs->protocol_handler.out_packet.header.size);
    printf("[Client] Sent CLIENT_REGISTER with UDP port %hu\n", local_udp);

    if (cs->config.on_peer_connected)
        cs->config.on_peer_connected(cs->config.user_data, peer);
}


void on_peer_disconnected_cs(void *user_data, peer_t *peer) {
    network_cs_t *cs = (network_cs_t *) user_data;
    if (cs) {
        printf("[network_cs] Peer %s disconnected.\n", peer->id);
        if (cs->config.on_peer_disconnected) {
            cs->config.on_peer_disconnected(cs->config.user_data, peer);
        }
    }
}

void on_client_input_cs(void* user_data, peer_t* from, entity_t entity_id, uint8_t cmd, const void* extra, uint16_t extra_len) {
    network_cs_t *cs = (network_cs_t *) user_data;
    if (cs) {
        if (cs->config.on_client_input) {
            cs->config.on_client_input(user_data, from, entity_id, cmd, extra, extra_len);
        }
    }
}


// ---------- init / update / destroy ----------

network_cs_t *network_cs_init(const network_architecture_config_t *config, ecs_t *ecs) {
    network_cs_t *cs_arch = (network_cs_t*)malloc(sizeof(network_cs_t));
    if (!cs_arch) return NULL;
    if (config->is_server) {
        ecs_set_dirty_hook(ecsnet_dirty_hook);
    } else {
        ecs_set_dirty_hook(NULL);
    }
    cs_arch->ecs = ecs;
    cs_arch->config = *config;
    cs_arch->sync_acc = 0.f; // si tu struct lo tiene; si no, ignora

    connection_manager_init(&cs_arch->connection_manager);
    cs_arch->connection_manager.is_server   = config->is_server;
    cs_arch->connection_manager.user_data   = cs_arch;
    cs_arch->connection_manager.on_receive  = on_packet_received_cs;
    cs_arch->connection_manager.on_connect  = on_peer_connected_cs;
    cs_arch->connection_manager.on_disconnect = on_peer_disconnected_cs;
    cs_arch->connection_manager.on_disconnect = on_peer_disconnected_cs;
    cs_arch->config.on_client_input = on_client_input_cs;

    protocol_handler_init(&cs_arch->protocol_handler);

    // Initialize the network map and ID counter.  The server assigns
    // unique network IDs to replicated entities, while clients use the
    // map to translate incoming network IDs to local entities.
    network_map_init(&cs_arch->network_map);
    cs_arch->next_network_id = 1;

    // TCP
    net_socket_t tcp_listen = net_socket_create(SOCKET_TYPE_TCP);
    if (config->is_server) {
        net_socket_bind(&tcp_listen, config->ip_address, config->tcp_port);
        net_socket_listen(&tcp_listen, 10);
    } else {
        net_socket_set_non_blocking(&tcp_listen);
    }
    connection_manager_add_listen_socket(&cs_arch->connection_manager, tcp_listen, SOCKET_TYPE_TCP);

    // UDP
    net_socket_t udp_listen = net_socket_create(SOCKET_TYPE_UDP);
    if (config->is_server) {
        net_socket_bind(&udp_listen, config->ip_address, config->udp_port);
    } else {
        // puerto efímero
        net_socket_bind(&udp_listen, config->ip_address, 0);
    }
    connection_manager_add_listen_socket(&cs_arch->connection_manager, udp_listen, SOCKET_TYPE_UDP);

    return cs_arch;
}
void network_cs_update(network_cs_t *cs, float dt) {
    if (!cs) return;

    // Always update connection manager I/O
    connection_manager_update(&cs->connection_manager);

    // Only the server sends snapshots
    if (cs->config.is_server) {
        float hz = (cs->config.ecs_sync_hz > 0.f ? cs->config.ecs_sync_hz : 20.f);
        cs->sync_acc += dt;
        if (cs->sync_acc >= 1.0f / hz) {
            cs->sync_acc = 0.f;
            /*
             * Iterate over each connected peer and send a delta snapshot
             * tailored to that peer's interest mask.  We maintain a
             * persistent round‑robin index (rr_base) across all peers so
             * that different peers see entities in a consistent order.  After
             * sending to all peers we clear the dirty flags on all
             * components and reset the prioritized_entities bitset.
             */
            static entity_t rr_base = 0;
            for (int i = 0; i < cs->connection_manager.peer_count; ++i) {
                peer_t* p = &cs->connection_manager.peers[i];
                if (!p->is_connected) continue;
                // Pass zero as soft_limit so that send_dirty_chunked_rr_single uses
                // its internal clamp.  Specifying a value larger than the
                // packet buffer would otherwise cause the send to fail.
                send_dirty_chunked_rr_single(cs->ecs, &cs->connection_manager, p->id, 0, rr_base);
            }
            // Clear dirty flags on all components after sending to all peers
            ecs_t* ecs = cs->ecs;
            for (entity_t e = 0; e < MAX_ENTITIES; ++e) {
                // Reset prioritized flag for next tick
                prioritized_entities[e] = false;
                for (component_t c = 0; c < ecs->registered_component_count; ++c) {
                    if (!ecs_has_component(ecs, e, c)) continue;
                    if (ecs_is_component_dirty(ecs, e, c)) {
                        ecs_clear_component_dirty(ecs, e, c);
                    }
                }
            }
            // Advance the round‑robin starting index for the next tick
            rr_base = (rr_base + 1) % MAX_ENTITIES;
        }
    }
}

void network_cs_destroy(network_cs_t *cs) {
    if (!cs) return;
    // Release connection resources
    connection_manager_destroy(&cs->connection_manager);
    // Destroy the network ID map (client side) and free storage
    network_map_destroy(&cs->network_map);
    // Finally free the network architecture struct
    free(cs);
}
