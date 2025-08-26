cmake_minimum_required(VERSION 3.25)
project(Pong)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# SFML path
set(SFML_DIR "C:/SFML-2.5.1/lib/cmake/SFML")
find_package(SFML 2.5.1 COMPONENTS graphics window system REQUIRED)

# Add the static ecsnet library
add_subdirectory(lib/ecsnet ${CMAKE_BINARY_DIR}/mylib_build)

# Main executables
#add_executable(Pong
#        src/PongServer.cpp
#        src/PongServer.h)

add_executable(PongServer src/PongServer.cpp ${SOURCES_COMMON})
add_executable(PongClient src/PongClient.cpp ${SOURCES_COMMON})

# Link with SFML and ecsnet
target_link_libraries(PongServer PRIVATE sfml-graphics sfml-window sfml-system ecsnet)
target_link_libraries(PongClient PRIVATE sfml-graphics sfml-window sfml-system ecsnet)


# Include ecsnet headers
target_include_directories(PongServer PRIVATE lib/ecsnet/include)
target_include_directories(PongClient PRIVATE lib/ecsnet/include)

# ---- Copy SFML DLLs automatically ----
if (MSVC)  # Only for Visual Studio/Windows
    set(SFML_DLL_DIR "C:/SFML-2.5.1/bin")  #Local SFML DLLs path
    foreach(target PongServer PongClient)
        add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<IF:$<CONFIG:Debug>,${SFML_DLL_DIR}/sfml-graphics-d-2.dll,${SFML_DLL_DIR}/sfml-graphics-2.dll>
                $<IF:$<CONFIG:Debug>,${SFML_DLL_DIR}/sfml-window-d-2.dll,${SFML_DLL_DIR}/sfml-window-2.dll>
                $<IF:$<CONFIG:Debug>,${SFML_DLL_DIR}/sfml-system-d-2.dll,${SFML_DLL_DIR}/sfml-system-2.dll>
                $<TARGET_FILE_DIR:${target}>
        )
    endforeach()
endif ()

# ---- Copy assets automaticaly (i.e. Pong-Game.ttf) ----
set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")
foreach(target PongServer PongClient)
    add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ASSETS_DIR}/Pong-Game.ttf"
            $<TARGET_FILE_DIR:${target}>
    )
endforeach()