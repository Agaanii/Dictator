//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// ECS/System.cpp
// Systems operate on Components of Entities
// They form the core logic of the engine
// Systems should have no reference to each other.

#include "../Core/typedef.h"

#include "System.h"

System::System()
{
}

System::~System()
{
}

[[gsl::suppress(type.4)]]
bool System::Operate(timeuS microseconds, std::vector<Entity> entities)
{
	
	for (auto& entity : entities)
	{
		if (!OperateSingle(microseconds, entity))
		{
			return false;
		}
	}
	return true;
}
