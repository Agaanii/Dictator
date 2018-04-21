//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Systems/UnitDeath.cpp
// Runs through units that have health, kills the ones that don't have any left.

#include "../Core/typedef.h"

#include "Systems.h"

#include "../ECS/System.h"
#include "../ECS/ECS.h"

void KillDyingUnits(ECS_Core::Manager& manager);

void UnitDeath::ProgramInit() {}
void UnitDeath::SetupGameplay() {}

void UnitDeath::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	switch (phase)
	{
	case GameLoopPhase::PREPARATION:
	case GameLoopPhase::INPUT:
	case GameLoopPhase::ACTION:
	case GameLoopPhase::ACTION_RESPONSE:
	case GameLoopPhase::RENDER:
		return;

	case GameLoopPhase::CLEANUP:
		KillDyingUnits(m_managerRef);
		break;
	}
}

void KillDyingUnits(ECS_Core::Manager& manager)
{
	manager.forEntitiesMatching<ECS_Core::Signatures::S_Living>(
		[&manager](ecs::EntityIndex mI,
			const ECS_Core::Components::C_Health& health)
	{
		if (health.m_currentHealth <= 0)
		{
			manager.kill(mI);
		}
	});
}

bool UnitDeath::ShouldExit()
{
	return false;
}

DEFINE_SYSTEM_INSTANTIATION(UnitDeath);
