#include <net/net_socket.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
// #pragma comment(lib, "sw2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#endif

int net_socket_init()
{
#ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData);
#else
    return 0;
#endif;
}

void net_sockets_cleanup(void)
{
#ifdef _WIN32
    WSACleanup();
#endif;
}

net_socket_t net_socket_create(socket_type_t type)
{
    net_socket_t net_socket;
    net_socket.type = type;

    int protocol = (type == SOCKET_TYPE_TCP) ? SOCK_STREAM : SOCK_DGRAM;

#ifdef _WIN32
    net_socket.fd = (int)socket(AF_INET, protocol, 0);
#else
    net_socket.fd = socket(AF_INET, protocol, 0);
#endif

    return net_socket;
}

net_socket_connect(net_socket_t *net_socket, const char *ip, uint16_t port)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);
#ifdef _WIN32
    return connect((SOCKET)net_socket->fd, (struct sockaddr *)&addr, sizeof(addr));
#else
    return connect(net_socket->fd, (struct sockaddr *)&addr, sizeof(addr));
#endif
}

int net_socket_send(net_socket_t *net_socket, const void *data, int len)
{
#ifdef _WIN32
    return send((SOCKET)net_socket->fd, data, len, 0);
#else
    return send(net_socket->fd, data, len, 0);
#endif
}

int net_socket_receive(net_socket_t *net_socket, void *buffer, int max_len)
{
#ifdef _WIN32
    return recv((SOCKET)net_socket->fd, buffer, max_len, 0);
#else
    return recv(net_socket->fd, buffer, max_len, 0);
#endif
}
int net_socket_close(net_socket_t *net_socket)
{
#ifdef _WIN32
    return closesocket((SOCKET)net_socket->fd);
#else
    return close(net_socket->fd);
#endif
}
