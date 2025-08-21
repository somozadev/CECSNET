#ifndef ECS_H
#define ECS_H

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "ecs_types.h"
#include "config.h"
#include "ecs_builtin.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Dirty flag used by the networking layer to sync ECS data.
 * It marks whether an entity's component has been modified.
 */
typedef struct {
    component_t component_id;
    const void *data;
} dirty_component_t;

/**
 * @brief Initializes the ECS engine, setting up the default built-in components.
 * @param ecs A pointer to the ECS instance to initialize.
 */
ECSNET_API void ecs_init(ecs_t *ecs);

    /**
     * @brief Creates a new entity and adds it to the entities pool.
     * @param ecs A pointer to the ECS instance.
     * @return The ID of the newly created entity.
     */
    ECSNET_API entity_t ecs_create_entity(ecs_t *ecs);
    /**
     * @brief Tries to create a new entity by id and adds it to the entities pool.
     * @param ecs A pointer to the ECS instance.
     * @param id A given id.
     * @return The ID of the newly created entity.
     */
    ECSNET_API entity_t ecs_try_create_entity_by_id(ecs_t* ecs, entity_t id);

/**
 * @brief Destroys a given entity and removes it from the entities pool.
 * @param ecs A pointer to the ECS instance.
 * @param entity The ID of the entity to destroy.
 */
ECSNET_API void ecs_destroy_entity(ecs_t *ecs, entity_t entity);

/**
 * @brief Serializes an entire entity, including its components, into a buffer.
 * @param ecs A pointer to the ECS instance.
 * @param entity The ID of the entity to serialize.
 * @param out_buffer The output buffer to write the serialized data to.
 * @param out_size A pointer to a variable that will store the size of the serialized data.
 * @param max_out_size The maximum size of the output buffer.
 * @return True if serialization was successful, false otherwise.
 */
bool ecs_serialize_entity(ecs_t *ecs, entity_t entity, uint8_t *out_buffer, size_t *out_size, size_t max_out_size);

/**
 * @brief Deserializes an entity with its components from a given buffer.
 * @param ecs A pointer to the ECS instance.
 * @param in_buffer The input buffer containing the serialized entity data.
 * @return The ID of the deserialized entity.
 */
entity_t ecs_deserialize_entity(ecs_t *ecs, const uint8_t *in_buffer);

/**
 * @brief Adds a given component to a given entity.
 * @param ecs A pointer to the ECS instance.
 * @param entity The ID of the entity.
 * @param component The ID of the component to add.
 * @param data A pointer to the actual component data.
 * @return True if the component was added successfully, false otherwise.
 */
ECSNET_API bool ecs_add_component(ecs_t *ecs, entity_t entity, component_t component, void *data);

/**
 * @brief Retrieves a component from an entity.
 * @param ecs A pointer to the ECS instance.
 * @param entity The ID of the entity.
 * @param component The ID of the component to retrieve.
 * @return A pointer to the component's data, or NULL if the entity does not have the component.
 */
ECSNET_API void *ecs_get_component(ecs_t *ecs, entity_t entity, component_t component);

/**
 * @brief Returns a component's name based on its ID.
 * @param ecs A pointer to the ECS instance.
 * @param component The ID of the component.
 * @return The name of the component, or NULL if it doesn't exist.
 */
const char *ecs_get_component_name(ecs_t *ecs, component_t component);

/**
 * @brief Checks if an entity has a given component.
 * @param ecs A pointer to the ECS instance.
 * @param entity The ID of the entity.
 * @param component The ID of the component to check.
 * @return True if the entity has the component, false otherwise.
 */
ECSNET_API bool ecs_has_component(ecs_t *ecs, entity_t entity, component_t component);

/**
 * @brief Removes a given component from an entity.
 * @param ecs A pointer to the ECS instance.
 * @param entity The ID of the entity.
 * @param component The ID of the component to remove.
 * @return True if the component was removed successfully, false otherwise.
 */
ECSNET_API bool ecs_remove_component(ecs_t *ecs, entity_t entity, component_t component);

/**
 * @brief Marks a component as dirty for the networking layer.
 * This method should be called whenever a component is modified inside a system's update function.
 * @param ecs A pointer to the ECS instance.
 * @param entity The ID of the entity.
 * @param component The ID of the component to mark as dirty.
 */
void ecs_mark_component_dirty(ecs_t *ecs, entity_t entity, component_t component);

/**
 * @brief Gets the number of dirty components for a given entity.
 * @param ecs A pointer to the ECS instance.
 * @param entity The ID of the entity.
 * @param out_dirty_components An array to be filled with information about dirty components.
 * @return The number of dirty components found.
 */
int ecs_get_dirty_components(ecs_t *ecs, entity_t entity, dirty_component_t *out_dirty_components);

/**
 * @brief Resets the dirty flag for a component.
 * @param ecs A pointer to the ECS instance.
 * @param entity The ID of the entity.
 * @param component The ID of the component to clear the dirty flag for.
 */
void ecs_clear_component_dirty(ecs_t *ecs, entity_t entity, component_t component);

/**
 * @brief Registers a new component with the ECS engine.
 * @param ecs A pointer to the ECS instance.
 * @param component_descriptor A descriptor containing the component's name and size.
 * @return The ID of the newly registered component.
 */
ECSNET_API component_t ecs_register_component(ecs_t *ecs, component_descriptor_t component_descriptor);

/**
 * @brief Registers a new system with the ECS engine.
 * @param ecs A pointer to the ECS instance.
 * @param func The system function to register.
 */
ECSNET_API void ecs_register_system(ecs_t *ecs, system_func_t func);

/**
 * @brief The main ECS update function. It calls all registered systems.
 * @param ecs A pointer to the ECS instance.
 * @param dt The delta time since the last update.
 */
ECSNET_API void ecs_update(ecs_t *ecs, float dt);


// Default components.
ECSNET_API extern component_t COMPONENT_POSITION;
ECSNET_API extern component_t COMPONENT_ROTATION;
ECSNET_API extern component_t COMPONENT_TRANSFORM;
ECSNET_API extern component_t COMPONENT_VELOCITY;


/**
 * @brief Registers the default systems with the ECS engine.
 * @param ecs A pointer to the ECS instance.
 */
ECSNET_API void ecs_register_builtin_systems(ecs_t *ecs);

/**
 * @brief Registers the default components with the ECS engine.
 * @param ecs A pointer to the ECS instance.
 */
ECSNET_API void ecs_register_builtin_components(ecs_t *ecs);



#ifdef __cplusplus
}
#endif

#endif
