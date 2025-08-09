#include <stdio.h>
#include "ecs.h"
#include <winsock2.h>
#include <windows.h>
#include <ws2def.h>

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    return 0;
}
