#pragma once

/**
 * @file ecs_config.h
 * @brief Configuration constants for the ECS framework and related subsystems.
 *
 * This header defines compile-time limits and settings for:
 * - ECS entity, component, and system counts.
 * - Networking parameters such as buffer sizes and socket types.
 * - Platform-specific configuration macros.
 */

/**
 * @brief Definition for the ECSNET_API macro, which determines the visibility of the ECSNET library.
 */
// #if defined(_WIN32) || defined(_WIN64)
//   #ifdef BUILD_DLL
//     #define ECSNET_API __declspec(dllexport)
//   #else
//     #define ECSNET_API __declspec(dllimport)
//   #endif
// #else
//   #define ECSNET_API __attribute__((visibility("default")))
// #endif
//
#define ECSNET_API
//
// #if defined(_WIN32)
//   #if defined(ECSNET_EXPORTS)
//     #define ECSNET_API __declspec(dllexport)
//   #else
//     #define ECSNET_API __declspec(dllimport)
//   #endif
// #else
//   #define ECSNET_API
// #endif
/* ========================= ECS CONFIGURATION ========================= */

/**
 * @brief Maximum number of entities allowed in the ECS world.
 */
#define MAX_ENTITIES 1024

/**
 * @brief Maximum number of different component types that can be registered.
 */
#define MAX_COMPONENTS 32

/**
 * @brief Maximum number of systems that can be registered and run.
 */
#define MAX_SYSTEMS 64

/* ========================= NETWORKING CONFIGURATION ========================= */

/**
 * @brief Size (in bytes) of the buffer used for network communication.
 */
#define NETWORK_BUFFER_SIZE 4096

/**
 * @brief Default socket type for network connections.
 * @details 1 = TCP, 2 = UDP
 */
#define DEFAULT_SOCKET_TYPE 1

/**
 * @brief Timeout for network operations in milliseconds.
 */
#define NETWORK_TIMEOUT_MS 3000

/* ========================= PLATFORM CONFIGURATION ========================= */

/**
 * @brief Name of the target platform as a string.
 *
 * This is determined at compile time.
 * - "Windows" for _WIN32
 * - "Unix" for all other platforms
 */
#ifdef _WIN32
#define PLATFORM_NAME "Windows"
#else
#define PLATFORM_NAME "Unix"
#endif
