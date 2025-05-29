#pragma once

#include<stdint.h> 

typedef enum {SOCKET_TYPE_TCP, SOCKET_TYPE_UDP} socket_type_t; 

typedef struct { 
    socket_type_t type;
    int fd;
} net_socket_t;

net_socket_t net_socket_create(socket_type_t type);

int net_socket_connect(net_socket_t* socket, const char* ip, uint16_t port);

int net_socket_send(net_socket_t* socket, const void* data, int len);

int net_socket_receive(net_socket_t* socket, void* buffer, int max_len);

int net_socket_close(net_socket_t* socket);

int net_socket_init(void);

void net_socket_cleanup(void);