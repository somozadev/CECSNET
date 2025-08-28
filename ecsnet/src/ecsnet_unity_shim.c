// ecsnet_unity_shim.c
#include "ecsnet_unity_shim.h"
#include "network_cs.h"
#include "protocol_handler.h"

ECSNET_API void ecsnet_send_input(network_architecture_t* arch, peer_t* serverPeer, uint8_t cmd) {
    if (!arch || !arch->impl || !serverPeer) return;
    protocol_handler_t h; protocol_handler_init(&h);
    protocol_handler_pack_client_input(&h, 0, cmd, NULL, 0);
    network_cs_t* cs = (network_cs_t*)arch->impl;
    protocol_handler_send_packet(&cs->connection_manager, serverPeer->id, &h);
}
