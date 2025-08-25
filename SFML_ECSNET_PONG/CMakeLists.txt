cmake_minimum_required(VERSION 3.25)
project(Pong)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ruta a SFML
set(SFML_DIR "C:/SFML-2.5.1/lib/cmake/SFML")
find_package(SFML 2.5.1 COMPONENTS graphics window system REQUIRED)

# Añadir la librería estática ecsnet
add_subdirectory(lib/ecsnet ${CMAKE_BINARY_DIR}/mylib_build)

# Ejecutable principal
#add_executable(MiPong src/main.cpp
#        src/PongServer.cpp
#        src/PongServer.h)

add_executable(PongServer src/PongServer.cpp ${SOURCES_COMMON})
add_executable(PongClient src/PongClient.cpp ${SOURCES_COMMON})

# Enlazar con SFML y ecsnet
#target_link_libraries(MiPong PRIVATE sfml-graphics sfml-window sfml-system ecsnet)
target_link_libraries(PongServer PRIVATE sfml-graphics sfml-window sfml-system ecsnet)
target_link_libraries(PongClient PRIVATE sfml-graphics sfml-window sfml-system ecsnet)


# Incluir headers de ecsnet
#target_include_directories(MiPong PRIVATE lib/ecsnet/include)
target_include_directories(PongServer PRIVATE lib/ecsnet/include)
target_include_directories(PongClient PRIVATE lib/ecsnet/include)

# ---- Copiar DLLs de SFML automáticamente ----
if (MSVC)  # Solo para Visual Studio/Windows
    set(SFML_DLL_DIR "C:/SFML-2.5.1/bin")  # Ruta donde están las DLLs de SFML
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

# ---- Copiar assets automáticamente (ej. Pong-Game.ttf) ----
set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")
foreach(target PongServer PongClient)
    add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ASSETS_DIR}/Pong-Game.ttf"
            $<TARGET_FILE_DIR:${target}>
    )
endforeach()