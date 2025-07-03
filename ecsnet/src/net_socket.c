#include "net_socket.h"
#include <ws2def.h>
#include <winsock2.h>
#include "config.h"

net_socket_t net_socket_create(socket_type_t type)
{
    net_socket_t net_socket;
    net_socket.type = type;

    int protocol = (type == SOCKET_TYPE_TCP) ? SOCK_STREAM : SOCK_DGRAM;
    if (PLATFORM_NAME == "Windows")
    {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        net_socket.fd = (int)socket(AF_INET, protocol, 0);
    }
    else
        net_socket.fd = socket(AF_INET, protocol, 0);
}

int net_socket_connect(net_socket_t *net_socket, const char *ip, uint16_t port)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);
    if (PLATFORM_NAME == "Windows")
        return connect((SOCKET)net_socket->fd, (struct sockaddr *)&addr, sizeof(addr));
    else
        return connect(net_socket->fd, (struct sockaddr *)&addr, sizeof(addr));
}

int net_socket_send(net_socket_t *socket, const void* data, int len)
{
    if (PLATFORM_NAME == "Windows")
        return send((SOCKET)socket->fd, data, len, 0);
    else
        return send(socket->fd, data, len, 0);
}

int net_socket_receive(net_socket_t *socket, const void *buffer, int max_len)
{
    if (PLATFORM_NAME == "Windows")
        return recv((SOCKET)socket->fd, buffer, max_len, 0);
    else
        return recv(socket->fd, buffer, max_len, 0);
}

int net_socket_close(net_socket_t *socket)
{
    if (PLATFORM_NAME == "Windows")
        return closesocket((SOCKET)socket->fd);
    else
        return close(socket->fd);
}

void net_socket_cleanup(void)
{
    if (PLATFORM_NAME == "Windows")
        WSACleanup();
}