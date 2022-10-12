// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharacter.h"
#include "DungeonSpace.h"
#include "RRPDungeon.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"

// Sets default values
ABaseCharacter::ABaseCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;


	//// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Configure character movement
	BaseWalkSpeed = 1200.f;
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
}

void ABaseCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ABaseCharacter::MoveRight(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void ABaseCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ABaseCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ABaseCharacter::SpawnMinimap()
{
	
}

void ABaseCharacter::GenerateDungeon()
{
	TArray<AActor*> BSPDungeons;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADungeonSpace::StaticClass(), BSPDungeons);
	TArray<AActor*> RRPDungeons;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARRPDungeon::StaticClass(), RRPDungeons);

	if (BSPDungeons.Num() > 0)
	{
		ADungeonSpace* BSPDungeon = ((ADungeonSpace*)BSPDungeons[0]);
		if (IsValid(BSPDungeon))
		{
			BSPDungeon->GenerateDungeon();
		}
	}
	else if (RRPDungeons.Num() > 0) {
		ARRPDungeon* RRPDungeon = ((ARRPDungeon*)RRPDungeons[0]);
		if (IsValid(RRPDungeon))
		{
			RRPDungeon->GenerateDungeon();
		}
	}
}

void ABaseCharacter::DebugTile()
{
	/*if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, TEXT("Debug tile"));

	TArray<AActor*> DungeonSpaces;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADungeonSpace::StaticClass(), DungeonSpaces);

	if (DungeonSpaces.Num() > 0)
	{
		ADungeonSpace* DungeonSpace = ((ADungeonSpace*)DungeonSpaces[0]);
		if (IsValid(DungeonSpace))
		{
			FVector location = GetActorLocation();
			DungeonSpace->DebugTiles(location);
		}
	}*/
}

void ABaseCharacter::StartSprinting()
{
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * 3;
}

void ABaseCharacter::StopSprinting()
{
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
}

// Called when the game starts or when spawned
void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Blue, TEXT("using basecharacter"));

	
	
}

// Called every frame
void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);

	PlayerInputComponent->BindAxis("MoveForward", this, &ABaseCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABaseCharacter::MoveRight);
	PlayerInputComponent->BindAction("SpawnMinimap", EInputEvent::IE_Released, this, &ABaseCharacter::SpawnMinimap);
	PlayerInputComponent->BindAction("ResetDungeon", EInputEvent::IE_Released, this, &ABaseCharacter::GenerateDungeon);
	PlayerInputComponent->BindAction("Sprint", EInputEvent::IE_Pressed, this, &ABaseCharacter::StartSprinting);
	PlayerInputComponent->BindAction("Sprint", EInputEvent::IE_Released, this, &ABaseCharacter::StopSprinting);
	PlayerInputComponent->BindAction("DebugTile", EInputEvent::IE_Released, this, &ABaseCharacter::DebugTile);
}

