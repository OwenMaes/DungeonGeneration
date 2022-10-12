// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RRPDungeon.generated.h"

UENUM(BlueprintType)
enum class ETileNodeType : uint8 {
	EMPTY = 0 UMETA(DisplayName = "Empty"),
	ROOM = 1 UMETA(DisplayName = "Room"),
	CORRIDOR = 2 UMETA(DisplayName = "Corridor"),
	DOOR = 3 UMETA(DisplayName = "Door"),
};

UENUM(BlueprintType)
enum class ECorridorType : uint8 {
	RANDOMROOMCONNECT = 0 UMETA(DisplayName = "Random Room Connect"),
};

UENUM(BlueprintType)
enum class EHeuristicCost : uint8 {
	MANHATTAN = 0 UMETA(DisplayName = "Manhattan"),
	EUCLIDEAN = 1 UMETA(DisplayName = "Euclidean"),
	SQRTEUCLIDEAN = 2 UMETA(DisplayName = "SqrtEuclidean"),
	OCTILE = 3 UMETA(DisplayName = "Octile"),
	CHEBYSHEV = 4 UMETA(DisplayName = "Chebyshev"),
};

USTRUCT()
struct FTileConnection
{
	GENERATED_BODY()

		int FromNodeID;
	int ToNodeID;
	float ConnectionCost;

	FTileConnection(int fromNodeID, int toNodeID, float connectionCost)
		:FromNodeID(fromNodeID)
		, ToNodeID(toNodeID)
		, ConnectionCost(connectionCost)
	{

	}

	FTileConnection()
		:FromNodeID(-1)
		, ToNodeID(-1)
		, ConnectionCost(0)
	{

	}
};

USTRUCT()
struct FTileNode
{
	GENERATED_BODY()

		int NodeID;
	FVector TilePosition;
	TArray<FTileConnection> Connections;
	ETileNodeType TileNodeType;

	FTileNode(int nodeID, FVector tilePosition)
		:NodeID(nodeID)
		, TilePosition(tilePosition)
	{
		Connections = {};
		TileNodeType = ETileNodeType::EMPTY;
	}

	FTileNode()
		:NodeID(-1)
		, TilePosition()
	{
		Connections = {};
		TileNodeType = ETileNodeType::EMPTY;
	}

};

USTRUCT()
struct FTileNodeRecord
{
	GENERATED_BODY()

		FTileNode* TileNode;
	FTileConnection Connection;
	float CostSoFar;
	float EstimatedTotalCost;

	FTileNodeRecord()
		:TileNode(nullptr)
		, Connection()
		, CostSoFar(0.f)
		, EstimatedTotalCost(0.f)
	{

	}

	bool operator<(const FTileNodeRecord& other) const
	{
		return EstimatedTotalCost < other.EstimatedTotalCost;
	}

	friend bool operator==(const FTileNodeRecord& lhs, const FTileNodeRecord& rhs)
	{
		return lhs.TileNode->NodeID == rhs.TileNode->NodeID
			&& lhs.Connection.FromNodeID == rhs.Connection.FromNodeID
			&& lhs.Connection.ToNodeID == rhs.Connection.ToNodeID
			&& lhs.CostSoFar == rhs.CostSoFar
			&& lhs.EstimatedTotalCost == rhs.EstimatedTotalCost;
	}
};

USTRUCT()
struct FDoor
{
	GENERATED_BODY()
		UPROPERTY(EditAnywhere, meta = (TitleProperty = "Room ID"))
		int TileID;
	UPROPERTY(EditAnywhere, meta = (TitleProperty = "Corridor ID"))
		float CorridorID;
	UPROPERTY(EditAnywhere, meta = (TitleProperty = "Door direction"))
		FVector Direction;
	UPROPERTY(EditAnywhere, meta = (TitleProperty = "Door direction"))
		TArray<int32> WallInstancesToRemove;

	FDoor()
		:TileID(-1)
		, CorridorID(-1)
		, Direction()
	{
		WallInstancesToRemove = {};
	}

};

USTRUCT(BlueprintType)
struct FRoom
{
	GENERATED_BODY()
		UPROPERTY(EditAnywhere, meta = (TitleProperty = "Room ID"))
		int RoomID;
	UPROPERTY(EditAnywhere, meta = (TitleProperty = "Room width"))
		float Width;
	UPROPERTY(EditAnywhere, meta = (TitleProperty = "Room height"))
		float Height;
	UPROPERTY(EditAnywhere, meta = (TitleProperty = "Room position"))
		FVector CentralPosition;
	TArray<FTileNode*> TileNodesOfRoom;

	FRoom()
		:RoomID(0)
		, Width(0)
		, Height(0)
		, CentralPosition(0, 0, 0)
	{
		TileNodesOfRoom = {};
	}
};

UCLASS()
class PROCEDURALGENDUNGEON_API ARRPDungeon : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ARRPDungeon();

	UFUNCTION(BlueprintCallable, Category = "RRPDungeon")
		void GenerateDungeon();

	/*The middle point of the dungeon.*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "RRPDungeon settings")
		FVector DungeonCentralPosition = {};

	/*The radius of the dungeon, this does not necessarily mean the actual dungeon size (which can vary).*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "RRPDungeon settings")
		float DungeonRadius = 6000;

	/*The size of the tiles (Floors & Walls).*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "RRPDungeon settings")
		float RoomTileSize = 600;

	/*The minimum amount of tiles used in width and height of a room.*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "RRPDungeon settings")
		int MinRoomTiles = 2;

	/*The maximum amount of tiles used in width and height of a room.*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "RRPDungeon settings")
		int MaxRoomTiles = 8;

	/*The number of rooms that will spawn in the dungeon.*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "RRPDungeon settings")
		int NrOfRooms = 12;

	/*The array of premade rooms, has to be less than the nr of rooms setting. The remaining rooms will be randomly generated.*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "RRPDungeon settings")
		TArray<FRoom> ArrayOfPremadeRooms = {};

	/*The array of premade rooms, has to be less than the nr of rooms setting. The remaining rooms will be randomly generated.*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "RRPDungeon settings")
		TArray<FRoom> ArrayOfRooms = {};

	/*The cost of the connection to a empty TileNode (Pathfinding).*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "RRPDungeon settings")
		float EmptyTileConnectionCost = 1.f;

	/*The cost of the connection to a corridor TileNode (Pathfinding).*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "RRPDungeon settings")
		float CorridorConnectionCost = 100;

	/*The cost of the connection to a room TileNode (Pathfinding).*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "RRPDungeon settings")
		float RoomConnectionCost = 550;

	/*The function used to calculate the Heuristic Cost (Pathfinding).*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "RRPDungeon settings")
		EHeuristicCost HeuresticCostFunction = EHeuristicCost::MANHATTAN;

	/*Draw debug.*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "RRPDungeon settings")
		bool IsDrawingDebug = true;



protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Meshes")
		UInstancedStaticMeshComponent* FloorTileISMC;
	UPROPERTY(VisibleAnywhere, Category = "Meshes")
		UInstancedStaticMeshComponent* WallTileISMC;
private:

	TMap<int, FTileNode> TileNodeGrid = {};
	TMap<int, TArray<FTileNode*>> CorridorTiles = {};
	TArray<FVector> AdjacentDirections = { { 1, 0, 0 }, { 0, 1, 0 }, { -1, 0, 0 }, { 0, -1, 0 } };
	TMap<int, FDoor> DoorTiles = {};
	bool IsDungeonGenerating = false;
	bool HasOverlap = true;
	float TopOfGrid = FLT_MAX;
	float BotOfGrid = FLT_MIN;
	float RightOfGrid = FLT_MIN;
	float LeftOfGrid = FLT_MAX;
	int NrOfGridCols = 0;
	int NrOfGridRows = 0;


	void GenerateRooms();
	void SeperateRooms();
	void ContructTileNodeGrid();
	void AttachTileNodesToRooms();
	void RandomRoomConnect();
	void CreateCorridorFromPath(TArray<FTileNode*>& path);
	void CreateDoorTile(FTileNode* currentNode, FTileNode* nextNode, int corridorID);
	void SpawnInstancedMeshes();
	void SpawnMeshesOnTileNode(FTileNode* node, FTransform& floorTransform, FTransform& wallTransform, TArray<ETileNodeType>& tilesTypesToIgnore);
	void DrawDebugTiles(float timeDrawn);
	void ResetDungeon();
	FVector GetRandomPointInCircle();
	bool AreRoomsOverlapping(const FRoom& roomA, const FRoom& roomB, float margin) const;
	FTileNode* GetNodeFromPosition(const FVector& pos);
	TArray<FTileNode*> GetPathAStar(FTileNode* startNode, FTileNode* endNode);
	float GetHeuristicCost(FTileNode* startNode, FTileNode* endNode);
	bool IsPositionInGrid(const FVector& pos) const;
	bool IsNodeTileAndDoorFacingSameDirection(FTileNode* node, FTileNode* doorNode) const;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;


};
