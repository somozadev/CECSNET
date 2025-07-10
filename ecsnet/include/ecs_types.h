#pragma once
#include <stdint.h>
#include <stdbool.h>

//Definition of entity, a integer working as an identifier
typedef uint32_t entity_t;
//Definition of component, a integer working as an identifier
typedef uint32_t component_t;
//Definition of system, a function with a delta time input, ment to be used in a update loop
typedef void (*system_func_t)(float dt);

//Definition of serialize component function, ment to be implemented for any specific component 
typedef void (*serialize_func_t)(const void *data_in, uint8_t *buffer_out);
//Definition of deserialize component function, ment to be implemented for any specific component 
typedef void (*deserialize_func_t)(const uint8_t *buffer_in, void *data_out);
//Definition holding description of the component for qol and implementation of serialization over networks functions
typedef struct
{
    size_t size;
    const char *name;
    serialize_func_t serialize;
    deserialize_func_t deserialize;
} component_descriptor_t;
