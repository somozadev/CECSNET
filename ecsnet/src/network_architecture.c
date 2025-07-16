#include "network_architecture.h"
#include "protocol_handler.h"

#include <string.h>

#pragma region NET_ARCH_BASE
void ecs_network_architecture_init(network_architecture_t *network_architecture)
{
    if(network_architecture && network_architecture->initialize)
        network_architecture->initialize(network_architecture->implemented_architecture);
}
void ecs_network_architecture_send(network_architecture_t *network_architecture)
{
    if(network_architecture && network_architecture->send_data)
        network_architecture->send_data(network_architecture->implemented_architecture);
}
void ecs_network_architecture_receive(network_architecture_t *network_architecture)
{
    if(network_architecture && network_architecture->receive_data)
        network_architecture->receive_data(network_architecture->implemented_architecture);
}
void ecs_network_architecture_update(network_architecture_t *network_architecture)
{
    if(network_architecture && network_architecture->update)
        network_architecture->update(network_architecture->implemented_architecture);
}
#pragma endregion

#pragma region CLIENT_SERVER

network_architecture_t create_client_server_architecture()
{
    static network_architecture_client_server_t client_server = {0};
    
    network_architecture_t network_interface = { 
        .initialize = client_server_init,
        .update = client_server_update,
        .send_data= ecs_network_architecture_send,
        .receive_data= ecs_network_architecture_receive,
        .implemented_architecture = &client_server
    };
    return network_interface;
}
void client_server_init(void* self) {
    network_architecture_client_server_t* client_server = (network_architecture_client_server_t*)self;
    static connection_manager_t connection_manager;

    connection_manager_init(&connection_manager);
    client_server->connection_manager = &connection_manager;

    client_server->server_socket = net_socket_create(SOCKET_TYPE_UDP);
    //ip n port shall be provided ?
    net_socket_connect(&client_server->server_socket, 12345, 12);
    printf("[ClientServer] Initialized\n");
}

void client_server_update(void* self) {
    network_architecture_client_server_t* client_server = (network_architecture_client_server_t*)self;
    connection_manager_update(client_server->connection_manager);
    printf("[ClientServer] Update tick\n");
}

#pragma endregion

#pragma region P2P

network_architecture_t create_peer_to_peer_architecture() {
    static network_architecture_peer_to_peer_t p2p = {0};
    network_architecture_t network_interface = {
        .initialize = peer_to_peer_init,
        .update = peer_to_peer_update,
        .send_data = ecs_network_architecture_send,
        .receive_data = ecs_network_architecture_receive,
        .implemented_architecture = &p2p
    };
    return network_interface;
}

void peer_to_peer_init(void* self) {
    network_architecture_peer_to_peer_t* p2p = (network_architecture_peer_to_peer_t*)self;
    static connection_manager_t connection_manager;

    connection_manager_init(&connection_manager);
    p2p->connection_manager = &connection_manager;

    p2p->server_socket = net_socket_create(SOCKET_TYPE_UDP);
    //ip n port shall be provided ?
    net_socket_connect(&p2p->server_socket, 12345, 11);
    printf("[PeerToPeer] Initialized\n");
}

void peer_to_peer_update(void* self) {
    network_architecture_peer_to_peer_t* p2p = (network_architecture_peer_to_peer_t*)self;
    connection_manager_update(p2p->connection_manager);
    printf("[PeerToPeer] Update tick\n");
}


#pragma endregion