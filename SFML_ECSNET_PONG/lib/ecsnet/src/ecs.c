#include <stdlib.h>
#include <string.h>
#include "ecs.h"
#include "ecs_internal.h"
#include <stdio.h> // Debug printing

void ecs_init(ecs_t* ecs)
{
    if (!ecs) return;

    // Fully reset entity and component storage to a clean state.
    // This ensures no "garbage" memory remains from previous use.
    memset(ecs->entities, 0, sizeof(ecs->entities));
    memset(ecs->components, 0, sizeof(ecs->components));

    ecs->registered_component_count = 0;
    ecs->system_count = 0;

    // Register built-in ECS components and systems.
    ecs_register_builtin_components(ecs);
    ecs_register_builtin_systems(ecs);

    // Reset 'is_dirty' flags for all registered components.
    // The memset above already covers this, but the loop is kept
    // for clarity and possible future explicit initialization needs.
    for (int i = 0; i < ecs->registered_component_count; i++) {
        memset(&ecs->components[i].is_dirty, 0, sizeof(ecs->components[i].is_dirty));
    }
}

void ecs_update(ecs_t* ecs, float dt) {
    // Execute all registered systems with the given delta time.
    ecs_run_systems(ecs, dt);
}

#pragma region ENTITIES
entity_t ecs_create_entity(ecs_t* ecs)
{
    // Search for the first unused entity slot.
    for (entity_t i = 0; i < MAX_ENTITIES; ++i)
    {
        if (!ecs->entities[i].in_use)
        {
            ecs->entities[i].in_use = true;
            ecs->registered_entities_count++;
            return i;
        }
    }
    // No free entity slots available.
    return (entity_t)-1;
}
entity_t ecs_try_create_entity_by_id(ecs_t* ecs, entity_t id)
{
    // Search for the first unused entity slot.
        if (!ecs->entities[id].in_use)
        {
            ecs->entities[id].in_use = true;
            ecs->registered_entities_count++;
            return id;
        }
    // No free entity slots available.
    return (entity_t)-1;
}

void ecs_destroy_entity(ecs_t* ecs, entity_t entity)
{
    if (entity >= MAX_ENTITIES || !ecs->entities[entity].in_use)
        return;

    ecs->entities[entity].in_use = false;

    // Remove all components associated with this entity.
    for (int i = 0; i < ecs->registered_component_count; ++i)
    {
        if (ecs->components[i].used[entity])
            ecs->components[i].used[entity] = false;
            ecs->registered_entities_count--;
    }
}

bool ecs_serialize_entity(ecs_t* ecs, entity_t entity, uint8_t* out_buffer, size_t* out_size, size_t max_buffer_size) {
    if (entity >= MAX_ENTITIES || !ecs->entities[entity].in_use) {
        *out_size = 0;
        return false;
    }

    uint8_t* buffer_ptr = out_buffer;
    int components_to_serialize = 0;
    size_t required_size = sizeof(int); // Space for storing the number of components.

    // Calculate how much space the serialized entity will take.
    for (int i = 0; i < ecs->registered_component_count; ++i) {
        if (ecs->components[i].used[entity]) {
            required_size += sizeof(component_t) + ecs->components[i].descriptor.size;
            components_to_serialize++;
        }
    }

    // Fail if the output buffer is too small.
    if (required_size > max_buffer_size) {
        *out_size = required_size;
        return false;
    }

    // Write the number of components first.
    memcpy(buffer_ptr, &components_to_serialize, sizeof(int));
    buffer_ptr += sizeof(int);

    // Serialize each component's ID and its data.
    for (int i = 0; i < ecs->registered_component_count; ++i) {
        if (ecs->components[i].used[entity]) {
            component_t component_id = i;

            memcpy(buffer_ptr, &component_id, sizeof(component_t));
            buffer_ptr += sizeof(component_t);

            ecs->components[i].descriptor.serialize(
                (uint8_t*)ecs->components[i].data + ecs->components[i].descriptor.size * entity,
                buffer_ptr
            );

            buffer_ptr += ecs->components[i].descriptor.size;
        }
    }

    *out_size = buffer_ptr - out_buffer;
    return true;
}

entity_t ecs_deserialize_entity(ecs_t* ecs, const uint8_t* in_buffer) {
    if (!in_buffer) return (entity_t)-1;

    const uint8_t* buffer_ptr = in_buffer;
    int components_to_deserialize;

    // Read the number of components to expect.
    memcpy(&components_to_deserialize, buffer_ptr, sizeof(int));
    buffer_ptr += sizeof(int);

    // Allocate a new entity slot.
    entity_t new_entity = ecs_create_entity(ecs);
    if (new_entity == (entity_t)-1) return (entity_t)-1;

    // Load each component into the new entity.
    for (int i = 0; i < components_to_deserialize; ++i) {
        component_t component_id;
        memcpy(&component_id, buffer_ptr, sizeof(component_t));
        buffer_ptr += sizeof(component_t);

        if (component_id < ecs->registered_component_count) {
            size_t component_size = ecs->components[component_id].descriptor.size;
            void* component_data = malloc(component_size);
            if (!component_data) {
                printf("Error: Memory allocation failed during deserialization.\n");
                ecs_destroy_entity(ecs, new_entity);
                return (entity_t)-1;
            }

            ecs->components[component_id].descriptor.deserialize(buffer_ptr, component_data);
            ecs_add_component(ecs, new_entity, component_id, component_data);
            free(component_data);

            buffer_ptr += component_size;
        } else {
            // Invalid component ID — likely corrupted or mismatched data.
            printf("Error: Deserialized component ID is out of range.\n");
            ecs_destroy_entity(ecs, new_entity);
            return (entity_t)-1;
        }
    }

    return new_entity;
}
#pragma endregion

#pragma region COMPONENTS
component_t ecs_register_component(ecs_t* ecs, component_descriptor_t descriptor)
{
    if (ecs->registered_component_count >= MAX_COMPONENTS)
        return (component_t)-1;

    component_t component_id = ecs->registered_component_count++;
    component_storage_t* storage = &ecs->components[component_id];

    storage->descriptor = descriptor;
    storage->data = calloc(MAX_ENTITIES, descriptor.size);
    if (!storage->data) {
        printf("Error: Failed to allocate memory for component data.\n");
        return (component_t)-1;
    }

    // Reset usage and dirty flags for all entities.
    memset(storage->used, 0, sizeof(storage->used));
    memset(storage->is_dirty, 0, sizeof(storage->is_dirty));

    return component_id;
}

bool ecs_add_component(ecs_t* ecs, entity_t entity, component_t component, void *data)
{
    if (entity >= MAX_ENTITIES || component >= ecs->registered_component_count)
        return false;

    component_storage_t* component_storage = &ecs->components[component];
    void *ptr = (uint8_t *)component_storage->data + component_storage->descriptor.size * entity;

    memcpy(ptr, data, component_storage->descriptor.size);
    component_storage->used[entity] = true;
    component_storage->is_dirty[entity] = true;

    return true;
}

void* ecs_get_component(ecs_t* ecs, entity_t entity, component_t component) {
    if (entity >= MAX_ENTITIES || component >= ecs->registered_component_count)
        return NULL;

    component_storage_t* component_storage = &ecs->components[component];
    if (!component_storage->used[entity]) return NULL;

    return (uint8_t*)component_storage->data + component_storage->descriptor.size * entity;
}

const char* ecs_get_component_name(ecs_t* ecs, component_t component) {
    if (component >= ecs->registered_component_count) {
        return NULL;
    }
    return ecs->components[component].descriptor.name;
}

bool ecs_has_component(ecs_t* ecs, entity_t entity, component_t component) {
    if (entity >= MAX_ENTITIES || component >= ecs->registered_component_count)
        return false;

    return ecs->components[component].used[entity];
}

bool ecs_remove_component(ecs_t* ecs, entity_t entity, component_t component) {
    if (entity >= MAX_ENTITIES || component >= ecs->registered_component_count)
        return false;

    ecs->components[component].used[entity] = false;
    ecs->components[component].is_dirty[entity] = true;

    return true;
}

void ecs_mark_component_dirty(ecs_t* ecs, entity_t entity, component_t component) {
    if (entity >= MAX_ENTITIES || component >= ecs->registered_component_count)
        return;
    ecs->components[component].is_dirty[entity] = true;
}

int ecs_get_dirty_components(ecs_t* ecs, entity_t entity, dirty_component_t* out_dirty_components) {
    if (entity >= MAX_ENTITIES || !ecs->entities[entity].in_use)
        return 0;

    int dirty_count = 0;
    for (int i = 0; i < ecs->registered_component_count; ++i) {
        if (ecs->components[i].used[entity] && ecs->components[i].is_dirty[entity]) {
            out_dirty_components[dirty_count].component_id = i;
            out_dirty_components[dirty_count].data = (uint8_t*)ecs->components[i].data + ecs->components[i].descriptor.size * entity;
            dirty_count++;
        }
    }
    return dirty_count;
}

void ecs_clear_component_dirty(ecs_t* ecs, entity_t entity, component_t component) {
    if (entity >= MAX_ENTITIES || component >= ecs->registered_component_count)
        return;
    ecs->components[component].is_dirty[entity] = false;
}
#pragma endregion

#pragma region SYSTEMS
void ecs_register_system(ecs_t* ecs, system_func_t func)
{
    if(ecs->system_count < MAX_SYSTEMS)
        ecs->systems[ecs->system_count++] = func;
}

void ecs_run_systems(ecs_t* ecs, float dt)
{
    // Call all registered systems, passing in ECS state and delta time.
    for(int i=0; i<ecs->system_count; ++i)
        ecs->systems[i](ecs, dt);
}
#pragma endregion
