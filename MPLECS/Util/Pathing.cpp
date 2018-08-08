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