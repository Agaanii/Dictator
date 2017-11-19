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

enum class Direction
{
	NORTH,
	SOUTH,
	EAST,
	WEST
};

template<typename NUM_TYPE>
struct CartesianVector3
{
	NUM_TYPE m_x{ 0 };
	NUM_TYPE m_y{ 0 };
	NUM_TYPE m_z{ 0 };

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

	CartesianVector3 operator*(NUM_TYPE factor) const
	{
		auto copy(*this);
		copy.m_x *= factor;
		copy.m_y *= factor;
		copy.m_z *= factor;
		return copy;
	}

	template<typename U>
	CartesianVector3<U> cast()
	{
		return CartesianVector3<U>(
			static_cast<U>(m_x),
			static_cast<U>(m_y),
			static_cast<U>(m_z));
	}
};

template<typename NUM_TYPE>
struct CartesianVector2
{
	CartesianVector2() {}
	constexpr CartesianVector2(NUM_TYPE x, NUM_TYPE y) : m_x(x), m_y(y) {}
	NUM_TYPE m_x{ 0 };
	NUM_TYPE m_y{ 0 };

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

	CartesianVector2 operator*(NUM_TYPE factor) const
	{
		auto copy(*this);
		copy.m_x *= factor;
		copy.m_y *= factor;
		return copy;
	}

	CartesianVector2 operator/(const NUM_TYPE divisor) const
	{
		auto copy(*this);
		copy.m_x /= divisor;
		copy.m_y /= divisor;
		return copy;
	}

	template<typename U>
	CartesianVector2<U> cast()
	{
		return CartesianVector2<U>(
			static_cast<U>(m_x),
			static_cast<U>(m_y));
	}

	NUM_TYPE MagnitudeSq() const { return m_x * m_x + m_y * m_y; }

	bool operator==(const CartesianVector2& other) const { return m_x == other.m_x && m_y == other.m_y; }
	bool operator>(const CartesianVector2& other) const {
		if (*this == other) return false;
		if (m_x + m_y > other.m_x + other.m_y) return true;
		if (m_x + m_y < other.m_x + other.m_y) return false;
		return m_x > other.m_x;
	};
	bool operator<(const CartesianVector2& other) const {
		if (*this == other) return false;
		if (m_x + m_y < other.m_x + other.m_y) return true;
		if (m_x + m_y > other.m_x + other.m_y) return false;
		return m_x < other.m_x;
	};
	bool operator!=(const CartesianVector2& other) const { return !(*this == other); }
	bool operator<=(const CartesianVector2& other) const { return !(*this > other); }
	bool operator>=(const CartesianVector2& other) const { return !(*this < other); }
};

namespace std
{
	template<typename NUM_TYPE>
	bool operator<(const CartesianVector2<NUM_TYPE>& left, const CartesianVector2<NUM_TYPE>& right)
	{
		if (left.m_x == right.m_x && left.m_y == right.m_y) return false;
		if (left.m_x + left.m_y < right.m_x + right.m_y) return true;
		if (left.m_x + left.m_y > right.m_x + right.m_y) return false;
		return left.m_x < right.m_x;
	}
}

using CoordinateVector2 = CartesianVector2<s64>;

struct TilePosition
{
	TilePosition() {}
	TilePosition(const CoordinateVector2& qc, const CoordinateVector2& sc, const CoordinateVector2& c)
		: m_quadrantCoords(qc)
		, m_sectorCoords(sc)
		, m_coords(c)
	{ }

	template<typename NUM_TYPE>
	TilePosition(NUM_TYPE qx, NUM_TYPE qy, NUM_TYPE sx, NUM_TYPE sy, NUM_TYPE x, NUM_TYPE y)
		: m_quadrantCoords(qx, qy)
		, m_sectorCoords(sx, sy)
		, m_coords(x, y)
	{}
	CoordinateVector2 m_quadrantCoords;
	CoordinateVector2 m_sectorCoords;
	CoordinateVector2 m_coords;

	bool operator==(const TilePosition& other) const
	{
		return m_quadrantCoords == other.m_quadrantCoords
			&& m_sectorCoords == other.m_sectorCoords
			&& m_coords == other.m_coords;
	}

	bool operator<(const TilePosition& other) const
	{
		if (m_quadrantCoords < other.m_quadrantCoords) return true;
		if (other.m_quadrantCoords < m_quadrantCoords) return false;
		if (m_sectorCoords < other.m_sectorCoords) return true;
		if (other.m_sectorCoords < m_sectorCoords) return false;
		if (m_coords < other.m_coords) return true;
		return false;
	}

	TilePosition& operator-=(const TilePosition& other);
	TilePosition operator-(const TilePosition& other) const
	{
		return TilePosition(*this) -= other;
	}

	TilePosition& operator+=(const TilePosition& other);
	TilePosition operator+(const TilePosition& other) const
	{
		return TilePosition(*this) += other;
	}
};

template<class T>
T min(T&& a, T&& b)
{
	return a < b ? a : b;
}