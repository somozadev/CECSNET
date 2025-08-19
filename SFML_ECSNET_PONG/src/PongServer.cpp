//
// Created by mesom on 19/08/2025.
//

#include <iostream>
#include <thread>
#include <chrono>
#include <SFML/System/Clock.hpp>

#include "ecs.h"
#include "ecs_builtin.h"
#include "ecs_internal.h"
#include "network_cs.h"

#ifdef _WIN32
#include <winsock2.h>
#endif

// Component IDs asumidos por ECSNET
#define COMPONENT_POSITION 0
#define COMPONENT_VELOCITY 1


void on_server_connect_callback(void* user_data, peer_t* peer) {
    ecs_t* ecs = (ecs_t*)user_data;
    printf("[Server] Peer %s connected.\n", peer->id);

    for (entity_t e = 0; e < MAX_ENTITIES; ++e) {
        if (ecs_has_component(ecs, e, COMPONENT_POSITION)) {
            ecs_mark_component_dirty(ecs, e, COMPONENT_POSITION);
        }
        if (ecs_has_component(ecs, e, COMPONENT_VELOCITY)) {
            ecs_mark_component_dirty(ecs, e, COMPONENT_VELOCITY);
        }
    }
}

void on_server_disconnect_callback(void* user_data, peer_t* peer) {
    printf("[Server] Peer %s disconnected. \n", peer->id);
}

[[noreturn]] int main() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

    ecs_t server_ecs;
    ecs_init(&server_ecs);
    ecs_register_builtin_components(&server_ecs);
    ecs_register_builtin_systems(&server_ecs);

    // Crear entidades: pelota y paddles
    entity_t ball = ecs_create_entity(&server_ecs);
    position_t ball_pos = {400.f, 300.f};
    velocity_t ball_vel = {0.0f, 0.1f};
    ecs_add_component(&server_ecs, ball, COMPONENT_POSITION, &ball_pos);
    ecs_add_component(&server_ecs, ball, COMPONENT_VELOCITY, &ball_vel);
    //
    // entity_t paddle1 = ecs_create_entity(&server_ecs);
    // position_t paddle1_pos = {50.f, 250.f};
    // ecs_add_component(&server_ecs, paddle1, COMPONENT_POSITION, &paddle1_pos);
    //
    // entity_t paddle2 = ecs_create_entity(&server_ecs);
    // position_t paddle2_pos = {750.f, 250.f};
    // ecs_add_component(&server_ecs, paddle2, COMPONENT_POSITION, &paddle2_pos);

    // Inicializar arquitectura cliente-servidor
    network_architecture_config_t server_config = {
        .type = ARCH_CLIENT_SERVER,
        .ip_address = "127.0.0.1",
        .is_server = true,
        .tcp_port = 12345,
        .udp_port = 12345
    };
    network_cs_t* server_arch = network_cs_init(&server_config, &server_ecs);
    server_arch->ecs = &server_ecs;
    server_arch->connection_manager.user_data = &server_ecs;
    server_arch->connection_manager.on_connect = on_server_connect_callback;
    server_arch->connection_manager.on_disconnect = on_server_disconnect_callback;

    // Loop de servidor
    sf::Clock clock;
    while (true) {
        float dt = clock.restart().asSeconds();
        network_cs_update(server_arch);
        ecs_update(&server_ecs, dt);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    network_cs_destroy(server_arch);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
