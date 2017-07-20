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
// * Sector: A 100 x 100 grid of Tiles.
// * Quadrant: A 4x4 Grid of Sectors
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
	constexpr int SECTOR_SIDE_LENGTH = 20;
	constexpr int QUADRANT_SIDE_LENGTH = 2;

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

	struct Sector
	{
		ecs::EntityIndex m_tiles
			[TileConstants::SECTOR_SIDE_LENGTH]
			[TileConstants::SECTOR_SIDE_LENGTH];
	};
	struct Quadrant
	{
		Sector m_sectors
			[TileConstants::QUADRANT_SIDE_LENGTH]
			[TileConstants::QUADRANT_SIDE_LENGTH];
	};
	using SpawnedQuadrantMap = std::map<QuadrantId, Quadrant>;
	SpawnedQuadrantMap s_spawnedQuadrants;
	bool baseQuadrantSpawned{ false };
}

ecs::EntityIndex SpawnTile(
	int quadX, int quadY, 
	int secX, int secY,
	int X, int Y,
	ECS_Core::Manager& manager)
{
	auto index = manager.createIndex();
	manager.addComponent<ECS_Core::Components::C_TilePosition>(
		index,
		quadX, quadY,
		secX, secY,
		X, Y);
	manager.addComponent<ECS_Core::Components::C_PositionCartesian>(
		index,
		TileConstants::BASE_QUADRANT_ORIGIN_COORDINATE +
			(TileConstants::QUADRANT_SIDE_LENGTH * TileConstants::SECTOR_SIDE_LENGTH * TileConstants::TILE_SIDE_LENGTH * quadX) +
			(TileConstants::SECTOR_SIDE_LENGTH * TileConstants::TILE_SIDE_LENGTH * secX) +
			(TileConstants::TILE_SIDE_LENGTH * X),
		TileConstants::BASE_QUADRANT_ORIGIN_COORDINATE +
			(TileConstants::QUADRANT_SIDE_LENGTH * TileConstants::SECTOR_SIDE_LENGTH * TileConstants::TILE_SIDE_LENGTH * quadY) +
			(TileConstants::SECTOR_SIDE_LENGTH * TileConstants::TILE_SIDE_LENGTH * secY) +
			(TileConstants::TILE_SIDE_LENGTH * Y),
		0);
	auto& props = manager.addComponent<ECS_Core::Components::C_TileProperties>(index);
	// Later this will be based on all sorts of fun terrain generation
	// But that's later
	props.m_tileType = rand() % TileConstants::TILE_TYPE_COUNT;
	int moveCost = rand() % 6;
	if (moveCost) props.m_movementCost = moveCost;

	auto rect = std::make_unique<sf::RectangleShape>(sf::Vector2f(
		static_cast<float>(TileConstants::TILE_SIDE_LENGTH),
		static_cast<float>(TileConstants::TILE_SIDE_LENGTH)));
	rect->setFillColor({
		(sf::Uint8)((props.m_tileType & 1) ? 255 : 0),
		(sf::Uint8)((props.m_tileType & 2) ? 255 : 0),
		(sf::Uint8)((props.m_tileType & 4) ? 255 : 0) });
	manager.addComponent<ECS_Core::Components::C_SFMLDrawable>(
		index,
		std::move(rect),
		ECS_Core::Components::DrawLayer::TERRAIN,
		0);
	return index;
}

void SpawnQuadrant(int X, int Y, ECS_Core::Manager& manager)
{
	if (TileNED::s_spawnedQuadrants.find({ X,Y }) 
		!= TileNED::s_spawnedQuadrants.end())
	{
		// Tile is already here
		return;
	}
	auto& quadrant = TileNED::s_spawnedQuadrants[{X, Y}];
	for (auto secX = 0; secX < TileConstants::QUADRANT_SIDE_LENGTH; ++secX)
	{
		for (auto secY = 0; secY < TileConstants::QUADRANT_SIDE_LENGTH; ++secY)
		{
			auto& sector = quadrant.m_sectors[secX][secY];
			for (auto tileX = 0; tileX < TileConstants::SECTOR_SIDE_LENGTH; ++tileX)
			{
				for (auto tileY = 0; tileY < TileConstants::SECTOR_SIDE_LENGTH; ++tileY)
				{
					sector.m_tiles[tileX][tileY] = SpawnTile(X, Y, secX, secY, tileX, tileY, manager);
				}
			}
		}
	}
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