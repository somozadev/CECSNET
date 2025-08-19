#pragma once

#include "network_architecture.h"
#include "connection_manager.h"
#include "protocol_handler.h"
#include "ecs.h"
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Represents the client-server network architecture implementation.
 *
 * This struct contains the core components for a client-server setup,
 * including the connection manager, protocol handler, and a pointer
 * to the ECS instance.
 */
ECSNET_API typedef struct network_cs_t {
    ecs_t* ecs;                                 /**< A pointer to the ECS instance. */
    network_architecture_config_t config;      /**< Configuration for this network instance. */
    connection_manager_t connection_manager;    /**< The connection manager instance. */
    protocol_handler_t protocol_handler;        /**< The protocol handler instance. */
} network_cs_t;

// callbacks

/**
 * @brief Callback function to process a received network packet in a client-server context.
 * @param user_data A generic pointer to the network_cs_t instance.
 * @param peer A pointer to the peer that sent the data.
 * @param data A pointer to the received data buffer.
 * @param len The length of the received data.
 */
ECSNET_API void on_packet_received_cs(void* user_data, peer_t* peer, const void* data, int len);

/**
 * @brief Callback function triggered when a new peer connects.
 * @param user_data A generic pointer to the network_cs_t instance.
 * @param peer A pointer to the newly connected peer.
 */
ECSNET_API void on_peer_connected_cs(void* user_data, peer_t* peer);

/**
 * @brief Callback function triggered when a peer disconnects.
 * @param user_data A generic pointer to the network_cs_t instance.
 * @param peer A pointer to the disconnected peer.
 */
ECSNET_API void on_peer_disconnected_cs(void* user_data, peer_t* peer);

/**
 * @brief Initializes the client-server network architecture.
 * @param config A pointer to the configuration settings.
 * @param ecs A pointer to the ECS instance.
 * @return A pointer to the newly created network_cs_t instance, or NULL on failure.
 */
network_cs_t* network_cs_init(const network_architecture_config_t* config, ecs_t* ecs);

/**
 * @brief Updates the client-server network architecture.
 * This function should be called regularly in the main loop to process network events.
 * @param network_cs A pointer to the network_cs_t instance.
 */
void network_cs_update(network_cs_t* network_cs);

/**
 * @brief Shuts down and frees all resources used by the client-server network architecture.
 * @param network_cs A pointer to the network_cs_t instance to destroy.
 */
void network_cs_destroy(network_cs_t* network_cs);

#ifdef __cplusplus
}
#endif

