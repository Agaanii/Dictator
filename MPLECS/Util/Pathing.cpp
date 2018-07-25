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

auto Pathing::PathingSide::Opposite(Enum d) -> Enum
{
	{
		switch (d)
		{
		case NORTH: return SOUTH;
		case SOUTH: return NORTH;
		case EAST: return WEST;
		case WEST: return EAST;
		}
		return _COUNT;
	}
}
