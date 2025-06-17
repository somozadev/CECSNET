#pragma once


#include "ecs.h"
#include "network_sync.h"


EXPORT void ecsnet_initialize();
EXPORT void ecsnet_tick(float dt);
EXPORT void ecsnet_spawn_entity();
EXPORT void ecsnet_send();
EXPORT void ecsnet_receive();