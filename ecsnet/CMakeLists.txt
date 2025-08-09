cmake_minimum_required(VERSION 3.10)
project(ecsnet C)

set(CMAKE_C_STANDARD 11)

# Archivos fuente principales
set(SOURCES
        src/main.c
        src/connection_manager.c
        src/ecs.c
        src/ecs_builtin.c
        src/net_socket.c
        src/network_architecture.c
        src/protocol_handler.c
        src/ecs_sync.cpp
        include/ecs_sync.h
)

# Ejecutable principal
add_executable(ecsnet ${SOURCES}
        src/ecs_sync.cpp
        include/ecs_sync.h)
target_include_directories(ecsnet PRIVATE include)

# Librerías para Windows
if (WIN32)
    target_link_libraries(ecsnet ws2_32)
endif()

# Ejecutable de tests (opcional)
add_executable(test_ecs
        tests/test_ecs.c
        tests/test.c
        ${SOURCES}
        src/ecs_sync.cpp
        include/ecs_sync.h
        tests/test_net.h
        tests/test_net.c
)
target_include_directories(test_ecs PRIVATE include)
set_property(TARGET test_ecs PROPERTY C_STANDARD 11)

if (WIN32)
    target_link_libraries(test_ecs ws2_32)
endif()
