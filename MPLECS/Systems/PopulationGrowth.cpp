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

void PopulationGrowth::ProgramInit() {}
void PopulationGrowth::SetupGameplay() {}

void GainLevels(ECS_Core::Manager& manager)
{
	auto& governorList = manager.entitiesMatching<ECS_Core::Signatures::S_Governor>();
	for (auto&& govHandle : governorList)
	{
		auto& population = manager.getComponent<ECS_Core::Components::C_Population>(govHandle);
		for (auto&& popSegment : population.m_populations)
		{
			for (auto&& specialty : popSegment.second.m_specialties)
			{
				// TODO: Manually configured XP thresholds
				s32 xpThreshold = specialty.second.m_level * specialty.second.m_level * 10000;
				if (specialty.second.m_experience >= xpThreshold)
				{
					++specialty.second.m_level;
					specialty.second.m_experience -= xpThreshold;
				}
			}
		}
	}
}

void PopulationGrowth::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	switch (phase)
	{
	case GameLoopPhase::PREPARATION:
	case GameLoopPhase::INPUT:
	case GameLoopPhase::ACTION_RESPONSE:
	case GameLoopPhase::RENDER:
	case GameLoopPhase::CLEANUP:
		return;
	case GameLoopPhase::ACTION:
		GainLevels(m_managerRef);
		return;
	}
}

bool PopulationGrowth::ShouldExit()
{
	return false;
}

DEFINE_SYSTEM_INSTANTIATION(PopulationGrowth);