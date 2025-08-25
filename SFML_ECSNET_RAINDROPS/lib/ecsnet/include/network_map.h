// network_map.h
#pragma once

#include <stdint.h>
#include <stdlib.h>
#include "ecs.h"

/*
 * Simple mapping from global network IDs to local entity IDs.
 *
 * When a client receives updates from the server, the network ID is
 * used to look up (or create) the corresponding local entity.  The
 * mapping grows dynamically as new network IDs are seen.  Network
 * IDs should never be reused by the server; clients assume each
 * unique ID refers to a persistent entity.
 */

typedef struct {
    uint32_t network_id;
    entity_t local_id;
} network_id_pair_t;

typedef struct {
    network_id_pair_t *pairs;
    size_t count;
    size_t capacity;
} network_map_t;

static inline void network_map_init(network_map_t *map) {
    map->pairs = NULL;
    map->count = 0;
    map->capacity = 0;
}

static inline void network_map_destroy(network_map_t *map) {
    if (map && map->pairs) {
        free(map->pairs);
        map->pairs = NULL;
        map->count = 0;
        map->capacity = 0;
    }
}

// Look up a network ID.  Returns (entity_t)-1 if not found.
static inline entity_t network_map_lookup(network_map_t *map, uint32_t network_id) {
    if (!map || !map->pairs) return (entity_t)-1;
    for (size_t i = 0; i < map->count; ++i) {
        if (map->pairs[i].network_id == network_id) {
            return map->pairs[i].local_id;
        }
    }
    return (entity_t)-1;
}

// Insert a new mapping.  Does not check for duplicates.  Grows the
// underlying array when capacity is exceeded.
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