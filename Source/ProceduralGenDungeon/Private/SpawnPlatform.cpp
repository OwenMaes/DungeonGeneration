// Fill out your copyright notice in the Description page of Project Settings.


#include "SpawnPlatform.h"

// Sets default values
ASpawnPlatform::ASpawnPlatform()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Platform = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
}

// Called when the game starts or when spawned
void ASpawnPlatform::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASpawnPlatform::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

