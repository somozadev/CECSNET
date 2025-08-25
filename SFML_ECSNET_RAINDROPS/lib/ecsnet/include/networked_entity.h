// networked_entity.h
#pragma once

#include <stdint.h>
#include <string.h>
#include "ecs.h"

/*
 * Component representing network identity and interest groups for an entity.
 *
 * Each entity that should be replicated across the network gets this
 * component.  `network_id` is a globally unique identifier assigned by
 * the server and never reused.  `interest_groups` is a bitmask of up to
 * 32 groups indicating which clients should receive updates for this
 * entity.  Clients can subscribe to one or more interest groups; the
 * server will only send entities whose group mask overlaps with the
 * client's interest mask.
 */
typedef uint32_t interest_mask_t;

typedef struct {
    uint32_t        network_id;
    interest_mask_t interest_groups;
} networked_entity_t;

// Global component ID for NetworkedEntity, defined in ecs_builtin.c
extern component_t COMPONENT_NETWORKED_ENTITY;

// Serialize the component into an 8‑byte blob: 4 bytes network_id, 4 bytes mask
static inline void networked_entity_serialize(const void *src, uint8_t *dst) {
    const networked_entity_t *ne = (const networked_entity_t *)src;
    memcpy(dst, &ne->network_id, sizeof(uint32_t));
    memcpy(dst + sizeof(uint32_t), &ne->interest_groups, sizeof(uint32_t));
}

/*
 * Deserialize from an 8‑byte blob.
 *
 * The parameter order matches deserialize_func_t: the first argument is
 * the input byte buffer (const uint8_t*), and the second argument is the
 * output pointer where the deserialized component will be written.
 */
static inline void networked_entity_deserialize(const uint8_t *buffer_in, void *data_out) {
    networked_entity_t *ne = (networked_entity_t *)data_out;
    memcpy(&ne->network_id, buffer_in, sizeof(uint32_t));
    memcpy(&ne->interest_groups, buffer_in + sizeof(uint32_t), sizeof(uint32_t));
}

// Register the NetworkedEntity component.  This should be called from
// ecs_register_builtin_components().  Stores the resulting component ID
// into the global COMPONENT_NETWORKED_ENTITY.
static inline void ecs_register_networked_component(ecs_t *ecs) {
    component_descriptor_t desc = {
        .size = sizeof(networked_entity_t),
        .name = "NetworkedEntity",
        .serialize = networked_entity_serialize,
        .deserialize = networked_entity_deserialize,
    };
    COMPONENT_NETWORKED_ENTITY = ecs_register_component(ecs, desc);
}