//-----------------------------------------------------------------------------
// All code is property of Dictator Developers Inc
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2018

// Systems/Education.cpp
// When populations have elders,
// They train children
// Older children take priority

#include "../Core/typedef.h"

#include "Systems.h"

#include "../ECS/System.h"
#include "../ECS/ECS.h"

void TeachChildren(ECS_Core::Manager& manager)
{
	// Get current time
	// Assume the first entity is the one that has a valid time
	auto timeEntities = manager.entitiesMatching<ECS_Core::Signatures::S_TimeTracker>();
	if (timeEntities.size() == 0)
	{
		return;
	}
	const auto& time = manager.getComponent<ECS_Core::Components::C_TimeTracker>(timeEntities.front());
	manager.forEntitiesMatching<ECS_Core::Signatures::S_Population>([&time](
		const ecs::EntityIndex&,
		ECS_Core::Components::C_Population& population,
		const ECS_Core::Components::C_ResourceInventory&)
	{
		// Order elders by by highest-to-lowest total skill
		// order children by eldest-to-youngest
		// Each elder can teach up to X children (default 5)
		// Total experience gain across all children distributed per child population
		using namespace ECS_Core::Components;
		struct ElderReference
		{
			ElderReference(s64 birthMonth, PopulationSegment* segment)
				: m_birthMonth(birthMonth)
				, m_pop(segment)
			{
				for (auto&&[skill, experience] : m_pop->m_specialties)
				{
					// TODO: Read total XP thresholds instead of calculating
					// Sum(i^2) from 1 to n = 
					//		i(i+1)(2i + 1) / 6
					f64 totalSkillXP = experience.m_level * (experience.m_level + 1)
						* (2 * experience.m_level + 1) * 10000 / 6;
					totalSkillXP += experience.m_experience;
					m_totalXp += totalSkillXP;
				}
			}
			s64 m_birthMonth;
			PopulationSegment* m_pop;
			f64 m_totalXp{ 0 };
		};

		std::vector<ElderReference> elders;
		for (auto&&[birthMonth, elderPop] : population.m_populations)
		{
			if (elderPop.m_class != PopulationClass::ELDERS)
			{
				continue;
			}
			elders.emplace_back(birthMonth, &elderPop);
		}

		std::sort(elders.begin(), elders.end(),
			[](auto& left, auto& right) {
			if (left.m_totalXp < right.m_totalXp)
			{
				return true;
			}
			if (left.m_totalXp > right.m_totalXp)
			{
				return false;
			}
			return left.m_birthMonth < right.m_birthMonth;
		});

		if (elders.size() == 0)
		{
			return ecs::IterationBehavior::CONTINUE;
		}
		auto elderIter = elders.begin();
		auto remainingTeachCount = 5 * (elderIter->m_pop->m_numMen + elderIter->m_pop->m_numWomen);
		for (auto&&[birthMonth, childPop] : reverse(population.m_populations))
		{
			if (elderIter == elders.end())
			{
				break;
			}
			if (childPop.m_class != PopulationClass::CHILDREN)
			{
				continue;
			}
			auto childrenRemaining = childPop.m_numMen + childPop.m_numWomen;
			while (childrenRemaining > 0 && elderIter != elders.end())
			{
				auto usedTeachCount = min(remainingTeachCount,
					childrenRemaining);

				childrenRemaining -= usedTeachCount;
				remainingTeachCount -= usedTeachCount;

				for (auto&&[skill, experience] : elderIter->m_pop->m_specialties)
				{
					if (experience.m_level == 1
						&& experience.m_experience == 0)
					{
						continue;
					}
					auto& childExperience = childPop.m_specialties[skill];
					if (childExperience.m_level < experience.m_level)
					{
						// Learning normally
						childExperience.m_experience += time.m_frameDuration
							* (experience.m_level - childExperience.m_level);
					}
					else
					{
						// Researching with guidance
						// Slower the higher level the child is over the elder
						childExperience.m_experience += time.m_frameDuration /
							(2 + childExperience.m_level - experience.m_level);
					}
				}

				if ((remainingTeachCount) <= 0)
				{
					if (++elderIter == elders.end())
					{
						break;
					}
					else
					{
						remainingTeachCount = 5 * (elderIter->m_pop->m_numMen
							+ elderIter->m_pop->m_numWomen);
					}
				}
			}
		}
		return ecs::IterationBehavior::CONTINUE;
	});
}

void Education::ProgramInit() {}
void Education::SetupGameplay() {}

void Education::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	switch (phase)
	{
	case GameLoopPhase::PREPARATION:
	case GameLoopPhase::INPUT:
		break;
	case GameLoopPhase::ACTION:
		TeachChildren(m_managerRef);
		break;
	case GameLoopPhase::ACTION_RESPONSE:
	case GameLoopPhase::RENDER:
	case GameLoopPhase::CLEANUP:
		return;
	}
}

bool Education::ShouldExit()
{
	return false;
}

DEFINE_SYSTEM_INSTANTIATION(Education);