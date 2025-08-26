// network_map.h
#pragma once

#include <stdint.h>
#include <stdlib.h>
#include "ecs.h"

/**
 * @brief Pair mapping between a global network ID and a local entity ID.
 */
typedef struct {
    uint32_t network_id; ///< Unique ID assigned by the server for an entity.
    entity_t local_id; ///< Corresponding local entity ID in the ECS.
} network_id_pair_t;

/**
 * @brief Dynamic mapping structure from network IDs to local entity IDs.
 * This map grows dynamically as new network IDs are encountered.
 * Clients rely on this to map persistent server-assigned IDs
 * to their local ECS entities.
 */
typedef struct {
    network_id_pair_t *pairs; ///< Array of ID pairs.
    size_t count; ///< Array of ID pairs.
    size_t capacity; ///< Allocated capacity of the pairs array.
} network_map_t;

/**
 * @brief Initializes an empty network map.
 * @param map Pointer to the network_map_t instance to initialize.
 */
static inline void network_map_init(network_map_t *map) {
    map->pairs = NULL;
    map->count = 0;
    map->capacity = 0;
}

/**
 * @brief Destroys a network map and frees allocated memory.
 * @param map Pointer to the network_map_t instance to destroy.
 */
static inline void network_map_destroy(network_map_t *map) {
    if (map && map->pairs) {
        free(map->pairs);
        map->pairs = NULL;
        map->count = 0;
        map->capacity = 0;
    }
}


/**
 * @brief Looks up the local entity ID associated with a given network ID.
 * @param map Pointer to the network_map_t instance.
 * @param network_id The global network ID to look up.
 * @return The corresponding local entity ID, or (entity_t)-1 if not found.
 */
static inline entity_t network_map_lookup(network_map_t *map, uint32_t network_id) {
    if (!map || !map->pairs) return (entity_t)-1;
    for (size_t i = 0; i < map->count; ++i) {
        if (map->pairs[i].network_id == network_id) {
            return map->pairs[i].local_id;
        }
    }
    return (entity_t)-1;
}

/**
 * @brief Inserts a new mapping into the network map.
 * @note This function does not check for duplicate network IDs.
 *       The underlying array grows dynamically if the capacity is exceeded.
 * @param map Pointer to the network_map_t instance.
 * @param network_id The global network ID to insert.
 * @param local_id The corresponding local entity ID to insert.
 */
static inline void network_map_insert(network_map_t *map, uint32_t network_id, entity_t local_id) {
    if (!map) return;
    if (map->count >= map->capacity) {
        size_t new_cap = map->capacity == 0 ? 16 : map->capacity * 2;
        network_id_pair_t *new_pairs = (network_id_pair_t *)realloc(map->pairs, new_cap * sizeof(network_id_pair_t));
        if (!new_pairs) return;
        map->pairs = new_pairs;
        map->capacity = new_cap;
    }
    map->pairs[map->count].network_id = network_id;
    map->pairs[map->count].local_id = local_id;
    map->count++;
}

/**
 * @brief Removes a mapping for a given network ID.
 * @param map Pointer to the network_map_t instance.
 * @param network_id The global network ID to remove.
 * @return 1 if the mapping was found and removed, 0 otherwise.
 */
static inline int network_map_remove(network_map_t *map, uint32_t network_id) {
    if (!map || !map->pairs) return 0;
    for (size_t i = 0; i < map->count; ++i) {
        if (map->pairs[i].network_id == network_id) {
            // Move the last element into this position and shrink the count
            map->pairs[i] = map->pairs[map->count - 1];
            map->count--;
            return 1;
        }
    }
    return 0;
}