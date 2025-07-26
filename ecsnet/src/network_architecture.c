#include "network_architecture.h"

#include <stdio.h>
#include <stdlib.h>

#include "protocol_handler.h"

#include <string.h>

#pragma region NET_ARCH_BASE
void ecs_network_architecture_init(void *self)
{
    network_architecture_t* network_architecture = (network_architecture_t *)self;
    if (network_architecture && network_architecture->initialize)
        network_architecture->initialize(network_architecture->implemented_architecture);
}
void ecs_network_architecture_send(void *self)
{
    network_architecture_t* network_architecture = (network_architecture_t *)self;
    if (network_architecture && network_architecture->send_data)
        network_architecture->send_data(network_architecture->implemented_architecture);
}
void ecs_network_architecture_receive(void *self)
{
    network_architecture_t* network_architecture = (network_architecture_t *)self;
    if (network_architecture && network_architecture->receive_data)
        network_architecture->receive_data(network_architecture->implemented_architecture);
}
void ecs_network_architecture_update(void *self)
{
    network_architecture_t* network_architecture = (network_architecture_t *)self;
    if (network_architecture && network_architecture->update)
        network_architecture->update(network_architecture->implemented_architecture);
}
void ecs_network_architecture_destroy(void *self)
{
    network_architecture_t* network_architecture = (network_architecture_t *)self;
    if (network_architecture && network_architecture->destroy)
        network_architecture->destroy(network_architecture->implemented_architecture);
    free(network_architecture);
}

#pragma endregion

#pragma region CLIENT_SERVER

network_architecture_t *create_client_server_architecture()
{
    // static network_architecture_client_server_t client_server = {0}; Not using memory management
    // network_architecture_t network_interface = {
    //     .initialize = client_server_init,
    //     .update = client_server_update,
    //     .send_data = ecs_network_architecture_send,
    //     .receive_data = ecs_network_architecture_receive,
    //     .implemented_architecture = &client_server};
    // return network_interface;

    network_architecture_client_server_t *client_server = malloc(sizeof(network_architecture_client_server_t));
    if (!client_server)
        return NULL;
    memset(client_server, 0, sizeof(*client_server));

    client_server->connection_manager = malloc(sizeof(connection_manager_t));
    if (!client_server->connection_manager)
    {
        free(client_server);
        return NULL;
    }
    connection_manager_init(client_server->connection_manager);
    client_server->server_socket = net_socket_create(SOCKET_TYPE_UDP);
    net_socket_connect(&client_server->server_socket, "12345", 12);
    network_architecture_t *interface = malloc(sizeof(network_architecture_t));
    if (!interface)
    {
        free(client_server->connection_manager);
        free(client_server);
        return NULL;
    }

    interface->initialize = client_server_init;
    interface->update = client_server_update;
    interface->send_data = ecs_network_architecture_send;
    interface->receive_data = ecs_network_architecture_receive;
    interface->destroy = destroy_client_server_architecture;
    interface->implemented_architecture = client_server;

    return interface;
}
void destroy_client_server_architecture(void *self)
{
    network_architecture_client_server_t *client_server = (network_architecture_client_server_t *)self;

    if (!client_server)
        return;

    net_socket_close(&client_server->server_socket);

    if (client_server->connection_manager)
    {
        free(client_server->connection_manager);
    }

    free(client_server);
}

void client_server_init(void *self)
{
    network_architecture_client_server_t *client_server = (network_architecture_client_server_t *)self;
    static connection_manager_t connection_manager;

    connection_manager_init(&connection_manager);
    client_server->connection_manager = &connection_manager;

    client_server->server_socket = net_socket_create(SOCKET_TYPE_UDP);
    // ip n port shall be provided ?
    net_socket_connect(&client_server->server_socket, "12345", 12);
    printf("[ClientServer] Initialized\n");
}

void client_server_update(void *self)
{
    network_architecture_client_server_t *client_server = (network_architecture_client_server_t *)self;
    connection_manager_update(client_server->connection_manager);
    printf("[ClientServer] Update tick\n");
}

#pragma endregion

#pragma region P2P

// network_architecture_t create_peer_to_peer_architecture() Not using memory management
// {
//     static network_architecture_peer_to_peer_t p2p = {0};
//     network_architecture_t network_interface = {
//         .initialize = peer_to_peer_init,
//         .update = peer_to_peer_update,
//         .send_data = ecs_network_architecture_send,
//         .receive_data = ecs_network_architecture_receive,
//         .implemented_architecture = &p2p};
//     return network_interface;
// }

network_architecture_t *create_peer_to_peer_architecture()
{
    network_architecture_peer_to_peer_t *p2p = malloc(sizeof(network_architecture_peer_to_peer_t));
    if (!p2p)
        return NULL;

    memset(p2p, 0, sizeof(*p2p));

    p2p->connection_manager = malloc(sizeof(connection_manager_t));
    if (!p2p->connection_manager)
    {
        free(p2p);
        return NULL;
    }

    connection_manager_init(p2p->connection_manager);

    p2p->server_socket = net_socket_create(SOCKET_TYPE_UDP);
    net_socket_connect(&p2p->server_socket, "12345", 11);

    network_architecture_t *interface = malloc(sizeof(network_architecture_t));
    if (!interface)
    {
        free(p2p->connection_manager);
        free(p2p);
        return NULL;
    }

    interface->initialize = peer_to_peer_init;
    interface->update = peer_to_peer_update;
    interface->send_data = ecs_network_architecture_send;
    interface->receive_data = ecs_network_architecture_receive;
    interface->destroy = destroy_peer_to_peer_architecture;
    interface->implemented_architecture = p2p;

    return interface;
}

void destroy_peer_to_peer_architecture(void *self)
{
    network_architecture_peer_to_peer_t *p2p = (network_architecture_peer_to_peer_t *)self;
    if (!p2p)
        return;

    net_socket_close(&p2p->server_socket);

    if (p2p->connection_manager)
    {
        free(p2p->connection_manager);
    }

    free(p2p);
}

void peer_to_peer_init(void *self)
{
    network_architecture_peer_to_peer_t *p2p = (network_architecture_peer_to_peer_t *)self;
    static connection_manager_t connection_manager;

    connection_manager_init(&connection_manager);
    p2p->connection_manager = &connection_manager;

    p2p->server_socket = net_socket_create(SOCKET_TYPE_UDP);
    // ip n port shall be provided ?
    net_socket_connect(&p2p->server_socket, "12345", 11);
    printf("[PeerToPeer] Initialized\n");
}

void peer_to_peer_update(void *self)
{
    network_architecture_peer_to_peer_t *p2p = (network_architecture_peer_to_peer_t *)self;
    connection_manager_update(p2p->connection_manager);
    printf("[PeerToPeer] Update tick\n");
}

#pragma endregion