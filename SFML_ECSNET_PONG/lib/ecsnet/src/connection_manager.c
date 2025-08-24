#include "connection_manager.h"
#include "protocol_handler.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/select.h>
#include <arpa/inet.h>
#endif

// Auxiliary function to find a peer by its IP and port for a specific socket type.
peer_t* find_peer_by_addr(connection_manager_t* connection_manager, const struct sockaddr_in* addr, socket_type_t socket_type) {
    // Iterate through the list of active peers.
    for (int i = 0; i < connection_manager->peer_count; ++i) {
        const struct sockaddr_in* peer_addr;
        // Determine which address to use for comparison based on the socket type.
        if (socket_type == SOCKET_TYPE_TCP) {
            peer_addr = &connection_manager->peers[i].addr_tcp;
        } else {
            peer_addr = &connection_manager->peers[i].addr_udp;
        }

        // Compare the IP address and port to find a match.
        if (peer_addr->sin_addr.s_addr == addr->sin_addr.s_addr &&
            peer_addr->sin_port == addr->sin_port) {
            return &connection_manager->peers[i];
            }
    }
    // No matching peer was found.
    return NULL;
}
void connection_manager_init(connection_manager_t* connection_manager) {
    if (!connection_manager) return;
    // Clear all peer data, set the peer count to zero, and clear listen sockets.
    memset(connection_manager->peers, 0, sizeof(connection_manager->peers));
    connection_manager->peer_count = 0;
    memset(connection_manager->listen_sockets, 0, sizeof(connection_manager->listen_sockets));
    // Clear the function pointers for all callbacks and the user data pointer.
    connection_manager->on_receive = NULL;
    connection_manager->on_connect = NULL;
    connection_manager->on_disconnect = NULL;
    connection_manager->user_data = NULL;
}

void connection_manager_add_listen_socket(connection_manager_t* connection_manager, net_socket_t socket, socket_type_t socket_type) {
    if (connection_manager && socket_type < PROTOCOL_COUNT)
        // Store the socket in the correct position of the listen_sockets array.
        connection_manager->listen_sockets[socket_type] = socket;
}

void connection_manager_remove_peer(connection_manager_t* connection_manager, const char* peer_id) {
    // Find the peer to remove by iterating through the list.
    for (int i = 0; i < connection_manager->peer_count; ++i) {
        if (strcmp(connection_manager->peers[i].id, peer_id) == 0) {
            peer_t* peer_to_remove = &connection_manager->peers[i];

            // Call the disconnection callback if it is set.
            if (connection_manager->on_disconnect) {
                connection_manager->on_disconnect(connection_manager->user_data, peer_to_remove);
            }

            // Close the TCP socket for the removed peer.
            if (peer_to_remove->net_sockets[SOCKET_TYPE_TCP].fd != -1) {
                net_socket_close(&peer_to_remove->net_sockets[SOCKET_TYPE_TCP]);
            }

            // Replace the removed peer with the last peer in the array to maintain a contiguous list.
            connection_manager->peers[i] = connection_manager->peers[connection_manager->peer_count - 1];
            // Decrease the peer count.
            connection_manager->peer_count--;
            return;
        }
    }
}

int connection_manager_send_to_peer(connection_manager_t* cm, const char* peer_id, const void* data, int len) {
    if (!cm || !peer_id || !data || len <= 0) return -1;

    const network_packet_t* pkt = (const network_packet_t*)data;

    for (int i = 0; i < cm->peer_count; ++i) {
        peer_t* p = &cm->peers[i];
        if (strcmp(p->id, peer_id) != 0) continue;

        const int fd_tcp = p->net_sockets[SOCKET_TYPE_TCP].fd;
        const int fd_udp = cm->listen_sockets[SOCKET_TYPE_UDP].fd; // siempre usar el listen UDP del manager

        // Solo los updates ECS van por UDP
        const int is_ecs_update =
            pkt &&
            (pkt->header.type == PACKET_TYPE_ENTITY_UPDATE ||
             pkt->header.type == PACKET_TYPE_MULTI_ENTITY_UPDATE);

        if (is_ecs_update && p->udp_ready && fd_udp != INVALID_SOCKET && p->addr_udp.sin_port != 0) {
            return net_socket_sendto(&cm->listen_sockets[SOCKET_TYPE_UDP], data, len, &p->addr_udp);
        }

        // Todo lo demás por TCP
        if (fd_tcp != INVALID_SOCKET) {
            return net_socket_send(&p->net_sockets[SOCKET_TYPE_TCP], data, len);
        }

        return -3; // sin ruta válida
    }
    return -1; // peer no encontrado
}
int connection_manager_send_to_peer_udp(connection_manager_t* connection_manager, const char* peer_id, const void* data, int len) {
    if (!connection_manager || !peer_id || !data || len <= 0) return -1;
    for (int i = 0; i < connection_manager->peer_count; ++i) {
        peer_t* p = &connection_manager->peers[i];
        if (strcmp(p->id, peer_id) == 0) {
            if (connection_manager->listen_sockets[SOCKET_TYPE_UDP].fd != INVALID_SOCKET && p->udp_ready && p->addr_udp.sin_port != 0) {
                return net_socket_sendto(&connection_manager->listen_sockets[SOCKET_TYPE_UDP], data, len, &p->addr_udp);
            }
            return -2;
        }
    }
    return -3;
}

int connection_manager_broadcast(connection_manager_t* cm, const void* data, int len) {
    if (!cm || !data || len <= 0) return 0;
    int ok = 0;
    for (int i = 0; i < cm->peer_count; ++i) {
        peer_t* p = &cm->peers[i];
        int sent = -1;

        if (p->udp_ready && cm->listen_sockets[SOCKET_TYPE_UDP].fd != INVALID_SOCKET) {
            sent = net_socket_sendto(&cm->listen_sockets[SOCKET_TYPE_UDP], data, len, &p->addr_udp);
        }
        if (sent < 0 && p->net_sockets[SOCKET_TYPE_TCP].fd != INVALID_SOCKET) {
            sent = net_socket_send(&p->net_sockets[SOCKET_TYPE_TCP], data, len);
        }
        if (sent >= 0) ok++;
    }
    return ok;
}



int connection_manager_add_peer_tcp(connection_manager_t* cm, SOCKET accepted_fd, const struct sockaddr_in* addr) {
    // Check if the maximum number of peers has been reached.
    if (cm->peer_count >= MAX_PEERS) {
        closesocket(accepted_fd);
        return -1;
    }

    // Check if the peer already exists.
    if (find_peer_by_addr(cm, addr, SOCKET_TYPE_TCP)) {
        closesocket(accepted_fd);
        return -2;
    }

    // Allocate a new peer entry and initialize its data.
    peer_t* new_peer = &cm->peers[cm->peer_count++];
    memset(new_peer, 0, sizeof(peer_t));

    new_peer->addr_tcp = *addr;
    new_peer->is_connected = true;
    snprintf(new_peer->id, sizeof(new_peer->id), "TCP_Peer_%s:%hu", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));

    // Configure the new peer's TCP socket.
    new_peer->net_sockets[SOCKET_TYPE_TCP].fd = (int)accepted_fd;
    new_peer->net_sockets[SOCKET_TYPE_TCP].type = SOCKET_TYPE_TCP;
    net_socket_set_non_blocking(&new_peer->net_sockets[SOCKET_TYPE_TCP]);

    // Initialize UDP-related fields to a known invalid state.
    memset(&new_peer->addr_udp, 0, sizeof(new_peer->addr_udp));
    new_peer->net_sockets[SOCKET_TYPE_UDP].fd = -1;

    // Call the connection callback if it is set.
    if (cm->on_connect) cm->on_connect(cm->user_data, new_peer);
    return 0;
}

int connection_manager_add_peer_client_tcp(connection_manager_t* cm, net_socket_t client_socket, const struct sockaddr_in* server_addr) {
    // Check if the maximum number of peers has been reached.
    if (cm->peer_count >= MAX_PEERS) {
        net_socket_close(&client_socket);
        return -1;
    }

    // Allocate a new peer entry to represent the server.
    peer_t* new_peer = &cm->peers[cm->peer_count++];
    memset(new_peer, 0, sizeof(peer_t));

    // Store the server's TCP and UDP addresses.
    new_peer->addr_tcp = *server_addr;
    new_peer->is_connected = false;
    snprintf(new_peer->id, sizeof(new_peer->id), "Server_%s:%hu", inet_ntoa(server_addr->sin_addr), ntohs(server_addr->sin_port));

    // Store the client's TCP socket.
    new_peer->net_sockets[SOCKET_TYPE_TCP] = client_socket;

    // Configure the UDP address and socket for the peer.
    new_peer->addr_udp = new_peer->addr_tcp;
    new_peer->addr_udp.sin_port = 0; // The port will be updated later.
    new_peer->net_sockets[SOCKET_TYPE_UDP] = cm->listen_sockets[SOCKET_TYPE_UDP];

    printf("[CM-Client] Server peer added with FD: %d. Waiting for connection...\n", new_peer->net_sockets[SOCKET_TYPE_TCP].fd);

    return 0;
}

int connection_manager_connect_to_server(connection_manager_t* connection_manager, const char* ip, uint16_t port) {
    if (!connection_manager || !ip) {
        printf("[CM-Client] ERROR: connection_manager or IP are invalid.\n");
        return -1;
    }

    // Create a new TCP socket for the client.
    net_socket_t client_socket = net_socket_create(SOCKET_TYPE_TCP);
    if (client_socket.fd == INVALID_SOCKET) {
        printf("[CM-Client] ERROR: Failed to create TCP socket.\n");
        return -1;
    }

    // Prepare the server's address structure.
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    printf("[CM-Client] Attempting to connect to %s:%hu...\n", ip, port);
    // Try to connect to the server.
    int result = connect(client_socket.fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    bool connection_in_progress = false;
    if (result == 0) {
        // Immediate connection success.
        connection_in_progress = true;
        printf("[CM-Client] Immediate TCP connection successful.\n");
    } else {
#ifdef _WIN32
        int error_code = WSAGetLastError();
        // Handle non-blocking socket behavior where the connection is in progress.
        if (error_code == WSAEWOULDBLOCK) {
            connection_in_progress = true;
            printf("[CM-Client] `connect()` returned WSAEWOULDBLOCK. Connection in progress.\n");
        } else {
            fprintf(stderr, "[CM-Client] ERROR: Fatal connection failure: %d. Closing socket.\n", error_code);
            net_socket_close(&client_socket);
            return -1;
        }
#else
        // Handle non-blocking socket behavior on other platforms.
        if (errno == EINPROGRESS) {
            connection_in_progress = true;
            printf("[CM-Client] `connect()` returned EINPROGRESS. Connection in progress.\n");
        } else {
            perror("[CM-Client] ERROR: Fatal connection failure");
            net_socket_close(&client_socket);
            return -1;
        }
#endif
    }

    if (connection_in_progress) {
        // If a connection is in progress, add a new peer to manage it.
        connection_manager_add_peer_client_tcp(connection_manager, client_socket, &server_addr);
        return 0;
    }
    return -1;
}
net_socket_t* connection_manager_get_listen_socket(connection_manager_t* cm, socket_type_t type) {
    if (cm && type < PROTOCOL_COUNT) {
        // Return a pointer to the specified listen socket.
        return &cm->listen_sockets[type];
    }
    return NULL;
}

uint16_t connection_manager_get_udp_local_port(connection_manager_t* cm) {
    if (!cm) return 0;
    net_socket_t* udp = &cm->listen_sockets[SOCKET_TYPE_UDP];
    if (udp && udp->fd != INVALID_SOCKET) {
        return net_socket_get_local_port(udp);
    }
    return 0;
}

int connection_manager_set_peer_udp_remote_port_by_id(connection_manager_t* cm, const char* peer_id, uint16_t port) {
    for (int i = 0; i < cm->peer_count; ++i) {
        peer_t* p = &cm->peers[i];
        if (strcmp(p->id, peer_id) == 0) {
            p->addr_udp = p->addr_tcp;
            p->addr_udp.sin_port = htons(port);
            // usar SIEMPRE el socket UDP listen del manager para enviar a este peer
            p->net_sockets[SOCKET_TYPE_UDP] = cm->listen_sockets[SOCKET_TYPE_UDP];
            p->udp_ready = 1;
            return 0;
        }
    }
    return -1;
}

void connection_manager_update(connection_manager_t* connection_manager) {
    if (!connection_manager) return;

    // Set up file descriptor sets for `select()`.
    fd_set read_fds, write_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);

    SOCKET max_fd = 0;

    // Add listen sockets to the read set.
    net_socket_t* udp_listen_socket = &connection_manager->listen_sockets[SOCKET_TYPE_UDP];
    if (udp_listen_socket->fd != INVALID_SOCKET) {
        FD_SET(udp_listen_socket->fd, &read_fds);
        if (udp_listen_socket->fd > max_fd) max_fd = udp_listen_socket->fd;
    }

    net_socket_t* tcp_listen_socket = &connection_manager->listen_sockets[SOCKET_TYPE_TCP];
    if (tcp_listen_socket->fd != INVALID_SOCKET) {
        FD_SET(tcp_listen_socket->fd, &read_fds);
        if (tcp_listen_socket->fd > max_fd) max_fd = tcp_listen_socket->fd;
    }

    // Add peer sockets to the sets.
    for (int i = 0; i < connection_manager->peer_count; ++i) {
        net_socket_t* peer_tcp_socket = &connection_manager->peers[i].net_sockets[SOCKET_TYPE_TCP];
        if (peer_tcp_socket->fd != INVALID_SOCKET) {
            // Check for incoming data.
            FD_SET(peer_tcp_socket->fd, &read_fds);
            // On the client, check for a completed connection attempt.
            if (!connection_manager->is_server && !connection_manager->peers[i].is_connected) {
                FD_SET(peer_tcp_socket->fd, &write_fds);
            }
            if (peer_tcp_socket->fd > max_fd) max_fd = peer_tcp_socket->fd;
        }
    }

    if (max_fd == 0) return;

    // Set a short timeout for the select call.
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100;

    // Call select to check for socket activity.
    int activity = select((int)max_fd + 1, &read_fds, &write_fds, NULL, &timeout);

    if (activity <= 0) return;

    // Handle new TCP connections (server-side).
    if (connection_manager->is_server && tcp_listen_socket->fd != INVALID_SOCKET && FD_ISSET(tcp_listen_socket->fd, &read_fds)) {
        struct sockaddr_in new_peer_addr;
        socklen_t addr_len = sizeof(new_peer_addr);
        SOCKET new_socket_fd = accept((SOCKET)tcp_listen_socket->fd, (struct sockaddr*)&new_peer_addr, &addr_len);

        if (new_socket_fd != INVALID_SOCKET) {
            printf("[CM-Server] Client connection accepted. FD: %d\n", (int)new_socket_fd);
            connection_manager_add_peer_tcp(connection_manager, new_socket_fd, &new_peer_addr);
        }
        if (--activity <= 0) return;
    }

    // Handle incoming UDP data.
    if (udp_listen_socket->fd != -1 && FD_ISSET(udp_listen_socket->fd, &read_fds)) {
        uint8_t buffer[MAX_PACKET_SIZE];
        struct sockaddr_in sender_addr;
        int bytes_received = net_socket_receive_from(udp_listen_socket, buffer, MAX_PACKET_SIZE, &sender_addr);
        if (bytes_received > 0) {
            peer_t* peer = find_peer_by_addr(connection_manager, &sender_addr, SOCKET_TYPE_UDP);
            if (!peer) {
                printf("[CM-Server] UDP packet received from an unknown peer. Ignoring.\n");
            } else if (connection_manager->on_receive) {
                // Call the receive callback.
                connection_manager->on_receive(connection_manager->user_data, peer, buffer, bytes_received);
            }
        }
        if (--activity <= 0) return;
    }

    // Iterate through peers to handle TCP events.
    for (int i = 0; i < connection_manager->peer_count; ) {
        net_socket_t* peer_tcp_socket = &connection_manager->peers[i].net_sockets[SOCKET_TYPE_TCP];
        peer_t* peer = &connection_manager->peers[i];
        bool peer_removed = false;

        // Handle a completed client connection.
        if (!connection_manager->is_server && !peer->is_connected && peer_tcp_socket->fd != INVALID_SOCKET && FD_ISSET(peer_tcp_socket->fd, &write_fds)) {
            int error = 0;
            socklen_t len = sizeof(error);
            getsockopt(peer_tcp_socket->fd, SOL_SOCKET, SO_ERROR, (char*)&error, &len);

            if (error == 0) {
                printf("[CM-Client] TCP connection to peer successful. FD: %d\n", (int)peer_tcp_socket->fd);
                peer->is_connected = true; // Mark the connection as complete.
                if (connection_manager->on_connect) {
                    connection_manager->on_connect(connection_manager->user_data, peer);
                }
            } else {
                fprintf(stderr, "[CM-Client] ERROR: TCP connection failure to peer %s: %d. Removing peer.\n", peer->id, error);
                connection_manager_remove_peer(connection_manager, peer->id);
                peer_removed = true;
            }
            if (--activity <= 0) break;
        }

        // Handle incoming TCP data from a peer.
        if (peer_tcp_socket->fd != INVALID_SOCKET && FD_ISSET(peer_tcp_socket->fd, &read_fds)) {
            char buffer[MAX_PACKET_SIZE];
            int bytes_received = recv(peer_tcp_socket->fd, buffer, sizeof(buffer), 0);

            if (bytes_received <= 0) {
                fprintf(stdout, "[CM] TCP Peer %s disconnected or reception error.\n", peer->id);
                connection_manager_remove_peer(connection_manager, peer->id);
                peer_removed = true;
            } else {
                // Append al buffer del peer
                if (peer->tcp_rx_len + bytes_received > (int)sizeof(peer->tcp_rx_buf)) {
                    // Desbordado: resetea (o desconecta, como prefieras)
                    peer->tcp_rx_len = 0;
                } else {
                    memcpy(peer->tcp_rx_buf + peer->tcp_rx_len, buffer, bytes_received);
                    peer->tcp_rx_len += bytes_received;

                    // Extrae paquetes completos
                    for (;;) {
                        if (peer->tcp_rx_len < (int)sizeof(packet_header_t)) break;

                        packet_header_t hdr;
                        memcpy(&hdr, peer->tcp_rx_buf, sizeof(packet_header_t));

                        // sanity
                        if (hdr.size < sizeof(packet_header_t) || hdr.size > MAX_PACKET_SIZE) {
                            // stream corrupto: resetea (o desconecta)
                            peer->tcp_rx_len = 0;
                            break;
                        }
                        if (peer->tcp_rx_len < (int)hdr.size) break; // espera más bytes

                        // Tenemos paquete completo [0..hdr.size)
                        if (connection_manager->on_receive) {
                            connection_manager->on_receive(connection_manager->user_data,
                                                           peer,
                                                           peer->tcp_rx_buf,
                                                           (int)hdr.size);
                        }
                        // Desplaza el sobrante a la cabeza
                        int remain = peer->tcp_rx_len - (int)hdr.size;
                        if (remain > 0) {
                            memmove(peer->tcp_rx_buf, peer->tcp_rx_buf + hdr.size, remain);
                        }
                        peer->tcp_rx_len = remain;
                    }
                }
            }
        }

        // Only increment the counter if the current peer was not removed.
        if (!peer_removed) {
            i++;
        }
    }
}

void connection_manager_destroy(connection_manager_t* connection_manager) {
    if (!connection_manager) return;

    // Close all listen sockets.
    for (int i = 0; i < PROTOCOL_COUNT; ++i) {
        if (connection_manager->listen_sockets[i].fd != -1)
            net_socket_close(&connection_manager->listen_sockets[i]);
    }

    // Close all TCP sockets for active peers.
    for (int i = 0; i < connection_manager->peer_count; ++i) {
        if (connection_manager->peers[i].net_sockets[SOCKET_TYPE_TCP].fd != -1) {
            net_socket_close(&connection_manager->peers[i].net_sockets[SOCKET_TYPE_TCP]);
        }
    }

    // Reset the entire structure to zero.
    memset(connection_manager, 0, sizeof(connection_manager_t));
}