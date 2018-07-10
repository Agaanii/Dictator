//-----------------------------------------------------------------------------
// All code is property of Dictator Developers Inc
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2018

// Systems/SystemTemplate.cpp
// The boilerplate system code, to ease new system creation

#include "../Core/typedef.h"

#include "Systems.h"

#include "../ECS/System.h"
#include "../ECS/ECS.h"

void SystemTemplate::ProgramInit() {}
void SystemTemplate::SetupGameplay() {}

void SystemTemplate::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	switch (phase)
	{
	case GameLoopPhase::PREPARATION:
	case GameLoopPhase::INPUT:
	case GameLoopPhase::ACTION:
	case GameLoopPhase::ACTION_RESPONSE:
	case GameLoopPhase::RENDER:
	case GameLoopPhase::CLEANUP:
		return;
	}
}

bool SystemTemplate::ShouldExit()
{
	return false;
}

DEFINE_SYSTEM_INSTANTIATION(SystemTemplate);