// Copyright Epic Games, Inc. All Rights Reserved.


#include "ProceduralGenDungeonGameModeBase.h"
#include "BaseCharacter.h"
#include "BasePlayerController.h"

AProceduralGenDungeonGameModeBase::AProceduralGenDungeonGameModeBase()
{
	DefaultPawnClass = ABaseCharacter::StaticClass();
	PlayerControllerClass = ABasePlayerController::StaticClass();
}

void AProceduralGenDungeonGameModeBase::BeginPlay()
{
	
}
