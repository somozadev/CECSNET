#pragma once

#include <stdint.h>
#include "ecs.h"
#include "ecs_types.h"
#include "connection_manager.h"
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief The maximum size of a network packet.
 *
 * This constant defines the maximum allowed size for a single network packet,
 * including the header and payload.
 */
#define MAX_PACKET_SIZE 1024
#define INPUT_UP     0x01
#define INPUT_DOWN   0x02
#define INPUT_SPAWN  0x80

/**
 * @brief Enumeration of all available packet types.
 *
 * This defines the type of message being sent, which dictates how the payload
 * should be interpreted by the receiver.
 */
typedef enum {
    PACKET_TYPE_INVALID, /**< Invalid or unknown packet type. */
    PACKET_TYPE_ENTITY_UPDATE, /**< Contains updates for one or more ECS entities. */
    PACKET_TYPE_MULTI_ENTITY_UPDATE, /**< Sent by the server to sync every entity and component into a new client. */
    PACKET_TYPE_CLIENT_REGISTER, /**< Sent by a client to register with the server. */
    PACKET_TYPE_SERVER_ACK, /**< Sent by the server to acknowledge a client's registration. */
    PACKET_TYPE_CLIENT_INPUT, /**< Ment to be used with clients input sending back to the server. */
} packet_type_t;

/**
 * @brief The header of a network packet.
 *
 * Contains metadata about the packet, such as its size and type.
 * Note on padding: The size is 2 bytes and the enum type is 4 bytes,
 * which results in 2 bytes of padding to align the struct to an 8-byte boundary.
 */
ECSNET_API typedef struct {
    uint16_t size; /**< The total size of the packet in bytes. */
    packet_type_t type; /**< The type of the packet. */
} packet_header_t;

/**
 * @brief A full network packet structure.
 *
 * This struct combines the header and the data payload, providing a complete
 * packet representation for sending and receiving.
 */
ECSNET_API typedef struct {
    packet_header_t header; /**< The packet header. */
    uint8_t data[MAX_PACKET_SIZE - sizeof(packet_header_t)]; /**< The packet payload. */
} network_packet_t;


/**
 * @brief The protocol handler structure.
 *
 * Manages the state for incoming and outgoing network packets, providing
 * functionality for packing, unpacking, and processing network data.
 */
ECSNET_API typedef struct protocol_handler_t {
    network_packet_t out_packet; /**< The buffer for the outgoing packet. */
    network_packet_t in_packet; /**< The buffer for the incoming packet. */
} protocol_handler_t;

/**
 * @brief Initializes the protocol handler.
 * @param handler A pointer to the protocol_handler_t instance to initialize.
 */
void protocol_handler_init(protocol_handler_t *handler);

/**
 * @brief Packs an entity update into the outgoing packet.
 * @param handler A pointer to the protocol_handler_t instance.
 * @param entity_id The ID of the entity being updated.
 * @param data A pointer to the serialized entity data.
 * @param data_len The length of the serialized data.
 */
void protocol_handler_pack_entity_update(protocol_handler_t *handler, entity_t entity_id, const uint8_t *data,
                                         uint16_t data_len);

/**
 * @brief Packs a client registration message into the outgoing packet.
 * @param handler A pointer to the protocol_handler_t instance.
 * @param udp_port The UDP port of the client.
 */
void protocol_handler_pack_client_register(protocol_handler_t *handler, uint16_t udp_port);
bool protocol_handler_unpack_client_register(const network_packet_t* pkt, uint16_t* out_port);

/**
 * @brief Packs a server acknowledgment message into the outgoing packet.
 * @param handler A pointer to the protocol_handler_t instance.
 */
void protocol_handler_pack_server_ack(protocol_handler_t *handler);
/**
 * @brief Pack a client input packet (PACKET_TYPE_CLIENT_INPUT).
 *        Layout: [entity_t][uint8_t cmd][extra...].
 *
 * @param handler   Initialized protocol handler to write into.
 * @param entity_id Target entity id (use 0 if not applicable).
 * @param input_cmd Command code (e.g., INPUT_UP/DOWN/SPAWN).
 * @param extra     Optional command-specific bytes (may be NULL).
 * @param extra_len Size of @p extra in bytes (0 ok; oversize is truncated).
 */
void protocol_handler_pack_client_input(protocol_handler_t* handler, entity_t entity_id, uint8_t input_cmd, const void* extra, uint16_t extra_len);

bool protocol_handler_unpack_client_input(const network_packet_t* pkt, entity_t* out_eid, uint8_t* out_cmd, const void** out_extra, uint16_t* out_extra_len);

/**
 * @brief Processes received network data.
 * @param handler A pointer to the protocol_handler_t instance.
 * @param peer A pointer to the peer that sent the data.
 * @param data A pointer to the raw received data.
 * @param len The length of the received data.
 */
void protocol_handler_process_received_data(protocol_handler_t *handler, ecs_t* ecs, peer_t *peer, const void *data, int len);

/**
 * @brief Sends the current outgoing packet to a specific peer.
 * @param cm A pointer to the connection_manager_t instance.
 * @param peer_id The ID of the peer to send the packet to.
 * @param handler A pointer to the protocol_handler_t instance.
 */
void protocol_handler_send_packet(connection_manager_t *cm, const char *peer_id, protocol_handler_t *handler);


#ifdef __cplusplus
}
#endif
