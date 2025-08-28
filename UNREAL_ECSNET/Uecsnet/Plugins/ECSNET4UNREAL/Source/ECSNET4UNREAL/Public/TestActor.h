// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ecs_internal.h"
#include "TestActor.generated.h"

struct ecs_t;
struct network_architecture_t;
struct peer_t;

UCLASS()
class ECSNET4UNREAL_API ATestActor : public AActor
{
	GENERATED_BODY()

public:
	ATestActor();
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ECSNet")
	FString ServerIp = TEXT("127.0.0.1");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ECSNet")
	int32 TcpPort = 51660;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ECSNet")
	int32 UdpPort = 51660;

private:
	ecs_t Ecs;
	network_architecture_t* Arch = nullptr;
	peer_t* ServerPeer = nullptr;

	static void OnPacketReceivedStatic(void* user, peer_t* peer, const void* data, int len);
	static void OnPeerConnectedStatic(void* user, peer_t* peer);
	static void OnPeerDisconnectedStatic(void* user, peer_t* peer);

	void OnPacketReceived(peer_t* peer, const void* data, int len);
	void OnPeerConnected(peer_t* peer);
	void OnPeerDisconnected(peer_t* peer);
};