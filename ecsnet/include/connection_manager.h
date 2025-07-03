#pragma once
#include "net_socket.h"

#define MAX_PEERS 16

typedef struct {
    net_socket_t net_socket; 
    char id[64];
} peer_t;

typedef struct { 
    peer_t peers[MAX_PEERS]; 
    int peer_count; 
    
    void (*on_receive)(peer_t*, const void*, int);
    void (*on_connect)(peer_t*);
    void (*on_disconnect)(peer_t*);
} connection_manager_t; 


// Initializes the connection manager and clears all peers
void connection_manager_init(connection_manager_t* connection_manager);
// Adds a new peer to the manager. -1 = max peers connected. -2 = peer already connected. 0 = success.
int connection_manager_add_peer(connection_manager_t* connection_manager, peer_t* peer);
//  Removes a given peer by ID from the manager
void connection_manager_remove_peer(connection_manager_t* connection_manager, const char*peer_id); 
// Sends data to a specific peer by ID
int connection_manager_send_to_peer(connection_manager_t* connection_manager, const char* peer_id, const void* data, int len);
// Sends data to all peers 
int connection_manager_broadcast(connection_manager_t* connection_manager, const void* data, int len);
//Update the manager (polling/receiving)
void connection_manager_update(connection_manager_t* connection_manager);