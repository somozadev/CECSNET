#ifndef ECS_INTERNAL_H
#define ECS_INTERNAL_H

#include "ecs_types.h"

/**
 * @brief Metadata for an entity.
 *
 * This structure holds internal bookkeeping information for an entity,
 * such as whether it is currently active (in use) in the ECS world.
 */
typedef struct {
    bool in_use; /**< Indicates if this entity slot is currently allocated. */
} entity_meta_t;

/**
 * @brief Storage container for a single component type.
 *
 * Each registered component type gets its own storage structure.
 * It contains:
 * - The descriptor describing the component type (size, name, serialization functions).
 * - A contiguous block of memory for all entities' data of this component type.
 * - Arrays to track which entities have this component (`used`)
 *   and which ones have been modified (`is_dirty`).
 */
struct component_storage_t {
    component_descriptor_t descriptor; /**< Metadata describing the component type. */
    void *data;                        /**< Contiguous memory storing component data for all entities. */
    bool used[MAX_ENTITIES];           /**< Flags indicating which entities have this component. */
    bool is_dirty[MAX_ENTITIES];       /**< Flags indicating which components have been modified. */
};

/**
 * @brief Internal structure representing the ECS world state.
 *
 * This holds:
 * - All entity metadata.
 * - Component storage for every registered component type.
 * - A list of registered systems and the total count.
 * - The number of registered components.
 *
 * This is the core ECS structure passed to all internal and public ECS functions.
 */
struct ecs_t {
    entity_meta_t entities[MAX_ENTITIES];      /**< Metadata for all possible entities. */
    component_storage_t components[MAX_COMPONENTS]; /**< Storage for all registered component types. */
    uint32_t registered_component_count;       /**< Total number of registered component types. */
    system_func_t systems[MAX_SYSTEMS];        /**< Array of registered system function pointers. */
    int system_count;                          /**< Current number of registered systems. */
};

/**
 * @brief Executes all registered ECS systems.
 *
 * This function iterates through the ECS world's registered system pool
 * and calls each system with the ECS state and the provided delta time.
 *
 * @param ecs Pointer to the ECS world state.
 * @param dt  Delta time in seconds since the last update.
 */
static void ecs_run_systems(ecs_t *ecs, float dt);

#endif
