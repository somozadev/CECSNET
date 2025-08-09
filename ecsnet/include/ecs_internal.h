#pragma once
#include "ecs.h"

typedef struct
{
    bool in_use;
} entity_meta_t;

typedef struct
{
    component_descriptor_t descriptor;
    void *data;
    bool used[MAX_ENTITIES];
    bool is_dirty[MAX_ENTITIES];
} component_storage_t;


static entity_meta_t entities[MAX_ENTITIES];
static component_storage_t components[MAX_COMPONENTS];
static uint32_t registered_component_count = 0;
static system_func_t systems[MAX_SYSTEMS];
static int system_count = 0;

//Update function to run the systems in the ecs engine systems pool internally
static void ecs_run_systems(float dt);
