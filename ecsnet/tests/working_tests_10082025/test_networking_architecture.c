#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
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
void on_client_receive_callback(peer_t* peer, const void* data, int len) {
    printf("[Cliente] Recibido paquete del servidor. Tamano: %d\n", len);

    // Asume que el paquete es un PositionComponent serializado
    // En un sistema real, un 'header' indicaría el tipo de paquete.
    // Aquí, se hace una simple verificación.
    // Esto es solo para el test, el protocolo real lo manejaría de forma más robusta.
    assert(len == sizeof(position_t) + sizeof(uint32_t) + sizeof(uint8_t) && "Tamano del paquete recibido incorrecto.");

    // Verificación de los datos. Esto es solo una suposición.
    // La data de la entidad se decodificaría aquí.
    uint32_t entity_id;
    uint8_t component_id;
    position_t pos;
    memcpy(&entity_id, data, sizeof(uint32_t));
    memcpy(&component_id, (const uint8_t*)data + sizeof(uint32_t), sizeof(uint8_t));
    memcpy(&pos, (const uint8_t*)data + sizeof(uint32_t) + sizeof(uint8_t), sizeof(position_t));

    assert(component_id == COMPONENT_POSITION && "Component ID incorrecto.");
    assert(pos.x == 10.0f && pos.y == 20.0f && "Datos del componente incorrectos.");

    printf("[Cliente] Datos de posicion verificados: x=%.2f, y=%.2f\n", pos.x, pos.y);
    g_client_received_data_flag = true;
}

// Callback de conexión del servidor (para observar el evento).
void on_server_connect_callback(peer_t* peer) {
    printf("[Servidor] Peer %s conectado. ✅\n", peer->id);
}

// Callback de desconexión del servidor (para observar el evento).
void on_server_disconnect_callback(peer_t* peer) {
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
    ecs_init(&client_ecs);
    ecs_register_builtin_components(&client_ecs);

    // Creamos una entidad con un PositionComponent en el ECS del servidor.
    position_t test_pos = {10.0f, 20.0f};
    entity_t entity_to_sync = ecs_create_entity(&server_ecs);
    ecs_add_component(&server_ecs, entity_to_sync, COMPONENT_POSITION, &test_pos);
    printf("Entidad %d creada con PositionComponent en el servidor.\n", entity_to_sync);

    // --- Test 1: Conexión Cliente-Servidor ---
    printf("\n--- Test 1: Conexión y Detección de Peer ---\n");
    network_architecture_config_t server_config = { .type = ARCH_CLIENT_SERVER, .ip_address = "127.0.0.1", .port = 12345, .is_server = true };
    network_architecture_config_t client_config = { .type = ARCH_CLIENT_SERVER, .ip_address = "127.0.0.1", .port = 12346, .is_server = false };


    network_cs_t* server_arch = network_cs_init(&server_config, &server_ecs);
    network_cs_t* client_arch = network_cs_init(&client_config, &client_ecs);

    assert(server_arch != NULL && client_arch != NULL && "Fallo al inicializar la arquitectura de red.");

    // Los callbacks se asignan a la connection_manager.
    server_arch->connection_manager.on_connect = on_server_connect_callback;
    server_arch->connection_manager.on_disconnect = on_server_disconnect_callback;
    client_arch->connection_manager.on_receive = on_client_receive_callback;

    printf("Cliente intentando conectarse al servidor...\n");
    connection_manager_connect_to_server(&client_arch->connection_manager, server_config.ip_address, server_config.port);

    // --- Bucle de ejecución simulada ---
    printf("Esperando a que el cliente se conecte y el servidor lo detecte...\n");
    int i = 0;
    while (server_arch->connection_manager.peer_count == 0 && i < 200) { // Simula 5 segundos de ejecución (500 * 10ms)
        // Lógica de actualización de la red
        network_cs_update(server_arch);
        network_cs_update(client_arch);

        // Simula la espera
#ifdef _WIN32
        Sleep(10);
#else
        usleep(10000);
#endif
        i++;
    }

    assert(server_arch->connection_manager.peer_count > 0 && "Server has no connected peers");
    i = 0;
    while (i < 200 && !g_client_received_data_flag) { // Simula 5 segundos de ejecución (500 * 10ms)
        network_cs_update(server_arch);
        network_cs_update(client_arch);

        // Simula la espera
#ifdef _WIN32
        Sleep(10);
#else
        usleep(10000);
#endif
        i++;
    }


    // Verificar si el cliente recibió los datos
    assert(g_client_received_data_flag && "El cliente no recibió los datos del servidor. ❌");
    if (g_client_received_data_flag) {
        printf("El cliente recibió los datos correctamente. ✅\n");
    }

    // Limpieza
    network_cs_destroy(server_arch);
    network_cs_destroy(client_arch);

#ifdef _WIN32
    WSACleanup();
#endif

    printf("\nTodos los tests de la arquitectura de red completados con exito.\n");
    return success;
}