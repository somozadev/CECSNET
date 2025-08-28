#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ecs_internal.h"
#include "network_cs.h"  
#include "RaindropClientActor.generated.h"

struct ecs_t;
struct network_architecture_t;

UCLASS()
class ECSNET4UNREAL_API ARaindropClientActor : public AActor
{
	GENERATED_BODY()

public:
	ARaindropClientActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

private:
	ecs_t Ecs;
	network_architecture_t* Arch = nullptr;

	const char* ServerPeerId = nullptr;
	
	TMap<int32, AActor*> EntityActors;


	static void OnPacketReceivedStatic(void* user, peer_t* peer, const void* data, int len);
	void OnPacketReceived(peer_t* peer, const void* data, int len);

	void ParseMultiEntityUpdate(const network_packet_t* packet);
	void ParseEntityUpdate(const network_packet_t* packet);
	
	void SyncEntities();
	void SendSpawnInput();

public:
	UPROPERTY(EditAnywhere)
	FString ServerIp = TEXT("127.0.0.1");

	UPROPERTY(EditAnywhere)
	int32 TcpPort = 51660;

	UPROPERTY(EditAnywhere)
	int32 UdpPort = 51660;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> DropClass;  
};
