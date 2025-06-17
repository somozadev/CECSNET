#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef uint32_t entity_t;
typedef uint32_t component_t;
typedef void (*system_func_t)(float dt);

typedef void (*serialize_func_t)(const void *data_in, uint8_t *buffer_out);
typedef void (*deserialize_func_t)(const uint8_t *buffer_in, void *data_out);
typedef struct
{
    size_t size;
    const char *name;
    serialize_func_t serialize;
    deserialize_func_t deserialize;
} component_descriptor_t;
