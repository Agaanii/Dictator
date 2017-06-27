//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Core/typedef.h
// A set of shorter names for common datatypes

#pragma once

#include <chrono>

using u8 = unsigned char;
using s8 = char;
using u16 = unsigned short;
using s16 = short;
using u32 = unsigned int;
using s32 = int;
using u64 = unsigned long long;
using s64 = long long;

using f32 = float;
using f64 = double;

using timeuS = s64;

struct CartesianVector
{
	f64 m_x{ 0 };
	f64 m_y{ 0 };
	f64 m_z{ 0 };

	CartesianVector& operator+=(const CartesianVector& other)
	{
		m_x += other.m_x;
		m_y += other.m_y;
		m_z += other.m_z;
		return *this;
	}

	CartesianVector operator+(const CartesianVector& other) const
	{
		CartesianVector copy(*this);
		copy += other;
		return copy;
	}

	CartesianVector operator*(f64 factor) const
	{
		CartesianVector copy(*this);
		copy.m_x *= factor;
		copy.m_y *= factor;
		copy.m_z *= factor;
		return copy;
	}
};