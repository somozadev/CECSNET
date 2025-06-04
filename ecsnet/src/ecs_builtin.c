#include "ecs_builtin.h"
#include <string.h>

component_t COMPONENT_POSITION = (component_t)-1;
component_t COMPONENT_ROTATION = (component_t)-1;
component_t COMPONENT_TRANSFORM = (component_t)-1;

void ecs_register_builtin_components()
{
    COMPONENT_POSITION = ecs_register_component((component_descriptor_t){
        .name = "Position",
        .size = sizeof(position_t),
        .serialize = serialize_position,
        .deserialize = deserialize_position});
    COMPONENT_ROTATION = ecs_register_component((component_descriptor_t){
        .name = "Rotation",
        .size = sizeof(rotation_t),
        .serialize = serialize_rotation,
        .deserialize = deserialize_rotation});
    COMPONENT_TRANSFORM = ecs_register_component((component_descriptor_t){
        .name = "Transform",
        .size = sizeof(transform_t),
        .serialize = serialize_transform,
        .deserialize = deserialize_transform});
}

void serialize_position(const void *data,  uint8_t *out)
{
    const position_t *pos = (const position_t *)data;
    WRITE_FLOAT(out, 0, pos->x);
    WRITE_FLOAT(out, 4, pos->y);
}
void deserialize_position(const uint8_t *in, void *data)
{
    position_t *pos = (position_t *)data;
    READ_FLOAT(pos->x, in, 0);
    READ_FLOAT(pos->y, in, 4);
}

void serialize_rotation(const void *data,  uint8_t *out)
{
    const rotation_t *rot = (const rotation_t *)data;
    WRITE_FLOAT(out, 0, rot->x);
    WRITE_FLOAT(out, 4, rot->y);
    WRITE_FLOAT(out, 8, rot->z);
    WRITE_FLOAT(out, 12, rot->w);
}
void deserialize_rotation(const uint8_t *in, void *data)
{
    rotation_t *rot = (rotation_t *)data;
    READ_FLOAT(rot->x, in, 0);
    READ_FLOAT(rot->y, in, 4);
    READ_FLOAT(rot->z, in, 8);
    READ_FLOAT(rot->w, in, 12);
}

void serialize_transform(const void *data,  uint8_t *out)
{
    transform_t *transform = (transform_t *)data;
    serialize_position(&transform->position, out);
    serialize_rotation(&transform->rotation, out + 2 * sizeof(float));
}
void deserialize_transform(const uint8_t *in, void *data)
{
    transform_t *transform = (transform_t *)data;
    deserialize_position(in, &transform->position);
    deserialize_rotation(in + 2 * sizeof(float), &transform->rotation);
}