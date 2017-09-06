//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Util/Pathing.h
// A*-based implementation of a pathing system

// A* Algorithm implemented thanks to the kind people who wrote the wikipedia page

#include "../Core/typedef.h"
#include "../ECS/ECS.h"

#include <deque>

namespace Pathing
{
	struct SortedCoordinate;
}

namespace std
{
	bool operator<(const Pathing::SortedCoordinate& left, const Pathing::SortedCoordinate& right);
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
	}

	template<int X, int Y>
	using MovementCostArray2 = std::optional<int>[X][Y];

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

	static const CoordinateVector2 neighborOffsets[] = {
		{ -1, 0 }, // NORTH
		{  1, 0 }, // SOUTH
		{  0, 1 }, // EAST
		{  0,-1 }, // WEST
	};

	// Movement Cost is cost to enter a node.
	template<int X, int Y>
	std::optional<std::deque<CoordinateVector2>> GetPath(
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
			const auto& currentNode = *openPoints.begin();
			if (currentNode.m_coordinates == goal)
			{
				std::deque<CoordinateVector2> result;
				auto currentCoords = currentNode.m_coordinates;
				while (true) // because I'm evil
				{
					result.push_front(currentCoords);
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
}
