#pragma once
#include <stdint.h>
#include <stdbool.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

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
// Makes the socket non-blocking
int net_socket_set_non_blocking(net_socket_t* socket);
//Connects the net socket to a given ip and port
int net_socket_connect(net_socket_t* socket,  char* ip, uint16_t port);
//Sends data over the socket connection with a given lenght
int net_socket_send(net_socket_t* socket, const void* data, int len);
// Sends data to a specific address (for connectionless sockets like UDP)
int net_socket_sendto(net_socket_t* socket, const void* data, int len, const struct sockaddr_in* addr);
//Receives a data buffer over the socket connection with a given maximum lenght
int net_socket_receive(net_socket_t* socket, void* buffer, int max_len);
// Binds the socket to a given port and IP (for server-side sockets)
int net_socket_bind(net_socket_t* socket, const char* ip, uint16_t port);
//Closes the net socket connection from it's connected ip and port
int net_socket_close(net_socket_t* socket);
//Cleanup needed for windows platforms after closing a connection
void net_socket_cleanup(void);