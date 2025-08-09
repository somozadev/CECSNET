#pragma once
#include <stdint.h>
#include <stdbool.h>

#include "connection_manager.h"
#include "ecs_types.h"

#define MAX_PACKET_SIZE 1024
#define MESSAGE_HEADER_SIZE (sizeof(uint8_t) + sizeof(uint16_t))

typedef enum {
    MSG_TYPE_INVALID = 0,
    MSG_TYPE_ENTITY_SNAPSHOT,
    MSG_TYPE_ENTITY_CREATE,
    MSG_TYPE_ENTITY_DESTROY,
    MSG_TYPE_INPUT,
    MSG_TYPE_ACK
} message_type_t;

typedef struct {
    message_type_t type;
    uint16_t size;
} packet_header_t;

typedef struct {
    packet_header_t header;
    uint8_t payload[MAX_PACKET_SIZE - sizeof(packet_header_t)];
} network_packet_t;


typedef struct {
    network_packet_t out_packet;
    //int (*on_receive_packet)(connection_manager_t*, peer_t*, network_packet_t*);
} protocol_handler_t;


// Inicializa el Protocol Handler
void protocol_handler_init(protocol_handler_t* handler);
// Empaqueta los cambios 'dirty' de una entidad en el paquete de salida.
// Devuelve el tamaño empaquetado o un código de error si el paquete está lleno.
int protocol_handler_pack_entity_update(protocol_handler_t* handler, entity_t entity, uint8_t* serialized_data, size_t data_size);
// Envía el paquete de salida actual a un peer. El paquete se resetea después del envío.
int protocol_handler_send_packet(protocol_handler_t* handler, connection_manager_t* cm, peer_t* peer);
// Procesa un paquete de red entrante.
// Esta es la función que se pasará como callback a connection_manager_t::on_receive.
void protocol_handler_process_received_data(peer_t* peer, const void* data, int len);
// Se asume que tienes esta función en otro lugar para obtener el handler global
protocol_handler_t* get_protocol_handler();
