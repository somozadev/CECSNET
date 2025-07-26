#pragma once
#include "ecs.h"
#include "connection_manager.h"
#include "protocol_handler.h"

typedef struct
{
    void (*initialize)(void *self);
    void (*send_data)(void *self);
    void (*receive_data)(void *self);
    void (*update)(void *self);    
    void (*destroy)(void*);
    void *implemented_architecture;
} network_architecture_t;

typedef struct { 
    net_socket_t server_socket;
    connection_manager_t* connection_manager; 

} network_architecture_client_server_t; 

typedef struct { 
    net_socket_t server_socket;
    connection_manager_t* connection_manager; 

} network_architecture_peer_to_peer_t; 


void ecs_network_architecture_init(void *self);
void ecs_network_architecture_send(void *self);
void ecs_network_architecture_receive(void *self);
void ecs_network_architecture_update(void *self);
void ecs_network_architecture_destroy(void *self);


network_architecture_t* create_client_server_architecture();
void client_server_init(void* self);
void client_server_update(void* self);
void destroy_client_server_architecture(void *self);

network_architecture_t* create_peer_to_peer_architecture();
void peer_to_peer_init(void* self);
void peer_to_peer_update(void* self);
void destroy_peer_to_peer_architecture(void *self);
