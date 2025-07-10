#pragma once

//ecs configuration
#define MAX_ENTITIES 1024
#define MAX_COMPONENTS 32
#define MAX_SYSTEMS 64

//networking configuration
#define NETWORK_BUFFER_SIZE 4096
#define DEFAULT_SOCKET_TYPE 1 //1=TCP 2=UDP
#define NETWORK_TIMEOUT_MS 3000

//Compiling target platform configuration
#ifdef _WIN32
#define PLATFORM_NAME "Windows"
#else
#define PLATFORM_NAME "Unix"
#endif


