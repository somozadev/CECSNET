#include <SFML/Graphics.hpp>
#include <iostream>
#include "ecs.h"
#include "ecs_builtin.h"
#include "ecs_internal.h"
#include "network_cs.h"

#ifdef _WIN32
#include <winsock2.h>
#endif

#define INPUT_UP 0x01
#define INPUT_DOWN 0x02

ecs_t client_ecs;

void print_entity_table(ecs_t *ecs) {
    // Limpiar pantalla
    printf("\033[2J\033[H"); // ANSI escape: limpiar pantalla + mover cursor a (0,0)

    printf("| Entity ID | Component ID | Data Preview |\n");
    printf("|-----------|--------------|--------------|\n");

    for (int entity = 0; entity < ecs->registered_entities_count; ++entity) {
        for (int comp_id = 0; comp_id < ecs->registered_component_count; ++comp_id) {
            if (ecs_has_component(ecs, entity, comp_id)) {
                void *comp_data = ecs_get_component(ecs, entity, comp_id);
                printf("| %9d | %12d | %12p |\n", entity, comp_id, comp_data);
            }
        }
    }
}
void on_client_receive_callback(void *user_data, peer_t *peer, const void *data, int len) {
    auto *ecs = static_cast<ecs_t *>(user_data);
    const auto *packet = static_cast<const network_packet_t *>(data);

    if (packet->header.type != PACKET_TYPE_ENTITY_UPDATE) {
        printf("[Client] Unknown packet type: %d\n", packet->header.type);
        return;
    }

    const uint8_t *current_data = packet->data;
    int remaining_data = len - sizeof(packet_header_t);

    // 1) entity_id
    if (remaining_data < sizeof(entity_t)) {
        printf("[Client] Malformed packet: no entity id\n");
        return;
    }
    entity_t entity_id;
    memcpy(&entity_id, current_data, sizeof(entity_id));
    current_data += sizeof(entity_id);
    remaining_data -= sizeof(entity_id);
    ecs_try_create_entity_by_id(ecs,entity_id);


    // 2) leer todos los componentes que vengan
    while (remaining_data > sizeof(uint32_t)) {
        uint32_t comp_id;
        memcpy(&comp_id, current_data, sizeof(uint32_t));
        current_data += sizeof(uint32_t);
        remaining_data -= sizeof(uint32_t);

        size_t comp_size = ecs->components[comp_id].descriptor.size;
        if (remaining_data < comp_size) {
            printf("[Client] Malformed packet: not enough bytes for comp %u\n", comp_id);
            break;
        }

        ecs_add_component(ecs, entity_id, comp_id, (void*)current_data);

        current_data += comp_size;
        remaining_data -= comp_size;

        print_entity_table(ecs);
    }
    // const auto ecs = static_cast<ecs_t *>(user_data);
    // const auto *packet = static_cast<const network_packet_t *>(data);
    //
    // if (packet->header.type != PACKET_TYPE_ENTITY_UPDATE) {
    //     printf("[Client] Unknown packet type: %d\n", packet->header.type);
    //     return;
    // }
    //
    // const uint8_t *current_data = packet->data;
    //
    // // entity_id from packet
    // entity_t entity_id;
    // memcpy(&entity_id, current_data, sizeof(entity_t));
    // current_data += sizeof(entity_t);
    // //first component ID
    // uint32_t first_component_id;
    // memcpy(&first_component_id, current_data, sizeof(uint32_t));
    // current_data += sizeof(uint32_t);
    // //Calculate remaining data
    // int remaining_data = len - sizeof(packet_header_t) - sizeof(entity_t) - sizeof(uint32_t);
    //
    // // printf("[Debug] Entity ID: %d, First Component ID: %d, Remaining data: %d bytes\n",
    // //        entity_id, first_component_id, remaining_data);
    //
    // if (first_component_id == COMPONENT_POSITION) {
    //     if (remaining_data >= sizeof(position_t)) {
    //         position_t pos;
    //         memcpy(&pos, current_data, sizeof(position_t));
    //         current_data += sizeof(position_t);
    //         remaining_data -= sizeof(position_t);
    //
    //         printf("[Client] Position received: x=%.2f, y=%.2f\n", pos.x, pos.y);
    //         ecs_add_component(ecs, entity_id, COMPONENT_POSITION, &pos);
    //
    //         // Check if it's the expected position
    //     } else {
    //         printf("[Error] Not enough data for Position\n");
    //         return;
    //     }
    // }
    // if (remaining_data >= sizeof(uint32_t)) {
    //     uint32_t second_component_id;
    //     memcpy(&second_component_id, current_data, sizeof(uint32_t));
    //     current_data += sizeof(uint32_t);
    //     remaining_data -= sizeof(uint32_t);
    //
    //     printf("[Debug] Second Component ID: %d\n", second_component_id);
    //
    //     if (second_component_id == COMPONENT_VELOCITY) {
    //         if (remaining_data >= sizeof(velocity_t)) {
    //             velocity_t vel;
    //             memcpy(&vel, current_data, sizeof(velocity_t));
    //             current_data += sizeof(velocity_t);
    //             remaining_data -= sizeof(velocity_t);
    //
    //             printf("[Client] Velocity received: x=%.2f, y=%.2f\n", vel.x, vel.y);
    //             ecs_add_component(ecs, entity_id, COMPONENT_VELOCITY, &vel);
    //         } else {
    //             printf("[Error] Not enough data for Velocity\n");
    //         }
    //     }
    // }
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    ecs_init(&client_ecs);

    // .ip_address = "141.147.94.67",
    // .tcp_port = 51666,
    // .udp_port = 51666

    network_architecture_config_t client_config = {
        .type = ARCH_CLIENT_SERVER,
        .ip_address = "127.0.0.1",
        .is_server = false,
        .tcp_port = 51660,
        .udp_port = 51660,
        .on_packet_received = on_client_receive_callback,
        .user_data = &client_ecs
    };
    network_architecture_t *client_arch= nullptr;
    network_architecture_init(&client_arch, &client_config, &client_ecs);


    sf::RenderWindow window(sf::VideoMode(800, 600), "Pong ECSNET Client");

    sf::RectangleShape ballShape(sf::Vector2f(20.f, 20.f));
    ballShape.setFillColor(sf::Color::White);
    sf::RectangleShape ballShape2(sf::Vector2f(20.f, 20.f));
    ballShape2.setFillColor(sf::Color::White);

    position_t *ball_pos = nullptr;
    position_t *ball2_pos = nullptr;
    entity_t ball_entity = 0;
    entity_t ball2_entity = 1;


    sf::Clock clock;
    //enviar inputs
    //net update
    //render
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        sf::Event event;
        while (window.pollEvent(event))
            if (event.type == sf::Event::Closed)
                window.close();


        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {

        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
        }
        network_architecture_update(client_arch, dt);

        ball_pos = (position_t *) ecs_get_component(&client_ecs, ball_entity, COMPONENT_POSITION);
        ball2_pos = (position_t *) ecs_get_component(&client_ecs, ball2_entity, COMPONENT_POSITION);
     //   for (entity_t e = 0; e < MAX_ENTITIES; ++e) {
     //       if (ecs_has_component(&client_ecs, e, COMPONENT_POSITION)) {
     //           auto pos = (position_t*)ecs_get_component(&client_ecs, e, COMPONENT_POSITION);
     //            printf("[Client] Entity %d Pos: %.2f, %.2f\n", e, pos->x, pos->y);
     //       }
     //   }
        window.clear(sf::Color::Black);
        if (ball_pos) {
            ballShape.setPosition(ball_pos->x, ball_pos->y);
            window.draw(ballShape);
        } if (ball2_pos) {
            ballShape2.setPosition(ball2_pos->x, ball2_pos->y);
            window.draw(ballShape2);
        }
        window.display();

       sf::sleep(sf::milliseconds(16));
    }

    network_architecture_destroy(client_arch);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
