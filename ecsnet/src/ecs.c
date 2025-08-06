#include <stdlib.h>
#include <string.h>
#include "ecs.h"
#include "ecs_builtin.h"

typedef struct
{
    bool in_use;
} entity_meta_t;

typedef struct
{
    component_descriptor_t descriptor;
    void *data;
    bool used[MAX_ENTITIES];
    bool is_dirty[MAX_ENTITIES]; //will tell the NET layer which components have been modified
} component_storage_t;



static entity_meta_t entities[MAX_ENTITIES];
static component_storage_t components[MAX_COMPONENTS];
static uint32_t registered_component_count = 0;

static system_func_t systems[MAX_SYSTEMS];
static int system_count = 0;

void ecs_init()
{
    memset(entities, 0, sizeof(entity_meta_t));
    memset(components, 0, sizeof(component_storage_t));
    registered_component_count = 0;
    ecs_register_builtin_components();

    for (int i = 0; i < MAX_COMPONENTS; i++) //initialise is_dirty flags
        memset(&components[i].is_dirty, 0, sizeof(components[i].is_dirty));
}

#pragma region ENTITIES
entity_t ecs_create_entity()
{
    for (entity_t i = 0; i < MAX_ENTITIES; ++i)
    {
        if (!entities[i].in_use)
        {
            entities[i].in_use = true;
            return i;
        }
    }
    return (entity_t)-1;
}

void ecs_destroy_entity(entity_t entity)
{
    if (entity >= MAX_ENTITIES || !entities[entity].in_use)
        return;

    entities[entity].in_use = false;
    for (int i = 0; i < registered_component_count; ++i)
    {
        if (components[i].used[entity])
            components[i].used[entity] = false;
    }
}
bool ecs_serialize_entity(entity_t entity, uint8_t* out_buffer, size_t* out_size, size_t max_buffer_size) {
    if (entity >= MAX_ENTITIES || !entities[entity].in_use) {
        *out_size = 0;
        return false;
    }
    uint8_t* buffer_ptr = out_buffer;
    int components_to_serialize = 0;
    size_t required_size = sizeof(int);
    for (int i = 0; i < registered_component_count; ++i) {
        if (components[i].used[entity]) {
            required_size += sizeof(component_t) + components[i].descriptor.size;
            components_to_serialize++;
        }
    }
    if (required_size > max_buffer_size) {
        *out_size = required_size;
        return false; // Buffer too small
    }
    // First write number of components
    memcpy(buffer_ptr, &components_to_serialize, sizeof(int));
    buffer_ptr += sizeof(int);

    // Second serialize and write data
    for (int i = 0; i < registered_component_count; ++i) {
        if (components[i].used[entity]) {
            // Component ID first
            memcpy(buffer_ptr, &i, sizeof(component_t));
            buffer_ptr += sizeof(component_t);

            // Component serialized data second
            components[i].descriptor.serialize((uint8_t*)components[i].data + components[i].descriptor.size * entity, buffer_ptr);
            buffer_ptr += components[i].descriptor.size;
        }
    }
    *out_size = buffer_ptr - out_buffer;
    return true;
}

entity_t ecs_deserialize_entity(const uint8_t* in_buffer) {
    if (!in_buffer) return (entity_t)-1;

    const uint8_t* buffer_ptr = in_buffer;
    int components_to_deserialize;

    // First get number of components
    memcpy(&components_to_deserialize, buffer_ptr, sizeof(int));
    buffer_ptr += sizeof(int);

    // Create the new entity
    entity_t new_entity = ecs_create_entity();
    if (new_entity == (entity_t)-1) return (entity_t)-1;

    for (int i = 0; i < components_to_deserialize; ++i) {
        component_t component_id;

        // Get component ID
        memcpy(&component_id, buffer_ptr, sizeof(component_t));
        buffer_ptr += sizeof(component_t);

        // Assumed a valid ID, deserialize data
        if (component_id < registered_component_count) {
            size_t component_size = components[component_id].descriptor.size;
            void* component_data = malloc(component_size);
            components[component_id].descriptor.deserialize(buffer_ptr, component_data);

            ecs_add_component(new_entity, component_id, component_data);
            free(component_data); // Free up temporal buffer

            buffer_ptr += component_size;
        }
    }

    return new_entity;
}

#pragma endregion
#pragma region COMPONENTS
component_t ecs_register_component(component_descriptor_t descriptor)
{
    if (registered_component_count >= MAX_COMPONENTS)
        return (component_t)-1;
    component_t component = registered_component_count++;
    components[component].descriptor = descriptor;
    components[component].data = calloc(MAX_ENTITIES, descriptor.size);
    memset(components[component].used, 0, sizeof(components[component].used));
    return component;
}

bool ecs_add_component(entity_t entity, component_t component, void *data)
{
    if (entity >= MAX_ENTITIES || component >= registered_component_count)
        return false;
    component_storage_t* component_storage = &components[component];
    void *ptr = (uint8_t *)component_storage->data + component_storage->descriptor.size * entity;
    memcpy(ptr, data, component_storage->descriptor.size);
    component_storage->used[entity]=true;
    component_storage->is_dirty[entity]=true;
    return true;
}

void* ecs_get_component(entity_t entity, component_t component) {
    if (entity >= MAX_ENTITIES || component >= registered_component_count)
        return NULL;
    component_storage_t* component_storage = &components[component];
    if (!component_storage->used[entity]) return NULL;
    return (uint8_t*)component_storage->data + component_storage->descriptor.size * entity;
}
const char* ecs_get_component_name(component_t component) {
    if (component >= registered_component_count) {
        return NULL;
    }
    return components[component].descriptor.name;
}
bool ecs_has_component(entity_t entity, component_t component) {
    if (entity >= MAX_ENTITIES || component >= registered_component_count)
        return false;
    return components[component].used[entity];
}

bool ecs_remove_component(entity_t entity, component_t component) {
    if (entity >= MAX_ENTITIES || component >= registered_component_count)
        return false;
    components[component].used[entity] = false;
    components[component].is_dirty[entity] = true;
    return true;
}
void ecs_mark_component_dirty(entity_t entity, component_t component) {
    if (entity >= MAX_ENTITIES || component >= registered_component_count)
        return;
    components[component].is_dirty[entity] = true;
}
int ecs_get_dirty_components(entity_t entity, dirty_component_t* out_dirty_components) {
    if (entity >= MAX_ENTITIES || !entities[entity].in_use)
        return 0;

    int dirty_count = 0;
    for (int i = 0; i < registered_component_count; ++i) {
        if (components[i].used[entity] && components[i].is_dirty[entity]) {
            out_dirty_components[dirty_count].component_id = i;
            out_dirty_components[dirty_count].data = (uint8_t*)components[i].data + components[i].descriptor.size * entity;
            dirty_count++;
        }
    }
    return dirty_count;
}

void ecs_clear_component_dirty(entity_t entity, component_t component) {
    if (entity >= MAX_ENTITIES || component >= registered_component_count)
        return;
    components[component].is_dirty[entity] = false;
}
#pragma endregion
#pragma region SYSTEMS
void ecs_register_system(system_func_t func)
{
    if(system_count < MAX_SYSTEMS)
        systems[system_count++] = func;
}
void ecs_run_systems(float dt)
{
    for(int i=0; i<system_count; ++i)
        systems[i](dt);
}
#pragma endregion

void ecs_update(float dt) {
    ecs_run_systems(dt);
}