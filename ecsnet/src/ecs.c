#include <stdlib.h>
#include <string.h>
#include "ecs.h"
#include "ecs_internal.h"
#include <stdio.h> // Debug printing

// Expand the capacity for entity arrays and update all component storages accordingly.
static void ecs_expand_entities(ecs_t *ecs, size_t new_capacity) {
    if (!ecs || new_capacity <= ecs->entity_capacity) return;
    // Allocate new arrays
    entity_meta_t *new_entities = calloc(new_capacity, sizeof(entity_meta_t));
    component_signature_t *new_signatures = calloc(new_capacity, sizeof(component_signature_t));
    if (!new_entities || !new_signatures) {
        fprintf(stderr, "ECS expand: failed to allocate memory for %zu entities\n", new_capacity);
        return;
    }
    // Copy existing data
    memcpy(new_entities, ecs->entities, ecs->entity_capacity * sizeof(entity_meta_t));
    memcpy(new_signatures, ecs->signatures, ecs->entity_capacity * sizeof(component_signature_t));
    // Initialize new region
    memset(new_entities + ecs->entity_capacity, 0, (new_capacity - ecs->entity_capacity) * sizeof(entity_meta_t));
    memset(new_signatures + ecs->entity_capacity, 0, (new_capacity - ecs->entity_capacity) * sizeof(component_signature_t));
    // Update component storages to handle larger entity capacity
    for (size_t i = 0; i < ecs->registered_component_count; ++i) {
        component_storage_t *storage = &ecs->components[i];
        size_t old_cap = storage->capacity;
        size_t new_cap = new_capacity;
        // Reallocate component data
        void *new_data = calloc(new_cap, storage->descriptor.size);
        if (!new_data) {
            fprintf(stderr, "ECS expand: failed to allocate component data for component %zu\n", i);
            continue;
        }
        // Copy old data per entity
        for (size_t e = 0; e < ecs->entity_capacity; ++e) {
            memcpy((uint8_t*)new_data + storage->descriptor.size * e,
                   (uint8_t*)storage->data + storage->descriptor.size * e,
                   storage->descriptor.size);
        }
        free(storage->data);
        storage->data = new_data;
        // Reallocate used and is_dirty arrays
        bool *new_used = calloc(new_cap, sizeof(bool));
        bool *new_dirty = calloc(new_cap, sizeof(bool));
        if (!new_used || !new_dirty) {
            fprintf(stderr, "ECS expand: failed to allocate used/dirty arrays\n");
            // We intentionally leak to avoid corrupting existing pointers
        } else {
            memcpy(new_used, storage->used, ecs->entity_capacity * sizeof(bool));
            memcpy(new_dirty, storage->is_dirty, ecs->entity_capacity * sizeof(bool));
            free(storage->used);
            free(storage->is_dirty);
            storage->used = new_used;
            storage->is_dirty = new_dirty;
        }
        storage->capacity = new_cap;
    }
    // Free old arrays and assign new
    free(ecs->entities);
    free(ecs->signatures);
    ecs->entities = new_entities;
    ecs->signatures = new_signatures;
    ecs->entity_capacity = new_capacity;
}

// Expand the component storage array when new component types are registered beyond capacity
static void ecs_expand_component_array(ecs_t *ecs, size_t new_capacity) {
    if (!ecs || new_capacity <= ecs->component_capacity) return;
    component_storage_t *new_components = calloc(new_capacity, sizeof(component_storage_t));
    if (!new_components) {
        fprintf(stderr, "ECS expand components: allocation failed\n");
        return;
    }
    // Copy old component storages; the storage's internal pointers stay valid
    memcpy(new_components, ecs->components, ecs->registered_component_count * sizeof(component_storage_t));
    // Initialize new empty slots
    memset(new_components + ecs->component_capacity, 0, (new_capacity - ecs->component_capacity) * sizeof(component_storage_t));
    free(ecs->components);
    ecs->components = new_components;
    ecs->component_capacity = new_capacity;
}
ECSNET_API ecs_t* ecs_create(void) {
    ecs_t* ecs = calloc(1, sizeof(ecs_t));
    ecs_init(ecs);
    return ecs;
}

ECSNET_API void ecs_destroy(ecs_t* ecs) {
    if (!ecs) return;
    // libera entities, components, signatures…
    free(ecs->entities);
    free(ecs->signatures);
    free(ecs->components);
    free(ecs->systems);
    free(ecs);
}

// Initializes ECS core structures and registers built-in components/systems.
// Returns void; if allocation fails, prints error & ECS sits as partially initialized.
ECSNET_API void ecs_init(ecs_t* ecs)
{
    if (!ecs) return;

    // Initialize dynamic capacities
    ecs->entity_capacity = INITIAL_ENTITY_CAPACITY;
    ecs->component_capacity = INITIAL_COMPONENT_CAPACITY;
    ecs->registered_entities_count = 0;
    ecs->registered_component_count = 0;
    ecs->system_count = 0;

    // Allocate entity metadata and signatures
    ecs->entities = calloc(ecs->entity_capacity, sizeof(entity_meta_t));
    ecs->signatures = calloc(ecs->entity_capacity, sizeof(component_signature_t));
    // Allocate component storage array
    ecs->components = calloc(ecs->component_capacity, sizeof(component_storage_t));
    if (!ecs->entities || !ecs->components || !ecs->signatures) {
        fprintf(stderr, "ECS init: memory allocation failed\n");
        return;
    }

    // Register built-in ECS components and systems. These functions will
    // populate ecs->components using ecs_register_component(), which handles
    // dynamic allocation for component storages.
    ecs_register_builtin_components(ecs);
    ecs_register_builtin_systems(ecs);
}

// Execute all registered systems with the given delta time.
ECSNET_API void ecs_update(ecs_t* ecs, float dt) {
    ecs_run_systems(ecs, dt);
}

#pragma region ENTITIES

// Creates a new entity, returns its ID. Expands entity capacity if necessary (recursively).
ECSNET_API entity_t ecs_create_entity(ecs_t* ecs)
{
    if (!ecs) return (entity_t)-1;
    // Search for the first unused entity slot.
    for (entity_t i = 0; i < ecs->entity_capacity; ++i)
    {
        if (!ecs->entities[i].in_use)
        {
            ecs->entities[i].in_use = true;
            ecs->registered_entities_count++;
            ecs->signatures[i] = 0;
            return i;
        }
    }
    // No free entity slots available; expand capacity and try again.
    size_t new_capacity = ecs->entity_capacity == 0 ? INITIAL_ENTITY_CAPACITY : ecs->entity_capacity * 2;
    ecs_expand_entities(ecs, new_capacity);
    // After expanding, retry creation recursively.
    return ecs_create_entity(ecs);
}

// Tries to create a new entity with the given ID. Returns the entity ID if successful, or -1 if the ID is already in use.
ECSNET_API entity_t ecs_try_create_entity_by_id(ecs_t* ecs, entity_t id)
{
    if (!ecs) return (entity_t)-1;
    // Ensure the entity array is large enough.
    if (id >= ecs->entity_capacity) {
        size_t new_capacity = ecs->entity_capacity;
        while (new_capacity <= id) new_capacity *= 2;
        ecs_expand_entities(ecs, new_capacity);
    }
    if (!ecs->entities[id].in_use) {
        ecs->entities[id].in_use = true;
        ecs->registered_entities_count++;
        ecs->signatures[id] = 0;
        return id;
    }
    return (entity_t)-1;
}
// Destroys the entity with the given ID.
ECSNET_API void ecs_destroy_entity(ecs_t* ecs, entity_t entity)
{
    if (!ecs || entity >= ecs->entity_capacity || !ecs->entities[entity].in_use)
        return;

    ecs->entities[entity].in_use = false;
    ecs->registered_entities_count--;
    // Remove all components associated with this entity and clear signature bit
    for (uint32_t i = 0; i < ecs->registered_component_count; ++i)
    {
        component_storage_t *storage = &ecs->components[i];
        if (storage->used && storage->used[entity]) {
            storage->used[entity] = false;
            storage->is_dirty[entity] = true;
        }
    }
    // Clear signature bits
    ecs->signatures[entity] = 0;
}
//Serializes all components of an entity into out_buffer.
//- out_size: returns the used size.
// - max_buffer_size: max out size allowed.
// Returns false if buffer isn't sufficient or the entity is invalid.
ECSNET_API bool ecs_serialize_entity(ecs_t* ecs, entity_t entity, uint8_t* out_buffer, size_t* out_size, size_t max_buffer_size) {
    if (!ecs || entity >= ecs->entity_capacity || !ecs->entities[entity].in_use) {
        *out_size = 0;
        return false;
    }

    uint8_t* buffer_ptr = out_buffer;
    int components_to_serialize = 0;
    size_t required_size = sizeof(int); // Space for storing the number of components.

    // Calculate how much space the serialized entity will take.
    for (uint32_t i = 0; i < ecs->registered_component_count; ++i) {
        component_storage_t *storage = &ecs->components[i];
        if (storage->used && storage->used[entity]) {
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
    for (uint32_t i = 0; i < ecs->registered_component_count; ++i) {
        component_storage_t *storage = &ecs->components[i];
        if (storage->used && storage->used[entity]) {
            component_t component_id = i;
            memcpy(buffer_ptr, &component_id, sizeof(component_t));
            buffer_ptr += sizeof(component_t);

            storage->descriptor.serialize(
                (uint8_t*)storage->data + storage->descriptor.size * entity,
                buffer_ptr
            );

            buffer_ptr += storage->descriptor.size;
        }
    }

    *out_size = buffer_ptr - out_buffer;
    return true;
}
// Creates a new entity from serialized data.
// Reads component count, IDs and data.
// Returns entity ID or -1 error (invalid ID, corrupted data or malloc fail).
ECSNET_API entity_t ecs_deserialize_entity(ecs_t* ecs, const uint8_t* in_buffer) {
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
// Registers a new component type, allocating storage for all entities.
// Returns the new component ID, or -1 on allocation failure.
ECSNET_API component_t ecs_register_component(ecs_t* ecs, component_descriptor_t descriptor)
{
    if (!ecs) return (component_t)-1;
    // Expand component array if needed
    if (ecs->registered_component_count >= ecs->component_capacity) {
        size_t new_capacity = ecs->component_capacity == 0 ? INITIAL_COMPONENT_CAPACITY : ecs->component_capacity * 2;
        ecs_expand_component_array(ecs, new_capacity);
    }
    component_t component_id = ecs->registered_component_count++;
    component_storage_t* storage = &ecs->components[component_id];

    storage->descriptor = descriptor;
    storage->capacity = ecs->entity_capacity;
    storage->data = calloc(storage->capacity, descriptor.size);
    storage->used = calloc(storage->capacity, sizeof(bool));
    storage->is_dirty = calloc(storage->capacity, sizeof(bool));
    if (!storage->data || !storage->used || !storage->is_dirty) {
        fprintf(stderr, "Error: Failed to allocate memory for component storage.\n");
        return (component_t)-1;
    }
    // Initialize usage and dirty flags to zero
    memset(storage->used, 0, storage->capacity * sizeof(bool));
    memset(storage->is_dirty, 0, storage->capacity * sizeof(bool));
    return component_id;
}
// Adds a component instance to an entity (copies data).
// Expands entity/component arrays if needed.
// Updates signature bits.
// Returns true on success, false on invalid params.
ECSNET_API bool ecs_add_component(ecs_t* ecs, entity_t entity, component_t component, void *data)
{
    if (!ecs || component >= ecs->registered_component_count) return false;
    // Ensure entity capacity
    if (entity >= ecs->entity_capacity) {
        size_t new_capacity = ecs->entity_capacity;
        while (new_capacity <= entity) new_capacity *= 2;
        ecs_expand_entities(ecs, new_capacity);
    }
    component_storage_t* component_storage = &ecs->components[component];
    // Ensure component storage arrays are large enough for this entity
    if (entity >= component_storage->capacity) {
        // Expand each component's arrays: triggered when entity capacity grew.
        ecs_expand_entities(ecs, entity + 1);
    }
    void *ptr = (uint8_t *)component_storage->data + component_storage->descriptor.size * entity;
    memcpy(ptr, data, component_storage->descriptor.size);
    component_storage->used[entity] = true;
    component_storage->is_dirty[entity] = true;
    // Update entity signature bit
    ecs->signatures[entity] |= (1ULL << component);
    return true;
}
// Returns pointer to component data of entity, or NULL si no existe/invalid.
ECSNET_API void* ecs_get_component(ecs_t* ecs, entity_t entity, component_t component) {
    if (!ecs || component >= ecs->registered_component_count || entity >= ecs->entity_capacity)
        return NULL;
    component_storage_t* storage = &ecs->components[component];
    if (!storage->used || !storage->used[entity]) return NULL;
    return (uint8_t*)storage->data + storage->descriptor.size * entity;
}
// Returns the registered name of a component, or NULL if the id is invalid.
ECSNET_API const char* ecs_get_component_name(ecs_t* ecs, component_t component) {
    if (component >= ecs->registered_component_count) {
        return NULL;
    }
    return ecs->components[component].descriptor.name;
}
// Checks if entity currently owns given component (via signature bit).
ECSNET_API bool ecs_has_component(ecs_t* ecs, entity_t entity, component_t component) {
    if (!ecs || component >= ecs->registered_component_count || entity >= ecs->entity_capacity)
        return false;
    return (ecs->signatures[entity] & (1ULL << component)) != 0;
}
// Returns true if component of entity is marked as dirty (modified).
ECSNET_API bool ecs_is_component_dirty(ecs_t* ecs, entity_t entity, component_t component) {
    if (!ecs || component >= ecs->registered_component_count || entity >= ecs->entity_capacity)
        return false;
    component_storage_t *storage = &ecs->components[component];
    if (!storage->is_dirty) return false;
    return storage->is_dirty[entity];
}
// Removes component from entity (sets used=false, dirty=true, clears signature bit).
// Returns false if invalid params.
ECSNET_API bool ecs_remove_component(ecs_t* ecs, entity_t entity, component_t component) {
    if (!ecs || component >= ecs->registered_component_count || entity >= ecs->entity_capacity)
        return false;
    component_storage_t *storage = &ecs->components[component];
    if (storage->used) storage->used[entity] = false;
    if (storage->is_dirty) storage->is_dirty[entity] = true;
    // Clear signature bit
    ecs->signatures[entity] &= ~(1ULL << component);
    return true;
}
// Sets a global callback invoked when any component becomes dirty.
static void (*ecs_dirty_hook)(entity_t) = NULL;

 // Marks a component as dirty (modified). Invokes the global ecs_dirty_hook callback if it's set.
ECSNET_API void ecs_mark_component_dirty(ecs_t* ecs, entity_t entity, component_t component) {
    if (!ecs || component >= ecs->registered_component_count || entity >= ecs->entity_capacity)
        return;
    component_storage_t *storage = &ecs->components[component];
    bool was_dirty = storage->is_dirty ? storage->is_dirty[entity] : false;
    if (storage->is_dirty) storage->is_dirty[entity] = true;
    if (!was_dirty && ecs_dirty_hook) ecs_dirty_hook(entity);
}
// Sets a global callback invoked when any component becomes dirty.
void ecs_set_dirty_hook(void (*hook)(entity_t)) {
    ecs_dirty_hook = hook;
}

// Fills out_dirty_components[] with all dirty components of an entity.
// Returns count of dirty components found.
ECSNET_API int ecs_get_dirty_components(ecs_t* ecs, entity_t entity, dirty_component_t* out_dirty_components) {
    if (!ecs || entity >= ecs->entity_capacity || !ecs->entities[entity].in_use)
        return 0;
    int dirty_count = 0;
    for (uint32_t i = 0; i < ecs->registered_component_count; ++i) {
        component_storage_t *storage = &ecs->components[i];
        if (storage->used && storage->used[entity] && storage->is_dirty && storage->is_dirty[entity]) {
            out_dirty_components[dirty_count].component_id = i;
            out_dirty_components[dirty_count].data = (uint8_t*)storage->data + storage->descriptor.size * entity;
            dirty_count++;
        }
    }
    return dirty_count;
}
// Resets dirty flag for a component of an entity.
ECSNET_API void ecs_clear_component_dirty(ecs_t* ecs, entity_t entity, component_t component) {
    if (!ecs || component >= ecs->registered_component_count || entity >= ecs->entity_capacity)
        return;
    component_storage_t *storage = &ecs->components[component];
    if (storage->is_dirty) storage->is_dirty[entity] = false;
}
#pragma endregion

#pragma region SYSTEMS
// Registers a system (function pointer). Max = MAX_SYSTEMS.
// Systems run each update with ecs + delta time.
ECSNET_API void ecs_register_system(ecs_t* ecs, system_func_t func)
{
    if(ecs->system_count < MAX_SYSTEMS)
        ecs->systems[ecs->system_count++] = func;
}
// Runs all registered systems sequentially.
// Each system can read/modify ECS state.
void ecs_run_systems(ecs_t* ecs, float dt)
{
    // Call all registered systems, passing in ECS state and delta time.
    for(int i=0; i<ecs->system_count; ++i)
        ecs->systems[i](ecs, dt);
}
#pragma endregion
