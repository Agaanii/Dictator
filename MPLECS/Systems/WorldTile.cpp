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
	};

	using Pathability = bool[Pathing::PathingSide::_COUNT][Pathing::PathingSide::_COUNT];

	struct Sector
	{
		Pathability m_pathability;
		Tile m_tiles
			[TileConstants::SECTOR_SIDE_LENGTH]
		[TileConstants::SECTOR_SIDE_LENGTH];
		int m_seedTileType;
		CoordinateVector2 m_seedPosition;
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

	struct WorldCoordinates
	{
		WorldCoordinates() { }
		WorldCoordinates(CoordinateVector2&& quad, CoordinateVector2&& sec, CoordinateVector2&& tile)
			: m_quadrant(quad)
			, m_sector(sec)
			, m_tile(tile)
		{}
		CoordinateVector2 m_quadrant;
		CoordinateVector2 m_sector;
		CoordinateVector2 m_tile;
	};

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

			auto quad = TileNED::s_spawnedQuadrants.find(quadPosition);
			if (quad == TileNED::s_spawnedQuadrants.end())
			{
				continue;
			}
			auto& seedingSector = quad->second.m_sectors[secPosition.m_x][secPosition.m_y];

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
		BASE_QUADRANT_ORIGIN_COORDINATE +
		(quadrantSideLength * coordinates.m_x),
		BASE_QUADRANT_ORIGIN_COORDINATE +
		(quadrantSideLength * coordinates.m_y),
		0);

	auto rect = std::make_unique<sf::RectangleShape>(sf::Vector2f(
		static_cast<float>(quadrantSideLength),
		static_cast<float>(quadrantSideLength)));
	auto& quadrant = TileNED::s_spawnedQuadrants[coordinates];
	quadrant.m_texture.create(quadrantSideLength, quadrantSideLength);
	for (auto secX = 0; secX < TileConstants::QUADRANT_SIDE_LENGTH; ++secX)
	{
		for (auto secY = 0; secY < TileConstants::QUADRANT_SIDE_LENGTH; ++secY)
		{
			auto& sector = quadrant.m_sectors[secX][secY];
			sector.m_seedTileType = rand() % TileConstants::TILE_TYPE_COUNT;
			sector.m_seedPosition = 
				{rand() % TileConstants::SECTOR_SIDE_LENGTH,
				 rand() % TileConstants::SECTOR_SIDE_LENGTH};
		}
	}

	for (auto secX = 0; secX < TileConstants::QUADRANT_SIDE_LENGTH; ++secX)
	{
		for (auto secY = 0; secY < TileConstants::QUADRANT_SIDE_LENGTH; ++secY)
		{
			auto& sector = quadrant.m_sectors[secX][secY];

			auto relevantSeeds = GetRelevantSeeds(coordinates, secX, secY);
			assert(relevantSeeds.size() > 0);

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
						weightBorders.push_back(totalWeight += 1.0 / (distance * distance));
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
						tile.m_movementCost = (rand() % 6) + 1;
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

			using namespace Pathing::PathingSide;
			bool nwPathable = false;
			if (sector.m_tiles[0][0].m_movementCost)
			{
				nwPathable = true;
			}
			else
			{
				//  Start at top and left edge, other than corner, start trying to find paths
				auto isPathable = [](const TileNED::Tile& tile) -> bool {
					return (bool)tile.m_movementCost;
				};
				for (auto&& tileRow : sector.m_tiles)
				{
					for (auto&& tile : tileRow)
					{
						bool pathable = isPathable(tile);
					}
				}
			}
			sector.m_pathability[NORTH][WEST] = nwPathable;
			sector.m_pathability[WEST][NORTH] = nwPathable;

			bool nePathable = false;
			if (sector.m_tiles[TILE_SIDE_LENGTH - 1][0].m_movementCost)
			{
				nePathable = true;
			}
			else
			{
			}
			sector.m_pathability[NORTH][EAST] = nePathable;
			sector.m_pathability[EAST][NORTH] = nePathable;

			bool sePathable = false;
			if (sector.m_tiles[TILE_SIDE_LENGTH - 1][TILE_SIDE_LENGTH - 1].m_movementCost)
			{
				sePathable = true;
				sector.m_pathability[SOUTH][EAST] = true;
				sector.m_pathability[EAST][SOUTH] = true;
			}

			bool swPathable = false;
			if (sector.m_tiles[0][TILE_SIDE_LENGTH - 1].m_movementCost)
			{
				swPathable = true;
			}
			else
			{

			}
			sector.m_pathability[SOUTH][WEST] = swPathable;
			sector.m_pathability[WEST][SOUTH] = swPathable;
		}
	}
	rect->setTexture(&quadrant.m_texture);
	manager.addComponent<ECS_Core::Components::C_SFMLDrawable>(
		index,
		std::move(rect),
		ECS_Core::Components::DrawLayer::TERRAIN,
		0);
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
	return {
		{ min(0, sign(offsetFromQuadrantOrigin.m_x)) + sign(offsetFromQuadrantOrigin.m_x) * (abs(offsetFromQuadrantOrigin.m_x) / (QUADRANT_SIDE_LENGTH * SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH)),
		min(0, sign(offsetFromQuadrantOrigin.m_y)) + sign(offsetFromQuadrantOrigin.m_y) * (abs(offsetFromQuadrantOrigin.m_y) / (QUADRANT_SIDE_LENGTH * SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH)) },

		{ min(0, sign(offsetFromQuadrantOrigin.m_x)) + sign(offsetFromQuadrantOrigin.m_x) * (abs(offsetFromQuadrantOrigin.m_x) % (QUADRANT_SIDE_LENGTH * SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH)) / (SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH),
		min(0, sign(offsetFromQuadrantOrigin.m_y)) + sign(offsetFromQuadrantOrigin.m_y) * (abs(offsetFromQuadrantOrigin.m_x) % (QUADRANT_SIDE_LENGTH * SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH)) / (SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH) },

		{ min(0, sign(offsetFromQuadrantOrigin.m_x)) + sign(offsetFromQuadrantOrigin.m_x) * (abs(offsetFromQuadrantOrigin.m_x) % (SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH)) / TILE_SIDE_LENGTH,
		min(0, sign(offsetFromQuadrantOrigin.m_y)) + sign(offsetFromQuadrantOrigin.m_y) * (abs(offsetFromQuadrantOrigin.m_x) % (SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH)) / TILE_SIDE_LENGTH }
	};
}

CoordinateVector2 CoordinatesToWorldPosition(const TileNED::WorldCoordinates& worldCoords)
{
	using namespace TileConstants;
	return {
		BASE_QUADRANT_ORIGIN_COORDINATE +
		(((((QUADRANT_SIDE_LENGTH * worldCoords.m_quadrant.m_x)
			+ worldCoords.m_sector.m_x) * SECTOR_SIDE_LENGTH)
			+ worldCoords.m_tile.m_x) * TILE_SIDE_LENGTH),

		BASE_QUADRANT_ORIGIN_COORDINATE +
		(((((QUADRANT_SIDE_LENGTH * worldCoords.m_quadrant.m_y)
			+ worldCoords.m_sector.m_y) * SECTOR_SIDE_LENGTH)
			+ worldCoords.m_tile.m_y) * TILE_SIDE_LENGTH)
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
	s64 smallestDistance = std::numeric_limits<int>::max();
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
	s64 smallestDistance = std::numeric_limits<int>::max();
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

void TileNED::CheckWorldClick(ECS_Core::Manager& manager)
{
	auto inputEntities = manager.entitiesMatching<ECS_Core::Signatures::S_Input>();
	if (inputEntities.size() == 0) return;
	ECS_Core::Components::C_UserInputs& inputComponent = manager.getComponent<ECS_Core::Components::C_UserInputs>(inputEntities.front());
	if (inputComponent.m_unprocessedThisFrameDownMouseButtonFlags & (u8)ECS_Core::Components::MouseButtons::LEFT)
	{
		auto mouseWorldCoords = WorldPositionToCoordinates(inputComponent.m_currentMousePosition.m_worldPosition.cast<s64>());
		auto&& quadrantCoords = mouseWorldCoords.m_quadrant;

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

		inputComponent.ProcessMouseDown(ECS_Core::Components::MouseButtons::LEFT);
	}
}

void WorldTile::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	switch (phase)
	{
	case GameLoopPhase::PREPARATION:
		if (!TileNED::baseQuadrantSpawned)
		{
			SpawnQuadrant({0, 0}, m_managerRef);
			TileNED::baseQuadrantSpawned = true;
		}
		break;
	case GameLoopPhase::ACTION:
		TileNED::CheckWorldClick(m_managerRef);
		break;

	case GameLoopPhase::INPUT:
	case GameLoopPhase::ACTION_RESPONSE:
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