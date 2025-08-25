#include <SFML/Graphics.hpp>
#include <iostream>
#include <cmath>
#include <thread>
#include <chrono>

#include "ecs_internal.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

extern "C" {
#include "ecsnet.h"
#include "ecs_types.h"
#include "ecs_builtin.h"
#include "network_architecture.h"
}

// --- Global ECS instances ---
ecs_t server_ecs;
ecs_t client_ecs;

// --- Game constants ---
const float PADDLE_SPEED = 300.0f; // pixels per second
const float BALL_SPEED = 400.0f;
const sf::Vector2f PADDLE_SIZE(20, 100);
const sf::Vector2f BALL_SIZE(20, 20);
const unsigned int WINDOW_WIDTH = 800;
const unsigned int WINDOW_HEIGHT = 600;

// --- Helper functions ---
entity_t create_paddle(ecs_t* ecs, float x, float y) {
    entity_t e = ecs_create_entity(ecs);
    position_t pos = {x, y};
    velocity_t vel = {0, 0};
    ecs_add_component(ecs, e, COMPONENT_POSITION, &pos);
    ecs_add_component(ecs, e, COMPONENT_VELOCITY, &vel);
    return e;
}

entity_t create_ball(ecs_t* ecs, float x, float y, float vx, float vy) {
    entity_t e = ecs_create_entity(ecs);
    position_t pos = {x, y};
    velocity_t vel = {vx, vy};
    ecs_add_component(ecs, e, COMPONENT_POSITION, &pos);
    ecs_add_component(ecs, e, COMPONENT_VELOCITY, &vel);
    return e;
}

// --- Main ---
int main() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

    // --- Initialize ECS ---
    ecs_init(&server_ecs);
    ecs_register_builtin_components(&server_ecs);
    ecs_register_builtin_systems(&server_ecs);

    ecs_init(&client_ecs);
    ecs_register_builtin_components(&client_ecs);
    ecs_register_builtin_systems(&client_ecs);

    // --- Create game entities on server ---
    entity_t paddle1 = create_paddle(&server_ecs, 50, WINDOW_HEIGHT/2 - PADDLE_SIZE.y/2);
    entity_t paddle2 = create_paddle(&server_ecs, WINDOW_WIDTH - 70, WINDOW_HEIGHT/2 - PADDLE_SIZE.y/2);
    entity_t ball = create_ball(&server_ecs, WINDOW_WIDTH/2, WINDOW_HEIGHT/2, BALL_SPEED, BALL_SPEED);

    // --- Initialize network ---
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
    network_cs_t* server_arch = network_cs_init(&server_config, &server_ecs);
    network_cs_t* client_arch = network_cs_init(&client_config, &client_ecs);

    // --- Create SFML window ---
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Pong ECSNET");

    sf::Clock clock;

    while (window.isOpen()) {
        sf::Time dt = clock.restart();
        float delta = dt.asSeconds();

        // --- Handle SFML events ---
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // --- Simple input for paddle1 ---
        velocity_t* vel1 = (velocity_t*)ecs_get_component(&server_ecs, paddle1, COMPONENT_VELOCITY);
        if (vel1) {
            vel1->y = 0;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) vel1->y = -PADDLE_SPEED;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) vel1->y = PADDLE_SPEED;
        }

        // --- Update ECS ---
        ecs_update(&server_ecs, delta);

        // --- Collision: simple top/bottom walls ---
        position_t* ball_pos = (position_t*)ecs_get_component(&server_ecs, ball, COMPONENT_POSITION);
        velocity_t* ball_vel = (velocity_t*)ecs_get_component(&server_ecs, ball, COMPONENT_VELOCITY);
        if (ball_pos && ball_vel) {
            if (ball_pos->y <= 0 || ball_pos->y >= WINDOW_HEIGHT - BALL_SIZE.y) ball_vel->y *= -1;
        }

        // --- Update network ---
        network_cs_update(server_arch);
        network_cs_update(client_arch);

        // --- Render ---
        window.clear(sf::Color::Black);

        // Draw paddles
        position_t* p1_pos = (position_t*)ecs_get_component(&server_ecs, paddle1, COMPONENT_POSITION);
        position_t* p2_pos = (position_t*)ecs_get_component(&server_ecs, paddle2, COMPONENT_POSITION);
        position_t* ball_client_pos = (position_t*)ecs_get_component(&server_ecs, ball, COMPONENT_POSITION);
        if (p1_pos) {
            sf::RectangleShape r1(PADDLE_SIZE);
            r1.setPosition(p1_pos->x, p1_pos->y);
            r1.setFillColor(sf::Color::White);
            window.draw(r1);
        }
        if (p2_pos) {
            sf::RectangleShape r2(PADDLE_SIZE);
            r2.setPosition(p2_pos->x, p2_pos->y);
            r2.setFillColor(sf::Color::White);
            window.draw(r2);
        }

        // Draw ball
        // position_t* ball_client_pos = (position_t*)ecs_get_component(&client_ecs, ball, COMPONENT_POSITION);
        if (ball_client_pos) {
            sf::RectangleShape rball(BALL_SIZE);
            rball.setPosition(ball_client_pos->x, ball_client_pos->y);
            rball.setFillColor(sf::Color::White);
            window.draw(rball);
        }

        window.display();

        // --- Sleep a bit to limit frame rate ---
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    // Cleanup
    network_cs_destroy(server_arch);
    network_cs_destroy(client_arch);

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
