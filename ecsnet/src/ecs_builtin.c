#include "ecs_builtin.h"
#include <string.h>

component_t COMPONENT_POSITION = (component_t)-1;
component_t COMPONENT_ROTATION = (component_t)-1;
component_t COMPONENT_TRANSFORM = (component_t)-1;
component_t COMPONENT_VELOCITY = (component_t)-1;

void ecs_register_builtin_systems()
{
    ecs_register_system(system_movement);
}

void system_movement(float dt)
{
    for (entity_t e = 0; e < MAX_ENTITIES; ++e)
    {
        if (ecs_has_component(e, COMPONENT_POSITION) && ecs_has_component(e, COMPONENT_VELOCITY))
        {
            position_t* pos = ecs_get_component(e, COMPONENT_POSITION);
            velocity_t* vel = ecs_get_component(e, COMPONENT_VELOCITY);

            pos->x += vel->x * dt;
            pos->y += vel->y * dt;
        }
    }
}

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
    COMPONENT_VELOCITY = ecs_register_component((component_descriptor_t){
        .name = "Velocity",
        .size = sizeof(velocity_t),
        .serialize = serialize_velocity,
        .deserialize = deserialize_velocity});
}

void serialize_position(const void *data, uint8_t *out)
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

void serialize_rotation(const void *data, uint8_t *out)
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

void serialize_transform(const void *data, uint8_t *out)
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

void serialize_velocity(const void *data, uint8_t *out)
{
    const velocity_t *vel = (const velocity_t *)data;
    WRITE_FLOAT(out, 0, vel->x);
    WRITE_FLOAT(out, 4, vel->y);
}
void deserialize_velocity(const uint8_t *in, void *data)
{
    velocity_t *vel = (velocity_t *)data;
    READ_FLOAT(vel->x, in, 0);
    READ_FLOAT(vel->y, in, 4);
}
