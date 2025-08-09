#include "net_socket.h"
#include <winsock2.h>
#include <windows.h>
#include <ws2def.h>
#include "config.h"
#include <fcntl.h>

net_socket_t net_socket_create(socket_type_t type) {
    net_socket_t net_socket;
    net_socket.type = type;

    int protocol = (type == SOCKET_TYPE_TCP) ? SOCK_STREAM : SOCK_DGRAM;
#ifdef _WIN32
    net_socket.fd = (int) socket(AF_INET, protocol, 0);
#else
        net_socket.fd = socket(AF_INET, protocol, 0);
#endif
    net_socket_set_non_blocking(&net_socket);
    return net_socket;
}

int net_socket_set_non_blocking(net_socket_t *socket) {
    if (!socket) return -1;

#ifdef _WIN32
    u_long mode = 1; // 1 to enable non-blocking, 0 to disable
    return ioctlsocket((SOCKET) socket->fd, FIONBIO, &mode);
#else
    int flags = fcntl(socket->fd, F_GETFL, 0);
    if (flags == -1) return -1;
    flags |= O_NONBLOCK;
    return fcntl(socket->fd, F_SETFL, flags);
#endif
}

int net_socket_connect(net_socket_t *net_socket, char *ip, uint16_t port) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

#ifdef _WIN32
    return connect((SOCKET) net_socket->fd, (struct sockaddr *) &addr, sizeof(addr));
#else
    inet_pton(AF_INET, ip, &addr.sin_addr);
    return connect(net_socket->fd, (struct sockaddr *) &addr, sizeof(addr));
#endif
}

int net_socket_send(net_socket_t *socket, const void *data, int len) {
#ifdef _WIN32
    return send((SOCKET) socket->fd, data, len, 0);
#else
    return send(socket->fd, data, len, 0);
#endif
}

int net_socket_sendto(net_socket_t* socket, const void* data, int len, const struct sockaddr_in* addr)
{
    if (!socket || !data || !addr) return -1;

#ifdef _WIN32
    return sendto((SOCKET)socket->fd, (const char*)data, len, 0, (const struct sockaddr*)addr, sizeof(struct sockaddr_in));
#else
    return sendto(socket->fd, data, len, 0, (const struct sockaddr*)addr, sizeof(struct sockaddr_in));
#endif
}

int net_socket_receive(net_socket_t *socket, void *buffer, int max_len) {
#ifdef _WIN32
    return recv((SOCKET) socket->fd, buffer, max_len, 0);
#else
    return recv(socket->fd, buffer, max_len, 0);
#endif
}

int net_socket_bind(net_socket_t* socket, const char* ip, uint16_t port)
{
    if (!socket) return -1;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

#ifdef _WIN32
    addr.sin_addr.s_addr = inet_addr(ip);   // inet_addr is a quick way to convert IP string to binary for IPv4
#else
    inet_pton(AF_INET, ip, &addr.sin_addr);
#endif

    if (addr.sin_addr.s_addr == INADDR_NONE) {
        perror("Invalid IP address");
        return -1;
    }

    if (PLATFORM_NAME == "Windows")
        return bind((SOCKET)socket->fd, (struct sockaddr*)&addr, sizeof(addr));
    else
        return bind(socket->fd, (struct sockaddr*)&addr, sizeof(addr));
}
int net_socket_close(net_socket_t *socket) {
#ifdef _WIN32
    return closesocket((SOCKET) socket->fd);
#else
    return close(socket->fd);
#endif
}

void net_socket_cleanup(void) {
    if (PLATFORM_NAME == "Windows")
        WSACleanup();
}
