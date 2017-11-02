//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Systems/WorldTile.cpp
// Creates and updates all tiles in the world
// When interactions extend to a new set of tiles, creates those and starts updating them

#include "../Core/typedef.h"

#include "Systems.h"

#include "../ECS/System.h"
#include "../ECS/ECS.h"

#include "../Util/Pathing.h"

#include <limits>
#include <random>

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

namespace TileConstants
{
	constexpr int TILE_SIDE_LENGTH = 5;
	constexpr int SECTOR_SIDE_LENGTH = 100;
	constexpr int QUADRANT_SIDE_LENGTH = 4;

	constexpr int BASE_QUADRANT_ORIGIN_COORDINATE =
		-TILE_SIDE_LENGTH *
		SECTOR_SIDE_LENGTH *
		QUADRANT_SIDE_LENGTH / 2;

	// Will later be configuration data
	constexpr int TILE_TYPE_COUNT = 8;
}
using namespace ECS_Core::Components;
// Non-entity data the tile system needs.
namespace TileNED
{
	using QuadrantId = CoordinateVector2;
	using namespace TileConstants;

	struct Tile
	{
		int m_tileType;
		std::optional<int> m_movementCost; // If notset, unpathable
										   // Each 1 pixel is 4 components: RGBA
		sf::Uint32 m_tilePixels[TILE_SIDE_LENGTH * TILE_SIDE_LENGTH];
		std::optional<ecs::EntityIndex> m_owningBuilding; // If notset, no building owns this tile
	};

	using Pathability = bool[Pathing::PathingSide::_COUNT][Pathing::PathingSide::_COUNT];

	struct Sector
	{
		Pathability m_pathability;
		Tile m_tiles
			[TileConstants::SECTOR_SIDE_LENGTH]
		[TileConstants::SECTOR_SIDE_LENGTH];
	};
	struct Quadrant
	{
		Pathability m_pathability;
		Sector m_sectors
			[TileConstants::QUADRANT_SIDE_LENGTH]
		[TileConstants::QUADRANT_SIDE_LENGTH];

		sf::Texture m_texture;
	};
	using SpawnedQuadrantMap = std::map<QuadrantId, Quadrant>;
	SpawnedQuadrantMap s_spawnedQuadrants;
	bool baseQuadrantSpawned{ false };

	struct SectorSeed
	{
		int m_seedTileType{ rand() % TileConstants::TILE_TYPE_COUNT };
		CoordinateVector2 m_seedPosition{
			rand() % TileConstants::SECTOR_SIDE_LENGTH,
			rand() % TileConstants::SECTOR_SIDE_LENGTH };
	};
	struct QuadrantSeed
	{
		SectorSeed m_sectors
			[TileConstants::QUADRANT_SIDE_LENGTH]
		[TileConstants::QUADRANT_SIDE_LENGTH];
	};
	using SeededQuadrantMap = std::map<QuadrantId, QuadrantSeed>;
	SeededQuadrantMap s_quadrantSeeds;

	void CheckBuildingPlacements(ECS_Core::Manager& manager);

	using WorldCoordinates = ECS_Core::Components::TilePosition;

	struct TileSide
	{
		TileSide() = default;
		TileSide(const TileNED::WorldCoordinates& coords, const Direction& direction)
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
		TileNED::WorldCoordinates m_coords;
		Direction m_direction{ Direction::NORTH };
	};
	std::set<TileSide> GetAdjacents(const WorldCoordinates& coordinates);

	void GrowTerritories(ECS_Core::Manager& manager, timeuS frameDuration);
	TileNED::Quadrant& FetchQuadrant(const ECS_Core::Components::CoordinateVector2 & quadrantCoords, ECS_Core::Manager & manager);
	void CheckWorldClick(ECS_Core::Manager& manager);
	void SpawnBetween(
		CoordinateVector2 origin,
		CoordinateVector2 target,
		ECS_Core::Manager& manager);

	struct SortByOriginDist
	{
		// Acts as operator<
		bool operator()(
			const CoordinateVector2& left,
			const CoordinateVector2& right) const
		{
			if (left == right) return false;
			if (left.MagnitudeSq() < right.MagnitudeSq()) return true;
			if (left.MagnitudeSq() > right.MagnitudeSq()) return false;

			if (left.m_x < right.m_x) return true;
			if (left.m_x > right.m_x) return false;

			if (left.m_y < right.m_y) return true;
			return false;
		}
	};

	using CoordinateFromOriginSet = std::set<CoordinateVector2, SortByOriginDist>;
	void TouchConnectedCoordinates(
		const CoordinateVector2& origin,
		CoordinateFromOriginSet& untouched,
		CoordinateFromOriginSet& touched);

	enum class DrawPriority
	{
		LANDSCAPE,
		TERRITORY_BORDER,
		FLAVOR_BUILDING,
		LOGICAL_BUILDING,
	};
}

ECS_Core::Components::TilePosition& ECS_Core::Components::TilePosition::operator+=(const TilePosition& other)
{
	m_quadrantCoords += other.m_quadrantCoords;
	m_sectorCoords += other.m_sectorCoords;
	m_coords += other.m_coords;
	assert(m_coords.m_x >= 0);
	assert(m_coords.m_y >= 0);
	assert(m_sectorCoords.m_x >= 0);
	assert(m_sectorCoords.m_y >= 0);

	m_sectorCoords.m_x += m_coords.m_x / TileConstants::SECTOR_SIDE_LENGTH;
	m_sectorCoords.m_y += m_coords.m_y / TileConstants::SECTOR_SIDE_LENGTH;
	m_coords.m_x %= TileConstants::SECTOR_SIDE_LENGTH;
	m_coords.m_y %= TileConstants::SECTOR_SIDE_LENGTH;

	m_quadrantCoords.m_x += m_sectorCoords.m_x / TileConstants::QUADRANT_SIDE_LENGTH;
	m_quadrantCoords.m_y += m_sectorCoords.m_y / TileConstants::QUADRANT_SIDE_LENGTH;
	m_sectorCoords.m_x %= TileConstants::QUADRANT_SIDE_LENGTH;
	m_sectorCoords.m_y %= TileConstants::QUADRANT_SIDE_LENGTH;
	return *this;
}

ECS_Core::Components::TilePosition& ECS_Core::Components::TilePosition::operator-=(const TilePosition& other)
{
	m_quadrantCoords -= other.m_quadrantCoords;
	m_sectorCoords -= other.m_sectorCoords;
	m_coords -= other.m_coords;

	while (m_coords.m_x < 0)
	{
		--m_sectorCoords.m_x;
		m_coords.m_x += TileConstants::SECTOR_SIDE_LENGTH;
	}
	while (m_coords.m_y < 0)
	{
		--m_sectorCoords.m_y;
		m_coords.m_y += TileConstants::SECTOR_SIDE_LENGTH;
	}

	while (m_sectorCoords.m_x < 0)
	{
		--m_quadrantCoords.m_x;
		m_sectorCoords.m_x += TileConstants::QUADRANT_SIDE_LENGTH;
	}
	while (m_sectorCoords.m_y < 0)
	{
		--m_quadrantCoords.m_y;
		m_sectorCoords.m_y += TileConstants::QUADRANT_SIDE_LENGTH;
	}
	return *this;
}

struct SectorSeedPosition
{
	int m_type;
	// Position is within the 3x3 sector square
	// Top left sector (-1, -1) from current sector is origin
	CoordinateVector2 m_position;
};

std::vector<SectorSeedPosition> GetRelevantSeeds(
	const ECS_Core::Components::CoordinateVector2 & coordinates,
	int secX,
	int secY)
{
	std::vector<SectorSeedPosition> relevantSeeds;
	CoordinateVector2 quadPosition;
	CoordinateVector2 secPosition;
	for (int x = -1; x < 2; ++x)
	{
		auto secPosX = secX + x;
		if (secPosX < 0)
		{
			// Go one quadrant left, if available
			secPosition.m_x = TileConstants::QUADRANT_SIDE_LENGTH - 1;
			quadPosition.m_x = coordinates.m_x - 1;
		}
		else if (secPosX >= TileConstants::QUADRANT_SIDE_LENGTH)
		{
			secPosition.m_x = 0;
			quadPosition.m_x = coordinates.m_x + 1;
		}
		else
		{
			secPosition.m_x = secPosX;
			quadPosition.m_x = coordinates.m_x;
		}
		for (int y = -1; y < 2; ++y)
		{
			auto secPosY = secY + y;
			if (secPosY < 0)
			{
				// Go one quadrant left, if available
				secPosition.m_y = TileConstants::QUADRANT_SIDE_LENGTH - 1;
				quadPosition.m_y = coordinates.m_y - 1;
			}
			else if (secPosY >= TileConstants::QUADRANT_SIDE_LENGTH)
			{
				secPosition.m_y = 0;
				quadPosition.m_y = coordinates.m_y + 1;
			}
			else
			{
				secPosition.m_y = secPosY;
				quadPosition.m_y = coordinates.m_y;
			}

			auto quad = TileNED::s_quadrantSeeds[quadPosition];
			auto& seedingSector = quad.m_sectors[secPosition.m_x][secPosition.m_y];

			relevantSeeds.push_back({
				seedingSector.m_seedTileType,
				{ seedingSector.m_seedPosition.m_x + ((x + 1) * TileConstants::SECTOR_SIDE_LENGTH),
				seedingSector.m_seedPosition.m_y + ((y + 1) * TileConstants::SECTOR_SIDE_LENGTH) }
			});
		}
	}
	return relevantSeeds;
}

f64 RandDouble()
{
	return static_cast<f64>(rand()) / static_cast<f64>(RAND_MAX + 1);
}

void SpawnQuadrant(const CoordinateVector2& coordinates, ECS_Core::Manager& manager)
{
	using namespace TileConstants;
	if (TileNED::s_spawnedQuadrants.find(coordinates)
		!= TileNED::s_spawnedQuadrants.end())
	{
		// Quadrant is already here
		return;
	}
	auto index = manager.createIndex();
	auto quadrantSideLength = QUADRANT_SIDE_LENGTH * SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH;
	manager.addComponent<ECS_Core::Components::C_QuadrantPosition>(
		index,
		coordinates);
	manager.addComponent<ECS_Core::Components::C_PositionCartesian>(
		index,
		static_cast<f64>(BASE_QUADRANT_ORIGIN_COORDINATE +
		(quadrantSideLength * coordinates.m_x)),
		static_cast<f64>(BASE_QUADRANT_ORIGIN_COORDINATE +
		(quadrantSideLength * coordinates.m_y)),
		0);

	auto rect = std::make_shared<sf::RectangleShape>(sf::Vector2f(
		static_cast<float>(quadrantSideLength),
		static_cast<float>(quadrantSideLength)));
	auto& quadrant = TileNED::s_spawnedQuadrants[coordinates];
	quadrant.m_texture.create(quadrantSideLength, quadrantSideLength);
	for (auto secX = 0; secX < TileConstants::QUADRANT_SIDE_LENGTH; ++secX)
	{
		for (auto secY = 0; secY < TileConstants::QUADRANT_SIDE_LENGTH; ++secY)
		{
			auto& sector = quadrant.m_sectors[secX][secY];

			auto relevantSeeds = GetRelevantSeeds(coordinates, secX, secY);
			assert(relevantSeeds.size() > 0);

			std::optional<int> movementCosts[SECTOR_SIDE_LENGTH][SECTOR_SIDE_LENGTH];
			for (auto tileX = 0; tileX < TileConstants::SECTOR_SIDE_LENGTH; ++tileX)
			{
				for (auto tileY = 0; tileY < TileConstants::SECTOR_SIDE_LENGTH; ++tileY)
				{
					auto& tile = sector.m_tiles[tileX][tileY];

					// Pick a seed
					// Will be chosen by weighted random, based on distance from nearest 9 seeds
					// Seeds are from local sector, and the 8 adjacent and corner-adj sectors
					auto locationForSeeding = CoordinateVector2(
						tileX + TileConstants::SECTOR_SIDE_LENGTH,
						tileY + TileConstants::SECTOR_SIDE_LENGTH);

					std::vector<f64> weightBorders;
					f64 totalWeight = 0;
					for (auto&& seed : relevantSeeds)
					{
						f64 distance = static_cast<f64>(
							(locationForSeeding - seed.m_position).MagnitudeSq());
						weightBorders.push_back(totalWeight += 1.0 / (distance * distance * distance));
					}

					auto weightedValue = RandDouble() * totalWeight;
					size_t weightedPosition = 0;
					for (; weightedPosition < weightBorders.size(); ++weightedPosition)
					{
						if (weightedValue < weightBorders[weightedPosition])
						{
							break;
						}
					}
					// If we get 1.0, it won't be less (probably) so just use that edge
					// No huge effect
					if (weightedPosition >= relevantSeeds.size()) weightedPosition = relevantSeeds.size() - 1;
					tile.m_tileType = relevantSeeds[weightedPosition].m_type;
					if (tile.m_tileType) // Make type 0 unpathable for testing
					{
						tile.m_movementCost = (rand() % 6) + 1;
						movementCosts[tileX][tileY] = tile.m_movementCost;
					}
					for (auto& pixel : tile.m_tilePixels)
					{
						pixel =
							(((tile.m_tileType & 1) ? 255 : 0) << 0) + // R
							(((tile.m_tileType & 2) ? 255 : 0) << 8) + // G
							(((tile.m_tileType & 4) ? 255 : 0) << 16) + // B
							+(0xFF << 24); // A
					}
					quadrant.m_texture.update(
						reinterpret_cast<const sf::Uint8*>(tile.m_tilePixels),
						TILE_SIDE_LENGTH,
						TILE_SIDE_LENGTH,
						((secX * SECTOR_SIDE_LENGTH) + tileX) * TILE_SIDE_LENGTH,
						((secY * SECTOR_SIDE_LENGTH) + tileY) * TILE_SIDE_LENGTH);
				}
			}
		}
	}
	rect->setTexture(&quadrant.m_texture);
	auto& drawable = manager.addComponent<ECS_Core::Components::C_SFMLDrawable>(index);
	drawable.m_drawables[ECS_Core::Components::DrawLayer::TERRAIN][static_cast<u64>(TileNED::DrawPriority::LANDSCAPE)].push_back({ rect,{ 0,0 } });
}

template<class T>
inline int sign(T x)
{
	return (x < 0) ? -1 : ((x > 0) ? 1 : 0);
}

template<class T, class U>
int min(T&& a, U&& b)
{
	return a < b ? a : b;
}

TileNED::WorldCoordinates WorldPositionToCoordinates(const CoordinateVector2& worldPos)
{
	using namespace TileConstants;
	auto offsetFromQuadrantOrigin = worldPos - CoordinateVector2(
		BASE_QUADRANT_ORIGIN_COORDINATE,
		BASE_QUADRANT_ORIGIN_COORDINATE);
	return TilePosition{
		{ min(0, sign(offsetFromQuadrantOrigin.m_x)) + sign(offsetFromQuadrantOrigin.m_x) * (abs(offsetFromQuadrantOrigin.m_x) / (QUADRANT_SIDE_LENGTH * SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH)),
		min(0, sign(offsetFromQuadrantOrigin.m_y)) + sign(offsetFromQuadrantOrigin.m_y) * (abs(offsetFromQuadrantOrigin.m_y) / (QUADRANT_SIDE_LENGTH * SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH)) },

		{ min(0, sign(offsetFromQuadrantOrigin.m_x)) + sign(offsetFromQuadrantOrigin.m_x) * (abs(offsetFromQuadrantOrigin.m_x) % (QUADRANT_SIDE_LENGTH * SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH)) / (SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH),
		min(0, sign(offsetFromQuadrantOrigin.m_y)) + sign(offsetFromQuadrantOrigin.m_y) * (abs(offsetFromQuadrantOrigin.m_y) % (QUADRANT_SIDE_LENGTH * SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH)) / (SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH) },

		{ min(0, sign(offsetFromQuadrantOrigin.m_x)) + sign(offsetFromQuadrantOrigin.m_x) * (abs(offsetFromQuadrantOrigin.m_x) % (SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH)) / TILE_SIDE_LENGTH,
		min(0, sign(offsetFromQuadrantOrigin.m_y)) + sign(offsetFromQuadrantOrigin.m_y) * (abs(offsetFromQuadrantOrigin.m_y) % (SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH)) / TILE_SIDE_LENGTH }
	};
}

CoordinateVector2 CoordinatesToWorldPosition(const TileNED::WorldCoordinates& worldCoords)
{
	using namespace TileConstants;
	return {
		BASE_QUADRANT_ORIGIN_COORDINATE +
		(((((QUADRANT_SIDE_LENGTH * worldCoords.m_quadrantCoords.m_x)
			+ worldCoords.m_sectorCoords.m_x) * SECTOR_SIDE_LENGTH)
			+ worldCoords.m_coords.m_x) * TILE_SIDE_LENGTH),

		BASE_QUADRANT_ORIGIN_COORDINATE +
		(((((QUADRANT_SIDE_LENGTH * worldCoords.m_quadrantCoords.m_y)
			+ worldCoords.m_sectorCoords.m_y) * SECTOR_SIDE_LENGTH)
			+ worldCoords.m_coords.m_y) * TILE_SIDE_LENGTH)
	};
}

CoordinateVector2 CoordinatesToWorldOffset(const TileNED::WorldCoordinates& worldOffset)
{
	using namespace TileConstants;
	return {
		(((((QUADRANT_SIDE_LENGTH * worldOffset.m_quadrantCoords.m_x)
		+ worldOffset.m_sectorCoords.m_x) * SECTOR_SIDE_LENGTH)
			+ worldOffset.m_coords.m_x) * TILE_SIDE_LENGTH),

			(((((QUADRANT_SIDE_LENGTH * worldOffset.m_quadrantCoords.m_y)
				+ worldOffset.m_sectorCoords.m_y) * SECTOR_SIDE_LENGTH)
				+ worldOffset.m_coords.m_y) * TILE_SIDE_LENGTH)
	};
}

void TileNED::SpawnBetween(
	CoordinateVector2 origin,
	CoordinateVector2 target,
	ECS_Core::Manager& manager)
{
	// Base case: Spawn between a place and itself
	if (origin == target)
	{
		// Spawn just in case, will return immediately in most cases.
		SpawnQuadrant(target, manager);
		return;
	}

	SpawnQuadrant(target, manager);
	SpawnQuadrant(origin, manager);

	auto mid = (target + origin) / 2;
	SpawnQuadrant(mid, manager);

	if (mid == target || mid == origin)
	{
		// We're all filled in on this segment.
		return;
	}

	SpawnBetween(origin, mid, manager);
	SpawnBetween(mid + CoordinateVector2(sign(target.m_x - origin.m_x), sign(target.m_y - origin.m_y)), target, manager);
}

// Precondition: touched is empty
void TileNED::TouchConnectedCoordinates(
	const CoordinateVector2& origin,
	CoordinateFromOriginSet& untouched,
	CoordinateFromOriginSet& touched)
{
	if (s_spawnedQuadrants.find(origin) == s_spawnedQuadrants.end())
	{
		return;
	}
	if (touched.find(origin) != touched.end())
	{
		return;
	}

	touched.insert(origin);
	untouched.erase(origin);
	TouchConnectedCoordinates(origin + CoordinateVector2(0, 1), untouched, touched);
	TouchConnectedCoordinates(origin + CoordinateVector2(1, 0), untouched, touched);
	TouchConnectedCoordinates(origin - CoordinateVector2(0, 1), untouched, touched);
	TouchConnectedCoordinates(origin - CoordinateVector2(1, 0), untouched, touched);
}

CoordinateVector2 FindNearestQuadrant(const TileNED::SpawnedQuadrantMap& searchedQuadrants, const CoordinateVector2& quadrantCoords)
{
	CoordinateVector2 closest;
	s64 smallestDistance = std::numeric_limits<s64>::max();
	// Assume the initial tile is the closest for a start
	for (auto&& quadrant : searchedQuadrants)
	{
		auto&& quad = quadrant.first;
		auto&& dist = quad - quadrantCoords;
		auto&& distanceSq = dist.MagnitudeSq();
		if (distanceSq < smallestDistance)
		{
			closest = quad;
			smallestDistance = distanceSq;
		}
	}
	return closest;
}

CoordinateVector2 FindNearestQuadrant(const TileNED::CoordinateFromOriginSet& searchedQuadrants, const CoordinateVector2& quadrantCoords)
{
	CoordinateVector2 closest;
	s64 smallestDistance = std::numeric_limits<s64>::max();
	// Assume the initial tile is the closest for a start
	for (auto&& quadrant : searchedQuadrants)
	{
		auto&& dist = quadrant - quadrantCoords;
		auto&& distanceSq = dist.MagnitudeSq();
		if (distanceSq < smallestDistance)
		{
			closest = quadrant;
			smallestDistance = distanceSq;
		}
	}
	return closest;
}

void TileNED::CheckBuildingPlacements(ECS_Core::Manager& manager)
{
	auto& completedBuildings = manager.entitiesMatching<ECS_Core::Signatures::S_CompleteBuilding>();
	auto& inProgressBuildings = manager.entitiesMatching<ECS_Core::Signatures::S_InProgressBuilding>();
	auto& ghostBuildings = manager.entitiesMatching<ECS_Core::Signatures::S_PlannedBuildingPlacement>();

	for (auto& ghost : ghostBuildings)
	{
		auto& tilePosition = manager.getComponent<ECS_Core::Components::C_TilePosition>(ghost).m_position;

		bool collisionFound{ false };
		for (auto& complete : completedBuildings)
		{
			if (tilePosition == manager.getComponent<ECS_Core::Components::C_TilePosition>(complete).m_position)
			{
				collisionFound = true;
				break;
			}
		}
		for (auto& inProgress : inProgressBuildings)
		{
			if (tilePosition == manager.getComponent<ECS_Core::Components::C_TilePosition>(inProgress).m_position)
			{
				collisionFound = true;
				break;
			}
		}
		manager.getComponent<ECS_Core::Components::C_BuildingGhost>(ghost).m_currentPlacementValid = !collisionFound;
	}
}

std::set<TileNED::TileSide> TileNED::GetAdjacents(const WorldCoordinates& coords)
{
	return {
		{coords + WorldCoordinates{ { 0,0 },{ 0,0 },{ 1,0 } }, Direction::EAST},
		{coords + WorldCoordinates{ { 0,0 },{ 0,0 },{ 0,1 } }, Direction::SOUTH},
		{coords - WorldCoordinates{ { 0,0 },{ 0,0 },{ 1,0 } }, Direction::WEST},
		{coords - WorldCoordinates{ { 0,0 },{ 0,0 },{ 0,1 } }, Direction::NORTH},
	};
}

// Hack: Get reference to global font

extern std::optional<sf::Font> s_font;

void TileNED::GrowTerritories(ECS_Core::Manager& manager, timeuS frameDuration)
{
	auto& territoryEntities = manager.entitiesMatching<ECS_Core::Signatures::S_CompleteBuilding>();

	for (auto& territoryEntity : territoryEntities)
	{
		// Make sure territory is growing into a valid spot
		auto& territory = manager.getComponent<C_Territory>(territoryEntity);
		auto& buildingTilePos = manager.getComponent<C_TilePosition>(territoryEntity);
		bool needsGrowthTile = true;
		if (territory.m_nextGrowthTile)
		{
			auto& coordinates = territory.m_nextGrowthTile->m_tile;
			if (!FetchQuadrant(coordinates.m_quadrantCoords, manager)
				.m_sectors[coordinates.m_sectorCoords.m_x][coordinates.m_sectorCoords.m_y]
				.m_tiles[coordinates.m_coords.m_x][coordinates.m_coords.m_y]
				.m_owningBuilding)
			{
				needsGrowthTile = false;
			}
		}
		if (needsGrowthTile)
		{
			// get all tiles adjacent to the territory that are not yet claimed
			std::vector<TilePosition> availableGrowthTiles;
			for (auto& tile : territory.m_ownedTiles)
			{
				for (auto& adjacent : GetAdjacents(tile))
				{
					auto& adjacentTile = FetchQuadrant(adjacent.m_coords.m_quadrantCoords, manager)
						.m_sectors[adjacent.m_coords.m_sectorCoords.m_x][adjacent.m_coords.m_sectorCoords.m_y]
						.m_tiles[adjacent.m_coords.m_coords.m_x][adjacent.m_coords.m_coords.m_y];
					if (!adjacentTile.m_owningBuilding && adjacentTile.m_movementCost)
					{
						availableGrowthTiles.push_back(adjacent.m_coords);
					}
				}
			}

			if (availableGrowthTiles.size())
			{
				std::random_device rd;
				std::mt19937 g(rd());
				std::shuffle(availableGrowthTiles.begin(), availableGrowthTiles.end(), g);

				territory.m_nextGrowthTile = { territory.m_nextGrowthTile ? territory.m_nextGrowthTile->m_progress : 0.f, availableGrowthTiles.front() };
			}
		}

		// Now grow if we can
		if (territory.m_nextGrowthTile)
		{
			territory.m_nextGrowthTile->m_progress += (0.0000002 * frameDuration);
			if (territory.m_nextGrowthTile->m_progress >= 1)
			{
				auto& tile = territory.m_nextGrowthTile->m_tile;
				FetchQuadrant(tile.m_quadrantCoords, manager)
					.m_sectors[tile.m_sectorCoords.m_x][tile.m_sectorCoords.m_y]
					.m_tiles[tile.m_coords.m_x][tile.m_coords.m_y].m_owningBuilding = territoryEntity;
				territory.m_ownedTiles.insert(tile);
				territory.m_nextGrowthTile.reset();

				if (manager.hasComponent<ECS_Core::Components::C_SFMLDrawable>(territoryEntity))
				{
					auto& drawable = manager.getComponent<ECS_Core::Components::C_SFMLDrawable>(territoryEntity);

					//// Add graphics for the individual tile
					//{
					//	auto hexagon = std::make_shared<sf::ConvexShape>(6);
					//	auto positionOffset = CoordinatesToWorldOffset(tile - buildingTilePos.m_position);
					//	hexagon->setPoint(0, { 0.5f * TileConstants::TILE_SIDE_LENGTH, 0.f });
					//	hexagon->setPoint(1, { 0.f, 0.333f * TileConstants::TILE_SIDE_LENGTH });
					//	hexagon->setPoint(2, { 0.f, 0.667f * TileConstants::TILE_SIDE_LENGTH });
					//	hexagon->setPoint(3, { 0.5f * TileConstants::TILE_SIDE_LENGTH, 1.f * TileConstants::TILE_SIDE_LENGTH });
					//	hexagon->setPoint(4, { 1.f * TileConstants::TILE_SIDE_LENGTH, 0.667f * TileConstants::TILE_SIDE_LENGTH });
					//	hexagon->setPoint(5, { 1.f * TileConstants::TILE_SIDE_LENGTH, 0.333f * TileConstants::TILE_SIDE_LENGTH });
					//	hexagon->setOutlineThickness(0.1f);
					//	hexagon->setOutlineColor(sf::Color());
					//	hexagon->setFillColor(sf::Color(64, 64, 64, 192));

					//	drawable.m_drawables[ECS_Core::Components::DrawLayer::TERRAIN][static_cast<u64>(TileNED::DrawPriority::LANDSCAPE)].push_back({hexagon, positionOffset.cast<f64>()});
					//}

					// redraw the borders
					{
						// Remove previous borders
						drawable.m_drawables[ECS_Core::Components::DrawLayer::TERRAIN].erase(static_cast<u64>(TileNED::DrawPriority::TERRITORY_BORDER));

						// Tile in territory + direction from tile which is open
						std::set<std::pair<TilePosition, Direction>> edges;
						// For each tile in this territory, see if it has any adjacent spaces which are not in the territory
						for (auto&& ownedTile : territory.m_ownedTiles)
						{
							for (auto&& adj : GetAdjacents(ownedTile))
							{
								if (!territory.m_ownedTiles.count(adj.m_coords))
								{
									edges.insert({ ownedTile, adj.m_direction });
								}
							}
						}

						// Draw each segment
						for (auto&& edge : edges)
						{
							auto positionOffset = CoordinatesToWorldOffset(edge.first - buildingTilePos.m_position).cast<f64>();
							bool isVertical = (edge.second == Direction::EAST || edge.second == Direction::WEST);

							// sf Line type is always 1 pixel wide, so use rectangle so we can control thickness
							auto line = std::make_shared<sf::RectangleShape>(
								isVertical 
								? sf::Vector2f{0.1f, 1.f * TileConstants::TILE_SIDE_LENGTH}
								: sf::Vector2f{1.f * TileConstants::TILE_SIDE_LENGTH, 0.1f});

							switch (edge.second)
							{
							case Direction::NORTH:
								// Go from top left corner, no offset
								break;
							case Direction::SOUTH:
								// start from bottom left
								positionOffset.m_y += TileConstants::TILE_SIDE_LENGTH - 0.1;
								break;
							case Direction::EAST:
								// Start from top right
								positionOffset.m_x += TileConstants::TILE_SIDE_LENGTH - 0.1;
								break;
							case Direction::WEST:
								// Go from top left corner
								break;
							}
							line->setFillColor({});
							drawable.m_drawables[ECS_Core::Components::DrawLayer::TERRAIN][static_cast<u64>(TileNED::DrawPriority::TERRITORY_BORDER)].push_back({line, positionOffset});
						}
					}
				}
			}
		}
	}
}

TileNED::Quadrant& TileNED::FetchQuadrant(const ECS_Core::Components::CoordinateVector2 & quadrantCoords, ECS_Core::Manager & manager)
{
	if (TileNED::s_spawnedQuadrants.find(quadrantCoords) == TileNED::s_spawnedQuadrants.end())
	{
		// We're going to need to spawn world up to that point.
		// first: find the closest available world tile
		auto closest = FindNearestQuadrant(s_spawnedQuadrants, quadrantCoords);

		SpawnBetween(
			closest,
			quadrantCoords,
			manager);

		// Find all quadrants which can't be reached by repeated cardinal direction movement from the origin
		CoordinateFromOriginSet touchedCoordinates, untouchedCoordinates;
		for (auto&& quadrant : s_spawnedQuadrants)
		{
			untouchedCoordinates.insert(quadrant.first);
		}
		TouchConnectedCoordinates({ 0, 0 }, untouchedCoordinates, touchedCoordinates);

		// Start with the closest untouched, connect it. We'll only need to add one to connect it, we know they're corner-to-corner
		// To be secure about it, connect on both sides. Screw your RAM.
		while (untouchedCoordinates.size())
		{
			auto nearestDisconnected = untouchedCoordinates.begin();
			auto nearestConnected = FindNearestQuadrant(touchedCoordinates, *nearestDisconnected);
			for (; nearestDisconnected != untouchedCoordinates.end(); ++nearestDisconnected)
			{
				if ((nearestConnected - *nearestDisconnected).MagnitudeSq() == 2)
				{
					break;
				}
			}
			if (nearestDisconnected == untouchedCoordinates.end())
			{
				break;
			}
			SpawnQuadrant({ nearestDisconnected->m_x, nearestConnected.m_y }, manager);
			SpawnQuadrant({ nearestConnected.m_x, nearestDisconnected->m_y }, manager);

			touchedCoordinates.clear();
			TouchConnectedCoordinates(nearestConnected, untouchedCoordinates, touchedCoordinates);
		}
	}
	return s_spawnedQuadrants[quadrantCoords];
}

void TileNED::CheckWorldClick(ECS_Core::Manager& manager)
{
	auto inputEntities = manager.entitiesMatching<ECS_Core::Signatures::S_Input>();
	if (inputEntities.size() == 0) return;
	ECS_Core::Components::C_UserInputs& inputComponent = manager.getComponent<ECS_Core::Components::C_UserInputs>(inputEntities.front());
	if (inputComponent.m_unprocessedThisFrameDownMouseButtonFlags & (u8)ECS_Core::Components::MouseButtons::LEFT)
	{
		auto&& quadrantCoords = inputComponent.m_currentMousePosition.m_tilePosition->m_position.m_quadrantCoords;

		FetchQuadrant(quadrantCoords, manager);

		inputComponent.ProcessMouseDown(ECS_Core::Components::MouseButtons::LEFT);
	}
}

void WorldTile::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	switch (phase)
	{
	case GameLoopPhase::INPUT:
		// Fill in tile position of the mouse
		for (auto&& inputEntity : m_managerRef.entitiesMatching<ECS_Core::Signatures::S_Input>())
		{
			auto& inputComponent = m_managerRef.getComponent<ECS_Core::Components::C_UserInputs>(inputEntity);
			inputComponent.m_currentMousePosition.m_tilePosition =
				WorldPositionToCoordinates(inputComponent.m_currentMousePosition.m_worldPosition.cast<s64>());
			for (auto&& mouseInputs : inputComponent.m_heldMouseButtonInitialPositions)
			{
				if (!mouseInputs.second.m_position.m_tilePosition)
				{
					mouseInputs.second.m_position.m_tilePosition =
						WorldPositionToCoordinates(mouseInputs.second.m_position.m_worldPosition.cast<s64>());
				}
			}
		}
		break;
	case GameLoopPhase::PREPARATION:
		if (!TileNED::baseQuadrantSpawned)
		{
			SpawnQuadrant({ 0, 0 }, m_managerRef);
			TileNED::baseQuadrantSpawned = true;
		}
		break;
	case GameLoopPhase::ACTION:
		// Grow territories that are able to do so before taking any actions
		TileNED::GrowTerritories(m_managerRef, frameDuration);

		TileNED::CheckWorldClick(m_managerRef);
		break;

	case GameLoopPhase::ACTION_RESPONSE:
		// Update position of any world-tile drawables
		m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_TilePositionable>(
			[](
				ecs::EntityIndex mI,
				ECS_Core::Components::C_PositionCartesian& position,
				const ECS_Core::Components::C_TilePosition& tilePosition)
		{
			auto worldPosition = CoordinatesToWorldPosition(tilePosition.m_position);
			position.m_position.m_x = static_cast<f64>(worldPosition.m_x);
			position.m_position.m_y = static_cast<f64>(worldPosition.m_y);

		});

		TileNED::CheckBuildingPlacements(m_managerRef);
		break;

	case GameLoopPhase::RENDER:
	case GameLoopPhase::CLEANUP:
		return;
	}
}

bool WorldTile::ShouldExit()
{
	return false;
}

DEFINE_SYSTEM_INSTANTIATION(WorldTile);