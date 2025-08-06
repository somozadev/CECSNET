#pragma once

#include "ecs_types.h"

typedef struct {
    component_t component_id;
    const void* data;
} dirty_component_t;

//Initialises the ecs engine, setting up the default builtin components 
void ecs_init(void);
//Creates a new entity and adds it to the entities pool
entity_t ecs_create_entity(void);
//Destroys a given entity and removes it from the entities pool
void ecs_destroy_entity(entity_t entity);
//Serialize an entire entity, including its components
bool ecs_serialize_entity(entity_t entity, uint8_t* out_buffer, size_t* out_size, size_t max_out_size);
//Deserializes an entity with its components from a given buffer
entity_t ecs_deserialize_entity(const uint8_t* in_buffer);
//Adds a given component to a given entity, being data the actual component data i.e position={0,0,0}. data = &position
bool ecs_add_component(entity_t entity, component_t component, void* data);
//Retrieves a component (id) used by an entity, returns null if such entity isn't using it
void* ecs_get_component(entity_t entity, component_t component);
//Returns a components name based on its ID or NULL if it doesn't exist
const char* ecs_get_component_name(component_t component);
//Checks if an entity is using a given component
bool ecs_has_component(entity_t entity, component_t component);
//Removes a given component from a given entity component's pool
bool ecs_remove_component(entity_t entity, component_t component);
//Marks a component as dirty for the NET layer. This method shall be called for any modified component inside a systems update function [ecs_run_systems(dt)]
void ecs_mark_component_dirty(entity_t entity, component_t component);
//Returns the number of dirty components found
int ecs_get_dirty_components(entity_t entity,dirty_component_t* out_dirty_components);
//Resets the is dirty flag
void ecs_clear_component_dirty(entity_t entity, component_t component);
//Registers a new component into the ecs engine
component_t ecs_register_component(component_descriptor_t component_descriptor);
//Registers a new system into the ecs engine
void ecs_register_system(system_func_t func);
//Update function to run the systems in the ecs engine systems pool internally
static void ecs_run_systems(float dt);
//Exposed update function, it calls ecs_run_systems(dt)
void ecs_update(float dt);