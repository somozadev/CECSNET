cmake_minimum_required(VERSION 3.16)
project(ecsnet C)

set(CMAKE_C_STANDARD 99)

# Archivos fuente de la librería
file(GLOB ECSNET_SOURCES src/*.c)

# Compilar como librería estática
add_library(ecsnet STATIC ${ECSNET_SOURCES})

# Rutas de includes públicos
target_include_directories(ecsnet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Enlazar con winsock en Windows
if (WIN32)
    target_link_libraries(ecsnet PUBLIC ws2_32)
endif()
