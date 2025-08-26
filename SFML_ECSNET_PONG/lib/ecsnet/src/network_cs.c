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
 * sets the corresponding flag here.  During replication, we always
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


// Safe memcpy helper.
// Writes `n` bytes from `p` into dst at offset `pos`.
// Returns new position, or (size_t)-1 if out of capacity.
static size_t wr_mem(uint8_t* dst, size_t cap, size_t pos, const void* p, size_t n) {
    if (pos + n > cap) return (size_t)-1;
    memcpy(dst + pos, p, n);
    return pos + n;
}
// Helper: wrap buffer into a MULTI_ENTITY_UPDATE packet and send to peer.
// Prefers UDP, falls back to TCP.
// Returns number of bytes sent, or <0 on failure.
static int send_multi_to_peer(connection_manager_t* cm, const char* peer_id, uint8_t* buf, size_t size) {
    if (size > MAX_PACKET_SIZE) return -1;
    packet_header_t* hdr = (packet_header_t*)buf;
    hdr->type = PACKET_TYPE_MULTI_ENTITY_UPDATE;
    hdr->size = (uint16_t)size;

    // First UDP if it's ready, otherwise TCP
    int s = connection_manager_send_to_peer_udp(cm, peer_id, buf, (int)size);
    if (s < 0) s = connection_manager_send_to_peer(cm, peer_id, buf, (int)size);
    return s;
}

/*
 * Send a full snapshot of all networked entities to a single peer.  The
 * payload is chunked into multiple packets that respect a user‑defined
 * soft limit (typically around 1.2THSPkB).  Each entity is identified by
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
}

static void send_dirty_chunked_rr(ecs_t* ecs, connection_manager_t* cm,
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

    /* Build an ordered entity list:
       - first: entities flagged in prioritized_entities (recently dirtied)
       - then: all others, in round-robin order for fairness.
       The 'added' array prevents duplicates.
    */
    entity_t order[MAX_ENTITIES];
    bool added[MAX_ENTITIES] = { false };
    size_t order_count = 0;
    // Add prioritized entities first
    for (entity_t e = 0; e < MAX_ENTITIES; ++e) {
        if (prioritized_entities[e]) {
            order[order_count++] = e;
            added[e] = true;
            /* Clear the priority flag: We only want to prioritize the first
            time an entity becomes dirty; subsequent times it returns to the
            normal cycle. */
            prioritized_entities[e] = false;
        }
    }
    // Add the rest in round-robin order
    for (entity_t step = 0; step < MAX_ENTITIES; ++step) {
        entity_t e = (rr + step) % MAX_ENTITIES;
        if (!added[e]) {
            order[order_count++] = e;
            added[e] = true;
        }
    }

    /* We traverse the sorted list and pack the dirty components. If
       when trying to write an entity, not even one component fits, we flush
       the current package and reproduce that same entity in the next one. */
    for (size_t idx = 0; idx < order_count; ++idx) {
        entity_t e = order[idx];

        // Reserver entitiy's header
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

            /*If adding this component exceeds the soft_limit, decide: if none have been written yet for this entity, leave 'pos' in ent_pos and
            send the current packet to start a new one (it will reprocess this entity again). If some components have already been written, close
            the entity, send the packet, and reprocess. */
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
            /* We clear the dirty flag now that this component has been written.
            Note: If the entity needs to be reprocessed later because the package
            became full, the remaining components will still be dirty and will therefore
            be resent. */
            ecs_clear_component_dirty(ecs, e, c);
        }

        if (comp_count == 0) {
            // Nothing written from this entity.
            pos = ent_pos;
            continue;
        }

        // Write the amount of components in the entity's header.
        buf[cc_pos] = comp_count;
        ents++;

        /*If there's no room for at least one more entity and one component, submit
        now and start a new package.*/
        if (pos + sizeof(entity_t) + 1 + 8 > soft_limit) {
flush:
            if (ents > 0) {
                // Fill up count and initialize header.
                memcpy(buf + hdr_sz, &ents, sizeof(uint16_t));
                packet_header_t* hdr = (packet_header_t*)buf;
                hdr->type = PACKET_TYPE_MULTI_ENTITY_UPDATE;
                hdr->size = (uint16_t)pos;

                if (peer_id) {
                    // Unicast sent (UDP preferred, TCP fallback).
                    send_multi_to_peer(cm, peer_id, buf, pos);
                } else {
                    // Broadcast: Always initialize hdr before sending.
                    connection_manager_broadcast(cm, buf, (int)pos);
                }
            }
            // Restart buffer and counters for a new packet.
            pos  = base_sz;
            ents = 0;
            // Decrement idx to reprocess the same entity if we skipped due flush.
            idx--;
        }
    }

    // Send any remaining entity in the buffer at the end.
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

    // Step round‑robin for next call.
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
static void send_dirty_chunked_rr_single(network_cs_t* cs,
                                         const char* peer_id,
                                         size_t soft_limit,
                                         entity_t rr_base)
{
    if (!cs || !peer_id) return;
    ecs_t* ecs = cs->ecs;
    connection_manager_t* cm = &cs->connection_manager;
    if (!ecs || !cm) return;
    // Clamp soft_limit so it never exceeds the packet buffer.  Falling back
    // to MAX_PACKET_SIZE - 64 leaves headroom for headers and avoids
    // buffer overflows that would otherwise prevent any data from being
    // transmitted.
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

    // ---------------------------------------------------------------------
    // First, serialise any pending destruction events.  Each entry in
    // cs->pending_destroy_ids represents a network_id that was recently
    // destroyed on the server.  We send these ahead of normal dirty
    // components.  A zero comp_count signals to the client that it
    // should delete its local entity and remove it from the network map.
    for (size_t di = 0; di < cs->pending_destroy_count; ++di) {
        uint32_t net_id = cs->pending_destroy_ids[di];
        // Each deletion record consists of a network_id (4 bytes) and a
        // comp_count byte (1 byte).  If writing this would exceed the
        // soft_limit, flush the current packet first.
        if (pos + sizeof(uint32_t) + 1 > soft_limit) {
            if (ents > 0) {
                memcpy(buf + hdr_sz, &ents, sizeof(uint16_t));
                packet_header_t* hdr = (packet_header_t*)buf;
                hdr->type = PACKET_TYPE_MULTI_ENTITY_UPDATE;
                hdr->size = (uint16_t)pos;
                send_multi_to_peer(cm, peer_id, buf, pos);
            }
            pos  = base_sz;
            ents = 0;
        }
        // Write network_id
        size_t ent_pos = pos;
        pos = wr_mem(buf, cap, pos, &net_id, sizeof(uint32_t));
        if (pos == (size_t)-1) {
            // Buffer overflow; flush and retry
            pos = ent_pos;
            if (ents > 0) {
                memcpy(buf + hdr_sz, &ents, sizeof(uint16_t));
                packet_header_t* hdr = (packet_header_t*)buf;
                hdr->type = PACKET_TYPE_MULTI_ENTITY_UPDATE;
                hdr->size = (uint16_t)pos;
                send_multi_to_peer(cm, peer_id, buf, pos);
            }
            pos  = base_sz;
            ents = 0;
            // retry this deletion on next iteration
            di--;
            continue;
        }
        // Write comp_count = 0
        if (pos + 1 > cap) {
            // Can't write comp_count; flush and retry
            pos = ent_pos;
            if (ents > 0) {
                memcpy(buf + hdr_sz, &ents, sizeof(uint16_t));
                packet_header_t* hdr = (packet_header_t*)buf;
                hdr->type = PACKET_TYPE_MULTI_ENTITY_UPDATE;
                hdr->size = (uint16_t)pos;
                send_multi_to_peer(cm, peer_id, buf, pos);
            }
            pos  = base_sz;
            ents = 0;
            di--;
            continue;
        }
        buf[pos++] = 0;
        ents++;
        // If there is not enough space for another entry and minimal component,
        // flush now to avoid overflow in subsequent writes.
        if (pos + sizeof(uint32_t) + 1 + 8 > soft_limit) {
            if (ents > 0) {
                memcpy(buf + hdr_sz, &ents, sizeof(uint16_t));
                packet_header_t* hdr = (packet_header_t*)buf;
                hdr->type = PACKET_TYPE_MULTI_ENTITY_UPDATE;
                hdr->size = (uint16_t)pos;
                send_multi_to_peer(cm, peer_id, buf, pos);
            }
            pos  = base_sz;
            ents = 0;
        }
    }

    // ---------------------------------------------------------------------
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

    // Iterate ordered list and serialise dirty components
    for (size_t idx = 0; idx < order_count; ++idx) {
        entity_t e = order[idx];
        // Skip entities without a NetworkedEntity component
        if (!ecs_has_component(ecs, e, COMPONENT_NETWORKED_ENTITY)) continue;
        networked_entity_t* ne = ecs_get_component(ecs, e, COMPONENT_NETWORKED_ENTITY);
        if (!ne) continue;
        // Filter by interest mask
        if ((ne->interest_groups & mask) == 0) continue;

        size_t ent_pos = pos;
        uint32_t net_id = ne->network_id;
        pos = wr_mem(buf, cap, pos, &net_id, sizeof(uint32_t));
        if (pos == (size_t)-1) {
            // Buffer overflow; flush and retry this entity
            pos = ent_pos;
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
            continue;
        }
        size_t cc_pos = pos;
        if (pos + 1 > cap) {
            // Not enough space to write comp_count; flush
            pos = ent_pos;
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
            continue;
        }
        buf[pos++] = 0; // placeholder for comp_count
        uint8_t comp_count = 0;

        for (component_t c = 0; c < ecs->registered_component_count; ++c) {
            if (c == COMPONENT_NETWORKED_ENTITY) continue;
            if (!ecs_has_component(ecs, e, c)) continue;
            if (!ecs_is_component_dirty(ecs, e, c)) continue;
            const void* comp_data = ecs_get_component(ecs, e, c);
            size_t comp_size = ecs->components[c].descriptor.size;
            if (pos + sizeof(component_t) + comp_size > soft_limit) {
                // If nothing has been written yet, flush and retry this entity later
                if (comp_count == 0) {
                    pos = ent_pos;
                    // flush current packet
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
                    goto continue_outer_loop;
                }
                // Otherwise, finish this entity and flush
                buf[cc_pos] = comp_count;
                ents++;
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
                goto continue_outer_loop;
            }
            // Write component id and data
            pos = wr_mem(buf, cap, pos, &c, sizeof(component_t));
            pos = wr_mem(buf, cap, pos, comp_data, comp_size);
            if (pos == (size_t)-1) {
                // Buffer overflow; flush and retry
                pos = ent_pos;
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
                goto continue_outer_loop;
            }
            comp_count++;
        }
        // If no components were written, skip this entity
        if (comp_count == 0) {
            pos = ent_pos;
            continue;
        }
        buf[cc_pos] = comp_count;
        ents++;
        // Flush if not enough space for another entity header and minimal component
        if (pos + sizeof(uint32_t) + 1 + 8 > soft_limit) {
            if (ents > 0) {
                memcpy(buf + hdr_sz, &ents, sizeof(uint16_t));
                packet_header_t* hdr = (packet_header_t*)buf;
                hdr->type = PACKET_TYPE_MULTI_ENTITY_UPDATE;
                hdr->size = (uint16_t)pos;
                send_multi_to_peer(cm, peer_id, buf, pos);
            }
            pos  = base_sz;
            ents = 0;
        }
        continue_outer_loop: ;
    }
    // Send any remaining entities in buffer (including deletions) at end
    if (ents > 0) {
        memcpy(buf + hdr_sz, &ents, sizeof(uint16_t));
        packet_header_t* hdr = (packet_header_t*)buf;
        hdr->type = PACKET_TYPE_MULTI_ENTITY_UPDATE;
        hdr->size = (uint16_t)pos;
        send_multi_to_peer(cm, peer_id, buf, pos);
    }
}


// ---------- connection_manager callbacks ----------
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
            // initial snapshot over UDP (hashed). Passing 0 uses the default
            // soft limit (clamped to the buffer size) to avoid overrunning the
            // packet buffer.
            send_full_state_chunked(cs->ecs, &cs->connection_manager, peer->id, 0);
        }
        return; // Don't resend to PH nor the app.
    }

    // If we are a client and receive a snapshot/delta of multiple entities,
    // we must interpret the data using network_id instead of entity_t.
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
            // Look up or create the local entity for this network_id.
            // If comp_count == 0, this indicates the entity was destroyed on the server.
            if (comp_count == 0) {
                // If a mapping exists, destroy the local entity and remove the mapping
                entity_t local_e = network_map_lookup(&cs->network_map, net_id);
                if (local_e != (entity_t)-1) {
                    // Remove all ECS components and free the entity
                    ecs_destroy_entity(cs->ecs, local_e);
                    // Remove from the network map so the ID can be reused by the server
                    network_map_remove(&cs->network_map, net_id);
                }
                // No component payload follows for a delete event
                continue;
            }

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
    // Remaining packets: Defer to protocol handler and user callback
    protocol_handler_process_received_data(&cs->protocol_handler, cs->ecs, peer, data, len);
    if (cs->config.on_packet_received)
        cs->config.on_packet_received(cs->config.user_data, peer, data, len);
}

void on_peer_connected_cs(void *user_data, peer_t *peer) {
    network_cs_t *cs = (network_cs_t *) user_data;
    if (!cs || !peer) return;

    printf("[network_cs] Peer %s connected.\n", peer->id);

    if (cs->config.is_server) {
        // Server  sends ACK via TCP (client will answer with CLIENT_REGISTER)

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
    // 1) Prepare the UDP mapping to the server (same TCP IP, UDP port of the config server)
    peer->addr_udp = peer->addr_tcp;
    peer->addr_udp.sin_port = htons(cs->config.udp_port);

    net_socket_t *udp_listen =
        connection_manager_get_listen_socket(&cs->connection_manager, SOCKET_TYPE_UDP);
    if (udp_listen) {
        peer->net_sockets[SOCKET_TYPE_UDP] = *udp_listen;
        peer->udp_ready = 1;
    }

    // 2) Send CLIENT_REGISTER over TCP with your local ephemeral UDP port
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
    cs_arch->sync_acc = 0.f;

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

    // Initialise the pending destroy queue.  When server‑side entities are
    // destroyed their network IDs will be appended here.  Clients do not
    // use this queue.  We allocate an initial small capacity and
    // expand as needed in network_cs_mark_destroy().
    cs_arch->pending_destroy_ids = NULL;
    cs_arch->pending_destroy_count = 0;
    cs_arch->pending_destroy_capacity = 0;

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
        // Temp port
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
                send_dirty_chunked_rr_single(cs, p->id, 0, rr_base);
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
            // Clear the pending destruction list now that deletion events have
            // been sent to all peers.  If the list is not cleared here,
            // clients would receive duplicate delete messages on subsequent
            // ticks.
            if (cs->pending_destroy_count > 0) {
                cs->pending_destroy_count = 0;
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

    // Free pending destroy queue
    if (cs->pending_destroy_ids) {
        free(cs->pending_destroy_ids);
        cs->pending_destroy_ids = NULL;
        cs->pending_destroy_count = 0;
        cs->pending_destroy_capacity = 0;
    }
    // Finally free the network architecture struct
    free(cs);
}

// Append a network_id to the pending destroy queue.  Internal helper.
static void pending_destroy_append(network_cs_t* cs, uint32_t network_id) {
    if (!cs) return;
    // Only the server queues destroy events
    if (!cs->config.is_server) return;
    // Avoid duplicate entries: check if already present
    for (size_t i = 0; i < cs->pending_destroy_count; ++i) {
        if (cs->pending_destroy_ids[i] == network_id) {
            return;
        }
    }
    if (cs->pending_destroy_count >= cs->pending_destroy_capacity) {
        size_t new_cap = cs->pending_destroy_capacity == 0 ? 16 : cs->pending_destroy_capacity * 2;
        uint32_t* new_ids = (uint32_t*)realloc(cs->pending_destroy_ids, new_cap * sizeof(uint32_t));
        if (!new_ids) {
            // Allocation failed; drop the destroy event
            return;
        }
        cs->pending_destroy_ids = new_ids;
        cs->pending_destroy_capacity = new_cap;
    }
    cs->pending_destroy_ids[cs->pending_destroy_count++] = network_id;
}

void network_cs_mark_network_id_destroy(network_cs_t* cs, uint32_t network_id) {
    if (!cs) return;
    // Only queue destroys on the server
    if (!cs->config.is_server) return;
    pending_destroy_append(cs, network_id);
}

void network_cs_mark_entity_destroy(network_cs_t* cs, entity_t entity) {
    if (!cs) return;
    // Only the server queues destroy events
    if (!cs->config.is_server) return;
    if (!cs->ecs) return;
    if (!ecs_has_component(cs->ecs, entity, COMPONENT_NETWORKED_ENTITY)) return;
    networked_entity_t* ne = ecs_get_component(cs->ecs, entity, COMPONENT_NETWORKED_ENTITY);
    if (!ne) return;
    pending_destroy_append(cs, ne->network_id);
}
