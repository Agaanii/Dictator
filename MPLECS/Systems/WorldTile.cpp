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

	struct Sector
	{
		Tile m_tiles
			[TileConstants::SECTOR_SIDE_LENGTH]
			[TileConstants::SECTOR_SIDE_LENGTH];
	};
	struct Quadrant
	{
		Sector m_sectors
			[TileConstants::QUADRANT_SIDE_LENGTH]
			[TileConstants::QUADRANT_SIDE_LENGTH];

		sf::Texture m_texture;
	};
	using SpawnedQuadrantMap = std::map<QuadrantId, Quadrant>;
	SpawnedQuadrantMap s_spawnedQuadrants;
	bool baseQuadrantSpawned{ false };
}

void SpawnQuadrant(int X, int Y, ECS_Core::Manager& manager)
{
	using namespace TileConstants;
	if (TileNED::s_spawnedQuadrants.find({ X,Y }) 
		!= TileNED::s_spawnedQuadrants.end())
	{
		// Tile is already here
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
		}
	}
	rect->setTexture(&quadrant.m_texture);
	manager.addComponent<ECS_Core::Components::C_SFMLDrawable>(
		index,
		std::move(rect),
		ECS_Core::Components::DrawLayer::TERRAIN,
		0);
}

void WorldTile::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	switch (phase)
	{
	case GameLoopPhase::PREPARATION:
		if (!TileNED::baseQuadrantSpawned) SpawnQuadrant(0, 0, m_managerRef);
		break;
	case GameLoopPhase::INPUT:
	case GameLoopPhase::ACTION:
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