//-----------------------------------------------------------------------------
// All code is property of Dictator Developers Inc
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2018

// Systems/Government.cpp
// The best system of government ever created

#include "../Core/typedef.h"

#include "Government.h"

#include "../Util/WorkerStructs.h"

#include <algorithm>

void BeginBuildingConstruction(ECS_Core::Manager & manager, const ecs::EntityIndex& ghostEntity)
{
	manager.addComponent<ECS_Core::Components::C_BuildingConstruction>(ghostEntity).m_placingGovernor =
		manager.getComponent<ECS_Core::Components::C_BuildingGhost>(ghostEntity).m_placingGovernor;
	manager.delComponent<ECS_Core::Components::C_BuildingGhost>(ghostEntity);
	auto& drawable = manager.getComponent<ECS_Core::Components::C_SFMLDrawable>(ghostEntity);
	for (auto&[p, buildings] : drawable.m_drawables[ECS_Core::Components::DrawLayer::BUILDING])
	{
		for (auto&& building : buildings)
		{
			sf::Shape* shapeDrawable = dynamic_cast<sf::Shape*>(building.m_graphic.get());
			if (shapeDrawable)
				shapeDrawable->setFillColor(sf::Color(128, 128, 128, 26));
		}
	}
}

std::map<int, ECS_Core::Components::YieldBuckets> s_buildingCosts
{
	{
		0, 
		{
			{ECS_Core::Components::Yields::FOOD, 50},
			{ECS_Core::Components::Yields::WOOD, 50}
		}
	}
};

bool CreateBuildingGhost(ECS_Core::Manager & manager, ecs::Impl::Handle &governor, TilePosition & position, int buildingType)
{
	// Validate inputs
	if (!manager.hasComponent<ECS_Core::Components::C_ResourceInventory>(governor))
	{
		return false;
	}
	using namespace ECS_Core;
	bool governorHasPending{ false };
	// Look through current ghosts, make sure this governor doesn't yet have one active
	manager.forEntitiesMatching<Signatures::S_PlannedBuildingPlacement>(
		[&governor, &governorHasPending](
			const ecs::EntityIndex&,
			const Components::C_BuildingDescription&,
			const Components::C_TilePosition&,
			const Components::C_BuildingGhost& ghost)
	{
		if (ghost.m_placingGovernor == governor)
		{
			governorHasPending = true;
		}
		return ecs::IterationBehavior::CONTINUE;
	});
	if (governorHasPending)
	{
		return false;
	}

	auto buildingCostIter = s_buildingCosts.find(buildingType);
	if (buildingCostIter == s_buildingCosts.end())
	{
		return false;
	}

	// Grab governor, check resource inventory
	auto& governorInventory = manager.getComponent<ECS_Core::Components::C_ResourceInventory>(governor);
	std::set<int> insufficientResources;
	for (auto&&[resource, cost] : buildingCostIter->second)
	{
		auto govResourceIter = governorInventory.m_collectedYields.find(resource);
		if (govResourceIter == governorInventory.m_collectedYields.end())
		{
			if (cost != 0)
			{
				insufficientResources.insert(resource);
			}
			continue;
		}
		if (govResourceIter->second < cost)
		{
			insufficientResources.insert(resource);
		}
	}
	if (insufficientResources.size())
	{
		return false;
	}

	auto ghostEntity = manager.createHandle();
	auto& ghost = manager.addComponent<ECS_Core::Components::C_BuildingGhost>(ghostEntity);
	ghost.m_placingGovernor = governor;
	ghost.m_paidYield = buildingCostIter->second;
	for (auto&&[resource, cost] : buildingCostIter->second)
	{
		governorInventory.m_collectedYields[resource] -= cost;
	}
	manager.addComponent<ECS_Core::Components::C_TilePosition>(ghostEntity) = position;
	manager.addComponent<ECS_Core::Components::C_PositionCartesian>(ghostEntity);
	manager.addComponent<ECS_Core::Components::C_BuildingDescription>(ghostEntity).m_buildingType = buildingType;

	auto& drawable = manager.addComponent<ECS_Core::Components::C_SFMLDrawable>(ghostEntity);
	auto shape = std::make_shared<sf::CircleShape>(2.5f);
	shape->setOutlineColor(sf::Color(128, 128, 128, 255));
	shape->setFillColor(sf::Color(128, 128, 128, 128));
	drawable.m_drawables[ECS_Core::Components::DrawLayer::UNIT][0].push_back({ shape,{ 0,0 } });
	return true;
}

void ConstructRequestedBuildings(ECS_Core::Manager& manager)
{
	using namespace ECS_Core;
	manager.forEntitiesMatching<Signatures::S_Planner>([&manager](
		const ecs::EntityIndex& governorIndex,
		ECS_Core::Components::C_ActionPlan& actionPlan)
	{
		auto governorHandle = manager.getHandle(governorIndex);

		for (auto&& action : actionPlan.m_plan)
		{
			// Man do I want metaclasses for safe_union
			if (std::holds_alternative<Action::LocalPlayer::CreateBuildingFromGhost>(action))
			{
				auto& create = std::get<Action::LocalPlayer::CreateBuildingFromGhost>(action);
				manager.forEntitiesMatching<Signatures::S_PlannedBuildingPlacement>([&manager, &governorHandle](
					const ecs::EntityIndex& ghostEntity,
					const Components::C_BuildingDescription&,
					const Components::C_TilePosition&,
					const Components::C_BuildingGhost& ghost)
				{
					if (ghost.m_placingGovernor != governorHandle)
					{
						return ecs::IterationBehavior::CONTINUE;
					}
					if (!ghost.m_currentPlacementValid)
					{
						return ecs::IterationBehavior::CONTINUE;
					}
					BeginBuildingConstruction(manager, ghostEntity);
					return ecs::IterationBehavior::BREAK;
				});
			}
			else if (std::holds_alternative<Action::LocalPlayer::CreateBuildingGhost>(action))
			{
				auto& ghost = std::get<Action::LocalPlayer::CreateBuildingGhost>(action);
				CreateBuildingGhost(manager, governorHandle, ghost.m_position, ghost.m_buildingClassId);
			}
		}
		return ecs::IterationBehavior::CONTINUE;
	});
}

void UpdateAgendas(ECS_Core::Manager& manager)
{
	using namespace ECS_Core;
	manager.forEntitiesMatching<ECS_Core::Signatures::S_Governor>([&manager](
		ecs::EntityIndex mI,
		const ECS_Core::Components::C_Realm& realm,
		ECS_Core::Components::C_Agenda& agenda)
	{
		ECS_Core::Components::YieldBuckets polityCollectedYields;

		for (auto&& territory : realm.m_territories)
		{
			if (manager.hasComponent<Components::C_ResourceInventory>(territory))
			{
				for (auto&&[resource, amount] : manager.getComponent<Components::C_ResourceInventory>(territory).m_collectedYields)
				{
					polityCollectedYields[resource] += amount;
				}
			}
		}

		std::sort(agenda.m_yieldPriority.begin(),
			agenda.m_yieldPriority.end(),
			[&polityCollectedYields](const int& left, const int& right) -> bool {
			if (polityCollectedYields[left] < polityCollectedYields[right]) return true;
			if (polityCollectedYields[right] < polityCollectedYields[left]) return false;
			return left < right;
		}
		);
		return ecs::IterationBehavior::CONTINUE;
	});
}

f64 AssignYieldWorkers(
	std::pair<const WorkerSkillKey, std::vector<WorkerProductionValue>> & skillLevel,
	WorkerAssignmentMap &assignments,
	f64 &amountToWork,
	const int& yield);

void GainIncomes(ECS_Core::Manager& manager)
{
	// Get current time
	// Assume the first entity is the one that has a valid time
	auto timeEntities = manager.entitiesMatching<ECS_Core::Signatures::S_TimeTracker>();
	if (timeEntities.size() == 0)
	{
		return;
	}
	const auto& time = manager.getComponent<ECS_Core::Components::C_TimeTracker>(timeEntities.front());
	manager.forEntitiesMatching<ECS_Core::Signatures::S_Governor>(
		[&manager, &time](
			ecs::EntityIndex mI,
			ECS_Core::Components::C_Realm& realm,
			const ECS_Core::Components::C_Agenda& agenda)
	{
		for (auto&& territoryHandle : realm.m_territories)
		{
			if (!manager.isHandleValid(territoryHandle) || !manager.hasComponent<ECS_Core::Components::C_TileProductionPotential>(territoryHandle)
				|| !manager.hasComponent<ECS_Core::Components::C_Territory>(territoryHandle)
				|| !manager.hasComponent<ECS_Core::Components::C_Population>(territoryHandle)
				|| !manager.hasComponent<ECS_Core::Components::C_ResourceInventory>(territoryHandle))
			{
				continue;
			}
			WorkerAssignmentMap assignments;

			SkillMap skillMap;

			auto& territoryYield = manager.getComponent<ECS_Core::Components::C_TileProductionPotential>(territoryHandle);
			auto& territory = manager.getComponent<ECS_Core::Components::C_Territory>(territoryHandle);
			auto& population = manager.getComponent<ECS_Core::Components::C_Population>(territoryHandle);
			auto& inventory = manager.getComponent<ECS_Core::Components::C_ResourceInventory>(territoryHandle);

			// Choose which yields will be worked based on government priorities
			// If production is the focus, highest skill works first
			// If training is the focus, lowest skill works first
			// After yields are determined, increase experience for the workers
			// Experience advances more slowly for higher skill
			f64 totalWorkerCount{ 0 };
			f64 totalSubsistenceCount{ 0 };
			for (auto&&[birthMonth, pop] : population.m_populations)
			{
				if (pop.m_class == ECS_Core::Components::PopulationClass::WORKERS)
				{
					auto totalProductiveEffort = (pop.m_womensHealth * pop.m_numWomen)
						+ (pop.m_mensHealth * pop.m_numMen);
					auto subsistenceEffort = ((1 - pop.m_womensHealth) * pop.m_numWomen)
						+ ((1 - pop.m_mensHealth) * pop.m_numMen);
					totalWorkerCount += totalProductiveEffort;
					totalSubsistenceCount += subsistenceEffort;
					assignments[birthMonth].m_assignments.insert({ -1, { birthMonth, totalProductiveEffort } });
					for (auto&& yieldType : agenda.m_yieldPriority)
					{
						// Make sure there are entries in the specialties for every skill needed to work the region
						pop.m_specialties[yieldType];
					}
					for (auto&&[skillType, experience] : pop.m_specialties)
					{
						skillMap[skillType][{experience.m_level, experience.m_experience}].push_back({ birthMonth, totalProductiveEffort });
					}
				}
			}
			inventory.m_collectedYields[ECS_Core::Components::Yields::FOOD] += time.m_frameDuration * totalSubsistenceCount / 80;

			// We know who we want to work first
			// Decide where they're working
			// Figure out the actual amount yielded by this work
			for (auto&& yield : agenda.m_yieldPriority)
			{
				auto availableYield = territoryYield.m_availableYields.find(yield);
				if (availableYield == territoryYield.m_availableYields.end())
				{
					continue;
				}

				f64 amountToWork = availableYield->second.m_workableTiles;

				switch (agenda.m_popAgenda)
				{
				case ECS_Core::Components::PopulationAgenda::TRAINING:
					// Iterate from lowest skill to highest
					for (auto&& skillLevel : skillMap[yield])
					{
						if (amountToWork == 0)
						{
							break;
						}

						availableYield->second.m_productionProgress +=
							AssignYieldWorkers(skillLevel, assignments, amountToWork, yield)
							* time.m_frameDuration;
					}
					break;

				case ECS_Core::Components::PopulationAgenda::PRODUCTION:
					// Iterate from highest skill to lowest
					for (auto&& skillLevel : reverse(skillMap[yield]))
					{
						if (amountToWork == 0)
						{
							break;
						}

						availableYield->second.m_productionProgress +=
							AssignYieldWorkers(skillLevel, assignments, amountToWork, yield)
							* time.m_frameDuration;
					}
					break;
				}
			}

			for (auto&&[tileType, production] : territoryYield.m_availableYields)
			{
				s32 gainAmount = static_cast<s32>(production.m_productionProgress / production.m_productionInterval);
				for (auto&&[resource, yield] : production.m_productionYield)
				{
					inventory.m_collectedYields[resource] += gainAmount * yield;
				}
				production.m_productionProgress -= production.m_productionInterval * gainAmount;
			}

			// And assign experience to workers
			for (auto&&[birthMonth, workers] : assignments)
			{
				for (auto&[skillType, assignment] : workers.m_assignments)
				{
					if (skillType < 0) continue;
					population.m_populations[birthMonth].m_specialties[skillType]
						.m_experience += assignment.m_productiveValue;
				}
			}
		}
		return ecs::IterationBehavior::CONTINUE;
	});
}

f64 AssignYieldWorkers(
	std::pair<const WorkerSkillKey, std::vector<WorkerProductionValue>> & skillLevel,
	WorkerAssignmentMap &assignments,
	f64 &amountToWork,
	const int & yield)
{
	f64 totalYieldAmount = 0;
	for (auto&& worker : skillLevel.second)
	{
		auto assignment = assignments.find(worker.m_workerKey);
		if (assignment == assignments.end())
		{
			continue;
		}
		// Find how many people are still unassigned
		auto countWorkingYield = min<f64>(amountToWork, assignment->second.m_assignments[-1].m_productiveValue);
		assignment->second.m_assignments[yield].m_productiveValue += countWorkingYield;
		assignment->second.m_assignments[-1].m_productiveValue -= countWorkingYield;
		amountToWork -= countWorkingYield;
		totalYieldAmount += countWorkingYield * (sqrt(skillLevel.first.m_level));
	}
	return totalYieldAmount;
}

void Government::ProgramInit() {}

extern sf::Font s_font;
void Government::SetupGameplay()
{
	auto localPlayer = m_managerRef.createHandle();
	m_managerRef.addComponent<ECS_Core::Components::C_Realm>(localPlayer);
	m_managerRef.addComponent<ECS_Core::Components::C_Agenda>(localPlayer).m_yieldPriority = { 0,1,2,3,4,5,6,7 };
	m_managerRef.addComponent<ECS_Core::Components::C_UserInputs>(localPlayer);
	m_managerRef.addComponent<ECS_Core::Components::C_ActionPlan>(localPlayer);

	m_managerRef.addTag<ECS_Core::Tags::T_LocalPlayer>(localPlayer);
}

void Government::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	switch (phase)
	{
	case GameLoopPhase::PREPARATION:
		break;
	case GameLoopPhase::INPUT:
		break;
	case GameLoopPhase::ACTION:
		ConstructRequestedBuildings(m_managerRef);
		m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_WealthPlanner>([&manager = m_managerRef](
			const ecs::EntityIndex& entityIndex,
			const ECS_Core::Components::C_ActionPlan& actionPlan,
			ECS_Core::Components::C_Realm& realm) {
			for (auto&& action : actionPlan.m_plan)
			{
				if (std::holds_alternative<Action::CreateBuildingUnit>(action))
				{
					auto& builder = std::get<Action::CreateBuildingUnit>(action);
					auto costIter = s_buildingCosts.find(builder.m_buildingTypeId);
					if (costIter == s_buildingCosts.end())
					{
						continue;
					}
					if (builder.m_popSource && manager.hasComponent<ECS_Core::Components::C_ResourceInventory>(*builder.m_popSource))
					{
						auto& buildingInventory = manager.getComponent<ECS_Core::Components::C_ResourceInventory>(*builder.m_popSource);
						bool hasResources = true;
						for (auto&& [resource, cost] : costIter->second)
						{
							// Check to see that all resources required are available
							auto inventoryStore = buildingInventory.m_collectedYields.find(resource);
							if (inventoryStore == buildingInventory.m_collectedYields.end()
								|| inventoryStore->second < cost)
							{
								hasResources = false;
								break;
							}
						}
						if (!hasResources)
						{
							// TODO: Surface Error
							continue;
						}
					}

					// Make sure the population source entity is still around
					auto newEntityHandle = [&manager, &builder]() -> std::optional<ecs::Impl::Handle>
					{
						if (builder.m_popSource)
						{
							if (!manager.hasComponent<ECS_Core::Components::C_Population>(*builder.m_popSource))
							{
								// TODO: Surface Error
								return std::nullopt;
							}
							// Take the youngest workers, 10:5 male:female (more men die on the road)
							auto& populationSource = manager.getComponent<ECS_Core::Components::C_Population>(*builder.m_popSource);
							int sourceTotalMen{ 0 };
							int sourceTotalWomen{ 0 };
							for (auto&& [birthMonth,pop] : populationSource.m_populations)
							{
								if (pop.m_class != ECS_Core::Components::PopulationClass::WORKERS)
								{
									continue;
								}
								sourceTotalMen += pop.m_numMen;
								sourceTotalWomen += pop.m_numWomen;
							}
							int totalMenToMove = 10;
							int totalWomenToMove = 5;
							if (totalMenToMove > (sourceTotalMen - 3) || totalWomenToMove > (sourceTotalWomen - 3))
							{
								return std::nullopt;
							}

							// Spawn entity for the unit, then take costs and population
							auto newEntity = manager.createHandle();
							auto& movingUnit = manager.addComponent<ECS_Core::Components::C_MovingUnit>(newEntity);
							auto& moverInventory = manager.addComponent<ECS_Core::Components::C_ResourceInventory>(newEntity);
							moverInventory.m_collectedYields[ECS_Core::Components::Yields::FOOD] = 50;
							auto& population = manager.addComponent<ECS_Core::Components::C_Population>(newEntity);
							int menMoved = 0;
							int womenMoved = 0;
							for (auto&& [birthMonth,pop] : populationSource.m_populations)
							{
								if (menMoved == totalMenToMove && totalWomenToMove == 5)
								{
									break;
								}
								if (pop.m_class != ECS_Core::Components::PopulationClass::WORKERS)
								{
									continue;
								}
								auto menToMove = min<int>(totalMenToMove - menMoved, pop.m_numMen);
								auto womenToMove = min<int>(totalWomenToMove - womenMoved, pop.m_numWomen);

								auto& popCopy = population.m_populations[birthMonth];
								popCopy = pop;
								popCopy.m_numMen = menToMove;
								popCopy.m_numWomen = womenToMove;

								pop.m_numMen -= menToMove;
								pop.m_numWomen -= womenToMove;


								menMoved += menToMove;
								womenMoved += womenToMove;
							}
							return newEntity;
						}
						else
						{
							auto newEntity = manager.createHandle();
							auto& movingUnit = manager.addComponent<ECS_Core::Components::C_MovingUnit>(newEntity);

							auto& population = manager.addComponent<ECS_Core::Components::C_Population>(newEntity);
							auto& moverInventory = manager.addComponent<ECS_Core::Components::C_ResourceInventory>(newEntity);
							moverInventory.m_collectedYields[ECS_Core::Components::Yields::FOOD] = 50;
							auto timeFront = manager.entitiesMatching<ECS_Core::Signatures::S_TimeTracker>().front();
							auto& time = manager.getComponent<ECS_Core::Components::C_TimeTracker>(timeFront);
							auto& foundingPopulation = population.m_populations[-12 * (time.m_year - 15) - time.m_month];
							foundingPopulation.m_numMen = 25;
							foundingPopulation.m_numWomen = 25;
							foundingPopulation.m_class = ECS_Core::Components::PopulationClass::WORKERS;
							return newEntity;
						}
					}();

					if (!newEntityHandle)
					{
						continue;
					}
					auto& buildingDesc = manager.addComponent<ECS_Core::Components::C_BuildingDescription>(*newEntityHandle);
					buildingDesc.m_buildingType = builder.m_buildingTypeId;
					auto& tilePosition = manager.addComponent<ECS_Core::Components::C_TilePosition>(*newEntityHandle);
					tilePosition.m_position = builder.m_spawningPosition;

					auto& drawable = manager.addComponent<ECS_Core::Components::C_SFMLDrawable>(*newEntityHandle);
					auto unitIcon = std::make_shared<sf::CircleShape>(2.5f, 3);
					unitIcon->setFillColor({ 45,45,45 });
					unitIcon->setOutlineColor({ 120,120,120 });
					unitIcon->setOutlineThickness(-0.3f);
					drawable.m_drawables[ECS_Core::Components::DrawLayer::UNIT][0].push_back({
						unitIcon, {0, 0} });

					auto& cartesianPosition = manager.addComponent<ECS_Core::Components::C_PositionCartesian>(*newEntityHandle);

					if (builder.m_popSource && manager.hasComponent<ECS_Core::Components::C_ResourceInventory>(*builder.m_popSource))
					{
						auto& buildingInventory = manager.getComponent<ECS_Core::Components::C_ResourceInventory>(*builder.m_popSource);
						for (auto&& [resource, cost] : costIter->second)
						{
							buildingInventory.m_collectedYields[resource] -= cost;
						}
					}
				}
			}
			return ecs::IterationBehavior::CONTINUE;
		});
		break;
	case GameLoopPhase::ACTION_RESPONSE:
		UpdateAgendas(m_managerRef);
		GainIncomes(m_managerRef);
		break;
	case GameLoopPhase::RENDER:
		break;
	case GameLoopPhase::CLEANUP:
		using namespace ECS_Core;
		m_managerRef.forEntitiesMatching<Signatures::S_Governor>([&manager = m_managerRef](const ecs::EntityIndex& entity,
			Components::C_Realm& realm,
			const Components::C_Agenda&)
		{
			for (auto iter = realm.m_territories.begin(); iter != realm.m_territories.end();)
			{
				if (manager.isHandleValid(*iter))
				{
					++iter;
				}
				else
				{
					realm.m_territories.erase(iter++);
				}
			}
			return ecs::IterationBehavior::CONTINUE;
		});
		return;
	}
}

bool Government::ShouldExit()
{
	return false;
}

DEFINE_SYSTEM_INSTANTIATION(Government);