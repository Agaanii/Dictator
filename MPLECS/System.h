//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Core/System.h
// Base class from which all System code should derive
// As well as System registration to the game loop

#pragma once

#include "Core/typedef.h"
#include "ECS.h"

#include <memory>


enum class GameLoopPhase
{
	PREPARATION,
	INPUT,
	ACTION,
	RENDER,
	CLEANUP
};

class SystemBase
{
public:
	SystemBase();
	virtual ~SystemBase() {}

	virtual void Operate(GameLoopPhase phase, const timeuS& frameDuration) = 0;
	virtual bool ShouldExit() = 0;
protected:
	ECS_Core::Manager& m_managerRef;
};

void RegisterSystem(std::unique_ptr<SystemBase>&& system);

template<typename System, typename... Ts>
struct SystemRegistrar
{
	SystemRegistrar(Ts&&... args) 
	{
		RegisterSystem(static_cast<std::unique_ptr<SystemBase>>(std::make_unique<System>(args...)));
	}
};
