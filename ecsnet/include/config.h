#pragma once

#define MAX_ENTITIES 1024
#define MAX_COMPONENTS 32


#define NETWORK_BUFFER_SIZE 4096
//1=TCP 2=UDP
#define DEFAULT_SOCKET_TYPE 1 

#define NETWORK_TIMEOUT_MS 3000

#ifdef _WIN32
#define PLATFORM_NAME "Windows"
#else
#define PLATFORM_NAME "Unix"
#endif


#define ENABLE_DEBUG_LOGS 0

