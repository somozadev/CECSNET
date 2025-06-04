#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef uint32_t entity_t;
typedef uint32_t component_t;

typedef struct
{
    size_t size;
    const char *name;
    void (*serialize)(const void *data_in, uint8_t *buffer_out);
    void (*deserialize)(const void *data_out, uint8_t *buffer_in);
} component_descriptor_t;

