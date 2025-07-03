#pragma once
#include "ecs.h"
#include "connection_manager.h"
#include "protocol_handler.h"

typedef struct
{
    connection_manager_t* connection_manager; 
    void (*initialize)(void *self);
    void (*send_data)(void *self);
    void (*receive_data)(void *self);
    void (*update)(void *self);
    void *implemented_architecture;
} network_architecture_t;

void ecs_network_architecture_init(network_architecture_t *network_architecture);
void ecs_network_architecture_send(network_architecture_t *network_architecture);
void ecs_network_architecture_receive(network_architecture_t *network_architecture);
void ecs_network_architecture_update(network_architecture_t *network_architecture);