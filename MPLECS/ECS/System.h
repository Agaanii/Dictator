//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// ECS/System.h
// Systems operate on Components of Entities
// They form the core logic of the engine
// Systems should have no reference to each other.
// 

#pragma once

#include "Entity.h"

#include <vector>

class System
{
public:
	System();
	~System();

	void Operate(s64 microseconds, std::vector<Entity> entities);
	virtual void Operate(s64 microseconds, Entity& entity) = 0;
};
