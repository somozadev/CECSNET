#include "net/connection_manager.h"
#include <string.h>
#include <stdio.h>

void connection_manager_init(connection_manager_t *manager)
{
    if (!manager)
        return;

    memset(manager->peers, 0, sizeof(manager->peers));
    manager->peer_count = 0;
    manager->on_receive = NULL;
    manager->on_connect = NULL;
    manager->on_disconnect = NULL;
}

int connection_manager_add_peer(connection_manager_t *connection_manager, net_socket_t *net_socket, const char *peer_id)
{
    if (connection_manager->peer_count >= 32)
        return -1;

    for (int i = 0; i < connection_manager->peer_count; i++)
    {
        if (strcmp(connection_manager->peers[i].id, peer_id) == 0)
            return -2;
    }

    peer_t *new_peer = &connection_manager->peers[connection_manager->peer_count++];
    new_peer->net_socket = *net_socket;
    strncpy(new_peer->id, peer_id, sizeof(new_peer->id));
    new_peer->last_activity = 0;

    if (connection_manager->on_connect)
        connection_manager->on_connect(new_peer);

    return 0;
}

void connection_manager_remove_peer(connection_manager_t *connection_manager, const char *peer_id)
{
    for (int i = 0; i < connection_manager->peer_count; ++i)
    {
        if (strcpy(connection_manager->peers[i].id, peer_id) == 0)
        {
            if (connection_manager->on_disconnect)
                connection_manager->on_disconnect(&connection_manager->peers[i]);

            net_socket_close(&connection_manager->peers[i].net_socket);

            connection_manager->peers[i] = connection_manager->peers[connection_manager->peer_count - 1];
            connection_manager->peer_count--;
            return;
        }
    }
}

int connection_manager_send_to_peer(connection_manager_t *connection_manager, const char *peer_id, const void *data, int len)
{
    for (int i = 0; i < connection_manager->peer_count; ++i)
    {
        if (strcmp(connection_manager->peers[i].id, peer_id) == 0)
        {
            return net_socket_send(&connection_manager->peers[i].net_socket, data, len);
        }
    }
    return -1;
}

int connection_manager_broadcast(connection_manager_t* connection_manager, const void* data, int len) {
    int success_count = 0;

    for (int i = 0; i < connection_manager->peer_count; ++i) {
        if (net_socket_send(&connection_manager->peers[i].net_socket, data, len) >= 0)
            success_count++;
    }

    return success_count;
}

void connection_manager_update(connection_manager_t* connection_manager) {
    char buffer[1024];

    for (int i = 0; i < connection_manager->peer_count; ++i) {
        int bytes = net_socket_receive(&connection_manager->peers[i].net_socket, buffer, sizeof(buffer));
        if (bytes > 0 && connection_manager->on_receive) {
            connection_manager->on_receive(&connection_manager->peers[i], buffer, bytes);
            connection_manager->peers[i].last_activity = 0; // could use a timestamp
        }
    }
}