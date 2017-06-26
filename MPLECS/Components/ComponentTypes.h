//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Components/ComponentTypes.h
// Enumerators containing all defined:
// * Components
// * Filters
// * Standard Component Combinations
#pragma once

enum class ComponentTypes
{
	POSITION,
	VELOCITY,
	GRAPHICS,

	// Count enum, keep this last
	_COUNT
};

enum class FilterTypes
{
	PHYSICS,
	// Count enum, keep this last
	_COUNT
};