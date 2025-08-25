// PongServer_updated.cpp
// Updated server for improved ECSNet library
// Date: 2025-08-24

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <random>
#include <thread>

#include <SFML/System/Clock.hpp>

#include "ecs.h"
#include "ecs_builtin.h"
// Include internal definitions to allow stack allocation of ecs_t
#include "ecs_internal.h"
// Network architecture and specific client-server implementation
#include "network_architecture.h"
#include "network_cs.h"
// Socket abstraction
#include "net_socket.h"

// --------------------------- Configuration ----------------------------------

namespace config {
constexpr float    kWindowWidth        = 800.0f;
constexpr float    kWindowHeight       = 600.0f;
constexpr int      kNumBalls           = 10;
constexpr float    kTopSpawnY          = 5.0f;
constexpr float    kWrapResetY         = -5.0f;
constexpr float    kSpawnMarginX       = 10.0f;
constexpr float    kMinVelY            = 60.0f;
constexpr float    kMaxVelY            = 180.0f;
constexpr float    kMinVelX            = -20.0f;
constexpr float    kMaxVelX            = 20.0f;
constexpr float    kWrapPadding        = 5.0f;
constexpr char     kBindIp[]           = "127.0.0.1";
constexpr uint16_t kTcpPort            = 51660;
constexpr uint16_t kUdpPort            = 51660;
constexpr int      kFrameMs            = 16;     // ~60 FPS
// Input command mask for spawning (reuse macro defined in protocol_handler.h)
constexpr uint8_t kInputSpawnCmd = INPUT_SPAWN;
} // namespace config

// ------------------------------ Abstractions --------------------------------

/// Wraps ECS + networking pointers the server needs to operate.
struct ServerContext {
    ecs_t* ecs{nullptr};
    network_architecture_t* arch{nullptr};
};

/// Responsible for serializing and sending full entity state to a peer.
class NetworkSyncService {
public:
    explicit NetworkSyncService(ServerContext& ctx) : ctx_(ctx) {}
    void sendFullStateToPeer(peer_t* peer) const {
        if (!ctx_.ecs || !ctx_.arch || !peer) return;
        for (entity_t e = 0; e < ctx_.ecs->registered_entities_count; ++e) {
            uint8_t syncData[MAX_PACKET_SIZE];
            size_t  syncSize = 0;
            bool    hasData  = false;
            for (component_t c = 0; c < ctx_.ecs->registered_component_count; ++c) {
                if (!ecs_has_component(ctx_.ecs, e, c)) continue;
                const void* componentData = ecs_get_component(ctx_.ecs, e, c);
                size_t componentSize = ctx_.ecs->components[c].descriptor.size;
                if (syncSize + sizeof(component_t) + componentSize > MAX_PACKET_SIZE) break;
                std::memcpy(syncData + syncSize, &c, sizeof(component_t));
                syncSize += sizeof(component_t);
                std::memcpy(syncData + syncSize, componentData, componentSize);
                syncSize += componentSize;
                hasData = true;
            }
            if (hasData) {
                protocol_handler_t handler;
                protocol_handler_init(&handler);
                protocol_handler_pack_entity_update(&handler, e, syncData, static_cast<uint16_t>(syncSize));
                auto* cs = static_cast<network_cs_t*>(ctx_.arch->impl);
                protocol_handler_send_packet(&cs->connection_manager, peer->id, &handler);
            }
        }
    }
private:
    ServerContext& ctx_;
};

/// Spawns initial balls with randomized positions/velocities.
class BallSpawner {
public:
    BallSpawner(ecs_t& ecs, network_cs_t* net) : ecs_(ecs), net_(net), rng_(std::random_device{}()) {}
    void spawnInitialBalls(int count) {
        std::uniform_real_distribution<float> distX(config::kSpawnMarginX, config::kWindowWidth - config::kSpawnMarginX);
        std::uniform_real_distribution<float> distVy(config::kMinVelY, config::kMaxVelY);
        std::uniform_real_distribution<float> distVx(config::kMinVelX, config::kMaxVelX);
        for (int i = 0; i < count; ++i) {
            position_t pos{distX(rng_), config::kTopSpawnY};
            velocity_t vel{distVx(rng_), distVy(rng_)};
            entity_t ball = ecs_create_entity(&ecs_);
            ecs_add_component(&ecs_, ball, COMPONENT_POSITION, &pos);
            ecs_add_component(&ecs_, ball, COMPONENT_VELOCITY, &vel);
            // Assign a unique network ID and default interest group (bit 0) if networking is available
            if (net_) {
                network_cs_assign_network_id(net_, ball, 1);
            }
        }
    }
private:
    ecs_t& ecs_;
    network_cs_t* net_;
    std::mt19937 rng_;
};

/// Applies vertical and horizontal wrapping.
class WrapSystem {
public:
    explicit WrapSystem(ecs_t& ecs) : ecs_(ecs) {}
    void update() const {
        for (entity_t e = 0; e < ecs_.registered_entities_count; ++e) {
            if (!ecs_has_component(&ecs_, e, COMPONENT_POSITION)) continue;
            auto* pos = static_cast<position_t*>(ecs_get_component(&ecs_, e, COMPONENT_POSITION));
            if (!pos) continue;
            // Horizontal wrap
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

class GameServer {
public:
    GameServer() : syncService_(ctx_) { ctx_.ecs = &ecs_; }

    int run() {
        // Initialize socket subsystem
        net_socket_init();
        // Initialize ECS
        ecs_init(&ecs_);
        // Spawn initial entities
        // Spawn a set of initial balls.  We pass nullptr for the
        // networking pointer here because the network architecture
        // has not yet been initialised.  After network initialisation
        // completes we will assign network IDs to all existing
        // entities.
        BallSpawner spawner(ecs_, /*net=*/nullptr);
        spawner.spawnInitialBalls(config::kNumBalls);
        // Initialize networking architecture
        if (!initNetwork()) {
            std::cerr << "[Server] Failed to initialize networking.\n";
            net_socket_cleanup();
            return 1;
        }
        // After networking is ready, assign a network ID and interest mask
        // to every pre-existing entity so that they replicate to clients.  The
        // default group mask is bit 0 (value 1).  We only assign if the
        // entity does not already have a NetworkedEntity component.
        if (ctx_.arch && ctx_.arch->impl) {
            network_cs_t* net = static_cast<network_cs_t*>(ctx_.arch->impl);
            for (entity_t e = 0; e < ecs_.entity_capacity; ++e) {
                if (!ecs_.entities[e].in_use) continue;
                if (!ecs_has_component(&ecs_, e, COMPONENT_NETWORKED_ENTITY)) {
                    network_cs_assign_network_id(net, e, 1);
                }
            }
        }
        // Main loop
        sf::Clock clock;
        WrapSystem wrap(ecs_);
        while (true) {
            const float dt = clock.restart().asSeconds();
            ecs_update(&ecs_, dt);
            wrap.update();
            if (ctx_.arch) network_architecture_update(ctx_.arch, dt);
            std::this_thread::sleep_for(std::chrono::milliseconds(config::kFrameMs));
        }
        // unreachable
    }

private:
    static GameServer* s_instance;
    static void onPeerConnected(void* /*user_data*/, peer_t* peer) {
        if (!s_instance || !peer) return;
        std::printf("[Server] Peer %s connected. Waiting for library snapshot/ACK.\n", peer->id);
        // Optionally send full state here
    }
    static void onPeerDisconnected(void* /*user_data*/, peer_t* peer) {
        if (!peer) return;
        std::printf("[Server] Peer %s disconnected.\n", peer->id);
    }
    static void OnClientInputReceived(void* user_data, peer_t* from, entity_t eid, uint8_t cmd, const void* extra, uint16_t extra_len) {
        auto* self = static_cast<GameServer*>(user_data);
        if (!self) return;
        self->HandleInputReceived(from, eid, cmd, extra, extra_len);
    }
    void HandleInputReceived(peer_t* from, entity_t /*eid*/, uint8_t cmd, const void* extra, uint16_t extra_len) {
        std::printf("[Server] Received input from %s -> cmd=%u\n", from ? from->id : "(null)", cmd);
        if (cmd == config::kInputSpawnCmd) {
            if (extra_len < sizeof(float) * 2) return;
            float x, y;
            std::memcpy(&x, extra, sizeof(float));
            std::memcpy(&y, static_cast<const uint8_t*>(extra) + sizeof(float), sizeof(float));
            position_t pos{ x, y };
            velocity_t vel{ 0.f, 120.f };
            entity_t e = ecs_create_entity(&ecs_);
            ecs_add_component(&ecs_, e, COMPONENT_POSITION, &pos);
            ecs_add_component(&ecs_, e, COMPONENT_VELOCITY, &vel);
            ecs_mark_component_dirty(&ecs_, e, COMPONENT_POSITION);
            ecs_mark_component_dirty(&ecs_, e, COMPONENT_VELOCITY);
            // If networking is active assign a network ID and default
            // interest mask (bit 0).  Without this, the new entity would
            // never be replicated to clients.

            if (ctx_.arch && ctx_.arch->impl) {
                auto* net = static_cast<network_cs_t*>(ctx_.arch->impl);
                network_cs_assign_network_id(net, e, 1);
            }
            std::printf("[Server] SPAWN from %s -> e=%u (%.1f, %.1f)\n", from ? from->id : "(null)", e, x, y);
        }
    }
    bool initNetwork() {
        network_architecture_config_t config_server{};
        config_server.type                 = ARCH_CLIENT_SERVER;
        config_server.ip_address           = config::kBindIp;
        config_server.is_server            = true;
        config_server.tcp_port             = config::kTcpPort;
        config_server.udp_port             = config::kUdpPort;
        config_server.on_peer_connected    = &GameServer::onPeerConnected;
        config_server.on_peer_disconnected = &GameServer::onPeerDisconnected;
        config_server.on_client_input      = &GameServer::OnClientInputReceived;
        config_server.user_data            = this;
        config_server.ecs_sync_hz          = 128.0f;
        network_architecture_init(&ctx_.arch, &config_server, &ecs_);
        s_instance = this;
        return ctx_.arch != nullptr;
    }
    ecs_t ecs_{};
    ServerContext ctx_{};
    NetworkSyncService syncService_;
};

GameServer* GameServer::s_instance = nullptr;

// ---------------------------------- main ------------------------------------

int main() {
    GameServer server;
    return server.run();
}