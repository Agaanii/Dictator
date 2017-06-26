//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// ECS/Entity.h
// An entity is a Noun of the game engine
// It has a set of components on which Systems can act
#pragma once

#include "../Core/typedef.h"

#include "Component.h"
#include "../Components/ComponentTypes.h"

#include <memory>
#include <vector>

class Entity
{
public:
	Entity();
	~Entity();

private:
	std::unique_ptr<Component> m_components[(s64)ComponentTypes::_COUNT];
};
