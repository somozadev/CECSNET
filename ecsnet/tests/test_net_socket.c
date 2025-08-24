// test_net_socket.c
// Prueba de las funciones básicas de net_socket: init, bind, sendto y receive_from.

#include <stdio.h>
#include <string.h>
#include "../include/net_socket.h"
#include "../include/connection_manager.h"

int main(void) {
    // Init sockets system (windows needs it)
    net_socket_init();

    // Create UDP socket for the server and bind it to localhost port 12345
    net_socket_t server = net_socket_create(SOCKET_TYPE_UDP);
    if (net_socket_bind(&server, "127.0.0.1", 12345) < 0) {
        printf("Error: Couldn't bind server socket\n");
        net_socket_cleanup();
        return 1;
    }
    uint16_t server_port = net_socket_get_local_port(&server);
    printf("Server UDP in port %u\n", server_port);

    // Create UDP socket for the client
    net_socket_t client = net_socket_create(SOCKET_TYPE_UDP);

    // Msg to send
    const char *msg = "Hello world!";
    int sent = net_socket_sendto(&client, msg, (int)strlen(msg) + 1, &server.addr);
    if (sent < 0) {
        printf("Error sending datagram\n");
    } else {
        printf("Client received %d bytes\n", sent);
    }

    // Receiving in server
    char buf[64];
    struct sockaddr_in sender;
    int recvd = net_socket_receive_from(&server, buf, sizeof(buf), &sender);
    if (recvd < 0) {
        printf("Error receiving datagram\n");
    } else {
        printf("Server received %d bytes: %s\n", recvd, buf);
    }

    // Close sockets and cleanup
    net_socket_close(&client);
    net_socket_close(&server);
    net_socket_cleanup();
    return 0;
}