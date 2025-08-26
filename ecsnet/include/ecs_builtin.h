#ifndef ECS_BUILTIN_H
#define ECS_BUILTIN_H

#include <stdint.h>
#include "ecs_types.h"


#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Component data structure for entity position.
 */
typedef struct {
    float x; /**< X-axis position. */
    float y; /**< Y-axis position. */
} position_t;

/**
 * @brief Component data structure for entity rotation (quaternion format).
 */
typedef struct {
    float x; /**< X-axis rotation component. */
    float y; /**< Y-axis rotation component. */
    float z; /**< Z-axis rotation component. */
    float w; /**< W-axis rotation component (scalar part). */
} rotation_t;

/**
 * @brief Component data structure for entity transformation.
 * Combines position and rotation into a single component.
 */
typedef struct {
    position_t position; /**< Position component. */
    rotation_t rotation; /**< Rotation component. */
} transform_t;

/**
 * @brief Component data structure for entity velocity.
 */
typedef struct {
    float x; /**< X-axis velocity. */
    float y; /**< Y-axis velocity. */
} velocity_t;

// Forward declaration of the NetworkedEntity component ID (defined in ecs_builtin.c).
ECSNET_API extern component_t COMPONENT_NETWORKED_ENTITY;

/**
 * @brief ECS system responsible for applying velocity to position over time.
 * @param ecs Pointer to the ECS world.
 * @param dt  Delta time in seconds since the last update.
 */
ECSNET_API void system_movement(ecs_t *ecs, float dt);

/**
 * @brief Serialize a position component to a byte buffer.
 * @param data Pointer to the position_t structure.
 * @param out  Pointer to the output byte buffer.
 */
ECSNET_API  void serialize_position(const void *data, uint8_t *out);

/**
 * @brief Deserialize a position component from a byte buffer.
 * @param in   Pointer to the input byte buffer.
 * @param data Pointer to the position_t structure to populate.
 */
ECSNET_API  void deserialize_position(const uint8_t *in, void *data);

/**
 * @brief Serialize a rotation component to a byte buffer.
 * @param data Pointer to the rotation_t structure.
 * @param out  Pointer to the output byte buffer.
 */
ECSNET_API  void serialize_rotation(const void *data, uint8_t *out);

/**
 * @brief Deserialize a rotation component from a byte buffer.
 * @param in   Pointer to the input byte buffer.
 * @param data Pointer to the rotation_t structure to populate.
 */
ECSNET_API  void deserialize_rotation(const uint8_t *in, void *data);

/**
 * @brief Serialize a transform component to a byte buffer.
 * @param data Pointer to the transform_t structure.
 * @param out  Pointer to the output byte buffer.
 */
ECSNET_API  void serialize_transform(const void *data, uint8_t *out);

/**
 * @brief Deserialize a transform component from a byte buffer.
 * @param in   Pointer to the input byte buffer.
 * @param data Pointer to the transform_t structure to populate.
 */
ECSNET_API  void deserialize_transform(const uint8_t *in, void *data);

/**
 * @brief Serialize a velocity component to a byte buffer.
 * @param data Pointer to the velocity_t structure.
 * @param out  Pointer to the output byte buffer.
 */
ECSNET_API  void serialize_velocity(const void *data, uint8_t *out);

/**
 * @brief Deserialize a velocity component from a byte buffer.
 * @param in   Pointer to the input byte buffer.
 * @param data Pointer to the velocity_t structure to populate.
 */
ECSNET_API  void deserialize_velocity(const uint8_t *in, void *data);


#ifdef __cplusplus
}
#endif

#endif
