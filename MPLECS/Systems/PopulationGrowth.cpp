//-----------------------------------------------------------------------------
// All code is property of Dictator Developers Inc
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2018

// Systems/PopulationGrowth.cpp
// Handles promotion of experiences populations, 
// and creation of new populations

#include "../Core/typedef.h"

#include "PopulationGrowth.h"

void PopulationGrowth::ProgramInit() {}
void PopulationGrowth::SetupGameplay() {}

void PopulationGrowth::GainLevels()
{
	using namespace ECS_Core;
	m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_Population>([](
		const ecs::EntityIndex&,
		Components::C_Population& population,
		const Components::C_ResourceInventory&)
	{
		for (auto&& [birthMonth,popSegment] : population.m_populations)
		{
			for (auto&& [skillType,experience]: popSegment.m_specialties)
			{
				// TODO: Manually configured XP thresholds
				s32 xpThreshold = experience.m_level * experience.m_level * 10000;
				if (experience.m_experience >= xpThreshold)
				{
					++experience.m_level;
					experience.m_experience -= xpThreshold;
				}
			}
		}
		return ecs::IterationBehavior::CONTINUE;
	});
}

void PopulationGrowth::AgePopulations()
{
	using namespace ECS_Core;
	auto& timeEntity = m_managerRef.entitiesMatching<Signatures::S_TimeTracker>();
	if (timeEntity.size() == 0) return;
	auto& time = m_managerRef.getComponent<Components::C_TimeTracker>(timeEntity.front());
	m_managerRef.forEntitiesMatching<Signatures::S_Population>([&time](
		const ecs::EntityIndex&,
		Components::C_Population& population,
		const Components::C_ResourceInventory&)
	{
		for (auto&& [birthMonth,popSegment] : population.m_populations)
		{
			auto yearsOld = ((12 * time.m_year + time.m_month) + birthMonth) / 12;
			if (popSegment.m_class == ECS_Core::Components::PopulationClass::CHILDREN
				&& yearsOld >= 15)
			{
				popSegment.m_class = ECS_Core::Components::PopulationClass::WORKERS;
			}
			if (popSegment.m_class == ECS_Core::Components::PopulationClass::WORKERS
				&& yearsOld >= 65)
			{
				popSegment.m_class = ECS_Core::Components::PopulationClass::ELDERS;
			}
		}
		return ecs::IterationBehavior::CONTINUE;
	});
}

void PopulationGrowth::BirthChildren()
{
	using namespace ECS_Core;
	auto& timeEntity = m_managerRef.entitiesMatching<Signatures::S_TimeTracker>();
	if (timeEntity.size() == 0) return;
	auto& time = m_managerRef.getComponent<Components::C_TimeTracker>(timeEntity.front());
	m_managerRef.forEntitiesMatching<Signatures::S_Population>([&time](
		const ecs::EntityIndex&,
		Components::C_Population& population,
		const Components::C_ResourceInventory&) -> ecs::IterationBehavior
	{
		s32 potentialMotherCount{ 0 };
		s32 potentialFatherCount{ 0 };
		s32 boyCount{ 0 };
		s32 girlCount{ 0 };
		for (auto&& [birthMonth,popSegment] : population.m_populations)
		{
			// Age keys are in months
			auto popAge = ((12 * time.m_year + time.m_month) + birthMonth) / 12;
			if (popAge >= 15 && popAge <= 45)
			{
				// Women are of birthing age
				potentialMotherCount += popSegment.m_numWomen;
			}
			if (popAge >= 15 && popAge <= 60)
			{
				// Fathers are still potent
				potentialFatherCount += popSegment.m_numMen;
			}
			if (popAge < 15)
			{
				boyCount += popSegment.m_numMen;
				girlCount += popSegment.m_numWomen;
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

void PopulationGrowth::FeedWomen(
	const ECS_Core::Components::C_TimeTracker& time,
	f64& foodAmount,
	ECS_Core::Components::PopulationSegment& pop)
{
	auto womensFoodRequired = time.m_frameDuration * pop.m_numWomen / (12 * 30);
	if (foodAmount > womensFoodRequired)
	{
		pop.m_womensHealth = min<f64>(1, time.m_frameDuration / 30. + pop.m_womensHealth);
		foodAmount -= womensFoodRequired;
	}
	else
	{
		pop.m_womensHealth = max<f64>(0, pop.m_womensHealth -
			time.m_frameDuration * (womensFoodRequired - foodAmount) / (30 * womensFoodRequired));
		foodAmount = 0;
	}
}

void PopulationGrowth::FeedMen(
	const ECS_Core::Components::C_TimeTracker& time,
	f64& foodAmount,
	ECS_Core::Components::PopulationSegment& pop)
{
	auto mensFoodRequired = time.m_frameDuration * pop.m_numMen / (12 * 30);
	if (foodAmount > mensFoodRequired)
	{
		pop.m_mensHealth = min<f64>(1, time.m_frameDuration / 30. + pop.m_mensHealth);
		foodAmount -= mensFoodRequired;
	}
	else
	{
		pop.m_mensHealth = max<f64>(0, pop.m_mensHealth -
			time.m_frameDuration * (mensFoodRequired - foodAmount) / (30 * mensFoodRequired));
		foodAmount = 0;
	}
}

void PopulationGrowth::ConsumeResources()
{
	using namespace ECS_Core;
	auto& timeEntity = m_managerRef.entitiesMatching<Signatures::S_TimeTracker>();
	if (timeEntity.size() == 0) return;
	auto& time = m_managerRef.getComponent<Components::C_TimeTracker>(timeEntity.front());
	m_managerRef.forEntitiesMatching<Signatures::S_Population>([&time, this](
		const ecs::EntityIndex&,
		Components::C_Population& population,
		Components::C_ResourceInventory& resources) -> ecs::IterationBehavior
	{
		auto& foodAmount = resources.m_collectedYields[Components::Yields::FOOD];

		// Workers eat first
		// Then children
		// Then elders
		// Younger before older
		for (auto&& [birthMonth,pop] : population.m_populations)
		{
			if (pop.m_class != Components::PopulationClass::WORKERS)
			{
				continue;
			}

			// women eat before men
			FeedWomen(time, foodAmount, pop);
			FeedMen(time, foodAmount, pop);
		}
		for (auto&& [birthMonth,pop] : population.m_populations)
		{
			if (pop.m_class != Components::PopulationClass::CHILDREN)
			{
				continue;
			}
			FeedWomen(time, foodAmount, pop);
			FeedMen(time, foodAmount, pop);
		}
		for (auto&& [birthMonth,pop] : population.m_populations)
		{
			if (pop.m_class != Components::PopulationClass::ELDERS)
			{
				continue;
			}
			FeedWomen(time, foodAmount, pop);
			FeedMen(time, foodAmount, pop);
		}

		return ecs::IterationBehavior::CONTINUE;
	});
}

void PopulationGrowth::CauseNaturalDeaths()
{
	using namespace ECS_Core;
	auto& timeEntity = m_managerRef.entitiesMatching<Signatures::S_TimeTracker>();
	if (timeEntity.size() == 0) return;
	auto& time = m_managerRef.getComponent<Components::C_TimeTracker>(timeEntity.front());
	m_managerRef.forEntitiesMatching<Signatures::S_Population>([&time](
		const ecs::EntityIndex&,
		Components::C_Population& population,
		const Components::C_ResourceInventory&) -> ecs::IterationBehavior
	{
		// Chance of dying = % of a year since previous frame
		// Multiplied by population age
		f64 frameYearPercent = time.m_frameDuration / (12 * 30);
		std::vector<ECS_Core::Components::PopulationKey> deadSegments;
		for (auto&& [birthMonth,popSegment] : population.m_populations)
		{
			// Age keys are in months
			auto popAge = ((12 * time.m_year + time.m_month) + birthMonth) / 12;

			// Healthiest people are ~25
			auto distanceFromHealth = max<int>(1, abs(popAge - 25));
			auto womensDeathChance = frameYearPercent
				* (1 - (popSegment.m_womensHealth / 2))
				* popSegment.m_numWomen
				* distanceFromHealth / 150;
			auto randDouble = RandDouble();
			auto femaleDeathCount = min<int>(popSegment.m_numWomen, static_cast<int>(womensDeathChance / randDouble));
			auto mensDeathChance = frameYearPercent
				* (1 - (popSegment.m_mensHealth / 2))
				* popSegment.m_numMen
				* distanceFromHealth / 150;
			randDouble = RandDouble();
			auto maleDeathCount = min<int>(popSegment.m_numMen, static_cast<int>(mensDeathChance / randDouble));

			if (maleDeathCount < 0)
			{
				maleDeathCount = 0;
			}
			if (femaleDeathCount < 0)
			{
				femaleDeathCount = 0;
			}
			popSegment.m_numMen -= maleDeathCount;
			popSegment.m_numWomen -= femaleDeathCount;

			if (popSegment.m_numMen <= 0
				&& popSegment.m_numWomen <= 0)
			{
				deadSegments.emplace_back(birthMonth);
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
		GainLevels();
		{
			auto&& timeEntity = m_managerRef.getComponent<ECS_Core::Components::C_TimeTracker>(
				m_managerRef.entitiesMatching<ECS_Core::Signatures::S_TimeTracker>().front());
			if (timeEntity.IsNewMonth())
			{
				AgePopulations();
				BirthChildren();
			}
			ConsumeResources();
			CauseNaturalDeaths();
		}
		return;
	}
}

bool PopulationGrowth::ShouldExit()
{
	return false;
}

DEFINE_SYSTEM_INSTANTIATION(PopulationGrowth);