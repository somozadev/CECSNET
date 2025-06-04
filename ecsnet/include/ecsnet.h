#pragma once

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif

#include "ecs.h"
#include "network_sync.h"


EXPORT void ecsnet_initialize();
EXPORT void ecsnet_tick(float dt);
EXPORT void ecsnet_spawn_entity();
EXPORT void ecsnet_send();
EXPORT void ecsnet_receive();