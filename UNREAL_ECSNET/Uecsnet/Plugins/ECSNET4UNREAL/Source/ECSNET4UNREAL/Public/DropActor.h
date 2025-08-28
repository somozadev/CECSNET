// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DropActor.generated.h"

UCLASS()
class ECSNET4UNREAL_API ADropActor : public AActor
{
	GENERATED_BODY()

public:
	ADropActor();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* Mesh;
};