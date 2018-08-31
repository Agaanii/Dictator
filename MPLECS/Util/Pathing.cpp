//-----------------------------------------------------------------------------
// All code is property of Dictator Developers Inc
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2018

// Util/Pathing.cpp
// A*-based implementation of a pathing system

#include "Pathing.h"
namespace std
{
	bool operator<(const Pathing::SortedCoordinate& left, const Pathing::SortedCoordinate& right)
	{
		if (left.m_airDistToTarget == 0 && right.m_airDistToTarget != 0)
		{
			return true;
		}
		if (left.m_airDistToTarget != 0 && right.m_airDistToTarget == 0)
		{
			return false;
		}
		if ((left.m_airDistToTarget + left.m_costToPoint) < (right.m_airDistToTarget + right.m_costToPoint))
		{
			return true;
		}
		if ((left.m_airDistToTarget + left.m_costToPoint) > (right.m_airDistToTarget + right.m_costToPoint))
		{
			return false;
		}
		return left.m_coordinates < right.m_coordinates;
	}

	bool operator<(const Pathing::SortedDirectionalCoordinate& left, const Pathing::SortedDirectionalCoordinate& right)
	{
		if (left.m_airDistToTarget == 0 && right.m_airDistToTarget != 0)
		{
			return true;
		}
		if (left.m_airDistToTarget != 0 && right.m_airDistToTarget == 0)
		{
			return false;
		}
		if ((left.m_airDistToTarget + left.m_costToPoint) < (right.m_airDistToTarget + right.m_costToPoint))
		{
			return true;
		}
		if ((left.m_airDistToTarget + left.m_costToPoint) > (right.m_airDistToTarget + right.m_costToPoint))
		{
			return false;
		}
		if (left.m_entry < right.m_entry) return true;
		if (left.m_entry > right.m_entry) return false;

		if (left.m_exit < right.m_exit) return true;
		if (left.m_exit > right.m_exit) return false;

		return left.m_coordinates < right.m_coordinates;
	}
}

PathingDirection Opposite(PathingDirection d)
{
	switch (d)
	{
	case PathingDirection::NORTH: return PathingDirection::SOUTH;
	case PathingDirection::SOUTH: return PathingDirection::NORTH;
	case PathingDirection::EAST:  return PathingDirection::WEST;
	case PathingDirection::WEST:  return PathingDirection::EAST;
	}
	return PathingDirection::_COUNT;
}

PathingDirection Clockwise90(PathingDirection d)
{
	switch (d)
	{
	case PathingDirection::NORTH: return PathingDirection::EAST;
	case PathingDirection::EAST: return PathingDirection::SOUTH;
	case PathingDirection::SOUTH: return PathingDirection::WEST;
	case PathingDirection::WEST: return PathingDirection::NORTH;
	}
	return PathingDirection::_COUNT;
}

PathingDirection Counterclockwise90(PathingDirection d)
{
	switch (d)
	{
	case PathingDirection::NORTH: return PathingDirection::WEST;
	case PathingDirection::WEST: return PathingDirection::SOUTH;
	case PathingDirection::SOUTH: return PathingDirection::EAST;
	case PathingDirection::EAST: return PathingDirection::NORTH;
	}
	return PathingDirection::_COUNT;
}

Direction Opposite(Direction d)
{
	switch (d)
	{
	case Direction::NORTH: return Direction::SOUTH;
	case Direction::SOUTH: return Direction::NORTH;
	case Direction::EAST:  return Direction::WEST;
	case Direction::WEST:  return Direction::EAST;
	case Direction::NORTHEAST: return Direction::SOUTHWEST;
	case Direction::SOUTHEAST: return Direction::NORTHWEST;
	case Direction::NORTHWEST:  return Direction::SOUTHEAST;
	case Direction::SOUTHWEST:  return Direction::NORTHEAST;
	}
	return Direction::_COUNT;
}

auto Pathing::GetPath(
	const DirectionMovementCostMap& movementCosts,
	const CoordinateVector2& origin,
	const CoordinateVector2& goal) 
	-> std::optional<MacroPath>
{
	std::map<CoordinateVector2, std::map<int, std::set<int>>> visited;
	std::map<CoordinateVector2, std::map<int, std::map<int, int>>> costToPoint;
	std::map<CoordinateVector2, int> pointHeuristic;
	std::map<CoordinateVector2, PathingDirection> fastestDirectionIntoNode;
	for (auto&&[coordinates, costs] : movementCosts)
	{
		for (int inDirection = static_cast<int>(PathingDirection::NORTH); inDirection <= static_cast<int>(PathingDirection::_COUNT); ++inDirection)
		{
			for (int outDirection = static_cast<int>(PathingDirection::NORTH); outDirection <= static_cast<int>(PathingDirection::_COUNT); ++outDirection)
			{
				costToPoint[coordinates][inDirection][outDirection] = std::numeric_limits<int>::max();
			}
		}
		pointHeuristic[coordinates] = 0;
		fastestDirectionIntoNode[coordinates] = PathingDirection::_COUNT;
	}
	std::set<SortedDirectionalCoordinate> openPoints;
	for (int exitDirection = static_cast<int>(PathingDirection::NORTH); exitDirection <= static_cast<int>(PathingDirection::_COUNT); ++exitDirection)
	{
		if (!movementCosts.at(origin)[static_cast<int>(PathingDirection::_COUNT)][exitDirection]) continue;

		// Cost to enter the current node is always 0
		costToPoint[origin][static_cast<int>(PathingDirection::_COUNT)][exitDirection] = 0;
		openPoints.insert({ origin, 0, pointHeuristic[origin], static_cast<int>(PathingDirection::_COUNT), exitDirection,{} });
	}

	while (!openPoints.empty())
	{
		auto currentNode = *openPoints.begin();
		if (currentNode.m_coordinates == goal && currentNode.m_exit == PathingDirection::_COUNT)
		{
			MacroPath result;
			result.m_totalPathCost = costToPoint[goal]
				[static_cast<int>(currentNode.m_entry)][static_cast<int>(currentNode.m_exit)];
			result.m_path = currentNode.m_previousPath;
			return result;
		}

		if (visited[currentNode.m_coordinates]
			[static_cast<int>(currentNode.m_entry)].count(static_cast<int>(currentNode.m_exit)))
		{
			openPoints.erase(currentNode);
			continue;
		}

		visited[currentNode.m_coordinates]
			[static_cast<int>(currentNode.m_entry)].insert(static_cast<int>(currentNode.m_exit));

		auto& neighborOffset = neighborOffsets[static_cast<int>(currentNode.m_exit)];
		auto neighborCoords = currentNode.m_coordinates + neighborOffset;
		auto neighborCostIter = movementCosts.find(neighborCoords);
		if (neighborCostIter == movementCosts.end())
		{
			openPoints.erase(currentNode);
			continue;
		}
		auto nodeMovementCosts = neighborCostIter->second;
		auto originDirection = static_cast<int>(Opposite(currentNode.m_exit));
		for (int exitDirection = static_cast<int>(PathingDirection::NORTH); exitDirection <= static_cast<int>(PathingDirection::_COUNT); ++exitDirection)
		{
			if (exitDirection == originDirection) continue;
			if (!nodeMovementCosts[originDirection][exitDirection]) continue;

			if (visited[neighborCoords][originDirection].count(exitDirection))
			{
				continue;
			}

			auto costToNeighbor = currentNode.m_costToPoint + *nodeMovementCosts[originDirection][exitDirection];
			if (costToNeighbor >= costToPoint[neighborCoords][originDirection][exitDirection])
			{
				continue;
			}

			fastestDirectionIntoNode[neighborCoords] = Opposite(currentNode.m_exit);
			costToPoint[neighborCoords][originDirection][exitDirection] = static_cast<int>(costToNeighbor);
			openPoints.insert({
				neighborCoords,
				static_cast<int>(costToNeighbor),
				pointHeuristic[neighborCoords],
				originDirection,
				exitDirection,
				currentNode.m_previousPath });
		}

		openPoints.erase(currentNode);
	}
	return std::nullopt;
}