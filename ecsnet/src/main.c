#include <stdio.h>
#include "ecs.h"
#include "net.h"

int main()
{
    if (net_init() != 0)
    {
        printf("Failed to init networking.\n");
        return 1;
    }

    net_socket sock = net_connect("127.0.0.1", 12345);
    if (sock < 0)
    {
        printf("Connection failed.\n");
        net_cleanup();
        return 1;
    }

    net_send(sock, "Hello from client!");

    char buffer[1024];
    int bytes = net_receive(sock, buffer, sizeof(buffer) - 1);
    if (bytes > 0)
    {
        buffer[bytes] = '\0';
        printf("Received: %s\n", buffer);
    }

    net_close(sock);
    net_cleanup();
    return 0;
}