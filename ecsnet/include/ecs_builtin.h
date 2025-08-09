#pragma once
#include <stdint.h>

//Definition  for positions component data
typedef struct
{
    float x, y;
} position_t;

//Definition  for rotations component data
typedef struct
{
    float x, y, z, w;
} rotation_t;

//Definition  for transforms component data
typedef struct
{
    position_t position;
    rotation_t rotation;
} transform_t;

//Definition  for velocitys component data
typedef struct
{
    float x, y;
} velocity_t;

//Definition for movement system
static void system_movement(float dt);

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
