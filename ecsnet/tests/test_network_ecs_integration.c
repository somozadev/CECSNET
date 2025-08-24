// test_network_ecs_integration.c
// Esqueleto de prueba que combina ECS y networking.
// Crea un servidor y un cliente, ambos con su propio ECS, y define callbacks
// de recepción para mostrar cómo se podría sincronizar entidades.

#include <stdio.h>
#include "../include/net_socket.h"
#include "../include/connection_manager.h"
#include "../include/network_cs.h"
#include "../include/network_architecture.h"
#include "../include/ecs.h"
#include "../include/ecs_internal.h"
#include "../include/ecs_types.h"

// Reception callback: Prints received packet length
static void on_receive(void *user_data, peer_t *peer, const void *data, int len) {
    (void)user_data;
    (void)peer;
    printf("Received packet of %d bytes\n", len);
}

int main(void) {
    // Creates ECS for server and client
    net_socket_init();

    ecs_t ecs_server;
    ecs_t ecs_client;
    ecs_init(&ecs_server);
    ecs_init(&ecs_client);

    // Configure server
    network_architecture_t *arch_server = NULL;
    network_architecture_config_t config_server = {
        .type = ARCH_CLIENT_SERVER,
        .ip_address = "127.0.0.1",
        .port = 12345,
        .is_server = true,
        .tcp_port = 12345,
        .udp_port = 12346,
        .ecs_sync_hz = 20.0f,
        .on_peer_connected = NULL,
        .on_peer_disconnected = NULL,
        .on_packet_received = on_receive,
        .on_client_input = NULL,
        .user_data = NULL
    };
    network_architecture_init(&arch_server, &config_server, &ecs_server);

    // Configure client
    network_architecture_t *arch_client = NULL;
    network_architecture_config_t config_client = {
        .type = ARCH_CLIENT_SERVER,
        .ip_address = "127.0.0.1",
        .port = 12345,
        .is_server = false,
        .tcp_port = 12345,
        .udp_port = 123456,
        .ecs_sync_hz = 20.0f,
        .on_peer_connected = NULL,
        .on_peer_disconnected = NULL,
        .on_packet_received = on_receive,
        .on_client_input = NULL,
        .user_data = NULL
    };
    network_architecture_init(&arch_client, &config_client, &ecs_client);

    // Normally client and server connect via network_architecture_connect_to_server
    // and then entities state will be sent. In this test we only invoke network update.

    for (int i = 0; i < 5; ++i) {
        network_architecture_update(arch_server, 0.016f);
        network_architecture_update(arch_client, 0.016f);
    }

    // Destroy
    network_architecture_destroy(arch_client);
    network_architecture_destroy(arch_server);
    return 0;
}