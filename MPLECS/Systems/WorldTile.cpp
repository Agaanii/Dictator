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

// Non-entity data the tile system needs.
namespace TileNED
{
	using QuadrantId = std::pair<int, int>;
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
		WorldCoordinates(CartesianVector2&& quad, CartesianVector2&& sec, CartesianVector2&& tile)
			: m_quadrant(quad)
			, m_sector(sec)
			, m_tile(tile)
		{}
		CartesianVector2 m_quadrant;
		CartesianVector2 m_sector;
		CartesianVector2 m_tile;
	};
}

void SpawnQuadrant(int X, int Y, ECS_Core::Manager& manager)
{
	using namespace TileConstants;
	if (TileNED::s_spawnedQuadrants.find({ X,Y }) 
		!= TileNED::s_spawnedQuadrants.end())
	{
		// Quadrant is already here
		return;
	}
	auto index = manager.createIndex();
	auto quadrantSideLength = QUADRANT_SIDE_LENGTH * SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH;
	manager.addComponent<ECS_Core::Components::C_QuadrantPosition>(
		index,
		X, Y);
	manager.addComponent<ECS_Core::Components::C_PositionCartesian>(
		index,
		BASE_QUADRANT_ORIGIN_COORDINATE +
		(quadrantSideLength * X),
		BASE_QUADRANT_ORIGIN_COORDINATE +
		(quadrantSideLength * Y),
		0);

	auto rect = std::make_unique<sf::RectangleShape>(sf::Vector2f(
		static_cast<float>(quadrantSideLength),
		static_cast<float>(quadrantSideLength)));
	auto& quadrant = TileNED::s_spawnedQuadrants[{X, Y}];
	quadrant.m_texture.create(quadrantSideLength, quadrantSideLength);
	for (auto secX = 0; secX < TileConstants::QUADRANT_SIDE_LENGTH; ++secX)
	{
		for (auto secY = 0; secY < TileConstants::QUADRANT_SIDE_LENGTH; ++secY)
		{
			auto& sector = quadrant.m_sectors[secX][secY];
			for (auto tileX = 0; tileX < TileConstants::SECTOR_SIDE_LENGTH; ++tileX)
			{
				for (auto tileY = 0; tileY < TileConstants::SECTOR_SIDE_LENGTH; ++tileY)
				{
					auto& tile = sector.m_tiles[tileX][tileY];
					// Later this will be based on all sorts of fun terrain generation
					// But that's later
					tile.m_tileType = rand() % TileConstants::TILE_TYPE_COUNT;
					if (tile.m_tileType) // Make type 0 unpathable for testing
						tile.m_movementCost = rand() % 6;
					for (auto& pixel : tile.m_tilePixels)
					{
						pixel = 
							(((tile.m_tileType & 1) ? 255 : 0) << 0) + // R
							(((tile.m_tileType & 2) ? 255 : 0) << 8) + // G
							(((tile.m_tileType & 4) ? 255 : 0) << 16)  + // B
							+ (0xFF << 24); // A
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
int sign(T x)
{
	return x < 0 ? -1 : 1;
}

template<class T, class U>
int min(T&& a, U&& b)
{
	return a < b ? a : b;
}


TileNED::WorldCoordinates WorldPositionToCoordinates(const CartesianVector2& worldPos)
{
	using namespace TileConstants;
	auto offsetFromQuadrantOrigin = worldPos - CartesianVector2(
		BASE_QUADRANT_ORIGIN_COORDINATE,
		BASE_QUADRANT_ORIGIN_COORDINATE);
	return {
		{ min(0, sign(offsetFromQuadrantOrigin.m_x)) + sign(offsetFromQuadrantOrigin.m_x) * (int)(abs(offsetFromQuadrantOrigin.m_x) / (QUADRANT_SIDE_LENGTH * SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH)),
		  min(0, sign(offsetFromQuadrantOrigin.m_y)) + sign(offsetFromQuadrantOrigin.m_y) * (int)(abs(offsetFromQuadrantOrigin.m_y) / (QUADRANT_SIDE_LENGTH * SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH))},

		{ min(0, sign(offsetFromQuadrantOrigin.m_x)) + sign(offsetFromQuadrantOrigin.m_x) * ((int)abs(offsetFromQuadrantOrigin.m_x) % (QUADRANT_SIDE_LENGTH * SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH)) / (SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH),
		  min(0, sign(offsetFromQuadrantOrigin.m_y)) + sign(offsetFromQuadrantOrigin.m_y) * ((int)abs(offsetFromQuadrantOrigin.m_x) % (QUADRANT_SIDE_LENGTH * SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH)) / (SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH)},

		{ min(0, sign(offsetFromQuadrantOrigin.m_x)) + sign(offsetFromQuadrantOrigin.m_x) * ((int)abs(offsetFromQuadrantOrigin.m_x) % (SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH)) / TILE_SIDE_LENGTH,
		  min(0, sign(offsetFromQuadrantOrigin.m_y)) + sign(offsetFromQuadrantOrigin.m_y) * ((int)abs(offsetFromQuadrantOrigin.m_x) % (SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH)) / TILE_SIDE_LENGTH}
	};
}

CartesianVector2 CoordinatesToWorldPosition(const TileNED::WorldCoordinates& worldCoords)
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


void CheckWorldClick(ECS_Core::Manager& manager)
{
	auto inputEntities = manager.entitiesMatching<ECS_Core::Signatures::S_Input>();
	if (inputEntities.size() == 0) return;
	ECS_Core::Components::C_UserInputs& inputComponent = manager.getComponent<ECS_Core::Components::C_UserInputs>(inputEntities.front());
	if (inputComponent.m_unprocessedThisFrameDownMouseButtonFlags & (u8)ECS_Core::Components::MouseButtons::LEFT)
	{
		auto mouseWorldCoords = WorldPositionToCoordinates(inputComponent.m_currentMousePosition.m_worldPosition);
		
		if (TileNED::s_spawnedQuadrants.find({ (int)mouseWorldCoords.m_quadrant.m_x, (int)mouseWorldCoords.m_quadrant.m_y }) == TileNED::s_spawnedQuadrants.end())
		{
			SpawnQuadrant((int)mouseWorldCoords.m_quadrant.m_x, (int)mouseWorldCoords.m_quadrant.m_y, manager);
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
			SpawnQuadrant(0, 0, m_managerRef);
			TileNED::baseQuadrantSpawned = true;
		}
		break;
	case GameLoopPhase::ACTION:
		CheckWorldClick(m_managerRef);
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