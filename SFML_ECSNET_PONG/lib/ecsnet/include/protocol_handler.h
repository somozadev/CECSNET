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

/**
 * @brief Enumeration of all available packet types.
 *
 * This defines the type of message being sent, which dictates how the payload
 * should be interpreted by the receiver.
 */
typedef enum {
    PACKET_TYPE_INVALID,        /**< Invalid or unknown packet type. */
    PACKET_TYPE_ENTITY_UPDATE,  /**< Contains updates for one or more ECS entities. */
    PACKET_TYPE_CLIENT_REGISTER,/**< Sent by a client to register with the server. */
    PACKET_TYPE_SERVER_ACK      /**< Sent by the server to acknowledge a client's registration. */
} packet_type_t;

/**
 * @brief The header of a network packet.
 *
 * Contains metadata about the packet, such as its size and type.
 * Note on padding: The size is 2 bytes and the enum type is 4 bytes,
 * which results in 2 bytes of padding to align the struct to an 8-byte boundary.
 */
ECSNET_API typedef struct {
    uint16_t size;          /**< The total size of the packet in bytes. */
    packet_type_t type;     /**< The type of the packet. */
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
    network_packet_t in_packet;  /**< The buffer for the incoming packet. */
} protocol_handler_t;

/**
 * @brief Initializes the protocol handler.
 * @param handler A pointer to the protocol_handler_t instance to initialize.
 */
void protocol_handler_init(protocol_handler_t* handler);

/**
 * @brief Packs an entity update into the outgoing packet.
 * @param handler A pointer to the protocol_handler_t instance.
 * @param entity_id The ID of the entity being updated.
 * @param data A pointer to the serialized entity data.
 * @param data_len The length of the serialized data.
 */
void protocol_handler_pack_entity_update(protocol_handler_t* handler, entity_t entity_id, const uint8_t* data, uint16_t data_len);

/**
 * @brief Packs a client registration message into the outgoing packet.
 * @param handler A pointer to the protocol_handler_t instance.
 * @param udp_port The UDP port of the client.
 */
void protocol_handler_pack_client_register(protocol_handler_t* handler, uint16_t udp_port);

/**
 * @brief Packs a server acknowledgment message into the outgoing packet.
 * @param handler A pointer to the protocol_handler_t instance.
 */
void protocol_handler_pack_server_ack(protocol_handler_t* handler);

/**
 * @brief Processes received network data.
 * @param handler A pointer to the protocol_handler_t instance.
 * @param peer A pointer to the peer that sent the data.
 * @param data A pointer to the raw received data.
 * @param len The length of the received data.
 */
void protocol_handler_process_received_data(protocol_handler_t* handler, peer_t* peer, const void* data, int len);

/**
 * @brief Sends the current outgoing packet to a specific peer.
 * @param cm A pointer to the connection_manager_t instance.
 * @param peer_id The ID of the peer to send the packet to.
 * @param handler A pointer to the protocol_handler_t instance.
 */
void protocol_handler_send_packet(connection_manager_t* cm, const char* peer_id, protocol_handler_t* handler);

#ifdef __cplusplus
}
#endif

