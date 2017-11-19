//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Systems/Government.cpp
// The best system of government ever created

#include "../Core/typedef.h"

#include "Systems.h"

#include "../ECS/System.h"
#include "../ECS/ECS.h"

std::optional<ecs::EntityIndex> s_playerGovernorEntity;

void Government::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	switch (phase)
	{
	case GameLoopPhase::PREPARATION:
		if (!s_playerGovernorEntity)
		{
			s_playerGovernorEntity = m_managerRef.createIndex();
			m_managerRef.addComponent<ECS_Core::Components::C_Realm>(*s_playerGovernorEntity);
			m_managerRef.addComponent<ECS_Core::Components::C_ResourceInventory>(*s_playerGovernorEntity);
		}
	case GameLoopPhase::INPUT:
	case GameLoopPhase::ACTION:
	case GameLoopPhase::ACTION_RESPONSE:
	case GameLoopPhase::RENDER:
	case GameLoopPhase::CLEANUP:
		return;
	}
}

bool Government::ShouldExit()
{
	return false;
}

DEFINE_SYSTEM_INSTANTIATION(Government);