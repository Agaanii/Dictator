//-----------------------------------------------------------------------------
// All code is property of Dictator Developers Inc
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2018

// Util/Pathing.h
// A*-based implementation of a pathing system

// A* Algorithm implemented thanks to the kind people who wrote the wikipedia page

#pragma once

#include "../Core/typedef.h"
#include "../ECS/ECS.h"

#include <deque>

namespace Pathing
{
	struct SortedCoordinate;
	struct SortedDirectionalCoordinate;
}

namespace std
{
	bool operator<(const Pathing::SortedCoordinate& left, const Pathing::SortedCoordinate& right);
	bool operator<(const Pathing::SortedDirectionalCoordinate& left, const Pathing::SortedDirectionalCoordinate& right);
}

namespace Pathing
{
	using namespace ECS_Core::Components;
	namespace PathingSide
	{
		enum Enum
		{
			NORTH,
			SOUTH,
			EAST,
			WEST,

			_COUNT
		};

		Enum Opposite(Enum d);
	}

	template<int X, int Y>
	using MovementCostArray2 = std::optional<int>[X][Y];

	// Used for items where movement cost varies depending on direction entered and exited
	template <int X, int Y>
	using DirectionMovementCostArray = std::optional<int>[X][Y][PathingSide::_COUNT][PathingSide::_COUNT];

	struct SortedCoordinate
	{
		SortedCoordinate(
			const CoordinateVector2& coordinates,
			int costToPoint,
			int airDistToTarget)
			: m_coordinates(coordinates)
			, m_costToPoint(costToPoint)
			, m_airDistToTarget(airDistToTarget)
		{
		}

		CoordinateVector2 m_coordinates;
		int m_costToPoint{ 0 };
		int m_airDistToTarget{ 0 };
	};

	struct SortedDirectionalCoordinate
	{
		SortedDirectionalCoordinate(
			const CoordinateVector2& coordinates,
			int costToPoint,
			int airDistToTarget,
			int entry,
			int exit,
			const std::deque<CoordinateVector2>& previousPath)
			: m_coordinates(coordinates)
			, m_costToPoint(costToPoint)
			, m_airDistToTarget(airDistToTarget)
			, m_entry(static_cast<PathingSide::Enum>(entry))
			, m_exit(static_cast<PathingSide::Enum>(exit))
			, m_previousPath(previousPath)
		{
			m_previousPath.push_back(coordinates);
		}

		CoordinateVector2 m_coordinates;
		int m_costToPoint{ 0 };
		int m_airDistToTarget{ 0 };
		PathingSide::Enum m_entry{ PathingSide::_COUNT };
		PathingSide::Enum m_exit{ PathingSide::_COUNT };
		std::deque<CoordinateVector2> m_previousPath;
	};

	static const CoordinateVector2 neighborOffsets[] = {
		{ 0, -1 }, // NORTH
	{ 0,  1 }, // SOUTH
	{ 1,  0 }, // EAST
	{ -1,  0 }, // WEST
	};

	// Movement Cost is cost to enter a node.
	template<int X, int Y>
	std::optional<Path> GetPath(
		const MovementCostArray2<X, Y>& movementCosts,
		const CoordinateVector2& origin,
		const CoordinateVector2& goal)
	{
		bool triviallyReachable = false;
		for (int direction = PathingSide::NORTH; direction < PathingSide::_COUNT; ++direction)
		{
			auto& neighborOffset = neighborOffsets[direction];
			auto neighborCoords = goal + neighborOffset;
			if (neighborCoords.m_x < 0 || neighborCoords.m_x >= X ||
				neighborCoords.m_y < 0 || neighborCoords.m_y >= Y)
			{
				continue;
			}
			if (movementCosts[neighborCoords.m_x][neighborCoords.m_y])
			{
				triviallyReachable = true;
				break;
			}
		}
		if (!triviallyReachable)
		{
			return std::nullopt;
		}
		bool visited[X][Y];
		int costToPoint[X][Y];
		int pointHeuristic[X][Y];
		PathingSide::Enum fastestDirectionIntoNode[X][Y];
		for (auto i = 0; i < X; ++i)
		{
			for (auto j = 0; j < Y; ++j)
			{
				visited[i][j] = false;
				costToPoint[i][j] = std::numeric_limits<int>::max();
				pointHeuristic[i][j] = static_cast<int>((goal - CoordinateVector2(i, j)).MagnitudeSq());
				fastestDirectionIntoNode[i][j] = PathingSide::_COUNT;
			}
		}
		// Cost to enter the current node is always 0
		costToPoint[origin.m_x][origin.m_y] = 0;
		std::set<SortedCoordinate> openPoints;
		openPoints.insert({ origin, 0, pointHeuristic[origin.m_x][origin.m_y] });

		while (!openPoints.empty())
		{
			auto currentNode = *openPoints.begin();
			if (currentNode.m_coordinates == goal)
			{
				Path result;
				result.m_totalPathCost = costToPoint[goal.m_x][goal.m_y];
				auto currentCoords = currentNode.m_coordinates;
				while (true) // because I'm evil
				{
					result.m_path.push_front(currentCoords);
					if (origin == currentCoords)
					{
						return result;
					}
					currentCoords -= neighborOffsets[fastestDirectionIntoNode
						[currentCoords.m_x][currentCoords.m_y]];
				}
			}

			if (visited[currentNode.m_coordinates.m_x][currentNode.m_coordinates.m_y])
			{
				openPoints.erase(currentNode);
				continue;
			}

			visited[currentNode.m_coordinates.m_x][currentNode.m_coordinates.m_y] = true;

			for (int direction = PathingSide::NORTH; direction < PathingSide::_COUNT; ++direction)
			{
				auto& neighborOffset = neighborOffsets[direction];
				auto neighborCoords = currentNode.m_coordinates + neighborOffset;
				if (neighborCoords.m_x < 0 || neighborCoords.m_x >= X ||
					neighborCoords.m_y < 0 || neighborCoords.m_y >= Y)
				{
					continue;
				}
				if (visited[neighborCoords.m_x][neighborCoords.m_y])
				{
					continue;
				}
				if (!movementCosts[neighborCoords.m_x][neighborCoords.m_y])
				{
					continue;
				}

				auto costToNeighbor = currentNode.m_costToPoint + *movementCosts[neighborCoords.m_x][neighborCoords.m_y];
				if (costToNeighbor >= costToPoint[neighborCoords.m_x][neighborCoords.m_y])
				{
					continue;
				}

				fastestDirectionIntoNode[neighborCoords.m_x][neighborCoords.m_y] = static_cast<PathingSide::Enum>(direction);
				costToPoint[neighborCoords.m_x][neighborCoords.m_y] = costToNeighbor;
				openPoints.insert({
					neighborCoords,
					costToNeighbor,
					pointHeuristic[neighborCoords.m_x][neighborCoords.m_y]
					});
			}

			openPoints.erase(currentNode);
		}
		return std::nullopt;
	}

	// Movement cost is cost to move through the previous node from a certain side to
	// the side adjacent to the current node
	template<int X, int Y>
	std::optional<Path> GetPath(
		const DirectionMovementCostArray<X, Y>& movementCosts,
		const CoordinateVector2& origin,
		PathingSide::Enum originDirection,
		const CoordinateVector2& goal)
	{
		bool triviallyReachable = false;
		for (int direction = PathingSide::NORTH; direction < PathingSide::_COUNT; ++direction)
		{
			auto& neighborOffset = neighborOffsets[direction];
			auto neighborCoords = goal + neighborOffset;
			if (neighborCoords.m_x < 0 || neighborCoords.m_x >= X ||
				neighborCoords.m_y < 0 || neighborCoords.m_y >= Y)
			{
				continue;
			}
			auto& neighborMovementCosts = movementCosts[neighborCoords.m_x][neighborCoords.m_y];
			for (int enterDirection = PathingSide::NORTH; enterDirection < PathingSide::_COUNT; ++enterDirection)
			{
				if (enterDirection == PathingSide::Opposite(static_cast<PathingSide::Enum>(direction)))
				{
					continue;
				}
				if (neighborMovementCosts[enterDirection][PathingSide::Opposite(static_cast<PathingSide::Enum>(direction))])
				{
					triviallyReachable = true;
					break;
				}
			}
			if (triviallyReachable) break;
		}
		if (!triviallyReachable)
		{
			return std::nullopt;
		}
		bool visited[X][Y][PathingSide::_COUNT][PathingSide::_COUNT];
		int costToPoint[X][Y][PathingSide::_COUNT][PathingSide::_COUNT];
		int pointHeuristic[X][Y];
		PathingSide::Enum fastestDirectionIntoNode[X][Y];
		for (auto i = 0; i < X; ++i)
		{
			for (auto j = 0; j < Y; ++j)
			{
				for (int inDirection = PathingSide::NORTH; inDirection < PathingSide::_COUNT; ++inDirection)
				{
					for (int outDirection = PathingSide::NORTH; outDirection < PathingSide::_COUNT; ++outDirection)
					{
						visited[i][j][inDirection][outDirection] = false;
						costToPoint[i][j][inDirection][outDirection] = std::numeric_limits<int>::max();
					}
				}
				pointHeuristic[i][j] = static_cast<int>((goal - CoordinateVector2(i, j)).MagnitudeSq());
				fastestDirectionIntoNode[i][j] = PathingSide::_COUNT;
			}
		}
		std::set<SortedDirectionalCoordinate> openPoints;
		for (int exitDirection = PathingSide::NORTH; exitDirection < PathingSide::_COUNT; ++exitDirection)
		{
			if (exitDirection == originDirection) continue;
			if (!movementCosts[origin.m_x][origin.m_y][originDirection][exitDirection]) continue;

			// Cost to enter the current node is always 0
			costToPoint[origin.m_x][origin.m_y][originDirection][exitDirection] = 0;
			openPoints.insert({ origin, 0, pointHeuristic[origin.m_x][origin.m_y], originDirection, exitDirection,{} });
		}
		fastestDirectionIntoNode[origin.m_x][origin.m_y] = originDirection;

		while (!openPoints.empty())
		{
			auto currentNode = *openPoints.begin();
			if (currentNode.m_coordinates == goal)
			{
				Path result;
				result.m_totalPathCost = costToPoint[goal.m_x][goal.m_y][currentNode.m_entry][currentNode.m_exit];
				result.m_path = currentNode.m_previousPath;
				return result;
			}

			if (visited[currentNode.m_coordinates.m_x][currentNode.m_coordinates.m_y][currentNode.m_entry][currentNode.m_exit])
			{
				openPoints.erase(currentNode);
				continue;
			}

			visited[currentNode.m_coordinates.m_x][currentNode.m_coordinates.m_y][currentNode.m_entry][currentNode.m_exit] = true;

			auto& neighborOffset = neighborOffsets[currentNode.m_exit];
			auto neighborCoords = currentNode.m_coordinates + neighborOffset;
			if (neighborCoords.m_x < 0 || neighborCoords.m_x >= X ||
				neighborCoords.m_y < 0 || neighborCoords.m_y >= Y)
			{
				openPoints.erase(currentNode);
				continue;
			}
			auto nodeMovementCosts = movementCosts[currentNode.m_coordinates.m_x][currentNode.m_coordinates.m_y];
			auto originDirection = PathingSide::Opposite(currentNode.m_exit);
			for (int exitDirection = PathingSide::NORTH; exitDirection < PathingSide::_COUNT; ++exitDirection)
			{
				if (exitDirection == originDirection) continue;
				if (!nodeMovementCosts[originDirection][exitDirection]) continue;

				if (visited[neighborCoords.m_x][neighborCoords.m_y][originDirection][exitDirection])
				{
					continue;
				}

				auto costToNeighbor = currentNode.m_costToPoint + *nodeMovementCosts[originDirection][exitDirection];
				if (costToNeighbor >= costToPoint[neighborCoords.m_x][neighborCoords.m_y][originDirection][exitDirection])
				{
					continue;
				}

				fastestDirectionIntoNode[neighborCoords.m_x][neighborCoords.m_y] = PathingSide::Opposite(currentNode.m_exit);
				costToPoint[neighborCoords.m_x][neighborCoords.m_y][originDirection][exitDirection] = costToNeighbor;
				openPoints.insert({
					neighborCoords,
					costToNeighbor,
					pointHeuristic[neighborCoords.m_x][neighborCoords.m_y],
					originDirection,
					exitDirection,
					currentNode.m_previousPath });
			}

			openPoints.erase(currentNode);
		}
		return std::nullopt;
	}
}
