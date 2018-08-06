//-----------------------------------------------------------------------------
// All code is property of Dictator Developers Inc
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2018

// Systems/WorldTile.cpp
// Creates and updates all tiles in the world
// When interactions extend to a new set of tiles, creates those and starts updating them

#include "../Core/typedef.h"

#include "Systems.h"

#include "../ECS/System.h"
#include "../ECS/ECS.h"
#include "../Components/UIComponents.h"

#include "../Util/Pathing.h"

#include <chrono>
#include <iostream>
#include <limits>
#include <random>
#include <thread>

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

extern sf::Font s_font;
namespace TileConstants
{
	constexpr int TILE_SIDE_LENGTH = 5;
	constexpr int SECTOR_SIDE_LENGTH = 100;
	constexpr int QUADRANT_SIDE_LENGTH = 6;

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
		ECS_Core::Components::TileType m_tileType;
		std::optional<int> m_movementCost; // If notset, unpathable
										   // Each 1 pixel is 4 components: RGBA
		sf::Uint32 m_tilePixels[TILE_SIDE_LENGTH * TILE_SIDE_LENGTH];
		std::optional<ecs::Impl::Handle> m_owningBuilding; // If notset, no building owns this tile
	};

	struct Sector
	{
		Tile m_tiles
			[TileConstants::SECTOR_SIDE_LENGTH]
		[TileConstants::SECTOR_SIDE_LENGTH];
		std::optional<int> m_tileMovementCosts
			[TileConstants::SECTOR_SIDE_LENGTH]
		[TileConstants::SECTOR_SIDE_LENGTH];

		// Relevant index of the tile on each border being used for 
		// pathing between sectors
		std::optional<int> m_pathingBorderTiles[static_cast<int>(PathingDirection::_COUNT)];
	};
	struct Quadrant
	{
		Sector m_sectors
			[TileConstants::QUADRANT_SIDE_LENGTH]
		[TileConstants::QUADRANT_SIDE_LENGTH];

		sf::Texture m_texture;
		std::optional<int> m_sectorCrossingPathCosts
			[TileConstants::QUADRANT_SIDE_LENGTH]
		[TileConstants::QUADRANT_SIDE_LENGTH]
		[static_cast<int>(PathingDirection::_COUNT) + 1]
		[static_cast<int>(PathingDirection::_COUNT) + 1];
		std::optional<std::deque<CoordinateVector2>> m_sectorCrossingPaths
			[TileConstants::QUADRANT_SIDE_LENGTH]
		[TileConstants::QUADRANT_SIDE_LENGTH]
		[static_cast<int>(PathingDirection::_COUNT) + 1]
		[static_cast<int>(PathingDirection::_COUNT) + 1];
	};
	using SpawnedQuadrantMap = std::map<QuadrantId, Quadrant>;
	SpawnedQuadrantMap s_spawnedQuadrants;
	bool s_baseQuadrantSpawned{ false };
	bool s_startingBuilderSpawned{ false };

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

	using WorldCoordinates = TilePosition;

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

	void GrowTerritories(ECS_Core::Manager& manager);
	TileNED::Tile & GetTile(const TilePosition& buildingTilePos, ECS_Core::Manager & manager);
	TileNED::Quadrant& FetchQuadrant(const CoordinateVector2 & quadrantCoords, ECS_Core::Manager & manager);
	void SpawnBetween(
		CoordinateVector2 origin,
		CoordinateVector2 target,
		ECS_Core::Manager& manager);
	void ReturnDeadBuildingTiles(ECS_Core::Manager& manager);

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

TilePosition& TilePosition::operator+=(const TilePosition& other)
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

TilePosition& TilePosition::operator-=(const TilePosition& other)
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
	const CoordinateVector2 & coordinates,
	int secX,
	int secY)
{
	std::vector<SectorSeedPosition> relevantSeeds;
	srand(static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
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

std::thread SpawnQuadrant(const CoordinateVector2& coordinates, ECS_Core::Manager& manager)
{
	using namespace TileConstants;
	if (TileNED::s_spawnedQuadrants.find(coordinates)
		!= TileNED::s_spawnedQuadrants.end())
	{
		// Quadrant is already here
		return std::thread([]() {});
	}

	return std::thread([&manager, coordinates]() {
		auto index = manager.createHandle();
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
							weightBorders.push_back(totalWeight += 100. / pow(distance, 10));
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

		// Threads to fill in movement costs in the sector data
		std::vector<std::thread> movementFillThreads;
		for (int sectorI = 0; sectorI < QUADRANT_SIDE_LENGTH; ++sectorI)
		{
			for (int sectorJ = 0; sectorJ < QUADRANT_SIDE_LENGTH; ++sectorJ)
			{
				auto& sector = quadrant.m_sectors[sectorI][sectorJ];
				movementFillThreads.emplace_back([&sector]() {
					for (int i = 0; i < SECTOR_SIDE_LENGTH; ++i)
					{
						for (int j = 0; j < SECTOR_SIDE_LENGTH; ++j)
						{
							sector.m_tileMovementCosts[i][j] = sector.m_tiles[i][j].m_movementCost;
						}
					}
				});
			}
		}
		for (auto&& thread : movementFillThreads)
		{
			thread.join();
		}

		// First, pick the points for input/output on each border
		// Go along each horizontal border, and each vertical
		std::vector<std::thread> borderThreads;

		for (int sectorI = 0; sectorI < QUADRANT_SIDE_LENGTH; ++sectorI)
		{
			for (int sectorJ = 0; sectorJ < QUADRANT_SIDE_LENGTH - 1; ++sectorJ)
			{
				// Horizontal border
				borderThreads.emplace_back([sectorI, sectorJ, &quadrant]() {
					auto& upperSector = quadrant.m_sectors[sectorI][sectorJ];
					auto& lowerSector = quadrant.m_sectors[sectorI][sectorJ + 1];

					auto middleIndex = SECTOR_SIDE_LENGTH / 2;
					for (int i = 1; i <= SECTOR_SIDE_LENGTH; ++i)
					{
						auto borderIndex = [&i, &middleIndex]() {
							if (i % 2 == 0) return middleIndex - (i / 2);
							else return middleIndex + (i / 2);
						}();
						// Check if each is leavable (don't try to put people on a 1-tile island
						// As well as the tile itself being pathable
						bool upperTileValid = upperSector.m_tileMovementCosts[borderIndex][SECTOR_SIDE_LENGTH - 1].has_value();
						bool lowerTileValid = lowerSector.m_tileMovementCosts[borderIndex][0].has_value();
						int upperPathableTiles = 0;
						int lowerPathableTiles = 0;

						if (upperSector.m_tileMovementCosts[borderIndex][SECTOR_SIDE_LENGTH - 2]) ++upperPathableTiles;
						if (lowerSector.m_tileMovementCosts[borderIndex][1]) ++lowerPathableTiles;
						if (borderIndex > 0)
						{
							if (upperSector.m_tileMovementCosts[borderIndex - 1][SECTOR_SIDE_LENGTH - 1]) ++upperPathableTiles;
							if (lowerSector.m_tileMovementCosts[borderIndex - 1][0]) ++lowerPathableTiles;
						}
						if (borderIndex < SECTOR_SIDE_LENGTH - 1)
						{
							if (upperSector.m_tileMovementCosts[borderIndex + 1][SECTOR_SIDE_LENGTH - 1]) ++upperPathableTiles;
							if (lowerSector.m_tileMovementCosts[borderIndex + 1][0]) ++lowerPathableTiles;
						}

						if (upperTileValid && lowerTileValid && upperPathableTiles > 0 && lowerPathableTiles > 0)
						{
							upperSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::SOUTH)] = borderIndex;
							lowerSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::NORTH)] = borderIndex;
							break;
						}
					}
				});

				// Vertical border
				borderThreads.emplace_back([sectorI, sectorJ, &quadrant]() {
					auto& leftSector = quadrant.m_sectors[sectorJ][sectorI];
					auto& rightSector = quadrant.m_sectors[sectorJ + 1][sectorI];

					auto middleIndex = SECTOR_SIDE_LENGTH / 2;
					for (int i = 1; i <= SECTOR_SIDE_LENGTH; ++i)
					{
						auto borderIndex = [&i, &middleIndex]() {
							if (i % 2 == 0) return middleIndex - (i / 2);
							else return middleIndex + (i / 2);
						}();

						// Check if each is leavable (don't try to put people on a 1-tile island
						// As well as the tile itself being pathable
						bool leftTileValid = leftSector.m_tileMovementCosts[SECTOR_SIDE_LENGTH - 1][borderIndex].has_value();
						bool rightTileValid = rightSector.m_tileMovementCosts[0][borderIndex].has_value();
						int leftPathableTiles = 0;
						int rightPathableTiles = 0;
						if (leftSector.m_tileMovementCosts[SECTOR_SIDE_LENGTH - 2][borderIndex]) ++leftPathableTiles;
						if (rightSector.m_tileMovementCosts[1][borderIndex]) ++rightPathableTiles;
						if (borderIndex > 0)
						{
							if (leftSector.m_tileMovementCosts[SECTOR_SIDE_LENGTH - 1][borderIndex - 1]) ++leftPathableTiles;
							if (rightSector.m_tileMovementCosts[0][borderIndex - 1]) ++rightPathableTiles;
						}
						if (borderIndex < SECTOR_SIDE_LENGTH - 1)
						{
							if (leftSector.m_tileMovementCosts[SECTOR_SIDE_LENGTH - 1][borderIndex + 1]) ++leftPathableTiles;
							if (rightSector.m_tileMovementCosts[0][borderIndex + 1]) ++rightPathableTiles;
						}

						if (leftTileValid && rightTileValid && leftPathableTiles > 0 && rightPathableTiles > 0)
						{
							leftSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::EAST)] = borderIndex;
							rightSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::WEST)] = borderIndex;
							break;
						}
					}
				});
			}

			// Top border (one-sided)
			borderThreads.emplace_back([sectorI, &quadrant]() {
				auto& sector = quadrant.m_sectors[sectorI][0];
				auto middleIndex = SECTOR_SIDE_LENGTH / 2;
				for (int i = 1; i <= SECTOR_SIDE_LENGTH; ++i)
				{
					auto borderIndex = [&i, &middleIndex]() {
						if (i % 2 == 0) return middleIndex - (i / 2);
						else return middleIndex + (i / 2);
					}();

					bool tileValid = sector.m_tileMovementCosts[borderIndex][0].has_value();
					int pathableTiles = 0;
					if (sector.m_tileMovementCosts[borderIndex][1]) ++pathableTiles;
					if (borderIndex > 0 && sector.m_tileMovementCosts[borderIndex - 1][0]) ++pathableTiles;
					if (borderIndex < SECTOR_SIDE_LENGTH - 1 && sector.m_tileMovementCosts[borderIndex + 1][0]) ++pathableTiles;

					if (tileValid && pathableTiles > 0)
					{
						sector.m_pathingBorderTiles[static_cast<int>(PathingDirection::NORTH)] = borderIndex;
					}
				}
			});

			// Bottom border (one-sided)
			borderThreads.emplace_back([sectorI, &quadrant]() {
				auto& sector = quadrant.m_sectors[sectorI][QUADRANT_SIDE_LENGTH - 1];
				auto middleIndex = SECTOR_SIDE_LENGTH / 2;
				for (int i = 1; i <= SECTOR_SIDE_LENGTH; ++i)
				{
					auto borderIndex = [&i, &middleIndex]() {
						if (i % 2 == 0) return middleIndex - (i / 2);
						else return middleIndex + (i / 2);
					}();

					bool tileValid = sector.m_tileMovementCosts[borderIndex][SECTOR_SIDE_LENGTH - 1].has_value();
					int pathableTiles = 0;
					if (sector.m_tileMovementCosts[borderIndex][SECTOR_SIDE_LENGTH - 2]) ++pathableTiles;
					if (borderIndex > 0 && sector.m_tileMovementCosts[borderIndex - 1][SECTOR_SIDE_LENGTH - 1]) ++pathableTiles;
					if (borderIndex < SECTOR_SIDE_LENGTH - 1 && sector.m_tileMovementCosts[borderIndex + 1][SECTOR_SIDE_LENGTH - 1]) ++pathableTiles;

					if (tileValid && pathableTiles > 0)
					{
						sector.m_pathingBorderTiles[static_cast<int>(PathingDirection::SOUTH)] = borderIndex;
					}
				}
			});

			// Left border (one-sided)
			borderThreads.emplace_back([sectorI, &quadrant]() {
				auto& sector = quadrant.m_sectors[0][sectorI];
				auto middleIndex = SECTOR_SIDE_LENGTH / 2;
				for (int i = 1; i <= SECTOR_SIDE_LENGTH; ++i)
				{
					auto borderIndex = [&i, &middleIndex]() {
						if (i % 2 == 0) return middleIndex - (i / 2);
						else return middleIndex + (i / 2);
					}();

					bool tileValid = sector.m_tileMovementCosts[0][borderIndex].has_value();
					int pathableTiles = 0;
					if (sector.m_tileMovementCosts[1][borderIndex]) ++pathableTiles;
					if (borderIndex > 0 && sector.m_tileMovementCosts[0][borderIndex - 1]) ++pathableTiles;
					if (borderIndex < SECTOR_SIDE_LENGTH - 1 && sector.m_tileMovementCosts[0][borderIndex + 1]) ++pathableTiles;

					if (tileValid && pathableTiles > 0)
					{
						sector.m_pathingBorderTiles[static_cast<int>(PathingDirection::WEST)] = borderIndex;
					}
				}
			});

			// Right border (one-sided)
			borderThreads.emplace_back([sectorI, &quadrant]() {
				auto& sector = quadrant.m_sectors[QUADRANT_SIDE_LENGTH - 1][sectorI];
				auto middleIndex = SECTOR_SIDE_LENGTH / 2;
				for (int i = 1; i <= SECTOR_SIDE_LENGTH; ++i)
				{
					auto borderIndex = [&i, &middleIndex]() {
						if (i % 2 == 0) return middleIndex - (i / 2);
						else return middleIndex + (i / 2);
					}();

					bool tileValid = sector.m_tileMovementCosts[SECTOR_SIDE_LENGTH - 1][borderIndex].has_value();
					int pathableTiles = 0;
					if (sector.m_tileMovementCosts[SECTOR_SIDE_LENGTH - 2][borderIndex]) ++pathableTiles;
					if (borderIndex > 0 && sector.m_tileMovementCosts[SECTOR_SIDE_LENGTH - 1][borderIndex - 1]) ++pathableTiles;
					if (borderIndex < SECTOR_SIDE_LENGTH - 1 && sector.m_tileMovementCosts[SECTOR_SIDE_LENGTH - 1][borderIndex + 1]) ++pathableTiles;

					if (tileValid && pathableTiles > 0)
					{
						sector.m_pathingBorderTiles[static_cast<int>(PathingDirection::EAST)] = borderIndex;
					}
				}
			});
		}
		for (auto&& thread : borderThreads)
		{
			// Make sure all finish before we start finding paths
			thread.join();
		}

		std::vector<std::thread> pathFindingThreads;
		for (int sectorI = 0; sectorI < QUADRANT_SIDE_LENGTH; ++sectorI)
		{
			for (int sectorJ = 0; sectorJ < QUADRANT_SIDE_LENGTH; ++sectorJ)
			{
				auto& sector = quadrant.m_sectors[sectorI][sectorJ];

				for (int sourceSide = static_cast<int>(PathingDirection::NORTH); sourceSide < static_cast<int>(PathingDirection::_COUNT); ++sourceSide)
				{
					for (int targetSide = static_cast<int>(PathingDirection::NORTH); targetSide < static_cast<int>(PathingDirection::_COUNT); ++targetSide)
					{
						if (sourceSide == targetSide
							|| !sector.m_pathingBorderTiles[sourceSide]
							|| !sector.m_pathingBorderTiles[targetSide])
						{
							continue;
						}

						pathFindingThreads.emplace_back([&sector, sourceSide, targetSide, &quadrant, sectorI, sectorJ]() {
							auto GetSideTile = [](auto side, auto index) -> CoordinateVector2 {
								switch (side)
								{
								case PathingDirection::NORTH: return { index, 0 };
								case PathingDirection::SOUTH: return { index, TileNED::SECTOR_SIDE_LENGTH - 1 };
								case PathingDirection::EAST: return { TileNED::SECTOR_SIDE_LENGTH - 1, index };
								case PathingDirection::WEST: return { 0, index };
								}
								return {};
							};
							auto path = Pathing::GetPath(
								sector.m_tileMovementCosts,
								GetSideTile(sourceSide, *sector.m_pathingBorderTiles[sourceSide]),
								GetSideTile(targetSide, *sector.m_pathingBorderTiles[targetSide]));
							if (path)
							{
								quadrant.m_sectorCrossingPaths[sectorI][sectorJ][sourceSide][targetSide] = path->m_path;
								quadrant.m_sectorCrossingPathCosts[sectorI][sectorJ][sourceSide][targetSide] = path->m_totalPathCost;
							}
						});
					}
				}
			}
		}
		for (auto&& thread : pathFindingThreads)
		{
			thread.join();
		}
	});
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
	using namespace ECS_Core;
	manager.forEntitiesMatching<Signatures::S_PlannedBuildingPlacement>([&manager](
		const ecs::EntityIndex&,
		const Components::C_BuildingDescription&,
		const Components::C_TilePosition& ghostTilePosition,
		Components::C_BuildingGhost& ghost)
	{
		auto& tile = GetTile(ghostTilePosition.m_position, manager);
		bool collisionFound{ tile.m_owningBuilding || !tile.m_movementCost };
		manager.forEntitiesMatching<Signatures::S_CompleteBuilding>([&collisionFound, &ghostTilePosition](
			const ecs::EntityIndex&,
			const Components::C_BuildingDescription&,
			const Components::C_TilePosition& buildingTilePosition,
			const Components::C_Territory&,
			const Components::C_TileProductionPotential&,
			const Components::C_ResourceInventory&) -> ecs::IterationBehavior
		{
			if (ghostTilePosition.m_position == buildingTilePosition.m_position)
			{
				collisionFound = true;
				return ecs::IterationBehavior::BREAK;
			}
			return ecs::IterationBehavior::CONTINUE;
		});
		manager.forEntitiesMatching<Signatures::S_InProgressBuilding>([&collisionFound, &ghostTilePosition](
			const ecs::EntityIndex&,
			const Components::C_BuildingDescription&,
			const Components::C_TilePosition& constructingTilePosition,
			const Components::C_BuildingConstruction&) -> ecs::IterationBehavior
		{
			if (ghostTilePosition.m_position == constructingTilePosition.m_position)
			{
				collisionFound = true;
				return ecs::IterationBehavior::BREAK;
			}
			return ecs::IterationBehavior::CONTINUE;
		});
		ghost.m_currentPlacementValid = !collisionFound;
		return ecs::IterationBehavior::CONTINUE;
	});
}

std::set<TileNED::TileSide> TileNED::GetAdjacents(const WorldCoordinates& coords)
{
	return {
		{ coords + WorldCoordinates{ { 0,0 },{ 0,0 },{ 1,0 } }, Direction::EAST },
	{ coords + WorldCoordinates{ { 0,0 },{ 0,0 },{ 0,1 } }, Direction::SOUTH },
	{ coords - WorldCoordinates{ { 0,0 },{ 0,0 },{ 1,0 } }, Direction::WEST },
	{ coords - WorldCoordinates{ { 0,0 },{ 0,0 },{ 0,1 } }, Direction::NORTH },
	};
}

void TileNED::GrowTerritories(ECS_Core::Manager& manager)
{
	using namespace ECS_Core;
	// Get current time
	// Assume the first entity is the one that has a valid time
	auto timeEntities = manager.entitiesMatching<ECS_Core::Signatures::S_TimeTracker>();
	if (timeEntities.size() == 0)
	{
		return;
	}
	const auto& time = manager.getComponent<ECS_Core::Components::C_TimeTracker>(timeEntities.front());

	// Run through the buildings that are in progress, make sure they contain their own building placement
	manager.forEntitiesMatching<ECS_Core::Signatures::S_InProgressBuilding>([&manager](
		const ecs::EntityIndex& entity,
		const Components::C_BuildingDescription&,
		const Components::C_TilePosition& tilePos,
		const Components::C_BuildingConstruction&)
	{
		auto& placementTile = GetTile(tilePos.m_position, manager);
		if (!placementTile.m_owningBuilding)
		{
			placementTile.m_owningBuilding = manager.getHandle(entity);
		}
		return ecs::IterationBehavior::CONTINUE;
	});

	std::random_device rd;
	std::mt19937 g(rd());
	manager.forEntitiesMatching<Signatures::S_CompleteBuilding>([&manager, &time, &g](
		const ecs::EntityIndex& territoryEntity,
		const Components::C_BuildingDescription&,
		const Components::C_TilePosition& buildingTilePos,
		Components::C_Territory& territory,
		Components::C_TileProductionPotential& yieldPotential,
		const Components::C_ResourceInventory&)
	{
		// Make sure territory is growing into a valid spot
		bool needsGrowthTile = true;
		if (!territory.m_nextGrowthTile)
		{
			// get all tiles adjacent to the territory that are not yet claimed
			std::map<s64, std::vector<TilePosition>> availableGrowthTiles;
			auto buildingWorldPos = CoordinatesToWorldPosition(buildingTilePos.m_position);
			for (auto& tile : territory.m_ownedTiles)
			{
				for (auto& adjacent : GetAdjacents(tile))
				{
					auto& adjacentTile = GetTile(adjacent.m_coords, manager);
					if (!adjacentTile.m_owningBuilding && adjacentTile.m_movementCost)
					{
						auto tileDistance = (CoordinatesToWorldPosition(adjacent.m_coords) - buildingWorldPos).MagnitudeSq();
						if (tileDistance <= 2000)
						{
							availableGrowthTiles[tileDistance].push_back(adjacent.m_coords);
						}
					}
				}
			}

			if (availableGrowthTiles.size())
			{
				auto selectedGrowthTiles = availableGrowthTiles.begin()->second;
				if (selectedGrowthTiles.size())
				{
					std::shuffle(selectedGrowthTiles.begin(), selectedGrowthTiles.end(), g);

					territory.m_nextGrowthTile = { 0.f, selectedGrowthTiles.front() };

					auto& tile = GetTile(territory.m_nextGrowthTile->m_tile, manager);
					tile.m_owningBuilding = manager.getHandle(territoryEntity);
				}
			}
		}

		// Now grow if we can
		if (territory.m_nextGrowthTile)
		{
			auto& tile = GetTile(territory.m_nextGrowthTile->m_tile, manager);
			territory.m_nextGrowthTile->m_progress += (0.2 * time.m_frameDuration / sqrt(territory.m_ownedTiles.size()));
			if (territory.m_nextGrowthTile->m_progress >= 1)
			{
				territory.m_ownedTiles.insert(territory.m_nextGrowthTile->m_tile);
				territory.m_nextGrowthTile.reset();

				// Update yield potential
				yieldPotential.m_availableYields.clear();
				for (auto&& tilePos : territory.m_ownedTiles)
				{
					auto& ownedTile = GetTile(tilePos, manager);
					auto&& yield = yieldPotential.m_availableYields[static_cast<ECS_Core::Components::YieldType>(ownedTile.m_tileType)];
					++yield.m_workableTiles;
					yield.m_productionYield = {
						{ ECS_Core::Components::Yields::FOOD, 1 } };
					yield.m_productionYield[ownedTile.m_tileType] += 2;
				}

				if (manager.hasComponent<ECS_Core::Components::C_SFMLDrawable>(territoryEntity))
				{
					auto& drawable = manager.getComponent<ECS_Core::Components::C_SFMLDrawable>(territoryEntity);
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
							static const float BORDER_PIXEL_WIDTH = 0.25f;

							auto indicatorOffset = positionOffset + CartesianVector2<f64>{BORDER_PIXEL_WIDTH, BORDER_PIXEL_WIDTH};
							auto sideIndicator = std::make_shared<sf::CircleShape>(BORDER_PIXEL_WIDTH, 4);
							sideIndicator->setFillColor({});
							switch (edge.second)
							{
							case Direction::NORTH:
								// Move half right, not down
								indicatorOffset.m_x += TileConstants::TILE_SIDE_LENGTH / 2;
								break;
							case Direction::SOUTH:
								// move half right and all the way down
								indicatorOffset.m_x += TileConstants::TILE_SIDE_LENGTH / 2;
								indicatorOffset.m_y += TileConstants::TILE_SIDE_LENGTH - (4 * BORDER_PIXEL_WIDTH);
								break;
							case Direction::EAST:
								// Move all the way right, half down
								indicatorOffset.m_x += TileConstants::TILE_SIDE_LENGTH - (4 * BORDER_PIXEL_WIDTH);
								indicatorOffset.m_y += TileConstants::TILE_SIDE_LENGTH / 2;
								break;
							case Direction::WEST:
								// Move half down, not right
								indicatorOffset.m_y += TileConstants::TILE_SIDE_LENGTH / 2;
								break;
							}
							drawable.m_drawables[ECS_Core::Components::DrawLayer::TERRAIN][static_cast<u64>(TileNED::DrawPriority::TERRITORY_BORDER)].push_back({ sideIndicator, indicatorOffset });

							// sf Line type is always 1 pixel wide, so use rectangle so we can control thickness
							auto line = std::make_shared<sf::RectangleShape>(
								isVertical
								? sf::Vector2f{ BORDER_PIXEL_WIDTH, 1.f * TileConstants::TILE_SIDE_LENGTH }
							: sf::Vector2f{ 1.f * TileConstants::TILE_SIDE_LENGTH,BORDER_PIXEL_WIDTH });

							switch (edge.second)
							{
							case Direction::NORTH:
								// Go from top left corner, no offset
								break;
							case Direction::SOUTH:
								// start from bottom left
								positionOffset.m_y += TileConstants::TILE_SIDE_LENGTH - BORDER_PIXEL_WIDTH;
								break;
							case Direction::EAST:
								// Start from top right
								positionOffset.m_x += TileConstants::TILE_SIDE_LENGTH - BORDER_PIXEL_WIDTH;
								break;
							case Direction::WEST:
								// Go from top left corner
								break;
							}
							line->setFillColor({});
							drawable.m_drawables[ECS_Core::Components::DrawLayer::TERRAIN][static_cast<u64>(TileNED::DrawPriority::TERRITORY_BORDER)].push_back({ line, positionOffset });

						}
					}
				}
			}
		}
		return ecs::IterationBehavior::CONTINUE;
	});
}

TileNED::Tile &TileNED::GetTile(const TilePosition& buildingTilePos, ECS_Core::Manager & manager)
{
	return FetchQuadrant(buildingTilePos.m_quadrantCoords, manager)
		.m_sectors[buildingTilePos.m_sectorCoords.m_x][buildingTilePos.m_sectorCoords.m_y]
		.m_tiles[buildingTilePos.m_coords.m_x][buildingTilePos.m_coords.m_y];
}

TileNED::Quadrant& TileNED::FetchQuadrant(const CoordinateVector2 & quadrantCoords, ECS_Core::Manager & manager)
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

void TileNED::ReturnDeadBuildingTiles(ECS_Core::Manager& manager)
{
	using namespace ECS_Core;
	manager.forEntitiesMatching<Signatures::S_DestroyedBuilding>([&manager](
		const ecs::EntityIndex& deadBuildingEntity,
		const Components::C_BuildingDescription&,
		const Components::C_TilePosition& buildingPosition)
	{
		auto& buildingTile = FetchQuadrant(buildingPosition.m_position.m_quadrantCoords, manager)
			.m_sectors[buildingPosition.m_position.m_sectorCoords.m_x][buildingPosition.m_position.m_sectorCoords.m_y]
			.m_tiles[buildingPosition.m_position.m_coords.m_x][buildingPosition.m_position.m_coords.m_y];
		buildingTile.m_owningBuilding.reset();

		if (manager.hasComponent<ECS_Core::Components::C_Territory>(deadBuildingEntity))
		{
			for (auto&& tile : manager.getComponent<ECS_Core::Components::C_Territory>(deadBuildingEntity).m_ownedTiles)
			{
				FetchQuadrant(tile.m_quadrantCoords, manager)
					.m_sectors[tile.m_sectorCoords.m_x][tile.m_sectorCoords.m_y]
					.m_tiles[tile.m_coords.m_x][tile.m_coords.m_y].m_owningBuilding.reset();
			}
		}
		return ecs::IterationBehavior::CONTINUE;
	});
}

void ProcessSelectTile(
	const Action::LocalPlayer::SelectTile& select,
	ECS_Core::Manager& manager,
	const ecs::EntityIndex& governorEntity)
{
	bool unitFound = false;
	manager.forEntitiesMatching<ECS_Core::Signatures::S_MovingUnit>([&unitFound, &select, &manager, &governorEntity](
		const ecs::EntityIndex& entity,
		const ECS_Core::Components::C_TilePosition& position,
		const ECS_Core::Components::C_MovingUnit&,
		const ECS_Core::Components::C_Population&) {
		if (select.m_position == position.m_position)
		{
			if (!manager.hasComponent<ECS_Core::Components::C_Selection>(entity))
			{
				auto&& governorHandle = manager.getHandle(governorEntity);
				manager.forEntitiesMatching<ECS_Core::Signatures::S_SelectedMovingUnit>([&governorHandle, &manager](
					const ecs::EntityIndex& selectedEntity,
					const ECS_Core::Components::C_TilePosition& position,
					const ECS_Core::Components::C_MovingUnit&,
					const ECS_Core::Components::C_Population&,
					const ECS_Core::Components::C_Selection& selector) {
					if (selector.m_selector == governorHandle)
					{
						manager.delComponent<ECS_Core::Components::C_Selection>(selectedEntity);
						if (manager.hasComponent<ECS_Core::Components::C_UIFrame>(selectedEntity))
						{
							manager.delComponent<ECS_Core::Components::C_UIFrame>(selectedEntity);
							if (manager.hasComponent<ECS_Core::Components::C_SFMLDrawable>(selectedEntity))
							{
								manager.getComponent<ECS_Core::Components::C_SFMLDrawable>(selectedEntity)
									.m_drawables.erase(ECS_Core::Components::DrawLayer::MENU);
							}
						}
					}
					return ecs::IterationBehavior::CONTINUE;
				});
				if (!manager.hasComponent<ECS_Core::Components::C_UIFrame>(entity))
				{
					using namespace ECS_Core::Components;
					auto& uiFrame = manager.addComponent<ECS_Core::Components::C_UIFrame>(entity);
					uiFrame.m_frame = DefineUIFrame(
						"Unit",
						UIDataReader<C_MovingUnit, int>([](const C_MovingUnit& mover) {
						return mover.m_movementPerDay;
					}),
						UIDataReader<C_Population, s32>([](const C_Population& pop) -> s32 {
						s32 result{ 0 };
						for (auto&& population : pop.m_populations)
						{
							if (population.second.m_class == PopulationClass::WORKERS)
								result += population.second.m_numMen;
						}
						return result;
					}),
						UIDataReader<C_Population, s32>([](const C_Population& pop) -> s32 {
						s32 result{ 0 };
						for (auto&& population : pop.m_populations)
						{
							if (population.second.m_class == PopulationClass::WORKERS)
								result += population.second.m_numWomen;
						}
						return result;
					}),
						UIDataReader<C_Population, s32>([](const C_Population& pop) -> s32 {
						s32 result{ 0 };
						for (auto&& population : pop.m_populations)
						{
							if (population.second.m_class == PopulationClass::CHILDREN)
								result += population.second.m_numMen + population.second.m_numWomen;
						}
						return result;
					}),
						UIDataReader<C_Population, s32>([](const C_Population& pop) -> s32 {
						s32 result{ 0 };
						for (auto&& population : pop.m_populations)
						{
							if (population.second.m_class == PopulationClass::ELDERS)
								result += population.second.m_numMen + population.second.m_numWomen;
						}
						return result;
					}),
						UIDataReader<C_Population, f64>([](const C_Population& pop) -> f64 {
						f64 totalHealth{ 0 };
						s32 totalPopulation{ 0 };
						for (auto&& population : pop.m_populations)
						{
							totalHealth += (population.second.m_mensHealth * population.second.m_numMen)
								+ (population.second.m_womensHealth * population.second.m_numWomen);
							totalPopulation += population.second.m_numMen + population.second.m_numWomen;
						}
						return totalHealth / max<s32>(1, totalPopulation);
					}),
						DataBinding(ECS_Core::Components::C_ResourceInventory, m_collectedYields));
					uiFrame.m_dataStrings[{0}] = { {}, std::make_shared<sf::Text>() };
					uiFrame.m_dataStrings[{1}] = { { 20,0 }, std::make_shared<sf::Text>() };
					uiFrame.m_dataStrings[{2}] = { { 20,30 }, std::make_shared<sf::Text>() };
					uiFrame.m_dataStrings[{3}] = { { 20,60 }, std::make_shared<sf::Text>() };
					uiFrame.m_dataStrings[{4}] = { { 20,90 }, std::make_shared<sf::Text>() };
					uiFrame.m_dataStrings[{5}] = { { 20, 140 }, std::make_shared<sf::Text>() };
					uiFrame.m_dataStrings[{6, 0}] = { { 100,0 }, std::make_shared<sf::Text>() };
					uiFrame.m_dataStrings[{6, 1}] = { { 100,30 }, std::make_shared<sf::Text>() };
					uiFrame.m_dataStrings[{6, 2}] = { { 100,60 }, std::make_shared<sf::Text>() };
					uiFrame.m_dataStrings[{6, 3}] = { { 100,90 }, std::make_shared<sf::Text>() };
					uiFrame.m_dataStrings[{6, 4}] = { { 100,120 }, std::make_shared<sf::Text>() };
					uiFrame.m_dataStrings[{6, 5}] = { { 100,150 }, std::make_shared<sf::Text>() };
					uiFrame.m_dataStrings[{6, 6}] = { { 100,180 }, std::make_shared<sf::Text>() };
					uiFrame.m_dataStrings[{6, 7}] = { { 100,210 }, std::make_shared<sf::Text>() };
					uiFrame.m_topLeftCorner = { 50,50 };
					uiFrame.m_size = { 200, 240 };
					if (!manager.hasComponent<ECS_Core::Components::C_SFMLDrawable>(entity))
					{
						manager.addComponent<ECS_Core::Components::C_SFMLDrawable>(entity);
					}
					auto& drawable = manager.getComponent<ECS_Core::Components::C_SFMLDrawable>(entity);

					auto windowBackground = std::make_shared<sf::RectangleShape>(sf::Vector2f(200, 240));

					windowBackground->setFillColor({});
					drawable.m_drawables[ECS_Core::Components::DrawLayer::MENU][0].push_back({ windowBackground,{} });

					if (manager.hasComponent<ECS_Core::Components::C_BuildingDescription>(entity))
					{
						Button moveButton;
						Button buildButton;

						moveButton.m_topLeftCorner = { 90,0 };
						moveButton.m_size = { 30,30 };
						moveButton.m_onClick = [&manager](const ecs::EntityIndex& /*clicker*/, const ecs::EntityIndex& clickedEntity) {
							return Action::LocalPlayer::PlanMotion(manager.getHandle(clickedEntity));
						};

						buildButton.m_size = { 30,30 };
						buildButton.m_onClick = [](const ecs::EntityIndex& /*clicker*/, const ecs::EntityIndex& clickedEntity) {
							return Action::SettleBuildingUnit(clickedEntity);
						};

						uiFrame.m_buttons.push_back(moveButton);
						uiFrame.m_buttons.push_back(buildButton);

						auto moveGraphic = std::make_shared<sf::RectangleShape>(sf::Vector2f(30, 30));
						auto buildGraphic = std::make_shared<sf::RectangleShape>(sf::Vector2f(30, 30));
						moveGraphic->setFillColor({ 40, 40, 200 });
						drawable.m_drawables[ECS_Core::Components::DrawLayer::MENU][1].push_back({ moveGraphic, moveButton.m_topLeftCorner });

						buildGraphic->setFillColor({ 85, 180, 100 });
						drawable.m_drawables[ECS_Core::Components::DrawLayer::MENU][1].push_back({ buildGraphic, buildButton.m_topLeftCorner });

					}
					for (auto&& dataStr : uiFrame.m_dataStrings)
					{
						dataStr.second.m_text->setFillColor({ 255,255,255 });
						dataStr.second.m_text->setOutlineColor({ 128,128,128 });
						dataStr.second.m_text->setFont(s_font);
						drawable.m_drawables[ECS_Core::Components::DrawLayer::MENU][255].push_back({ dataStr.second.m_text, dataStr.second.m_relativePosition });
					}
				}
				manager.addComponent<ECS_Core::Components::C_Selection>(entity).m_selector = governorHandle;

				unitFound = true;
			}
			return ecs::IterationBehavior::BREAK;
		}
		return ecs::IterationBehavior::CONTINUE;
	});
	if (unitFound)
	{
		return;
	}

	auto& tile = TileNED::GetTile(select.m_position, manager);
	if (tile.m_owningBuilding)
	{
		using namespace ECS_Core::Components;
		if (manager.hasComponent<C_Population>(*tile.m_owningBuilding)
			&& !manager.hasComponent<C_UIFrame>(*tile.m_owningBuilding))
		{
			auto& uiFrame = manager.addComponent<C_UIFrame>(*tile.m_owningBuilding);
			uiFrame.m_frame = DefineUIFrame("Building",
				UIDataReader<C_Population, s32>([](const C_Population& pop) -> s32 {
				s32 result{ 0 };
				for (auto&& population : pop.m_populations)
				{
					if (population.second.m_class == PopulationClass::WORKERS)
						result += population.second.m_numMen;
				}
				return result;
			}),
				UIDataReader<C_Population, s32>([](const C_Population& pop) -> s32 {
				s32 result{ 0 };
				for (auto&& population : pop.m_populations)
				{
					if (population.second.m_class == PopulationClass::WORKERS)
						result += population.second.m_numWomen;
				}
				return result;
			}),
				UIDataReader<C_Population, s32>([](const C_Population& pop) -> s32 {
				s32 result{ 0 };
				for (auto&& population : pop.m_populations)
				{
					if (population.second.m_class == PopulationClass::CHILDREN)
						result += population.second.m_numMen + population.second.m_numWomen;
				}
				return result;
			}),
				UIDataReader<C_Population, s32>([](const C_Population& pop) -> s32 {
				s32 result{ 0 };
				for (auto&& population : pop.m_populations)
				{
					if (population.second.m_class == PopulationClass::ELDERS)
						result += population.second.m_numMen + population.second.m_numWomen;
				}
				return result;
			}),
				UIDataReader<C_Population, f64>([](const C_Population& pop) -> f64 {
				f64 totalHealth{ 0 };
				s32 totalPopulation{ 0 };
				for (auto&& population : pop.m_populations)
				{
					totalHealth += (population.second.m_mensHealth * population.second.m_numMen)
						+ (population.second.m_womensHealth * population.second.m_numWomen);
					totalPopulation += population.second.m_numMen + population.second.m_numWomen;
				}
				return totalHealth / max<s32>(1, totalPopulation);
			}),
				DataBinding(ECS_Core::Components::C_ResourceInventory, m_collectedYields));
			uiFrame.m_dataStrings[{0}] = { { 20,0 }, std::make_shared<sf::Text>() };
			uiFrame.m_dataStrings[{1}] = { { 20,30 }, std::make_shared<sf::Text>() };
			uiFrame.m_dataStrings[{2}] = { { 20,60 }, std::make_shared<sf::Text>() };
			uiFrame.m_dataStrings[{3}] = { { 20,90 }, std::make_shared<sf::Text>() };
			uiFrame.m_dataStrings[{4}] = { { 20, 140 }, std::make_shared<sf::Text>() };
			uiFrame.m_dataStrings[{5, 0}] = { { 100,0 }, std::make_shared<sf::Text>() };
			uiFrame.m_dataStrings[{5, 1}] = { { 100,30 }, std::make_shared<sf::Text>() };
			uiFrame.m_dataStrings[{5, 2}] = { { 100,60 }, std::make_shared<sf::Text>() };
			uiFrame.m_dataStrings[{5, 3}] = { { 100,90 }, std::make_shared<sf::Text>() };
			uiFrame.m_dataStrings[{5, 4}] = { { 100,120 }, std::make_shared<sf::Text>() };
			uiFrame.m_dataStrings[{5, 5}] = { { 100,150 }, std::make_shared<sf::Text>() };
			uiFrame.m_dataStrings[{5, 6}] = { { 100,180 }, std::make_shared<sf::Text>() };
			uiFrame.m_dataStrings[{5, 7}] = { { 100,210 }, std::make_shared<sf::Text>() };
			uiFrame.m_size = { 200, 240 };
			uiFrame.m_topLeftCorner = { 0, 300 };

			ECS_Core::Components::Button closeButton;
			closeButton.m_topLeftCorner.m_x = uiFrame.m_size.m_x - 30;
			closeButton.m_size = { 30, 30 };
			closeButton.m_onClick = [](const ecs::EntityIndex& /*clicker*/, const ecs::EntityIndex& clickedEntity)
			{
				return Action::LocalPlayer::CloseUIFrame(clickedEntity);
			};
			uiFrame.m_buttons.push_back(closeButton);

			ECS_Core::Components::Button newBuildingButton;
			newBuildingButton.m_size = { 30,30 };
			newBuildingButton.m_topLeftCorner = uiFrame.m_size - newBuildingButton.m_size;
			newBuildingButton.m_onClick = [&manager](const ecs::EntityIndex& /*clicker*/, const ecs::EntityIndex& clickedEntity)
			{
				Action::CreateBuildingUnit create;
				create.m_movementSpeed = 5;
				create.m_popSource = clickedEntity;
				create.m_buildingTypeId = 0;
				if (manager.hasComponent<ECS_Core::Components::C_TilePosition>(clickedEntity))
				{
					create.m_spawningPosition = manager.getComponent<ECS_Core::Components::C_TilePosition>(clickedEntity).m_position;
				}
				return create;
			};
			uiFrame.m_buttons.push_back(newBuildingButton);

			ECS_Core::Components::Button newCaravanButton;
			newCaravanButton.m_size = { 30, 30 };
			newCaravanButton.m_topLeftCorner = { 0, uiFrame.m_size.m_y - 30 };
			newCaravanButton.m_onClick = [&manager](const ecs::EntityIndex& /*clicker*/, const ecs::EntityIndex& clickedEntity)
			{
				return Action::LocalPlayer::PlanCaravan(manager.getHandle(clickedEntity));
			};
			uiFrame.m_buttons.push_back(newCaravanButton);

			if (!manager.hasComponent<ECS_Core::Components::C_SFMLDrawable>(*tile.m_owningBuilding))
			{
				manager.addComponent<ECS_Core::Components::C_SFMLDrawable>(*tile.m_owningBuilding);
			}
			auto& drawable = manager.getComponent<ECS_Core::Components::C_SFMLDrawable>(*tile.m_owningBuilding);
			auto windowBackground = std::make_shared<sf::RectangleShape>(sf::Vector2f(200, 240));
			windowBackground->setFillColor({});
			drawable.m_drawables[ECS_Core::Components::DrawLayer::MENU][0].push_back({ windowBackground,{} });

			auto closeGraphic = std::make_shared<sf::RectangleShape>(sf::Vector2f(30, 30));
			closeGraphic->setFillColor({ 200, 30, 30 });
			drawable.m_drawables[ECS_Core::Components::DrawLayer::MENU][1].push_back({ closeGraphic, closeButton.m_topLeftCorner });

			auto spawnGraphic = std::make_shared<sf::RectangleShape>(sf::Vector2f(30, 30));
			spawnGraphic->setFillColor({ 30, 200, 30 });
			drawable.m_drawables[ECS_Core::Components::DrawLayer::MENU][1].push_back({ spawnGraphic, newBuildingButton.m_topLeftCorner });

			auto caravanGraphic = std::make_shared<sf::RectangleShape>(sf::Vector2f(30, 30));
			caravanGraphic->setFillColor({ 200, 100, 30 });
			drawable.m_drawables[ECS_Core::Components::DrawLayer::MENU][1].push_back({ caravanGraphic, newCaravanButton.m_topLeftCorner });

			for (auto&& dataStr : uiFrame.m_dataStrings)
			{
				dataStr.second.m_text->setFillColor({ 255,255,255 });
				dataStr.second.m_text->setOutlineColor({ 128,128,128 });
				dataStr.second.m_text->setFont(s_font);
				drawable.m_drawables[ECS_Core::Components::DrawLayer::MENU][255].push_back({ dataStr.second.m_text, dataStr.second.m_relativePosition });
			}
		}
	}
}

std::optional<ECS_Core::Components::MoveToPoint> GetPath(
	const TilePosition& sourcePosition,
	const TilePosition& targetPosition,
	ECS_Core::Manager& manager)
{
	// Make sure you can get from source tile to target tile
	// Are they in the same quadrant?
	if (sourcePosition.m_quadrantCoords != targetPosition.m_quadrantCoords)
	{
		auto& targetQuadrant = TileNED::FetchQuadrant(targetPosition.m_quadrantCoords, manager);
		auto& sourceQuadrant = TileNED::FetchQuadrant(sourcePosition.m_quadrantCoords, manager);

		// Get shortest path between quadrants

		// Then in each quadrant in path, find shortest sector path from entry to exit 
		// (if in the same quadrant, find shortest sector path from source to target)

		// For each sector in path, find shortest tile path entry to exit 
		// (if in same sector, shortest tile path source to target)
	}
	else if (sourcePosition.m_sectorCoords != targetPosition.m_sectorCoords)
	{
		auto& quadrant = TileNED::FetchQuadrant(targetPosition.m_quadrantCoords, manager);
		// Fill in the cost from current point to each side
		for (int direction = static_cast<int>(PathingDirection::NORTH); direction < static_cast<int>(PathingDirection::_COUNT); ++direction)
		{
			const auto& startingSector = quadrant.m_sectors[sourcePosition.m_sectorCoords.m_x][sourcePosition.m_sectorCoords.m_y];
			auto startingExitTile = [&direction,
				&sector = startingSector]()->CoordinateVector2 {
				switch (static_cast<PathingDirection>(direction))
				{
				case PathingDirection::NORTH: return { *sector.m_pathingBorderTiles[static_cast<int>(PathingDirection::NORTH)], 0 };
				case PathingDirection::SOUTH: return { *sector.m_pathingBorderTiles[static_cast<int>(PathingDirection::SOUTH)], TileNED::SECTOR_SIDE_LENGTH - 1 };
				case PathingDirection::EAST: return { TileNED::SECTOR_SIDE_LENGTH - 1 , *sector.m_pathingBorderTiles[static_cast<int>(PathingDirection::EAST)] };
				case PathingDirection::WEST: return { 0, *sector.m_pathingBorderTiles[static_cast<int>(PathingDirection::WEST)] };
				}
				return { 0,0 };
			}();
			auto startingPath = Pathing::GetPath(
				startingSector.m_tileMovementCosts,
				sourcePosition.m_coords,
				startingExitTile);
			if (startingPath)
			{
				quadrant.m_sectorCrossingPathCosts[sourcePosition.m_sectorCoords.m_x][sourcePosition.m_sectorCoords.m_y]
					[static_cast<int>(PathingDirection::_COUNT)][direction]
					= startingPath->m_totalPathCost;
				quadrant.m_sectorCrossingPaths[sourcePosition.m_sectorCoords.m_x][sourcePosition.m_sectorCoords.m_y]
					[static_cast<int>(PathingDirection::_COUNT)][direction]
					= startingPath->m_path;
			}
			const auto& endingSector = quadrant.m_sectors
				[targetPosition.m_sectorCoords.m_x]
			[targetPosition.m_sectorCoords.m_y];
			auto endingEntryTile = [&direction,
				&sector = endingSector]()->CoordinateVector2 {
				switch (static_cast<PathingDirection>(direction))
				{
				case PathingDirection::NORTH: return { *sector.m_pathingBorderTiles[static_cast<int>(PathingDirection::NORTH)], 0 };
				case PathingDirection::SOUTH: return { *sector.m_pathingBorderTiles[static_cast<int>(PathingDirection::SOUTH)], TileNED::SECTOR_SIDE_LENGTH - 1 };
				case PathingDirection::EAST: return { TileNED::SECTOR_SIDE_LENGTH - 1 , *sector.m_pathingBorderTiles[static_cast<int>(PathingDirection::EAST)] };
				case PathingDirection::WEST: return { 0, *sector.m_pathingBorderTiles[static_cast<int>(PathingDirection::WEST)] };
				}
				return { 0,0 };
			}();
			auto endingPath = Pathing::GetPath(
				endingSector.m_tileMovementCosts,
				endingEntryTile,
				targetPosition.m_coords);
			if (endingPath)
			{
				quadrant.m_sectorCrossingPathCosts[targetPosition.m_sectorCoords.m_x][targetPosition.m_sectorCoords.m_y]
					[direction][static_cast<int>(PathingDirection::_COUNT)]
					= endingPath->m_totalPathCost;
				quadrant.m_sectorCrossingPaths[targetPosition.m_sectorCoords.m_x][targetPosition.m_sectorCoords.m_y]
					[direction][static_cast<int>(PathingDirection::_COUNT)]
					= endingPath->m_path;
			}
		}

		auto sectorPath = Pathing::GetPath(
			quadrant.m_sectorCrossingPathCosts,
			sourcePosition.m_sectorCoords,
			targetPosition.m_sectorCoords);

		if (sectorPath && sectorPath->m_path.size() >= 2)
		{
			// Glue together: starting tile to exit tile of first sector
			// Rest of the sectors
			// Final sector entry tile to target tile.

			const auto& startingMacroPath = sectorPath->m_path.front();
			const auto& endingMacroPath = sectorPath->m_path.back();
			ECS_Core::Components::MoveToPoint overallPath;
			for (auto&& tile : *quadrant.m_sectorCrossingPaths[sourcePosition.m_sectorCoords.m_x][sourcePosition.m_sectorCoords.m_y]
				[static_cast<int>(PathingDirection::_COUNT)][static_cast<int>(startingMacroPath.m_exitDirection)])
			{
				overallPath.m_path.push_back({ { sourcePosition.m_quadrantCoords, sourcePosition.m_sectorCoords, tile },
					*quadrant.m_sectors[sourcePosition.m_sectorCoords.m_x][sourcePosition.m_sectorCoords.m_y].m_tileMovementCosts[tile.m_x][tile.m_y] });
			}

			for (int i = 1; i < sectorPath->m_path.size() - 1; ++i)
			{
				auto& path = sectorPath->m_path[i];
				auto& crossingPath = quadrant.m_sectorCrossingPaths
					[path.m_node.m_x]
				[path.m_node.m_y]
				[static_cast<int>(path.m_entryDirection)]
				[static_cast<int>(path.m_exitDirection)];
				for (auto&& tile : *crossingPath)
				{
					overallPath.m_path.push_back({ { sourcePosition.m_quadrantCoords,{ path.m_node.m_x, path.m_node.m_y }, tile },
						*quadrant.m_sectors[path.m_node.m_x][path.m_node.m_y].m_tileMovementCosts[tile.m_x][tile.m_y] });
				}
			}

			for (auto&& tile : *quadrant.m_sectorCrossingPaths[targetPosition.m_sectorCoords.m_x][targetPosition.m_sectorCoords.m_y]
				[static_cast<int>(endingMacroPath.m_entryDirection)][static_cast<int>(PathingDirection::_COUNT)])
			{
				overallPath.m_path.push_back({ { targetPosition.m_quadrantCoords, targetPosition.m_sectorCoords, tile },
					*quadrant.m_sectors[targetPosition.m_sectorCoords.m_x][targetPosition.m_sectorCoords.m_y].m_tileMovementCosts[tile.m_x][tile.m_y] });
			}
			overallPath.m_targetPosition = targetPosition;

			// Clear the temporaries
			for (int direction = static_cast<int>(PathingDirection::NORTH); direction < static_cast<int>(PathingDirection::_COUNT); ++direction)
			{
				quadrant.m_sectorCrossingPaths[sourcePosition.m_sectorCoords.m_x][sourcePosition.m_sectorCoords.m_y]
					[static_cast<int>(PathingDirection::_COUNT)][direction].reset();
				quadrant.m_sectorCrossingPathCosts[sourcePosition.m_sectorCoords.m_x][sourcePosition.m_sectorCoords.m_y]
					[static_cast<int>(PathingDirection::_COUNT)][direction].reset();
				quadrant.m_sectorCrossingPaths[targetPosition.m_sectorCoords.m_x][targetPosition.m_sectorCoords.m_y]
					[direction][static_cast<int>(PathingDirection::_COUNT)].reset();
				quadrant.m_sectorCrossingPathCosts[targetPosition.m_sectorCoords.m_x][targetPosition.m_sectorCoords.m_y]
					[direction][static_cast<int>(PathingDirection::_COUNT)].reset();
			}
			return overallPath;
		}
	}
	else
	{
		auto& quadrant = TileNED::FetchQuadrant(targetPosition.m_quadrantCoords, manager);
		auto& sector = quadrant.m_sectors[sourcePosition.m_sectorCoords.m_x][sourcePosition.m_sectorCoords.m_y];
		auto totalPath = Pathing::GetPath(sector.m_tileMovementCosts,
			sourcePosition.m_coords,
			targetPosition.m_coords);
		if (totalPath)
		{
			ECS_Core::Components::MoveToPoint path;
			for (auto&& tile : totalPath->m_path)
			{
				path.m_path.push_back({ { sourcePosition.m_quadrantCoords, sourcePosition.m_sectorCoords, tile },
					*sector.m_tileMovementCosts[tile.m_x][tile.m_y] });
			}
			path.m_targetPosition = targetPosition;
			return path;
		}
	}
	return std::nullopt;
}

void WorldTile::ProgramInit() {}
void WorldTile::SetupGameplay() {
	std::thread([&manager = m_managerRef]() {
		auto spawnThread = SpawnQuadrant({ 0, 0 }, manager);
		spawnThread.join();
		TileNED::s_baseQuadrantSpawned = true;
	}).detach();
}

void WorldTile::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	switch (phase)
	{
	case GameLoopPhase::PREPARATION:
		if (TileNED::s_baseQuadrantSpawned && !TileNED::s_startingBuilderSpawned)
		{
			using namespace TileNED;
			// Spawn the initial builder unit
			// First, pick the location
			// Try a random sector
			// and a random tile in that sector.
			// If there's a path from the tile to all edges of the sector, spawn there.
			bool tileFound{ false };
			while (!tileFound)
			{
				auto sectorX = rand() % QUADRANT_SIDE_LENGTH;
				auto sectorY = rand() % QUADRANT_SIDE_LENGTH;
				auto tileX = rand() % SECTOR_SIDE_LENGTH;
				auto tileY = rand() % SECTOR_SIDE_LENGTH;

				auto& sector = FetchQuadrant({ 0,0 }, m_managerRef).m_sectors[sectorX][sectorY];
				if (sector.m_pathingBorderTiles[static_cast<int>(PathingDirection::NORTH)] &&
					Pathing::GetPath(sector.m_tileMovementCosts,
						{ tileX, tileY },
						{ 0, *sector.m_pathingBorderTiles[static_cast<int>(PathingDirection::NORTH)] }) &&
					sector.m_pathingBorderTiles[static_cast<int>(PathingDirection::SOUTH)] &&
					Pathing::GetPath(sector.m_tileMovementCosts,
						{ tileX, tileY },
						{ SECTOR_SIDE_LENGTH - 1, *sector.m_pathingBorderTiles[static_cast<int>(PathingDirection::SOUTH)] }) &&
					sector.m_pathingBorderTiles[static_cast<int>(PathingDirection::EAST)] &&
					Pathing::GetPath(sector.m_tileMovementCosts,
						{ tileX, tileY },
						{ *sector.m_pathingBorderTiles[static_cast<int>(PathingDirection::EAST)], SECTOR_SIDE_LENGTH - 1 }) &&
					sector.m_pathingBorderTiles[static_cast<int>(PathingDirection::WEST)] &&
					Pathing::GetPath(sector.m_tileMovementCosts,
						{ tileX, tileY },
						{ *sector.m_pathingBorderTiles[static_cast<int>(PathingDirection::WEST)], 0 }))
				{
					tileFound = true;
					s_startingBuilderSpawned = true;
					m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_UserIO>([&](
						const ecs::EntityIndex&,
						const ECS_Core::Components::C_UserInputs&,
						ECS_Core::Components::C_ActionPlan& plan)
					{
						Action::CreateBuildingUnit buildingUnit;
						buildingUnit.m_spawningPosition = { 0,0, sectorX, sectorY, tileX, tileY };
						buildingUnit.m_movementSpeed = 1;
						plan.m_plan.push_back(buildingUnit);
						plan.m_plan.push_back(Action::LocalPlayer::CenterCamera(CoordinatesToWorldPosition(buildingUnit.m_spawningPosition)));
						return ecs::IterationBehavior::CONTINUE;
					});
				}

			}
		}
		break;
	case GameLoopPhase::INPUT:
		// Fill in tile position of the mouse
		m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_Input>([](
			const ecs::EntityIndex&,
			ECS_Core::Components::C_UserInputs& inputComponent)
		{
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
			return ecs::IterationBehavior::CONTINUE;
		});

		m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_Planner>([&manager = m_managerRef](
			const ecs::EntityIndex& governorEntity,
			ECS_Core::Components::C_ActionPlan& actionPlan)
		{
			for (auto&& action : actionPlan.m_plan)
			{
				if (std::holds_alternative<Action::SetTargetedMovement>(action))
				{
					auto& setMovement = std::get<Action::SetTargetedMovement>(action);
					if (setMovement.m_path) continue;
					if (!manager.hasComponent<ECS_Core::Components::C_TilePosition>(setMovement.m_mover)) continue;
					if (!manager.hasComponent<ECS_Core::Components::C_MovingUnit>(setMovement.m_mover)) continue;
					auto& sourcePosition = manager.getComponent<ECS_Core::Components::C_TilePosition>(setMovement.m_mover).m_position;
					auto& targetPosition = setMovement.m_targetPosition;

					auto path = GetPath(sourcePosition, targetPosition, manager);
					if (!path)
					{
						continue;
					}

					auto& movingUnit = manager.getComponent<ECS_Core::Components::C_MovingUnit>(setMovement.m_mover);
					movingUnit.m_currentMovement = *path;

					if (setMovement.m_targetingIcon)
					{
						manager.addTag<ECS_Core::Tags::T_Dead>(*setMovement.m_targetingIcon);
					}
				}
				else if (std::holds_alternative<Action::CreateCaravan>(action))
				{
					auto& createCaravan = std::get<Action::CreateCaravan>(action);
					auto& deliveryTile = TileNED::GetTile(createCaravan.m_deliveryPosition, manager);
					if (!deliveryTile.m_owningBuilding || !createCaravan.m_popSource)
					{
						continue;
					}
					// Target building is valid, find a path between them, check resources and population
					if (!manager.hasComponent<ECS_Core::Components::C_TilePosition>(*deliveryTile.m_owningBuilding)
						|| !manager.hasComponent<ECS_Core::Components::C_Population>(*createCaravan.m_popSource)
						|| !manager.hasComponent<ECS_Core::Components::C_ResourceInventory>(*createCaravan.m_popSource))
					{
						continue;
					}
					auto& sourcePopulation = manager.getComponent<ECS_Core::Components::C_Population>(*createCaravan.m_popSource);
					auto& sourceInventory = manager.getComponent<ECS_Core::Components::C_ResourceInventory>(*createCaravan.m_popSource);

					// Check that 
					int sourceTotalMen{ 0 };
					int sourceTotalWomen{ 0 };
					for (auto&& pop : sourcePopulation.m_populations)
					{
						if (pop.second.m_class != ECS_Core::Components::PopulationClass::WORKERS)
						{
							continue;
						}
						sourceTotalMen += pop.second.m_numMen;
						sourceTotalWomen += pop.second.m_numWomen;
					}
					int totalMenToMove = 10;
					int totalWomenToMove = 5;
					if (totalMenToMove > (sourceTotalMen - 3) || totalWomenToMove > (sourceTotalWomen - 3))
					{
						continue;
					}

					// Requires 100 food, 50 wood to create
					if (sourceInventory.m_collectedYields[ECS_Core::Components::Yields::FOOD] < 100
						|| sourceInventory.m_collectedYields[ECS_Core::Components::Yields::WOOD] < 50)
					{
						continue;
					}

					// Make sure we can get there
					auto path = GetPath(
						createCaravan.m_spawningPosition,
						manager.getComponent<ECS_Core::Components::C_TilePosition>(*deliveryTile.m_owningBuilding).m_position,
						manager);
					if (!path)
					{
						continue;
					}

					// Spawn entity for the unit, then take costs and population
					// Then we'll take 100 each of the highest resource counts
					auto newEntity = manager.createHandle();
					manager.addComponent<ECS_Core::Components::C_TilePosition>(newEntity).m_position = path->m_path.front().m_tile;
					manager.addComponent<ECS_Core::Components::C_PositionCartesian>(newEntity);
					auto& movingUnit = manager.addComponent<ECS_Core::Components::C_MovingUnit>(newEntity);
					auto& caravanPath = manager.addComponent<ECS_Core::Components::C_CaravanPath>(newEntity);
					movingUnit.m_currentMovement = *path;
					caravanPath.m_basePath = *path;
					caravanPath.m_originBuildingHandle = manager.getHandle(*createCaravan.m_popSource);
					caravanPath.m_targetBuildingHandle = *deliveryTile.m_owningBuilding;

					auto& moverInventory = manager.addComponent<ECS_Core::Components::C_ResourceInventory>(newEntity);
					auto& population = manager.addComponent<ECS_Core::Components::C_Population>(newEntity);
					int menMoved = 0;
					int womenMoved = 0;
					for (auto&& pop : sourcePopulation.m_populations)
					{
						if (menMoved == totalMenToMove && totalWomenToMove == 5)
						{
							break;
						}
						if (pop.second.m_class != ECS_Core::Components::PopulationClass::WORKERS)
						{
							continue;
						}
						auto menToMove = min<int>(totalMenToMove - menMoved, pop.second.m_numMen);
						auto womenToMove = min<int>(totalWomenToMove - womenMoved, pop.second.m_numWomen);

						auto& popCopy = population.m_populations[pop.first];
						popCopy = pop.second;
						popCopy.m_numMen = menToMove;
						popCopy.m_numWomen = womenToMove;

						pop.second.m_numMen -= menToMove;
						pop.second.m_numWomen -= womenToMove;


						menMoved += menToMove;
						womenMoved += womenToMove;
					}

					sourceInventory.m_collectedYields[ECS_Core::Components::Yields::FOOD] -= 100;
					sourceInventory.m_collectedYields[ECS_Core::Components::Yields::WOOD] -= 50;
					moverInventory.m_collectedYields[ECS_Core::Components::Yields::FOOD] = 50;

					std::map<f64, std::vector<ECS_Core::Components::YieldType>, std::greater<f64>> heldResources;
					for (auto&& resource : sourceInventory.m_collectedYields)
					{
						heldResources[resource.second].push_back(resource.first);
					}
					int resourcesMoved = 0;
					for (auto&& heldResource : heldResources)
					{
						s32 amountToMove = min<s32>(static_cast<s32>(heldResource.first), 100);
						for (auto&& resource : heldResource.second)
						{
							sourceInventory.m_collectedYields[resource] -= amountToMove;
							moverInventory.m_collectedYields[resource] += amountToMove;
							if (++resourcesMoved >= 3)
							{
								break;
							}
						}
						if (resourcesMoved >= 3)
						{
							break;
						}
					}

					auto& drawable = manager.addComponent<ECS_Core::Components::C_SFMLDrawable>(newEntity);
					auto pentagon = std::make_shared<sf::CircleShape>(2.5f, 5);
					pentagon->setFillColor({ 255, 185, 60, 128 });
					pentagon->setOutlineColor({ 255, 185, 60 });
					pentagon->setOutlineThickness(-0.5f);
					drawable.m_drawables[ECS_Core::Components::DrawLayer::UNIT][0].push_back({ pentagon,{} });

					if (createCaravan.m_targetingIcon)
					{
						manager.addTag<ECS_Core::Tags::T_Dead>(*createCaravan.m_targetingIcon);
					}
				}
				else if (std::holds_alternative<Action::SettleBuildingUnit>(action))
				{
					auto& settle = std::get<Action::SettleBuildingUnit>(action);
					if (!manager.hasComponent<ECS_Core::Components::C_BuildingDescription>(settle.m_builderIndex)
						|| !manager.hasComponent<ECS_Core::Components::C_TilePosition>(settle.m_builderIndex))
					{
						continue;
					}
					auto& position = manager.getComponent<ECS_Core::Components::C_TilePosition>(settle.m_builderIndex).m_position;
					// Check to make sure this tile is unoccupied
					if (TileNED::GetTile(position, manager).m_owningBuilding)
					{
						continue;
					}

					manager.delComponent<ECS_Core::Components::C_MovingUnit>(settle.m_builderIndex);
					manager.delComponent<ECS_Core::Components::C_UIFrame>(settle.m_builderIndex);
					if (manager.hasComponent<ECS_Core::Components::C_SFMLDrawable>(settle.m_builderIndex))
					{
						manager.getComponent<ECS_Core::Components::C_SFMLDrawable>(settle.m_builderIndex).m_drawables.erase(ECS_Core::Components::DrawLayer::MENU);
					}
					manager.addComponent<ECS_Core::Components::C_BuildingConstruction>(settle.m_builderIndex).m_placingGovernor = manager.getHandle(governorEntity);
				}
			}
			return ecs::IterationBehavior::CONTINUE;
		});
		break;
	case GameLoopPhase::ACTION:
		// Grow territories that are able to do so before taking any actions
		TileNED::GrowTerritories(m_managerRef);
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
			return ecs::IterationBehavior::CONTINUE;
		});

		TileNED::CheckBuildingPlacements(m_managerRef);

		m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_UserIO>(
			[&manager = m_managerRef](
				const ecs::EntityIndex& governorEntity,
				ECS_Core::Components::C_UserInputs& inputs,
				ECS_Core::Components::C_ActionPlan& actionPlan)
		{
			for (auto&& action : actionPlan.m_plan)
			{
				if (std::holds_alternative<Action::LocalPlayer::SelectTile>(action))
				{
					ProcessSelectTile(
						std::get<Action::LocalPlayer::SelectTile>(action),
						manager,
						governorEntity);
					continue;
				}
				else if (std::holds_alternative<Action::LocalPlayer::PlanMotion>(action))
				{
					auto& planMotion = std::get<Action::LocalPlayer::PlanMotion>(action);
					if (!manager.hasComponent<ECS_Core::Components::C_MovingUnit>(planMotion.m_moverHandle)
						|| !manager.hasComponent<ECS_Core::Components::C_TilePosition>(planMotion.m_moverHandle))
					{
						// Only try to move if we have a mover unit with a tile position
						continue;
					}
					auto targetingEntity = manager.createHandle();
					auto& targetScreenPosition = manager.addComponent<ECS_Core::Components::C_PositionCartesian>(targetingEntity);
					auto& targetTilePosition = manager.addComponent<ECS_Core::Components::C_TilePosition>(targetingEntity);
					targetTilePosition.m_position = manager.getComponent<ECS_Core::Components::C_TilePosition>(planMotion.m_moverHandle).m_position;

					auto& moverInfo = manager.addComponent<ECS_Core::Components::C_MovementTarget>(targetingEntity);
					moverInfo.m_moverHandle = planMotion.m_moverHandle;
					moverInfo.m_governorHandle = manager.getHandle(governorEntity);

					auto& drawable = manager.addComponent<ECS_Core::Components::C_SFMLDrawable>(targetingEntity);
					auto targetGraphic = std::make_shared<sf::CircleShape>(2.5f, 6);
					targetGraphic->setFillColor({ 128, 128, 0, 128 });
					targetGraphic->setOutlineColor({ 128, 128, 0 });
					targetGraphic->setOutlineThickness(-0.75f);
					drawable.m_drawables[ECS_Core::Components::DrawLayer::EFFECT][128].push_back({ targetGraphic,{} });
				}
				else if (std::holds_alternative<Action::LocalPlayer::PlanCaravan>(action))
				{
					auto& planMotion = std::get<Action::LocalPlayer::PlanCaravan>(action);
					if (!manager.hasComponent<ECS_Core::Components::C_TilePosition>(planMotion.m_sourceHandle))
					{
						// Only try to move if we have a mover unit with a tile position
						continue;
					}
					auto targetingEntity = manager.createHandle();
					auto& targetScreenPosition = manager.addComponent<ECS_Core::Components::C_PositionCartesian>(targetingEntity);
					auto& targetTilePosition = manager.addComponent<ECS_Core::Components::C_TilePosition>(targetingEntity);
					targetTilePosition.m_position = manager.getComponent<ECS_Core::Components::C_TilePosition>(planMotion.m_sourceHandle).m_position;

					auto& caravanInfo = manager.addComponent<ECS_Core::Components::C_CaravanPlan>(targetingEntity);
					caravanInfo.m_sourceBuildingHandle = planMotion.m_sourceHandle;
					caravanInfo.m_governorHandle = manager.getHandle(governorEntity);

					auto& drawable = manager.addComponent<ECS_Core::Components::C_SFMLDrawable>(targetingEntity);
					auto targetGraphic = std::make_shared<sf::CircleShape>(2.5f, 8);
					targetGraphic->setFillColor({ 128, 128, 64, 128 });
					targetGraphic->setOutlineColor({ 128, 128,64 });
					targetGraphic->setOutlineThickness(-0.75f);
					drawable.m_drawables[ECS_Core::Components::DrawLayer::EFFECT][128].push_back({ targetGraphic,{} });
				}
			}
			return ecs::IterationBehavior::CONTINUE;
		});
		break;

	case GameLoopPhase::RENDER:
		break;
	case GameLoopPhase::CLEANUP:
		TileNED::ReturnDeadBuildingTiles(m_managerRef);
		return;
	}
}

bool WorldTile::ShouldExit()
{
	return false;
}

DEFINE_SYSTEM_INSTANTIATION(WorldTile);