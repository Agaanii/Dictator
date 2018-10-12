//-----------------------------------------------------------------------------
// All code is property of Dictator Developers Inc
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2018

// Systems/WorldTile.cpp
// Creates and updates all tiles in the world
// When interactions extend to a new set of tiles, creates those and starts updating them

#include "../Core/typedef.h"

#include "WorldTile.h"

#include "../Util/Pathing.h"

#include "../Components/UIComponents.h"

#include <chrono>
#include <limits>
#include <mutex>
#include <random>

extern sf::Font s_font;
using namespace ECS_Core::Components;

enum class DrawPriority
{
	LANDSCAPE,
	TERRITORY_BORDER,
	FLAVOR_BUILDING,
	LOGICAL_BUILDING,
};

bool WorldTile::SortByOriginDist::operator()(
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

TilePosition& TilePosition::operator+=(const TilePosition& other)
{
	m_quadrantCoords += other.m_quadrantCoords;
	m_sectorCoords += other.m_sectorCoords;
	m_coords += other.m_coords;
	for (; m_coords.m_x < 0; m_coords.m_x += TileConstants::SECTOR_SIDE_LENGTH, --m_sectorCoords.m_x);
	for (; m_coords.m_y < 0; m_coords.m_y += TileConstants::SECTOR_SIDE_LENGTH, --m_sectorCoords.m_y);
	for (; m_sectorCoords.m_x < 0; m_sectorCoords.m_x += TileConstants::QUADRANT_SIDE_LENGTH, --m_quadrantCoords.m_x);
	for (; m_sectorCoords.m_y < 0; m_sectorCoords.m_y += TileConstants::QUADRANT_SIDE_LENGTH, --m_quadrantCoords.m_y);

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

void WorldTile::SeedForQuadrant(const CoordinateVector2& coordinates)
{
	for (int x = -1; x < 2; ++x)
	{
		for (int y = -1; y < 2; ++y)
		{
			m_quadrantSeeds[{x + coordinates.m_x, y + coordinates.m_y}];
		}
	}
}

std::vector<WorldTile::SectorSeedPosition> WorldTile::GetRelevantSeeds(
	const CoordinateVector2 & coordinates,
	int secX,
	int secY) const
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

			auto quad = m_quadrantSeeds.at(quadPosition);
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

template<int SQUARE_SIDE_LENGTH>
bool WithinSquare(const CoordinateVector2& coords)
{
	return coords.m_x >= 0
		&& coords.m_y >= 0
		&& coords.m_x < SQUARE_SIDE_LENGTH
		&& coords.m_y < SQUARE_SIDE_LENGTH;
}

std::thread WorldTile::SpawnQuadrant(const CoordinateVector2& coordinates)
{
	srand(static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
	using namespace TileConstants;
	if (m_spawnedQuadrants.find(coordinates)
		!= m_spawnedQuadrants.end())
	{
		// Quadrant is already here
		return std::thread([]() {});
	}

	return std::thread([&manager = m_managerRef, coordinates, this]() {
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
		auto& quadrant = m_spawnedQuadrants[coordinates];
		SeedForQuadrant(coordinates);
		quadrant.m_texture.create(quadrantSideLength, quadrantSideLength);
		std::mutex textureUpdateMutex, randomMutex;
		std::vector<std::thread> tileCreationThreads;
		for (auto secX = 0; secX < TileConstants::QUADRANT_SIDE_LENGTH; ++secX)
		{
			for (auto secY = 0; secY < TileConstants::QUADRANT_SIDE_LENGTH; ++secY)
			{
				tileCreationThreads.emplace_back(
					[secY, secX, &coordinates, &textureUpdateMutex, &randomMutex, &quadrant, this]() {
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
								std::lock_guard randomLock(randomMutex);
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
							{
								std::lock_guard textureLock(textureUpdateMutex);
								quadrant.m_texture.update(
									reinterpret_cast<const sf::Uint8*>(tile.m_tilePixels._Elems),
									TILE_SIDE_LENGTH,
									TILE_SIDE_LENGTH,
									((secX * SECTOR_SIDE_LENGTH) + tileX) * TILE_SIDE_LENGTH,
									((secY * SECTOR_SIDE_LENGTH) + tileY) * TILE_SIDE_LENGTH);
							}
						}
					}
				});
			}
		}
		for (auto&& thread : tileCreationThreads)
		{
			thread.join();
		}
		rect->setTexture(&quadrant.m_texture);
		auto& drawable = manager.addComponent<ECS_Core::Components::C_SFMLDrawable>(index);
		drawable.m_drawables[ECS_Core::Components::DrawLayer::TERRAIN][static_cast<u64>(DrawPriority::LANDSCAPE)].push_back({ rect,{ 0,0 } });

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

		// Starting in the middle of each sector, find the nearest (by movement cost)
		// edge tile on each edge
		std::vector<std::thread> borderThreads;

		for (int sectorI = 0; sectorI < QUADRANT_SIDE_LENGTH; ++sectorI)
		{
			for (int sectorJ = 0; sectorJ < QUADRANT_SIDE_LENGTH; ++sectorJ)
			{
				borderThreads.emplace_back([sectorI, sectorJ, &quadrant]() {
					auto& sector = quadrant.m_sectors[sectorI][sectorJ];

					// Select a tile in the middle of the sector
					auto midpoint = []() constexpr -> s64 {
						if constexpr(SECTOR_SIDE_LENGTH % 2 == 0)
						{
							return SECTOR_SIDE_LENGTH / 2 - 1;
						}
						else
						{
							return SECTOR_SIDE_LENGTH / 2;
						}
					}();
					// Spiral out, until a moveable tile is found
					int xOffset = 0;
					int yOffset = 0;
					int nextXChange = 0;
					int nextYChange = -1;
					for (int i = 0; i < SECTOR_SIDE_LENGTH * SECTOR_SIDE_LENGTH; ++i, xOffset += nextXChange, yOffset += nextYChange)
					{
						if (sector.m_tileMovementCosts[midpoint + xOffset][midpoint + yOffset])
						{
							auto centerTileCoords = CoordinateVector2{ midpoint + xOffset, midpoint + yOffset };

							// Expand out from selected center tile, ordered by movement cost to point
							bool visited[SECTOR_SIDE_LENGTH][SECTOR_SIDE_LENGTH];
							for (int i = 0; i < SECTOR_SIDE_LENGTH; ++i)
							{
								for (int j = 0; j < SECTOR_SIDE_LENGTH; ++j)
								{
									visited[i][j] = false;
								}
							}
							std::map<s64, std::vector<CoordinateVector2>> openTiles;
							openTiles[0].push_back(centerTileCoords);

							while (openTiles.size())
							{
								auto iter = openTiles.begin();
								auto& tile = iter->second.front();
								if (visited[tile.m_x][tile.m_y])
								{
									iter->second.erase(iter->second.begin());
									if (iter->second.size() == 0)
									{
										openTiles.erase(iter->first);
									}
									continue;
								}
								visited[tile.m_x][tile.m_y] = true;
								if (tile.m_y == 0)
								{
									sector.m_pathingBorderTileCandidates[static_cast<u8>(PathingDirection::NORTH)][iter->first].push_back(tile.m_x);
								}
								if (tile.m_y == SECTOR_SIDE_LENGTH - 1)
								{
									sector.m_pathingBorderTileCandidates[static_cast<u8>(PathingDirection::SOUTH)][iter->first].push_back(tile.m_x);
								}
								if (tile.m_x == SECTOR_SIDE_LENGTH - 1)
								{
									sector.m_pathingBorderTileCandidates[static_cast<u8>(PathingDirection::EAST)][iter->first].push_back(tile.m_y);
								}
								if (tile.m_x == 0)
								{
									sector.m_pathingBorderTileCandidates[static_cast<u8>(PathingDirection::WEST)][iter->first].push_back(tile.m_y);
								}

								auto northNext = tile + Pathing::neighborOffsets[static_cast<u8>(PathingDirection::NORTH)];
								if (WithinSquare<SECTOR_SIDE_LENGTH>(northNext) && sector.m_tileMovementCosts[northNext.m_x][northNext.m_y]
									&& !visited[northNext.m_x][northNext.m_y])
								{
									openTiles[*sector.m_tileMovementCosts[northNext.m_x][northNext.m_y] + iter->first].push_back(northNext);
								}

								auto southNext = tile + Pathing::neighborOffsets[static_cast<u8>(PathingDirection::SOUTH)];
								if (WithinSquare<SECTOR_SIDE_LENGTH>(southNext) && sector.m_tileMovementCosts[southNext.m_x][southNext.m_y]
									&& !visited[southNext.m_x][southNext.m_y])
								{
									openTiles[*sector.m_tileMovementCosts[southNext.m_x][southNext.m_y] + iter->first].push_back(southNext);
								}

								auto eastNext = tile + Pathing::neighborOffsets[static_cast<u8>(PathingDirection::EAST)];
								if (WithinSquare<SECTOR_SIDE_LENGTH>(eastNext) && sector.m_tileMovementCosts[eastNext.m_x][eastNext.m_y]
									&& !visited[eastNext.m_x][eastNext.m_y])
								{
									openTiles[*sector.m_tileMovementCosts[eastNext.m_x][eastNext.m_y] + iter->first].push_back(eastNext);
								}

								auto westNext = tile + Pathing::neighborOffsets[static_cast<u8>(PathingDirection::WEST)];
								if (WithinSquare<SECTOR_SIDE_LENGTH>(westNext) && sector.m_tileMovementCosts[westNext.m_x][westNext.m_y]
									&& !visited[westNext.m_x][westNext.m_y])
								{
									openTiles[*sector.m_tileMovementCosts[westNext.m_x][westNext.m_y] + iter->first].push_back(westNext);
								}
								iter->second.erase(iter->second.begin());
								if (iter->second.size() == 0)
								{
									openTiles.erase(iter->first);
								}
							}
							if (sector.m_pathingBorderTileCandidates[static_cast<u8>(PathingDirection::NORTH)].size()
								|| sector.m_pathingBorderTileCandidates[static_cast<u8>(PathingDirection::SOUTH)].size()
								|| sector.m_pathingBorderTileCandidates[static_cast<u8>(PathingDirection::EAST)].size()
								|| sector.m_pathingBorderTileCandidates[static_cast<u8>(PathingDirection::WEST)].size())
							{
								// First time we find a way to the edge, keep it
								return;
							}
						}
						if (xOffset == yOffset || (xOffset < 0 && (xOffset == -yOffset)) || (xOffset > 0 && xOffset == 1 - yOffset))
						{
							std::swap(nextXChange, nextYChange);
							nextXChange = -nextXChange;
						}
					}
				});
			}
		}
		for (auto&& thread : borderThreads)
		{
			// Make sure all finish before we start finding paths
			thread.join();
		}

		std::vector<std::thread> borderSelectionThreads;
		for (int sectorI = 0; sectorI < QUADRANT_SIDE_LENGTH; ++sectorI)
		{
			for (int sectorJ = 0; sectorJ < QUADRANT_SIDE_LENGTH - 1; ++sectorJ)
			{
				auto& northSector = quadrant.m_sectors[sectorI][sectorJ];
				auto& southSector = quadrant.m_sectors[sectorI][sectorJ + 1];
				auto& westSector = quadrant.m_sectors[sectorJ][sectorI];
				auto& eastSector = quadrant.m_sectors[sectorJ + 1][sectorI];

				// Upper/lower border
				auto northSouthTile = FindCommonBorderTile(northSector, static_cast<int>(PathingDirection::SOUTH),
					southSector, static_cast<int>(PathingDirection::NORTH));
				if (northSouthTile)
				{
					northSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::SOUTH)] = *northSouthTile;
					southSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::NORTH)] = *northSouthTile;
				}

				// Left/right border
				auto eastWestTile = FindCommonBorderTile(westSector, static_cast<int>(PathingDirection::EAST),
					eastSector, static_cast<int>(PathingDirection::WEST));
				if (eastWestTile)
				{
					westSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::EAST)] = *eastWestTile;
					eastSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::WEST)] = *eastWestTile;
				}
			}

			auto westQuadrantIter = m_spawnedQuadrants.find(coordinates + Pathing::neighborOffsets[static_cast<int>(PathingDirection::WEST)]);
			if (westQuadrantIter == m_spawnedQuadrants.end())
			{
				auto& sector = quadrant.m_sectors[0][sectorI];
				if (sector.m_pathingBorderTileCandidates[static_cast<int>(PathingDirection::WEST)].size() > 0)
				{
					sector.m_pathingBorderTiles[static_cast<int>(PathingDirection::WEST)] =
						sector.m_pathingBorderTileCandidates[static_cast<int>(PathingDirection::WEST)].begin()->second.front();
				}
			}
			else
			{
				auto& eastSector = quadrant.m_sectors[0][sectorI];
				auto& westSector = westQuadrantIter->second.m_sectors[QUADRANT_SIDE_LENGTH - 1][sectorI];
				auto borderTile = FindCommonBorderTile(westSector, static_cast<int>(PathingDirection::EAST),
					eastSector, static_cast<int>(PathingDirection::WEST));
				if (borderTile)
				{
					westSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::EAST)] = *borderTile;
					eastSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::WEST)] = *borderTile;
				}
			}

			auto eastQuadrantIter = m_spawnedQuadrants.find(coordinates + Pathing::neighborOffsets[static_cast<int>(PathingDirection::EAST)]);
			if (eastQuadrantIter == m_spawnedQuadrants.end())
			{
				auto& eastSector = quadrant.m_sectors[QUADRANT_SIDE_LENGTH - 1][sectorI];
				if (eastSector.m_pathingBorderTileCandidates[static_cast<int>(PathingDirection::EAST)].size() > 0)
				{
					eastSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::EAST)] =
						eastSector.m_pathingBorderTileCandidates[static_cast<int>(PathingDirection::EAST)].begin()->second.front();
				}
			}
			else
			{
				auto& westSector = quadrant.m_sectors[QUADRANT_SIDE_LENGTH - 1][sectorI];
				auto& eastSector = eastQuadrantIter->second.m_sectors[0][sectorI];
				auto borderTile = FindCommonBorderTile(westSector, static_cast<int>(PathingDirection::EAST),
					eastSector, static_cast<int>(PathingDirection::WEST));
				if (borderTile)
				{
					westSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::EAST)] = *borderTile;
					eastSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::WEST)] = *borderTile;
				}
			}

			auto northQuadrantIter = m_spawnedQuadrants.find(coordinates + Pathing::neighborOffsets[static_cast<int>(PathingDirection::NORTH)]);
			if (northQuadrantIter == m_spawnedQuadrants.end())
			{
				auto& northSector = quadrant.m_sectors[sectorI][0];
				if (northSector.m_pathingBorderTileCandidates[static_cast<int>(PathingDirection::NORTH)].size() > 0)
				{
					northSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::NORTH)] =
						northSector.m_pathingBorderTileCandidates[static_cast<int>(PathingDirection::NORTH)].begin()->second.front();
				}
			}
			else
			{
				auto& southSector = quadrant.m_sectors[sectorI][0];
				auto& northSector = northQuadrantIter->second.m_sectors[sectorI][QUADRANT_SIDE_LENGTH - 1];
				auto borderTile = FindCommonBorderTile(northSector, static_cast<int>(PathingDirection::SOUTH),
					southSector, static_cast<int>(PathingDirection::NORTH));
				if (borderTile)
				{
					northSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::SOUTH)] = *borderTile;
					southSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::NORTH)] = *borderTile;
				}
			}

			auto southQuadrantIter = m_spawnedQuadrants.find(coordinates + Pathing::neighborOffsets[static_cast<int>(PathingDirection::SOUTH)]);
			if (southQuadrantIter == m_spawnedQuadrants.end())
			{
				auto& southSector = quadrant.m_sectors[sectorI][QUADRANT_SIDE_LENGTH - 1];
				if (southSector.m_pathingBorderTileCandidates[static_cast<int>(PathingDirection::SOUTH)].size() > 0)
				{
					southSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::SOUTH)] =
						southSector.m_pathingBorderTileCandidates[static_cast<int>(PathingDirection::SOUTH)].begin()->second.front();
				}
			}
			else
			{
				auto& northSector = quadrant.m_sectors[sectorI][QUADRANT_SIDE_LENGTH - 1];
				auto& southSector = southQuadrantIter->second.m_sectors[sectorI][0];
				auto borderTile = FindCommonBorderTile(southSector, static_cast<int>(PathingDirection::NORTH),
					northSector, static_cast<int>(PathingDirection::SOUTH));
				if (borderTile)
				{
					northSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::SOUTH)] = *borderTile;
					southSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::NORTH)] = *borderTile;
				}
			}
		}

		std::vector<std::thread> sectorPathFindingThreads;
		auto northQuadrantIter = m_spawnedQuadrants.find(coordinates + Pathing::neighborOffsets[static_cast<int>(PathingDirection::NORTH)]);
		auto southQuadrantIter = m_spawnedQuadrants.find(coordinates + Pathing::neighborOffsets[static_cast<int>(PathingDirection::SOUTH)]);
		auto eastQuadrantIter = m_spawnedQuadrants.find(coordinates + Pathing::neighborOffsets[static_cast<int>(PathingDirection::EAST)]);
		auto westQuadrantIter = m_spawnedQuadrants.find(coordinates + Pathing::neighborOffsets[static_cast<int>(PathingDirection::WEST)]);
		for (int sectorI = 0; sectorI < QUADRANT_SIDE_LENGTH; ++sectorI)
		{
			for (int sectorJ = 0; sectorJ < QUADRANT_SIDE_LENGTH; ++sectorJ)
			{
				FillSectorPathing(
					quadrant.m_sectors[sectorI][sectorJ],
					sectorPathFindingThreads,
					quadrant,
					sectorI,
					sectorJ);				
			}

			if (northQuadrantIter != m_spawnedQuadrants.end())
			{
				FillSectorPathing(
					northQuadrantIter->second.m_sectors[sectorI][QUADRANT_SIDE_LENGTH - 1],
					sectorPathFindingThreads,
					northQuadrantIter->second,
					sectorI,
					QUADRANT_SIDE_LENGTH - 1);
			}

			if (southQuadrantIter != m_spawnedQuadrants.end())
			{
				FillSectorPathing(
					southQuadrantIter->second.m_sectors[sectorI][0],
					sectorPathFindingThreads,
					southQuadrantIter->second,
					sectorI,
					0);
			}

			if (eastQuadrantIter != m_spawnedQuadrants.end())
			{
				FillSectorPathing(
					eastQuadrantIter->second.m_sectors[0][sectorI],
					sectorPathFindingThreads,
					eastQuadrantIter->second,
					0,
					sectorI);
			}

			if (westQuadrantIter != m_spawnedQuadrants.end())
			{
				FillSectorPathing(
					westQuadrantIter->second.m_sectors[QUADRANT_SIDE_LENGTH - 1][sectorI],
					sectorPathFindingThreads,
					westQuadrantIter->second,
					QUADRANT_SIDE_LENGTH - 1,
					sectorI);
			}
		}
		for (auto&& thread : sectorPathFindingThreads)
		{
			thread.join();
		}

		FillQuadrantPathingEdges(quadrant);

		static std::mutex sectorSelectionMutex;
		{
			std::lock_guard<std::mutex> lock(sectorSelectionMutex);
			//Select border sectors for this quadrant, update border sectors for existing border sectors
			if (northQuadrantIter == m_spawnedQuadrants.end())
			{
				quadrant.m_pathingBorderSectors[static_cast<int>(PathingDirection::NORTH)] =
					quadrant.m_pathingBorderSectorCandidates[static_cast<int>(PathingDirection::NORTH)].begin()->second.front();
			}
			else
			{
				auto borderSector = FindCommonBorderSector(
					quadrant, static_cast<int>(PathingDirection::NORTH),
					northQuadrantIter->second, static_cast<int>(PathingDirection::SOUTH));
				if (borderSector)
				{
					quadrant.m_pathingBorderSectors[static_cast<int>(PathingDirection::NORTH)] = *borderSector;
					northQuadrantIter->second.m_pathingBorderSectors[static_cast<int>(PathingDirection::SOUTH)] = *borderSector;
				}
			}

			if (southQuadrantIter == m_spawnedQuadrants.end())
			{
				quadrant.m_pathingBorderSectors[static_cast<int>(PathingDirection::SOUTH)] =
					quadrant.m_pathingBorderSectorCandidates[static_cast<int>(PathingDirection::SOUTH)].begin()->second.front();
			}
			else
			{
				auto borderSector = FindCommonBorderSector(
					quadrant, static_cast<int>(PathingDirection::SOUTH),
					southQuadrantIter->second, static_cast<int>(PathingDirection::NORTH));
				if (borderSector)
				{
					quadrant.m_pathingBorderSectors[static_cast<int>(PathingDirection::SOUTH)] = *borderSector;
					southQuadrantIter->second.m_pathingBorderSectors[static_cast<int>(PathingDirection::NORTH)] = *borderSector;
				}
			}

			if (westQuadrantIter == m_spawnedQuadrants.end())
			{
				quadrant.m_pathingBorderSectors[static_cast<int>(PathingDirection::WEST)] =
					quadrant.m_pathingBorderSectorCandidates[static_cast<int>(PathingDirection::WEST)].begin()->second.front();
			}
			else
			{
				auto borderSector = FindCommonBorderSector(
					quadrant, static_cast<int>(PathingDirection::WEST),
					westQuadrantIter->second, static_cast<int>(PathingDirection::EAST));
				if (borderSector)
				{
					quadrant.m_pathingBorderSectors[static_cast<int>(PathingDirection::WEST)] = *borderSector;
					westQuadrantIter->second.m_pathingBorderSectors[static_cast<int>(PathingDirection::EAST)] = *borderSector;
				}
			}

			if (eastQuadrantIter == m_spawnedQuadrants.end())
			{
				quadrant.m_pathingBorderSectors[static_cast<int>(PathingDirection::EAST)] =
					quadrant.m_pathingBorderSectorCandidates[static_cast<int>(PathingDirection::WEST)].begin()->second.front();
			}
			else
			{
				auto borderSector = FindCommonBorderSector(
					quadrant, static_cast<int>(PathingDirection::EAST),
					eastQuadrantIter->second, static_cast<int>(PathingDirection::WEST));
				if (borderSector)
				{
					quadrant.m_pathingBorderSectors[static_cast<int>(PathingDirection::EAST)] = *borderSector;
					eastQuadrantIter->second.m_pathingBorderSectors[static_cast<int>(PathingDirection::WEST)] = *borderSector;
				}
			}

			// Now fill in pathing for each quadrant which was touched
			FillCrossQuadrantPaths(quadrant, coordinates);
			if (northQuadrantIter != m_spawnedQuadrants.end()) { FillCrossQuadrantPaths(northQuadrantIter->second, northQuadrantIter->first); }
			if (eastQuadrantIter != m_spawnedQuadrants.end()) { FillCrossQuadrantPaths(eastQuadrantIter->second, eastQuadrantIter->first); }
			if (southQuadrantIter != m_spawnedQuadrants.end()) { FillCrossQuadrantPaths(southQuadrantIter->second, southQuadrantIter->first); }
			if (westQuadrantIter != m_spawnedQuadrants.end()) { FillCrossQuadrantPaths(westQuadrantIter->second, westQuadrantIter->first); }
		}
		quadrant.m_spawningComplete = true;
	});
}

void WorldTile::FillSectorPathing(
	Sector& sector,
	std::vector<std::thread>& pathFindingThreads,
	Quadrant& quadrant,
	int sectorI,
	int sectorJ)
{
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
					case PathingDirection::SOUTH: return { index, TileConstants::SECTOR_SIDE_LENGTH - 1 };
					case PathingDirection::EAST: return { TileConstants::SECTOR_SIDE_LENGTH - 1, index };
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

void WorldTile::FillCrossQuadrantPaths(Quadrant& quadrant, const CoordinateVector2& coordinates)
{
	static std::mutex movementCostAccessMutex;
	std::lock_guard<std::mutex> lock(movementCostAccessMutex);

	auto& crossQuadrantPathCosts = m_quadrantMovementCosts[coordinates];
	auto& crossQuadrantPaths = m_quadrantPaths[coordinates];
	for (int sourceDirection = 0; sourceDirection < static_cast<int>(PathingDirection::_COUNT); ++sourceDirection)
	{
		auto sourceTile = GetQuadrantSideTile(quadrant, coordinates, sourceDirection);
		if (!sourceTile) continue;
		for (int targetDirection = 0; targetDirection < static_cast<int>(PathingDirection::_COUNT); ++targetDirection)
		{
			auto targetTile = GetQuadrantSideTile(quadrant, coordinates, targetDirection);
			if (!targetTile) continue;
			auto path = FindSingleQuadrantPath(
				quadrant,
				*sourceTile,
				*targetTile);
			if (path)
			{
				crossQuadrantPathCosts[sourceDirection][targetDirection] = path->m_totalPathCost;
				crossQuadrantPaths[sourceDirection][targetDirection] = path->m_path;
			}
			else
			{
				crossQuadrantPathCosts[sourceDirection][targetDirection].reset();
				crossQuadrantPaths[sourceDirection][targetDirection].reset();
			}
		}
	}
}

std::optional<TilePosition> WorldTile::GetQuadrantSideTile(
	const Quadrant& quadrant,
	const CoordinateVector2& quadrantCoords,
	int direction)
{
	using namespace TileConstants;
	if (!quadrant.m_pathingBorderSectors[direction]) return std::nullopt;
	switch (static_cast<PathingDirection>(direction))
	{
	case PathingDirection::NORTH:
		return TilePosition{
			quadrantCoords,
			{*quadrant.m_pathingBorderSectors[direction], 0},
			{*quadrant.m_sectors[*quadrant.m_pathingBorderSectors[direction]][0].m_pathingBorderTiles[direction], 0} };
	case PathingDirection::SOUTH:
		return TilePosition{
			quadrantCoords,
		{ *quadrant.m_pathingBorderSectors[direction], QUADRANT_SIDE_LENGTH - 1 },
		{ *quadrant.m_sectors[QUADRANT_SIDE_LENGTH - 1][*quadrant.m_pathingBorderSectors[direction]].m_pathingBorderTiles[direction],
			SECTOR_SIDE_LENGTH - 1 } };
	case PathingDirection::EAST:
		return TilePosition{
			quadrantCoords,
		{ QUADRANT_SIDE_LENGTH - 1, *quadrant.m_pathingBorderSectors[direction] },
		{ SECTOR_SIDE_LENGTH - 1,
			*quadrant.m_sectors[QUADRANT_SIDE_LENGTH - 1][*quadrant.m_pathingBorderSectors[direction]].m_pathingBorderTiles[direction] } };
	case PathingDirection::WEST:
		return TilePosition{
			quadrantCoords,
		{ 0, *quadrant.m_pathingBorderSectors[direction] },
		{ 0, *quadrant.m_sectors[0][*quadrant.m_pathingBorderSectors[direction]].m_pathingBorderTiles[direction]} };
	}
	return {};
}

std::optional<s64> WorldTile::FindCommonBorderTile(
	const Sector& sector1,
	int sector1Side,
	const Sector& sector2,
	int sector2Side)
{
	if (sector1.m_pathingBorderTileCandidates[sector1Side].size() > 0
		&& sector2.m_pathingBorderTileCandidates[sector2Side].size() > 0)
	{
		auto& s1Candidates = sector1.m_pathingBorderTileCandidates[sector1Side];
		auto& s2Candidates = sector2.m_pathingBorderTileCandidates[sector2Side];
		std::map<s64, std::map<int, std::vector<s64>>> sharedTiles; // outer key = total cost, inner key = total index
		for (auto&[s1Cost, s1Tiles] : s1Candidates)
		{
			for (auto&[s2Cost, s2Tiles] : s2Candidates)
			{
				for (int s1I = 0; s1I < s1Tiles.size(); ++s1I)
				{
					for (int s2I = 0; s2I < s2Tiles.size(); ++s2I)
					{
						if (s1Tiles[s1I] == s2Tiles[s2I])
						{
							sharedTiles[s1Cost + s2Cost][s1I + s2I].push_back(s1Tiles[s1I]);
						}
					}
				}
			}
		}
		if (sharedTiles.size())
		{
			return sharedTiles.begin()->second.begin()->second.front();
		}
	}
	return std::nullopt;
}


std::optional<s64> WorldTile::FindCommonBorderSector(
	const Quadrant& quadrant1,
	int quadrant1Side,
	const Quadrant& quadrant2,
	int quadrant2Side)
{
	if (quadrant1.m_pathingBorderSectorCandidates[quadrant1Side].size() > 0
		&& quadrant2.m_pathingBorderSectorCandidates[quadrant2Side].size() > 0)
	{
		auto& q1Candidates = quadrant1.m_pathingBorderSectorCandidates[quadrant1Side];
		auto& q2Candidates = quadrant2.m_pathingBorderSectorCandidates[quadrant2Side];
		std::map<s64, std::map<int, std::vector<s64>>> sharedSectors; // outer key = total cost, inner key = total index
		for (auto&[q1Cost, q1Sectors] : q1Candidates)
		{
			for (auto&[q2Cost, q2Sectors] : q2Candidates)
			{
				for (int s1I = 0; s1I < q1Sectors.size(); ++s1I)
				{
					for (int s2I = 0; s2I < q2Sectors.size(); ++s2I)
					{
						if (q1Sectors[s1I] == q2Sectors[s2I])
						{
							sharedSectors[q1Cost + q2Cost][s1I + s2I].push_back(q1Sectors[s1I]);
						}
					}
				}
			}
		}
		if (sharedSectors.size())
		{
			return sharedSectors.begin()->second.begin()->second.front();
		}
	}
	return std::nullopt;
}

void WorldTile::FillQuadrantPathingEdges(Quadrant& quadrant)
{
	using namespace TileConstants;
	constexpr int TILE_COUNT = QUADRANT_SIDE_LENGTH * SECTOR_SIDE_LENGTH;

	// Select a tile in the middle of the sector
	for (auto&& candidates : quadrant.m_pathingBorderSectorCandidates)
	{
		candidates.clear();
	}
	auto midpoint = []() constexpr->s64 {
		constexpr int TILE_COUNT = QUADRANT_SIDE_LENGTH * SECTOR_SIDE_LENGTH;
		if constexpr(TILE_COUNT % 2 == 0)
		{
			return TILE_COUNT / 2 - 1;
		}
		else
		{
			return TILE_COUNT / 2;
		}
	}();
	// Spiral out, until a moveable tile is found
	int xOffset = 0;
	int yOffset = 0;
	int nextXChange = 0;
	int nextYChange = -1;
	for (int i = 0; i < TILE_COUNT * TILE_COUNT; ++i, xOffset += nextXChange, yOffset += nextYChange)
	{
		auto coordinates = CoordinateVector2{ midpoint + xOffset, midpoint + yOffset };

		auto sectorCoordinates = coordinates / SECTOR_SIDE_LENGTH;
		auto tileCoordinates = coordinates % SECTOR_SIDE_LENGTH;

		struct FoundPath
		{
			FoundPath(int edgeI, s64 pathCost)
				: m_edgeI(edgeI)
				, m_pathCost(pathCost)
			{ }
			int m_edgeI;
			s64 m_pathCost;
		};
		std::map<PathingDirection, std::vector<FoundPath>> foundPaths;
		std::vector<std::thread> pathThreads;
		for (int edgeI = 0; edgeI < QUADRANT_SIDE_LENGTH; ++edgeI)
		{
			{
				auto&& targetSector = quadrant.m_sectors[edgeI][0];
				if (targetSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::NORTH)])
				{
					auto path = FindSingleQuadrantPath(
						quadrant,
						{ {}, sectorCoordinates, tileCoordinates },
						{ {},{ edgeI, 0 },{ *targetSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::NORTH)], 0 } });
					if (path)
					{ 
						foundPaths[PathingDirection::NORTH].push_back({ edgeI, path->m_totalPathCost });
					}
				}
			}
			{
				auto&& targetSector = quadrant.m_sectors[edgeI][QUADRANT_SIDE_LENGTH - 1];
				if (targetSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::SOUTH)])
				{
					auto path = FindSingleQuadrantPath(
						quadrant,
						{ {}, sectorCoordinates, tileCoordinates },
						{ {},{ edgeI, QUADRANT_SIDE_LENGTH - 1 },{ *targetSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::SOUTH)], SECTOR_SIDE_LENGTH - 1 } });
					if (path)
					{
						foundPaths[PathingDirection::SOUTH].push_back({ edgeI, path->m_totalPathCost });
					}
				}
			}
			{
				auto&& targetSector = quadrant.m_sectors[0][edgeI];
				if (targetSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::WEST)])
				{
					auto path = FindSingleQuadrantPath(
						quadrant,
						{ {}, sectorCoordinates, tileCoordinates },
						{ {},{ 0, edgeI },{ 0, *targetSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::WEST)]} });
					if (path)
					{
						foundPaths[PathingDirection::WEST].push_back({ edgeI, path->m_totalPathCost });
					}
				}
			}
			{
				auto&& targetSector = quadrant.m_sectors[QUADRANT_SIDE_LENGTH - 1][edgeI];
				if (targetSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::EAST)])
				{
					auto path = FindSingleQuadrantPath(
						quadrant,
						{ {}, sectorCoordinates, tileCoordinates },
						{ {},{ QUADRANT_SIDE_LENGTH - 1, edgeI},{ SECTOR_SIDE_LENGTH - 1, *targetSector.m_pathingBorderTiles[static_cast<int>(PathingDirection::EAST)] } });
					if (path)
					{
						foundPaths[PathingDirection::EAST].push_back({ edgeI, path->m_totalPathCost });
					}
				}
			}
		}

		if (foundPaths[PathingDirection::NORTH].size()
			&& foundPaths[PathingDirection::SOUTH].size()
			&& foundPaths[PathingDirection::EAST].size()
			&& foundPaths[PathingDirection::WEST].size())
		{
			for (auto&&[direction, paths] : foundPaths)
			{
				for (auto&& path : paths)
				{
					quadrant.m_pathingBorderSectorCandidates[static_cast<s64>(direction)][path.m_pathCost].push_back(path.m_edgeI);
				}
			}
			break;
		}

		if (xOffset == yOffset || (xOffset < 0 && (xOffset == -yOffset)) || (xOffset > 0 && xOffset == 1 - yOffset))
		{
			std::swap(nextXChange, nextYChange);
			nextXChange = -nextXChange;
		}
	}
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

template<class T>
T min(T&& a, T&& b)
{
	return a < b ? a : b;
}

WorldTile::WorldCoordinates WorldTile::WorldPositionToCoordinates(const CoordinateVector2& worldPos)
{
	using namespace TileConstants;
	auto offsetFromQuadrantOrigin = worldPos - CoordinateVector2(
		BASE_QUADRANT_ORIGIN_COORDINATE,
		BASE_QUADRANT_ORIGIN_COORDINATE);
	auto quadrantCoords = CoordinateVector2(
		min(0, sign(offsetFromQuadrantOrigin.m_x)) + sign(offsetFromQuadrantOrigin.m_x) * (abs(offsetFromQuadrantOrigin.m_x) / (QUADRANT_SIDE_LENGTH * SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH)),
		min(0, sign(offsetFromQuadrantOrigin.m_y)) + sign(offsetFromQuadrantOrigin.m_y) * (abs(offsetFromQuadrantOrigin.m_y) / (QUADRANT_SIDE_LENGTH * SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH)));
	auto sectorCoords = CoordinateVector2(
		min(0, sign(offsetFromQuadrantOrigin.m_x)) + sign(offsetFromQuadrantOrigin.m_x) * (abs(offsetFromQuadrantOrigin.m_x) % (QUADRANT_SIDE_LENGTH * SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH)) / (SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH),
		min(0, sign(offsetFromQuadrantOrigin.m_y)) + sign(offsetFromQuadrantOrigin.m_y) * (abs(offsetFromQuadrantOrigin.m_y) % (QUADRANT_SIDE_LENGTH * SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH)) / (SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH));
	auto tileCoords = CoordinateVector2(
		min(0, sign(offsetFromQuadrantOrigin.m_x)) + sign(offsetFromQuadrantOrigin.m_x) * (abs(offsetFromQuadrantOrigin.m_x) % (SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH)) / TILE_SIDE_LENGTH,
		min(0, sign(offsetFromQuadrantOrigin.m_y)) + sign(offsetFromQuadrantOrigin.m_y) * (abs(offsetFromQuadrantOrigin.m_y) % (SECTOR_SIDE_LENGTH * TILE_SIDE_LENGTH)) / TILE_SIDE_LENGTH);
	while (tileCoords.m_x < 0) tileCoords.m_x += SECTOR_SIDE_LENGTH;
	while (tileCoords.m_y < 0) tileCoords.m_y += SECTOR_SIDE_LENGTH;
	while (sectorCoords.m_x < 0) sectorCoords.m_x += QUADRANT_SIDE_LENGTH;
	while (sectorCoords.m_y < 0) sectorCoords.m_y += QUADRANT_SIDE_LENGTH;
	return { quadrantCoords, sectorCoords, tileCoords };
}

CoordinateVector2 WorldTile::CoordinatesToWorldPosition(const WorldCoordinates& worldCoords)
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

CoordinateVector2 WorldTile::CoordinatesToWorldOffset(const WorldCoordinates& worldOffset)
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

std::thread WorldTile::SpawnBetween(
	CoordinateVector2 origin,
	CoordinateVector2 target)
{
	return std::thread([=]() {
		// Base case: Spawn between a place and itself
		if (origin == target)
		{
			// Spawn just in case, will return immediately in most cases.
			SpawnQuadrant(target).join();
			return;
		}

		SpawnQuadrant(target).join();
		SpawnQuadrant(origin).join();

		auto mid = (target + origin) / 2;
		SpawnQuadrant(mid).join();

		if (mid == target || mid == origin)
		{
			// We're all filled in on this segment.
			return;
		}

		SpawnBetween(origin, mid).join();
		SpawnBetween(mid + CoordinateVector2(sign(target.m_x - origin.m_x), sign(target.m_y - origin.m_y)), target).join();
	});
}

// Precondition: touched is empty
void WorldTile::TouchConnectedCoordinates(
	const CoordinateVector2& origin,
	CoordinateFromOriginSet& untouched,
	CoordinateFromOriginSet& touched)
{
	if (m_spawnedQuadrants.find(origin) == m_spawnedQuadrants.end())
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

CoordinateVector2 WorldTile::FindNearestQuadrant(const SpawnedQuadrantMap& searchedQuadrants, const CoordinateVector2& quadrantCoords)
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

CoordinateVector2 WorldTile::FindNearestQuadrant(const CoordinateFromOriginSet& searchedQuadrants, const CoordinateVector2& quadrantCoords)
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

std::set<WorldTile::TileSide> WorldTile::GetAdjacents(const WorldCoordinates& coords)
{
	return {
		{ coords + WorldCoordinates{ { 0,0 },{ 0,0 },{ 1,0 } }, Direction::EAST },
	{ coords + WorldCoordinates{ { 0,0 },{ 0,0 },{ 0,1 } }, Direction::SOUTH },
	{ coords - WorldCoordinates{ { 0,0 },{ 0,0 },{ 1,0 } }, Direction::WEST },
	{ coords - WorldCoordinates{ { 0,0 },{ 0,0 },{ 0,1 } }, Direction::NORTH },
	};
}

void WorldTile::GrowTerritories()
{
	using namespace ECS_Core;
	// Get current time
	// Assume the first entity is the one that has a valid time
	auto timeEntities = m_managerRef.entitiesMatching<ECS_Core::Signatures::S_TimeTracker>();
	if (timeEntities.size() == 0)
	{
		return;
	}
	const auto& time = m_managerRef.getComponent<ECS_Core::Components::C_TimeTracker>(timeEntities.front());

	// Run through the buildings that are in progress, make sure they contain their own building placement
	m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_InProgressBuilding>(
		[&manager = m_managerRef, this](
		const ecs::EntityIndex& entity,
		const Components::C_BuildingDescription&,
		const Components::C_TilePosition& tilePos,
		const Components::C_BuildingConstruction&)
	{
		auto placementTileOpt = GetTile(tilePos.m_position);
		if (placementTileOpt)
		{
			auto& placementTile = **placementTileOpt;
			if (!placementTile.m_owningBuilding)
			{
				placementTile.m_owningBuilding = manager.getHandle(entity);
			}
		}
		return ecs::IterationBehavior::CONTINUE;
	});

	std::random_device rd;
	std::mt19937 g(rd());
	m_managerRef.forEntitiesMatching<Signatures::S_CompleteBuilding>(
		[&manager = m_managerRef, &time, &g, this](
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
					auto adjacentTileOpt = GetTile(adjacent.m_coords);
					if (adjacentTileOpt)
					{
						auto& adjacentTile = **adjacentTileOpt;
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
			}

			if (availableGrowthTiles.size())
			{
				auto selectedGrowthTiles = availableGrowthTiles.begin()->second;
				if (selectedGrowthTiles.size())
				{
					std::shuffle(selectedGrowthTiles.begin(), selectedGrowthTiles.end(), g);

					territory.m_nextGrowthTile = { 0.f, selectedGrowthTiles.front() };

					auto tile = GetTile(territory.m_nextGrowthTile->m_tile);
					if (tile)
					{
						(*tile)->m_owningBuilding = manager.getHandle(territoryEntity);
					}
				}
			}
		}

		// Now grow if we can
		if (territory.m_nextGrowthTile)
		{
			auto& tile = GetTile(territory.m_nextGrowthTile->m_tile);
			territory.m_nextGrowthTile->m_progress += (0.2 * time.m_frameDuration / sqrt(territory.m_ownedTiles.size()));
			if (territory.m_nextGrowthTile->m_progress >= 1)
			{
				territory.m_ownedTiles.insert(territory.m_nextGrowthTile->m_tile);
				territory.m_nextGrowthTile.reset();

				UpdateTerritoryProductionPotential(yieldPotential, territory);

				if (manager.hasComponent<ECS_Core::Components::C_SFMLDrawable>(territoryEntity))
				{
					auto& drawable = manager.getComponent<ECS_Core::Components::C_SFMLDrawable>(territoryEntity);
					// redraw the borders
					{
						// Remove previous borders
						drawable.m_drawables[ECS_Core::Components::DrawLayer::TERRAIN].erase(static_cast<u64>(DrawPriority::TERRITORY_BORDER));

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
						for (auto&& [edgePosition, direction] : edges)
						{
							auto positionOffset = CoordinatesToWorldOffset(edgePosition - buildingTilePos.m_position).cast<f64>();
							bool isVertical = (direction == Direction::EAST || direction == Direction::WEST);
							static const float BORDER_PIXEL_WIDTH = 0.25f;

							auto indicatorOffset = positionOffset + CartesianVector2<f64>{BORDER_PIXEL_WIDTH, BORDER_PIXEL_WIDTH};
							auto sideIndicator = std::make_shared<sf::CircleShape>(BORDER_PIXEL_WIDTH, 4);
							sideIndicator->setFillColor({});
							switch (direction)
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
							drawable.m_drawables[ECS_Core::Components::DrawLayer::TERRAIN][static_cast<u64>(DrawPriority::TERRITORY_BORDER)].push_back({ sideIndicator, indicatorOffset });

							// sf Line type is always 1 pixel wide, so use rectangle so we can control thickness
							auto line = std::make_shared<sf::RectangleShape>(
								isVertical
								? sf::Vector2f{ BORDER_PIXEL_WIDTH, 1.f * TileConstants::TILE_SIDE_LENGTH }
							: sf::Vector2f{ 1.f * TileConstants::TILE_SIDE_LENGTH,BORDER_PIXEL_WIDTH });

							switch (direction)
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
							drawable.m_drawables[ECS_Core::Components::DrawLayer::TERRAIN][static_cast<u64>(DrawPriority::TERRITORY_BORDER)].push_back({ line, positionOffset });
						}
					}
				}
			}
		}
		return ecs::IterationBehavior::CONTINUE;
	});
}

void WorldTile::UpdateTerritoryProductionPotential(ECS_Core::Components::C_TileProductionPotential & yieldPotential, const ECS_Core::Components::C_Territory & territory)
{
	// Update yield potential
	yieldPotential.m_availableYields.clear();
	for (auto&& tilePos : territory.m_ownedTiles)
	{
		auto ownedTileOpt = GetTile(tilePos);
		if (ownedTileOpt)
		{
			auto&& ownedTile = **ownedTileOpt;
			auto&& yield = yieldPotential.m_availableYields[static_cast<ECS_Core::Components::YieldType>(ownedTile.m_tileType)];
			++yield.m_workableTiles;
			yield.m_productionYield = {
				{ ECS_Core::Components::Yields::FOOD, 1 } };
			yield.m_productionYield[ownedTile.m_tileType] += 2;
		}
	}
}

std::optional<WorldTile::Tile*> WorldTile::GetTile(const TilePosition& buildingTilePos)
{
	if (!m_spawnedQuadrants.count(buildingTilePos.m_quadrantCoords))
	{
		FetchQuadrant(buildingTilePos.m_quadrantCoords);
		return std::nullopt;
	}
	if (!FetchQuadrant(buildingTilePos.m_quadrantCoords).m_spawningComplete) return std::nullopt;
	return &FetchQuadrant(buildingTilePos.m_quadrantCoords)
		.m_sectors[buildingTilePos.m_sectorCoords.m_x][buildingTilePos.m_sectorCoords.m_y]
		.m_tiles[buildingTilePos.m_coords.m_x][buildingTilePos.m_coords.m_y];
}

WorldTile::Quadrant& WorldTile::FetchQuadrant(const CoordinateVector2 & quadrantCoords)
{
	if (m_spawnedQuadrants.find(quadrantCoords) == m_spawnedQuadrants.end())
	{
		std::thread([=]() {
			// We're going to need to spawn world up to that point.
			// first: find the closest available world tile
			auto closest = FindNearestQuadrant(m_spawnedQuadrants, quadrantCoords);

			SpawnBetween(
				closest,
				quadrantCoords).join();

			// Find all quadrants which can't be reached by repeated cardinal direction movement from the origin
			CoordinateFromOriginSet touchedCoordinates, untouchedCoordinates;
			for (auto&& quadrant : m_spawnedQuadrants)
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
				SpawnQuadrant({ nearestDisconnected->m_x, nearestConnected.m_y }).join();
				SpawnQuadrant({ nearestConnected.m_x, nearestDisconnected->m_y }).join();

				touchedCoordinates.clear();
				TouchConnectedCoordinates(nearestConnected, untouchedCoordinates, touchedCoordinates);
			}
		}).detach();
	}
	return m_spawnedQuadrants[quadrantCoords];
}

void WorldTile::ReturnDeadBuildingTiles()
{
	using namespace ECS_Core;
	m_managerRef.forEntitiesMatching<Signatures::S_DestroyedBuilding>(
		[&manager = m_managerRef, this](
		const ecs::EntityIndex& deadBuildingEntity,
		const Components::C_BuildingDescription&,
		const Components::C_TilePosition& buildingPosition)
	{
		auto& buildingTile = FetchQuadrant(buildingPosition.m_position.m_quadrantCoords)
			.m_sectors[buildingPosition.m_position.m_sectorCoords.m_x][buildingPosition.m_position.m_sectorCoords.m_y]
			.m_tiles[buildingPosition.m_position.m_coords.m_x][buildingPosition.m_position.m_coords.m_y];
		buildingTile.m_owningBuilding.reset();

		if (manager.hasComponent<ECS_Core::Components::C_Territory>(deadBuildingEntity))
		{
			for (auto&& tile : manager.getComponent<ECS_Core::Components::C_Territory>(deadBuildingEntity).m_ownedTiles)
			{
				FetchQuadrant(tile.m_quadrantCoords)
					.m_sectors[tile.m_sectorCoords.m_x][tile.m_sectorCoords.m_y]
					.m_tiles[tile.m_coords.m_x][tile.m_coords.m_y].m_owningBuilding.reset();
			}
		}
		return ecs::IterationBehavior::CONTINUE;
	});
}

void WorldTile::ProcessSelectTile(
	const Action::LocalPlayer::SelectTile& select,
	const ecs::EntityIndex& governorEntity)
{
	bool unitFound = false;
	m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_MovingUnit>(
		[&unitFound, &select, &manager = m_managerRef, &governorEntity, this](
			const ecs::EntityIndex& entity,
			const ECS_Core::Components::C_TilePosition& position,
			const ECS_Core::Components::C_MovingUnit&,
			const ECS_Core::Components::C_Population&,
			const ECS_Core::Components::C_Vision&) {
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
						for (auto&&[birthMonth, population] : pop.m_populations)
						{
							if (population.m_class == PopulationClass::WORKERS)
								result += population.m_numMen;
						}
						return result;
					}),
						UIDataReader<C_Population, s32>([](const C_Population& pop) -> s32 {
						s32 result{ 0 };
						for (auto&&[birthMonth, population] : pop.m_populations)
						{
							if (population.m_class == PopulationClass::WORKERS)
								result += population.m_numWomen;
						}
						return result;
					}),
						UIDataReader<C_Population, s32>([](const C_Population& pop) -> s32 {
						s32 result{ 0 };
						for (auto&&[birthMonth, population] : pop.m_populations)
						{
							if (population.m_class == PopulationClass::CHILDREN)
								result += population.m_numMen + population.m_numWomen;
						}
						return result;
					}),
						UIDataReader<C_Population, s32>([](const C_Population& pop) -> s32 {
						s32 result{ 0 };
						for (auto&&[birthMonth, population] : pop.m_populations)
						{
							if (population.m_class == PopulationClass::ELDERS)
								result += population.m_numMen + population.m_numWomen;
						}
						return result;
					}),
						UIDataReader<C_Population, f64>([](const C_Population& pop) -> f64 {
						f64 totalHealth{ 0 };
						s32 totalPopulation{ 0 };
						for (auto&&[birthMonth, population] : pop.m_populations)
						{
							totalHealth += (population.m_mensHealth * population.m_numMen)
								+ (population.m_womensHealth * population.m_numWomen);
							totalPopulation += population.m_numMen + population.m_numWomen;
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
							return Action::LocalPlayer::PlanTargetedMotion(manager.getHandle(clickedEntity));
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
					for (auto&&[key, dataStr] : uiFrame.m_dataStrings)
					{
						dataStr.m_text->setFillColor({ 255,255,255 });
						dataStr.m_text->setOutlineColor({ 128,128,128 });
						dataStr.m_text->setFont(s_font);
						drawable.m_drawables[ECS_Core::Components::DrawLayer::MENU][255].push_back({ dataStr.m_text, dataStr.m_relativePosition });
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

	auto tileOpt = GetTile(select.m_position);
	if (tileOpt)
	{
		auto&& tile = **tileOpt;
		if (tile.m_owningBuilding)
		{
			using namespace ECS_Core::Components;
			if (m_managerRef.hasComponent<C_Population>(*tile.m_owningBuilding)
				&& !m_managerRef.hasComponent<C_UIFrame>(*tile.m_owningBuilding))
			{
				auto& uiFrame = m_managerRef.addComponent<C_UIFrame>(*tile.m_owningBuilding);
				uiFrame.m_frame = DefineUIFrame("Building",
					UIDataReader<C_Population, s32>([](const C_Population& pop) -> s32 {
					s32 result{ 0 };
					for (auto&&[birthMonth, population] : pop.m_populations)
					{
						if (population.m_class == PopulationClass::WORKERS)
							result += population.m_numMen;
					}
					return result;
				}),
					UIDataReader<C_Population, s32>([](const C_Population& pop) -> s32 {
					s32 result{ 0 };
					for (auto&&[birthMonth, population] : pop.m_populations)
					{
						if (population.m_class == PopulationClass::WORKERS)
							result += population.m_numWomen;
					}
					return result;
				}),
					UIDataReader<C_Population, s32>([](const C_Population& pop) -> s32 {
					s32 result{ 0 };
					for (auto&&[birthMonth, population] : pop.m_populations)
					{
						if (population.m_class == PopulationClass::CHILDREN)
							result += population.m_numMen + population.m_numWomen;
					}
					return result;
				}),
					UIDataReader<C_Population, s32>([](const C_Population& pop) -> s32 {
					s32 result{ 0 };
					for (auto&&[birthMonth, population] : pop.m_populations)
					{
						if (population.m_class == PopulationClass::ELDERS)
							result += population.m_numMen + population.m_numWomen;
					}
					return result;
				}),
					UIDataReader<C_Population, f64>([](const C_Population& pop) -> f64 {
					f64 totalHealth{ 0 };
					s32 totalPopulation{ 0 };
					for (auto&&[birthMonth, population] : pop.m_populations)
					{
						totalHealth += (population.m_mensHealth * population.m_numMen)
							+ (population.m_womensHealth * population.m_numWomen);
						totalPopulation += population.m_numMen + population.m_numWomen;
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
				newBuildingButton.m_onClick = [&manager = m_managerRef](const ecs::EntityIndex& /*clicker*/, const ecs::EntityIndex& clickedEntity)
				{
					Action::CreateBuildingUnit create;
					create.m_movementSpeed = 20;
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
				newCaravanButton.m_onClick = [&manager = m_managerRef](const ecs::EntityIndex& /*clicker*/, const ecs::EntityIndex& clickedEntity)
				{
					return Action::LocalPlayer::PlanCaravan(manager.getHandle(clickedEntity));
				};
				uiFrame.m_buttons.push_back(newCaravanButton);;

				ECS_Core::Components::Button newScoutButton;
				newScoutButton.m_size = { 30, 30 };
				newScoutButton.m_topLeftCorner = { 30, uiFrame.m_size.m_y - 30 };
				newScoutButton.m_onClick = [&manager = m_managerRef](const ecs::EntityIndex& /*clicker*/, const ecs::EntityIndex& clickedEntity)
				{
					return Action::LocalPlayer::PlanDirectionScout(manager.getHandle(clickedEntity));
				};
				uiFrame.m_buttons.push_back(newScoutButton);

				if (!m_managerRef.hasComponent<ECS_Core::Components::C_SFMLDrawable>(*tile.m_owningBuilding))
				{
					m_managerRef.addComponent<ECS_Core::Components::C_SFMLDrawable>(*tile.m_owningBuilding);
				}
				auto& drawable = m_managerRef.getComponent<ECS_Core::Components::C_SFMLDrawable>(*tile.m_owningBuilding);
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

				auto scoutGraphic = std::make_shared<sf::RectangleShape>(sf::Vector2f(30, 30));
				scoutGraphic->setFillColor({ 100, 200, 90 });
				drawable.m_drawables[ECS_Core::Components::DrawLayer::MENU][1].push_back({ scoutGraphic, newScoutButton.m_topLeftCorner });

				for (auto&&[key, dataStr] : uiFrame.m_dataStrings)
				{
					dataStr.m_text->setFillColor({ 255,255,255 });
					dataStr.m_text->setOutlineColor({ 128,128,128 });
					dataStr.m_text->setFont(s_font);
					drawable.m_drawables[ECS_Core::Components::DrawLayer::MENU][255].push_back({ dataStr.m_text, dataStr.m_relativePosition });
				}
			}
		}
	}
}

std::optional<ECS_Core::Components::MoveToPoint> WorldTile::GetPath(
	const TilePosition& sourcePosition,
	const TilePosition& targetPosition)
{
	// Make sure you can get from source tile to target tile
	// Are they in the same quadrant?
	if (sourcePosition.m_quadrantCoords != targetPosition.m_quadrantCoords)
	{
		auto& targetQuadrant = FetchQuadrant(targetPosition.m_quadrantCoords);
		auto& sourceQuadrant = FetchQuadrant(sourcePosition.m_quadrantCoords);

		return FindMultiQuadrantPath(sourceQuadrant, sourcePosition, targetQuadrant, targetPosition);
	}
	// Sweet, are they in the same sector?
	else if (sourcePosition.m_sectorCoords != targetPosition.m_sectorCoords)
	{
		return FindSingleQuadrantPath(FetchQuadrant(targetPosition.m_quadrantCoords), sourcePosition, targetPosition);
	}
	// Excellent. Cheap pathing
	else
	{
		auto& sector = FetchQuadrant(targetPosition.m_quadrantCoords)
			.m_sectors[sourcePosition.m_sectorCoords.m_x][sourcePosition.m_sectorCoords.m_y];
		return FindSingleSectorPath(sector.m_tileMovementCosts, sourcePosition, targetPosition);
	}
	return std::nullopt;
}

const std::optional<ECS_Core::Components::MoveToPoint> WorldTile::FindMultiQuadrantPath(
	const WorldTile::Quadrant& sourceQuadrant,
	const TilePosition& sourcePosition,
	const WorldTile::Quadrant& targetQuadrant,
	const TilePosition& targetPosition)
{
	// Get shortest path between quadrants
	auto quadrantPathCostCopyPtr = std::make_unique<decltype(m_quadrantMovementCosts)>(m_quadrantMovementCosts);
	auto& quadrantPathCostCopy = *quadrantPathCostCopyPtr;
	auto quadrantPathCopyPtr = std::make_unique<decltype(m_quadrantPaths)>(m_quadrantPaths);
	auto& quadrantPathCopy = *quadrantPathCopyPtr;
	// Fill in the cost from current point to each side
	for (int direction = static_cast<int>(PathingDirection::NORTH); direction < static_cast<int>(PathingDirection::_COUNT); ++direction)
	{
		auto startingExitTile = GetQuadrantSideTile(sourceQuadrant, sourcePosition.m_quadrantCoords, direction);
		if (startingExitTile)
		{
			auto startingPath =
				FindSingleQuadrantPath(sourceQuadrant, sourcePosition, *startingExitTile);
			if (startingPath)
			{
				quadrantPathCostCopy[sourcePosition.m_quadrantCoords]
					[static_cast<int>(PathingDirection::_COUNT)][direction]
					= startingPath->m_totalPathCost;
				quadrantPathCopy[sourcePosition.m_quadrantCoords]
					[static_cast<int>(PathingDirection::_COUNT)][direction]
					= startingPath->m_path;
			}
		}
		auto endingEntranceTile = GetQuadrantSideTile(targetQuadrant, targetPosition.m_quadrantCoords, direction);
		if (endingEntranceTile)
		{
			auto endingPath =
				FindSingleQuadrantPath(targetQuadrant, *endingEntranceTile, targetPosition);
			if (endingPath)
			{
				quadrantPathCostCopy[targetPosition.m_quadrantCoords]
					[direction][static_cast<int>(PathingDirection::_COUNT)]
					= endingPath->m_totalPathCost;
				quadrantPathCopy[targetPosition.m_quadrantCoords]
					[direction][static_cast<int>(PathingDirection::_COUNT)]
					= endingPath->m_path;
			}
		}
	}

	auto quadrantPath = Pathing::GetPath(
		quadrantPathCostCopy,
		sourcePosition.m_quadrantCoords,
		targetPosition.m_quadrantCoords);

	if (quadrantPath)
	{
		if (quadrantPath->m_path.size() <= 3)
		{
			// We run a risk of the path being obviously inefficient because of our
			// multi-level pathing
			// Glue all the quadrants in the path together and get a path with the same algorithm as we
			// use for single-quadrant paths.
			// This avoids the inefficient appearance for local pathing which may cross a border.

			auto& firstQuadrant = quadrantPath->m_path.front();
			auto& lastQuadrant = quadrantPath->m_path.back();
			std::vector<CoordinateVector2> involvedQuadrants;

			if (firstQuadrant.m_node.m_x == lastQuadrant.m_node.m_x)
			{
				// A bit of a waste of space if there are only 2
				// Shame
				auto sectorCrossingPathCostsPtr = std::make_unique<
					Quadrant::PathCostArray<TileConstants::QUADRANT_SIDE_LENGTH, TileConstants::QUADRANT_SIDE_LENGTH * 3>>();
				auto& sectorCrossingPathCosts = *sectorCrossingPathCostsPtr;

				auto sectorCrossingPathsPtr = std::make_unique<
					Quadrant::PathArray<TileConstants::QUADRANT_SIDE_LENGTH, TileConstants::QUADRANT_SIDE_LENGTH * 3>>();
				auto& sectorCrossingPaths = *sectorCrossingPathsPtr;

				auto tileMovementsPtr = std::make_unique<Sector::MultiSectorMovementArray<
					TileConstants::QUADRANT_SIDE_LENGTH,
					TileConstants::QUADRANT_SIDE_LENGTH * 3,
					TileConstants::SECTOR_SIDE_LENGTH,
					TileConstants::SECTOR_SIDE_LENGTH>>();
				auto& tileMovements = *tileMovementsPtr;

				auto leastY = min(firstQuadrant.m_node.m_y, lastQuadrant.m_node.m_y);
				auto greatestY = max(firstQuadrant.m_node.m_y, lastQuadrant.m_node.m_y);
				for (auto y = leastY; y <= greatestY; ++y)
				{
					auto& quadrant = FetchQuadrant({ firstQuadrant.m_node.m_x, y });
					for (int sectorX = 0; sectorX < TileConstants::QUADRANT_SIDE_LENGTH; ++sectorX)
					{
						for (int sectorY = 0; sectorY < TileConstants::QUADRANT_SIDE_LENGTH; ++sectorY)
						{
							auto yIndex = sectorY + TileConstants::QUADRANT_SIDE_LENGTH * (y - leastY);
							sectorCrossingPathCosts[sectorX][yIndex] = quadrant.m_sectorCrossingPathCosts[sectorX][sectorY];
							sectorCrossingPaths[sectorX][yIndex] = quadrant.m_sectorCrossingPaths[sectorX][sectorY];
							tileMovements[sectorX][yIndex] = quadrant.m_sectors[sectorX][sectorY].m_tileMovementCosts;
						}
					}
				}
				auto localOffset = TilePosition({ sourcePosition.m_quadrantCoords.m_x, leastY }, {}, {});
				auto localSourcePosition = sourcePosition - localOffset;
				auto localTargetPosition = targetPosition - localOffset;
				for (; localSourcePosition.m_quadrantCoords.m_y > 0;
					--localSourcePosition.m_quadrantCoords.m_y, localSourcePosition.m_sectorCoords.m_y += TileConstants::QUADRANT_SIDE_LENGTH);
				for (; localTargetPosition.m_quadrantCoords.m_y > 0;
					--localTargetPosition.m_quadrantCoords.m_y, localTargetPosition.m_sectorCoords.m_y += TileConstants::QUADRANT_SIDE_LENGTH);

				auto& startingSector = FetchQuadrant(sourcePosition.m_quadrantCoords).m_sectors[sourcePosition.m_sectorCoords.m_x][sourcePosition.m_sectorCoords.m_y];
				auto& endingSector = FetchQuadrant(targetPosition.m_quadrantCoords).m_sectors[targetPosition.m_sectorCoords.m_x][targetPosition.m_sectorCoords.m_y];
				PrepareStartAndEndSectorPaths(
					startingSector, sourcePosition.m_coords, localSourcePosition.m_sectorCoords,
					sectorCrossingPathCosts, sectorCrossingPaths,
					endingSector, targetPosition.m_coords, localTargetPosition.m_sectorCoords);

				auto localPath = FindSingleQuadrantPath(sectorCrossingPathCosts, sectorCrossingPaths, tileMovements, localSourcePosition, localTargetPosition);
				if (localPath)
				{
					ECS_Core::Components::MoveToPoint overallPath;
					// Translate back from local to global
					for (auto&& coordinate : *localPath)
					{
						// operator+ handles conversion back into standard size quadrants
						auto globalCoordinate = coordinate + localOffset;
						auto movementCost =
							*FetchQuadrant(globalCoordinate.m_quadrantCoords).
							m_sectors[globalCoordinate.m_sectorCoords.m_x][globalCoordinate.m_sectorCoords.m_y].
							m_tileMovementCosts[globalCoordinate.m_coords.m_x][globalCoordinate.m_coords.m_y];
						overallPath.m_path.push_back({
							globalCoordinate,
							movementCost });
						overallPath.m_totalPathCost += movementCost;
					}
					overallPath.m_targetPosition = targetPosition;
					return overallPath;
				}
			}
			else if (firstQuadrant.m_node.m_y == lastQuadrant.m_node.m_y)
			{
				auto sectorCrossingPathCostsPtr = std::make_unique<
					Quadrant::PathCostArray<TileConstants::QUADRANT_SIDE_LENGTH * 3, TileConstants::QUADRANT_SIDE_LENGTH>>();
				auto& sectorCrossingPathCosts = *sectorCrossingPathCostsPtr;

				auto sectorCrossingPathsPtr = std::make_unique<
					Quadrant::PathArray<TileConstants::QUADRANT_SIDE_LENGTH * 3, TileConstants::QUADRANT_SIDE_LENGTH>>();
				auto& sectorCrossingPaths = *sectorCrossingPathsPtr;

				auto tileMovementsPtr = std::make_unique<Sector::MultiSectorMovementArray<
					TileConstants::QUADRANT_SIDE_LENGTH * 3,
					TileConstants::QUADRANT_SIDE_LENGTH,
					TileConstants::SECTOR_SIDE_LENGTH,
					TileConstants::SECTOR_SIDE_LENGTH>>();
				auto& tileMovements = *tileMovementsPtr;

				auto leastX = min(firstQuadrant.m_node.m_x, lastQuadrant.m_node.m_x);
				auto greatestX = max(firstQuadrant.m_node.m_x, lastQuadrant.m_node.m_x);
				for (auto x = leastX; x <= greatestX; ++x)
				{
					auto& quadrant = FetchQuadrant({ x, firstQuadrant.m_node.m_y });
					for (int sectorX = 0; sectorX < TileConstants::QUADRANT_SIDE_LENGTH; ++sectorX)
					{
						for (int sectorY = 0; sectorY < TileConstants::QUADRANT_SIDE_LENGTH; ++sectorY)
						{
							auto xIndex = sectorX + TileConstants::QUADRANT_SIDE_LENGTH * (x - leastX);
							sectorCrossingPathCosts[xIndex][sectorY] = quadrant.m_sectorCrossingPathCosts[sectorX][sectorY];
							sectorCrossingPaths[xIndex][sectorY] = quadrant.m_sectorCrossingPaths[sectorX][sectorY];
							tileMovements[xIndex][sectorY] = quadrant.m_sectors[sectorX][sectorY].m_tileMovementCosts;
						}
					}
				}
				auto localOffset = TilePosition({ leastX, sourcePosition.m_quadrantCoords.m_y }, {}, {});
				auto localSourcePosition = sourcePosition - localOffset;
				auto localTargetPosition = targetPosition - localOffset;
				for (; localSourcePosition.m_quadrantCoords.m_x > 0;
					--localSourcePosition.m_quadrantCoords.m_x, localSourcePosition.m_sectorCoords.m_x += TileConstants::QUADRANT_SIDE_LENGTH);
				for (; localTargetPosition.m_quadrantCoords.m_x > 0;
					--localTargetPosition.m_quadrantCoords.m_x, localTargetPosition.m_sectorCoords.m_x += TileConstants::QUADRANT_SIDE_LENGTH);

				auto& startingSector = FetchQuadrant(sourcePosition.m_quadrantCoords).m_sectors[sourcePosition.m_sectorCoords.m_x][sourcePosition.m_sectorCoords.m_y];
				auto& endingSector = FetchQuadrant(targetPosition.m_quadrantCoords).m_sectors[targetPosition.m_sectorCoords.m_x][targetPosition.m_sectorCoords.m_y];
				PrepareStartAndEndSectorPaths(
					startingSector, sourcePosition.m_coords, localSourcePosition.m_sectorCoords,
					sectorCrossingPathCosts, sectorCrossingPaths,
					endingSector, targetPosition.m_coords, localTargetPosition.m_sectorCoords);

				auto localPath = FindSingleQuadrantPath(sectorCrossingPathCosts, sectorCrossingPaths, tileMovements, localSourcePosition, localTargetPosition);
				if (localPath)
				{
					ECS_Core::Components::MoveToPoint overallPath;
					// Translate back from local to global
					for (auto&& coordinate : *localPath)
					{
						// operator+ handles conversion back into standard size quadrants
						auto globalCoordinate = coordinate + localOffset;
						auto movementCost =
							*FetchQuadrant(globalCoordinate.m_quadrantCoords).
							m_sectors[globalCoordinate.m_sectorCoords.m_x][globalCoordinate.m_sectorCoords.m_y].
							m_tileMovementCosts[globalCoordinate.m_coords.m_x][globalCoordinate.m_coords.m_y];
						overallPath.m_path.push_back({
							globalCoordinate,
							movementCost });
						overallPath.m_totalPathCost += movementCost;
					}
					overallPath.m_targetPosition = targetPosition;
					return overallPath;
				}
			}
			else
			{
				auto sectorCrossingPathCostsPtr = std::make_unique<
					Quadrant::PathCostArray<TileConstants::QUADRANT_SIDE_LENGTH * 2, TileConstants::QUADRANT_SIDE_LENGTH * 2>>();
				auto& sectorCrossingPathCosts = *sectorCrossingPathCostsPtr;

				auto sectorCrossingPathsPtr = std::make_unique<
					Quadrant::PathArray<TileConstants::QUADRANT_SIDE_LENGTH * 2, TileConstants::QUADRANT_SIDE_LENGTH * 2>>();
				auto& sectorCrossingPaths = *sectorCrossingPathsPtr;

				auto tileMovementsPtr = std::make_unique<Sector::MultiSectorMovementArray<
					TileConstants::QUADRANT_SIDE_LENGTH * 2,
					TileConstants::QUADRANT_SIDE_LENGTH * 2,
					TileConstants::SECTOR_SIDE_LENGTH,
					TileConstants::SECTOR_SIDE_LENGTH>>();
				auto& tileMovements = *tileMovementsPtr;

				// They're diagonally adjacent. Include the cross-adjacent quadrants
				auto lessX = min(firstQuadrant.m_node.m_x, lastQuadrant.m_node.m_x);
				auto lessY = min(firstQuadrant.m_node.m_y, lastQuadrant.m_node.m_y);
				auto greaterX = max(firstQuadrant.m_node.m_x, lastQuadrant.m_node.m_x);
				auto greaterY = max(firstQuadrant.m_node.m_y, lastQuadrant.m_node.m_y);

				for (auto x = lessX; x <= greaterX; ++x)
				{
					auto xIndex = (x - lessX) * TileConstants::QUADRANT_SIDE_LENGTH;
					for (auto y = lessY; y <= greaterY; ++y)
					{
						auto yIndex = (y - lessY) * TileConstants::QUADRANT_SIDE_LENGTH;
						auto& quadrant = FetchQuadrant({ x, y });
						for (int sectorX = 0; sectorX < TileConstants::QUADRANT_SIDE_LENGTH; ++sectorX)
						{
							for (int sectorY = 0; sectorY < TileConstants::QUADRANT_SIDE_LENGTH; ++sectorY)
							{
								sectorCrossingPathCosts[xIndex + sectorX][yIndex + sectorY] = quadrant.m_sectorCrossingPathCosts[sectorX][sectorY];
								sectorCrossingPaths[xIndex + sectorX][yIndex + sectorY] = quadrant.m_sectorCrossingPaths[sectorX][sectorY];
								tileMovements[xIndex + sectorX][yIndex + sectorY] = quadrant.m_sectors[sectorX][sectorY].m_tileMovementCosts;
							}
						}
					}
				}
				auto localOffset = TilePosition({ lessX, lessY }, {}, {});
				auto localSourcePosition = sourcePosition - localOffset;
				auto localTargetPosition = targetPosition - localOffset;
				for (; localSourcePosition.m_quadrantCoords.m_x > 0;
					--localSourcePosition.m_quadrantCoords.m_x, localSourcePosition.m_sectorCoords.m_x += TileConstants::QUADRANT_SIDE_LENGTH);
				for (; localTargetPosition.m_quadrantCoords.m_x > 0;
					--localTargetPosition.m_quadrantCoords.m_x, localTargetPosition.m_sectorCoords.m_x += TileConstants::QUADRANT_SIDE_LENGTH);
				for (; localSourcePosition.m_quadrantCoords.m_y > 0;
					--localSourcePosition.m_quadrantCoords.m_y, localSourcePosition.m_sectorCoords.m_y += TileConstants::QUADRANT_SIDE_LENGTH);
				for (; localTargetPosition.m_quadrantCoords.m_y > 0;
					--localTargetPosition.m_quadrantCoords.m_y, localTargetPosition.m_sectorCoords.m_y += TileConstants::QUADRANT_SIDE_LENGTH);

				auto& startingSector = FetchQuadrant(sourcePosition.m_quadrantCoords).m_sectors[sourcePosition.m_sectorCoords.m_x][sourcePosition.m_sectorCoords.m_y];
				auto& endingSector = FetchQuadrant(targetPosition.m_quadrantCoords).m_sectors[targetPosition.m_sectorCoords.m_x][targetPosition.m_sectorCoords.m_y];
				PrepareStartAndEndSectorPaths(
					startingSector, sourcePosition.m_coords, localSourcePosition.m_sectorCoords,
					sectorCrossingPathCosts, sectorCrossingPaths,
					endingSector, targetPosition.m_coords, localTargetPosition.m_sectorCoords);

				auto localPath = FindSingleQuadrantPath(sectorCrossingPathCosts, sectorCrossingPaths, tileMovements, localSourcePosition, localTargetPosition);
				if (localPath)
				{
					ECS_Core::Components::MoveToPoint overallPath;
					// Translate back from local to global
					for (auto&& coordinate : *localPath)
					{
						// operator+ handles conversion back into standard size quadrants
						auto globalCoordinate = coordinate + localOffset;
						auto movementCost =
							*FetchQuadrant(globalCoordinate.m_quadrantCoords).
							m_sectors[globalCoordinate.m_sectorCoords.m_x][globalCoordinate.m_sectorCoords.m_y].
							m_tileMovementCosts[globalCoordinate.m_coords.m_x][globalCoordinate.m_coords.m_y];
						overallPath.m_path.push_back({
							globalCoordinate,
							movementCost });
						overallPath.m_totalPathCost += movementCost;
					}
					overallPath.m_targetPosition = targetPosition;
					return overallPath;
				}
			}
		}
		else
		{
			// Glue together: starting tile to exit tile of first sector
			// Rest of the sectors
			// Final sector entry tile to target tile.

			const auto& startingMacroPath = quadrantPath->m_path.front();
			const auto& endingMacroPath = quadrantPath->m_path.back();
			ECS_Core::Components::MoveToPoint overallPath;
			for (auto&& tile : *quadrantPathCopy[sourcePosition.m_quadrantCoords]
				[static_cast<int>(PathingDirection::_COUNT)][static_cast<int>(startingMacroPath.m_exitDirection)])
			{
				overallPath.m_path.push_back(tile);
			}
			overallPath.m_totalPathCost += *quadrantPathCostCopy[sourcePosition.m_quadrantCoords]
				[static_cast<int>(PathingDirection::_COUNT)][static_cast<int>(startingMacroPath.m_exitDirection)];

			for (int i = 1; i < quadrantPath->m_path.size() - 1; ++i)
			{
				auto& path = quadrantPath->m_path[i];
				auto& crossingPath = quadrantPathCopy
					[path.m_node]
				[static_cast<int>(path.m_entryDirection)]
				[static_cast<int>(path.m_exitDirection)];
				for (auto&& tile : *crossingPath)
				{
					overallPath.m_path.push_back(tile);
				}
				overallPath.m_totalPathCost += *quadrantPathCostCopy
					[path.m_node]
				[static_cast<int>(path.m_entryDirection)]
				[static_cast<int>(path.m_exitDirection)];
			}

			for (auto&& tile : *quadrantPathCopy[targetPosition.m_quadrantCoords]
				[static_cast<int>(endingMacroPath.m_entryDirection)][static_cast<int>(PathingDirection::_COUNT)])
			{
				overallPath.m_path.push_back(tile);
			}
			overallPath.m_targetPosition = targetPosition;
			overallPath.m_totalPathCost += *quadrantPathCostCopy[targetPosition.m_quadrantCoords]
				[static_cast<int>(endingMacroPath.m_entryDirection)][static_cast<int>(PathingDirection::_COUNT)];
			return overallPath;
		}
	}
	return std::nullopt;
}

template <int X, int Y>
std::optional<ECS_Core::Components::MoveToPoint> WorldTile::FindSingleSectorPath(
	const Sector::MovementCostArray<X, Y>& movementCosts,
	const TilePosition& sourcePosition,
	const TilePosition& targetPosition)
{
	auto totalPath = Pathing::GetPath(movementCosts,
		sourcePosition.m_coords,
		targetPosition.m_coords);
	if (totalPath)
	{
		ECS_Core::Components::MoveToPoint path;
		for (auto&& tile : totalPath->m_path)
		{
			path.m_path.push_back({ { sourcePosition.m_quadrantCoords, sourcePosition.m_sectorCoords, tile },
				*movementCosts[tile.m_x][tile.m_y] });
		}
		path.m_targetPosition = targetPosition;
		path.m_totalPathCost = totalPath->m_totalPathCost;
		return path;
	}
	return std::nullopt;
}

std::optional<CoordinateVector2> WorldTile::GetSectorBorderTile(const Sector& sector, PathingDirection direction) const
{
	auto dInt = static_cast<int>(direction);
	if (!sector.m_pathingBorderTiles[dInt]) return std::nullopt;
	switch (direction)
	{
	case PathingDirection::NORTH: return CoordinateVector2{ *sector.m_pathingBorderTiles[dInt], 0 };
	case PathingDirection::SOUTH: return CoordinateVector2{ *sector.m_pathingBorderTiles[dInt], TileConstants::SECTOR_SIDE_LENGTH - 1 };
	case PathingDirection::EAST: return CoordinateVector2{ TileConstants::SECTOR_SIDE_LENGTH - 1 , *sector.m_pathingBorderTiles[dInt] };
	case PathingDirection::WEST: return CoordinateVector2{ 0, *sector.m_pathingBorderTiles[dInt] };
	}
	return CoordinateVector2{ 0,0 };
}

template <int X, int Y>
void WorldTile::PrepareStartAndEndSectorPaths(
	const WorldTile::Sector& startingSector,
	const CoordinateVector2& sourceTilePosition,
	const CoordinateVector2& sourceSectorCoords,
	Quadrant::PathCostArray<X, Y>& pathCosts,
	Quadrant::PathArray<X, Y>& paths,
	const WorldTile::Sector& endingSector,
	const CoordinateVector2& targetTilePosition,
	const CoordinateVector2& targetSectorCoords)
{
	for (int direction = static_cast<int>(PathingDirection::NORTH); direction < static_cast<int>(PathingDirection::_COUNT); ++direction)
	{
		auto borderTile = GetSectorBorderTile(startingSector, static_cast<PathingDirection>(direction));
		if (borderTile)
		{
			auto startingPath = Pathing::GetPath(
				startingSector.m_tileMovementCosts,
				sourceTilePosition,
				*borderTile);
			if (startingPath)
			{
				pathCosts[sourceSectorCoords.m_x][sourceSectorCoords.m_y]
					[static_cast<int>(PathingDirection::_COUNT)][direction]
					= startingPath->m_totalPathCost;
				paths[sourceSectorCoords.m_x][sourceSectorCoords.m_y]
					[static_cast<int>(PathingDirection::_COUNT)][direction]
					= startingPath->m_path;
			}
		}
		borderTile = GetSectorBorderTile(endingSector, static_cast<PathingDirection>(direction));
		if (borderTile)
		{
			auto endingPath = Pathing::GetPath(
				endingSector.m_tileMovementCosts,
				*borderTile,
				targetTilePosition);
			if (endingPath)
			{
				pathCosts[targetSectorCoords.m_x][targetSectorCoords.m_y]
					[direction][static_cast<int>(PathingDirection::_COUNT)]
					= endingPath->m_totalPathCost;
				paths[targetSectorCoords.m_x][targetSectorCoords.m_y]
					[direction][static_cast<int>(PathingDirection::_COUNT)]
					= endingPath->m_path;
			}
		}
	}
}

std::optional<ECS_Core::Components::MoveToPoint> WorldTile::FindSingleQuadrantPath(
	const WorldTile::Quadrant& quadrant,
	const TilePosition& sourcePosition,
	const TilePosition& targetPosition)
{
	auto sectorPathCostCopyPtr = std::make_unique<decltype(quadrant.m_sectorCrossingPathCosts)>(quadrant.m_sectorCrossingPathCosts);
	auto sectorPathCopyPtr = std::make_unique<decltype(quadrant.m_sectorCrossingPaths)>(quadrant.m_sectorCrossingPaths);
	auto sectorPathCostCopy = *sectorPathCostCopyPtr;
	auto sectorPathCopy = *sectorPathCopyPtr;

	auto movementCostsCopyPtr = std::make_unique<Sector::MultiSectorMovementArray<
		TileConstants::QUADRANT_SIDE_LENGTH,
		TileConstants::QUADRANT_SIDE_LENGTH,
		TileConstants::SECTOR_SIDE_LENGTH,
		TileConstants::SECTOR_SIDE_LENGTH>>();
	auto& movementCostsCopy = *movementCostsCopyPtr;
	for (int secX = 0; secX < TileConstants::QUADRANT_SIDE_LENGTH; ++secX)
	{
		for (int secY = 0; secY < TileConstants::QUADRANT_SIDE_LENGTH; ++secY)
		{
			movementCostsCopy[secX][secY] = quadrant.m_sectors[secX][secY].m_tileMovementCosts;
		}
	}

	// Fill in the cost from current point to each side
	PrepareStartAndEndSectorPaths(
		quadrant.m_sectors[sourcePosition.m_sectorCoords.m_x][sourcePosition.m_sectorCoords.m_y],
		sourcePosition.m_coords,
		sourcePosition.m_sectorCoords,
		sectorPathCostCopy,
		sectorPathCopy,
		quadrant.m_sectors[targetPosition.m_sectorCoords.m_x][targetPosition.m_sectorCoords.m_y],
		targetPosition.m_coords,
		targetPosition.m_sectorCoords);

	auto pathTilePositions = FindSingleQuadrantPath(sectorPathCostCopy, sectorPathCopy, movementCostsCopy, sourcePosition, targetPosition);
	if (pathTilePositions)
	{
		ECS_Core::Components::MoveToPoint overallPath;
		// Translate back from local to global
		for (auto&& coordinate : *pathTilePositions)
		{
			// operator+ handles conversion back into standard size quadrants
			auto movementCost =
				*FetchQuadrant(coordinate.m_quadrantCoords).
				m_sectors[coordinate.m_sectorCoords.m_x][coordinate.m_sectorCoords.m_y].
				m_tileMovementCosts[coordinate.m_coords.m_x][coordinate.m_coords.m_y];
			overallPath.m_path.push_back({
				coordinate,
				movementCost });
			overallPath.m_totalPathCost += movementCost;
		}
		overallPath.m_targetPosition = targetPosition;
		return overallPath;
	}
	return std::nullopt;
}

template <int SX, int SY, int X, int Y>
std::optional<std::deque<TilePosition>> WorldTile::FindSingleQuadrantPath(
	Quadrant::PathCostArray<SX,SY>& sectorPathCostCopy,
	Quadrant::PathArray<SX,SY>& sectorPathCopy,
	const Sector::MultiSectorMovementArray<SX, SY, X, Y>& sectorMovementCosts,
	const TilePosition& sourcePosition,
	const TilePosition& targetPosition)
{
	auto sectorPath = Pathing::GetPath(
		sectorPathCostCopy,
		sourcePosition.m_sectorCoords,
		targetPosition.m_sectorCoords);

	if (sectorPath)
	{
		if (sectorPath->m_path.size() <= 3)
		{
			// We run a risk of the path being obviously inefficient because of our
			// multi-level pathing
			// Glue all the quadrants in the path together and get a path with the same algorithm as we
			// use for single-quadrant paths.
			// This avoids the inefficient appearance for local pathing which may cross a border.

			auto& firstSector = sectorPath->m_path.front();
			auto& lastSector = sectorPath->m_path.back();
			std::vector<CoordinateVector2> involvedSectors;

			if (firstSector.m_node.m_x == lastSector.m_node.m_x)
			{
				// A bit of a waste of space if there are only 2
				// Shame
				Sector::MovementCostArray<TileConstants::SECTOR_SIDE_LENGTH, TileConstants::SECTOR_SIDE_LENGTH * 3>
					tileMovementCosts;

				auto leastY = min(firstSector.m_node.m_y, lastSector.m_node.m_y);
				auto greatestY = max(firstSector.m_node.m_y, lastSector.m_node.m_y);
				for (auto y = leastY; y <= greatestY; ++y)
				{
					auto& sector = sectorMovementCosts[firstSector.m_node.m_x][y];
					for (int tileX = 0; tileX < TileConstants::SECTOR_SIDE_LENGTH; ++tileX)
					{
						for (int tileY = 0; tileY < TileConstants::SECTOR_SIDE_LENGTH; ++tileY)
						{
							auto yIndex = tileY + TileConstants::SECTOR_SIDE_LENGTH * (y - leastY);
							tileMovementCosts[tileX][yIndex] = sector[tileX][tileY];
						}
					}
				}
				auto localOffset = TilePosition(sourcePosition.m_quadrantCoords, { sourcePosition.m_sectorCoords.m_x, leastY }, {});
				auto localSourcePosition = sourcePosition - localOffset;
				auto localTargetPosition = targetPosition - localOffset;
				for (; localSourcePosition.m_sectorCoords.m_y > 0;
					--localSourcePosition.m_sectorCoords.m_y, localSourcePosition.m_coords.m_y += TileConstants::SECTOR_SIDE_LENGTH);
				for (; localTargetPosition.m_sectorCoords.m_y > 0;
					--localTargetPosition.m_sectorCoords.m_y, localTargetPosition.m_coords.m_y += TileConstants::SECTOR_SIDE_LENGTH);

				auto localPath = FindSingleSectorPath(tileMovementCosts, localSourcePosition, localTargetPosition);
				if (localPath)
				{
					std::deque<TilePosition> overallPath;
					// Translate back from local to global
					for (auto&& coordinate : localPath->m_path)
					{
						// operator+ handles conversion back into standard size quadrants
						overallPath.push_back(coordinate.m_tile + localOffset);
					}
					return overallPath;
				}
			}
			else if (firstSector.m_node.m_y == lastSector.m_node.m_y)
			{
				Sector::MovementCostArray<TileConstants::SECTOR_SIDE_LENGTH * 3, TileConstants::SECTOR_SIDE_LENGTH>
					tileMovementCosts;

				auto leastX = min(firstSector.m_node.m_x, lastSector.m_node.m_x);
				auto greatestX = max(firstSector.m_node.m_x, lastSector.m_node.m_x);
				for (auto x = leastX; x <= greatestX; ++x)
				{
					auto& sector = sectorMovementCosts[x][firstSector.m_node.m_y];
					for (int tileX = 0; tileX < TileConstants::SECTOR_SIDE_LENGTH; ++tileX)
					{
						for (int tileY = 0; tileY < TileConstants::SECTOR_SIDE_LENGTH; ++tileY)
						{
							auto xIndex = tileX + TileConstants::SECTOR_SIDE_LENGTH * (x - leastX);
							tileMovementCosts[xIndex][tileY] = sector[tileX][tileY];
						}
					}
				}
				auto localOffset = TilePosition(sourcePosition.m_quadrantCoords, { leastX, sourcePosition.m_sectorCoords.m_y }, {});
				auto localSourcePosition = sourcePosition - localOffset;
				auto localTargetPosition = targetPosition - localOffset;
				for (; localSourcePosition.m_sectorCoords.m_x > 0;
					--localSourcePosition.m_sectorCoords.m_x, localSourcePosition.m_coords.m_x += TileConstants::SECTOR_SIDE_LENGTH);
				for (; localTargetPosition.m_sectorCoords.m_x > 0;
					--localTargetPosition.m_sectorCoords.m_x, localTargetPosition.m_coords.m_x += TileConstants::SECTOR_SIDE_LENGTH);

				auto localPath = FindSingleSectorPath(tileMovementCosts, localSourcePosition, localTargetPosition);
				if (localPath)
				{
					std::deque<TilePosition> overallPath;
					// Translate back from local to global
					for (auto&& coordinate : localPath->m_path)
					{
						// operator+ handles conversion back into standard size quadrants
						overallPath.push_back(coordinate.m_tile + localOffset);
					}
					return overallPath;
				}
			}
			else
			{
				Sector::MovementCostArray<TileConstants::SECTOR_SIDE_LENGTH * 2, TileConstants::SECTOR_SIDE_LENGTH * 2>
					tileMovementCosts;
				// They're diagonally adjacent. Include the cross-adjacent quadrants
				auto lessX = min(firstSector.m_node.m_x, lastSector.m_node.m_x);
				auto lessY = min(firstSector.m_node.m_y, lastSector.m_node.m_y);
				auto greaterX = max(firstSector.m_node.m_x, lastSector.m_node.m_x);
				auto greaterY = max(firstSector.m_node.m_y, lastSector.m_node.m_y);

				for (auto x = lessX; x <= greaterX; ++x)
				{
					auto xIndex = (x - lessX) * TileConstants::SECTOR_SIDE_LENGTH;
					for (auto y = lessY; y <= greaterY; ++y)
					{
						auto yIndex = (y - lessY) * TileConstants::SECTOR_SIDE_LENGTH;
						auto& sector = sectorMovementCosts[x][y];
						for (int sectorX = 0; sectorX < TileConstants::SECTOR_SIDE_LENGTH; ++sectorX)
						{
							for (int sectorY = 0; sectorY < TileConstants::SECTOR_SIDE_LENGTH; ++sectorY)
							{
								tileMovementCosts[xIndex + sectorX][yIndex + sectorY] = sector[sectorX][sectorY];
							}
						}
					}
				}
				auto localOffset = TilePosition(sourcePosition.m_quadrantCoords, { lessX, lessY }, {});
				auto localSourcePosition = sourcePosition - localOffset;
				auto localTargetPosition = targetPosition - localOffset;
				for (; localSourcePosition.m_sectorCoords.m_x > 0;
					--localSourcePosition.m_sectorCoords.m_x, localSourcePosition.m_coords.m_x += TileConstants::SECTOR_SIDE_LENGTH);
				for (; localTargetPosition.m_sectorCoords.m_x > 0;
					--localTargetPosition.m_sectorCoords.m_x, localTargetPosition.m_coords.m_x += TileConstants::SECTOR_SIDE_LENGTH);
				for (; localSourcePosition.m_sectorCoords.m_y > 0;
					--localSourcePosition.m_sectorCoords.m_y, localSourcePosition.m_coords.m_y += TileConstants::SECTOR_SIDE_LENGTH);
				for (; localTargetPosition.m_sectorCoords.m_y > 0;
					--localTargetPosition.m_sectorCoords.m_y, localTargetPosition.m_coords.m_y += TileConstants::SECTOR_SIDE_LENGTH);
				
				auto localPath = FindSingleSectorPath(tileMovementCosts, localSourcePosition, localTargetPosition);
				if (localPath)
				{
					std::deque<TilePosition> overallPath;
					// Translate back from local to global
					for (auto&& coordinate : localPath->m_path)
					{
						// operator+ handles conversion back into standard size quadrants
						overallPath.push_back(coordinate.m_tile + localOffset);
					}
					return overallPath;
				}
			}
		}
		else
		{
			// Glue together: starting tile to exit tile of first sector
			// Rest of the sectors
			// Final sector entry tile to target tile.

			const auto& startingMacroPath = sectorPath->m_path.front();
			const auto& endingMacroPath = sectorPath->m_path.back();
			std::deque<TilePosition> overallPath;
			for (auto&& tile : *sectorPathCopy[sourcePosition.m_sectorCoords.m_x][sourcePosition.m_sectorCoords.m_y]
				[static_cast<int>(PathingDirection::_COUNT)][static_cast<int>(startingMacroPath.m_exitDirection)])
			{
				overallPath.push_back({ sourcePosition.m_quadrantCoords, sourcePosition.m_sectorCoords, tile });
			}

			for (int i = 1; i < sectorPath->m_path.size() - 1; ++i)
			{
				auto& path = sectorPath->m_path[i];
				auto& crossingPath = sectorPathCopy
					[path.m_node.m_x]
				[path.m_node.m_y]
				[static_cast<int>(path.m_entryDirection)]
				[static_cast<int>(path.m_exitDirection)];
				for (auto&& tile : *crossingPath)
				{
					overallPath.push_back({ sourcePosition.m_quadrantCoords,{ path.m_node.m_x, path.m_node.m_y }, tile });
				}
			}

			for (auto&& tile : *sectorPathCopy[targetPosition.m_sectorCoords.m_x][targetPosition.m_sectorCoords.m_y]
				[static_cast<int>(endingMacroPath.m_entryDirection)][static_cast<int>(PathingDirection::_COUNT)])
			{
				overallPath.push_back({ targetPosition.m_quadrantCoords, targetPosition.m_sectorCoords, tile });
			}
			return overallPath;
		}
	}
	return std::nullopt;
}

void WorldTile::ProcessPlanDirectionScout(
	const Action::LocalPlayer::PlanDirectionScout& planDirectionScout,
	const ecs::EntityIndex & governorEntity)
{// spawn menu with 8 direction buttons
					// TODO: Also select mission duration
					// Entity will also hold:
					// * Spawning building
					// * Owning governor
	auto directionMenuHandle = m_managerRef.createHandle();
	auto& scoutPlan = m_managerRef.addComponent<ECS_Core::Components::C_ScoutingPlan>(directionMenuHandle);
	scoutPlan.m_sourceBuildingHandle = planDirectionScout.m_scoutSource;
	scoutPlan.m_governorHandle = m_managerRef.getHandle(governorEntity);

	auto& uiFrame = m_managerRef.addComponent<ECS_Core::Components::C_UIFrame>(directionMenuHandle);
	uiFrame.m_size = { 150,150 };
	uiFrame.m_topLeftCorner = { 50, 300 };
	auto& graphics = m_managerRef.addComponent<ECS_Core::Components::C_SFMLDrawable>(directionMenuHandle);
	static const std::map<Direction, CartesianVector2<f64>> directionOffsets
	{
		{ Direction::NORTH,{ 60, 0 } },
	{ Direction::NORTHWEST,{ 0, 0 } },
	{ Direction::WEST,{ 0, 60 } },
	{ Direction::SOUTHWEST,{ 0, 120 } },
	{ Direction::SOUTH,{ 60, 120 } },
	{ Direction::SOUTHEAST,{ 120, 120 } },
	{ Direction::EAST,{ 120, 60 } },
	{ Direction::NORTHEAST,{ 120, 0 } },
	};

	auto background = std::make_shared<sf::RectangleShape>(sf::Vector2f{ 150.f, 150.f });
	background->setFillColor({ 6,6,6 });

	graphics.m_drawables[ECS_Core::Components::DrawLayer::MENU][5].push_back({ background,{} });

	for (auto&& direction : c_directions)
	{
		ECS_Core::Components::Button directionButton;
		directionButton.m_topLeftCorner = directionOffsets.at(direction);
		directionButton.m_size = { 30,30 };
		directionButton.m_onClick = [direction, &manager = m_managerRef](const ecs::EntityIndex& /*clicker*/, const ecs::EntityIndex& clickedEntity) {
			auto& scoutPlan = manager.getComponent<ECS_Core::Components::C_ScoutingPlan>(clickedEntity);
			auto& sourcePosition = manager.getComponent<ECS_Core::Components::C_TilePosition>(scoutPlan.m_sourceBuildingHandle);
			manager.addTag<ECS_Core::Tags::T_Dead>(clickedEntity);
			return Action::CreateExplorationUnit(sourcePosition.m_position,
				manager.getEntityIndex(scoutPlan.m_sourceBuildingHandle),
				5,
				30,
				direction);
		};
		uiFrame.m_buttons.push_back(directionButton);

		auto buttonGraphic = std::make_shared<sf::RectangleShape>(sf::Vector2f{ 30.f,30.f });
		buttonGraphic->setFillColor({ 130,130,130 });
		graphics.m_drawables[ECS_Core::Components::DrawLayer::MENU][6].push_back({ buttonGraphic, directionOffsets.at(direction) });
	}
}

void WorldTile::CollectTiles(std::set<TilePosition>& possibleTiles, int movesRemaining, const TilePosition& position)
{
	auto tile = GetTile(position);
	if (!tile) return;
	if (!(*tile)->m_movementCost) return;
	if (!possibleTiles.insert(position).second) return;
	if (movesRemaining <= 0)
	{
		return;
	}
	CollectTiles(possibleTiles, movesRemaining - 1, position + TilePosition{ {}, {}, {0, 1 }});
	CollectTiles(possibleTiles, movesRemaining - 1, position + TilePosition{ {}, {}, {0, -1 }});
	CollectTiles(possibleTiles, movesRemaining - 1, position + TilePosition{ {}, {}, {1, 0 }});
	CollectTiles(possibleTiles, movesRemaining - 1, position + TilePosition{ {}, {}, {-1, 0 }});
}

void WorldTile::ProgramInit() {}
void WorldTile::SetupGameplay() {
	std::thread([this]() {
		auto spawnThread = SpawnQuadrant({ 0, 0 });
		spawnThread.join();
		m_baseQuadrantSpawned = true;

		for (int x = -1; x < 2; ++x)
		{
			for (int y = -1; y < 2; ++y)
			{
				if (x != 0 || y != 0)
				{
					SpawnQuadrant({ x,y }).join();
				}
			}
		}
	}).detach();
}

void WorldTile::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	switch (phase)
	{
	case GameLoopPhase::PREPARATION:
		if (m_baseQuadrantSpawned && !m_startingBuilderSpawned)
		{
			using namespace TileConstants;
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

				auto& sector = FetchQuadrant({ 0,0 }).m_sectors[sectorX][sectorY];
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
					m_startingBuilderSpawned = true;
					m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_UserIO>([&](
						const ecs::EntityIndex&,
						const ECS_Core::Components::C_UserInputs&,
						ECS_Core::Components::C_ActionPlan& plan)
					{
						Action::CreateBuildingUnit buildingUnit;
						buildingUnit.m_spawningPosition = { 0,0, sectorX, sectorY, tileX, tileY };
						buildingUnit.m_movementSpeed = 20;
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
		m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_Input>([this](
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

		m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_Planner>([&manager = m_managerRef, this](
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

					auto path = GetPath(sourcePosition, targetPosition);
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
					auto deliveryTileOpt = GetTile(createCaravan.m_deliveryPosition);
					if (!deliveryTileOpt)
					{
						continue;
					}
					auto&& deliveryTile = **deliveryTileOpt;
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
					for (auto&&[birthMonth, pop] : sourcePopulation.m_populations)
					{
						if (pop.m_class != ECS_Core::Components::PopulationClass::WORKERS)
						{
							continue;
						}
						sourceTotalMen += pop.m_numMen;
						sourceTotalWomen += pop.m_numWomen;
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
						manager.getComponent<ECS_Core::Components::C_TilePosition>(*deliveryTile.m_owningBuilding).m_position);
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
					manager.addComponent<ECS_Core::Components::C_Vision>(newEntity);
					auto& caravanPath = manager.addComponent<ECS_Core::Components::C_CaravanPath>(newEntity);
					movingUnit.m_currentMovement = *path;
					caravanPath.m_basePath = *path;
					caravanPath.m_originBuildingHandle = manager.getHandle(*createCaravan.m_popSource);
					caravanPath.m_targetBuildingHandle = *deliveryTile.m_owningBuilding;

					auto& moverInventory = manager.addComponent<ECS_Core::Components::C_ResourceInventory>(newEntity);
					auto& population = manager.addComponent<ECS_Core::Components::C_Population>(newEntity);
					int menMoved = 0;
					int womenMoved = 0;
					for (auto&&[birthMonth, pop] : sourcePopulation.m_populations)
					{
						if (menMoved == totalMenToMove && totalWomenToMove == 5)
						{
							break;
						}
						if (pop.m_class != ECS_Core::Components::PopulationClass::WORKERS)
						{
							continue;
						}
						auto menToMove = min<int>(totalMenToMove - menMoved, pop.m_numMen);
						auto womenToMove = min<int>(totalWomenToMove - womenMoved, pop.m_numWomen);

						auto& popCopy = population.m_populations[birthMonth];
						popCopy = pop;
						popCopy.m_numMen = menToMove;
						popCopy.m_numWomen = womenToMove;

						pop.m_numMen -= menToMove;
						pop.m_numWomen -= womenToMove;


						menMoved += menToMove;
						womenMoved += womenToMove;
					}

					sourceInventory.m_collectedYields[ECS_Core::Components::Yields::FOOD] -= 100;
					sourceInventory.m_collectedYields[ECS_Core::Components::Yields::WOOD] -= 50;
					moverInventory.m_collectedYields[ECS_Core::Components::Yields::FOOD] = 50;

					std::map<f64, std::vector<ECS_Core::Components::YieldType>, std::greater<f64>> heldResources;
					for (auto&& [resource, amount] : sourceInventory.m_collectedYields)
					{
						heldResources[amount].push_back(resource);
					}
					int resourcesMoved = 0;
					for (auto&& [amount,heldResources] : heldResources)
					{
						s32 amountToMove = min<s32>(static_cast<s32>(amount), 100);
						for (auto&& resource : heldResources)
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
					auto tileOpt = GetTile(position);
					if (!tileOpt)
					{
						continue;
					}
					if ((*tileOpt)->m_owningBuilding)
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
	{
		// Grow territories that are able to do so before taking any actions
		GrowTerritories();

		auto& time = m_managerRef.getComponent<ECS_Core::Components::C_TimeTracker>(
			m_managerRef.entitiesMatching<ECS_Core::Signatures::S_TimeTracker>().front());
		m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_MovingUnit>(
			[this, &time](
				const ecs::EntityIndex&,
				const ECS_Core::Components::C_TilePosition& tilePosition,
				ECS_Core::Components::C_MovingUnit& movement,
				const ECS_Core::Components::C_Population&,
				const ECS_Core::Components::C_Vision& vision)
		{
			if (!movement.m_explorationPlan)
			{
				// Only interested in explorers
				return ecs::IterationBehavior::CONTINUE;
			}
			if (movement.m_currentMovement)
			{
				// Only interested in the ones that aren't currently on a path
				return ecs::IterationBehavior::CONTINUE;
			}

			auto daysInCurrentDate = ((time.m_year * 12) + time.m_month) * 30 + time.m_day;
			auto leaveTimeTotalDays =
				((movement.m_explorationPlan->m_leavingYear * 12)
					+ movement.m_explorationPlan->m_leavingMonth) * 30
					+ movement.m_explorationPlan->m_leavingDay;
			if (daysInCurrentDate - leaveTimeTotalDays > movement.m_explorationPlan->m_daysToExplore)
			{
				if (tilePosition.m_position == movement.m_explorationPlan->m_homeBasePosition)
				{
					// Re-integrate, give info (government module will transition remaining people)
					movement.m_explorationPlan->m_explorationComplete = true;
				}
				else
				{
					auto path = GetPath(tilePosition.m_position, movement.m_explorationPlan->m_homeBasePosition);
					if (path)
					{
						movement.m_currentMovement = path;
					}
				}
			}
			else
			{
				// generate set of tiles the dude can see
				std::set<TilePosition> possibleTiles;
				CollectTiles(possibleTiles, vision.m_visionRadius, tilePosition.m_position);
				// order them by angle with exploration direction, secondary by length
				auto sortFunction = [&movement, &tilePosition, this](
					const TilePosition& left,
					const TilePosition& right) -> bool
				{
					static const std::map<Direction, CoordinateVector2> c_angles = {
						{ Direction::NORTH,{ 0, -1 } },
						{ Direction::NORTHEAST,{ 1, -1 } },
						{ Direction::EAST,{ 1, 0 } },
						{ Direction::SOUTHEAST,{ 1, 1 } },
						{ Direction::SOUTH,{ 0, 1 } },
						{ Direction::SOUTHWEST,{ -1, 1 } },
						{ Direction::WEST,{ -1, 0 } },
						{ Direction::NORTHWEST,{ -1, -1 } },
					};

					bool visitedLeft = movement.m_explorationPlan->m_visitedPathNodes.count(left) > 0;
					bool visitedRight = movement.m_explorationPlan->m_visitedPathNodes.count(right) > 0;
					if (visitedLeft && !visitedRight) return false;
					if (visitedRight && !visitedLeft) return true;

					auto leftDifference = CoordinatesToWorldPosition(left) - CoordinatesToWorldPosition(tilePosition.m_position);
					auto rightDifference = CoordinatesToWorldPosition(right) - CoordinatesToWorldPosition(tilePosition.m_position);

					if (leftDifference.MagnitudeSq() == 0 && rightDifference.MagnitudeSq() == 0) return false;
					if (leftDifference.MagnitudeSq() == 0) return false;
					if (rightDifference.MagnitudeSq() == 0) return true;

					auto& directionVector = c_angles.at(movement.m_explorationPlan->m_direction);
					// Which is closer in angle to the target direction?
					auto leftDot = directionVector.m_x * leftDifference.m_x + directionVector.m_y * leftDifference.m_y;
					auto rightDot = directionVector.m_x * rightDifference.m_x + directionVector.m_y * rightDifference.m_y;

					auto leftAngle = acos(1. * leftDot / (sqrt(leftDifference.MagnitudeSq()) * sqrt(directionVector.MagnitudeSq())));
					auto rightAngle = acos(1. * rightDot / (sqrt(rightDifference.MagnitudeSq()) * sqrt(directionVector.MagnitudeSq())));
					if (leftAngle < rightAngle) return true;
					if (rightAngle < leftAngle) return false;

					// Take the farther one
					return leftDifference.MagnitudeSq() > rightDifference.MagnitudeSq();
				};

				std::vector<TilePosition> positionVector(possibleTiles.begin(), possibleTiles.end());
				std::sort(positionVector.begin(), positionVector.end(), sortFunction);

				// Iterate through until we get a valid path
				for (auto&& position : positionVector)
				{
					auto path = GetPath(tilePosition.m_position, position);
					if (path)
					{
						movement.m_explorationPlan->m_visitedPathNodes.insert(position);
						movement.m_currentMovement = path;
						break;
					}
				}
			}

			return ecs::IterationBehavior::CONTINUE;
		});
	}
		break;

	case GameLoopPhase::ACTION_RESPONSE:
		// Update position of any world-tile drawables
		m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_TilePositionable>(
			[this](
				ecs::EntityIndex mI,
				ECS_Core::Components::C_PositionCartesian& position,
				const ECS_Core::Components::C_TilePosition& tilePosition)
		{
			auto worldPosition = CoordinatesToWorldPosition(tilePosition.m_position);
			position.m_position.m_x = static_cast<f64>(worldPosition.m_x);
			position.m_position.m_y = static_cast<f64>(worldPosition.m_y);
			return ecs::IterationBehavior::CONTINUE;
		});

		m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_MovingUnit>(
			[&manager = m_managerRef, this](
				ecs::EntityIndex mI,
				ECS_Core::Components::C_TilePosition& tilePosition,
				ECS_Core::Components::C_MovingUnit& mover,
				const ECS_Core::Components::C_Population&,
				const ECS_Core::Components::C_Vision&)
		{
			static const std::map<Direction, CoordinateVector2> c_angles = {
				{ Direction::NORTH,{ 0, -1 } },
			{ Direction::NORTHEAST,{ 1, -1 } },
			{ Direction::EAST,{ 1, 0 } },
			{ Direction::SOUTHEAST,{ 1, 1 } },
			{ Direction::SOUTH,{ 0, 1 } },
			{ Direction::SOUTHWEST,{ -1, 1 } },
			{ Direction::WEST,{ -1, 0 } },
			{ Direction::NORTHWEST,{ -1, -1 } },
			};
			if (mover.m_explorationPlan)
			{
				FetchQuadrant(tilePosition.m_position.m_quadrantCoords + c_angles.at(mover.m_explorationPlan->m_direction));
			}
			return ecs::IterationBehavior::CONTINUE;
		});

		m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_UserIO>(
			[&manager = m_managerRef, this](
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
						governorEntity);
					continue;
				}
				else if (std::holds_alternative<Action::LocalPlayer::PlanTargetedMotion>(action))
				{
					ProcessPlanTargetedMotion(
						std::get<Action::LocalPlayer::PlanTargetedMotion>(action),
						governorEntity);
				}
				else if (std::holds_alternative<Action::LocalPlayer::PlanCaravan>(action))
				{
					ProcessPlanCaravan(
						std::get<Action::LocalPlayer::PlanCaravan>(action),
						governorEntity);
				}
				else if (std::holds_alternative<Action::LocalPlayer::PlanDirectionScout>(action))
				{
					ProcessPlanDirectionScout(std::get<Action::LocalPlayer::PlanDirectionScout>(action), governorEntity);
				}
				else if (std::holds_alternative<Action::LocalPlayer::CancelMovementPlan>(action))
				{
					CancelMovementPlans();
				}
			}
			return ecs::IterationBehavior::CONTINUE;
		});

		m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_CompleteBuilding>(
			[this](
			const ecs::EntityIndex&,
			const ECS_Core::Components::C_BuildingDescription&,
			const ECS_Core::Components::C_TilePosition&,
			const ECS_Core::Components::C_Territory& territory,
			ECS_Core::Components::C_TileProductionPotential& potential,
			const ECS_Core::Components::C_ResourceInventory&)
		{
			if (potential.m_availableYields.size())
			{
				return ecs::IterationBehavior::CONTINUE;
			}

			UpdateTerritoryProductionPotential(potential, territory);
			return ecs::IterationBehavior::CONTINUE;
		});
		break;

	case GameLoopPhase::RENDER:
		break;
	case GameLoopPhase::CLEANUP:
		ReturnDeadBuildingTiles();
		return;
	}
}

void WorldTile::ProcessPlanCaravan(Action::LocalPlayer::PlanCaravan & planCaravan, const ecs::EntityIndex & governorEntity)
{
	if (!m_managerRef.hasComponent<ECS_Core::Components::C_TilePosition>(planCaravan.m_sourceHandle))
	{
		// Only try to move if we have a mover unit with a tile position
		return;
	}
	auto targetingEntity = m_managerRef.createHandle();
	auto& targetScreenPosition = m_managerRef.addComponent<ECS_Core::Components::C_PositionCartesian>(targetingEntity);
	auto& targetTilePosition = m_managerRef.addComponent<ECS_Core::Components::C_TilePosition>(targetingEntity);
	targetTilePosition.m_position = m_managerRef.getComponent<ECS_Core::Components::C_TilePosition>(planCaravan.m_sourceHandle).m_position;

	auto& caravanInfo = m_managerRef.addComponent<ECS_Core::Components::C_CaravanPlan>(targetingEntity);
	caravanInfo.m_sourceBuildingHandle = planCaravan.m_sourceHandle;
	caravanInfo.m_governorHandle = m_managerRef.getHandle(governorEntity);

	auto& drawable = m_managerRef.addComponent<ECS_Core::Components::C_SFMLDrawable>(targetingEntity);
	auto targetGraphic = std::make_shared<sf::CircleShape>(2.5f, 8);
	targetGraphic->setFillColor({ 128, 128, 64, 128 });
	targetGraphic->setOutlineColor({ 128, 128,64 });
	targetGraphic->setOutlineThickness(-0.75f);
	drawable.m_drawables[ECS_Core::Components::DrawLayer::EFFECT][128].push_back({ targetGraphic,{} });
}

void WorldTile::ProcessPlanTargetedMotion(Action::LocalPlayer::PlanTargetedMotion & planMotion, const ecs::EntityIndex & governorEntity)
{
	if (!m_managerRef.hasComponent<ECS_Core::Components::C_MovingUnit>(planMotion.m_moverHandle)
		|| !m_managerRef.hasComponent<ECS_Core::Components::C_TilePosition>(planMotion.m_moverHandle))
	{
		return;
	}
	auto targetingEntity = m_managerRef.createHandle();
	auto& targetScreenPosition = m_managerRef.addComponent<ECS_Core::Components::C_PositionCartesian>(targetingEntity);
	auto& targetTilePosition = m_managerRef.addComponent<ECS_Core::Components::C_TilePosition>(targetingEntity);
	targetTilePosition.m_position = m_managerRef.getComponent<ECS_Core::Components::C_TilePosition>(planMotion.m_moverHandle).m_position;

	auto& moverInfo = m_managerRef.addComponent<ECS_Core::Components::C_MovementTarget>(targetingEntity);
	moverInfo.m_moverHandle = planMotion.m_moverHandle;
	moverInfo.m_governorHandle = m_managerRef.getHandle(governorEntity);

	auto& drawable = m_managerRef.addComponent<ECS_Core::Components::C_SFMLDrawable>(targetingEntity);
	auto targetGraphic = std::make_shared<sf::CircleShape>(2.5f, 6);
	targetGraphic->setFillColor({ 128, 128, 0, 128 });
	targetGraphic->setOutlineColor({ 128, 128, 0 });
	targetGraphic->setOutlineThickness(-0.75f);
	drawable.m_drawables[ECS_Core::Components::DrawLayer::EFFECT][128].push_back({ targetGraphic,{} });
}

void WorldTile::CancelMovementPlans()
{
	m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_CaravanPlanIndicator>(
		[&manager = m_managerRef](
			const ecs::EntityIndex& entity,
			const ECS_Core::Components::C_CaravanPlan&,
			const ECS_Core::Components::C_TilePosition&)
	{
		manager.addTag<ECS_Core::Tags::T_Dead>(entity);
		return ecs::IterationBehavior::CONTINUE;
	});
	m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_MovementPlanIndicator>(
		[&manager = m_managerRef](
			const ecs::EntityIndex& entity,
			const ECS_Core::Components::C_MovementTarget&,
			const ECS_Core::Components::C_TilePosition&)
	{
		manager.addTag<ECS_Core::Tags::T_Dead>(entity);
		return ecs::IterationBehavior::CONTINUE;
	});
}

bool WorldTile::ShouldExit()
{
	return false;
}

DEFINE_SYSTEM_INSTANTIATION(WorldTile);