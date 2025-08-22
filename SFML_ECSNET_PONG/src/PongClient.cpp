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
    ecs_t* ecs = (ecs_t*)user_data;
    const network_packet_t* packet = (const network_packet_t*)data;

    if (packet->header.type != PACKET_TYPE_MULTI_ENTITY_UPDATE) {
        // maneja otros tipos
        return;
    }

    const uint8_t* current = packet->data;
    const uint8_t* end = ((const uint8_t*)packet) + packet->header.size;

    // Leer el número de entidades
    uint16_t entity_count;
    if (end - current < sizeof(uint16_t)) return;
    memcpy(&entity_count, current, sizeof(uint16_t));
    current += sizeof(uint16_t);

    for (uint16_t eidx = 0; eidx < entity_count; ++eidx) {
        // Leer entity_id
        if (end - current < sizeof(entity_t)) break;
        entity_t entity_id;
        memcpy(&entity_id, current, sizeof(entity_id));
        current += sizeof(entity_id);

        // Asegurar la existencia de la entidad en el ECS local
        ecs_try_create_entity_by_id(ecs, entity_id);

        // Leer número de componentes
        if (end - current < sizeof(uint8_t)) break;
        uint8_t comp_count;
        memcpy(&comp_count, current, sizeof(uint8_t));
        current += sizeof(uint8_t);

        // Leer los componentes
        for (uint8_t c = 0; c < comp_count; ++c) {
            if (end - current < sizeof(component_t)) break;
            component_t comp_id;
            memcpy(&comp_id, current, sizeof(component_t));
            current += sizeof(component_t);

            size_t comp_size = ecs->components[comp_id].descriptor.size;
            if (end - current < comp_size) break;

            // Copiar datos al componente local
            ecs_add_component(ecs, entity_id, comp_id, (void*)current);

            current += comp_size;
        }
    }
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
