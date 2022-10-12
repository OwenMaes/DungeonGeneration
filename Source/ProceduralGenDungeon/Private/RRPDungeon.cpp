// Fill out your copyright notice in the Description page of Project Settings.


#include "RRPDungeon.h"
#include "DrawDebugHelpers.h"
#include "Components/InstancedStaticMeshComponent.h"
#include <Runtime\Engine\Classes\Kismet\KismetMathLibrary.h>

// Sets default values
ARRPDungeon::ARRPDungeon()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	FloorTileISMC = CreateDefaultSubobject<class UInstancedStaticMeshComponent>(TEXT("Floor InstancedStaticMesh"));
	FloorTileISMC->SetMobility(EComponentMobility::Static);
	FloorTileISMC->SetCollisionProfileName("BlockAll");

	WallTileISMC = CreateDefaultSubobject<class UInstancedStaticMeshComponent>(TEXT("Wall InstancedStaticMesh"));
	WallTileISMC->SetMobility(EComponentMobility::Static);
	WallTileISMC->SetCollisionProfileName("BlockAll");
}

void ARRPDungeon::GenerateDungeon()
{
	if (!IsDungeonGenerating) {
		IsDungeonGenerating = true;
		ResetDungeon();
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-2, 2.f, FColor::Green, TEXT("Generating RRPDungeon..."));

		//TileNodes
		GenerateRooms();
		ContructTileNodeGrid();
		AttachTileNodesToRooms();

		//Corridors
		RandomRoomConnect();

		//Meshes
		SpawnInstancedMeshes();

		if (IsDrawingDebug) {
			DrawDebugTiles(5.f);
		}
		IsDungeonGenerating = false;
	}
}

// Called when the game starts or when spawned
void ARRPDungeon::BeginPlay()
{
	Super::BeginPlay();

}

FVector ARRPDungeon::GetRandomPointInCircle()
{
	FVector randomPoint{};
	randomPoint.X = DungeonCentralPosition.X + FMath::Cos(FMath::RandRange(0.f, PI * 2.f)) * FMath::RandRange(1.f, DungeonRadius);
	randomPoint.Y = DungeonCentralPosition.X + FMath::Sin(FMath::RandRange(0.f, PI * 2.f)) * FMath::RandRange(1.f, DungeonRadius);
	randomPoint.Z = DungeonCentralPosition.Z;

	return randomPoint;
}

void ARRPDungeon::SeperateRooms()
{
	HasOverlap = false;
	bool areRoomsOverlapping = true;
	TArray<FRoom> overlappingRooms{};
	FRoom otherSquareRoom{};
	FRoom currentSquareRoom{};
	float highestValue{};
	//while
	while (areRoomsOverlapping)
	{
		areRoomsOverlapping = false;
		for (auto& currentRoom : ArrayOfRooms)
		{
			highestValue = FMath::Max(currentRoom.Width, currentRoom.Height);
			currentSquareRoom.CentralPosition = currentRoom.CentralPosition;
			currentSquareRoom.Width = highestValue;
			currentSquareRoom.Height = highestValue;

			overlappingRooms.Empty();
			//A: get all overlapping rooms
			for (auto& otherRoom : ArrayOfRooms)
			{
				if (currentRoom.RoomID == otherRoom.RoomID)
					continue;

				//Change width and height to highest value -> square room
				highestValue = FMath::Max(otherRoom.Width, otherRoom.Height);
				otherSquareRoom.CentralPosition = otherRoom.CentralPosition;
				otherSquareRoom.Width = highestValue;
				otherSquareRoom.Height = highestValue;

				if (AreRoomsOverlapping(currentSquareRoom, otherSquareRoom, RoomTileSize)) {
					overlappingRooms.Add(otherSquareRoom);
					areRoomsOverlapping = true;
				}
			}

			if (overlappingRooms.Num() == 0)
				continue;

			//B: Get average direction of all overlapping rooms to current room and move in that direction
			FVector averageVelocity{};
			for (auto& overlappingRoom : overlappingRooms)
			{
				averageVelocity += currentRoom.CentralPosition - overlappingRoom.CentralPosition;
			}
			averageVelocity /= overlappingRooms.Num();
			averageVelocity.Normalize();
			averageVelocity *= RoomTileSize;

			currentRoom.CentralPosition += averageVelocity;

		}
	}

}

void ARRPDungeon::DrawDebugTiles(float timeDrawn)
{
	FVector extent{};
	FVector start{};
	FVector end{};
	float thickness = 50.f;
	for (auto& tile : TileNodeGrid)
	{
		DrawDebugString(GetWorld(), tile.Value.TilePosition, FString::FromInt(tile.Value.NodeID));

		extent.X = RoomTileSize / 2 - thickness;
		extent.Y = RoomTileSize / 2 - thickness;

		switch (tile.Value.TileNodeType)
		{
		case ETileNodeType::EMPTY:
			extent.Z = 50.f;
			DrawDebugBox(GetWorld(), tile.Value.TilePosition, extent, FColor::White, true, timeDrawn, 0, thickness);
			break;
		case ETileNodeType::ROOM:
			extent.Z = 150.f;
			DrawDebugBox(GetWorld(), tile.Value.TilePosition, extent, FColor::Blue, true, timeDrawn, 0, thickness);
			break;
		case ETileNodeType::CORRIDOR:
			extent.Z = 100.f;
			DrawDebugBox(GetWorld(), tile.Value.TilePosition, extent, FColor::Red, true, timeDrawn, 0, thickness);
			break;
		case ETileNodeType::DOOR:
			extent.Z = 150.f;
			DrawDebugBox(GetWorld(), tile.Value.TilePosition, extent, FColor::Green, true, timeDrawn, 0, thickness);
			break;
		default:
			break;
		}


		for (auto& conn : tile.Value.Connections)
		{
			if (auto fromNode = TileNodeGrid.Find(conn.FromNodeID))
				start = fromNode->TilePosition;
			else
				continue;
			if (auto toNode = TileNodeGrid.Find(conn.ToNodeID))
				end = toNode->TilePosition;
			else
				continue;

			if (0.f < conn.ConnectionCost && conn.ConnectionCost < CorridorConnectionCost)
				DrawDebugLine(GetWorld(), start, end, FColor::Black, true, timeDrawn, 0, 35.f);
			else if (conn.ConnectionCost < RoomConnectionCost)
				DrawDebugLine(GetWorld(), start, end, FColor::Purple, true, timeDrawn, 0, 35.f);
			else
				DrawDebugLine(GetWorld(), start, end, FColor::Yellow, true, timeDrawn, 0, 35.f);
		}
	}
}

void ARRPDungeon::ResetDungeon()
{
	TileNodeGrid.Empty();
	FloorTileISMC->ClearInstances();
	WallTileISMC->ClearInstances();
	CorridorTiles.Empty();
	CorridorTiles.Empty();
	DoorTiles.Empty();
	ArrayOfRooms.Empty();
}

void ARRPDungeon::ContructTileNodeGrid()
{
	//Create boundbox of rooms
	float width{};
	float height{};

	for (auto& room : ArrayOfRooms)
	{
		width = room.Width / 2;
		height = room.Height / 2;
		TopOfGrid = FMath::Min(TopOfGrid, room.CentralPosition.Y - height);
		BotOfGrid = FMath::Max(BotOfGrid, room.CentralPosition.Y + height);
		RightOfGrid = FMath::Max(RightOfGrid, room.CentralPosition.X + width);
		LeftOfGrid = FMath::Min(LeftOfGrid, room.CentralPosition.X - width);;
	}
	float gridPadding = RoomTileSize * 3;
	TopOfGrid -= gridPadding;
	BotOfGrid += gridPadding;
	LeftOfGrid -= gridPadding;
	RightOfGrid += gridPadding;

	//Create a grid of TileNodes from the boundbox
	NrOfGridCols = (RightOfGrid - LeftOfGrid) / RoomTileSize;
	NrOfGridRows = (BotOfGrid - TopOfGrid) / RoomTileSize;
	int tileIndex = 0;
	float x{}, y{};
	for (size_t row = 0; row < NrOfGridRows; row++)
	{
		for (size_t col = 0; col < NrOfGridCols; col++)
		{
			x = LeftOfGrid + col * RoomTileSize - RoomTileSize / 2;
			y = BotOfGrid - row * RoomTileSize + RoomTileSize / 2;
			FTileNode tileNode{};
			tileNode.NodeID = tileIndex;
			tileNode.TilePosition = { x, y, 0 };

			TileNodeGrid.Add(tileIndex, tileNode);
			tileIndex++;
		}
	}

	// Create connections for each node to adjacent nodes
	int adjacentNodeIdx{};
	int currentNodeIdx{};
	for (auto r = 0; r < NrOfGridRows; ++r)
	{
		for (auto c = 0; c < NrOfGridCols; ++c)
		{
			currentNodeIdx = r * NrOfGridCols + c;

			if (auto node = TileNodeGrid.Find(currentNodeIdx)) {
				for (auto dir : AdjacentDirections) //Left, Right, Top & Bottom
				{
					int adjCol = c + (int)dir.X;
					int adjRow = r + (int)dir.Y;

					if (0 <= adjCol && adjCol < NrOfGridCols && 0 <= adjRow && adjRow < NrOfGridRows)
					{
						adjacentNodeIdx = adjRow * NrOfGridCols + adjCol;
						if (auto adjacentNode = TileNodeGrid.Find(adjacentNodeIdx)) {
							node->Connections.Add({ currentNodeIdx, adjacentNodeIdx, EmptyTileConnectionCost });
						}
					}
				}
			}
		}
	}
}

bool ARRPDungeon::AreRoomsOverlapping(const FRoom& roomA, const FRoom& roomB, float margin) const
{
	float roomAWidth = (roomA.Width + margin) / 2.f;
	float roomAHeight = (roomA.Height + margin) / 2.f;
	float roomBWidth = (roomB.Width + margin) / 2.f;
	float roomBHeight = (roomB.Height + margin) / 2.f;


	if (roomA.CentralPosition.X - roomAWidth < roomB.CentralPosition.X + roomBWidth
		&& roomA.CentralPosition.X + roomAWidth > roomB.CentralPosition.X - roomBWidth
		&& roomA.CentralPosition.Y + roomAHeight > roomB.CentralPosition.Y - roomBHeight
		&& roomA.CentralPosition.Y - roomAHeight < roomB.CentralPosition.Y + roomBHeight)
		return true;

	return false;
}

void ARRPDungeon::SpawnInstancedMeshes()
{
	FTransform floorTransform{};
	FTransform wallTransform{};

	//Rooms
	TArray<ETileNodeType> tilesTypesToIgnore = { ETileNodeType::ROOM, ETileNodeType::DOOR };
	for (auto& currentRoom : ArrayOfRooms)
	{
		for (auto tile : currentRoom.TileNodesOfRoom)
		{
			SpawnMeshesOnTileNode(tile, floorTransform, wallTransform, tilesTypesToIgnore);
		}
	}

	//Corridors
	tilesTypesToIgnore = { ETileNodeType::CORRIDOR };
	for (auto& corridor : CorridorTiles)
	{
		for (auto tile : corridor.Value)
		{
			if (tile->TileNodeType == ETileNodeType::CORRIDOR)
				SpawnMeshesOnTileNode(tile, floorTransform, wallTransform, tilesTypesToIgnore);
		}
	}
}

void ARRPDungeon::SpawnMeshesOnTileNode(FTileNode* node, FTransform& floorTransform, FTransform& wallTransform, TArray<ETileNodeType>& tilesTypesToIgnore)
{
	//Spawn floor meshes
	floorTransform.SetLocation(node->TilePosition);
	FloorTileISMC->AddInstanceWorldSpace(floorTransform);

	//Spawn Wall meshes
	FVector position{ 0,0,0 };
	FRotator rot{};
	FTileNode* adjacentNode = nullptr;

	for (auto& dir : AdjacentDirections)
	{
		//Get adjacent node
		position = node->TilePosition + dir * RoomTileSize;
		adjacentNode = GetNodeFromPosition(position);

		//Calculate rotation and location
		rot = UKismetMathLibrary::FindLookAtRotation(dir, { 0,0,0 });
		wallTransform.SetRotation(rot.Quaternion());
		wallTransform.SetLocation(node->TilePosition + dir * (RoomTileSize / 2));

		//Check if position it out of the grid -> spawn wall
		if (!IsPositionInGrid(position)) {
			WallTileISMC->AddInstanceWorldSpace(wallTransform);
			continue;
		}

		//Check if adjacent tilenode type has to be blocked by a wall
		if (INDEX_NONE == tilesTypesToIgnore.Find(adjacentNode->TileNodeType)) {

			//Check if adjacent tile is a door tile and wall points towards door (blocking)
			if (adjacentNode->TileNodeType == ETileNodeType::DOOR) {
				if (IsNodeTileAndDoorFacingSameDirection(node, adjacentNode))
					continue;
			} //Check if node is Door and adjacent tile is corridor tile, check if door is facing opposite direction (blocking)
			else if (node->TileNodeType == ETileNodeType::DOOR && adjacentNode->TileNodeType == ETileNodeType::CORRIDOR) {
				if (IsNodeTileAndDoorFacingSameDirection(adjacentNode, node))
					continue;
			}

			auto intanceID = WallTileISMC->AddInstanceWorldSpace(wallTransform);

		}

	}
}

void ARRPDungeon::RandomRoomConnect()
{
	FRoom roomA{};
	FRoom roomB{};
	FVector velocity{};
	FVector direction{ 0,0,0 };
	FVector position{ };
	float xDistance{}, yDistance{};

	//Connect every room to the next room in the array
	for (size_t i = 0; i < ArrayOfRooms.Num() - 1; i++)
	{
		roomA = ArrayOfRooms[i];
		roomB = ArrayOfRooms[i + 1];

		auto startNode = GetNodeFromPosition(roomA.CentralPosition);
		auto endNode = GetNodeFromPosition(roomB.CentralPosition);

		if (!startNode || !endNode)
			continue;

		auto path = GetPathAStar(startNode, endNode);

		CreateCorridorFromPath(path);

	}
}

// Called every frame
void ARRPDungeon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ARRPDungeon::CreateCorridorFromPath(TArray<FTileNode*>& path)
{
	TArray<FTileNode*> corridor{};
	FTileNode* prevTileNode = nullptr;
	bool isDoorPlaced = false;
	int corridorID = CorridorTiles.Num();

	//Go through all the nodes of the path
	int pathIndex{};
	for (auto tileNode : path)
	{
		for (auto& con : tileNode->Connections)
		{
			if (auto adjacentNode = TileNodeGrid.Find(con.ToNodeID)) {
				if (adjacentNode->TileNodeType == ETileNodeType::CORRIDOR) {
					con.ConnectionCost = CorridorConnectionCost;

					adjacentNode->Connections.FilterByPredicate([tileNode](const FTileConnection& otherCon) {
						return tileNode->NodeID == otherCon.ToNodeID;
						});
				}
			}
		}

		switch (tileNode->TileNodeType)
		{
		case ETileNodeType::EMPTY: //Empty tiles change to corridor tiles
			//Check if start door is placed and if not change prev tile to a door tile
			if (!isDoorPlaced) {
				prevTileNode->TileNodeType = ETileNodeType::CORRIDOR;
				isDoorPlaced = true;
			}
			break;
		case ETileNodeType::ROOM:
			//If door is placed and we enter another room, tile becomes a door and setdoor resets
			if (isDoorPlaced) {
				CreateDoorTile(tileNode, path[pathIndex - 1], corridorID);
				isDoorPlaced = false;
			}
			break;
		case ETileNodeType::CORRIDOR:
			//Check if start door is placed and if not change prev tile to a door tile
			if (!isDoorPlaced) {
				CreateDoorTile(prevTileNode, path[pathIndex], corridorID);
				isDoorPlaced = true;
			}
			break;
		case ETileNodeType::DOOR:
			//If door is already placed, this is end door and if not already placed first door
			isDoorPlaced = !isDoorPlaced;
			break;
		default:
			break;
		}
		prevTileNode = tileNode;
		pathIndex++;

		if (tileNode->TileNodeType != ETileNodeType::ROOM)
			corridor.Add(tileNode);
	}

	CorridorTiles.Add(corridorID, corridor);
}

void ARRPDungeon::CreateDoorTile(FTileNode* currentNode, FTileNode* nextNode, int corridorID)
{
	FDoor door{};
	currentNode->TileNodeType = ETileNodeType::DOOR;
	door.CorridorID = corridorID;
	door.TileID = currentNode->NodeID;
	FVector direction = nextNode->TilePosition - currentNode->TilePosition;
	float epsilon = 0.005f;
	if (direction.X > epsilon || direction.X < -epsilon)
		direction.X /= abs(direction.X);
	if (direction.Y > epsilon || direction.Y < -epsilon)
		direction.Y /= abs(direction.Y);
	door.Direction = direction;
	DoorTiles.Add(currentNode->NodeID, door );
}

TArray<FTileNode*> ARRPDungeon::GetPathAStar(FTileNode* startNode, FTileNode* endNode)
{
	TArray<FTileNode*> path{};
	TArray<FTileNodeRecord> openList{};
	TArray<FTileNodeRecord> closedList{};
	FTileNodeRecord currentTileNodeRecord{};
	currentTileNodeRecord.EstimatedTotalCost = FLT_MAX;

	//Create a TileNodeRecord to start the loop
	FTileNodeRecord startRecord{};
	startRecord.TileNode = startNode;
	startRecord.EstimatedTotalCost = GetHeuristicCost(startNode, endNode) / RoomTileSize;
	openList.Add(startRecord);

	while (openList.Num() > 0) {
		//Get NodeRecord with lowest cost from openList
		currentTileNodeRecord = FMath::Min(openList);

		//Check if NodeRecord points to the goal
		if (currentTileNodeRecord.TileNode == endNode)
			break;

		//Loop through all the connections of the NodeRecord node
		for (auto con : currentTileNodeRecord.TileNode->Connections)
		{
			auto nodeFromCon = TileNodeGrid.Find(con.ToNodeID);

			//Calculate the total cost so far (G-cost)
			FTileNodeRecord newTileNodeRecord{};
			newTileNodeRecord.TileNode = nodeFromCon;
			newTileNodeRecord.CostSoFar = currentTileNodeRecord.CostSoFar + con.ConnectionCost;
			newTileNodeRecord.Connection = con;
			newTileNodeRecord.EstimatedTotalCost = newTileNodeRecord.CostSoFar + GetHeuristicCost(newTileNodeRecord.TileNode, endNode);

			//Check if the node from the connection is in the closed list
			auto foundNodeRecInClosedList = closedList.FindByPredicate([nodeFromCon](const FTileNodeRecord& tileNodeRecord)
				{
					return tileNodeRecord.TileNode->NodeID == nodeFromCon->NodeID;
				}
			);

			if (foundNodeRecInClosedList) //If so remove existing connection if the new connection is cheaper
			{
				if (newTileNodeRecord < *foundNodeRecInClosedList)
					closedList.Remove(*foundNodeRecInClosedList);
			}
			else { //Check failed, check if any of those connections lead to a node already on the open list

				auto foundNodeRecInOpenList = openList.FindByPredicate([nodeFromCon](const FTileNodeRecord& tileNodeRecord)
					{
						return tileNodeRecord.TileNode->NodeID == nodeFromCon->NodeID;
					}
				);

				if (foundNodeRecInOpenList) { //- If so remove existing connection if the new connection is cheaper
					if (newTileNodeRecord < *foundNodeRecInOpenList)
						openList.Remove(*foundNodeRecInOpenList);
				}

				//At this point any expensive connection should be removed (if it existed). 
				//We create a new nodeRecord and add it to the openList
				openList.Add(newTileNodeRecord);

			}

		}

		//Remove NodeRecord from the openList and add it to the closedList
		openList.Remove(currentTileNodeRecord);
		closedList.Add(currentTileNodeRecord);
	}

	//Reconstruct path from last connection to start node
	FTileNode* currentTileNode{};
	while (currentTileNodeRecord.TileNode->NodeID != startNode->NodeID)
	{
		path.Add(currentTileNodeRecord.TileNode);
		if (currentTileNodeRecord.TileNode->TileNodeType == ETileNodeType::EMPTY)
			currentTileNodeRecord.TileNode->TileNodeType = ETileNodeType::CORRIDOR;

		if (currentTileNodeRecord.Connection.FromNodeID != -1
			&& currentTileNodeRecord.Connection.ToNodeID != -1)
			currentTileNode = TileNodeGrid.Find(currentTileNodeRecord.Connection.FromNodeID);
		else
			break;

		auto foundNodeRecInClosedList = closedList.FindByPredicate([currentTileNode](const FTileNodeRecord& tileNodeRecord)
			{
				return tileNodeRecord.TileNode->NodeID == currentTileNode->NodeID;
			}
		);

		if (foundNodeRecInClosedList)
			currentTileNodeRecord = *foundNodeRecInClosedList;
		else
			break;


	}

	return path;
}

float ARRPDungeon::GetHeuristicCost(FTileNode* startNode, FTileNode* endNode) {

	float heuristicCost{};
	FVector toDestination = endNode->TilePosition - startNode->TilePosition;
	float x = abs(toDestination.X);
	float y = abs(toDestination.Y);
	float f{};

	switch (HeuresticCostFunction)
	{
	case EHeuristicCost::MANHATTAN:
		return float(x + y);
		break;
	case EHeuristicCost::EUCLIDEAN:
		return float(FMath::Square(x * x + y * y));
		break;
	case EHeuristicCost::SQRTEUCLIDEAN:
		return float(x * x + y * y);
		break;
	case EHeuristicCost::OCTILE:
		f = 0.414213562373095048801f; // == sqrt(2) - 1;
		return float((x < y) ? f * x + y : f * y + x);
		break;
	case EHeuristicCost::CHEBYSHEV:
		return FMath::Max(x, y);
		break;
	default:
		return 0.f;
		break;
	}
}

bool ARRPDungeon::IsPositionInGrid(const FVector& pos) const
{
	if (LeftOfGrid <= pos.X && pos.X <= RightOfGrid
		&& TopOfGrid <= pos.Y && pos.Y <= BotOfGrid)
		return true;
	return false;
}

bool ARRPDungeon::IsNodeTileAndDoorFacingSameDirection(FTileNode* node, FTileNode* doorNode) const
{
	FVector directionToDoor{};
	if (auto door = DoorTiles.Find(doorNode->NodeID))
	{
		directionToDoor = node->TilePosition - doorNode->TilePosition;
		directionToDoor.Normalize();
		bool isDoorFacingSameDirection = int(directionToDoor.X) == int(door->Direction.X)
			&& int(directionToDoor.Y) == int(door->Direction.Y);
		if (isDoorFacingSameDirection)
			return true;
	}
	return false;
}

FTileNode* ARRPDungeon::GetNodeFromPosition(const FVector& pos)
{
	float x = pos.X;
	float y = pos.Y;

	if (LeftOfGrid < 0.f)
		x += abs(LeftOfGrid);
	if (BotOfGrid > 0.f)
		y -= abs(BotOfGrid);
	if (y < 0.f) //The y is inverted, -y is top and +y is bot
		y *= -1.f;

	int c = int(x / RoomTileSize) + 1;
	c = FMath::Clamp(c, 0, NrOfGridCols - 1);
	int r = int(y / RoomTileSize) + 1;
	r = FMath::Clamp(r, 0, NrOfGridRows - 1);
	int idx = r * NrOfGridCols + c;

	if (auto foundNode = TileNodeGrid.Find(idx))
		return foundNode;

	return nullptr;
}

void ARRPDungeon::GenerateRooms()
{
	int currentNrOfRooms = ArrayOfPremadeRooms.Num();
	for (size_t i = 0; i < currentNrOfRooms; i++)
	{
		ArrayOfRooms.Add(ArrayOfPremadeRooms[i]);
	}

	for (size_t i = currentNrOfRooms; i < NrOfRooms; i++)
	{
		FRoom room{};
		room.RoomID = i;
		room.Width = FMath::RandRange(MinRoomTiles, MaxRoomTiles) * RoomTileSize;
		room.Height = FMath::RandRange(MinRoomTiles, MaxRoomTiles) * RoomTileSize;
		room.CentralPosition = GetRandomPointInCircle();
		ArrayOfRooms.Add(room);
	}

	SeperateRooms();
}

void ARRPDungeon::AttachTileNodesToRooms()
{
	float left{}, right{}, top{}, bot{};
	int col{}, row{};
	FVector positionInRoom{ 0,0,0 };

	for (auto& currentRoom : ArrayOfRooms) {
		//Calculate left, right, bot & top
		left = currentRoom.CentralPosition.X - currentRoom.Width / 2.f;
		right = currentRoom.CentralPosition.X + currentRoom.Width / 2.f;
		top = currentRoom.CentralPosition.Y - currentRoom.Height / 2.f;
		bot = currentRoom.CentralPosition.Y + currentRoom.Height / 2.f;

		//Loop through tiles of room
		for (float x = left; x < right; x += RoomTileSize)
		{
			for (float y = bot; y > top; y -= RoomTileSize) {
				//DEBUG

				positionInRoom.X = x + RoomTileSize / 2.f;
				positionInRoom.Y = y - RoomTileSize / 2.f;

				//Find valid node by using position2node
				if (auto node = GetNodeFromPosition(positionInRoom)) {
					node->TileNodeType = ETileNodeType::ROOM;
					currentRoom.TileNodesOfRoom.Add(node);

					//Change connection cost of room tile to and from
					for (auto& con : node->Connections)
					{
						if (auto adjacentNode = TileNodeGrid.Find(con.ToNodeID)) {
							//Change connection cost to adjacent node if also room tile
							if (adjacentNode->TileNodeType == ETileNodeType::ROOM)
								con.ConnectionCost = RoomConnectionCost;

							//Find connection back to original node
							auto connectionFrom = adjacentNode->Connections.FindByPredicate([node](const FTileConnection& connection) {
								return node->NodeID == connection.ToNodeID;
								});

							if (connectionFrom)
								connectionFrom->ConnectionCost = RoomConnectionCost;
						}
					}
				}
			}
		}


	}
}
 
