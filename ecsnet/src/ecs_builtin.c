#include "ecs_builtin.h"
#include "ecs.h"
#include <string.h>

/**
 * @brief Macro to write a float value into a byte buffer.
 *
 * This performs a binary copy of a float into a memory location at the specified offset.
 */
#define WRITE_FLOAT(out, offset, value) memcpy((out) + (offset), &(value), sizeof(float))
#define WRITE_BOOLEAN(out, offset, value) memcpy((out) + (offset), &(value), sizeof(bool))

/**
 * @brief Macro to read a float value from a byte buffer.
 *
 * This performs a binary copy of a float from a memory location at the specified offset.
 */
#define READ_FLOAT(var, in, offset) memcpy(&(var), (in) + (offset), sizeof(float))

// Global component identifiers, initialized to an invalid value (-1).
component_t COMPONENT_POSITION = (component_t)-1;
component_t COMPONENT_ROTATION = (component_t)-1;
component_t COMPONENT_TRANSFORM = (component_t)-1;
component_t COMPONENT_VELOCITY = (component_t)-1;

/**
 * @brief Registers all built-in ECS systems into the given ECS world.
 *
 * Currently, this only registers the movement system.
 */
void ecs_register_builtin_systems(ecs_t* ecs)
{
    ecs_register_system(ecs, system_movement);
}

/**
 * @brief System function that applies velocity to position over time.
 *
 * Iterates over all entities, checks if they have both Position and Velocity components,
 * and updates their position based on their velocity and the delta time.
 * Marks the Position component as "dirty" so it can be synchronized or processed later.
 */
void system_movement(ecs_t* ecs, float dt)
{
    for (entity_t e = 0; e < MAX_ENTITIES; ++e)
    {
        if (ecs_has_component(ecs, e, COMPONENT_POSITION) &&
            ecs_has_component(ecs, e, COMPONENT_VELOCITY))
        {

            velocity_t* vel = ecs_get_component(ecs, e, COMPONENT_VELOCITY);
            if (!vel->is_moving) return;
            position_t* pos = ecs_get_component(ecs, e, COMPONENT_POSITION);

            pos->x += vel->x * dt;
            pos->y += vel->y * dt;

            ecs_mark_component_dirty(ecs, e, COMPONENT_POSITION);
        }
    }
}

/**
 * @brief Registers all built-in ECS components into the given ECS world.
 *
 * Each component is associated with its name, size, and serialization/deserialization functions.
 */
void ecs_register_builtin_components(ecs_t* ecs)
{
    COMPONENT_POSITION = ecs_register_component(ecs, (component_descriptor_t){
        .name = "Position",
        .size = sizeof(position_t),
        .serialize = serialize_position,
        .deserialize = deserialize_position
    });

    COMPONENT_ROTATION = ecs_register_component(ecs, (component_descriptor_t){
        .name = "Rotation",
        .size = sizeof(rotation_t),
        .serialize = serialize_rotation,
        .deserialize = deserialize_rotation
    });

    COMPONENT_TRANSFORM = ecs_register_component(ecs, (component_descriptor_t){
        .name = "Transform",
        .size = sizeof(transform_t),
        .serialize = serialize_transform,
        .deserialize = deserialize_transform
    });

    COMPONENT_VELOCITY = ecs_register_component(ecs, (component_descriptor_t){
        .name = "Velocity",
        .size = sizeof(velocity_t),
        .serialize = serialize_velocity,
        .deserialize = deserialize_velocity
    });
}

/* ====== Serialization / Deserialization Implementations ====== */

/**
 * @brief Serializes a position_t into a byte buffer.
 */
void serialize_position(const void *data, uint8_t *out)
{
    const position_t *pos = (const position_t *)data;
    WRITE_FLOAT(out, 0, pos->x);
    WRITE_FLOAT(out, 4, pos->y);
}

/**
 * @brief Deserializes a position_t from a byte buffer.
 */
void deserialize_position(const uint8_t *in, void *data)
{
    position_t *pos = (position_t *)data;
    READ_FLOAT(pos->x, in, 0);
    READ_FLOAT(pos->y, in, 4);
}

/**
 * @brief Serializes a rotation_t into a byte buffer.
 */
void serialize_rotation(const void *data, uint8_t *out)
{
    const rotation_t *rot = (const rotation_t *)data;
    WRITE_FLOAT(out, 0, rot->x);
    WRITE_FLOAT(out, 4, rot->y);
    WRITE_FLOAT(out, 8, rot->z);
    WRITE_FLOAT(out, 12, rot->w);
}

/**
 * @brief Deserializes a rotation_t from a byte buffer.
 */
void deserialize_rotation(const uint8_t *in, void *data)
{
    rotation_t *rot = (rotation_t *)data;
    READ_FLOAT(rot->x, in, 0);
    READ_FLOAT(rot->y, in, 4);
    READ_FLOAT(rot->z, in, 8);
    READ_FLOAT(rot->w, in, 12);
}

/**
 * @brief Serializes a transform_t into a byte buffer.
 *
 * The position is written first, followed by the rotation.
 */
void serialize_transform(const void *data, uint8_t *out)
{
    transform_t *transform = (transform_t *)data;
    serialize_position(&transform->position, out);
    serialize_rotation(&transform->rotation, out + 2 * sizeof(float));
}

/**
 * @brief Deserializes a transform_t from a byte buffer.
 *
 * The position is read first, followed by the rotation.
 */
void deserialize_transform(const uint8_t *in, void *data)
{
    transform_t *transform = (transform_t *)data;
    deserialize_position(in, &transform->position);
    deserialize_rotation(in + 2 * sizeof(float), &transform->rotation);
}

/**
 * @brief Serializes a velocity_t into a byte buffer.
 */
void serialize_velocity(const void *data, uint8_t *out)
{
    const velocity_t *vel = (const velocity_t *)data;
    WRITE_FLOAT(out, 0, vel->x);
    WRITE_FLOAT(out, 4, vel->y);
    WRITE_BOOLEAN(out, 8, vel->is_moving);
}

/**
 * @brief Deserializes a velocity_t from a byte buffer.
 */
void deserialize_velocity(const uint8_t *in, void *data)
{
    velocity_t *vel = (velocity_t *)data;
    READ_FLOAT(vel->x, in, 0);
    READ_FLOAT(vel->y, in, 4);
    READ_FLOAT(vel->is_moving, in, 8);
}
