#pragma once
#include <stdint.h>
#include <stdbool.h>

#define MAX_PAYLOAD_SIZE 512

typedef enum {
    MSG_TYPE_INPUT = 1,
    MSG_TYPE_SNAPSHOT = 2,
    MSG_TYPE_ACK = 3
} message_type_t;

typedef struct {
    message_type_t type;
    uint16_t size;
    uint8_t payload[MAX_PAYLOAD_SIZE
    ];
} message_t;

typedef struct {
    int (*pack)(const message_t *, uint8_t *, int);

    int (*unpack)(const uint8_t *, int, message_t *);

    bool (*validate)(const message_t *);
} protocol_handler_t;


int pack_message(const message_t *message, uint8_t *out_bufer, int max_len);

int unpack_message(const uint8_t *buffer, int length, message_t *out_message);

bool validate_message(const message_t *message);

void handle_message(const message_t *message);

typedef enum {
    MSG_ENTITY_CREATE = 0x01,
    MSG_ENTITY_UPDATE = 0x02,
    MSG_ENTITY_REMOVE = 0x03
} ecs_message_type_t;

typedef struct {
    uint8_t type; // ECS message type
    uint16_t entity_id; // Unique entity identifier
    uint8_t component_mask; // Bitmask representing which components are included
    uint8_t payload[128]; // Serialized component data
} package_message_t;

// Packs a message for entity update/create with a given component mask and raw data.
int pack_entity_update(package_message_t *message, uint16_t entity_id, uint8_t mask, void *data_in, int len);

// Unpacks a received message and extracts the entity ID, mask and component data.
int unpack_entity_update( package_message_t *message, uint16_t *entity_id, uint8_t *mask, void *data_out);
