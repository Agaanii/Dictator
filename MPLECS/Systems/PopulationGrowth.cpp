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

void AgePopulations(ECS_Core::Manager& manager)
{
	for (auto&& govHandle : manager.entitiesMatching<ECS_Core::Signatures::S_Governor>())
	{
		// Potential for optimization here:
		// We need to remove and re-insert every element
		// If we change things so that key is birth month index, and calculate age
		// We don't need to move them all
		// That would mean we need to do reverse iteration though, to go for increasing age
		// Or could use -birthMonth as key
		auto& population = manager.getComponent<ECS_Core::Components::C_Population>(govHandle);
		for (auto popSegment = population.m_populations.rbegin();
			popSegment != population.m_populations.rend();
			++popSegment)
		{
			auto segmentCopy = popSegment->second;
			auto newAge = popSegment->first + 1;
			population.m_populations.emplace(newAge, segmentCopy);
			population.m_populations.erase(popSegment->first);
		}
		for (auto&& popSegment : population.m_populations)
		{
			if (popSegment.second.m_class == ECS_Core::Components::PopulationClass::CHILDREN 
				&& popSegment.first >= 15 * 12)
			{
				popSegment.second.m_class = ECS_Core::Components::PopulationClass::WORKERS;
			}
			if (popSegment.second.m_class == ECS_Core::Components::PopulationClass::WORKERS
				&& popSegment.first >= 65 * 12)
			{
				popSegment.second.m_class = ECS_Core::Components::PopulationClass::ELDERS;
			}
		}
	}
}

void BirthChildren(ECS_Core::Manager& manager)
{
	for (auto&& govHandle : manager.entitiesMatching<ECS_Core::Signatures::S_Governor>())
	{
		auto& population = manager.getComponent<ECS_Core::Components::C_Population>(govHandle);
		s32 potentialMotherCount{ 0 };
		s32 potentialFatherCount{ 0 };
		for (auto&& popSegment : population.m_populations)
		{
			// Age keys are in months
			auto popAge = popSegment.first / 12;
			if (popAge >= 15 && popAge <= 45)
			{
				// Women are of birthing age
				potentialMotherCount += popSegment.second.m_numWomen;
			}
			if (popAge >= 15 && popAge <= 60)
			{
				potentialFatherCount += popSegment.second.m_numMen;
			}
		}
		f64 childFloat = 1. * min(potentialMotherCount, potentialFatherCount * 2) / 12;
		s32 childCount = static_cast<s32>(round(childFloat));
		if (childCount == 0)
		{
			// Take the value of childFloat as a probability of a child, rather than a quantity
			if (RandDouble() < childFloat)
			{
				childCount = 1;
			}
			else
			{
				continue;
			}
		}
		auto maleChildCount = childCount / 2;
		auto femaleChildCount = childCount - maleChildCount;
		
		auto& newPopulation = population.m_populations[0];
		newPopulation.m_numMen = maleChildCount;
		newPopulation.m_numWomen = femaleChildCount;
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

		{
			auto&& timeEntity = m_managerRef.getComponent<ECS_Core::Components::C_TimeTracker>(
				m_managerRef.entitiesMatching<ECS_Core::Signatures::S_TimeTracker>().front());
			if (timeEntity.IsNewMonth())
			{
				AgePopulations(m_managerRef);
				BirthChildren(m_managerRef);
			}

		}
		return;
	}
}

bool PopulationGrowth::ShouldExit()
{
	return false;
}

DEFINE_SYSTEM_INSTANTIATION(PopulationGrowth);