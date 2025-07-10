#pragma once
#include "ecs.h"

//Macro to write floats into memory with a given destination, offset (bytes) and size(bytes)
#define WRITE_FLOAT(out, offset, value) memcpy((out) + (offset), &(value), sizeof(float))
//Macro to reaf floats from memory with a given output, offset (bytes) and size(bytes)
#define READ_FLOAT(var, in, offset) memcpy(&(var), (in) + (offset), sizeof(float))

//Default position component
extern component_t COMPONENT_POSITION;
//Default rotation component
extern component_t COMPONENT_ROTATION;
//Default transform component
extern component_t COMPONENT_TRANSFORM;

//Definition  for position's component data
typedef struct
{
    float x, y;
} position_t;

//Definition  for rotation's component data
typedef struct
{
    float x, y, z, w;
} rotation_t;

//Definition  for transform's component data
typedef struct
{
    position_t position;
    rotation_t rotation;
} transform_t;

//Definition  for velocity's component data
typedef struct
{
    float x, y;
} velocity_t;

//Registers the default systems into the ecs engine 
void ecs_register_buiiltin_systems(void);

//Definition for movement system
static void system_movement(float dt);

//Registers the default components into the ecs engine
void ecs_register_builtin_components(void);
//Implementation of  component_descriptor_t serialize_func_t for position's serialization 
static void serialize_position(const void *data, uint8_t *out);
//Implementation of  component_descriptor_t deserialize_func_t for position's deserialization
static void deserialize_position(const uint8_t *in, void *data);

//Implementation of  component_descriptor_t serialize_func_t for rotation's serialization 
static void serialize_rotation(const void *data, uint8_t *out);
//Implementation of  component_descriptor_t deserialize_func_t for rotation's deserialization
static void deserialize_rotation(const uint8_t *in, void *data);

//Implementation of  component_descriptor_t serialize_func_t for transform's serialization 
static void serialize_transform(const void *data, uint8_t *out);
//Implementation of  component_descriptor_t deserialize_func_t for transform's deserialization
static void deserialize_transform(const uint8_t *in, void *data);

//Implementation of  component_descriptor_t serialize_func_t for velocity's serialization 
static void serialize_velocity(const void *data, uint8_t *out);
//Implementation of  component_descriptor_t deserialize_func_t for velocity's deserialization
static void deserialize_velocity(const uint8_t *in, void *data);
