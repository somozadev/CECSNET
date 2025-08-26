// networked_entity.h
#pragma once

#include <stdint.h>
#include <string.h>
#include "ecs.h"

/**
 * @file networked_entity.h
 * @brief Defines the NetworkedEntity component used for network replication.
 *
 * Each entity that should be replicated across the network includes this
 * component. It stores a globally unique `network_id` assigned by the server
 * and a bitmask of `interest_groups` which determines which clients should
 * receive updates for this entity.
 */

/**
 * @typedef interest_mask_t
 * @brief Bitmask type for representing interest groups (up to 32 groups).
 */
typedef uint32_t interest_mask_t;
/**
 * @struct networked_entity_t
 * @brief Component representing network identity and interest groups.
 *
 * - `network_id`: Globally unique ID assigned by the server. Never reused.
 * - `interest_groups`: Bitmask of interest groups that determines which clients
 *   receive updates for this entity.
 */
typedef struct {
    uint32_t        network_id; ///< Globally unique ID assigned by the server.
    interest_mask_t interest_groups; ///< Bitmask of interest groups.
} networked_entity_t;

/**
 * @brief Global component ID for NetworkedEntity.
 * Defined in `ecs_builtin.c`. Used by the ECS to identify this component type.
 */
ECSNET_API extern component_t COMPONENT_NETWORKED_ENTITY;

/**
 * @brief Serializes a NetworkedEntity into an 8-byte buffer.
 * Format:
 * - 4 bytes: network_id
 * - 4 bytes: interest_groups
 *
 * @param src Pointer to the source NetworkedEntity component.
 * @param dst Output buffer where the serialized data is written (must be at least 8 bytes).
 */
static inline void networked_entity_serialize(const void *src, uint8_t *dst) {
    const networked_entity_t *ne = (const networked_entity_t *)src;
    memcpy(dst, &ne->network_id, sizeof(uint32_t));
    memcpy(dst + sizeof(uint32_t), &ne->interest_groups, sizeof(uint32_t));
}

/**
 * @brief Deserializes an 8-byte buffer into a NetworkedEntity.
 * The parameter order matches `deserialize_func_t`:
 * - The first argument is the input byte buffer (`const uint8_t*`).
 * - The second argument is the output pointer where the deserialized component will be written.
 * @param buffer_in Input buffer containing serialized data (at least 8 bytes).
 * @param data_out Pointer to the NetworkedEntity to populate.
 */
static inline void networked_entity_deserialize(const uint8_t *buffer_in, void *data_out) {
    networked_entity_t *ne = (networked_entity_t *)data_out;
    memcpy(&ne->network_id, buffer_in, sizeof(uint32_t));
    memcpy(&ne->interest_groups, buffer_in + sizeof(uint32_t), sizeof(uint32_t));
}

/**
 * @brief Registers the NetworkedEntity component in the ECS.
 * Should be called from `ecs_register_builtin_components()`.
 * Stores the resulting component ID into the global `COMPONENT_NETWORKED_ENTITY`.
 * @param ecs Pointer to the ECS instance where the component will be registered.
 */
static inline void ecs_register_networked_component(ecs_t *ecs) {
    component_descriptor_t desc = {
        .size = sizeof(networked_entity_t),
        .name = "NetworkedEntity",
        .serialize = networked_entity_serialize,
        .deserialize = networked_entity_deserialize,
    };
    COMPONENT_NETWORKED_ENTITY = ecs_register_component(ecs, desc);
}