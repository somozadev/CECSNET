// PongServer_updated.cpp
// Updated server for improved ECSNet library
// Date: 2025-08-24

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <random>
#include <thread>
#include <unordered_map>
#include <string>

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

class GameServer;

namespace config {
    constexpr float kWindowWidth = 800.0f;
    constexpr float kWindowHeight = 600.0f;
    constexpr float kPaddleWOne = 100.0f;
    constexpr float kPaddleWTwo = 700.0f;
    int kActivePaddlesCount = 0;
    constexpr int kNumBalls = 1;
    constexpr float kBallIncrementVelocityFactorPerCollision = 1.25f;
    constexpr float kWrapResetY = -5.0f;
    constexpr float kSpawnMarginX = 10.0f;
    constexpr float kMinVelY = -160.0f;
    constexpr float kMaxVelY = 160.0f;
    constexpr float kMinVelX = -160.0f;
    constexpr float kMaxVelX = 160.0f;
    constexpr float kWrapPadding = 5.0f;
    constexpr char kBindIp[] = "0.0.0.0";
    constexpr uint16_t kTcpPort = 51660;
    constexpr uint16_t kUdpPort = 51660;
    constexpr int kFrameMs = 16; // ~60 FPS
    // Input command mask for spawning (reuse macro defined in protocol_handler.h)
    constexpr uint8_t kInputSpawnCmd = INPUT_SPAWN;
    constexpr uint8_t kInputUpCmd = INPUT_UP;
    constexpr uint8_t kInputDownCmd = INPUT_DOWN;

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

    typedef struct {
        int left;
        int right;
    } score_t;

    component_t COMPONENT_SCORE = (component_t) -1;

    void serialize_score(const void *data, uint8_t *out) {
        const score_t *score = (const score_t *) data;
        std::memcpy(out, score, sizeof(score_t));
    }

    void deserialize_score(const uint8_t *in, void *data) {
        score_t *score = (score_t *) data;
        std::memcpy(score, in, sizeof(score_t));
    }
} // namespace config

// ------------------------------ Abstractions --------------------------------

/// Wraps ECS + networking pointers the server needs to operate.
struct ServerContext {
    ecs_t *ecs{nullptr};
    network_architecture_t *arch{nullptr};
};

/// Responsible for serializing and sending full entity state to a peer.
class NetworkSyncService {
public:
    explicit NetworkSyncService(ServerContext &ctx) : ctx_(ctx) {
    }

    void sendFullStateToPeer(peer_t *peer) const {
        if (!ctx_.ecs || !ctx_.arch || !peer) return;
        for (entity_t e = 0; e < ctx_.ecs->registered_entities_count; ++e) {
            uint8_t syncData[MAX_PACKET_SIZE];
            size_t syncSize = 0;
            bool hasData = false;
            for (component_t c = 0; c < ctx_.ecs->registered_component_count; ++c) {
                if (!ecs_has_component(ctx_.ecs, e, c)) continue;
                const void *componentData = ecs_get_component(ctx_.ecs, e, c);
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
                auto *cs = static_cast<network_cs_t *>(ctx_.arch->impl);
                protocol_handler_send_packet(&cs->connection_manager, peer->id, &handler);
            }
        }
    }

private:
    ServerContext &ctx_;
};

/// Spawns initial balls with randomized positions/velocities.
class BallSpawner {
public:
    BallSpawner(ecs_t &ecs, network_cs_t *net) : ecs_(ecs), net_(net), rng_(std::random_device{}()) {
    }

    void spawnBall() {
        position_t pos{config::kWindowWidth / 2, config::kWindowHeight / 2};
        std::uniform_real_distribution<float> distVy(config::kMinVelY, config::kMaxVelY);
        std::uniform_real_distribution<float> distVx(config::kMinVelX, config::kMaxVelX);

        float vx = 0.f;
        float vy = 0.f;

        // avoid too low speed in vx
        do {
            vx = distVx(rng_);
        } while (std::abs(vx) < 100.f);

        vy = distVy(rng_);

        velocity_t vel{vx, vy};
        entity_t ball = ecs_create_entity(&ecs_);
        ecs_add_component(&ecs_, ball, COMPONENT_POSITION, &pos);
        ecs_add_component(&ecs_, ball, COMPONENT_VELOCITY, &vel);

        config::entity_kind_t kindBall{config::ENTITY_KIND_BALL};
        ecs_add_component(&ecs_, ball, config::COMPONENT_ENTITY_KIND, &kindBall);

        // Assign a unique network ID and default interest group (bit 0) if networking is available
        if (net_) {
            network_cs_assign_network_id(net_, ball, 1);
        }
    }

private:
    ecs_t &ecs_;
    network_cs_t *net_;
    std::mt19937 rng_;
};

class BallCollisionSystem {
public:
    using ScoreCallback = void(*)(void *user, bool leftSide);

    BallCollisionSystem(ecs_t &ecs, network_cs_t *net, ScoreCallback cb, void *user)
        : ecs_(ecs), net_(net), onScoreCb_(cb), user_(user) {
    }

    void update() {
        for (entity_t e = 0; e < ecs_.entity_capacity; ++e) {
            if (!ecs_.entities[e].in_use) continue;
            if (!ecs_has_component(&ecs_, e, COMPONENT_POSITION)) continue;
            if (!ecs_has_component(&ecs_, e, COMPONENT_VELOCITY)) continue;
            if (!ecs_has_component(&ecs_, e, config::COMPONENT_ENTITY_KIND)) continue;

            auto *kind = static_cast<config::entity_kind_t *>(
                ecs_get_component(&ecs_, e, config::COMPONENT_ENTITY_KIND));
            if (kind->kind != config::ENTITY_KIND_BALL) continue;

            auto *pos = static_cast<position_t *>(ecs_get_component(&ecs_, e, COMPONENT_POSITION));
            auto *vel = static_cast<velocity_t *>(ecs_get_component(&ecs_, e, COMPONENT_VELOCITY));

            // Floor / roof bounce
            if (pos->y <= 0.f || pos->y >= config::kWindowHeight) {
                vel->y = -vel->y;
                ecs_mark_component_dirty(&ecs_, e, COMPONENT_VELOCITY);
            }

            // Paddle collision
            checkPaddleCollisions(e, pos, vel);

            // Left goal → right scores
            if (pos->x < 0.f) {
                if (onScoreCb_) onScoreCb_(user_, /*leftSide=*/false);
                destroyBall(e);
            }
            // Right goal → left scores
            else if (pos->x > config::kWindowWidth) {
                if (onScoreCb_) onScoreCb_(user_, /*leftSide=*/true);
                destroyBall(e);
            }
        }
    }

private:
    ecs_t &ecs_;
    network_cs_t *net_{};
    ScoreCallback onScoreCb_{};
    void *user_{};

    void destroyBall(entity_t ball) {
        if (net_ && ecs_has_component(&ecs_, ball, COMPONENT_NETWORKED_ENTITY)) {
            network_cs_mark_entity_destroy(net_, ball);
        }
        ecs_destroy_entity(&ecs_, ball);
        std::printf("[Server] Ball destroyed after score.\n");
    }

    static bool intersects(const position_t &ball, float ballRadius,
                           const position_t &paddle, float paddleW, float paddleH) {
        return (ball.x + ballRadius >= paddle.x &&
                ball.x - ballRadius <= paddle.x + paddleW &&
                ball.y + ballRadius >= paddle.y &&
                ball.y - ballRadius <= paddle.y + paddleH);
    }

    void checkPaddleCollisions(entity_t ball, position_t *pos, velocity_t *vel) {
        float ballRadius = 6.f;
        for (entity_t e = 0; e < ecs_.entity_capacity; ++e) {
            if (!ecs_.entities[e].in_use) continue;
            if (!ecs_has_component(&ecs_, e, COMPONENT_POSITION)) continue;
            if (!ecs_has_component(&ecs_, e, config::COMPONENT_ENTITY_KIND)) continue;

            auto *kind = static_cast<config::entity_kind_t *>(
                ecs_get_component(&ecs_, e, config::COMPONENT_ENTITY_KIND));


            // ----- COL W/ PADDLES -----
            if (kind->kind == config::ENTITY_KIND_PADDLE) {
                auto *paddlePos = static_cast<position_t *>(
                    ecs_get_component(&ecs_, e, COMPONENT_POSITION));
                float paddleW = 10.f;
                float paddleH = 50.f;

                if (intersects(*pos, ballRadius, *paddlePos, paddleW, paddleH)) {
                    vel->x = -vel->x * config::kBallIncrementVelocityFactorPerCollision;
                    ecs_mark_component_dirty(&ecs_, ball, COMPONENT_VELOCITY);
                }
            } // ----- COL W/ OTHER BALLS IN CASE WE WANT TO ADD MORE -----
            else if (kind->kind == config::ENTITY_KIND_BALL) {
                auto *otherPos = static_cast<position_t *>(
                    ecs_get_component(&ecs_, e, COMPONENT_POSITION));
                auto *otherVel = static_cast<velocity_t *>(
                    ecs_get_component(&ecs_, e, COMPONENT_VELOCITY));

                float dx = pos->x - otherPos->x;
                float dy = pos->y - otherPos->y;
                float distSq = dx * dx + dy * dy;
                float minDist = ballRadius * 2;

                if (distSq <= minDist * minDist) {
                    // Simplified elastic collision
                    std::swap(vel->x, otherVel->x);
                    std::swap(vel->y, otherVel->y);

                    ecs_mark_component_dirty(&ecs_, ball, COMPONENT_VELOCITY);
                    ecs_mark_component_dirty(&ecs_, e, COMPONENT_VELOCITY);

                    // repositioning to avoid infinite overlaps
                    float dist = std::sqrt(distSq);
                    if (dist > 0.0f) {
                        float overlap = 0.5f * (minDist - dist);
                        float nx = dx / dist;
                        float ny = dy / dist;

                        pos->x += nx * overlap;
                        pos->y += ny * overlap;
                        otherPos->x -= nx * overlap;
                        otherPos->y -= ny * overlap;

                        ecs_mark_component_dirty(&ecs_, ball, COMPONENT_POSITION);
                        ecs_mark_component_dirty(&ecs_, e, COMPONENT_POSITION);
                    }
                }
            }
        }
    }
};


class PaddleSpawner {
public:
    PaddleSpawner(ecs_t &ecs, network_cs_t *net) : ecs_(ecs), net_(net) {
    }

    void spawnNewPaddle(peer_t *peer) {
        if (!peer) return;
        std::string peerId(peer->id);

        // Don't create a paddle if peer already has one
        if (paddles_.find(peerId) != paddles_.end()) {
            std::printf("[Server] Peer %s already has an assigned paddle.\n", peer->id);
            return;
        }

        // Check for free slots (2 max)
        if (paddles_.size() >= 2) {
            std::printf("[Server] Too many active paddles! Cannot spawn another for %s\n", peer->id);
            return;
        }

        // Determine free side
        bool leftTaken = false;
        bool rightTaken = false;

        for (auto &kv: paddles_) {
            entity_t ent = kv.second;
            if (!ecs_has_component(&ecs_, ent, COMPONENT_POSITION)) continue;
            auto *pos = static_cast<position_t *>(ecs_get_component(&ecs_, ent, COMPONENT_POSITION));
            if (!pos) continue;

            if (pos->x == config::kPaddleWOne) leftTaken = true;
            if (pos->x == config::kPaddleWTwo) rightTaken = true;
        }

        entity_t paddle = ecs_create_entity(&ecs_);
        position_t pos;

        if (!leftTaken) {
            pos = {config::kPaddleWOne, config::kWindowHeight / 2.0f};
        } else {
            pos = {config::kPaddleWTwo, config::kWindowHeight / 2.0f};
        }

        ecs_add_component(&ecs_, paddle, COMPONENT_POSITION, &pos);

        config::entity_kind_t kindBall{config::ENTITY_KIND_PADDLE};
        ecs_add_component(&ecs_, paddle, config::COMPONENT_ENTITY_KIND, &kindBall);


        if (net_) {
            network_cs_assign_network_id(net_, paddle, 1);
        }

        paddles_[peerId] = paddle;

        std::printf("[Server] Spawned paddle for %s at (%.1f, %.1f)\n",
                    peer->id, pos.x, pos.y);

        if (paddles_.size() == 2) {
            BallSpawner ballSpawner(ecs_, net_);
            ballSpawner.spawnBall();
        }
    }

    void destroyPaddle(peer_t *peer) {
        if (!peer) return;
        std::string peerId(peer->id);

        auto it = paddles_.find(peerId);
        if (it == paddles_.end()) {
            std::printf("[Server] Peer %s has no paddle to destroy.\n", peer->id);
            return;
        }

        entity_t paddle = it->second;


        // Notify the network that this entity is about to be destroyed so
        // clients can remove their replicated copy.  We call this before
        // destroying the entity in the ECS so that the NetworkedEntity
        // component is still present, and we can obtain the network_id.
        if (net_) {
            network_cs_mark_entity_destroy(net_, paddle);
        }

        ecs_destroy_entity(&ecs_, paddle);
        paddles_.erase(it);

        std::printf("[Server] Destroyed paddle of peer %s\n", peer->id);
        if (paddles_.size() < 2) {
            // destroy all balls
            for (entity_t e = 0; e < ecs_.entity_capacity; ++e) {
                if (!ecs_.entities[e].in_use) continue;
                if (!ecs_has_component(&ecs_, e, config::COMPONENT_ENTITY_KIND)) continue;

                auto *kind = (config::entity_kind_t *) ecs_get_component(&ecs_, e, config::COMPONENT_ENTITY_KIND);
                if (kind->kind == config::ENTITY_KIND_BALL) {
                    network_cs_mark_entity_destroy(net_, e);
                    ecs_destroy_entity(&ecs_, e);
                    std::printf("[Server] Ball destroyed because player disconnected.\n");
                }
            }
        }
    }


    void movePaddle(peer_t *peer, uint8_t moveInput) {
        auto paddle = paddles_[peer->id];
        auto *pos = static_cast<position_t *>(ecs_get_component(&ecs_, paddle, COMPONENT_POSITION));
        if (moveInput == config::kInputUpCmd) {
            pos->y -= 20;
        } else if (moveInput == config::kInputDownCmd) {
            pos->y += 20;
        }
        ecs_mark_component_dirty(&ecs_, paddle, COMPONENT_POSITION);
    }

private:
    ecs_t &ecs_;
    network_cs_t *net_;
    std::unordered_map<std::string, entity_t> paddles_;
};

/// Applies vertical and horizontal wrapping.
class WrapSystem {
public:
    explicit WrapSystem(ecs_t &ecs) : ecs_(ecs) {
    }

    void update() const {
        for (entity_t e = 0; e < ecs_.registered_entities_count; ++e) {
            if (!ecs_has_component(&ecs_, e, COMPONENT_POSITION)) continue;
            auto *pos = static_cast<position_t *>(ecs_get_component(&ecs_, e, COMPONENT_POSITION));
            if (!pos) continue;

            // Skip entities with entity_kind BALL, since they are handled separately.
            if (ecs_has_component(&ecs_, e, config::COMPONENT_ENTITY_KIND)) {
                auto *kind = (config::entity_kind_t *) ecs_get_component(&ecs_, e, config::COMPONENT_ENTITY_KIND);
                if (kind && kind->kind == config::ENTITY_KIND_BALL) {
                    continue;
                }
            }

            // Horizontal wrap
            if (pos->x < -config::kWrapPadding) {
                pos->x = config::kWindowWidth + config::kWrapPadding;
                ecs_mark_component_dirty(&ecs_, e, COMPONENT_POSITION);
            } else if (pos->x > config::kWindowWidth + config::kWrapPadding) {
                pos->x = -config::kWrapPadding;
                ecs_mark_component_dirty(&ecs_, e, COMPONENT_POSITION);
            }

            // Vertical wrap
            if (pos->y > config::kWindowHeight + config::kWrapPadding) {
                pos->y = config::kWrapResetY; // Returns from bottom to top
                ecs_mark_component_dirty(&ecs_, e, COMPONENT_POSITION);
            } else if (pos->y < config::kWrapResetY - config::kWrapPadding) {
                pos->y = config::kWindowHeight; // Returns from top to bottom
                ecs_mark_component_dirty(&ecs_, e, COMPONENT_POSITION);
            }
        }
    }

private:
    ecs_t &ecs_;
};

// ------------------------------ Game Server ---------------------------------

class GameServer {
public:
    GameServer() : syncService_(ctx_), paddleSpawner_(nullptr) {
        ctx_.ecs = &ecs_;
    }

    int run() {
        // Initialize socket subsystem
        net_socket_init();
        // Initialize ECS
        ecs_init(&ecs_);
        // Initialize networking architecture
        if (!initNetwork()) {
            std::cerr << "[Server] Failed to initialize networking.\n";
            net_socket_cleanup();
            return 1;
        }
        if (ctx_.arch && ctx_.arch->impl) {
            auto *net = static_cast<network_cs_t *>(ctx_.arch->impl);
            paddleSpawner_ = std::make_unique<PaddleSpawner>(ecs_, net);
        }

        entity_t scoreEntity = ecs_create_entity(&ecs_);
        config::score_t scoreData{0, 0};
        ecs_add_component(&ecs_, scoreEntity, config::COMPONENT_SCORE, &scoreData);
        if (ctx_.arch && ctx_.arch->impl) {
            auto *net = static_cast<network_cs_t *>(ctx_.arch->impl);
            network_cs_assign_network_id(net, scoreEntity, 1); // Replicate to everyone
        }
        scoreEntity_ = scoreEntity;
        // After networking is ready, assign a network ID and interest mask
        // to every pre-existing entity so that they replicate to clients.  The
        // default group mask is bit 0 (value 1).  We only assign if the
        // entity does not already have a NetworkedEntity component.
        if (ctx_.arch && ctx_.arch->impl) {
            network_cs_t *net = static_cast<network_cs_t *>(ctx_.arch->impl);
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

        auto *net = static_cast<network_cs_t *>(ctx_.arch->impl);
        BallCollisionSystem ballCollision(ecs_, net, &GameServer::ScoreThunk, this);

        while (true) {
            const float dt = clock.restart().asSeconds();
            ecs_update(&ecs_, dt);
            wrap.update();
            ballCollision.update();
            if (ctx_.arch) network_architecture_update(ctx_.arch, dt);
            std::this_thread::sleep_for(std::chrono::milliseconds(config::kFrameMs));
        }
        // unreachable
    }

    void onScore(bool leftSide) {
        if (leftSide) {
            scoreLeft_++;
        } else {
            scoreRight_++;
        }
        std::printf("[Server] LEFT SCORES -> %d - %d\n", scoreLeft_, scoreRight_);
        //update score entity
        auto *score = (config::score_t *) ecs_get_component(&ecs_, scoreEntity_, config::COMPONENT_SCORE);
        score->left = scoreLeft_;
        score->right = scoreRight_;
        ecs_mark_component_dirty(&ecs_, scoreEntity_, config::COMPONENT_SCORE);


        if (ctx_.arch && ctx_.arch->impl) {
            auto *net = (network_cs_t *) ctx_.arch->impl;
            BallSpawner spawner(ecs_, net);
            spawner.spawnBall();
            std::printf("[Server] Spawning a new ball (total score=%d).\n", scoreLeft_ + scoreRight_);
        }
    }

private:
    static GameServer *s_instance;

    static void ScoreThunk(void *user, bool leftSide) {
        auto *self = static_cast<GameServer *>(user);
        if (self) self->onScore(leftSide);
    }

    static void onPeerConnected(void * /*user_data*/, peer_t *peer) {
        if (!s_instance || !peer) return;
        std::printf("[Server] Peer %s connected. Waiting for library snapshot/ACK.\n", peer->id);


        auto *self = s_instance;
        self->paddleSpawner_->spawnNewPaddle(peer);
        // Optionally send full state here
    }

    static void onPeerDisconnected(void * /*user_data*/, peer_t *peer) {
        if (!peer) return;
        std::printf("[Server] Peer %s disconnected.\n", peer->id);

        auto *self = s_instance;
        self->paddleSpawner_->destroyPaddle(peer);
    }

    static void OnClientInputReceived(void *user_data, peer_t *from, entity_t eid, uint8_t cmd, const void *extra,
                                      uint16_t extra_len) {
        auto *self = static_cast<GameServer *>(user_data);
        if (!self) return;
        self->HandleInputReceived(from, eid, cmd, extra, extra_len);
    }

    void HandleInputReceived(peer_t *from, entity_t /*eid*/, uint8_t cmd, const void *extra, uint16_t extra_len) {
        std::printf("[Server] Received input from %s -> cmd=%u\n", from ? from->id : "(null)", cmd);
        paddleSpawner_->movePaddle(from, cmd);
    }

    bool initNetwork() {
        network_architecture_config_t config_server{};
        config_server.type = ARCH_CLIENT_SERVER;
        config_server.ip_address = config::kBindIp;
        config_server.is_server = true;
        config_server.tcp_port = config::kTcpPort;
        config_server.udp_port = config::kUdpPort;
        config_server.on_peer_connected = &GameServer::onPeerConnected;
        config_server.on_peer_disconnected = &GameServer::onPeerDisconnected;
        config_server.on_client_input = &GameServer::OnClientInputReceived;
        config_server.user_data = this;
        config_server.ecs_sync_hz = 128.0f;
        network_architecture_init(&ctx_.arch, &config_server, &ecs_);
        s_instance = this;

        component_descriptor_t desc;
        desc.name = "EntityKind";
        desc.size = sizeof(config::entity_kind_t);
        desc.serialize = config::serialize_entity_kind;
        desc.deserialize = config::deserialize_entity_kind;

        config::COMPONENT_ENTITY_KIND = ecs_register_component(&ecs_, desc);

        component_descriptor_t scoreDesc;
        scoreDesc.name = "Score";
        scoreDesc.size = sizeof(config::score_t);
        scoreDesc.serialize = config::serialize_score;
        scoreDesc.deserialize = config::deserialize_score;

        config::COMPONENT_SCORE = ecs_register_component(&ecs_, scoreDesc);

        return ctx_.arch != nullptr;
    }

    int scoreLeft_ = 0;
    int scoreRight_ = 0;
    ecs_t ecs_{};
    entity_t scoreEntity_;
    ServerContext ctx_{};
    NetworkSyncService syncService_;
    std::unique_ptr<PaddleSpawner> paddleSpawner_;
};

GameServer *GameServer::s_instance = nullptr;

// ---------------------------------- main ------------------------------------

int main() {
    GameServer server;
    return server.run();
}
