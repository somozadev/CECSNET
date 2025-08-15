#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include "ecs.h"
#include "ecs_types.h"
#include "ecs_internal.h"
#include "protocol_handler.h"
#include "connection_manager.h"
#include "test_net.h"

#include <winsock2.h>
#include <psdk_inc/_wsadata.h>

// Callback de prueba para la recepción de datos
void on_receive_test_callback(peer_t *peer, const void *data, int len) {
    printf("[Test] Received data from peer %s: %s\n", peer->id, (const char *) data);
    assert(strcmp((const char*)data, "Hello from ECS!") == 0);
}

bool test_networking() {

    ecs_t my_ecs;
    ecs_t* ecs = &my_ecs;
    printf("Initializing ECS...\n");
    ecs_init(ecs);

    printf("Initializing NET...\n");
    bool success = true;

    // --- SETUP: Inicializar Winsock ---
    printf("\n--- Setup: Initializing Winsock ---\n");
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // --- TEST 1: Simulated Client-Server Communication ---
    printf("\n--- Test 1: Simulated Client-Server Communication ---\n");

    // Crear un socket de 'servidor' y lo bindeamos a un puerto
    net_socket_t server_socket = net_socket_create(SOCKET_TYPE_UDP);
    assert(server_socket.fd != -1 && "Failed to create server socket.");
    int bind_result = net_socket_bind(&server_socket, "127.0.0.1", 12345);
    assert(bind_result != -1 && "Failed to bind server socket.");
    printf("Server socket created and bound to 127.0.0.1:12345\n");

    // Crear un socket de 'cliente'
    net_socket_t client_socket = net_socket_create(SOCKET_TYPE_UDP);
    assert(client_socket.fd != -1 && "Failed to create client socket.");
    printf("Client socket created.\n");

    const char* test_message = "Hello from ECS!";
    char buffer[1024];

    // Definir la dirección del servidor para que el cliente sepa dónde enviar
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // El cliente envía el mensaje al servidor
    int send_result = net_socket_sendto(&client_socket, test_message, strlen(test_message) + 1, &server_addr);
    assert(send_result > 0 && "Failed to send data from client.");
    printf("Client successfully sent message: '%s'\n", test_message);

    // El servidor recibe el mensaje
    int receive_result = net_socket_receive(&server_socket, buffer, sizeof(buffer));
    assert(receive_result > 0 && "Failed to receive data on server.");
    assert(strcmp(buffer, test_message) == 0 && "Received message is incorrect.");
    printf("Server successfully received message: '%s'\n", buffer);

    // --- TEST 2: Protocol Handler (Empaquetado y Desempaquetado) ---
    printf("\n--- Test 2: Protocol Handler ---\n");

    // NOTA: Para esta prueba, no necesitamos sockets. Se simula el flujo de datos.
    protocol_handler_t handler;
    protocol_handler_init(&handler);

    // Simular datos de una entidad serializada
    entity_t test_entity = 101;
    uint8_t serialized_data[] = {0x01, 0x02, 0x03, 0x04};
    size_t data_size = sizeof(serialized_data);

    // Empaquetar los datos
    int pack_result = protocol_handler_pack_entity_update(&handler, test_entity, serialized_data, data_size);
    assert(pack_result > 0 && "Failed to pack entity update.");
    printf("Successfully packed entity %d into a network packet.\n", test_entity);

    // Simular el procesamiento del paquete recibido
    peer_t dummy_peer;
    strncpy(dummy_peer.id, "dummy_peer", sizeof(dummy_peer.id));
    printf("Simulating reception of the packed packet...\n");
    protocol_handler_process_received_data(
        ecs,
        &dummy_peer,
        &handler.out_packet,
        handler.out_packet.header.size
    );

    // --- CLEANUP ---
    printf("\n--- Cleanup ---\n");
    net_socket_close(&server_socket);
    net_socket_close(&client_socket);
    WSACleanup();
    printf("Cleanup complete.\n");

    return success;
}


// Callback de conexión de prueba
void on_peer_connected_test(peer_t* peer) {
    printf("[Servidor] Peer %s conectado. ✅\n", peer->id);
}

// Callback de desconexión de prueba
void on_peer_disconnected_test(peer_t* peer) {
    printf("[Servidor] Peer %s desconectado. ❌\n", peer->id);
}

