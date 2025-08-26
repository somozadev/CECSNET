cmake_minimum_required(VERSION 3.16)
project(ecsnet C)

set(CMAKE_C_STANDARD 99)

# Ecsnet library source files
file(GLOB ECSNET_SOURCES src/*.c)


# ================================
# STATIC LIBRARY (.lib in win, .a in linux)
# ================================
add_library(ecsnet_static STATIC ${ECSNET_SOURCES})
target_include_directories(ecsnet_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_definitions(ecsnet_static PRIVATE ECSNET_STATIC)

if (WIN32)
    target_link_libraries(ecsnet_static PUBLIC ws2_32)
endif()

set_target_properties(ecsnet_static PROPERTIES
        OUTPUT_NAME "ecsnet"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/static"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/static"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/static"
)

# ================================
# Dynamic library (.dll in windows, .so in linux)
# ================================
add_library(ecsnet_shared SHARED ${ECSNET_SOURCES})
target_include_directories(ecsnet_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (WIN32)
    target_link_libraries(ecsnet_shared PUBLIC ws2_32)
endif()

target_compile_definitions(ecsnet_shared PRIVATE ECSNET_EXPORTS)

set_target_properties(ecsnet_shared PROPERTIES
        OUTPUT_NAME "ecsnet"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/shared"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/shared"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/shared"
)

# ================================
# Optional: Install headers + libs
# ================================
install(DIRECTORY include/ DESTINATION include)
install(TARGETS ecsnet_static ecsnet_shared
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
)