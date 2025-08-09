#include "protocol_handler.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "ecs.h"

void protocol_handler_init(protocol_handler_t* handler)
{
    if (!handler) {
        return;
    }
    // Initialise out_packet with a size of 0
    memset(&handler->out_packet, 0, sizeof(handler->out_packet));
    handler->out_packet.header.type = MSG_TYPE_ENTITY_SNAPSHOT;
    handler->out_packet.header.size = sizeof(packet_header_t);

}

int protocol_handler_pack_entity_update(protocol_handler_t* handler, entity_t entity, uint8_t* serialized_data, size_t data_size)
{
    if (!handler || !serialized_data || data_size == 0) {
        return -1;
    }
    //Calculate total size needed for the given entity (id + serialized data)
    size_t required_size = sizeof(entity_t) + sizeof(size_t) + data_size;

    //Verify if out_packet has enough space
    if (handler->out_packet.header.size + required_size > MAX_PACKET_SIZE) {
        return -2;//package is full, cannot add more data
    }

    // Add entity's ID
    memcpy(handler->out_packet.payload + (handler->out_packet.header.size - sizeof(packet_header_t)), &entity, sizeof(entity_t));
    handler->out_packet.header.size += sizeof(entity_t);

    //Add serialized data size
    memcpy(handler->out_packet.payload + (handler->out_packet.header.size - sizeof(packet_header_t)), &data_size, sizeof(size_t));
    handler->out_packet.header.size += sizeof(size_t);

    // Add the actual serialized data from the component
    memcpy(handler->out_packet.payload + (handler->out_packet.header.size - sizeof(packet_header_t)), serialized_data, data_size);
    handler->out_packet.header.size += data_size;

    return (int)required_size;
}

int protocol_handler_send_packet(protocol_handler_t* handler, connection_manager_t* cm, peer_t* peer)
{
    if (!handler || !cm || !peer || handler->out_packet.header.size <= sizeof(packet_header_t)) {
        return -1; // No hay datos para enviar o parámetros inválidos
    }

    // Enviar el paquete a través del connection manager
    int bytes_sent = connection_manager_send_to_peer(cm, peer->id, &handler->out_packet, handler->out_packet.header.size);

    // Resetear el paquete de salida después de enviarlo
    handler->out_packet.header.size = sizeof(packet_header_t);

    return bytes_sent;
}

void protocol_handler_process_received_data(peer_t* peer, const void* data, int len)
{
    if (!data || len < sizeof(packet_header_t)) {
        return;
    }

    const network_packet_t* packet = (const network_packet_t*)data;

    // Verificar que el tamaño del paquete concuerda con la cabecera
    if (packet->header.size != len) {
        printf("[ProtocolHandler] Error: Packet size mismatch. Expected %d, got %d.\n", packet->header.size, len);
        return;
    }

    switch (packet->header.type) {
        case MSG_TYPE_ENTITY_SNAPSHOT:
            {
                printf("[ProtocolHandler] Received ENTITY_SNAPSHOT from peer %s. Total size: %d bytes.\n", peer->id, len);

                size_t offset = 0;
                size_t payload_size = len - sizeof(packet_header_t);

                while (offset < payload_size) {
                    entity_t entity_id;
                    size_t data_size;

                    // Leer el ID de la entidad y el tamaño de los datos
                    memcpy(&entity_id, packet->payload + offset, sizeof(entity_t));
                    offset += sizeof(entity_t);
                    memcpy(&data_size, packet->payload + offset, sizeof(size_t));
                    offset += sizeof(size_t);

                    // Verificar que hay suficientes datos para leer
                    if (offset + data_size > payload_size) {
                        printf("[ProtocolHandler] Error unpacking entity %d: Incomplete data in packet.\n", entity_id);
                        break;
                    }

                    // Llamar a la función de deserialización de tu ECS
                    ecs_deserialize_entity(packet->payload + offset); //warning: comment this line if testing with fake ecs in test_networking()
                    printf("  -> Deserialized entity %d (%zu bytes).\n", entity_id, data_size);

                    offset += data_size;
                }
            }
            break;
        case MSG_TYPE_INPUT:
            printf("[ProtocolHandler] Received INPUT message from peer %s.\n", peer->id);
            // Aquí iría la lógica para procesar la entrada del jugador
            break;
        // Otros tipos de mensajes...
        default:
            printf("[ProtocolHandler] Received unknown message type %d from peer %s.\n", packet->header.type, peer->id);
            break;
    }
}