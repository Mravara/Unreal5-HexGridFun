// Fill out your copyright notice in the Description page of Project Settings.


#include "HexGridManager.h"

#include "Hex.h"
#include "LineTypes.h"
#include "UOCTestGameMode.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AHexGridManager::AHexGridManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AHexGridManager::BeginPlay()
{
	Super::BeginPlay();

	AUOCTestGameMode* GameMode = Cast<AUOCTestGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	GameMode->GridManager = this;

    // Populate materials map
    Materials = decltype(Materials)
    {
        { EHexTypes::Invalid, InvalidMaterial },
        { EHexTypes::Blocked, BlockedMaterial },
        { EHexTypes::Dirt, DirtMaterial },
        { EHexTypes::Grass, GrassMaterial },
        { EHexTypes::Water, WaterMaterial },
    };

	// init vars
	TileWidth = OuterTileSize * 2.f;

	TileHeight = InnerTileSize * 2.f;

	HorizontalTileSpacing = TileWidth * (3.f / 4.f);

	VerticalTileSpacing = TileHeight / 2.f;

	// generate grid
	GenerateGrid();
}

void AHexGridManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// UE::Geometry::FLine3d line = UE::Geometry::FLine3d();
	
}

void AHexGridManager::GenerateGrid()
{
	UE_LOG(LogTemp, Warning, TEXT("HorizontalTileSpacing %f"), HorizontalTileSpacing);
	UE_LOG(LogTemp, Warning, TEXT("VerticalTileSpacing %f"), VerticalTileSpacing);
	UE_LOG(LogTemp, Warning, TEXT("TileWidth %f"), TileWidth);
	UE_LOG(LogTemp, Warning, TEXT("TileHeight %f"), TileHeight);
	UE_LOG(LogTemp, Warning, TEXT("OuterTileSize %f"), OuterTileSize);
	UE_LOG(LogTemp, Warning, TEXT("InnerTileSize %f"), InnerTileSize);

    // Generate grid and add the HexTileMap
	for (int q = LeftCount; q <= RightCount; q++)
	{
		const int QOffset = floor(q/2.f);
		for (int r = UpCount - QOffset; r <= DownCount - QOffset; r++)
		{
		    // Create hex
			Hex hex = Hex(q, r, -q-r);

		    // Get world location and rotation
			Point SpawnLocation = HexToWorldPoint(hex);
			float YawRotation = IsFlatTopLayout ? 30.f : 0.f;

		    // Instantiate blueprint on location
		    AHexTile* Tile = GetWorld()->SpawnActor<AHexTile>(HexTile, FVector(SpawnLocation.X, SpawnLocation.Y, 0.f), FRotator(0.f, YawRotation, 0.f));
			Tile->SetActorLabel(FString::Printf(TEXT("Tile_%d_%d_%d"), q, r, -q-r));
		    Tile->Init(Materials[EHexTypes::Grass]);

		    // Save to map for future use
			HexTileMap[hex] = Tile;
		}
	}
}

std::vector<Hex> AHexGridManager::GetNeighbors(const Hex& H)
{
    std::vector<Hex> Neighbors;
    for (auto Direction : DirectionVectors)
    {
        Hex TmpHex = Add(H, Direction);
        if (HexTileMap.count(TmpHex) > 0)
        {
            Neighbors.push_back(TmpHex);
        }
    }

    return Neighbors;
}

Hex AHexGridManager::GetHexDirection(const Hex& From, const Hex& To)
{
    return Subtract(To, From);
}

FVector AHexGridManager::GetVectorDirection(const Hex& From, const Hex& To)
{
    return FVector(To.Q - From.Q, To.R - From.R, To.S - From.S);
}

UMaterialInstance* AHexGridManager::GetMaterial(EHexTypes Type)
{
    if (Materials.count(Type))
    {
        return Materials[Type];
    }
    else
    {
        return Materials[EHexTypes::Invalid];
    }
}

AHexTile* AHexGridManager::GetTileByHex(Hex& H)
{
	if (HexTileMap.count(H))
	{
		return HexTileMap[H];
	}

	return nullptr;
}

Point AHexGridManager::HexToWorldPoint(const Hex Tile) const
{
	const float Right = Tile.Q * HorizontalTileSpacing; // 150
	const float Up = VerticalTileSpacing * Tile.Q + Tile.R * VerticalTileSpacing * 2.f; // 86.6

	// His way
	// Right = OuterTileSize * (3./2 * Tile.Q);
	// Up = -OuterTileSize * (sqrt(3)/2 * Tile.Q + sqrt(3) * Tile.R);

	return Point(Up, Right);
}

FVector AHexGridManager::HexToWorldLocation(const Hex Tile) const
{
	const float Right = Tile.Q * HorizontalTileSpacing; // 150
	const float Up = VerticalTileSpacing * Tile.Q + Tile.R * VerticalTileSpacing * 2.f; // 86.6

	// His way
	// Right = OuterTileSize * (3./2 * Tile.Q);
	// Up = -OuterTileSize * (sqrt(3)/2 * Tile.Q + sqrt(3) * Tile.R);

	return FVector(Up, Right, 0.f);
}

Hex AHexGridManager::WorldToHex(const FVector& Location) const
{
	// const float Right = Tile.Q * HorizontalTileSpacing; // 150
	// const float Up = -VerticalTileSpacing * Tile.Q + Tile.R * -VerticalTileSpacing * 2.f; // 86.6
	//
	// // His way
	// // Right = OuterTileSize * (3./2 * Tile.Q);
	// // Up = -OuterTileSize * (sqrt(3)/2 * Tile.Q + sqrt(3) * Tile.R);
	//
	// return Point(Up, Right);
	return HexRound(LocationToFractionalHex(Location));
}

FractionalHex AHexGridManager::LocationToFractionalHex(const FVector& Location) const
{
	Point pt = Point(Location.Y / OuterTileSize, Location.X / OuterTileSize);
	double q = (2.0 / 3.0) * pt.X;
    double r = (-1.0 / 3.0) * pt.X + sqrt(3.0) / 3.0 * pt.Y;
	return FractionalHex(q, r, -q - r);
}

FractionalHex AHexGridManager::LerpHex(Hex a, Hex b, double t)
{
    return FractionalHex(PreciseLerp(a.Q, b.Q, t),
                         PreciseLerp(a.R, b.R, t),
                         PreciseLerp(a.S, b.S, t));
}

float AHexGridManager::PreciseLerp(double a, double b, double t)
{
    return a * (1-t) + b * t;
    /* better for floating point precision than
       a + (b - a) * t, which is what I usually write */
}

std::vector<Hex> AHexGridManager::GetHexLine(const Hex& StartHex, const Hex& EndHex)
{
    const int HexDistance = Distance(StartHex, EndHex);
    
    std::vector<Hex> Results = {};
    double Step = 1.0 / FMath::Max(HexDistance, 1);

    for (int i = 0; i <= HexDistance; i++)
    {
        Results.push_back(HexRound(LerpHex(StartHex, EndHex, Step * i)));
    }

    return Results;
}

// custom approach
// std::vector<Hex> AHexGridManager::GetShortestPath(const Hex& Start, const Hex& End)
// {
//     std::list<Hex> ToSearch = { Start };
//     std::list<Hex> Processed;
//     std::vector<Hex> Path;
//     std::map<Hex, PathfindingInfo> InfoMap;
//     std::vector<Hex> Line = GetHexLine(Start, End);
//
//     while (!ToSearch.empty())
//     {
//         Hex Current = ToSearch.front();
//
//         // make sure it exists
//         if (!InfoMap.count(Current))
//         {
//             auto CurrentInfoIt = InfoMap.emplace(Current, PathfindingInfo());
//             PathfindingInfo& NeighborInfo = CurrentInfoIt.first->second;
//             NeighborInfo.SetToStartCost(Distance(Current, Start));
//             NeighborInfo.SetToEndCost(Distance(Current, End));
//         }
//         
//         // Find next
//         for (Hex Next : ToSearch)
//         {
//             // make sure it exists
//             if (InfoMap.count(Next) == 0)
//             {
//                 auto NextInfoIt = InfoMap.emplace(Current, PathfindingInfo());
//                 PathfindingInfo& NeighborInfo = NextInfoIt.first->second;
//                 NeighborInfo.SetToStartCost(Distance(Next, Start));
//                 NeighborInfo.SetToEndCost(Distance(Next, End));
//             }
//             
//             if (InfoMap[Next].GetTotalCost() < InfoMap[Current].GetTotalCost() ||
//                 InfoMap[Next].GetTotalCost() == InfoMap[Current].GetTotalCost() && InfoMap[Next].GetToEndCost() < InfoMap[Current].GetToEndCost())
//             {
//                 Current = Next;
//             }
//         }
//
//         Processed.push_back(Current);
//         ToSearch.remove(Current);
//
//         if (Current == End)
//         {
//             Hex CurrentPathHex = End;
//             int count = 100;
//             while (CurrentPathHex != Start)
//             {
//                 Path.push_back(CurrentPathHex);
//                 CurrentPathHex = InfoMap[CurrentPathHex].GetConnection();
//                 count--;
//                 if (count < 0)
//                 {
//                     GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, FString::Printf(TEXT("EARLY!!!!!!!!!!!: %d %d %d"), CurrentPathHex.Q, CurrentPathHex.R, CurrentPathHex.S));
//                     return Path;
//                 }
//             }
//             return Path;
//         }
//
//         if (Start != End)
//         {
//             for (Hex& Neighbor : GetNeighbors(Current))
//             {
//                 bool AlreadyProcessed = std::find(Processed.begin(), Processed.end(), Neighbor) != Processed.end(); 
//                 if (AlreadyProcessed)
//                     continue;
//             
//                 bool InSearchList = std::find(ToSearch.begin(), ToSearch.end(), Neighbor) != ToSearch.end();
//                 
//                 float CostToNeighbor = Distance(Start, Current) + ManhattanDistance(Current, Neighbor);
//
//                 if (!InSearchList || (CostToNeighbor < Distance(Neighbor, Start)))
//                 {
//                     auto NeighborInfoIt = InfoMap.emplace(Neighbor, PathfindingInfo());
//                     PathfindingInfo& NeighborInfo = NeighborInfoIt.first->second;
//
//                     // add tile cost
//                     NeighborInfo.SetTileCost(GetTileCost(Neighbor));
//
//                     // prefer the ones on the line
//                     if (std::find(Line.begin(), Line.end(), Neighbor) != Line.end())
//                     {
//                         NeighborInfo.SetBonusCost(-0.001f);
//                     }
//                     
//                     // fill info
//                     NeighborInfo.SetToStartCost(CostToNeighbor);
//                     NeighborInfo.SetConnection(Current);
//
//                     if (!InSearchList)
//                     {
//                         NeighborInfo.SetToEndCost(ManhattanDistance(Neighbor, End));
//                         ToSearch.push_back(Neighbor);
//                     }
//
//                     // add to map
//                     InfoMap[Neighbor] = NeighborInfo;
//                 }
//             }
//         }
//         
//     }
//
//     return Path;
// }

// red blob games
std::vector<Hex> AHexGridManager::GetShortestPath(const Hex& Start, const Hex& End)
{
    PriorityQueue Frontier;
    Frontier.put(Start, 0);

    std::vector<Hex> Path;
    std::unordered_map<Hex, Hex> CameFrom;
    std::unordered_map<Hex, double> CostSoFar;

    std::vector<Hex> Line = GetHexLine(Start, End);
    
    CameFrom[Start] = Start;
    CostSoFar[Start] = 0;

    // Find End Hex
    while (!Frontier.empty())
    {
        Hex Current = Frontier.get();

        if (Current == End)
        {
            break;
        }

        for (Hex Next : GetNeighbors(Current))
        {
            if (HexTileMap.count(Next) == 0)
            {
                continue;
            }

            AHexTile* Tile = GetTileByHex(Next);
            if (!Tile || Tile->TileType == EHexTypes::Invalid || Tile->TileType == EHexTypes::Blocked)
            {
                continue;
            }
            
            double NewCost = CostSoFar[Current] + GetHexCost(Next);

            if (std::find(Line.begin(), Line.end(), Current) != Line.end())
            {
                NewCost -= 0.0001;
            }

            if (CostSoFar.find(Next) == CostSoFar.end() || NewCost < CostSoFar[Next])
            {
                CostSoFar[Next] = NewCost;
                double Priority = NewCost + ManhattanDistance(Next, End);
                Frontier.put(Next, Priority);
                CameFrom[Next] = Current;
            }
        }
    }

    Hex Current = End;
    if (CameFrom.find(End) == CameFrom.end())
    {
        return Path; // no path can be found
    }

    // Generate path
    while (Current != Start)
    {
        Path.push_back(Current);
        Current = CameFrom[Current];
    }
    
    Path.push_back(Start); // optional
    std::reverse(Path.begin(), Path.end());

    Current = End;
    Path.push_back(Current);
    while (Current != Start)
    {
        Current = CameFrom[Current];
        Path.push_back(Current);
    }
    std::reverse(Path.begin(), Path.end());

    return Path;
}

Hex AHexGridManager::HexRound(const FractionalHex h) const
{
	int q = int(round(h.Q));
	int r = int(round(h.R));
	int s = int(round(h.S));
	double q_diff = abs(q - h.Q);
	double r_diff = abs(r - h.R);
	double s_diff = abs(s - h.S);
	if (q_diff > r_diff && q_diff > s_diff)
	{
		q = -r - s;
	}
	else if (r_diff > s_diff)
	{
		r = -q - s;
	}
	else
	{
		s = -q - r;
	}
	
	return Hex(q, r, s);
}

Hex AHexGridManager::Add(const Hex A, const Hex B)
{
	return Hex(A.Q + B.Q, A.R + B.R, A.S + B.S);
}

Hex AHexGridManager::Subtract(const Hex A, const Hex B)
{
	return Hex(A.Q - B.Q, A.R - B.R, A.S - B.S);
}

Hex AHexGridManager::Multiply(const Hex Tile, const int Multiplier)
{
	return Hex(Tile.Q * Multiplier, Tile.R * Multiplier, Tile.S * Multiplier);
}

Hex AHexGridManager::Divide(const Hex Tile, const int Divisor)
{
	return Hex(Tile.Q / Divisor, Tile.R / Divisor, Tile.S / Divisor);
}

int AHexGridManager::Length(const Hex Tile)
{
	return (
		int(FMath::Abs(Tile.Q) +
		FMath::Abs(Tile.R) +
		FMath::Abs(Tile.S)) / 2);
}

int AHexGridManager::ManhattanDistance(const Hex& A, const Hex& B)
{
    return FMath::Abs(A.Q - B.Q) + FMath::Abs(A.R - B.R) + FMath::Abs(A.S - B.S);
}

int AHexGridManager::Distance(const Hex& A, const Hex& B)
{
	return Length(Subtract(A, B));
}

std::vector<Hex> AHexGridManager::GetHexesInRange(const Hex StartingHex, const int Range) const
{
	// declare vector
	std::vector<Hex> Result;
	Result.reserve(GetHexCountForRange(Range));

	for (int q = -Range; q <= Range; q++)
	{
		const int r1 = std::max(-Range, -q - Range);
		const int r2 = std::min(Range, -q + Range);
		for (int r = r1; r <= r2; r++)
		{
			Result.push_back(Add(StartingHex, Hex(q, r)));
		}
	}

	return Result;
}

int AHexGridManager::GetHexCountForRange(const int Range)
{
	return 3 * Range * (Range + 1);
}

void AHexGridManager::SelectHexes(const std::vector<Hex>& Hexes)
{
    for (Hex Hex : Hexes)
    {
        const AHexTile* Tile = GetTileByHex(Hex);
        if (Tile)
        {
            Tile->Select(SelectedMaterial);
            SelectedHexes.push_back(Hex);
        }
    }
}

void AHexGridManager::UnselectHexes()
{
    for (Hex Hex : SelectedHexes)
    {
        const AHexTile* Tile = GetTileByHex(Hex);
        if (Tile)
        {
            Tile->Unselect();
        }
    }

    SelectedHexes.clear();
}

float AHexGridManager::GetFromStartCost(const Hex& Start, const Hex& Current)
{
    return Distance(Start, Current);
}

float AHexGridManager::GetToEndCost(const Hex& Current, const Hex& End)
{
    return Distance(Current, End);
}

float AHexGridManager::GetTotalCost(const Hex& Start, const Hex& Current, const Hex& End)
{
    return GetFromStartCost(Start, Current) + GetToEndCost(Current, End); 
}

float AHexGridManager::GetTileCost(const Hex& Hex)
{
    EHexTypes Type = HexTileMap[Hex]->TileType;
    if (HexTileCostMap.count(Type))
    {
        return HexTileCostMap[Type];
    }

    return 100;
}

// optimized
// float AHexGridManager::GetHexCost(const Hex& Current, const Hex& Next, const Hex& Last, const Hex& Start, const Hex& End)
// {
//     EHexTypes Type = HexTileMap[Next]->TileType;
//     if (HexTileCostMap.count(Type))
//     {
//         float Cost = HexTileCostMap[Type];
//         // penalty for going in the same direction too much
//         // Hex LastDirection = GetHexDirection(Last, Current);
//         // Hex NextDirection = GetHexDirection(Current, Next);
//         // if (LastDirection == NextDirection)
//         // {
//         //     Cost += 0.0001;
//         // }
//
//         // FVector StartToEndVector = GetVectorDirection(Current, Next);
//         // FVector NextToEndVector = GetVectorDirection(Next, End);
//
//         // double DotSinisa = FVector::DotProduct(StartToEndVector.GetSafeNormal(), NextToEndVector.GetSafeNormal());
//         // DotSinisa = DotSinisa / 100;
//
//         // check if current hex is on the line and reduce 0.001 from the cost.
//         const AUOCTestGameMode* GameMode = Cast<AUOCTestGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
//         
//         std::vector<Hex> Hexes = GameMode->GridManager->GetHexLine(Start, End);
//         if (std::find(Hexes.begin(), Hexes.end(), Current) != Hexes.end())
//         {
//             // exists
//             Cost -= 0.001f;
//         }
//
//         return Cost;
//     }
//
//     return 1000;
// }

float AHexGridManager::GetHexCost(const Hex& Tile)
{
    EHexTypes Type = HexTileMap[Tile]->TileType;
    if (HexTileCostMap.count(Type))
    {
        return HexTileCostMap[Type];
    }

    return 1000.f;
}