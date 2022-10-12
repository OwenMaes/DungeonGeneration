// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonSpace.h"
#include "DrawDebugHelpers.h"
#include "SpawnPlatform.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ADungeonSpace::ADungeonSpace()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TileRows = DungeonSize / TileSize;
	TileArray.Init(FTile(), TileRows * TileRows);
	RootSpace = nullptr;

	CubeISMC = CreateDefaultSubobject<class UInstancedStaticMeshComponent>(TEXT("Cube InstancedStaticMesh"));
	CubeISMC->SetMobility(EComponentMobility::Static);
	CubeISMC->SetCollisionProfileName("NoCollision");
	CubeISMC->NumCustomDataFloats = TileRows * TileRows;

	FloorTileISMC = CreateDefaultSubobject<class UInstancedStaticMeshComponent>(TEXT("Floor InstancedStaticMesh"));
	FloorTileISMC->SetMobility(EComponentMobility::Static);
	FloorTileISMC->SetCollisionProfileName("BlockAll");

	WallTileISMC = CreateDefaultSubobject<class UInstancedStaticMeshComponent>(TEXT("Wall InstancedStaticMesh"));
	WallTileISMC->SetMobility(EComponentMobility::Static);
	WallTileISMC->SetCollisionProfileName("BlockAll");





}

void ADungeonSpace::GenerateMinimap(FTransform& playerTransform)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Emerald, TEXT("Generating minimap..."));
	}
	float minDistanceFromPlayer = 10.f;
	FVector minimapPos = playerTransform.GetLocation() + playerTransform.GetRotation().Vector() * minDistanceFromPlayer;
	FVector FromActorToMinimapPos = minimapPos - GetActorLocation();

	//remove all instances of the cubeISMC
	CubeISMC->ClearInstances();

	//scale cube mesh to minimap tile size
	FTransform minimapTileTransform = GetTransform();
	minimapTileTransform.SetScale3D(FVector(float(MinimapTileSize) / CubeMeshSize, float(MinimapTileSize) / CubeMeshSize, float(MinimapTileSize) / CubeMeshSize));
	int tileIndex = -1;
	int rows = TileRows;
	int newInstanceIndex{};

	for (int row = 0; row < rows; row++)
	{
		for (int col = 0; col < rows; col++)
		{
			tileIndex = col + rows * row;
			//Check if index is valid and tile is not empty
			if (TileArray.IsValidIndex(tileIndex) && TileArray[tileIndex].tileType != ETileType::EMPTY)
			{
				//create minimap
				if (IsShowingMinimap)
				{
					minimapTileTransform.SetLocation(FVector(col * MinimapTileSize + FromActorToMinimapPos.X, row * MinimapTileSize + FromActorToMinimapPos.Y, FromActorToMinimapPos.Z -50.f));
					newInstanceIndex = CubeISMC->AddInstance(minimapTileTransform);
					TileArray[tileIndex].miniMapTileInstanceID = newInstanceIndex;
					switch (TileArray[tileIndex].tileType)
					{
					case ETileType::ROOM:
						CubeISMC->SetCustomDataValue(newInstanceIndex, 0, 0.15f, true);
						break;
					case ETileType::CORRIDOR:
						CubeISMC->SetCustomDataValue(newInstanceIndex, 0, 0.05f, true);
						break;
					}
				}
			}
		}
	}

	//works when the the dungeon space location = 0,0,0
	int tilePlayerIndex = int(playerTransform.GetLocation().X / TileSize) + TileRows * int(playerTransform.GetLocation().Y / TileSize);
	if (TileArray.IsValidIndex(tilePlayerIndex) && TileArray[tilePlayerIndex].tileType != ETileType::EMPTY)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Emerald, TEXT("Player is in the dungeon!"));
		}
		CubeISMC->SetCustomDataValue(TileArray[tilePlayerIndex].miniMapTileInstanceID, 0, 0.25f, true);
	}


}

void ADungeonSpace::DebugTiles(FVector& tilePos)
{
	int tileIndex = int(tilePos.X / TileSize) + TileRows * int(tilePos.Y / TileSize);
	FString infoTile{};
	infoTile.Append(TEXT("Center tile: type("));
	ShowDebugTile(tileIndex, infoTile, FColor::White);

	infoTile.Reset();
	infoTile.Append(TEXT("Right tile: type("));
	ShowDebugTile(tileIndex - 1, infoTile, FColor::Purple);

	infoTile.Reset();
	infoTile.Append(TEXT("Left tile: type("));
	ShowDebugTile(tileIndex + 1, infoTile, FColor::Green);

	infoTile.Reset();
	infoTile.Append(TEXT("Top tile: type("));
	ShowDebugTile(tileIndex + TileRows, infoTile, FColor::Yellow);

	infoTile.Reset();
	infoTile.Append(TEXT("Bot tile: type("));
	ShowDebugTile(tileIndex - TileRows, infoTile, FColor::Orange);
}

// Called when the game starts or when spawned
void ADungeonSpace::BeginPlay()
{
	Super::BeginPlay();

	GenerateDungeon();
	FString text;
	PrintTree(text, RootSpace);
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, text);
}

void ADungeonSpace::GenerateDungeon()
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Emerald, TEXT("Generating dungeon..."));

	ResetDungeon();

	//generate BSP Dungeon
	const int maxElements = pow(2, SplitIterations + 1) - 1;
	FData parentData = FData();
	parentData.width = DungeonSize;
	parentData.height = DungeonSize;
	parentData.left = 0;
	parentData.bottom = 0;
	parentData.seperation = ESeperation(FMath::RandRange(0, 1));
	parentData.tilesSeperated = FMath::RandRange(MinTilesPerRoom, DungeonSize / TileSize - MinTilesPerRoom);
	RootSpace = SplitSpace(nullptr, 0, maxElements, parentData);
	SelectDungeonRooms(RootSpace, 0);
	FillTileGrid();
	ConstructDungeonGrid();
	IsDungeonGenerated = true;
	MoveSpawnPlatform();
}

FSpace* ADungeonSpace::SplitSpace(FSpace* currentSpace, int index, int maxElements, FData parentData)
{
	if (index < maxElements)
	{
		int minRoomSize = TileSize * MinTilesPerRoom + TileSize * 2;

		FSpace* temp = new FSpace();
		temp->data.key = index;

		//check if the width and height are still big enough to split
		if (parentData.width <= minRoomSize && parentData.height <= minRoomSize)
			return currentSpace;

		//Change data depending on left or right of parent space
		if (index > 0)
		{
			if (index % 2 == 1)//odd = left or top of the space split
			{
				if (parentData.seperation == ESeperation::VERTICAL)
				{
					parentData.width = TileSize * parentData.tilesSeperated;
				}
				else
				{
					parentData.height = parentData.height - (TileSize * parentData.tilesSeperated);
					parentData.bottom = parentData.bottom + TileSize * parentData.tilesSeperated;
				}

				//create startpoint of corridor
				FCorridor* corridor = new FCorridor();
				corridor->start.X = parentData.left + (parentData.width / TileSize / 2 - 1) * TileSize;
				corridor->start.Y = parentData.bottom + (parentData.height / TileSize / 2 + 1) * TileSize;
				corridor->seperation = parentData.seperation;

				DungeonCorridors.Add(index, corridor);

			}
			else//even = right or bottom of the space split
			{
				if (parentData.seperation == ESeperation::VERTICAL)
				{
					parentData.width = parentData.width - (TileSize * parentData.tilesSeperated);
					parentData.left = parentData.left + TileSize * parentData.tilesSeperated;
				}
				else
				{
					parentData.height = TileSize * parentData.tilesSeperated;
				}

				//create endpoint of corridor if corridor exists
				if (DungeonCorridors.Contains(index - 1))
				{
					FCorridor* corridorOfSister = DungeonCorridors[index - 1];
					corridorOfSister->end.X = parentData.left + (parentData.width / TileSize / 2 + 1) * TileSize;
					corridorOfSister->end.Y = parentData.bottom + (parentData.height / TileSize / 2 - 1) * TileSize;

				}
			}
		}

		//change data of current space
		temp->data.width = parentData.width;
		temp->data.height = parentData.height;
		temp->data.left = parentData.left;
		temp->data.bottom = parentData.bottom;

		currentSpace = temp;



		//calculate next split
		int minXTiles = int((float(parentData.height) * MinRoomRatio)) / TileSize;
		int maxXTiles = (parentData.width / TileSize) - minXTiles;
		bool isVerticalSplitValid = minXTiles < maxXTiles;

		int minYTiles = int((float(parentData.width) * MinRoomRatio)) / TileSize;
		int maxYTiles = (parentData.height / TileSize) - minYTiles;
		bool isHorizontalSplitValid = minYTiles < maxYTiles;

		if (isVerticalSplitValid && isHorizontalSplitValid)
		{
			//randomize split
			parentData.seperation = ESeperation(FMath::RandRange(0, 1));
			if (parentData.seperation == ESeperation::VERTICAL)
				parentData.tilesSeperated = FMath::RandRange(minXTiles, maxXTiles);
			else
				parentData.tilesSeperated = FMath::RandRange(maxYTiles, maxYTiles);
		}
		else if (isVerticalSplitValid && !isHorizontalSplitValid)
		{
			//vertical split
			parentData.tilesSeperated = FMath::RandRange(minXTiles, maxXTiles);
			parentData.seperation = ESeperation::VERTICAL;
		}
		else if (!isVerticalSplitValid && isHorizontalSplitValid)
		{
			//horizontal split
			parentData.tilesSeperated = FMath::RandRange(minYTiles, maxYTiles);
			parentData.seperation = ESeperation::HORIZONTAL;
		}
		else // no split possible
			return currentSpace;


		currentSpace->left = SplitSpace(currentSpace->left, 2 * index + 1, maxElements, parentData);

		currentSpace->right = SplitSpace(currentSpace->right, 2 * index + 2, maxElements, parentData);
	}
	return currentSpace;
}

void ADungeonSpace::PrintTree(FString& string, FSpace* root)
{
	if (root != nullptr)
	{
		PrintTree(string, root->left);
		string.Append(FString::FromInt(root->data.key));
		string.Append(TEXT(" "));
		PrintTree(string, root->right);
	}
}

void ADungeonSpace::SelectDungeonRooms(FSpace* currentSpace, int currentDepth)
{
	if (currentSpace == nullptr)
		return;

	if (currentDepth == SplitIterations || (currentSpace->left == nullptr || currentSpace->right == nullptr))
	{
		DungeonRooms.Add(currentSpace);
	}

	SelectDungeonRooms(currentSpace->left, currentDepth + 1);
	SelectDungeonRooms(currentSpace->right, currentDepth + 1);
}

void ADungeonSpace::FillTileGrid()
{
	int tilesDungeon = DungeonSize / TileSize;
	int tileIndex;

	//Fill rooms in grid with floor tiles
	int left, right, top, bottom;
	for (int i = 0; i < DungeonRooms.Num(); i++)
	{
		ShrinkSpaceToRoom(DungeonRooms[i]); //todo fix corridor connections

		left = DungeonRooms[i]->data.left;
		right = DungeonRooms[i]->data.left + DungeonRooms[i]->data.width;
		bottom = DungeonRooms[i]->data.bottom;
		top = DungeonRooms[i]->data.bottom + DungeonRooms[i]->data.height;

		for (int row = bottom; row < top; row += TileSize)
		{
			for (int col = left; col < right; col += TileSize) {

				tileIndex = (col / TileSize) + tilesDungeon * (row / TileSize);
				if (TileArray.IsValidIndex(tileIndex))
				{
					TileArray[tileIndex].tileType = ETileType::ROOM;
					TileArray[tileIndex].objectsToSpawn.Add(FDungeonObject()); //default object is a floor
					TileArray[tileIndex].left = col;
					TileArray[tileIndex].bottom = row;
				}

			}

		}
	}

	//fill corridors in grid with floor tiles
	for (auto& elem : DungeonCorridors)
	{
		FCorridor* currentCorridor = elem.Value;
		int x, y;
		if (currentCorridor->seperation == ESeperation::VERTICAL) //vertical seperation = horizontal corridor
		{
			int startTile = -1, endTile = -1;
			y = currentCorridor->start.Y;
			for (x = currentCorridor->start.X; x <= currentCorridor->end.X; x += TileSize)
			{
				tileIndex = (x / TileSize) + tilesDungeon * (y / TileSize);
				if (TileArray.IsValidIndex(tileIndex) && TileArray[tileIndex].objectsToSpawn.Num() == 0)
				{
					TileArray[tileIndex].tileType = ETileType::CORRIDOR;
					TileArray[tileIndex].objectsToSpawn.Add(FDungeonObject()); //floor
					TileArray[tileIndex].left = x;
					TileArray[tileIndex].bottom = y;
				}
			}
		}
		else if (currentCorridor->seperation == ESeperation::HORIZONTAL)//horizontal seperation = vertical corridor
		{
			int startTile = -1, endTile = -1;
			x = currentCorridor->start.X;
			for (y = currentCorridor->start.Y; y >= currentCorridor->end.Y; y -= TileSize)
			{
				tileIndex = (x / TileSize) + tilesDungeon * (y / TileSize);
				if (TileArray.IsValidIndex(tileIndex) && TileArray[tileIndex].objectsToSpawn.Num() == 0)
				{
					TileArray[tileIndex].tileType = ETileType::CORRIDOR;
					TileArray[tileIndex].objectsToSpawn.Add(FDungeonObject()); //floor
					TileArray[tileIndex].left = x;
					TileArray[tileIndex].bottom = y;
				}
			}
		}
	}

	//add other objectsToSpawn to rooms
	for (int i = 0; i < DungeonRooms.Num(); i++)
	{
		left = DungeonRooms[i]->data.left;
		right = DungeonRooms[i]->data.left + DungeonRooms[i]->data.width;
		bottom = DungeonRooms[i]->data.bottom;
		top = DungeonRooms[i]->data.bottom + DungeonRooms[i]->data.height;

		for (int row = bottom; row < top; row += TileSize)
		{
			for (int col = left; col < right; col += TileSize) {

				tileIndex = (col / TileSize) + tilesDungeon * (row / TileSize);
				if (TileArray.IsValidIndex(tileIndex))
				{
					PlaceWalls(tileIndex);
				}

			}

		}

	}

	//add other objects to corridors (walls)
	for (auto& elem : DungeonCorridors)
	{
		FCorridor* currentCorridor = elem.Value;
		int x, y;
		if (currentCorridor->seperation == ESeperation::VERTICAL) //vertical seperation = horizontal corridor
		{
			int startTile = -1, endTile = -1;
			y = currentCorridor->start.Y;
			for (x = currentCorridor->start.X; x <= currentCorridor->end.X; x += TileSize)
			{
				tileIndex = (x / TileSize) + tilesDungeon * (y / TileSize);
				if (TileArray.IsValidIndex(tileIndex) && TileArray[tileIndex].tileType == ETileType::CORRIDOR)
				{
					PlaceWalls(tileIndex);
				}
			}
		}
		else if (currentCorridor->seperation == ESeperation::HORIZONTAL)//horizontal seperation = vertical corridor
		{
			int startTile = -1, endTile = -1;
			x = currentCorridor->start.X;
			for (y = currentCorridor->start.Y; y >= currentCorridor->end.Y; y -= TileSize)
			{
				tileIndex = (x / TileSize) + tilesDungeon * (y / TileSize);
				if (TileArray.IsValidIndex(tileIndex) && TileArray[tileIndex].tileType == ETileType::CORRIDOR)
				{
					PlaceWalls(tileIndex);
				}
			}
		}
	}

}

void ADungeonSpace::ConstructDungeonGrid()
{
	int rows = DungeonSize / TileSize;
	int tileIndex;
	FTransform dungeonTileTranform = GetTransform();
	UInstancedStaticMeshComponent* meshISMCToAddInstance = nullptr;
	int objectWidth;
	uint32 newInstanceIndex;

	for (int row = 0; row < rows; row++)
	{
		for (int col = 0; col < rows; col++)
		{
			tileIndex = col + rows * row;
			//Check if index is valid and tile is not empty
			if (TileArray.IsValidIndex(tileIndex) && TileArray[tileIndex].tileType != ETileType::EMPTY)
			{
				//create instances for all objectsToSpawn on the tile
				//loop over dungeon objectsToSpawn to create instances of meshes
				if (TileArray[tileIndex].objectsToSpawn.Num() > 0)
				{
					int left, bottom;
					for (int i = 0; i < TileArray[tileIndex].objectsToSpawn.Num(); i++)
					{
						left = TileArray[tileIndex].left;
						bottom = TileArray[tileIndex].bottom;
						//change ISMC depending on object type and the object width (helps with alighning object)
						switch (TileArray[tileIndex].objectsToSpawn[i].objectType)
						{
						case EDungeonObjectType::FLOOR:
							meshISMCToAddInstance = FloorTileISMC;
							objectWidth = 0;
							break;
						case EDungeonObjectType::WALL:
							meshISMCToAddInstance = WallTileISMC;
							objectWidth = WallTileWidth;
						case EDungeonObjectType::CEILING:
							break;
						case EDungeonObjectType::PILLAR:
							break;
						case EDungeonObjectType::TORCH:
							break;
						}

						//change transform to alignment of object
						FVector rotationVector = TileArray[tileIndex].objectsToSpawn[i].rotation;
						float customDataValue = 0.7f;
						switch (TileArray[tileIndex].objectsToSpawn[i].objectAlignement)
						{
						case EDungeonObjectAlign::LEFT:
							dungeonTileTranform.SetLocation(FVector(left + TileSize, bottom + TileSize / 2, 0));
							dungeonTileTranform.SetRotation(rotationVector.Rotation().Quaternion());
							customDataValue = 0.2f;
							break;
						case EDungeonObjectAlign::RIGHT:
							dungeonTileTranform.SetLocation(FVector(left, bottom + TileSize / 2, 0));
							dungeonTileTranform.SetRotation(rotationVector.Rotation().Quaternion());
							customDataValue = 0.2f;
							break;
						case EDungeonObjectAlign::TOP:
							dungeonTileTranform.SetLocation(FVector(left + TileSize / 2, bottom + TileSize, 0));
							dungeonTileTranform.SetRotation(rotationVector.Rotation().Quaternion());
							customDataValue = 0.7f;
							break;
						case EDungeonObjectAlign::BOTTOM:
							dungeonTileTranform.SetLocation(FVector(left + TileSize / 2, bottom, 0));
							dungeonTileTranform.SetRotation(rotationVector.Rotation().Quaternion());
							customDataValue = 0.7f;
							break;
						case EDungeonObjectAlign::CENTER:
							dungeonTileTranform.SetLocation(FVector(left + TileSize / 2, bottom + TileSize / 2, 0));
							dungeonTileTranform.SetRotation(rotationVector.Rotation().Quaternion());
							break;
						}

						if (meshISMCToAddInstance != nullptr)
						{
							newInstanceIndex = meshISMCToAddInstance->AddInstance(dungeonTileTranform);
							meshISMCToAddInstance->SetCustomDataValue(newInstanceIndex, 0, customDataValue, true);
						}

						meshISMCToAddInstance = nullptr;
					}
				}
			}
		}
	}
}

void ADungeonSpace::ShrinkSpaceToRoom(FSpace* currentSpace)
{
	if (currentSpace != nullptr)
	{
		//check if there are spare tiles
		int extraTilesInWidth = (currentSpace->data.width / TileSize) - MinTilesPerRoom;
		extraTilesInWidth = std::min(extraTilesInWidth, (currentSpace->data.width / TileSize / 2));
		if (extraTilesInWidth > 1)
		{
			extraTilesInWidth = FMath::RandRange(1, extraTilesInWidth);
			currentSpace->data.width -= extraTilesInWidth * TileSize;
			if (extraTilesInWidth % 2 == 1)
				extraTilesInWidth = -1;
			currentSpace->data.left += (extraTilesInWidth / 2) * TileSize;
		}


		int extraTilesInHeight = (currentSpace->data.height / TileSize) - MinTilesPerRoom;
		extraTilesInHeight = std::min(extraTilesInHeight, (currentSpace->data.height / TileSize) / 2);
		if (extraTilesInHeight > 1)
		{
			extraTilesInHeight = FMath::RandRange(1, extraTilesInHeight);
			currentSpace->data.height -= extraTilesInHeight * TileSize;
			if (extraTilesInHeight % 2 == 1)
				extraTilesInHeight = -1;
			currentSpace->data.bottom += (extraTilesInHeight / 2) * TileSize;
		}
	}
}

bool ADungeonSpace::CheckIfWallShouldBePlaced(int tileIndex, int adjacentTileIndex)
{
	EDungeonObjectAlign adjacentTileAlignment = EDungeonObjectAlign::RIGHT;
	//check if the adjacent tile is not in grid -> place wall
	if (!TileArray.IsValidIndex(adjacentTileIndex))
		return true;

	//put wall if adjacent tile is empty
	if (TileArray[adjacentTileIndex].tileType == ETileType::EMPTY)
		return true;

	return false;
}

bool ADungeonSpace::IsCorridorConnected(int tileIndex)
{
	//check for 2 connections
	int connections = 0;

	//adjacent tile LEFT
	int adjacentTileIndex = tileIndex - 1;
	if (TileArray.IsValidIndex(adjacentTileIndex) && TileArray[adjacentTileIndex].tileType != ETileType::EMPTY)
	{
		connections++;
	}

	//adjacent tile RIGHT
	adjacentTileIndex = tileIndex + 1;
	if (TileArray.IsValidIndex(adjacentTileIndex) && TileArray[adjacentTileIndex].tileType != ETileType::EMPTY)
	{
		connections++;
	}

	//adjacent tile TOP
	adjacentTileIndex = tileIndex + TileRows;
	if (TileArray.IsValidIndex(adjacentTileIndex) && TileArray[adjacentTileIndex].tileType != ETileType::EMPTY)
	{
		connections++;
	}

	//adjacent tile BOT
	adjacentTileIndex = tileIndex - TileRows;
	if (TileArray.IsValidIndex(adjacentTileIndex) && TileArray[adjacentTileIndex].tileType != ETileType::EMPTY)
	{
		connections++;
	}

	return connections > 1;
}

void ADungeonSpace::ShowDebugTile(int tileIndex, FString& tileInfo, FColor colorBox)
{
	if (TileArray.IsValidIndex(tileIndex))
	{
		FVector centerTile{ float(TileArray[tileIndex].left + TileSize / 2),  float(TileArray[tileIndex].bottom + TileSize / 2), GetActorLocation().Z };
		switch (TileArray[tileIndex].tileType)
		{
		case ETileType::EMPTY:
			tileInfo.Append(TEXT("EMPTY)"));
			break;
		case ETileType::CORRIDOR:
			tileInfo.Append(TEXT("CORRIDOR)"));
			break;
		case ETileType::ROOM:
			tileInfo.Append(TEXT("ROOM)"));
			break;
		}
		tileInfo.Append(TEXT(", tileID(")).Append(FString::FromInt(tileIndex)).Append(TEXT(")"));
		DrawDebugBox(GetWorld(), centerTile, FVector(TileSize / 2, TileSize / 2, 100.f), colorBox, true, 15.f, 0, 5.f);
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 10.f, colorBox, tileInfo);
	}

}

void ADungeonSpace::PlaceWalls(int tileIndex)
{
	int adjacentTileIndex = 0;
	
	//LEFT  	//check if first in row
	adjacentTileIndex = tileIndex + 1;
	if (CheckIfWallShouldBePlaced(tileIndex, adjacentTileIndex) || tileIndex % TileRows == TileRows - 1)
	{
		FDungeonObject dObject = FDungeonObject(EDungeonObjectType::WALL, EDungeonObjectAlign::LEFT, FVector(1, 0, 0));
		TileArray[tileIndex].objectsToSpawn.Add(dObject);
	}

	//RIGHT 	//check if last in row
	adjacentTileIndex = tileIndex - 1;
	if (CheckIfWallShouldBePlaced(tileIndex, adjacentTileIndex) || tileIndex % TileRows == 0)
	{
		FDungeonObject dObject = FDungeonObject(EDungeonObjectType::WALL, EDungeonObjectAlign::RIGHT, FVector(1, 0, 0));
		TileArray[tileIndex].objectsToSpawn.Add(dObject);
	}

	//TOP
	adjacentTileIndex = tileIndex + TileRows;
	if (CheckIfWallShouldBePlaced(tileIndex, adjacentTileIndex))
	{
		FDungeonObject dObject = FDungeonObject(EDungeonObjectType::WALL, EDungeonObjectAlign::TOP, FVector(0, -1, 0));
		TileArray[tileIndex].objectsToSpawn.Add(dObject);
	}

	//BOTTOM
	adjacentTileIndex = tileIndex - TileRows;
	if (CheckIfWallShouldBePlaced(tileIndex, adjacentTileIndex))
	{
		FDungeonObject dObject = FDungeonObject(EDungeonObjectType::WALL, EDungeonObjectAlign::BOTTOM, FVector(0, -1, 0));
		TileArray[tileIndex].objectsToSpawn.Add(dObject);
	}

	
}

void ADungeonSpace::ResetDungeon()
{
	TileArray.Init(FTile(), TileRows * TileRows);
	CubeISMC->ClearInstances();
	FloorTileISMC->ClearInstances();
	WallTileISMC->ClearInstances();
	DungeonRooms.Empty();
	DungeonCorridors.Empty();
}

void ADungeonSpace::MoveSpawnPlatform()
{
	
	TArray<AActor*> SpawnPlatforms;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpawnPlatform::StaticClass(), SpawnPlatforms);
	FVector newSpawnPos{};
	FVector dungeonSpaceTranform = GetActorLocation();
	newSpawnPos.X = dungeonSpaceTranform.X + (TileSize * TileRows) / 2;
	newSpawnPos.Y = dungeonSpaceTranform.Y + (TileSize * TileRows) / 2;
	newSpawnPos.Z = dungeonSpaceTranform.Z + (TileSize * TileRows) / 2;

	FString info{};
	info.Append(TEXT("New spawn pos: ")).Append(FString::FromInt(newSpawnPos.X).Append(TEXT(", ")));
	info.Append(FString::FromInt(newSpawnPos.Y).Append(TEXT(", ")));
	info.Append(FString::FromInt(newSpawnPos.Z));

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Emerald, info);

	if (SpawnPlatforms.Num() > 0)
	{
		ASpawnPlatform* spawnPlatform = ((ASpawnPlatform*)SpawnPlatforms[0]);
		if (IsValid(spawnPlatform))
		{
			spawnPlatform->SetActorLocation(newSpawnPos);
		}
	}

	ACharacter* character = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if(character != nullptr)
	{
		newSpawnPos.Z += 200.f;
		character->SetActorLocation(newSpawnPos);
	}
}

// Called every frame
void ADungeonSpace::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

