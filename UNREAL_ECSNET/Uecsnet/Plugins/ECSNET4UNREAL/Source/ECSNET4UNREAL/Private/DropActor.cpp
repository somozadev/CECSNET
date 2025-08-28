// Fill out your copyright notice in the Description page of Project Settings.


#include "DropActor.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"


ADropActor::ADropActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube")
	);
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}

	Mesh->SetWorldScale3D(FVector(0.1f, 0.1f, 0.5f));

	Mesh->SetMaterial(0, nullptr); 
}

void ADropActor::BeginPlay()
{
	Super::BeginPlay();
}