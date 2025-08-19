#include <SFML/Graphics.hpp>
#include <iostream>
#include "ecs.h"
#include "ecs_builtin.h"
#include "ecs_internal.h"
#include "network_cs.h"

#ifdef _WIN32
#include <winsock2.h>
#endif

#define COMPONENT_POSITION 0
#define COMPONENT_VELOCITY 1

ecs_t client_ecs;

void on_client_receive_callback(void* user_data, peer_t* peer, const void* data, int len) {
        ecs_t* ecs = (ecs_t*)user_data;
    const network_packet_t* packet = (const network_packet_t*)data;

    if (packet->header.type != PACKET_TYPE_ENTITY_UPDATE) {
        printf("[Client] Unknown packet type: %d\n", packet->header.type);
        return;
    }

    const uint8_t* current_data = packet->data;

    // entity_id from packet
    entity_t entity_id;
    memcpy(&entity_id, current_data, sizeof(entity_t));
    current_data += sizeof(entity_t);
    //first component ID
    uint32_t first_component_id;
    memcpy(&first_component_id, current_data, sizeof(uint32_t));
    current_data += sizeof(uint32_t);
    //Calculate remaining data
    int remaining_data = len - sizeof(packet_header_t) - sizeof(entity_t) - sizeof(uint32_t);

    printf("[Debug] Entity ID: %d, First Component ID: %d, Remaining data: %d bytes\n",
           entity_id, first_component_id, remaining_data);

    if (first_component_id == COMPONENT_POSITION) {
        if (remaining_data >= sizeof(position_t)) {
            position_t pos;
            memcpy(&pos, current_data, sizeof(position_t));
            current_data += sizeof(position_t);
            remaining_data -= sizeof(position_t);

            printf("[Client] Position received: x=%.2f, y=%.2f\n", pos.x, pos.y);
            ecs_add_component(ecs, entity_id, COMPONENT_POSITION, &pos);

            // Check if it's the expected position
        } else {
            printf("[Error] Not enough data for Position\n");
            return;
        }
    }
    if (remaining_data >= sizeof(uint32_t)) {
        uint32_t second_component_id;
        memcpy(&second_component_id, current_data, sizeof(uint32_t));
        current_data += sizeof(uint32_t);
        remaining_data -= sizeof(uint32_t);

        printf("[Debug] Second Component ID: %d\n", second_component_id);

        if (second_component_id == COMPONENT_VELOCITY) {
            if (remaining_data >= sizeof(velocity_t)) {
                velocity_t vel;
                memcpy(&vel, current_data, sizeof(velocity_t));
                current_data += sizeof(velocity_t);
                remaining_data -= sizeof(velocity_t);

                printf("[Client] Velocity received: x=%.2f, y=%.2f\n", vel.x, vel.y);
                ecs_add_component(ecs, entity_id, COMPONENT_VELOCITY, &vel);

            } else {
                printf("[Error] Not enough data for Velocity\n");
            }
        }
    }
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

    ecs_init(&client_ecs);
    ecs_register_builtin_components(&client_ecs);
    ecs_register_builtin_systems(&client_ecs);

    network_architecture_config_t server_config = {
        .type = ARCH_CLIENT_SERVER,
        .ip_address = "127.0.0.1",
        .is_server = true,
        .tcp_port = 12345,
        .udp_port = 12345
    };
    network_architecture_config_t client_config = {
        .type = ARCH_CLIENT_SERVER,
        .ip_address = "127.0.0.1",
        .is_server = false,
        .tcp_port = 12346,
        .udp_port = 12346
    };
    network_cs_t* client_arch = network_cs_init(&client_config, &client_ecs);
    client_arch->ecs = &client_ecs;
    client_arch->connection_manager.user_data = &client_ecs;
    client_arch->connection_manager.on_receive = on_client_receive_callback;

    connection_manager_connect_to_server(&client_arch->connection_manager, server_config.ip_address, server_config.tcp_port);

    sf::RenderWindow window(sf::VideoMode(800,600), "Pong ECSNET Client");

    sf::RectangleShape ballShape(sf::Vector2f(20.f, 20.f));
    ballShape.setFillColor(sf::Color::White);

    sf::Clock clock;
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        sf::Event event;
        while(window.pollEvent(event))
            if(event.type == sf::Event::Closed)
                window.close();

        network_cs_update(client_arch);
        ecs_update(&client_ecs, dt);
        position_t* ball_pos = nullptr;
        velocity_t* ball_vel = nullptr;
        entity_t ball_entity = 0;
        ball_pos = (position_t*)ecs_get_component(&client_ecs, ball_entity, COMPONENT_POSITION);
        ball_vel = (velocity_t*)ecs_get_component(&client_ecs, ball_entity, COMPONENT_VELOCITY);

        window.clear(sf::Color::Black);
        if (ball_pos) {
            ballShape.setPosition(ball_pos->x, ball_pos->y);
            window.draw(ballShape);
        }
        window.display();

        sf::sleep(sf::milliseconds(16));
    }

    network_cs_destroy(client_arch);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
