#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>  // For fabs
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

// Include library headers
#include "ecs.h"
#include "net_socket.h"
#include "connection_manager.h"
#include "network_architecture.h"
#include "network_cs.h"
#include "protocol_handler.h"
#include "ecs_types.h"
#include "ecs_internal.h"
#include "ecs_builtin.h"

// --- Test Variables and Callbacks ---

static bool g_client_received_data_flag = false;

// Test callback for data reception on the client.
void on_client_receive_callback(void* user_data, peer_t* peer, const void* data, int len) {
    printf("[Client] Received packet from server. Size: %d\n", len);

    const network_packet_t* packet = (const network_packet_t*)data;

    if (packet->header.type == PACKET_TYPE_ENTITY_UPDATE) {
        printf("[Client] Received entity update\n");

        const uint8_t* current_data = packet->data;

        // Extract entity_id from packet
        entity_t entity_id;
        memcpy(&entity_id, current_data, sizeof(entity_t));
        current_data += sizeof(entity_t);

        // The next field appears to be the first component ID, not a bitmask
        uint32_t first_component_id;
        memcpy(&first_component_id, current_data, sizeof(uint32_t));
        current_data += sizeof(uint32_t);

        // Calculate remaining data
        int remaining_data = len - sizeof(packet_header_t) - sizeof(entity_t) - sizeof(uint32_t);

        printf("[Debug] Entity ID: %d, First Component ID: %d, Remaining data: %d bytes\n",
               entity_id, first_component_id, remaining_data);

        // Show raw data for debugging
        printf("[Debug] Raw component data (%d bytes):\n", remaining_data);
        for (int i = 0; i < remaining_data && i < 32; i++) {
            printf("%02X ", current_data[i]);
            if ((i + 1) % 8 == 0) printf("\n");
        }
        printf("\n");

        // Process the first component
        if (first_component_id == COMPONENT_POSITION) {
            if (remaining_data >= sizeof(position_t)) {
                position_t pos;
                memcpy(&pos, current_data, sizeof(position_t));
                current_data += sizeof(position_t);
                remaining_data -= sizeof(position_t);

                printf("[Client] Position received: x=%.2f, y=%.2f\n", pos.x, pos.y);

                // Check if it's the expected position
                if (fabs(pos.x - 220.0f) < 0.1f && fabs(pos.y - 20.0f) < 0.1f) {
                    printf("[Client] ✅ Correct position received\n");
                    g_client_received_data_flag = true;
                } else {
                    printf("[Client] ⚠️ Position received but unexpected values\n");
                }
            } else {
                printf("[Error] Not enough data for Position\n");
                return;
            }
        }

        // Check if there's a second component
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

                    // Check if it's the expected velocity
                    if (fabs(vel.x - 5.0f) < 0.1f && fabs(vel.y - 3.0f) < 0.1f) {
                        printf("[Client] ✅ Correct velocity received\n");
                    } else {
                        printf("[Client] ⚠️ Velocity received but unexpected values\n");
                    }
                } else {
                    printf("[Error] Not enough data for Velocity\n");
                }
            }
        }

        if (remaining_data > 0) {
            printf("[Debug] Additional unprocessed data: %d bytes\n", remaining_data);
        }
    } else {
        printf("[Client] Unknown packet type: %d\n", packet->header.type);
    }
}

void on_server_connect_callback(void* user_data, peer_t* peer) {
    connection_manager_t* cm = (connection_manager_t*)user_data; // Cast the pointer if needed
    printf("[Server] Peer %s connected. ✅\n", peer->id);
}

void on_server_disconnect_callback(void* user_data, peer_t* peer) {
    connection_manager_t* cm = (connection_manager_t*)user_data; // Cast the pointer if needed
    printf("[Server] Peer %s disconnected. ❌\n", peer->id);
}

// --- Network Architecture Test Function ---

bool test_networking_architecture() {
    printf("--- Network Architecture Test (Client-Server) ---\n");
    bool success = true;
    g_client_received_data_flag = false;

    // --- SETUP: Initialize Winsock and ECS ---
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

    // Debug: Show component values
    printf("[Debug] COMPONENT_POSITION = %d\n", COMPONENT_POSITION);
    printf("[Debug] COMPONENT_VELOCITY = %d\n", COMPONENT_VELOCITY);
    printf("[Debug] sizeof(position_t) = %zu\n", sizeof(position_t));
    printf("[Debug] sizeof(velocity_t) = %zu\n", sizeof(velocity_t));
    printf("[Debug] sizeof(entity_t) = %zu\n", sizeof(entity_t));
    printf("[Debug] sizeof(packet_header_t) = %zu\n", sizeof(packet_header_t));

    // Create an entity with Position and Velocity on the server
    position_t test_pos = {220.0f, 20.0f};
    velocity_t test_vel = {5.0f, 3.0f};
    entity_t entity_to_sync = ecs_create_entity(&server_ecs);
    ecs_add_component(&server_ecs, entity_to_sync, COMPONENT_POSITION, &test_pos);
    ecs_add_component(&server_ecs, entity_to_sync, COMPONENT_VELOCITY, &test_vel);
    printf("Entity %d created with PositionComponent and VelocityComponent on the server.\n", entity_to_sync);

    // --- Test 1: Client-Server Connection ---
    printf("\n--- Test 1: Connection and Peer Detection ---\n");
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

    assert(server_arch != NULL && client_arch != NULL && "Failed to initialize network architecture.");

    // Assign callbacks
    server_arch->connection_manager.on_connect = on_server_connect_callback;
    server_arch->connection_manager.on_disconnect = on_server_disconnect_callback;
    client_arch->connection_manager.on_receive = on_client_receive_callback;

    printf("Client attempting to connect to server...\n");
    connection_manager_connect_to_server(&client_arch->connection_manager, server_config.ip_address, server_config.tcp_port);

    // --- Connection loop ---
    printf("Waiting for connection...\n");
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
    printf("Client successfully connected to server. ✅\n");

    // --- Test 2: Data Reception ---
    printf("\n--- Test 2: Data Reception ---\n");
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

    // Check if client received valid data
    if (g_client_received_data_flag) {
        printf("Client received valid data from server. ✅\n");
    } else {
        printf("Client did not receive valid data from server. ❌\n");
        success = false;
    }

    // --- Test 3: Simulation with ECS Update ---
    printf("\n--- Test 3: Simulation with ECS Update ---\n");

    // Show initial position
    position_t* server_pos = (position_t*)ecs_get_component(&server_ecs, entity_to_sync, COMPONENT_POSITION);
    velocity_t* server_vel = (velocity_t*)ecs_get_component(&server_ecs, entity_to_sync, COMPONENT_VELOCITY);

    if (server_pos && server_vel) {
        printf("[Server] Initial position: x=%.2f, y=%.2f\n", server_pos->x, server_pos->y);
        printf("[Server] Velocity: x=%.2f, y=%.2f\n", server_vel->x, server_vel->y);

        // Execute some simulation frames
            float dt = 0.016f; // ~60 FPS
        for (int frame = 0; frame < 5; frame++) {
            printf("\n[Frame %d]\n", frame + 1);

            // Update server ECS (this will execute the movement system)
            ecs_update(&server_ecs, dt);

            // Show new position
            printf("[Server] New position: x=%.2f, y=%.2f\n", server_pos->x, server_pos->y);

            // Update network (this should synchronize changes)
            network_cs_update(server_arch);
            network_cs_update(client_arch);

            // Small pause
#ifdef _WIN32
            Sleep(50);
#else
            usleep(50000);
#endif
        }

        // Check that the position changed
        if (fabs(server_pos->x - 220.0f) > 0.1f || fabs(server_pos->y - 20.0f) > 0.1f) {
            printf("[Server] ✅ The entity moved correctly with the movement system\n");
            printf("[Server] Final position: x=%.2f, y=%.2f\n", server_pos->x, server_pos->y);
        } else {
            printf("[Server] ⚠️ The entity did not move (possible issue with the movement system)\n");
        }
    }

    // Cleanup
    network_cs_destroy(server_arch);
    network_cs_destroy(client_arch);

#ifdef _WIN32
    WSACleanup();
#endif

    printf("\nNetwork architecture test completed.\n");
    return success;
}