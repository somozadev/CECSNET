#pragma once
#include "ecs_types.h"

void ecs_init(void);
entity_t ecs_create_entity(void);
void ecs_destroy_entity(entity_t entity);


bool ecs_add_component(entity_t entity, component_t component, void* data);
void* ecs_get_component(entity_t entity, component_t component);
bool ecs_has_component(entity_t entity, component_t component);
bool ecs_remove_component(entity_t entity, component_t component);

component_t ecs_register_component(component_descriptor_t component_descriptor);

void ecs_update(float dt);