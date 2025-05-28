#ifndef NET_H
#define NET_H


#ifdef _WIN32
#include <winsock2.h>
typedef SOCKET net_socket;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int net_socket;
#endif


int net_init(); // Init sockets (WSAStartup in Windows)
net_socket net_connect(const char* ip, int port); // Connect to server
int net_send(net_socket sock, const char* msg); // Send message
int net_receive(net_socket sock, char* buffer, int bufsize); // Receive message
void net_close(net_socket sock); // Close socket
void net_cleanup(); // Cleanup (WSACleanup en Windows)

#endif