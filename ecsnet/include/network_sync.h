#pragma once
#include "ecs.h"
#include "connection_manager.h"
#include "protocol_handler.h"

void ecs_network_sync_init();
void ecs_network_sync_track(entity_t entity);
void ecs_network_sync_send(connection_manager_t* connection_manager);
void ecs_network_sync_receive(const package_message_t* message);