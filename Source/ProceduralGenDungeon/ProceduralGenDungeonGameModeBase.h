// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ProceduralGenDungeonGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class PROCEDURALGENDUNGEON_API AProceduralGenDungeonGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
public:
	AProceduralGenDungeonGameModeBase();

protected:
	virtual void BeginPlay() override;
};
