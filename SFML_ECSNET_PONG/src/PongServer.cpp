//
// Created by mesom on 19/08/2025.
//

#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <SFML/System/Clock.hpp>

#include "ecs.h"
#include "ecs_builtin.h"
#include "ecs_internal.h"
#include "network_cs.h"

#ifdef _WIN32
#include <winsock2.h>
#endif

#define INPUT_UP   0x01
#define INPUT_DOWN 0x02

// Dimensiones del "mundo" (coinciden con la ventana del cliente)
static constexpr float WINDOW_WIDTH  = 800.f;
static constexpr float WINDOW_HEIGHT = 600.f;

static constexpr int   NUM_BALLS     = 150;
static constexpr float TOP_SPAWN_Y   = 5.f;    // ligeramente dentro de la pantalla
static constexpr float WRAP_RESET_Y  = -5.f;   // reaparece justo por encima

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
void on_packet_received_callback(void* user_data, peer_t* /*peer*/, const void* data, int len) {
    auto* ctx = static_cast<server_context_t*>(user_data);
    if (!ctx || !ctx->ecs) return;
    ecs_t* ecs = ctx->ecs;
    const auto* packet = static_cast<const network_packet_t*>(data);

    if (packet->header.type != PACKET_TYPE_CLIENT_INPUT) return;

    const uint8_t *current_data = packet->data;
    int remaining_data = len - sizeof(packet_header_t);
    if (remaining_data < (int)(sizeof(entity_t) + sizeof(uint8_t))) {
        printf("[Server] Malformed input packet\n");
        return;
    }

    entity_t entity_id;
    uint8_t input_command;
    memcpy(&entity_id, current_data, sizeof(entity_t)); current_data += sizeof(entity_t);
    memcpy(&input_command, current_data, sizeof(uint8_t));

    velocity_t* paddle_vel = (velocity_t*)ecs_get_component(ecs, entity_id, COMPONENT_VELOCITY);
    if (paddle_vel) {
        if (input_command == INPUT_UP)      paddle_vel->y = -10.f;
        else if (input_command == INPUT_DOWN) paddle_vel->y =  10.f;
        else                                 paddle_vel->y =   0.f;

        ecs_mark_component_dirty(ecs, entity_id, COMPONENT_VELOCITY);
        ecs_mark_component_dirty(ecs, entity_id, COMPONENT_POSITION);
    }
}


void on_peer_disconnect_callback(void* user_data, peer_t* peer) {
    printf("[Server] Peer %s disconnected.\n", peer->id);
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

    // ====== Crear 150 bolas en la parte superior ======
    // Distribuimos en X a lo largo del ancho y les damos velocidades Y positivas (hacia abajo).
    std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> dist_x(10.f, WINDOW_WIDTH - 10.f); // margen de 10 px a cada lado
    std::uniform_real_distribution<float> dist_vy(60.f, 180.f);              // velocidad hacia abajo
    std::uniform_real_distribution<float> dist_vx(-20.f, 20.f);              // un poco de drift lateral

    for (int i = 0; i < NUM_BALLS; ++i) {
        position_t pos = { dist_x(rng), TOP_SPAWN_Y };
        velocity_t vel = { dist_vx(rng), dist_vy(rng) };

        entity_t ball = ecs_create_entity(&server_ecs);
        ecs_add_component(&server_ecs, ball, COMPONENT_POSITION, &pos);
        ecs_add_component(&server_ecs, ball, COMPONENT_VELOCITY, &vel);
    }
    // ================================================

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

        // Actualiza simulación ECS (física/sistemas que ya tengas registrados)
        ecs_update(&server_ecs, dt);

        // ====== Wrap vertical: si se salen por abajo, vuelven arriba ======
        for (entity_t e = 0; e < server_ecs.registered_entities_count; ++e) {
            if (!ecs_has_component(&server_ecs, e, COMPONENT_POSITION)) continue;

            auto* pos = (position_t*)ecs_get_component(&server_ecs, e, COMPONENT_POSITION);
            if (!pos) continue;

            // Opcional: también puedes envolver en X si quieres que reaparezcan por los lados
            if (pos->x < -5.f)                pos->x = WINDOW_WIDTH + 5.f;
            else if (pos->x > WINDOW_WIDTH+5) pos->x = -5.f;

            if (pos->y > WINDOW_HEIGHT) {
                pos->y = WRAP_RESET_Y; // reaparece justo por encima de la ventana
                ecs_mark_component_dirty(&server_ecs, e, COMPONENT_POSITION);
            }
        }
        // ================================================================

        // Red/Net update
        network_architecture_update(server_arch, dt);

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    // Nunca se alcanza por el bucle infinito, pero lo dejamos por simetría:
    network_architecture_destroy(server_arch);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
