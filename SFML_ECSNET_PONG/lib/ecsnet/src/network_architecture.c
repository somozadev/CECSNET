#include "network_architecture.h"

#include <stdio.h>
#include <stdlib.h> // Required for malloc and free

#include "network_cs.h" // Client-Server implementation
// #include "network_p2p.h"
// #include "network_ls.h"


typedef struct {
    network_architecture_config_t config; // public struct
    void (*on_peer_connected_internal)(void* user_data, peer_t* peer);
    void (*on_peer_disconnected_internal)(void* user_data, peer_t* peer);
    void (*on_packet_received_internal)(void* user_data, peer_t* peer, const void* data, int len);
    void* user_data;  /**< User data passed to callbacks */
} network_architecture_internal_t;

void network_architecture_init(network_architecture_t **architecture, const network_architecture_config_t *config,
                               ecs_t *ecs) {
    if ( !config ) {
        return;
    }
    // Allocate memory for the main network architecture struct.
    *architecture = (network_architecture_t *) malloc(sizeof(network_architecture_t));
    if (!*architecture) return;

    // Store the configuration and ECS pointer.
    (*architecture)->config = *config;
    (*architecture)->type = config->type;
    (*architecture)->ecs = ecs;
    (*architecture)->impl = NULL;

    // Initialize the specific network implementation based on the configured type.
    switch (config->type) {
        case ARCH_CLIENT_SERVER:
            // Call the initialization function for the Client-Server module.
            network_cs_t *cs_impl = network_cs_init(config, ecs);
            if (cs_impl) {
                // Configure callbacks if provided
                if (config->on_peer_connected) {
                    cs_impl->connection_manager.on_connect = config->on_peer_connected;
                }
                if (config->on_peer_disconnected) {
                    cs_impl->connection_manager.on_disconnect = config->on_peer_disconnected;
                }
                if (config->on_packet_received) {
                    cs_impl->connection_manager.on_receive = config->on_packet_received;
                }
                if (config->user_data) {
                    cs_impl->connection_manager.user_data = config->user_data;
                }
            }
            (*architecture)->impl = cs_impl;
            if (!config->is_server)
            connection_manager_connect_to_server(&cs_impl->connection_manager, config->ip_address, config->tcp_port);
            break;
        // case ARCH_P2P:
        //     // Call the initialization function for the P2P module.
        //     (*architecture)->impl = network_p2p_init(config, ecs);
        //     break;
        // case ARCH_LISTEN_SERVER:
        //     (*architecture)->impl = network_ls_init(config, ecs);
        //     break;
        default:
            // Handle unrecognized or unsupported architecture types.
            fprintf(stderr, "Error: Architecture type not recognised.\n");
            break;
    }
}

void network_architecture_update(network_architecture_t *architecture, float dt) {
    if (!architecture || !architecture->impl) {
        return;
    }
    // This is the core of the polymorphic design pattern.
    switch (architecture->type) {
        case ARCH_CLIENT_SERVER:
            network_cs_update((network_cs_t *) architecture->impl);
            break;
        default: ;
            // case ARCH_P2P:
            //     network_p2p_update((network_p2p_t*)architecture->impl);
            //     break;
            // case ARCH_LISTEN_SERVER:
            //     network_ls_update((network_ls_t*)architecture->impl);
            //     break;
    }
}

void network_architecture_destroy(network_architecture_t *architecture) {
    if (!architecture) {
        return;
    }
    // Delegate the destruction call to the specific implementation
    // before freeing the main architecture struct.
    switch (architecture->type) {
        case ARCH_CLIENT_SERVER:
            network_cs_destroy((network_cs_t *) architecture->impl);
            break;
            // case ARCH_P2P:
            //     network_p2p_destroy((network_p2p_t*)architecture->impl);
            //     break;
            // case ARCH_LISTEN_SERVER:
            //     network_ls_destroy((network_ls_t*)architecture->impl);
            //     break;
    }
    free(architecture);
}

bool network_architecture_connect_to_server(network_architecture_t* architecture, const char* ip_address, uint16_t port) {
    connection_manager_t* cm = network_architecture_get_connection_manager(architecture);
    if (cm) {
        return connection_manager_connect_to_server(cm, ip_address, port) == 0;
    }
    return false;
}

bool network_architecture_send_to_peer(network_architecture_t *architecture, uint32_t peer_id, const void *data,
                                       int len) {
    if (!architecture || !architecture->impl || !data || len <= 0) {
        return false;
    }

    switch (architecture->type) {
        case ARCH_CLIENT_SERVER: {
            network_cs_t *cs_impl = (network_cs_t *) architecture->impl;

            // Find the peer by ID
            peer_t *target_peer = NULL;
            char peer_id_str[32];
            snprintf(peer_id_str, sizeof(peer_id_str), "%u", peer_id);

            for (int i = 0; i < cs_impl->connection_manager.peer_count; i++) {
                if (strcmp(cs_impl->connection_manager.peers[i].id, peer_id_str) == 0) {
                    target_peer = &cs_impl->connection_manager.peers[i];
                    break;
                }
            }

            if (!target_peer) {
                return false; // Peer not found
            }

            return connection_manager_send_to_peer(&cs_impl->connection_manager, peer_id_str, data, len);
            // Pack the raw data using protocol handler
            // protocol_handler_pack_raw_data(&cs_impl->protocol_handler, data, len);

            // Send the packet
            // return protocol_handler_send_packet(&cs_impl->connection_manager, peer_id, &cs_impl->protocol_handler);
        }
        default:
            return false;
    }
}

bool network_architecture_send_entity_update(network_architecture_t *architecture, uint32_t peer_id, entity_t entity_id,
                                             const void *component_data, int data_len) {
    if (!architecture || !architecture->impl || !component_data || data_len <= 0) {
        return false;
    }

    switch (architecture->type) {
        case ARCH_CLIENT_SERVER: {
            network_cs_t *cs_impl = (network_cs_t *) architecture->impl;

            // Find the peer by ID
            peer_t *target_peer = NULL;
            char peer_id_str[32];
            snprintf(peer_id_str, sizeof(peer_id_str), "%u", peer_id);
            for (int i = 0; i < cs_impl->connection_manager.peer_count; i++) {
                if (strcmp(cs_impl->connection_manager.peers[i].id, peer_id_str) == 0) {
                    target_peer = &cs_impl->connection_manager.peers[i];
                    break;
                }
            }

            if (!target_peer) {
                return false; // Peer not found
            }

            // Pack the entity update using protocol handler
            protocol_handler_pack_entity_update(&cs_impl->protocol_handler, entity_id, component_data, data_len);

            // Send the packet
            protocol_handler_send_packet(&cs_impl->connection_manager, peer_id_str, &cs_impl->protocol_handler);
            return true;
        }
        default:
            return false;
    }
}

bool network_architecture_broadcast(network_architecture_t *architecture, const void *data, int len) {
    if (!architecture || !architecture->impl || !data || len <= 0) {
        return false;
    }

    switch (architecture->type) {
        case ARCH_CLIENT_SERVER: {
            network_cs_t *cs_impl = (network_cs_t *) architecture->impl;
            bool all_sent = true;

            // Send to all connected peers
            for (int i = 0; i < cs_impl->connection_manager.peer_count; i++) {
                const char *peer_id = cs_impl->connection_manager.peers[i].id;
                if (!connection_manager_send_to_peer(&cs_impl->connection_manager, peer_id, data, len)) {
                    all_sent = false;
                }
            }

            return all_sent;
        }
        default:
            return false;
    }
}

int network_architecture_get_peer_count(network_architecture_t* architecture) {
    connection_manager_t* cm = network_architecture_get_connection_manager(architecture);
    if (cm) {
        return cm->peer_count;
    }
    return 0;
}

peer_t *network_architecture_get_peer(network_architecture_t *architecture, uint32_t peer_id) {
    if (!architecture || !architecture->impl) {
        return NULL;
    }

    switch (architecture->type) {
        case ARCH_CLIENT_SERVER: {
            network_cs_t *cs_impl = (network_cs_t *) architecture->impl;
            char peer_id_str[32];
            snprintf(peer_id_str, sizeof(peer_id_str), "%u", peer_id);

            // Find the peer by ID
            for (int i = 0; i < cs_impl->connection_manager.peer_count; i++) {
                if (strcmp(cs_impl->connection_manager.peers[i].id, peer_id_str) == 0) {
                    return &cs_impl->connection_manager.peers[i];
                }
            }
            return NULL; // Peer not found
        }
        default:
            return NULL;
    }
}

connection_manager_t* network_architecture_get_connection_manager(network_architecture_t* architecture) {
    if (!architecture || !architecture->impl) {
        return NULL;
    }

    switch (architecture->type) {
        case ARCH_CLIENT_SERVER:
            return &((network_cs_t*)architecture->impl)->connection_manager;
        default:
            return NULL;
    }
}

void network_architecture_set_callbacks(network_architecture_t* architecture,
                                       void (*on_connect)(void*, peer_t*),
                                       void (*on_disconnect)(void*, peer_t*),
                                       void (*on_receive)(void*, peer_t*, const void*, int)) {
    connection_manager_t* cm = network_architecture_get_connection_manager(architecture);
    if (cm) {
        cm->on_connect = on_connect;
        cm->on_disconnect = on_disconnect;
        cm->on_receive = on_receive;
    }
}
