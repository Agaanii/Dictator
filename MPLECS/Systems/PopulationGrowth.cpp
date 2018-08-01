//-----------------------------------------------------------------------------
// All code is property of Dictator Developers Inc
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2018

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
	using namespace ECS_Core;
	manager.forEntitiesMatching<ECS_Core::Signatures::S_Population>([](
		const ecs::EntityIndex&,
		Components::C_Population& population)
	{
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
		return ecs::IterationBehavior::CONTINUE;
	});
}

void AgePopulations(ECS_Core::Manager& manager)
{
	using namespace ECS_Core;
	auto& timeEntity = manager.entitiesMatching<Signatures::S_TimeTracker>();
	if (timeEntity.size() == 0) return;
	auto& time = manager.getComponent<Components::C_TimeTracker>(timeEntity.front());
	manager.forEntitiesMatching<Signatures::S_Population>([&time](
		const ecs::EntityIndex&,
		Components::C_Population& population)
	{
		for (auto&& popSegment : population.m_populations)
		{
			auto yearsOld = ((12 * time.m_year + time.m_month) + popSegment.first) / 12;
			if (popSegment.second.m_class == ECS_Core::Components::PopulationClass::CHILDREN 
				&& yearsOld >= 15)
			{
				popSegment.second.m_class = ECS_Core::Components::PopulationClass::WORKERS;
			}
			if (popSegment.second.m_class == ECS_Core::Components::PopulationClass::WORKERS
				&& yearsOld >= 65)
			{
				popSegment.second.m_class = ECS_Core::Components::PopulationClass::ELDERS;
			}
		}
		return ecs::IterationBehavior::CONTINUE;
	});
}

void BirthChildren(ECS_Core::Manager& manager)
{
	using namespace ECS_Core;
	auto& timeEntity = manager.entitiesMatching<Signatures::S_TimeTracker>();
	if (timeEntity.size() == 0) return;
	auto& time = manager.getComponent<Components::C_TimeTracker>(timeEntity.front());
	manager.forEntitiesMatching<Signatures::S_Population>([&time](
		const ecs::EntityIndex&,
		Components::C_Population& population) -> ecs::IterationBehavior
	{
		s32 potentialMotherCount{ 0 };
		s32 potentialFatherCount{ 0 };
		s32 boyCount{ 0 };
		s32 girlCount{ 0 };
		for (auto&& popSegment : population.m_populations)
		{
			// Age keys are in months
			auto popAge = ((12 * time.m_year + time.m_month) + popSegment.first) / 12;
			if (popAge >= 15 && popAge <= 45)
			{
				// Women are of birthing age
				potentialMotherCount += popSegment.second.m_numWomen;
			}
			if (popAge >= 15 && popAge <= 60)
			{
				// Fathers are still potent
				potentialFatherCount += popSegment.second.m_numMen;
			}
			if (popAge < 15)
			{
				boyCount += popSegment.second.m_numMen;
				girlCount += popSegment.second.m_numWomen;
			}
		}
		// Approximately 1 child every 4 years
		f64 childFloat = 1. * min(potentialMotherCount, potentialFatherCount * 2) / 36;
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
				return ecs::IterationBehavior::CONTINUE;
			}
		}
		s32 maleChildCount{ 0 };
		s32 femaleChildCount{ 0 };
		if (boyCount > girlCount)
		{
			maleChildCount = childCount / 2;
			femaleChildCount = childCount - maleChildCount;
		}
		else
		{
			femaleChildCount = childCount / 2;
			maleChildCount = childCount - femaleChildCount;
		}
		auto& newPopulation = population.m_populations[-12 * time.m_year - time.m_month];
		newPopulation.m_numMen = maleChildCount;
		newPopulation.m_numWomen = femaleChildCount;
		return ecs::IterationBehavior::CONTINUE;
	});
}

void CauseNaturalDeaths(ECS_Core::Manager& manager)
{

	using namespace ECS_Core;
	auto& timeEntity = manager.entitiesMatching<Signatures::S_TimeTracker>();
	if (timeEntity.size() == 0) return;
	auto& time = manager.getComponent<Components::C_TimeTracker>(timeEntity.front());
	manager.forEntitiesMatching<Signatures::S_Population>([&time](
		const ecs::EntityIndex&,
		Components::C_Population& population) -> ecs::IterationBehavior
	{
		// Chance of dying = % of a year since previous frame
		// Multiplied by population age
		f64 frameYearPercent = time.m_frameDuration / (12 * 30);
		std::vector<ECS_Core::Components::PopulationKey> deadSegments;
		for (auto&& popSegment : population.m_populations)
		{
			// Age keys are in months
			auto popAge = ((12 * time.m_year + time.m_month) + popSegment.first) / 12;

			// Healthiest people are ~25
			auto distanceFromHealth = max<int>(1, abs(popAge - 25));
			auto deathChance = frameYearPercent * (popSegment.second.m_numMen + popSegment.second.m_numWomen) * distanceFromHealth / 300;
			auto randDouble = RandDouble();
			auto deathCount = static_cast<int>(deathChance / randDouble);
			if (deathCount <= 0)
			{
				continue;
			}

			auto maleDeathCount = min<int>(deathCount - (deathCount / 2), popSegment.second.m_numMen);
			auto femaleDeathCount = min<int>(deathCount - maleDeathCount, popSegment.second.m_numWomen);

			popSegment.second.m_numMen -= maleDeathCount;
			popSegment.second.m_numWomen -= femaleDeathCount;

			if (popSegment.second.m_numMen <= 0
				&& popSegment.second.m_numWomen <= 0)
			{
				deadSegments.emplace_back(popSegment.first);
			}
		}
		for (auto&& deadKey : deadSegments)
		{
			population.m_populations.erase(deadKey);
		}
		return ecs::IterationBehavior::CONTINUE;
	});
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
			CauseNaturalDeaths(m_managerRef);
		}
		return;
	}
}

bool PopulationGrowth::ShouldExit()
{
	return false;
}

DEFINE_SYSTEM_INSTANTIATION(PopulationGrowth);