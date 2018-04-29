//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Systems/PopulationGrowth.cpp
// Handles promotion of experiences populations, 
// and creation of new populations

#include "../Core/typedef.h"

#include "Systems.h"

#include "../ECS/System.h"
#include "../ECS/ECS.h"

void PopulationGrowth::SetupGameplay() {}

void PopulationGrowth::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	switch (phase)
	{
		// TODO System: run skill upgrade checks
		// Roll against % experience, advance. reduce XP% and try again if success
		// Reduce new location's experience fractionally, increase pop
		// Reduce old location's experience fractionally, reduce pop
	case GameLoopPhase::PREPARATION:
	case GameLoopPhase::INPUT:
	case GameLoopPhase::ACTION:
	case GameLoopPhase::ACTION_RESPONSE:
	case GameLoopPhase::RENDER:
	case GameLoopPhase::CLEANUP:
		return;
	}
}

bool PopulationGrowth::ShouldExit()
{
	return false;
}

DEFINE_SYSTEM_INSTANTIATION(PopulationGrowth);