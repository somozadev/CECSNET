// ecsnet_unity_shim.c
#pragma once
#include "network_cs.h"
#include "config.h"
#include "protocol_handler.h"

ECSNET_API void ecsnet_send_input(network_architecture_t* arch, peer_t* serverPeer, uint8_t cmd);
