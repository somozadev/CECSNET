cmake_minimum_required(VERSION 3.16)
project(ecsnet C)

set(CMAKE_C_STANDARD 99)

# Archivos fuente de tu librería
file(GLOB ECSNET_SOURCES src/*.c)

# Compilar como librería compartida (dll en Windows, so en Linux, dylib en Mac)
add_library(ecsnet SHARED ${ECSNET_SOURCES})

# Opcional: si quieres también versión estática
add_library(ecsnet_static STATIC ${ECSNET_SOURCES})

# Rutas de includes públicos
target_include_directories(ecsnet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if (WIN32)
    target_link_libraries(ecsnet PRIVATE ws2_32)
    # Exportar símbolos para DLL en MinGW
    # Esto es necesario para que el enlazador de MinGW sepa que los símbolos
    # de tu librería deben ser exportados.
    # Con el if(MSVC) que tenías antes no hacía falta, pero para MinGW sí.
    add_definitions(-DECSNET_EXPORTS)
endif()
# Exportar símbolos en Windows automáticamente
if (MSVC)
    target_compile_definitions(ecsnet PRIVATE ECSNET_EXPORTS)
    target_link_libraries(ecsnet PRIVATE ws2_32)
endif()

#cmake_minimum_required(VERSION 3.10)
#project(ecsnet C)
#
#set(CMAKE_C_STANDARD 11)
#
## Archivos fuente principales
#set(SOURCES
#        src/main.c
#        src/connection_manager.c
#        src/ecs.c
#        src/ecs_builtin.c
#        src/net_socket.c
#        src/network_architecture.c
#        src/protocol_handler.c
#)
#
#
## Ejecutable principal
#add_executable(ecsnet ${SOURCES}
#        include/ecs_internal.h
#        src/network_cs.c
#        include/network_cs.h
#        include/ecsnet.h
#)
#target_include_directories(ecsnet PRIVATE include)
#
## Librerías para Windows
#if (WIN32)
#    target_link_libraries(ecsnet ws2_32)
#endif()
#
## Ejecutable de tests (opcional)
#add_executable(test_ecs
#        tests/test_ecs.c
#        tests/test.c
#        ${SOURCES}
#        tests/test_net.h
#        tests/test_net.c
#        tests/test_networking_architecture.c
#        tests/test_networking_architecture.h
#)
#target_include_directories(test_ecs PRIVATE include)
#set_property(TARGET test_ecs PROPERTY C_STANDARD 11)
#
#if (WIN32)
#    target_link_libraries(test_ecs ws2_32)
#endif()
