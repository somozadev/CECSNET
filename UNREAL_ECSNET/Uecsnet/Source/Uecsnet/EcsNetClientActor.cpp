// Fill out your copyright notice in the Description page of Project Settings.


#include "EcsNetClientActor.h"
#include "Uecsnet.h"


// Sets default values
AEcsNetClientActor::AEcsNetClientActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AEcsNetClientActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AEcsNetClientActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

