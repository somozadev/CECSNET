#pragma once

#include "net_socket.h"
#include <stdint.h>
#include <stdbool.h>
#include <winsock2.h>
#include "config.h" 


#ifdef __cplusplus
extern "C" {
#endif // Cierra el bloque extern "C"

/**
 * @brief Defines the number of network protocols being used (TCP and UDP).
 */
#define PROTOCOL_COUNT 2

/**
 * @brief Forward declaration of the protocol_handler_t, network_cs_t and connection_manager_t structures.
 */
typedef struct protocol_handler_t protocol_handler_t;
typedef struct network_cs_t network_cs_t;
typedef struct connection_manager_t connection_manager_t;
/**
 * @brief Defines the maximum number of peers that can be managed simultaneously.
 */
#define MAX_PEERS 32

/**
 * @brief Represents a connected network peer.
 *
 * This struct contains all the necessary information to identify and communicate
 * with a single remote peer, including its addresses and sockets.
 */
ECSNET_API typedef struct peer_t {
    char id[64];                            /**< The unique identifier for the peer (e.g., "IP:PORT"). */
    struct sockaddr_in addr_tcp;            /**< The TCP address of the peer. */
    struct sockaddr_in addr_udp;            /**< The UDP address of the peer. */
    net_socket_t net_sockets[PROTOCOL_COUNT]; /**< Sockets used for communication with this peer. */
    bool is_connected;                      /**< A flag indicating if the peer connection is currently active. */
    bool udp_ready;                      /**< A flag indicating if the peer udp connection is ready. */
} peer_t;


/**
 * @brief Manages all active network connections (peers) for a client or server.
 *
 * This structure holds an array of all connected peers, manages the listen sockets,
 * and stores pointers to callback functions for network events.
 */
ECSNET_API typedef struct connection_manager_t {
    peer_t peers[MAX_PEERS];                /**< An array of all managed peers. */
    int peer_count;                         /**< The current number of active peers. */
    net_socket_t listen_sockets[PROTOCOL_COUNT]; /**< Sockets for listening on both TCP and UDP. */
    bool is_server;                         /**< A flag indicating if this instance is a server. */
    void* user_data;                        /**< User data passed to callbacks  */

    // Callbacks for different network events
    void (*on_receive)(void*, peer_t*, const void*, int);       /**< The callback for when data is received. */
    void (*on_connect)(void*, peer_t*);                         /**< The callback for when a new peer connects. */
    void (*on_disconnect)(void*, peer_t*);                      /**< The callback for when a peer disconnects. */
} connection_manager_t;

/**
 * @brief Initializes the connection manager and clears all peers.
 * @param connection_manager A pointer to the connection_manager_t instance to initialize.
 */
void connection_manager_init(connection_manager_t* connection_manager);

/**
 * @brief Adds a new listen socket to the manager.
 * @param connection_manager A pointer to the connection_manager_t instance.
 * @param socket The socket to add.
 * @param socket_type The type of the socket (TCP or UDP).
 */
void connection_manager_add_listen_socket(connection_manager_t* connection_manager, net_socket_t socket, socket_type_t socket_type);

/**
 * @brief Removes a given peer by ID from the manager.
 * @param connection_manager A pointer to the connection_manager_t instance.
 * @param peer_id The ID of the peer to remove.
 */
void connection_manager_remove_peer(connection_manager_t* connection_manager, const char* peer_id);

/**
 * @brief Sends data to a specific peer by ID.
 * @param connection_manager A pointer to the connection_manager_t instance.
 * @param peer_id The ID of the peer to send the data to.
 * @param data A pointer to the data buffer.
 * @param len The length of the data to send.
 * @return The number of bytes sent, or a negative value on failure.
 */
int connection_manager_send_to_peer(connection_manager_t* connection_manager, const char* peer_id, const void* data, int len);

/**
 * @brief Sends data to all connected peers.
 * @param connection_manager A pointer to the connection_manager_t instance.
 * @param data A pointer to the data buffer.
 * @param len The length of the data to send.
 * @return The number of bytes sent, or a negative value on failure.
 */
int connection_manager_broadcast(connection_manager_t* connection_manager, const void* data, int len);

/**
 * @brief Attempts to connect to a server with a given IP and port.
 * @param connection_manager A pointer to the connection_manager_t instance.
 * @param ip The IP address of the server.
 * @param port The TCP port of the server.
 * @return 0 on success, -1 on failure.
 */
int connection_manager_connect_to_server(connection_manager_t* connection_manager, const char* ip, uint16_t port);

/**
 * @brief Updates the manager by polling for I/O and processing events.
 * This function should be called regularly in the main loop.
 * @param connection_manager A pointer to the connection_manager_t instance.
 */
void connection_manager_update(connection_manager_t* connection_manager);

/**
 * @brief Destroys the connection manager, closing all sockets and cleaning up resources.
 * @param connection_manager A pointer to the connection_manager_t instance to destroy.
 */
void connection_manager_destroy(connection_manager_t* connection_manager);

/**
 * @brief Retrieves a listen socket of a specific type.
 * @param cm A pointer to the connection_manager_t instance.
 * @param type The type of socket to retrieve (TCP or UDP).
 * @return A pointer to the net_socket_t instance, or NULL if not found.
 */
net_socket_t* connection_manager_get_listen_socket(connection_manager_t* cm, socket_type_t type);

uint16_t connection_manager_get_udp_local_port(connection_manager_t* cm);

int connection_manager_set_peer_udp_remote_port_by_id(connection_manager_t* cm, const char* peer_id, uint16_t remote_udp_port);

/**
 * @brief Finds a peer by its network address.
 * @param connection_manager A pointer to the connection_manager_t instance.
 * @param addr A pointer to the sockaddr_in structure of the peer.
 * @param socket_type The type of socket to check (TCP or UDP).
 * @return A pointer to the peer_t instance, or NULL if not found.
 */
peer_t* find_peer_by_addr(connection_manager_t* connection_manager, const struct sockaddr_in* addr, socket_type_t socket_type);

#ifdef __cplusplus
}
#endif
