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

#define INPUT_UP 0x01
#define INPUT_DOWN 0x02

struct server_context_t {
    ecs_t* ecs;
    network_architecture_t* arch;
};
void send_full_state_to_peer(ecs_t* ecs, network_architecture_t* arch, peer_t* peer) {
    for (entity_t e = 0; e < ecs->registered_entities_count; ++e) {
        bool has_data = false;
        uint8_t sync_data[MAX_PACKET_SIZE];
        size_t sync_size = 0;

        for (component_t c = 0; c < ecs->registered_component_count; ++c) {
            if (!ecs_has_component(ecs, e, c))
                continue;

            const void* component_data = ecs_get_component(ecs, e, c);
            size_t component_size = ecs->components[c].descriptor.size;

            if (sync_size + sizeof(component_t) + component_size > MAX_PACKET_SIZE)
                break;

            memcpy(sync_data + sync_size, &c, sizeof(component_t));
            sync_size += sizeof(component_t);

            memcpy(sync_data + sync_size, component_data, component_size);
            sync_size += component_size;

            has_data = true;
        }

        if (has_data) {
            protocol_handler_t handler;
            protocol_handler_init(&handler);
            protocol_handler_pack_entity_update(&handler, e, sync_data, sync_size);
            protocol_handler_send_packet(&reinterpret_cast<network_cs_t *>(arch)->connection_manager, peer->id, &handler);
        }
    }
}
void on_peer_connected_callback(void* user_data, peer_t* peer) {
    auto* ctx = static_cast<server_context_t *>(user_data);
    if (!ctx || !ctx->arch || !ctx->ecs || !peer) return;
    send_full_state_to_peer(ctx->ecs, ctx->arch, peer);
}
void on_packet_received_callback(void* user_data, peer_t* peer, const void* data, int len) {
    ecs_t* ecs = (ecs_t*)user_data;
    const auto* packet = (const network_packet_t*)data;

    if (packet->header.type == PACKET_TYPE_CLIENT_INPUT) {
        // Handle player input
        const uint8_t *current_data = packet->data;
        int remaining_data = len - sizeof(packet_header_t);

        if (remaining_data < sizeof(entity_t) + sizeof(uint8_t)) {
            printf("[Server] Malformed input packet\n");
            return;
        }

        entity_t entity_id;
        uint8_t input_command;

        // Read entity ID and input command from the packet
        memcpy(&entity_id, current_data, sizeof(entity_t));
        current_data += sizeof(entity_t);
        memcpy(&input_command, current_data, sizeof(uint8_t));

        printf("[Server] Received input for entity %u: command %d\n", entity_id, input_command);

        velocity_t* paddle_vel = (velocity_t*)ecs_get_component(ecs, entity_id, COMPONENT_VELOCITY);
        if (paddle_vel) {
            if (input_command == INPUT_UP) {
                paddle_vel->y = -10.f;
            } else if (input_command == INPUT_DOWN) {
                paddle_vel->y = 10.f;
            } else {
                paddle_vel->y = 0.0f; // Stop moving
            }
            ecs_mark_component_dirty(ecs, entity_id, COMPONENT_VELOCITY);
            ecs_mark_component_dirty(ecs, entity_id, COMPONENT_POSITION);
        }
    }
}

void on_peer_disconnect_callback(void* user_data, peer_t* peer) {
    printf("[Server] Peer %s disconnected. \n", peer->id);
}

[[noreturn]] int main() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

    ecs_t server_ecs;
    ecs_init(&server_ecs);

    server_context_t ctx;
    ctx.ecs = &server_ecs;
    ctx.arch = nullptr;


    position_t ball_pos = {500.f, 100.f};
    velocity_t ball_vel = {0.0f, 10.f};
    entity_t ball = ecs_create_entity(&server_ecs);
    ecs_add_component(&server_ecs, ball, COMPONENT_POSITION, &ball_pos);
    ecs_add_component(&server_ecs, ball, COMPONENT_VELOCITY, &ball_vel);

    position_t ball2_pos = {100.f, 200.f};
    velocity_t ball2_vel = {0.0f, 5.f};
    entity_t ball2 = ecs_create_entity(&server_ecs);
    ecs_add_component(&server_ecs, ball2, COMPONENT_POSITION, &ball2_pos);
    ecs_add_component(&server_ecs, ball2, COMPONENT_VELOCITY, &ball2_vel);

    // entity_t paddle1 = ecs_create_entity(&server_ecs);
    // position_t paddle1_pos = {400.f, 250.f};
    // ecs_add_component(&server_ecs, paddle1, COMPONENT_POSITION, &paddle1_pos);    //
    // ecs_add_component(&server_ecs, paddle1, COMPONENT_VELOCITY, &paddle_vel);
    // entity_t paddle2 = ecs_create_entity(&server_ecs);
    // position_t paddle2_pos = {400.f, 450.f};
    // ecs_add_component(&server_ecs, paddle2, COMPONENT_POSITION, &paddle2_pos);
    // ecs_add_component(&server_ecs, paddle2, COMPONENT_VELOCITY, &paddle_vel);

    // Inicializar arquitectura cliente-servidor
    network_architecture_config_t server_config = {
        .type = ARCH_CLIENT_SERVER,
        .ip_address = "127.0.0.1",
        .is_server = true,
        .tcp_port = 51660,
        .udp_port = 51660,
        .on_peer_connected = on_peer_connected_callback,
        .on_peer_disconnected = on_peer_disconnect_callback,
        .on_packet_received = on_packet_received_callback,
        .user_data = &ctx,
    };
    network_architecture_t *server_arch= nullptr;
    network_architecture_init(&server_arch, &server_config, &server_ecs);
    ctx.arch = server_arch;

    sf::Clock clock;
    while (true) {
        float dt = clock.restart().asSeconds();
        ecs_update(&server_ecs, dt);
        ecs_mark_component_dirty(&server_ecs, ball,  COMPONENT_POSITION);
        ecs_mark_component_dirty(&server_ecs, ball2, COMPONENT_POSITION);
        network_architecture_update(server_arch, dt);

        std::this_thread::sleep_for(std::chrono::milliseconds(16));

;    }

    network_architecture_destroy(server_arch);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
