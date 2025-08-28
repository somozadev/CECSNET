#include "RaindropClientActor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

#include <string>
#include "ecs.h"
#include "ecs_builtin.h"
#include "ecs_internal.h"
#include "network_architecture.h"
#include "network_cs.h"
#include "net_socket.h"

ARaindropClientActor::ARaindropClientActor(): Ecs()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ARaindropClientActor::BeginPlay()
{
    Super::BeginPlay();

    net_socket_init();
    ecs_init(&Ecs);

    static std::string IpAnsi;
    IpAnsi = TCHAR_TO_ANSI(*ServerIp);

    network_architecture_config_t cfg{};
    cfg.type = ARCH_CLIENT_SERVER;
    cfg.ip_address = IpAnsi.c_str();
    cfg.is_server = false;
    cfg.tcp_port = (uint16)TcpPort;
    cfg.udp_port = (uint16)UdpPort;
    cfg.on_packet_received = &ARaindropClientActor::OnPacketReceivedStatic;
    cfg.user_data = this;
    cfg.ecs_sync_hz = 60.f;

    network_architecture_init(&Arch, &cfg, &Ecs);

    if (!Arch)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to init ECSNet"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("EcsNet ClientActor BeginPlay: connecting to %s:%d"), *ServerIp, TcpPort);
    }
}

void ARaindropClientActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (Arch) network_architecture_update(Arch, DeltaTime);
    ecs_update(&Ecs, DeltaTime);

    SyncEntities();

    if (GetWorld() && GetWorld()->GetFirstPlayerController()->WasInputKeyJustPressed(EKeys::SpaceBar))
    {
        SendSpawnInput();
    }
}

void ARaindropClientActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    for (auto& kv : EntityActors)
    {
        if (kv.Value) kv.Value->Destroy();
    }
    EntityActors.Empty();

    if (Arch)
    {
        Arch->config.on_packet_received = nullptr;
        Arch->config.user_data = nullptr;
        network_architecture_destroy(Arch);
        Arch = nullptr;
    }
    
    FMemory::Memzero(Ecs);

    net_socket_cleanup();

    Super::EndPlay(EndPlayReason);
}

// ---------------- Callbacks ----------------
void ARaindropClientActor::OnPacketReceivedStatic(void* user, peer_t* peer, const void* data, int len)
{
    if (auto* self = static_cast<ARaindropClientActor*>(user))
    {
        self->OnPacketReceived(peer, data, len);
    }
}

void ARaindropClientActor::OnPacketReceived(peer_t* peer, const void* data, int len)
{
    UE_LOG(LogTemp, Warning, TEXT("EcsNet Packet received from peer, size=%d"), len);

    if (!data || len < (int)sizeof(packet_header_t)) return;

    if (!ServerPeerId && peer && peer->id)
    {
        ServerPeerId = peer->id;
        UE_LOG(LogTemp, Warning, TEXT("EcsNet Server peer id cached: %s"), *FString(ServerPeerId));
    }
    
    const auto* packet = static_cast<const network_packet_t*>(data);
    if (packet->header.size > len) return; // truncated

    switch (packet->header.type)
    {
    case PACKET_TYPE_MULTI_ENTITY_UPDATE:
        ParseMultiEntityUpdate(packet);
        break;
    case PACKET_TYPE_ENTITY_UPDATE:
        ParseEntityUpdate(packet);
        break;
    default:
        break;
    }
}
void ARaindropClientActor::ParseMultiEntityUpdate(const network_packet_t* packet)
{
    const uint8_t* cur = packet->data;
    const uint8_t* end = reinterpret_cast<const uint8_t*>(packet) + packet->header.size;

    if (end - cur < (ptrdiff_t)sizeof(uint16_t)) return;
    uint16_t entityCount = 0;
    memcpy(&entityCount, cur, sizeof(entityCount));
    cur += sizeof(entityCount);

    for (uint16_t i = 0; i < entityCount; ++i)
    {
        if (end - cur < (ptrdiff_t)sizeof(entity_t)) break;
        entity_t eid = 0;
        memcpy(&eid, cur, sizeof(eid));
        cur += sizeof(eid);
        ecs_try_create_entity_by_id(&Ecs, eid);

        if (end - cur < 1) break;
        uint8_t compCount = *cur++;
        
        for (uint8_t c = 0; c < compCount; ++c)
        {
            component_t cid{};
            if (end - cur < (ptrdiff_t)sizeof(component_t)) break;
            memcpy(&cid, cur, sizeof(cid));
            cur += sizeof(cid);

            size_t compSize = Ecs.components[cid].descriptor.size;
            if (end - cur < (ptrdiff_t)compSize) return;

            if (!ecs_has_component(&Ecs, eid, cid))
            {
                ecs_add_component(&Ecs, eid, cid, (void*)cur);
            }
            else
            {
                void* dst = ecs_get_component(&Ecs, eid, cid);
                if (dst) memcpy(dst, cur, compSize);
            }
            ecs_mark_component_dirty(&Ecs, eid, cid);
            cur += compSize;
        }
    }
}

void ARaindropClientActor::ParseEntityUpdate(const network_packet_t* packet)
{
    const uint8_t* cur = packet->data;
    const uint8_t* end = reinterpret_cast<const uint8_t*>(packet) + packet->header.size;

    if (end - cur < (ptrdiff_t)sizeof(entity_t)) return;
    entity_t eid{};
    memcpy(&eid, cur, sizeof(eid));
    cur += sizeof(eid);
    ecs_try_create_entity_by_id(&Ecs, eid);

    while (end - cur >= (ptrdiff_t)sizeof(component_t))
    {
        component_t cid{};
        memcpy(&cid, cur, sizeof(cid));
        cur += sizeof(cid);

        size_t compSize = Ecs.components[cid].descriptor.size;
        if (end - cur < (ptrdiff_t)compSize) break;

        if (!ecs_has_component(&Ecs, eid, cid))
        {
            ecs_add_component(&Ecs, eid, cid, (void*)cur);
        }
        else
        {
            void* dst = ecs_get_component(&Ecs, eid, cid);
            if (dst) memcpy(dst, cur, compSize);
        }
        ecs_mark_component_dirty(&Ecs, eid, cid);
        cur += compSize;
    }
}
// ---------------- Entity ↔ Actor sync ----------------
void ARaindropClientActor::SyncEntities()
{
    for (int32 e = 0; e < Ecs.entity_capacity; ++e)
    {
        if (!Ecs.entities[e].in_use) continue;
        if (!ecs_has_component(&Ecs, e, COMPONENT_POSITION)) continue;

        auto* pos = (position_t*)ecs_get_component(&Ecs, e, COMPONENT_POSITION);
        if (!pos) continue;

        AActor** found = EntityActors.Find(e);
        if (!found)
        {
            if (DropClass && DropClass->IsChildOf(AActor::StaticClass()))
            {
                FActorSpawnParameters SpawnInfo;
                AActor* NewDrop = GetWorld()->SpawnActor<AActor>(
                    DropClass,
                    FVector(pos->x, 0, pos->y),
                    FRotator::ZeroRotator,
                    SpawnInfo
                );
                EntityActors.Add(e, NewDrop);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("EcsNet DropClass not set or not an Actor!"));
            }
        }
        else if (*found)
        {
            (*found)->SetActorLocation(FVector(pos->x, pos->y, 0));
        }
    }
}



void ARaindropClientActor::SendSpawnInput()
{
    if (!Arch || !ServerPeerId) return;

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;

    FVector CamLoc;
    FRotator CamRot;
    PC->GetPlayerViewPoint(CamLoc, CamRot);

    FVector Forward = CamRot.Vector();

    FVector SpawnPos = CamLoc + Forward * 500.0f;

    struct SpawnXY { float x; float y; };
    SpawnXY xy;
    xy.x = SpawnPos.X;
    xy.y = SpawnPos.Z;
    
    protocol_handler_t handler;
    protocol_handler_init(&handler);
    protocol_handler_pack_client_input(&handler, 0, 0x80 /* INPUT_SPAWN */, &xy, sizeof(xy));

    auto* cs = static_cast<network_cs_t*>(Arch->impl);
    if (cs)
    {
        protocol_handler_send_packet(&cs->connection_manager, ServerPeerId, &handler);
        UE_LOG(LogTemp, Warning, TEXT("EcsNet Spawn input sent to server at X=%.1f, Z=%.1f"), xy.x, xy.y);
    }
} 