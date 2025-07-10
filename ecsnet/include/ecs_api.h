#pragma once

#include "ecs_types.h"

//Initialises the ecs engine, setting up the default builtin components 
void ecs_init(void);
//Creates a new entity and adds it to the entities pool
entity_t ecs_create_entity(void);
//Destroys a given entity and removes it from the entities pool
void ecs_destroy_entity(entity_t entity);

//Adds a given component to a given entity, being data the actual component data i.e position={0,0,0}. data = &position 
bool ecs_add_component(entity_t entity, component_t component, void* data);
//Retrieves a component (id) used by an entity, returns null if such entity isn't using it
void* ecs_get_component(entity_t entity, component_t component);
//Checks if an entity is using a given component
bool ecs_has_component(entity_t entity, component_t component);
//Removes a given component from a given entity component's pool
bool ecs_remove_component(entity_t entity, component_t component);
//Registers a new component into the ecs engine
component_t ecs_register_component(component_descriptor_t component_descriptor);
//Registers a new system into the ecs engine
void ecs_register_system(system_func_t func);
//Update function to run the systems in the ecs engine systems pool internally
static void ecs_run_systems(float dt);
//Exposed update function, it calls ecs_run_systems(dt)
void ecs_update(float dt); 