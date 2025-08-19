#ifndef ECS_TYPES_H
#define ECS_TYPES_H

#include <stdint.h>
#include "config.h"

/**
 * @brief Forward declaration of the ECS world structure.
 *
 * The full definition is provided in ecs.h.
 */
ECSNET_API typedef struct ecs_t ecs_t;

/**
 * @brief Unique identifier for an entity.
 *
 * Entities are represented internally as integers.
 */
typedef uint32_t entity_t;

/**
 * @brief Unique identifier for a component type.
 *
 * Components are represented internally as integers.
 */
typedef uint32_t component_t;

/**
 * @brief Function pointer type for ECS systems.
 *
 * A system is a function that operates on the ECS world each update cycle.
 *
 * @param ecs Pointer to the ECS world state.
 * @param dt  Delta time in seconds since the last update.
 */
typedef void (*system_func_t)(ecs_t *ecs, float dt);

/**
 * @brief Function pointer type for component serialization.
 *
 * This function should serialize a component's data into a byte buffer.
 *
 * @param data_in    Pointer to the component's source data.
 * @param buffer_out Pointer to the output byte buffer where the serialized data will be written.
 */
typedef void (*serialize_func_t)(const void *data_in, uint8_t *buffer_out);

/**
 * @brief Function pointer type for component deserialization.
 *
 * This function should reconstruct a component's data from a byte buffer.
 *
 * @param buffer_in Pointer to the byte buffer containing serialized component data.
 * @param data_out  Pointer to the memory location where the deserialized data will be stored.
 */
typedef void (*deserialize_func_t)(const uint8_t *buffer_in, void *data_out);

/**
 * @brief Descriptor for a component type.
 *
 * Contains metadata and function pointers required for managing
 * a specific component type within the ECS.
 */
ECSNET_API typedef struct component_descriptor_t component_descriptor_t;
typedef struct component_descriptor_t {
    size_t size;                       /**< Size of the component in bytes. */
    const char *name;                   /**< Human-readable component name. */
    serialize_func_t serialize;         /**< Pointer to the serialization function. */
    deserialize_func_t deserialize;     /**< Pointer to the deserialization function. */
} component_descriptor_t;

/**
 * @brief Forward declaration of the component storage structure.
 *
 * The full definition is provided in ecs_internal.h.
 */
ECSNET_API typedef struct component_storage_t component_storage_t;

#endif
