// Fill out your copyright notice in the Description page of Project Settings.


#include "TestActor.h"

#include <string>

#include "ecsnet.h"


// Sets default values
ATestActor::ATestActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void ATestActor::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("EcsNet ClientActor BeginPlay: connecting to %s:%d"),
		*ServerIp, TcpPort);

	 net_socket_init();

	 ecs_init(&Ecs);

	static std::string IpAnsi;
	IpAnsi = TCHAR_TO_ANSI(*ServerIp);

	network_architecture_config_t Cfg{};
	Cfg.type = ARCH_CLIENT_SERVER;
	Cfg.ip_address = IpAnsi.c_str();
	Cfg.is_server = false;
	Cfg.tcp_port = (uint16)TcpPort;
	Cfg.udp_port = (uint16)UdpPort;
	Cfg.user_data = nullptr;
	Cfg.ecs_sync_hz = 60.0f;
	Cfg.on_packet_received = &ATestActor::OnPacketReceivedStatic;
	Cfg.on_peer_connected  = &ATestActor::OnPeerConnectedStatic;
	Cfg.on_peer_disconnected = &ATestActor::OnPeerDisconnectedStatic;
	Cfg.user_data = this;  


	network_architecture_init(&Arch, &Cfg, &Ecs);


	if(Arch==nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("ECSNet Failed to initialize networking"));
		net_socket_cleanup();
		return;
	}

	if (Arch && &Ecs)
	{
		UE_LOG(LogTemp, Warning, TEXT("ECSNet initialized OK"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ECSNet initialization FAILED"));
	}
}

void ATestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Arch)
		network_architecture_update(Arch, DeltaTime);
	if (&Ecs)
		ecs_update(&Ecs, DeltaTime);
}

void ATestActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_LOG(LogTemp, Warning, TEXT("EcsNet ClientActor shutting down"));

	if (Arch)
	{
		network_architecture_destroy(Arch);
		Arch = nullptr;
	}
	net_socket_cleanup();

	Super::EndPlay(EndPlayReason);
}


void ATestActor::OnPacketReceivedStatic(void* user, peer_t* peer, const void* data, int len)
{
	if (auto* self = static_cast<ATestActor*>(user)) {
		self->OnPacketReceived(peer, data, len);
	}
}

void ATestActor::OnPeerConnectedStatic(void* user, peer_t* peer)
{
	if (auto* self = static_cast<ATestActor*>(user)) {
		self->OnPeerConnected(peer);
	}
}

void ATestActor::OnPeerDisconnectedStatic(void* user, peer_t* peer)
{
	if (auto* self = static_cast<ATestActor*>(user)) {
		self->OnPeerDisconnected(peer);
	}
}



void ATestActor::OnPacketReceived(peer_t* peer, const void* data, int len)
{
	UE_LOG(LogTemp, Warning, TEXT("EcsNet Packet received from server, size=%d"), len);

}

void ATestActor::OnPeerConnected(peer_t* peer)
{
	UE_LOG(LogTemp, Warning, TEXT("EcsNet Connected to server peer!"));
}

void ATestActor::OnPeerDisconnected(peer_t* peer)
{
	UE_LOG(LogTemp, Warning, TEXT("EcsNet Disconnected from server peer!"));
}
