// test_network_architecture_basic.c
// Inicializa y destruye la arquitectura de red en modo servidor y cliente.

#include <stdio.h>
#include "../include/net_socket.h"
#include "../include/connection_manager.h"
#include "../include/network_cs.h"
#include "../include/network_architecture.h"
#include "../include/ecs.h"
#include "../include/ecs_internal.h"
#include "../include/ecs_types.h"

int main(void) {
    net_socket_init();

    ecs_t ecs_server;
    ecs_t ecs_client;
    ecs_init(&ecs_server);
    ecs_init(&ecs_client);

    network_architecture_t *arch_server = NULL;
    network_architecture_t *arch_client = NULL;

    // Configure server (listen server mode)
    network_architecture_config_t config_server = {
        .type = ARCH_CLIENT_SERVER,
        .ip_address = "127.0.0.1",
        .port = 0,
        .is_server = true,
        .tcp_port = 12345,
        .udp_port = 12346,
        .ecs_sync_hz = 60.0f,
        .on_peer_connected = NULL,
        .on_peer_disconnected = NULL,
        .on_packet_received = NULL,
        .on_client_input = NULL,
        .user_data = NULL
    };
    network_architecture_init(&arch_server, &config_server, &ecs_server);
    printf("Networking architecture (server) initialised.\n");

    // Configure client
    network_architecture_config_t config_client = {
        .type = ARCH_CLIENT_SERVER,
        .ip_address = "127.0.0.1",
        .port = 12345,
        .is_server = false,
        .tcp_port = 12345,
        .udp_port = 123456,
        .ecs_sync_hz = 60.0f,
        .on_peer_connected = NULL,
        .on_peer_disconnected = NULL,
        .on_packet_received = NULL,
        .on_client_input = NULL,
        .user_data = NULL
    };
    network_architecture_init(&arch_client, &config_client, &ecs_client);
    printf("Networking architecture (client) initialised.\n");

    // Update both architectures (no real connections in this test)
    network_architecture_update(arch_server, 0.016f);
    network_architecture_update(arch_client, 0.016f);

    // Destroy architectures
    network_architecture_destroy(arch_client);
    network_architecture_destroy(arch_server);
    printf("Architectures correctly destroyed.\n");
    net_socket_cleanup();
    return 0;
}