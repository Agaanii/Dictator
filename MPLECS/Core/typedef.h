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

struct CartesianVector3
{
	f64 m_x{ 0 };
	f64 m_y{ 0 };
	f64 m_z{ 0 };

	CartesianVector3& operator+=(const CartesianVector3& other)
	{
		m_x += other.m_x;
		m_y += other.m_y;
		m_z += other.m_z;
		return *this;
	}

	CartesianVector3 operator+(const CartesianVector3& other) const
	{
		return CartesianVector3(*this) += other;
	}

	CartesianVector3 operator*(f64 factor) const
	{
		auto copy(*this);
		copy.m_x *= factor;
		copy.m_y *= factor;
		copy.m_z *= factor;
		return copy;
	}
};

struct CartesianVector2
{
	CartesianVector2() {}
	constexpr CartesianVector2(f64 x, f64 y) : m_x(x), m_y(y) {}
	constexpr CartesianVector2(int x, int y) : m_x(static_cast<f64>(x)), m_y(static_cast<f64>(y)){}
	f64 m_x{ 0 };
	f64 m_y{ 0 };

	CartesianVector2& operator +=(const CartesianVector2& other)
	{
		m_x += other.m_x;
		m_y += other.m_y;
		return *this;
	}

	CartesianVector2 operator+(const CartesianVector2& other) const
	{
		return CartesianVector2(*this) += other;
	}

	CartesianVector2& operator -=(const CartesianVector2& other)
	{
		m_x -= other.m_x;
		m_y -= other.m_y;
		return *this;
	}

	CartesianVector2 operator-(const CartesianVector2& other) const
	{
		return CartesianVector2(*this) -= other;
	}

	CartesianVector2 operator*(f64 factor) const
	{
		auto copy(*this);
		copy.m_x *= factor;
		copy.m_y *= factor;
		return copy;
	}
};