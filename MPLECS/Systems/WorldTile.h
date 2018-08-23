//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// ECS/WorldTile.h
// Creates and updates all tiles in the world
// When interactions extend to a new set of tiles, creates those and starts updating them

// Terms:
// * Tile: Unit of the world terrain
//         It has a position in a sector
//         At standard zoom, it's 5px square
// * Sector: A 20 x 20 grid of Tiles.
// * Quadrant: A 2x2 Grid of Sectors
// A world position is a combination of Quadrant X, Y
//    Sector X, Y
//    Tile X, Y
//  Printed as QX.QY:SX.SY:TX.TY
// The world starts with 1 quadrant, player spawns in the center of it
//    So spawn position is 0.0:1.1:0.0
// When a position beyond the current spawned quadrants becomes relevant,
// new quadrants are spawned from the current world out to that quadrant
// A thickness of up to 3 quadrants may be spawned to try to create pathing 
// to the target quadrant

#include "../ECS/System.h"

#include <array>
#include <thread>

namespace TileConstants
{
	constexpr int TILE_SIDE_LENGTH = 5;
	constexpr int SECTOR_SIDE_LENGTH = 60;
	constexpr int QUADRANT_SIDE_LENGTH = 4;

	constexpr int BASE_QUADRANT_ORIGIN_COORDINATE =
		-TILE_SIDE_LENGTH *
		SECTOR_SIDE_LENGTH *
		QUADRANT_SIDE_LENGTH / 2;

	// Will later be configuration data
	constexpr int TILE_TYPE_COUNT = 8;
}

class WorldTile : public SystemBase
{
	using QuadrantId = CoordinateVector2;
public:
	WorldTile() : SystemBase() { }
	virtual ~WorldTile() {}
	virtual void ProgramInit() override;
	virtual void SetupGameplay() override;
	virtual void Operate(GameLoopPhase phase, const timeuS& frameDuration) override;
	virtual bool ShouldExit() override;
protected:
	void CheckBuildingPlacements();

	struct Tile
	{
		ECS_Core::Components::TileType m_tileType;
		std::optional<int> m_movementCost; // If notset, unpathable
										   // Each 1 pixel is 4 components: RGBA
		std::array<sf::Uint32, TileConstants::TILE_SIDE_LENGTH * TileConstants::TILE_SIDE_LENGTH> m_tilePixels;
		std::optional<ecs::Impl::Handle> m_owningBuilding; // If notset, no building owns this tile
	};

	struct Sector
	{
		std::array<
			std::array<Tile, TileConstants::SECTOR_SIDE_LENGTH>,
			TileConstants::SECTOR_SIDE_LENGTH> m_tiles;

		std::array<
			std::array<std::optional<int>, TileConstants::SECTOR_SIDE_LENGTH>,
			TileConstants::SECTOR_SIDE_LENGTH> m_tileMovementCosts;

		// Relevant index of the tile on each border being used for 
		// pathing between sectors
		std::array<
			std::map<s64, std::vector<s64>>,
			static_cast<int>(PathingDirection::_COUNT)>
			m_pathingBorderTileCandidates;

		std::array<
			std::optional<s64>,
			static_cast<int>(PathingDirection::_COUNT)>
			m_pathingBorderTiles;
	};
	struct Quadrant
	{
		std::array<
			std::array<Sector, TileConstants::QUADRANT_SIDE_LENGTH>,
			TileConstants::QUADRANT_SIDE_LENGTH>
			m_sectors;

		sf::Texture m_texture;

		std::array<
			std::array<
				std::array<
					std::array<std::optional<int>, static_cast<int>(PathingDirection::_COUNT) + 1>,
					static_cast<int>(PathingDirection::_COUNT) + 1>,
				TileConstants::QUADRANT_SIDE_LENGTH>,
			TileConstants::QUADRANT_SIDE_LENGTH>
			m_sectorCrossingPathCosts;

		std::array<
			std::array<
				std::array<
					std::array<std::optional<std::deque<CoordinateVector2>>, static_cast<int>(PathingDirection::_COUNT) + 1>,
					static_cast<int>(PathingDirection::_COUNT) + 1>,
				TileConstants::QUADRANT_SIDE_LENGTH>,
			TileConstants::QUADRANT_SIDE_LENGTH>
			m_sectorCrossingPaths;
		std::array<std::map<s64, std::vector<s64>>, static_cast<int>(PathingDirection::_COUNT)> m_pathingBorderSectorCandidates;
		std::array<std::optional<s64>, static_cast<int>(PathingDirection::_COUNT)> m_pathingBorderSectors;
	};
	using SpawnedQuadrantMap = std::map<QuadrantId, Quadrant>;

	struct SectorSeed
	{
		int m_seedTileType{ rand() % TileConstants::TILE_TYPE_COUNT };
		CoordinateVector2 m_seedPosition{
			rand() % TileConstants::SECTOR_SIDE_LENGTH,
			rand() % TileConstants::SECTOR_SIDE_LENGTH };
	};
	struct SectorSeedPosition
	{
		int m_type;
		// Position is within the 3x3 sector square
		// Top left sector (-1, -1) from current sector is origin
		CoordinateVector2 m_position;
	};
	struct QuadrantSeed
	{
		std::array<
			std::array<SectorSeed, TileConstants::QUADRANT_SIDE_LENGTH>,
			TileConstants::QUADRANT_SIDE_LENGTH>
			m_sectors;
	};
	using SeededQuadrantMap = std::map<QuadrantId, QuadrantSeed>;
	using WorldCoordinates = TilePosition;

	struct TileSide
	{
		TileSide() = default;
		TileSide(const WorldCoordinates& coords, const Direction& direction)
			: m_coords(coords)
			, m_direction(direction)
		{}
		bool operator<(const TileSide& other) const
		{
			if (m_direction < other.m_direction)
			{
				return true;
			}
			else if (other.m_direction < m_direction)
			{
				return false;
			}
			return m_coords < other.m_coords;
		}
		WorldCoordinates m_coords;
		Direction m_direction{ Direction::NORTH };
	};
	std::set<TileSide> GetAdjacents(const WorldCoordinates& coordinates);
	std::optional<s64> FindCommonBorderTile(
		const Sector& sector1,
		int sector1Side,
		const Sector& sector2,
		int sector2Side);
	WorldCoordinates WorldPositionToCoordinates(const CoordinateVector2 & worldPos);
	CoordinateVector2 CoordinatesToWorldPosition(const WorldCoordinates & worldCoords);
	CoordinateVector2 CoordinatesToWorldOffset(const WorldCoordinates & worldOffset);
	std::vector<WorldTile::SectorSeedPosition> GetRelevantSeeds(
		const CoordinateVector2 & coordinates,
		int secX,
		int secY);

	void GrowTerritories();
	std::optional<Tile*> GetTile(const TilePosition& buildingTilePos);
	Quadrant& FetchQuadrant(const CoordinateVector2 & quadrantCoords);
	std::thread SpawnQuadrant(const CoordinateVector2& coordinates);
	void FillSectorPathing(
		Sector& sector,
		std::vector<std::thread>& pathFindingThreads,
		Quadrant& quadrant,
		int sectorI,
		int sectorJ);
	void FillQuadrantPathingEdges(Quadrant& quadrant);
	std::thread SpawnBetween(
		CoordinateVector2 origin,
		CoordinateVector2 target);
	void ReturnDeadBuildingTiles();

	void ProcessSelectTile(const Action::LocalPlayer::SelectTile & select, const ecs::EntityIndex & governorEntity);

	std::optional<ECS_Core::Components::MoveToPoint> GetPath(const TilePosition & sourcePosition, const TilePosition & targetPosition);
	
	std::optional<ECS_Core::Components::MoveToPoint> FindSingleQuadrantPath(
		const WorldTile::Quadrant& quadrant,
		const TilePosition& sourcePosition,
		const TilePosition& targetPosition);
	std::optional<ECS_Core::Components::MoveToPoint> FindSingleSectorPath(
		const WorldTile::Sector& sector,
		const TilePosition& sourcePosition,
		const TilePosition& targetPosition);


	struct SortByOriginDist
	{
		// Acts as operator<
		bool operator()(
			const CoordinateVector2& left,
			const CoordinateVector2& right) const;
	};

	using CoordinateFromOriginSet = std::set<CoordinateVector2, SortByOriginDist>;
	void TouchConnectedCoordinates(
		const CoordinateVector2& origin,
		CoordinateFromOriginSet& untouched,
		CoordinateFromOriginSet& touched);

	CoordinateVector2 FindNearestQuadrant(const SpawnedQuadrantMap & searchedQuadrants, const CoordinateVector2 & quadrantCoords);

	CoordinateVector2 FindNearestQuadrant(const CoordinateFromOriginSet & searchedQuadrants, const CoordinateVector2 & quadrantCoords);

	SpawnedQuadrantMap m_spawnedQuadrants;
	bool m_baseQuadrantSpawned{ false };
	bool m_startingBuilderSpawned{ false };
	SeededQuadrantMap m_quadrantSeeds;
};
template <> std::unique_ptr<WorldTile> InstantiateSystem();