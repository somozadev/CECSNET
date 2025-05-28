#include <stdio.h>
#include "net.h"
#include <string.h>

#ifdef _WIN32
#include <ws2tcpip.h>

int net_init()
{
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa);
}

void net_cleanup()
{
    WSACleanup();
}
void net_close(net_socket sock)
{
    closesocket(sock);
}
#else

int net_init() {
    return 0; // no-op on Unix
}

void net_cleanup() {
    // no-op
}

void net_close(net_socket sock) {
    close(sock);
}

#endif


net_socket net_connect(const char* ip, int port) {
    net_socket sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket creation failed");
        return -1;
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("connect failed");
        net_close(sock);
        return -1;
    }

    return sock;
}

int net_send(net_socket sock, const char* msg) {
    return send(sock, msg, (int)strlen(msg), 0);
}

int net_receive(net_socket sock, char* buffer, int bufsize) {
    return recv(sock, buffer, bufsize, 0);
}