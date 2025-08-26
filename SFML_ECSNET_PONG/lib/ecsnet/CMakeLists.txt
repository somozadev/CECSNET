cmake_minimum_required(VERSION 3.16)
project(ecsnet C)

set(CMAKE_C_STANDARD 99)

# Ecsnet library source files
file(GLOB ECSNET_SOURCES src/*.c)

# Compile as static library
add_library(ecsnet STATIC ${ECSNET_SOURCES})

# Include public paths
target_include_directories(ecsnet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link with winsock in Windows
if (WIN32)
    target_link_libraries(ecsnet PUBLIC ws2_32)
endif()
