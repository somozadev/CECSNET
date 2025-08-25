#pragma once

#include "network_architecture.h"
#include "connection_manager.h"
#include "protocol_handler.h"
#include "ecs.h"
#include "network_map.h"
#include "networked_entity.h"
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
    float sync_acc;        /**< The synchronization accuracy factor. */
    /**
     * Mapping of remote network IDs to local entity IDs on the client.
     * Initialized on both client and server but only used on the
     * client side when applying network updates.  On the server this
     * remains empty.
     */
    network_map_t network_map;

    /**
     * Next network identifier to assign when the server spawns a
     * new networked entity.  Each server instance is responsible for
     * guaranteeing uniqueness of network IDs across the lifetime of
     * the session.  Clients leave this at zero.
     */
    uint32_t next_network_id;

    /**
     * @brief Pending destroy list for server‑side entity removal.
     *
     * When an entity with a NetworkedEntity component is destroyed on the
     * server, its network_id should be queued here via
     * network_cs_mark_entity_destroy().  During the next network tick the
     * queued IDs will be sent to all clients as part of the delta
     * snapshot with a zero component count, signalling clients to
     * destroy their corresponding local entity and remove the mapping.
     * After transmitting to all peers the list is cleared.
     */
    uint32_t *pending_destroy_ids;
    size_t pending_destroy_count;
    size_t pending_destroy_capacity;
} network_cs_t;

/**
 * @brief Assigns a globally unique network ID to a newly created entity and
 *        attaches a NetworkedEntity component to it.  Only valid on the
 *        server.  Clients should never call this.
 *
 * This helper assigns the next available network ID from the
 * network_cs_t instance, increments the counter, and registers a
 * NetworkedEntity component with the specified interest groups on
 * the given entity.  It should be invoked immediately after
 * creating any entity that should be replicated to clients.
 *
 * @param cs Pointer to the network_cs_t instance (must be server).
 * @param e  The ECS entity to which to attach the NetworkedEntity component.
 * @param interest_groups Bitmask of groups this entity belongs to.
 */
static inline void network_cs_assign_network_id(network_cs_t* cs, entity_t e, interest_mask_t interest_groups) {
    if (!cs || !cs->ecs) return;
    // Only the server generates network IDs.  The client leaves next_network_id at 0.
    uint32_t nid = cs->next_network_id++;
    networked_entity_t ne = { .network_id = nid, .interest_groups = interest_groups };
    ecs_add_component(cs->ecs, e, COMPONENT_NETWORKED_ENTITY, &ne);
}

/**
 * @brief Sets the interest mask for a connected peer.
 *
 * This helper simply wraps connection_manager_set_peer_interest() and
 * is provided for convenience at the architecture layer.  Only the
 * server side should call this to customise what entities each
 * client receives.  On the client this has no effect.
 *
 * @param cs    Pointer to the network_cs_t instance.
 * @param peer_id Identifier of the peer whose interest mask will be updated.
 * @param mask  New bitmask of interest groups.
 */
static inline void network_cs_set_peer_interest(network_cs_t* cs, const char* peer_id, interest_mask_t mask) {
    if (!cs) return;
    connection_manager_set_peer_interest(&cs->connection_manager, peer_id, mask);
}

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
 * @param dt delta time.
 */
void network_cs_update(network_cs_t* network_cs, float dt);

/**
 * @brief Shuts down and frees all resources used by the client-server network architecture.
 * @param network_cs A pointer to the network_cs_t instance to destroy.
 */
void network_cs_destroy(network_cs_t* network_cs);

/**
 * @brief Queue a destruction event for a networked entity.
 *
 * When a replicated entity is removed on the server (e.g. when a
 * player disconnects and their paddle entity is destroyed), the
 * server should call this helper before calling ecs_destroy_entity().
 * It records the entity's network_id so that the next network tick
 * will transmit a delete message to all clients.  After sending the
 * message the server clears the queue.  Has no effect on clients.
 *
 * @param cs The network_cs_t instance (must be server).
 * @param entity The local ECS entity being destroyed.
 */
void network_cs_mark_entity_destroy(network_cs_t* cs, entity_t entity);

/**
 * @brief Queue a destruction event for an arbitrary network ID.
 *
 * This helper inserts a raw network_id into the pending destroy
 * queue.  It can be used if the caller already has the network_id
 * instead of the local entity.  Has no effect on clients.
 *
 * @param cs The network_cs_t instance (must be server).
 * @param network_id The network_id to enqueue for destruction.
 */
void network_cs_mark_network_id_destroy(network_cs_t* cs, uint32_t network_id);

#ifdef __cplusplus
}
#endif

