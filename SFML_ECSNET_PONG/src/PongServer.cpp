// server.cpp
// Cleaned & refactored for SOLID / Clean Code principles
// Date: 2025-08-22

#include <chrono>
#include <cstdint>
#include <cstring>    // memcpy
#include <iostream>
#include <random>
#include <thread>

#include <SFML/System/Clock.hpp>

#include "ecs.h"
#include "ecs_builtin.h"
#include "ecs_internal.h"
#include "network_cs.h"

#ifdef _WIN32
  #include <winsock2.h>
#endif

// --------------------------- Configuration ----------------------------------

namespace config {
constexpr float kWindowWidth        = 800.0f;
constexpr float kWindowHeight       = 600.0f;

constexpr int   kNumBalls           = 150;
constexpr float kTopSpawnY          = 5.0f;    // slightly inside the screen
constexpr float kWrapResetY         = -5.0f;   // just above the screen

constexpr float kSpawnMarginX       = 10.0f;
constexpr float kMinVelY            = 60.0f;
constexpr float kMaxVelY            = 180.0f;
constexpr float kMinVelX            = -20.0f;
constexpr float kMaxVelX            = 20.0f;

constexpr float kWrapPadding        = 5.0f;

constexpr char  kBindIp[]           = "127.0.0.1";
constexpr uint16_t kTcpPort         = 51660;
constexpr uint16_t kUdpPort         = 51660;

constexpr int   kFrameMs            = 16;      // ~60 FPS
} // namespace config

// --------------------------- RAII Utilities ---------------------------------

#ifdef _WIN32
/// RAII for Winsock initialization/cleanup (Single Responsibility)
class WinSockInit {
public:
    WinSockInit() {
        WSADATA wsaData{};
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "[Server] WSAStartup failed.\n";
        }
    }
    ~WinSockInit() { WSACleanup(); }
};
#endif

// ------------------------------ Abstractions --------------------------------

/// Wraps ECS + networking pointers the server needs to operate.
/// (Interface Segregation: only what we need)
struct ServerContext {
    ecs_t* ecs {nullptr};
    network_architecture_t* arch {nullptr};
};

/// Responsible for serializing and sending full entity state to a peer.
/// (Single Responsibility, Open/Closed: logic is encapsulated and reusable)
class NetworkSyncService {
public:
    explicit NetworkSyncService(ServerContext& ctx) : ctx_(ctx) {}

    void sendFullStateToPeer(peer_t* peer) const {
        if (!ctx_.ecs || !ctx_.arch || !peer) return;

        for (entity_t e = 0; e < ctx_.ecs->registered_entities_count; ++e) {
            uint8_t  syncData[MAX_PACKET_SIZE];
            size_t   syncSize = 0;
            bool     hasData  = false;

            for (component_t c = 0; c < ctx_.ecs->registered_component_count; ++c) {
                if (!ecs_has_component(ctx_.ecs, e, c)) continue;

                const void* componentData = ecs_get_component(ctx_.ecs, e, c);
                const size_t componentSize = ctx_.ecs->components[c].descriptor.size;

                if (syncSize + sizeof(component_t) + componentSize > MAX_PACKET_SIZE) {
                    // Avoid truncation: stop packing further components for this entity
                    break;
                }

                std::memcpy(syncData + syncSize, &c, sizeof(component_t));
                syncSize += sizeof(component_t);

                std::memcpy(syncData + syncSize, componentData, componentSize);
                syncSize += componentSize;

                hasData = true;
            }

            if (hasData) {
                protocol_handler_t handler;
                protocol_handler_init(&handler);
                protocol_handler_pack_entity_update(&handler, e, syncData, syncSize);
                auto* cs = (network_cs_t*)ctx_.arch->impl;
                protocol_handler_send_packet(&cs->connection_manager, peer->id, &handler);
            }
        }
    }

private:
    ServerContext& ctx_;
};

/// Spawns initial balls with randomized positions/velocities.
/// (Single Responsibility)
class BallSpawner {
public:
    explicit BallSpawner(ecs_t& ecs) : ecs_(ecs), rng_(std::random_device{}()) {}

    void spawnInitialBalls(int count) {
        std::uniform_real_distribution<float> distX(
            config::kSpawnMarginX, config::kWindowWidth - config::kSpawnMarginX);
        std::uniform_real_distribution<float> distVy(config::kMinVelY, config::kMaxVelY);
        std::uniform_real_distribution<float> distVx(config::kMinVelX, config::kMaxVelX);

        for (int i = 0; i < count; ++i) {
            position_t pos{distX(rng_), config::kTopSpawnY};
            velocity_t vel{distVx(rng_), distVy(rng_)};

            const entity_t ball = ecs_create_entity(&ecs_);
            ecs_add_component(&ecs_, ball, COMPONENT_POSITION, &pos);
            ecs_add_component(&ecs_, ball, COMPONENT_VELOCITY, &vel);
        }
    }

private:
    ecs_t& ecs_;
    std::mt19937 rng_;
};

/// Applies vertical wrapping and horizontal side wrapping.
/// (Single Responsibility, small & testable)
class WrapSystem {
public:
    explicit WrapSystem(ecs_t& ecs) : ecs_(ecs) {}

    void update() const {
        for (entity_t e = 0; e < ecs_.registered_entities_count; ++e) {
            if (!ecs_has_component(&ecs_, e, COMPONENT_POSITION)) continue;

            auto* pos = static_cast<position_t*>(ecs_get_component(&ecs_, e, COMPONENT_POSITION));
            if (!pos) continue;

            // Horizontal wrap (optional)
            if (pos->x < -config::kWrapPadding) {
                pos->x = config::kWindowWidth + config::kWrapPadding;
                ecs_mark_component_dirty(&ecs_, e, COMPONENT_POSITION);
            } else if (pos->x > config::kWindowWidth + config::kWrapPadding) {
                pos->x = -config::kWrapPadding;
                ecs_mark_component_dirty(&ecs_, e, COMPONENT_POSITION);
            }

            // Vertical wrap
            if (pos->y > config::kWindowHeight) {
                pos->y = config::kWrapResetY;
                ecs_mark_component_dirty(&ecs_, e, COMPONENT_POSITION);
            }
        }
    }

private:
    ecs_t& ecs_;
};

// ------------------------------ Game Server ---------------------------------

/// Orchestrates setup, main loop, and network callbacks.
/// (Dependency Inversion: depends on abstractions: ECS, Network)
class GameServer {
public:
    GameServer()
        : syncService_(ctx_) {
        ctx_.ecs = &ecs_;
    }

    // Set up ECS, spawn entities, init networking, run loop.
    int run() {
        // Initialize ECS
        ecs_init(&ecs_);

        // Spawn initial entities
        BallSpawner spawner(ecs_);
        spawner.spawnInitialBalls(config::kNumBalls);

        // Initialize networking architecture
        if (!initNetwork()) {
            std::cerr << "[Server] Failed to initialize networking.\n";
            return 1;
        }

        // Main loop
        sf::Clock clock;
        WrapSystem wrap(ecs_);

        while (true) {
            const float dt = clock.restart().asSeconds();

            ecs_update(&ecs_, dt);                     // physics/systems registered elsewhere
            wrap.update();                             // wrapping system
            network_architecture_update(ctx_.arch, dt); // networking pump

            std::this_thread::sleep_for(std::chrono::milliseconds(config::kFrameMs));
        }

        // Unreachable, but kept for symmetry/clarity
        // destroyNetwork();
        // return 0;
    }

private:
    static GameServer* s_instance;

    static void onPeerConnected(void* /*user_data*/, peer_t* peer) {
        if (!s_instance || !peer) return;
        std::printf("[Server] Peer %s connected. Waiting for library snapshot/ACK.\n", peer->id);
    }

    static void onPeerDisconnected(void* /*user_data*/, peer_t* peer) {
        if (!peer) return;
        std::printf("[Server] Peer %s disconnected.\n", peer->id);
    }
    static void OnClientInputReceived(void* user_data, peer_t* from, entity_t eid, uint8_t cmd,const void* extra, uint16_t extra_len)
    {
        auto* self = static_cast<GameServer*>(user_data);
        if (!self) return;
     self->HandleInputReceived(from, eid, cmd, extra, extra_len);
    }
    void HandleInputReceived(peer_t* from, entity_t eid, uint8_t cmd, const void* extra, uint16_t extra_len) {
        printf("[Server] Received input from %s -> e=%u cmd=%u\n" ,from,eid, cmd);
        if (cmd == INPUT_SPAWN) {
            if (extra_len < sizeof(float)*2) return;
            float x, y;
            memcpy(&x, extra, sizeof(float));
            memcpy(&y, (const uint8_t*)extra + sizeof(float), sizeof(float));

            position_t pos = { x, y };
            velocity_t vel = { 0.f, 120.f };
            entity_t e = ecs_create_entity(&ecs_);
            ecs_add_component(&ecs_, e, COMPONENT_POSITION, &pos);
            ecs_add_component(&ecs_, e, COMPONENT_VELOCITY, &vel);
            ecs_mark_component_dirty(&ecs_, e, COMPONENT_POSITION);
            ecs_mark_component_dirty(&ecs_, e, COMPONENT_VELOCITY);

            printf("[Server] SPAWN from %s -> e=%u (%.1f, %.1f)\n",
                   from ? from->id : "(null)", e, x, y);
        }
    }
    bool initNetwork() {
        network_architecture_config_t server_config{};
        server_config.type               = ARCH_CLIENT_SERVER;
        server_config.ip_address         = const_cast<char*>(config::kBindIp);
        server_config.is_server          = true;
        server_config.tcp_port           = config::kTcpPort;
        server_config.udp_port           = config::kUdpPort;
        server_config.on_peer_connected  = &GameServer::onPeerConnected;
        server_config.on_peer_disconnected = &GameServer::onPeerDisconnected;
        server_config.on_client_input = &GameServer::OnClientInputReceived;
        server_config.user_data            = this;
        network_architecture_t* arch = nullptr;
        server_config.ecs_sync_hz = 20.0f; // por ejemplo

        network_architecture_init(&arch, &server_config, &ecs_);
        ctx_.arch = arch;

        // Register global instance for callbacks (trampoline)
        s_instance = this;

        return ctx_.arch != nullptr;
    }

    void destroyNetwork() {
        if (ctx_.arch) {
            network_architecture_destroy(ctx_.arch);
            ctx_.arch = nullptr;
        }
    }

private:
    ecs_t ecs_{};
    ServerContext ctx_{};
    NetworkSyncService syncService_;
};

GameServer* GameServer::s_instance = nullptr;

// ---------------------------------- main ------------------------------------

int main() {
#ifdef _WIN32
    WinSockInit wsa; // RAII
#endif

    GameServer server;
    return server.run();
}
