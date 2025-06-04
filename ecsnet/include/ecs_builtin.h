#pragma once
#include "ecs.h"

#define WRITE_FLOAT(out, offset, value) memcpy((out) + (offset), &(value), sizeof(float))
#define READ_FLOAT(var, in, offset) memcpy(&(var), (in) + (offset), sizeof(float))


extern component_t COMPONENT_POSITION;
extern component_t COMPONENT_ROTATION;
extern component_t COMPONENT_TRANSFORM;

typedef struct
{
    float x, y;
} position_t;
typedef struct
{
    float x, y, z, w;
} rotation_t;
typedef struct
{
    position_t position;
    rotation_t rotation;
} transform_t;

void ecs_register_builtin_components();
void serialize_position(const void *data, uint8_t *out);
void deserialize_position(const uint8_t *in, void *data);

void serialize_rotation(const void *data, uint8_t *out);
void deserialize_rotation(const uint8_t *in, void *data);

void serialize_transform(const void *data, uint8_t *out);
void deserialize_transform(const uint8_t *in, void *data);
