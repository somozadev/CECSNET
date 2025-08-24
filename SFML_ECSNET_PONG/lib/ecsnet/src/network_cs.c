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
static void send_full_state_chunked(ecs_t* ecs, connection_manager_t* cm, const char* peer_id, size_t soft_limit) {
    if (!ecs || !cm || !peer_id) return;
    if (soft_limit == 0 || soft_limit > MAX_PACKET_SIZE) soft_limit = 1200;

    uint8_t buf[MAX_PACKET_SIZE];
    const size_t cap     = sizeof(buf);
    const size_t hdr_sz  = sizeof(packet_header_t);
    const size_t base_sz = hdr_sz + sizeof(uint16_t); // entity_count

    size_t pos  = base_sz;
    uint16_t ents = 0;

    for (entity_t e = 0; e < MAX_ENTITIES; ++e) {
        // Reservar cabecera de entidad
        size_t ent_pos = pos;
        pos = wr_mem(buf, cap, pos, &e, sizeof(entity_t));
        if (pos == (size_t)-1) goto flush;
        size_t cc_pos = pos; // comp_count
        if (pos + 1 > cap) goto flush;
        buf[pos++] = 0;
        uint8_t comp_count = 0;

        for (component_t c = 0; c < ecs->registered_component_count; ++c) {
            if (!ecs_has_component(ecs, e, c)) continue;
            const void* comp_data = ecs_get_component(ecs, e, c);
            size_t comp_size      = ecs->components[c].descriptor.size;

            // ¿cabe [cid][blob]?
            if (pos + sizeof(component_t) + comp_size > soft_limit) {
                // Si nada escrito aún para esta entidad, flushea paquete y vuelve a empezar con ella
                if (comp_count == 0) {
                    pos = ent_pos;
                    goto flush;
                }
                // Cierra entidad actual y flushea
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
            // Nada que enviar de esta entidad
            pos = ent_pos;
            continue;
        }
        buf[cc_pos] = comp_count;
        ents++;

        // margen para próxima entidad
        if (pos + sizeof(entity_t) + 1 + 8 > soft_limit) {
        flush:
            if (ents > 0) {
                memcpy(buf + hdr_sz, &ents, sizeof(uint16_t));
                send_multi_to_peer(cm, peer_id, buf, pos);
            }
            pos  = base_sz;
            ents = 0;
        }
    }

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
    if (soft_limit == 0 || soft_limit > MAX_PACKET_SIZE) soft_limit = 1200;

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

            // snapshot inicial por UDP (troceado)
            send_full_state_chunked(cs->ecs, &cs->connection_manager, peer->id, 1200);
        }
        return; // <- no lo reenvíes a PH ni a la app
    }

    // resto de paquetes → si quieres, PH + app
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

    protocol_handler_init(&cs_arch->protocol_handler);

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

    connection_manager_update(&cs->connection_manager);

    if (cs->config.is_server) {
        float hz = (cs->config.ecs_sync_hz > 0.f ? cs->config.ecs_sync_hz : 20.f);
        cs->sync_acc += dt;
        if (cs->sync_acc >= 1.0f / hz) {
            cs->sync_acc = 0.f;
            // broadcast de componentes dirty por UDP usando chunking/round-robin
            send_dirty_chunked_rr(cs->ecs, &cs->connection_manager, /*peer_id*/NULL, /*soft_limit*/1200);
        }
    }
}

void network_cs_destroy(network_cs_t *cs) {
    if (!cs) return;
    connection_manager_destroy(&cs->connection_manager);
    free(cs);
}
