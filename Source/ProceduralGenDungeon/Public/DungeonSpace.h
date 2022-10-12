// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonSpace.generated.h"

UENUM(BlueprintType)
enum class ESeperation : uint8 {
	VERTICAL = 0 UMETA(DisplayName = "Vertical"),
	HORIZONTAL = 1  UMETA(DisplayName = "Horizontal"),
};

UENUM(BlueprintType)
enum class ETileType : uint8 {
	EMPTY = 0 UMETA(DisplayName = "Empty"),
	ROOM = 1 UMETA(DisplayName = "Room"),
	CORRIDOR = 2  UMETA(DisplayName = "Corridor"),
};

UENUM(BlueprintType)
enum class EDungeonObjectType : uint8 {
	FLOOR = 0 UMETA(DisplayName = "Floor"),
	WALL = 1  UMETA(DisplayName = "Wall"),
	CEILING = 2  UMETA(DisplayName = "Ceiling"),
	PILLAR = 3  UMETA(DisplayName = "Pillar"),
	TORCH = 4  UMETA(DisplayName = "Torch"),
};

UENUM(BlueprintType)
enum class EDungeonObjectAlign : uint8 {
	LEFT = 0 UMETA(DisplayName = "Left"),
	RIGHT = 1  UMETA(DisplayName = "Right"),
	TOP = 2  UMETA(DisplayName = "Top"),
	BOTTOM = 3  UMETA(DisplayName = "Bottom"),
	CENTER = 4  UMETA(DisplayName = "Center"),
};

USTRUCT()
struct FDungeonObject
{
	GENERATED_BODY()
		EDungeonObjectType objectType;
	FVector rotation;
	EDungeonObjectAlign objectAlignement;

	FDungeonObject()
		:objectType(EDungeonObjectType::FLOOR)
		, rotation(1, 0, 0)
		, objectAlignement(EDungeonObjectAlign::CENTER)
	{

	}

	FDungeonObject(EDungeonObjectType type, EDungeonObjectAlign align, FVector rot)
		:objectType(type)
		, rotation(rot)
		, objectAlignement(align)
	{

	}
};

USTRUCT()
struct FTile
{
	GENERATED_BODY()
		int left;
	int bottom;
	TArray<FDungeonObject> objectsToSpawn;
	ETileType tileType;
	int corridorID;
	int miniMapTileInstanceID;

	FTile()
		:left(0),
		bottom(0),
		tileType(ETileType::EMPTY),
		corridorID(-1),
		miniMapTileInstanceID(0)
	{

	}

	FTile(int tileLeft, int tileBottom, ETileType tileTypex, int corridorIDx = -1)
		:left(tileLeft),
		bottom(tileBottom),
		tileType(tileTypex),
		corridorID(corridorIDx)
	{

	}
};

USTRUCT()
struct FCorridor
{
	GENERATED_BODY()
		FIntVector start;
	FIntVector end;
	ESeperation seperation;
};

USTRUCT()
struct FData
{
	GENERATED_BODY()
		int key;
	int width;
	int height;
	int left;
	int bottom;
	ESeperation seperation;
	int tilesSeperated;

};

USTRUCT()
struct FSpace
{
	GENERATED_BODY()
	FData data;
	FSpace* left;
	FSpace* right;

	FSpace()
		:data()
		, left(nullptr)
		, right(nullptr)
	{

	}
};

UCLASS()
class PROCEDURALGENDUNGEON_API ADungeonSpace : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ADungeonSpace();
	void GenerateMinimap(FTransform& playerTransform);
	void DebugTiles(FVector& tilePos);
	void GenerateDungeon();

	/*The size of the dungeon should be divisible by the tilesize.*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Dungeon")
		int DungeonSize = 36000;
	/*The number of times the spaces will be split up randomly.*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Dungeon")
		int SplitIterations = 5;
	/*The size of 1 tile. Must be the same size as the tile meshes (floors, ceilings and walls)*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Dungeon")
		int TileSize = 600;
	/*The minimum amount of tiles, a room requires (used for width and height).*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Dungeon")
		int MinTilesPerRoom = 2;
	/*The ratio of the room (0-1), used to make the rooms look normal and not long rectangles.*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Dungeon")
		float MinRoomRatio = 0.4f;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Dungeon")
		int WallTileWidth = 10;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Minimap")
		int CubeMeshSize = 100;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Minimap")
		bool IsShowingMinimap = true;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Minimap")
		int MinimapTileSize;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Minimap")
		FVector MinimapPos;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	UPROPERTY(VisibleAnywhere, Category = "Meshes")
		UInstancedStaticMeshComponent* CubeISMC;
	UPROPERTY(VisibleAnywhere, Category = "Meshes")
		UInstancedStaticMeshComponent* FloorTileISMC;
	UPROPERTY(VisibleAnywhere, Category = "Meshes")
		UInstancedStaticMeshComponent* WallTileISMC;


private:
	FSpace* RootSpace;
	TArray<FSpace*> DungeonRooms;
	TMap<int, FCorridor*> DungeonCorridors; //first space id, second corridor
	TArray<FTile> TileArray;
	int TileRows;
	bool IsDungeonGenerated;

	
	FSpace* SplitSpace(FSpace* currentSpace, int index, int maxElements, FData parentData);
	void PrintTree(FString& string, FSpace* root);
	void SelectDungeonRooms(FSpace* currentSpace, int currentDepth);
	void FillTileGrid();
	void ConstructDungeonGrid();
	void ShrinkSpaceToRoom(FSpace* currentSpace);
	bool CheckIfWallShouldBePlaced(int tileIndex, int adjacentTileIndex);
	bool IsCorridorConnected(int tileIndex);
	void PlaceCorridorsWalls(int tileIndex);
	void ShowDebugTile(int tileIndex, FString& tileInfo, FColor colorBox);
	void PlaceWalls(int tileIndex);
	void ResetDungeon();
	void MoveSpawnPlatform();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
