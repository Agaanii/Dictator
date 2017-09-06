//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

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
		return (left.m_airDistToTarget + left.m_costToPoint) <
			(right.m_airDistToTarget + right.m_costToPoint);
	}
};