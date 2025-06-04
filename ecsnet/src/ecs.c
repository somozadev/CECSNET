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
} component_storage_t;

static entity_meta_t entities[MAX_ENTITIES];
static component_storage_t components[MAX_COMPONENTS];
static uint32_t registered_component_count = 0;

void ecs_init()
{
    memset(entities, 0, sizeof(entity_meta_t));
    memset(components, 0, sizeof(component_storage_t));
    registered_component_count = 0;
    ecs_register_builtin_components();
}

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
    return true;
}


void* ecs_get_component(entity_t entity, component_t component) {
    if (entity >= MAX_ENTITIES || component >= registered_component_count)
        return NULL;
    component_storage_t* component_storage = &components[component];
    if (!component_storage->used[entity]) return NULL;
    return (uint8_t*)component_storage->data + component_storage->descriptor.size * entity;
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
    return true;
}

void ecs_update(float dt) {
    // Vacío por ahora
}