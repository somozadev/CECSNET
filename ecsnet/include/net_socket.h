#pragma once
#include <stdint.h>

typedef enum
{
    SOCKET_TYPE_TCP,
    SOCKET_TYPE_UDP
} socket_type_t;

typedef struct
{
    socket_type_t type;
    int fd;
} net_socket_t;

//Creates a new net socket with TCP or UDP configuration
net_socket_t net_socket_create(socket_type_t type);
//Connects the net socket to a given ip and port
int net_socket_connect(net_socket_t* socket,  char* ip, uint16_t port);
//Sends data over the socket connection with a given lenght
int net_socket_send(net_socket_t* socket, const void* data, int len);
//Receives a data buffer over the socket connection with a given maximum lenght
int net_socket_receive(net_socket_t* socket, void* buffer, int max_len);
//Closes the net socket connection from it's connected ip and port
int net_socket_close(net_socket_t* socket);
//Cleanup needed for windows platforms after closing a connection
void net_socket_cleanup(void);