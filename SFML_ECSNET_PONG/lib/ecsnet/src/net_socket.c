#include "net_socket.h"
#include <winsock2.h>
#include <windows.h>
#include <ws2def.h>
#include "config.h"
#include <fcntl.h>
#include <stdio.h>

/**
 * @brief Creates a new network socket.
 *
 * This function initializes a `net_socket_t` structure and creates a new
 * socket file descriptor with the specified protocol (TCP or UDP). It also
 * sets the `SO_REUSEADDR` option and makes the socket non-blocking.
 *
 * @param type The type of socket to create (TCP or UDP).
 * @return The newly created `net_socket_t` instance.
 */
net_socket_t net_socket_create(socket_type_t type) {
    net_socket_t net_socket;
    net_socket.fd = -1;
    net_socket.type = type;

    // Determine the protocol based on the socket type.
    int protocol = (type == SOCKET_TYPE_TCP) ? SOCK_STREAM : SOCK_DGRAM;
#ifdef _WIN32
    // Windows-specific socket creation.
    net_socket.fd = (int) socket(AF_INET, protocol, 0);
#else
    // POSIX-compliant socket creation.
    net_socket.fd = socket(AF_INET, protocol, 0);
#endif
    // Set the SO_REUSEADDR option to allow the socket to be bound to a port
    // that is still in a TIME_WAIT state.
    int optval = 1;
    if (setsockopt(net_socket.fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    // Make the socket non-blocking immediately after creation.
    net_socket_set_non_blocking(&net_socket);
    return net_socket;
}

/**
 * @brief Makes the socket non-blocking.
 *
 * This function changes the socket's mode to non-blocking using platform-specific
 * calls.
 *
 * @param socket A pointer to the `net_socket_t` instance.
 * @return 0 on success, or a negative value on failure.
 */
int net_socket_set_non_blocking(net_socket_t *socket) {
    if (!socket) return -1;

#ifdef _WIN32
    u_long mode = 1; // 1 to enable non-blocking, 0 to disable.
    return ioctlsocket((SOCKET) socket->fd, FIONBIO, &mode);
#else
    int flags = fcntl(socket->fd, F_GETFL, 0);
    if (flags == -1) return -1;
    flags |= O_NONBLOCK;
    return fcntl(socket->fd, F_SETFL, flags);
#endif
}

/**
 * @brief Connects the socket to a given IP and port.
 *
 * This function attempts to establish a connection to a remote host. On
 * non-blocking sockets, this call may return immediately with a "connection in
 * progress" error, which is expected.
 *
 * @param net_socket A pointer to the `net_socket_t` instance.
 * @param ip The IP address to connect to.
 * @param port The port to connect to.
 * @return 0 on success, or a negative value on failure.
 */
int net_socket_connect(net_socket_t *net_socket, char *ip, uint16_t port) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    // Convert the IP address from string to binary format.
    inet_pton(AF_INET, ip, &addr.sin_addr);

#ifdef _WIN32
    return connect((SOCKET) net_socket->fd, (struct sockaddr *) &addr, sizeof(addr));
#else
    return connect(net_socket->fd, (struct sockaddr *) &addr, sizeof(addr));
#endif
}

/**
 * @brief Sends data over the socket connection.
 *
 * This function sends a specified number of bytes over the connected socket.
 * It's generally used for stream-oriented sockets like TCP.
 *
 * @param socket A pointer to the `net_socket_t` instance.
 * @param data A pointer to the data buffer to send.
 * @param len The length of the data to send.
 * @return The number of bytes sent, or a negative value on failure.
 */
int net_socket_send(net_socket_t *socket, const void *data, int len) {
#ifdef _WIN32
    return send((SOCKET) socket->fd, (const char *)data, len, 0);
#else
    return send(socket->fd, data, len, 0);
#endif
}

/**
 * @brief Sends data to a specific address (for connectionless sockets like UDP).
 *
 * This function is used to send datagrams to a specified address, bypassing the
 * need for a prior connection.
 *
 * @param socket A pointer to the `net_socket_t` instance.
 * @param data A pointer to the data buffer to send.
 * @param len The length of the data to send.
 * @param addr A pointer to the destination address.
 * @return The number of bytes sent, or a negative value on failure.
 */
int net_socket_sendto(net_socket_t *socket, const void *data, int len, const struct sockaddr_in *addr) {
    if (!socket || !data || !addr) return -1;

#ifdef _WIN32
    return sendto((SOCKET) socket->fd, (const char *) data, len, 0, (const struct sockaddr *) addr, sizeof(struct sockaddr_in));
#else
    return sendto(socket->fd, data, len, 0, (const struct sockaddr*)addr, sizeof(struct sockaddr_in));
#endif
}

/**
 * @brief Receives a data buffer over the socket connection.
 *
 * This function reads data from a connected socket into a buffer.
 *
 * @param socket A pointer to the `net_socket_t` instance.
 * @param buffer A pointer to the buffer to store received data.
 * @param max_len The maximum length of the buffer.
 * @return The number of bytes received, or a negative value on failure.
 */
int net_socket_receive(net_socket_t *socket, void *buffer, int max_len) {
#ifdef _WIN32
    return recv((SOCKET) socket->fd, (char *)buffer, max_len, 0);
#else
    return recv(socket->fd, buffer, max_len, 0);
#endif
}

/**
 * @brief Receives a data buffer over a socket and returns the sender address (for UDP).
 *
 * This function is used for datagram-oriented sockets to receive data and also
 * identify the sender's address.
 *
 * @param socket A pointer to the `net_socket_t` instance.
 * @param buffer A pointer to the buffer to store received data.
 * @param max_len The maximum length of the buffer.
 * @param sender_addr A pointer to a `sockaddr_in` struct to store the sender's address.
 * @return The number of bytes received, or a negative value on failure.
 */
int net_socket_receive_from(net_socket_t *socket, void *buffer, int max_len, struct sockaddr_in *sender_addr) {
    if (!socket || !buffer || !sender_addr)
        return -1;
#ifdef _WIN32
    int addr_len = sizeof(*sender_addr);
    return recvfrom((SOCKET) socket->fd, (char *)buffer, max_len, 0, (struct sockaddr *) sender_addr, &addr_len);
#else
    socklen_t addr_len = sizeof(*sender_addr);
    return recvfrom(socket->fd, buffer, max_len, 0, (struct sockaddr*)sender_addr, &addr_len);
#endif
}

/**
 * @brief Binds the socket to a given IP and port.
 *
 * This function associates the socket with a specific network address, which is
 * a necessary step for server-side sockets.
 *
 * @param socket A pointer to the `net_socket_t` instance.
 * @param ip The IP address to bind to.
 * @param port The port to bind to.
 * @return 0 on success, or a negative value on failure.
 */
int net_socket_bind(net_socket_t *socket, const char *ip, uint16_t port) {
    if (!socket) return -1;

    socket->addr.sin_family = AF_INET;
    socket->addr.sin_port = htons(port);
    // Convert IP to binary format.
    if (inet_pton(AF_INET, ip, &socket->addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address/ Address not supported\n");
        return -1;
    }

    // Call bind and check the result.
    if (bind(socket->fd, (struct sockaddr*)&socket->addr, sizeof(socket->addr)) < 0) {
#ifdef _WIN32
        fprintf(stderr, "bind() failed with error code: %d\n", WSAGetLastError());
#else
        perror("bind() failed");
#endif
        return -1;
    }
    printf("[NetSocket-DEBUG] Socket FD %d successfully bound to %s:%u\n", socket->fd, ip, port);
    return 0;
}

uint16_t net_socket_get_local_port(const net_socket_t* s) {
    struct sockaddr_in addr; int len = (int)sizeof(addr);
    if (!s || s->fd == INVALID_SOCKET) return 0;
    if (getsockname(s->fd, (struct sockaddr*)&addr, &len) == 0)
        return ntohs(addr.sin_port);
    return 0;
}

/**
 * @brief Sets the socket in listening mode (only for TCP sockets).
 *
 * This function puts a TCP socket into a state where it can accept incoming
 * connections.
 *
 * @param socket A pointer to the `net_socket_t` instance.
 * @param backlog The maximum length of the pending connections queue.
 * @return 0 on success, or a negative value on failure.
 */
int net_socket_listen(net_socket_t* socket, int backlog) {
    if (!socket || socket->type != SOCKET_TYPE_TCP) return -1;
#ifdef _WIN32
    return listen((SOCKET)socket->fd, backlog);
#else
    return listen(socket->fd, backlog);
#endif
}

/**
 * @brief Closes the network socket connection.
 *
 * This function closes the socket file descriptor, releasing system resources.
 *
 * @param socket A pointer to the `net_socket_t` instance to close.
 * @return 0 on success, or a negative value on failure.
 */
int net_socket_close(net_socket_t *socket) {
#ifdef _WIN32
    return closesocket((SOCKET) socket->fd);
#else
    return close(socket->fd);
#endif
}

/**
 * @brief Performs any necessary cleanup after closing a connection.
 *
 * This is primarily for Windows platforms to clean up the Winsock library.
 */
void net_socket_cleanup(void) {
    if (PLATFORM_NAME == "Windows")
        WSACleanup();
}