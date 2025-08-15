#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>  // Para fabs
#include "test_networking_architecture.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

// Incluye las cabeceras de tus librerías
#include "ecs.h"
#include "net_socket.h"
#include "connection_manager.h"
#include "network_architecture.h"
#include "network_cs.h"
#include "protocol_handler.h"
#include "ecs_types.h"
#include "ecs_internal.h"
#include "ecs_builtin.h"

// --- Variables y Callbacks de Test ---

static bool g_client_received_data_flag = false;

// Callback de prueba para la recepción de datos en el cliente.
void on_client_receive_callback(void* user_data, peer_t* peer, const void* data, int len) {
    printf("[Cliente] Recibido paquete del servidor. Tamano: %d\n", len);

    const network_packet_t* packet = (const network_packet_t*)data;

    if (packet->header.type == PACKET_TYPE_ENTITY_UPDATE) {
        printf("[Cliente] Recibida actualización de entidad\n");

        const uint8_t* current_data = packet->data;

        // Extraer entity_id del paquete
        entity_t entity_id;
        memcpy(&entity_id, current_data, sizeof(entity_t));
        current_data += sizeof(entity_t);

        // El siguiente campo parece ser el primer component ID, no un bitmask
        uint32_t first_component_id;
        memcpy(&first_component_id, current_data, sizeof(uint32_t));
        current_data += sizeof(uint32_t);

        // Calcular datos restantes
        int remaining_data = len - sizeof(packet_header_t) - sizeof(entity_t) - sizeof(uint32_t);

        printf("[Debug] Entity ID: %d, First Component ID: %d, Datos restantes: %d bytes\n",
               entity_id, first_component_id, remaining_data);

        // Mostrar los datos raw para debugging
        printf("[Debug] Datos raw de componentes (%d bytes):\n", remaining_data);
        for (int i = 0; i < remaining_data && i < 32; i++) {
            printf("%02X ", current_data[i]);
            if ((i + 1) % 8 == 0) printf("\n");
        }
        printf("\n");

        // Procesar el primer componente
        if (first_component_id == COMPONENT_POSITION) {
            if (remaining_data >= sizeof(position_t)) {
                position_t pos;
                memcpy(&pos, current_data, sizeof(position_t));
                current_data += sizeof(position_t);
                remaining_data -= sizeof(position_t);

                printf("[Cliente] Position recibida: x=%.2f, y=%.2f\n", pos.x, pos.y);

                // Verificar si es la posición esperada
                if (fabs(pos.x - 220.0f) < 0.1f && fabs(pos.y - 20.0f) < 0.1f) {
                    printf("[Cliente] ✅ Position correcta recibida\n");
                    g_client_received_data_flag = true;
                } else {
                    printf("[Cliente] ⚠️ Position recibida pero valores inesperados\n");
                }
            } else {
                printf("[Error] No hay suficientes datos para Position\n");
                return;
            }
        }

        // Verificar si hay un segundo componente
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

                    printf("[Cliente] Velocity recibida: x=%.2f, y=%.2f\n", vel.x, vel.y);

                    // Verificar si es la velocidad esperada
                    if (fabs(vel.x - 5.0f) < 0.1f && fabs(vel.y - 3.0f) < 0.1f) {
                        printf("[Cliente] ✅ Velocity correcta recibida\n");
                    } else {
                        printf("[Cliente] ⚠️ Velocity recibida pero valores inesperados\n");
                    }
                } else {
                    printf("[Error] No hay suficientes datos para Velocity\n");
                }
            }
        }

        if (remaining_data > 0) {
            printf("[Debug] Datos adicionales no procesados: %d bytes\n", remaining_data);
        }
    } else {
        printf("[Cliente] Tipo de paquete desconocido: %d\n", packet->header.type);
    }
}

void on_server_connect_callback(void* user_data, peer_t* peer) {
    connection_manager_t* cm = (connection_manager_t*)user_data; // Se castea el puntero si es necesario
    printf("[Servidor] Peer %s conectado. ✅\n", peer->id);
}

void on_server_disconnect_callback(void* user_data, peer_t* peer) {
    connection_manager_t* cm = (connection_manager_t*)user_data; // Se castea el puntero si es necesario
    printf("[Servidor] Peer %s desconectado. ❌\n", peer->id);
}

// --- Función de Test de la Arquitectura de Red ---

bool test_networking_architecture() {
    printf("--- Test de Arquitectura de Red (Cliente-Servidor) ---\n");
    bool success = true;
    g_client_received_data_flag = false;

    // --- SETUP: Inicializar Winsock y ECS ---
    printf("\n--- Setup ---\n");
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    ecs_t server_ecs, client_ecs;
    ecs_init(&server_ecs);
    ecs_register_builtin_components(&server_ecs);
    ecs_register_builtin_systems(&server_ecs);
    ecs_init(&client_ecs);
    ecs_register_builtin_components(&client_ecs);
    ecs_register_builtin_systems(&client_ecs);

    // Debug: Mostrar los valores de los componentes
    printf("[Debug] COMPONENT_POSITION = %d\n", COMPONENT_POSITION);
    printf("[Debug] COMPONENT_VELOCITY = %d\n", COMPONENT_VELOCITY);
    printf("[Debug] sizeof(position_t) = %zu\n", sizeof(position_t));
    printf("[Debug] sizeof(velocity_t) = %zu\n", sizeof(velocity_t));
    printf("[Debug] sizeof(entity_t) = %zu\n", sizeof(entity_t));
    printf("[Debug] sizeof(packet_header_t) = %zu\n", sizeof(packet_header_t));

    // Creamos una entidad con Position y Velocity en el servidor
    position_t test_pos = {220.0f, 20.0f};
    velocity_t test_vel = {5.0f, 3.0f};
    entity_t entity_to_sync = ecs_create_entity(&server_ecs);
    ecs_add_component(&server_ecs, entity_to_sync, COMPONENT_POSITION, &test_pos);
    ecs_add_component(&server_ecs, entity_to_sync, COMPONENT_VELOCITY, &test_vel);
    printf("Entidad %d creada con PositionComponent y VelocityComponent en el servidor.\n", entity_to_sync);

    // --- Test 1: Conexión Cliente-Servidor ---
    printf("\n--- Test 1: Conexión y Detección de Peer ---\n");
    network_architecture_config_t server_config = {
        .type = ARCH_CLIENT_SERVER,
        .ip_address = "127.0.0.1",
        .tcp_port = 12345,
        .udp_port = 12345,
        .is_server = true
    };
    network_architecture_config_t client_config = {
        .type = ARCH_CLIENT_SERVER,
        .ip_address = "127.0.0.1",
        .tcp_port = 12346,
        .udp_port = 12346,
        .is_server = false
    };

    network_cs_t* server_arch = network_cs_init(&server_config, &server_ecs);
    network_cs_t* client_arch = network_cs_init(&client_config, &client_ecs);

    assert(server_arch != NULL && client_arch != NULL && "Fallo al inicializar la arquitectura de red.");

    // Asignar callbacks
    server_arch->connection_manager.on_connect = on_server_connect_callback;
    server_arch->connection_manager.on_disconnect = on_server_disconnect_callback;
    client_arch->connection_manager.on_receive = on_client_receive_callback;

    printf("Cliente intentando conectarse al servidor...\n");
    connection_manager_connect_to_server(&client_arch->connection_manager, server_config.ip_address, server_config.tcp_port);

    // --- Bucle de conexión ---
    printf("Esperando conexión...\n");
    int i = 0;
    while (server_arch->connection_manager.peer_count == 0 && i < 100) {
        network_cs_update(server_arch);
        network_cs_update(client_arch);
#ifdef _WIN32
        Sleep(10);
#else
        usleep(10000);
#endif
        i++;
    }

    assert(server_arch->connection_manager.peer_count > 0 && "Server has no connected peers");
    printf("Cliente conectado exitosamente al servidor. ✅\n");

    // --- Test 2: Recepción de datos ---
    printf("\n--- Test 2: Recepción de Datos ---\n");
    i = 0;
    while (i < 50 && !g_client_received_data_flag) {
        network_cs_update(server_arch);
        network_cs_update(client_arch);
#ifdef _WIN32
        Sleep(10);
#else
        usleep(10000);
#endif
        i++;
    }

    // Verificar si el cliente recibió datos válidos
    if (g_client_received_data_flag) {
        printf("El cliente recibió datos válidos del servidor. ✅\n");
    } else {
        printf("El cliente no recibió datos válidos del servidor. ❌\n");
        success = false;
    }

    // --- Test 3: Simulación con ECS Update ---
    printf("\n--- Test 3: Simulación con ECS Update ---\n");

    // Mostrar posición inicial
    position_t* server_pos = (position_t*)ecs_get_component(&server_ecs, entity_to_sync, COMPONENT_POSITION);
    velocity_t* server_vel = (velocity_t*)ecs_get_component(&server_ecs, entity_to_sync, COMPONENT_VELOCITY);

    if (server_pos && server_vel) {
        printf("[Servidor] Posición inicial: x=%.2f, y=%.2f\n", server_pos->x, server_pos->y);
        printf("[Servidor] Velocidad: x=%.2f, y=%.2f\n", server_vel->x, server_vel->y);

        // Ejecutar algunos frames de simulación
        float dt = 0.016f; // ~60 FPS
        for (int frame = 0; frame < 5; frame++) {
            printf("\n[Frame %d]\n", frame + 1);

            // Actualizar ECS del servidor (esto ejecutará el sistema de movimiento)
            ecs_update(&server_ecs, dt);

            // Mostrar nueva posición
            printf("[Servidor] Nueva posición: x=%.2f, y=%.2f\n", server_pos->x, server_pos->y);

            // Actualizar red (esto debería sincronizar los cambios)
            network_cs_update(server_arch);
            network_cs_update(client_arch);

            // Pequeña pausa
#ifdef _WIN32
            Sleep(50);
#else
            usleep(50000);
#endif
        }

        // Verificar que la posición cambió
        if (fabs(server_pos->x - 220.0f) > 0.1f || fabs(server_pos->y - 20.0f) > 0.1f) {
            printf("[Servidor] ✅ La entidad se movió correctamente con el sistema de movimiento\n");
            printf("[Servidor] Posición final: x=%.2f, y=%.2f\n", server_pos->x, server_pos->y);
        } else {
            printf("[Servidor] ⚠️ La entidad no se movió (posible problema con el sistema de movimiento)\n");
        }
    }

    // Limpieza
    network_cs_destroy(server_arch);
    network_cs_destroy(client_arch);

#ifdef _WIN32
    WSACleanup();
#endif

    printf("\nTest de arquitectura de red completado.\n");
    return success;
}