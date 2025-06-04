#pragma once
#include <stdint.h>

typedef enum { 
    MSG_ENTITY_CREATE = 0x01,
    MSG_ENTITY_UPDATE = 0x02, 
    MSG_ENTITY_REMOVE = 0x03
} message_type_t;

typedef struct { 
    uint8_t type;
    uint16_t entity_id;
    uint8_t component_mask;
    uint8_t payload[128];
} package_message_t; 

int pack_entity_update(package_message_t* message, uint16_t entity_id, uint8_t mask, void* data_in, int len);
int unpack_entity_update(const package_message_t* message, uint16_t* entity_id, uint8_t* mask, void* data_out);