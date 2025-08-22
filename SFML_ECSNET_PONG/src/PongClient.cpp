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

// Tamaño de las “gotas”
static constexpr float DROP_WIDTH  = 3.f;
static constexpr float DROP_HEIGHT = 14.f;

ecs_t client_ecs;

void print_entity_table(ecs_t *ecs) {
    printf("\033[2J\033[H");
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
        // maneja otros tipos si hace falta
        return;
    }

    const uint8_t* current = packet->data;
    const uint8_t* end = ((const uint8_t*)packet) + packet->header.size;

    // Leer el número de entidades
    uint16_t entity_count;
    if (end - current < (ptrdiff_t)sizeof(uint16_t)) return;
    memcpy(&entity_count, current, sizeof(uint16_t));
    current += sizeof(uint16_t);

    for (uint16_t eidx = 0; eidx < entity_count; ++eidx) {
        // Leer entity_id
        if (end - current < (ptrdiff_t)sizeof(entity_t)) break;
        entity_t entity_id;
        memcpy(&entity_id, current, sizeof(entity_id));
        current += sizeof(entity_id);

        // Asegurar existencia en ECS local
        ecs_try_create_entity_by_id(ecs, entity_id);

        // Leer número de componentes
        if (end - current < (ptrdiff_t)sizeof(uint8_t)) break;
        uint8_t comp_count;
        memcpy(&comp_count, current, sizeof(uint8_t));
        current += sizeof(uint8_t);

        // Leer componentes
        for (uint8_t c = 0; c < comp_count; ++c) {
            if (end - current < (ptrdiff_t)sizeof(component_t)) break;
            component_t comp_id;
            memcpy(&comp_id, current, sizeof(component_t));
            current += sizeof(component_t);

            size_t comp_size = ecs->components[comp_id].descriptor.size;
            if (end - current < (ptrdiff_t)comp_size) break;

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

    network_architecture_config_t client_config = {
        .type = ARCH_CLIENT_SERVER,
        .ip_address = "127.0.0.1",
        .is_server = false,
        .tcp_port = 51660,
        .udp_port = 51660,
        .on_packet_received = on_client_receive_callback,
        .user_data = &client_ecs
    };
    network_architecture_t *client_arch = nullptr;
    network_architecture_init(&client_arch, &client_config, &client_ecs);

    sf::RenderWindow window(sf::VideoMode(800, 600), "Pong ECSNET Client");
    window.setVerticalSyncEnabled(true);

    // Un shape reutilizable para todas las gotas
    sf::RectangleShape dropShape(sf::Vector2f(DROP_WIDTH, DROP_HEIGHT));
    dropShape.setFillColor(sf::Color(0, 220, 255)); // cian
    // Si prefieres que la posición sea el centro, descomenta:
    // dropShape.setOrigin(DROP_WIDTH * 0.5f, DROP_HEIGHT * 0.5f);

    sf::Clock clock;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();

        // Eventos de ventana
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // (Opcional) inputs al servidor para otra cosa
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
            // enviar INPUT_UP si fuese necesario
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
            // enviar INPUT_DOWN si fuese necesario
        }

        // Red
        network_architecture_update(client_arch, dt);

        // Render
        window.clear(sf::Color::Black);

        // DIBUJAR TODAS LAS ENTIDADES CON POSITION
        for (entity_t e = 0; e < client_ecs.registered_entities_count; ++e) {
            if (!ecs_has_component(&client_ecs, e, COMPONENT_POSITION))
                continue;

            auto* pos = (position_t*)ecs_get_component(&client_ecs, e, COMPONENT_POSITION);
            if (!pos) continue;

            dropShape.setPosition(pos->x, pos->y);
            window.draw(dropShape);
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
