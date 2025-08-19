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
    protocol_handler_process_received_data(&network_cs->protocol_handler, peer, data, len);

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

void network_cs_update(network_cs_t *network_cs) {
    if (!network_cs) return;
    // Delegate the update call to the connection manager.
    connection_manager_update(&network_cs->connection_manager);
    if (network_cs->config.is_server) {
        for (entity_t entity = 0; entity < MAX_ENTITIES; entity++) {
            bool entity_dirty = false;
            uint8_t sync_data[MAX_PACKET_SIZE];
            size_t sync_data_size = 0;

            for (component_t component = 0; component < network_cs->ecs->registered_component_count; component++) {
                if (network_cs->ecs->components[component].is_dirty[entity]) {
                    const void *component_data = ecs_get_component(network_cs->ecs, entity, component);
                    if (component_data) {
                        size_t component_size = network_cs->ecs->components[component].descriptor.size;

                        // Asegúrate de que hay espacio suficiente en sync_data
                        if (sync_data_size + sizeof(component_t) + component_size > MAX_PACKET_SIZE) {
                            break;
                        }

                        // Añadir ID del componente
                        memcpy(sync_data + sync_data_size, &component, sizeof(component_t));
                        sync_data_size += sizeof(component_t);

                        // Añadir datos del componente
                        memcpy(sync_data + sync_data_size, component_data, component_size);
                        sync_data_size += component_size;

                        entity_dirty = true;
                    }
                }
            }

            if (entity_dirty) {
                // Empaquetar la actualización de la entidad
                protocol_handler_pack_entity_update(
                    &network_cs->protocol_handler,
                    entity,
                    sync_data,
                    sync_data_size
                );

                // Enviar a todos los clientes conectados
                for (int i = 0; i < network_cs->connection_manager.peer_count; i++) {
                    peer_t *peer = &network_cs->connection_manager.peers[i];
                    protocol_handler_send_packet(
                        &network_cs->connection_manager,
                        peer->id,
                        &network_cs->protocol_handler
                    );
                }

                // Limpiar flags de dirty
                for (component_t component = 0; component < network_cs->ecs->registered_component_count; component++) {
                    ecs_clear_component_dirty(network_cs->ecs, entity, component);
                }
            }
        }
    }
}

void network_cs_destroy(network_cs_t *network_cs) {
    if (!network_cs) return;
    // Destroy the connection manager first to close sockets.
    connection_manager_destroy(&network_cs->connection_manager);
    // Free the memory for the main struct.
    free(network_cs);
}
