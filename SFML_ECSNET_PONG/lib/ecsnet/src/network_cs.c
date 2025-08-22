#include "network_cs.h"
#include <stdlib.h>
#include <stdio.h>
#include "ecs.h"
#include "ecs_internal.h"
#include "net_socket.h"
#include "protocol_handler.h"

void on_packet_received_cs(void *user_data, peer_t *peer, const void *data, int len) {
    network_cs_t *network_cs = (network_cs_t *) user_data;
    if (!network_cs || !network_cs->ecs) return;

    // Process the packet with the Protocol Handler.
    protocol_handler_process_received_data(&network_cs->protocol_handler, network_cs->ecs, peer, data, len);

    // Get the packet type after processing.
    const network_packet_t *packet = (const network_packet_t *) data;

    // Handle a client registration packet.
    if (packet->header.type == PACKET_TYPE_CLIENT_REGISTER) {
        // The registration logic is performed here on the server.
        // It's assumed the UDP port has already been extracted by the protocol_handler.

        // Get the UDP listen socket from the connection_manager.
        net_socket_t *udp_listen_socket = connection_manager_get_listen_socket(
            &network_cs->connection_manager, SOCKET_TYPE_UDP);
        if (udp_listen_socket) {
            // Assign the UDP listen socket to the peer.
            peer->net_sockets[SOCKET_TYPE_UDP] = *udp_listen_socket;
            printf("[network_cs] Assigning UDP listen socket to peer %s.\n", peer->id);

            // Pack and send an acknowledgment (ACK) to the client.
            protocol_handler_pack_server_ack(&network_cs->protocol_handler);
            connection_manager_send_to_peer(&network_cs->connection_manager, peer->id,
                                            &network_cs->protocol_handler.out_packet,
                                            network_cs->protocol_handler.out_packet.header.size);
        } else {
            printf("[network_cs] ERROR: UDP listen socket not found.\n");
        }
    }
}

void on_peer_connected_cs(void *user_data, peer_t *peer) {
    network_cs_t *network_cs = (network_cs_t *) user_data;
    if (network_cs) {
        printf("[network_cs] Peer %s connected.\n", peer->id);
        // If it is a server, this is the opportunity to send an ACK or registration.
        // In this case, the client already sent the registration, so the server just replies.
        if (!network_cs->config.is_server) return;
        for (entity_t e = 0; e < MAX_ENTITIES; ++e) {
            bool entity_has_data = false;
            uint8_t sync_data[MAX_PACKET_SIZE];
            size_t sync_data_size = 0;

            for (component_t c = 0; c < network_cs->ecs->registered_component_count; ++c) {
                if (ecs_has_component(network_cs->ecs, e, c)) {
                    const void *component_data = ecs_get_component(network_cs->ecs, e, c);
                    size_t component_size = network_cs->ecs->components[c].descriptor.size;

                    if (sync_data_size + sizeof(component_t) + component_size > MAX_PACKET_SIZE) {
                        break;
                    }

                    memcpy(sync_data + sync_data_size, &c, sizeof(component_t));
                    sync_data_size += sizeof(component_t);

                    memcpy(sync_data + sync_data_size, component_data, component_size);
                    sync_data_size += component_size;

                    entity_has_data = true;
                }
            }

            if (entity_has_data) {
                protocol_handler_pack_entity_update(
                    &network_cs->protocol_handler,
                    e,
                    sync_data,
                    sync_data_size
                );

                protocol_handler_send_packet(
                    &network_cs->connection_manager,
                    peer->id,
                    &network_cs->protocol_handler
                );
            }
        }
    }
}

void on_peer_disconnected_cs(void *user_data, peer_t *peer) {
    network_cs_t *network_cs = (network_cs_t *) user_data;
    if (network_cs) {
        printf("[network_cs] Peer %s disconnected.\n", peer->id);
    }
}

network_cs_t *network_cs_init(const network_architecture_config_t *config, ecs_t *ecs) {
    // Allocate memory for the client-server architecture struct.
    network_cs_t *cs_arch = malloc(sizeof(network_cs_t));
    if (!cs_arch) {
        return NULL;
    }
    cs_arch->ecs = ecs;
    cs_arch->config = *config;

    // Initialize the connection manager and assign callbacks.
    connection_manager_init(&cs_arch->connection_manager);
    cs_arch->connection_manager.is_server = config->is_server;
    cs_arch->connection_manager.user_data = cs_arch;
    cs_arch->connection_manager.on_receive = on_packet_received_cs;
    cs_arch->connection_manager.on_connect = on_peer_connected_cs;
    cs_arch->connection_manager.on_disconnect = on_peer_disconnected_cs;

    // Initialize the protocol handler.
    protocol_handler_init(&cs_arch->protocol_handler);

    // Create and configure the TCP listen socket.
    net_socket_t tcp_listen_socket = net_socket_create(SOCKET_TYPE_TCP);
    if (config->is_server) {
        net_socket_bind(&tcp_listen_socket, config->ip_address, config->tcp_port);
        // Bind the socket to the specified IP and port.
        net_socket_listen(&tcp_listen_socket, 10);
    } else {
        // For a client, set the socket to non-blocking mode.
        net_socket_set_non_blocking(&tcp_listen_socket);
    }
    // Add the TCP socket to the connection manager.
    connection_manager_add_listen_socket(&cs_arch->connection_manager, tcp_listen_socket, SOCKET_TYPE_TCP);

    // Create, bind, and add the UDP listen socket.
    net_socket_t udp_listen_socket = net_socket_create(SOCKET_TYPE_UDP);
    net_socket_bind(&udp_listen_socket, config->ip_address, config->udp_port);
    connection_manager_add_listen_socket(&cs_arch->connection_manager, udp_listen_socket, SOCKET_TYPE_UDP);

    return cs_arch;
}
void send_dirty_entities_batch(network_cs_t* network_cs) {
    network_packet_t pkt;
    pkt.header.type = PACKET_TYPE_MULTI_ENTITY_UPDATE;

    uint8_t* write_ptr = pkt.data;

    // Reservamos espacio para entity_count (2 bytes). Lo escribiremos al final.
    write_ptr += sizeof(uint16_t);
    uint16_t entity_count = 0;

    // Recorrer todas las entidades y componentes sucios
    for (entity_t entity = 0; entity < network_cs->ecs->registered_entities_count; entity++) {
        // Preparar un buffer temporal con las parejas (component_id + datos) de la entidad
        uint8_t comp_buffer[512];
        uint8_t* comp_ptr = comp_buffer;
        uint8_t comp_count = 0;

        for (component_t cid = 0; cid < network_cs->ecs->registered_component_count; cid++) {
            if (network_cs->ecs->components[cid].is_dirty[entity]) {
                size_t comp_size = network_cs->ecs->components[cid].descriptor.size;
                // Añadir el ID del componente
                memcpy(comp_ptr, &cid, sizeof(component_t));
                comp_ptr += sizeof(component_t);
                // Añadir los datos del componente
                const void* comp_data = ecs_get_component(network_cs->ecs, entity, cid);
                memcpy(comp_ptr, comp_data, comp_size);
                comp_ptr += comp_size;
                comp_count++;
            }
        }

        if (comp_count > 0) {
            // Comprobar que cabe la entidad en el paquete
            size_t entity_bytes = sizeof(entity_t) + sizeof(uint8_t) + (comp_ptr - comp_buffer);
            if ((write_ptr - pkt.data) + entity_bytes > sizeof(pkt.data)) {
                // Si no cabe, deja de añadir entidades (o envía un paquete parcial y continúa)
                break;
            }

            // Escribir entity_id
            memcpy(write_ptr, &entity, sizeof(entity_t));
            write_ptr += sizeof(entity_t);
            // Escribir número de componentes
            memcpy(write_ptr, &comp_count, sizeof(uint8_t));
            write_ptr += sizeof(uint8_t);
            // Copiar los pares (component_id + datos)
            memcpy(write_ptr, comp_buffer, comp_ptr - comp_buffer);
            write_ptr += (comp_ptr - comp_buffer);

            // Marcar que esta entidad se incluirá y limpia sus dirty flags
            entity_count++;
            for (component_t cid = 0; cid < network_cs->ecs->registered_component_count; cid++) {
                network_cs->ecs->components[cid].is_dirty[entity] = false;
            }
        }
    }

    // Escribir entity_count al principio del payload
    memcpy(pkt.data, &entity_count, sizeof(uint16_t));
    // Calcular el tamaño total del paquete (cabecera + payload)
    pkt.header.size = sizeof(packet_header_t) + (write_ptr - pkt.data);

    // Enviar este único paquete a todos los peers
    for (int i = 0; i < network_cs->connection_manager.peer_count; i++) {
        const char* peer_id = network_cs->connection_manager.peers[i].id;
        connection_manager_send_to_peer(&network_cs->connection_manager, peer_id, &pkt, pkt.header.size);
    }
}


void network_cs_update(network_cs_t *network_cs) {
    if (!network_cs) return;

    // Actualizar el gestor de conexiones (procesa paquetes entrantes y vacía 1 paquete saliente)
    connection_manager_update(&network_cs->connection_manager);

    if (network_cs->config.is_server) {

        send_dirty_entities_batch(network_cs);
    }
}


void network_cs_destroy(network_cs_t *network_cs) {
    if (!network_cs) return;
    // Destroy the connection manager first to close sockets.
    connection_manager_destroy(&network_cs->connection_manager);
    // Free the memory for the main struct.
    free(network_cs);
}
