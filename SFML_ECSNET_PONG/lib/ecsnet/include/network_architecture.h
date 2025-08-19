#pragma once
#include "protocol_handler.h"
#include "connection_manager.h"
#include "ecs.h"
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
// Forward declarations
ECSNET_API typedef struct peer_t peer_t;
typedef void (*on_receive_func_t)(const struct sockaddr_in *sender_addr, const void *data, int len);

/**
 * @brief Available network architecture types.
 */
ECSNET_API typedef enum {
    ARCH_CLIENT_SERVER,     /**< A client-server architecture. */
    ARCH_P2P,               /**< A peer-to-peer architecture. */
    ARCH_LISTEN_SERVER      /**< A dedicated listen server architecture. */
} network_architecture_type_t;

/**
 * @brief Structure for configuring the network architecture.
 */
ECSNET_API typedef struct {
    network_architecture_type_t type;   /**< The type of network architecture to use. */
    const char *ip_address;             /**< The IP address to bind to or connect to. */
    uint16_t port;                      /**< The main port for communication. */
    bool is_server;                     /**< A flag indicating if the instance is a server. */
    uint16_t tcp_port;                  /**< The TCP port to use. */
    uint16_t udp_port;                  /**< The UDP port to use. */

    // Unified callbacks
    void (*on_peer_connected)(void* user_data, peer_t* peer);
    void (*on_peer_disconnected)(void* user_data, peer_t* peer);
    void (*on_packet_received)(void* user_data, peer_t* peer, const void* data, int len);
    void* user_data;                    /**< User data passed to callbacks */
} network_architecture_config_t;

/**
 * @brief Opaque type for the network architecture.
 * This hides the internal implementation details from the user.
 */
ECSNET_API typedef struct network_architecture_t network_architecture_t;

/**
 * @brief Initializes the network architecture based on the configuration.
 * @param architecture A pointer to a network_architecture_t pointer.
 * @param config A pointer to the network configuration settings.
 * @param ecs A pointer to the ECS instance.
 */
ECSNET_API void network_architecture_init(network_architecture_t** architecture, const network_architecture_config_t* config, ecs_t* ecs);

/**
 * @brief The update function that should be called in the main game loop.
 * This handles all network-related tasks like receiving data and managing connections.
 * @param architecture A pointer to the network_architecture_t instance.
 */
ECSNET_API void network_architecture_update(network_architecture_t* architecture);

/**
 * @brief Shuts down and frees all network resources.
 * @param architecture A pointer to the network_architecture_t instance to destroy.
 */
ECSNET_API void network_architecture_destroy(network_architecture_t* architecture);

/**
 * @brief Connects to a server (for client architectures).
 * @param architecture A pointer to the network_architecture_t instance.
 * @param ip_address The server IP address.
 * @param port The server port.
 * @return true if connection attempt was successful, false otherwise.
 */
ECSNET_API bool network_architecture_connect_to_server(network_architecture_t* architecture, const char* ip_address, uint16_t port);

/**
 * @brief Sends raw data to a specific peer.
 * @param architecture A pointer to the network_architecture_t instance.
 * @param peer_id The ID of the target peer.
 * @param data The data to send.
 * @param len The length of the data.
 * @return true if data was sent successfully, false otherwise.
 */
ECSNET_API bool network_architecture_send_to_peer(network_architecture_t* architecture, uint32_t peer_id, const void* data, int len);

/**
 * @brief Sends an entity update to a specific peer.
 * @param architecture A pointer to the network_architecture_t instance.
 * @param peer_id The ID of the target peer.
 * @param entity_id The entity ID to send.
 * @param component_data The component data to send.
 * @param data_len The length of the component data.
 * @return true if entity update was sent successfully, false otherwise.
 */
bool network_architecture_send_entity_update(network_architecture_t* architecture, uint32_t peer_id, entity_t entity_id, const void* component_data, int data_len);

/**
 * @brief Broadcasts data to all connected peers.
 * @param architecture A pointer to the network_architecture_t instance.
 * @param data The data to broadcast.
 * @param len The length of the data.
 * @return true if data was broadcast successfully, false otherwise.
 */
ECSNET_API bool network_architecture_broadcast(network_architecture_t* architecture, const void* data, int len);

/**
 * @brief Gets the number of connected peers.
 * @param architecture A pointer to the network_architecture_t instance.
 * @return The number of connected peers.
 */
ECSNET_API int network_architecture_get_peer_count(network_architecture_t* architecture);

/**
 * @brief Gets a peer by its ID.
 * @param architecture A pointer to the network_architecture_t instance.
 * @param peer_id The ID of the peer to retrieve.
 * @return A pointer to the peer, or NULL if not found.
 */
ECSNET_API peer_t* network_architecture_get_peer(network_architecture_t* architecture, uint32_t peer_id);

/**
 * @brief Gets the connection manager from a network architecture instance.
 * @param architecture A pointer to the network_architecture_t instance.
 * @return A pointer to the connection manager, or NULL if invalid.
 */
ECSNET_API connection_manager_t* network_architecture_get_connection_manager(network_architecture_t* architecture);

/**
 * @brief Sets callback functions for network events.
 * @param architecture A pointer to the network_architecture_t instance.
 * @param on_connect Callback for when a peer connects.
 * @param on_disconnect Callback for when a peer disconnects.
 * @param on_receive Callback for when data is received.
 */
ECSNET_API void network_architecture_set_callbacks(network_architecture_t* architecture,
                                       void (*on_connect)(void*, peer_t*),
                                       void (*on_disconnect)(void*, peer_t*),
                                       void (*on_receive)(void*, peer_t*, const void*, int));


#ifdef __cplusplus
}
#endif

