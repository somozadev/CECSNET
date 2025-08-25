// PongClient_updated.cpp
// Updated client for improved ECSNet library
// Date: 2025-08-24

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>

#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>

#include "ecs.h"
#include "ecs_builtin.h"
// Include internal definitions to allow stack allocation of ecs_t
#include "ecs_internal.h"
// Network architecture and connection implementation
#include "network_architecture.h"
#include "network_cs.h"
// Socket abstraction
#include "net_socket.h"

// --------------------------- Configuration ----------------------------------

namespace config {
constexpr unsigned kHandlerPoolSize    = 64;
constexpr float    kPaddleWidth          = 10.0f;
constexpr float    kPaddleHeight         = 30.0f;
constexpr float    kBallRadius         = 3.0f;
constexpr unsigned kWindowWidth        = 800;
constexpr unsigned kWindowHeight       = 600;
constexpr char     kWindowTitle[]      = "Pong ECSNet Client";
constexpr char     kServerIp[]         = "127.0.0.1";
constexpr uint16_t kTcpPort            = 51660;
constexpr uint16_t kUdpPort            = 51660;
constexpr unsigned kSpawnRateLimitMs   = 30;   // anti‑spam
constexpr unsigned kSleepMs            = 16;   // ~60 fps
// Input flags (bitfield)
constexpr std::uint8_t kInputUp    = INPUT_UP;
constexpr std::uint8_t kInputDown  = INPUT_DOWN;
constexpr std::uint8_t kInputSpawn = 0x80;


enum entity_kind_e : uint8_t {
    ENTITY_KIND_BALL = 0,
    ENTITY_KIND_PADDLE = 1
};

typedef struct {
    uint8_t kind;
} entity_kind_t;

component_t COMPONENT_ENTITY_KIND = (component_t) -1;

void serialize_entity_kind(const void *data, uint8_t *out) {
    const entity_kind_t *kind = (const entity_kind_t *) data;
    out[0] = kind->kind;
}

void deserialize_entity_kind(const uint8_t *in, void *data) {
    entity_kind_t *kind = (entity_kind_t *) data;
    kind->kind = in[0];
}

} // namespace config

// --------------------------- Networking Services ----------------------------

/// Simple static pool for protocol handlers (avoids allocations).
class HandlerPool {
public:
    HandlerPool() : next_(0) {
        // lazily init per allocation
    }

    protocol_handler_t* alloc() {
        protocol_handler_t* h = &pool_[next_++ & (config::kHandlerPoolSize - 1)];
        protocol_handler_init(h);
        return h;
    }

private:
    protocol_handler_t pool_[config::kHandlerPoolSize]{};
    unsigned next_;
};

/// Thin façade to send packets through the architecture.
class NetSender {
public:
    explicit NetSender(network_architecture_t* arch) : arch_(arch) {}

    void sendToPeer(const char* peerId, protocol_handler_t* handler) const {
        if (!arch_ || !arch_->impl || !peerId) return;
        auto* cs = static_cast<network_cs_t*>(arch_->impl);
        protocol_handler_send_packet(&cs->connection_manager, peerId, handler);
    }

private:
    network_architecture_t* arch_{nullptr};
};

// ------------------------------- Client App ---------------------------------

/// Holds everything the client needs and wires callbacks.
class ClientApp {
public:
    ClientApp()
        : window_(sf::VideoMode(config::kWindowWidth, config::kWindowHeight), config::kWindowTitle),
          paddleShape_(sf::Vector2f(config::kPaddleWidth, config::kPaddleHeight)),
        ballShape_(config::kBallRadius){
        window_.setVerticalSyncEnabled(true);
        paddleShape_.setFillColor(sf::Color(0, 220, 255));
        ballShape_.setFillColor(sf::Color(255, 255, 255));
    }

    int run() {
        // Initialize socket subsystem
        net_socket_init();

        // Initialize ECS
        ecs_init(&ecs_);

        // Initialize networking
        if (!initNetwork()) {
            std::cerr << "[Client] Failed to initialize networking.\n";
            net_socket_cleanup();
            return 1;
        }

        // Main loop
        sf::Clock frameClock;
        while (window_.isOpen()) {
            const float dt = frameClock.restart().asSeconds();
            handleWindowEvents();
            if (netArch_) network_architecture_update(netArch_, dt);
            renderFrame();
            sf::sleep(sf::milliseconds(config::kSleepMs));
        }

        shutdownNetwork();
        net_socket_cleanup();
        return 0;
    }

private:
    // ------------------------- Packet Handling ------------------------------

    static void onPacketReceived(void* user_data, peer_t* peer, const void* data, int len) {
        auto* self = static_cast<ClientApp*>(user_data);
        if (!self) return;
        self->handlePacket(peer, data, len);
    }

    void handlePacket(peer_t* peer, const void* data, int len) {
        if (!data || len < static_cast<int>(sizeof(packet_header_t))) return;

        // Cache server peer ID on first packet
        if (!serverPeerId_ && peer && peer->id) {
            serverPeerId_ = peer->id;
            std::printf("[Client] Server peer id cached (on_packet): %s\n", serverPeerId_);
        }

        const auto* packet = static_cast<const network_packet_t*>(data);
        if (packet->header.size > static_cast<uint16_t>(len)) return; // truncated

        switch (packet->header.type) {
            case PACKET_TYPE_MULTI_ENTITY_UPDATE:
                parseMultiEntityUpdate(packet);
                break;
            case PACKET_TYPE_ENTITY_UPDATE:
                parseEntityUpdate(packet);
                break;
            default:
                break;
        }
    }

    bool readComponentId(const std::uint8_t*& cur, const std::uint8_t* end, component_t& outCid) {
        if (end - cur < static_cast<ptrdiff_t>(sizeof(component_t))) return false;
        std::memcpy(&outCid, cur, sizeof(component_t));
        cur += sizeof(component_t);
        if (outCid >= ecs_.registered_component_count) {
            std::printf("[Client] Skipping invalid component id %u\n", static_cast<unsigned>(outCid));
            return false;
        }
        return true;
    }

    void parseMultiEntityUpdate(const network_packet_t* packet) {
        const std::uint8_t* cur = packet->data;
        const std::uint8_t* end = reinterpret_cast<const std::uint8_t*>(packet) + packet->header.size;

        if (end - cur < static_cast<ptrdiff_t>(sizeof(std::uint16_t))) return;
        std::uint16_t entityCount = 0;
        std::memcpy(&entityCount, cur, sizeof(entityCount));
        cur += sizeof(entityCount);

        for (std::uint16_t i = 0; i < entityCount; ++i) {
            if (end - cur < static_cast<ptrdiff_t>(sizeof(entity_t))) break;
            entity_t eid = 0;
            std::memcpy(&eid, cur, sizeof(eid));
            cur += sizeof(eid);
            ecs_try_create_entity_by_id(&ecs_, eid);

            if (end - cur < static_cast<ptrdiff_t>(sizeof(std::uint8_t))) break;
            std::uint8_t compCount = 0;
            std::memcpy(&compCount, cur, sizeof(compCount));
            cur += sizeof(compCount);

            for (std::uint8_t c = 0; c < compCount; ++c) {
                component_t cid{};
                if (!readComponentId(cur, end, cid)) break;
                size_t compSize = ecs_.components[cid].descriptor.size;
                if (end - cur < static_cast<ptrdiff_t>(compSize)) return; // truncated

                if (!ecs_has_component(&ecs_, eid, cid)) {
                    ecs_add_component(&ecs_, eid, cid, const_cast<std::uint8_t*>(cur));
                } else {
                    void* dst = ecs_get_component(&ecs_, eid, cid);
                    if (dst) std::memcpy(dst, cur, compSize);
                }
                ecs_mark_component_dirty(&ecs_, eid, cid);
                cur += compSize;
            }
        }
    }

    void parseEntityUpdate(const network_packet_t* packet) {
        const std::uint8_t* cur = packet->data;
        const std::uint8_t* end = reinterpret_cast<const std::uint8_t*>(packet) + packet->header.size;
        if (end - cur < static_cast<ptrdiff_t>(sizeof(entity_t))) return;
        entity_t eid{};
        std::memcpy(&eid, cur, sizeof(eid));
        cur += sizeof(eid);
        ecs_try_create_entity_by_id(&ecs_, eid);

        while (end - cur >= static_cast<ptrdiff_t>(sizeof(component_t))) {
            component_t cid{};
            if (!readComponentId(cur, end, cid)) break;
            size_t compSize = ecs_.components[cid].descriptor.size;
            if (end - cur < static_cast<ptrdiff_t>(compSize)) break;
            if (!ecs_has_component(&ecs_, eid, cid)) {
                ecs_add_component(&ecs_, eid, cid, const_cast<std::uint8_t*>(cur));
            } else {
                void* dst = ecs_get_component(&ecs_, eid, cid);
                if (dst) std::memcpy(dst, cur, compSize);
            }
            ecs_mark_component_dirty(&ecs_, eid, cid);
            cur += compSize;
        }
    }

    // --------------------------- Window & Input -----------------------------

    void handleWindowEvents() {
        sf::Event event{};
        while (window_.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window_.close();
                return;
            }
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                handleSpawnAtClick(event.mouseButton.x, event.mouseButton.y);
            }
            if (event.type == sf::Event::KeyPressed && (event.key.code == sf::Keyboard::W || event.key.code == sf::Keyboard::S || event.key.code == sf::Keyboard::Up || event.key.code == sf::Keyboard::Down)) {
                handleMovePaddle(event.key.code);
                return;
            }
        }
    }
    void handleMovePaddle(sf::Keyboard::Key key) {
        if (!serverPeerId_) {
            std::printf("[Client] No server peer id yet; cannot send SPAWN.\n");
            return;
                  }
        protocol_handler_t* h = handlerPool_.alloc();

        if (key == sf::Keyboard::W || key == sf::Keyboard::Up) {
        protocol_handler_pack_client_input(h, 0, config::kInputUp, nullptr, 0);

        }
        else {
        protocol_handler_pack_client_input(h, 0, config::kInputDown, nullptr, 0);
        }
        NetSender sender(netArch_);
        sender.sendToPeer(serverPeerId_, h);
        std::printf("[Client] Paddle MOVED.\n");

    }

    void handleSpawnAtClick(int mouseX, int mouseY) {
        if (!serverPeerId_) {
            std::printf("[Client] No server peer id yet; cannot send SPAWN.\n");
            return;
        }
        sf::Vector2f world = window_.mapPixelToCoords(sf::Vector2i(mouseX, mouseY));
        struct SpawnXY { float x; float y; } xy{world.x, world.y};
        if (spawnRateClock_.getElapsedTime().asMilliseconds() < config::kSpawnRateLimitMs) return;
        spawnRateClock_.restart();
        protocol_handler_t* h = handlerPool_.alloc();
        protocol_handler_pack_client_input(h, 0, config::kInputSpawn, &xy, sizeof(xy));
        NetSender sender(netArch_);
        sender.sendToPeer(serverPeerId_, h);
        std::printf("[Client] Sent SPAWN at (%.1f, %.1f)\n", xy.x, xy.y);
    }

    // ------------------------------- Render ---------------------------------

    void renderFrame() {
        window_.clear(sf::Color::Black);
        // Draw all entities that have a Position component
        for (entity_t e = 0; e < ecs_.entity_capacity; ++e) {
            if (!ecs_.entities[e].in_use) continue;
            if (!ecs_has_component(&ecs_, e, COMPONENT_POSITION)) continue;
            auto* pos = static_cast<position_t*>(ecs_get_component(&ecs_, e, COMPONENT_POSITION));
            if (!pos) continue;
            if (ecs_has_component(&ecs_, e, config::COMPONENT_ENTITY_KIND)) {
                auto* kind = (config::entity_kind_t*)ecs_get_component(&ecs_, e, config::COMPONENT_ENTITY_KIND);
                if (kind->kind == config::ENTITY_KIND_BALL) {
                    // renderizar bola

                    ballShape_.setPosition(pos->x, pos->y);
                    window_.draw(ballShape_);

                } else if (kind->kind == config::ENTITY_KIND_PADDLE) {
                    // renderizar pala
                    paddleShape_.setPosition(pos->x, pos->y);
                    window_.draw(paddleShape_);
                }
            }

        }
        window_.display();
    }

    // ------------------------------ Networking ------------------------------

    bool initNetwork() {
        network_architecture_config_t cfg{};
        cfg.type               = ARCH_CLIENT_SERVER;
        cfg.ip_address         = config::kServerIp;
        cfg.is_server          = false;
        cfg.tcp_port           = config::kTcpPort;
        cfg.udp_port           = config::kUdpPort;
        cfg.on_packet_received = &ClientApp::onPacketReceived;
        cfg.user_data          = this;
        cfg.ecs_sync_hz        = 60.0f;
        network_architecture_init(&netArch_, &cfg, &ecs_);

        component_descriptor_t desc;
        desc.name = "EntityKind";
        desc.size = sizeof(config::entity_kind_t);
        desc.serialize = config::serialize_entity_kind;
        desc.deserialize = config::deserialize_entity_kind;

        config::COMPONENT_ENTITY_KIND = ecs_register_component(&ecs_, desc);

        return netArch_ != nullptr;
    }

    void shutdownNetwork() {
        if (netArch_) {
            network_architecture_destroy(netArch_);
            netArch_ = nullptr;
        }
    }

private:
    ecs_t ecs_{};
    network_architecture_t* netArch_{nullptr};
    const char* serverPeerId_{nullptr};
    HandlerPool handlerPool_{};
    sf::RenderWindow window_;
    sf::RectangleShape paddleShape_;
    sf::CircleShape ballShape_;
    sf::Clock spawnRateClock_;
};

// ---------------------------------- main ------------------------------------

int main() {
    ClientApp app;
    return app.run();
}