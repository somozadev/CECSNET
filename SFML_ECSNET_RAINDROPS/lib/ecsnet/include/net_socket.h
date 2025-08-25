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
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Enumeration of available socket types (TCP and UDP).
 */
typedef enum
{
    SOCKET_TYPE_TCP,    /**< TCP socket type. */
    SOCKET_TYPE_UDP,    /**< UDP socket type. */
    PROTOCOL_COUNT      /**< The total number of supported protocols. */
} socket_type_t;

/**
 * @brief Represents a network socket.
 *
 * This struct encapsulates the socket's file descriptor, type, and address
 * information.
 */
typedef struct
{
    struct sockaddr_in addr;    /**< The address associated with the socket. */
    socket_type_t type;         /**< The type of the socket (TCP or UDP). */
    int fd;                     /**< The socket file descriptor. */
} net_socket_t;

/**
 * @brief Creates a new network socket.
 * @param type The type of socket to create (TCP or UDP).
 * @return The newly created net_socket_t instance.
 */
net_socket_t net_socket_create(socket_type_t type);

/**
 * @brief Makes the socket non-blocking.
 * @param socket A pointer to the net_socket_t instance.
 * @return 0 on success, or a negative value on failure.
 */
int net_socket_set_non_blocking(net_socket_t* socket);

/**
 * @brief Connects the socket to a given IP and port.
 * @param socket A pointer to the net_socket_t instance.
 * @param ip The IP address to connect to.
 * @param port The port to connect to.
 * @return 0 on success, or a negative value on failure.
 */
int net_socket_connect(net_socket_t* socket,  char* ip, uint16_t port);

/**
 * @brief Sends data over the socket connection.
 * @param socket A pointer to the net_socket_t instance.
 * @param data A pointer to the data buffer to send.
 * @param len The length of the data to send.
 * @return The number of bytes sent, or a negative value on failure.
 */
int net_socket_send(net_socket_t* socket, const void* data, int len);

/**
 * @brief Sends data to a specific address (for connectionless sockets like UDP).
 * @param socket A pointer to the net_socket_t instance.
 * @param data A pointer to the data buffer to send.
 * @param len The length of the data to send.
 * @param addr A pointer to the destination address.
 * @return The number of bytes sent, or a negative value on failure.
 */
int net_socket_sendto(net_socket_t* socket, const void* data, int len, const struct sockaddr_in* addr);

/**
 * @brief Receives a data buffer over the socket connection.
 * @param socket A pointer to the net_socket_t instance.
 * @param buffer A pointer to the buffer to store received data.
 * @param max_len The maximum length of the buffer.
 * @return The number of bytes received, or a negative value on failure.
 */
int net_socket_receive(net_socket_t* socket, void* buffer, int max_len);

/**
 * @brief Receives a data buffer over a socket and returns the sender's address (for UDP).
 * @param socket A pointer to the net_socket_t instance.
 * @param buffer A pointer to the buffer to store received data.
 * @param max_len The maximum length of the buffer.
 * @param sender_addr A pointer to a sockaddr_in struct to store the sender's address.
 * @return The number of bytes received, or a negative value on failure.
 */
int net_socket_receive_from(net_socket_t* socket, void* buffer, int max_len, struct sockaddr_in* sender_addr);

/**
 * @brief Binds the socket to a given IP and port (for server-side sockets).
 * @param socket A pointer to the net_socket_t instance.
 * @param ip The IP address to bind to.
 * @param port The port to bind to.
 * @return 0 on success, or a negative value on failure.
 */
int net_socket_bind(net_socket_t* socket, const char* ip, uint16_t port);


uint16_t net_socket_get_local_port(const net_socket_t* s);
/**
 * @brief Sets the socket in listening mode (only for TCP sockets).
 * @param socket A pointer to the net_socket_t instance.
 * @param backlog The maximum length of the pending connections queue.
 * @return 0 on success, or a negative value on failure.
 */
int net_socket_listen(net_socket_t* socket, int backlog);

/**
 * @brief Closes the network socket connection.
 * @param socket A pointer to the net_socket_t instance to close.
 * @return 0 on success, or a negative value on failure.
 */
int net_socket_close(net_socket_t* socket);

/**
 * @brief Initializes underlying socket subsystem.
 *
 * On Windows platforms, this function calls WSAStartup() to initialize the
 * Winsock library. On POSIX systems it does nothing.  Call this once
 * before creating any sockets.
 */
void net_socket_init(void);

/**
 * @brief Performs any necessary cleanup of the socket subsystem.
 *
 * On Windows platforms, this function calls WSACleanup() to tear down
 * Winsock. On POSIX systems it does nothing.  Call this once when
 * shutting down networking.
 */
void net_socket_cleanup(void);

#ifdef __cplusplus
}
#endif

