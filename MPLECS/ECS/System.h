//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Core/System.h
// Base class from which all System code should derive
// As well as System registration to the game loop

#pragma once

#include "../Core/typedef.h"
#include "ECS.h"

#include <memory>

enum class GameLoopPhase
{
	// Create any per-frame objects that need to be pre-allocated
	PREPARATION,

	// Receive user input. Don't yet act on it.
	INPUT,

	// Take initial actions. Send commands, etc.
	ACTION,

	// Process results of actions which have been taken
	ACTION_RESPONSE,

	// Draw current game state and UI
	RENDER,

	// Destroy any per-frame objects which need it
	// This is the phase where it is safe to kill Entities
	CLEANUP
};

class SystemBase
{
public:
	SystemBase();
	virtual ~SystemBase() {}
	virtual void ProgramInit() = 0;
	virtual void SetupGameplay() = 0;
	virtual void Operate(GameLoopPhase phase, const timeuS& frameDuration) = 0;
	virtual bool ShouldExit() = 0;
protected:
	ECS_Core::Manager& m_managerRef;
};
