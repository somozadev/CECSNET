// #include "net/net_socket.h"
// #include "net/connection_manager.h"
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <winsock.h>
// #include <ws2def.h>
// #include <WS2tcpip.h>

// #define PORT 12345

// connection_manager_t connection_manager;

// void on_receive(peer_t* peer, const void* data, int len)
// {
//     printf("[Server] Received from %s: %.*s\n", peer->id, len, (char*) data);

//     char response[1024];
//     snprintf(response, sizeof(response), "Echo: %.*s", len, (char*) data);
//     net_socket_send(peer->net_socket, response, strlen(response));
// }

// void on_connect(peer_t* peer)
// {
//         printf("[Server] New connection: %s\n", peer->id);
// }


// void on_disconnect(peer_t* peer) {
//     printf("[Server] Disconnected: %s\n", peer->id);
// }

// int main() {
//     net_socket_init();

//     net_socket_t server = net_socket_create(SOCKET_TYPE_TCP);
    
//     struct sockaddr_in addr;
//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(PORT);
//     addr.sin_addr.s_addr = INADDR_ANY;

//     bind(server.fd, (struct sockaddr*)&addr, sizeof(addr));
//     listen(server.fd, 5);

//     printf("[Server] Listening on port %d...\n", PORT);

//     connection_manager_init(&connection_manager);
//     connection_manager.on_receive = on_receive;
//     connection_manager.on_connect = on_connect;
//     connection_manager.on_disconnect = on_disconnect;

//     while (1) {
//         struct sockaddr_in client_addr;
//         socklen_t len = sizeof(client_addr);
//         int client_fd = accept(server.fd, (struct sockaddr*)&client_addr, &len);

//         if (client_fd >= 0) {
//             net_socket_t client_sock;
//             client_sock.fd = client_fd;
//             client_sock.type = SOCKET_TYPE_TCP;

//             char peer_id[64];
//             snprintf(peer_id, sizeof(peer_id), "%s:%d",
//                      inet_ntoa(client_addr.sin_addr),
//                      ntohs(client_addr.sin_port));

//             connection_manager_add_peer(&connection_manager, &client_sock, peer_id);
//         }

//         connection_manager_update(&connection_manager);  // Check for messages
//     }

//     net_socket_close(&server);
//     net_sockets_cleanup();
//     return 0;
// }