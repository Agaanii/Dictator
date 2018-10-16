//-----------------------------------------------------------------------------
// All code is property of Dictator Developers Inc
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2018

// Systems/Government.cpp
// The best system of government ever created

#include "../Core/typedef.h"

#include "Government.h"

#include <algorithm>

void Government::UpdateAgendas()
{
	using namespace ECS_Core;
	m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_Governor>([&manager = m_managerRef](
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

void Government::GainIncomes()
{
	// Get current time
	// Assume the first entity is the one that has a valid time
	auto timeEntities = m_managerRef.entitiesMatching<ECS_Core::Signatures::S_TimeTracker>();
	if (timeEntities.size() == 0)
	{
		return;
	}
	const auto& time = m_managerRef.getComponent<ECS_Core::Components::C_TimeTracker>(timeEntities.front());
	m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_Governor>(
		[&manager = m_managerRef, &time, this](
			ecs::EntityIndex mI,
			ECS_Core::Components::C_Realm& realm,
			const ECS_Core::Components::C_Agenda& agenda)
	{
		for (auto&& territoryHandle : realm.m_territories)
		{
			if (!manager.isHandleValid(territoryHandle)
				|| !manager.hasComponent<ECS_Core::Components::C_TileProductionPotential>(territoryHandle)
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

f64 Government::AssignYieldWorkers(
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

void Government::MovePopulations(
	ECS_Core::Components::C_Population& populationSource,
	ECS_Core::Components::C_Population& populationTarget,
	int totalMenToMove,
	int totalWomenToMove)
{
	int menMoved = 0;
	int womenMoved = 0;
	for (auto&&[birthMonth, pop] : populationSource.m_populations)
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

		auto& popCopy = populationTarget.m_populations[birthMonth];
		popCopy = pop;
		popCopy.m_numMen = menToMove;
		popCopy.m_numWomen = womenToMove;

		pop.m_numMen -= menToMove;
		pop.m_numWomen -= womenToMove;


		menMoved += menToMove;
		womenMoved += womenToMove;
	}
}

void Government::MoveFullPopulation(
	ECS_Core::Components::C_Population& populationSource,
	ECS_Core::Components::C_Population& populationTarget)
{
	for (auto&&[popKey, pop] : populationSource.m_populations)
	{
		if (pop.m_class != ECS_Core::Components::PopulationClass::WORKERS)
		{
			continue;
		}
		auto popKeyCopy = popKey;
		while (populationTarget.m_populations.find(popKeyCopy) != populationTarget.m_populations.end())
		{
			++popKeyCopy.m_segmentIndex;
		}
		populationTarget.m_populations[popKeyCopy] = pop;
	}
	populationSource.m_populations.clear();
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


		m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_CommandUnit>([this](
			const ecs::EntityIndex& mI,
			const ECS_Core::Components::C_TilePosition& tilePosition,
			ECS_Core::Components::C_MovingUnit&,
			ECS_Core::Components::C_ResourceInventory& inventory,
			ECS_Core::Components::C_Population& population,
			ECS_Core::Components::C_CommandMessage& command)
		{
			if (!m_managerRef.isHandleValid(command.m_commandee))
			{
				return ecs::IterationBehavior::CONTINUE;
			}

			if (!m_managerRef.matchesSignature<ECS_Core::Signatures::S_CompleteBuilding>(m_managerRef.getEntityIndex(command.m_commandee)))
			{
				return ecs::IterationBehavior::CONTINUE;
			}

			if (tilePosition.m_position == m_managerRef.getComponent<ECS_Core::Components::C_TilePosition>(command.m_commandee).m_position
				 && command.m_commands.empty())
			{
				MoveFullPopulation(population, m_managerRef.getComponent<ECS_Core::Components::C_Population>(command.m_commandee));
				for (auto&&[resource, amount] : inventory.m_collectedYields)
				{
					m_managerRef.getComponent<ECS_Core::Components::C_ResourceInventory>(command.m_commandee).m_collectedYields[resource] += amount;
				}
				m_managerRef.addTag<ECS_Core::Tags::T_Dead>(mI);
			}

			return ecs::IterationBehavior::CONTINUE;
		});

		m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_WealthPlanner>(
			[&manager = m_managerRef, this](
				const ecs::EntityIndex& entityIndex,
				const ECS_Core::Components::C_ActionPlan& actionPlan,
				ECS_Core::Components::C_Realm& realm) {
			for (auto&& action : actionPlan.m_plan)
			{
				if (std::holds_alternative<Action::CreateBuildingUnit>(action.m_command))
				{
					auto& builder = std::get<Action::CreateBuildingUnit>(action.m_command);
					auto costIter = m_buildingCosts.find(builder.m_buildingTypeId);
					if (costIter == m_buildingCosts.end())
					{
						continue;
					}
					if (builder.m_popSource && realm.m_capitol && !action.m_delivered)
					{
						// Check if we're in the building that's to create this unit
						auto sourceHandle = manager.getHandle(*builder.m_popSource);
						if (sourceHandle != *realm.m_capitol)
						{
							// Instead of creating the unit directly, send a dude with the command and the resources
							// TODO: Make trades with a capitol and non-capitol do a full resource transfer except for food
							if (manager.hasComponent<ECS_Core::Components::C_ResourceInventory>(*realm.m_capitol)
								&& manager.hasComponent<ECS_Core::Components::C_Population>(*realm.m_capitol)
								&& manager.hasComponent<ECS_Core::Components::C_TilePosition>(*realm.m_capitol))
							{
								auto& capitolInventory = manager.getComponent<ECS_Core::Components::C_ResourceInventory>(*realm.m_capitol);
								bool hasResources = true;
								for (auto&&[resource, cost] : costIter->second)
								{
									// Check to see that all resources required are available
									auto inventoryStore = capitolInventory.m_collectedYields.find(resource);
									if (inventoryStore == capitolInventory.m_collectedYields.end()
										|| inventoryStore->second < cost)
									{
										hasResources = false;
										break;
									}
								}
								if (hasResources)
								{
									auto messengerHandle = manager.createHandle();
									MovePopulations(
										manager.getComponent<ECS_Core::Components::C_Population>(*realm.m_capitol),
										manager.addComponent<ECS_Core::Components::C_Population>(messengerHandle),
										2,
										0);

									auto& messengerInventory = manager.addComponent<ECS_Core::Components::C_ResourceInventory>(messengerHandle);
									for (auto&&[resource, cost] : costIter->second)
									{
										messengerInventory.m_collectedYields[resource] += cost;
										capitolInventory.m_collectedYields[resource] -= cost;
									}

									auto& command = manager.addComponent<ECS_Core::Components::C_CommandMessage>(messengerHandle);
									command.m_commands.push_back(action);
									command.m_commandee = sourceHandle;
									command.m_governor = manager.getHandle(entityIndex);

									manager.addComponent<ECS_Core::Components::C_TilePosition>(messengerHandle)
										= manager.getComponent<ECS_Core::Components::C_TilePosition>(*realm.m_capitol);

									manager.addComponent<ECS_Core::Components::C_PositionCartesian>(messengerHandle);
									auto& movement = manager.addComponent<ECS_Core::Components::C_MovingUnit>(messengerHandle);
									movement.m_movementPerDay = 5;
									
									auto& graphic = manager.addComponent<ECS_Core::Components::C_SFMLDrawable>(messengerHandle);
									auto circle = std::make_shared<sf::CircleShape>(2.5f, 4);
									circle->setFillColor({ 25,80,255 });
									circle->setOutlineColor({ 255,80,25 });
									circle->setOutlineThickness(-0.75f);
									graphic.m_drawables[ECS_Core::Components::DrawLayer::UNIT][7].push_back({ circle, {} });

									manager.addComponent<ECS_Core::Components::C_Vision>(messengerHandle);
								}
								continue;
							}
						}
					}
					if (builder.m_popSource && manager.hasComponent<ECS_Core::Components::C_ResourceInventory>(*builder.m_popSource))
					{
						auto& buildingInventory = manager.getComponent<ECS_Core::Components::C_ResourceInventory>(*builder.m_popSource);
						bool hasResources = true;
						for (auto&&[resource, cost] : costIter->second)
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
					auto newEntityHandle = [&manager, &builder, this]() -> std::optional<ecs::Impl::Handle>
					{
						if (builder.m_popSource)
						{
							if (!manager.hasComponent<ECS_Core::Components::C_Population>(*builder.m_popSource))
							{
								// TODO: Surface Error
								return std::nullopt;
							}
							// Take the youngest workers, 10:5 male:female (more men die on the road)
							ECS_Core::Components::C_Population& populationSource = manager.getComponent<ECS_Core::Components::C_Population>(*builder.m_popSource);
							int sourceTotalMen{ 0 };
							int sourceTotalWomen{ 0 };
							for (auto&&[birthMonth, pop] : populationSource.m_populations)
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
							manager.addComponent<ECS_Core::Components::C_Vision>(newEntity);
							auto& moverInventory = manager.addComponent<ECS_Core::Components::C_ResourceInventory>(newEntity);
							moverInventory.m_collectedYields[ECS_Core::Components::Yields::FOOD] = 50;
							MovePopulations(
								populationSource,
								manager.addComponent<ECS_Core::Components::C_Population>(newEntity),
								totalMenToMove,
								totalWomenToMove);
							return newEntity;
						}
						else
						{
							auto newEntity = manager.createHandle();
							auto& movingUnit = manager.addComponent<ECS_Core::Components::C_MovingUnit>(newEntity);
							manager.addComponent<ECS_Core::Components::C_Vision>(newEntity);

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
						for (auto&&[resource, cost] : costIter->second)
						{
							buildingInventory.m_collectedYields[resource] -= cost;
						}
					}
				}
				else if (std::holds_alternative<Action::CreateExplorationUnit>(action.m_command))
				{
					auto timeEntities = m_managerRef.entitiesMatching<ECS_Core::Signatures::S_TimeTracker>();
					if (timeEntities.size() == 0)
					{
						continue;
					}
					const auto& time = m_managerRef.getComponent<ECS_Core::Components::C_TimeTracker>(timeEntities.front());

					auto& createAction = std::get<Action::CreateExplorationUnit>(action.m_command);
					// Requires population of >5 Men and 1 food per planned day
					if (!createAction.m_popSource
						|| !m_managerRef.hasComponent<ECS_Core::Components::C_Population>(*createAction.m_popSource)
						|| !m_managerRef.hasComponent<ECS_Core::Components::C_ResourceInventory>(*createAction.m_popSource))
					{
						continue;
					}

					auto& popSource = m_managerRef.getComponent<ECS_Core::Components::C_Population>(*createAction.m_popSource);
					auto& inventorySource = m_managerRef.getComponent<ECS_Core::Components::C_ResourceInventory>(*createAction.m_popSource);

					int numMen = 0;
					for (auto&&[age, popSegment] : popSource.m_populations)
					{
						if (popSegment.m_class != ECS_Core::Components::PopulationClass::WORKERS)
						{
							continue;
						}
						if ((numMen += popSegment.m_numMen) > 8)
						{
							break;
						}
					}
					if (numMen <= 8)
					{
						continue;
					}
					auto foodNeeded = createAction.m_daysToExplore / 30;

					if (inventorySource.m_collectedYields[ECS_Core::Components::Yields::FOOD] < foodNeeded)
					{
						continue;
					}

					// We have what's needed to make the scout
					auto unitHandle = m_managerRef.createHandle();
					auto& unitInventory = m_managerRef.addComponent<ECS_Core::Components::C_ResourceInventory>(unitHandle);
					m_managerRef.addComponent<ECS_Core::Components::C_TilePosition>(unitHandle).m_position = createAction.m_spawningPosition;
					m_managerRef.addComponent<ECS_Core::Components::C_PositionCartesian>(unitHandle);

					unitInventory.m_collectedYields[ECS_Core::Components::Yields::FOOD] = foodNeeded;
					inventorySource.m_collectedYields[ECS_Core::Components::Yields::FOOD] -= foodNeeded;
					MovePopulations(
						popSource,
						m_managerRef.addComponent<ECS_Core::Components::C_Population>(unitHandle),
						5,
						0);

					manager.addComponent<ECS_Core::Components::C_Vision>(unitHandle);
					auto& movementPlan = m_managerRef.addComponent<ECS_Core::Components::C_MovingUnit>(unitHandle);
					ECS_Core::Components::ExplorationPlan explorePlan;
					explorePlan.m_daysToExplore = createAction.m_daysToExplore;
					explorePlan.m_direction = createAction.m_exploreDirection;
					explorePlan.m_leavingYear = time.m_year;
					explorePlan.m_leavingMonth = time.m_month;
					explorePlan.m_leavingDay = time.m_day;
					explorePlan.m_homeBasePosition = createAction.m_spawningPosition;
					explorePlan.m_homeBase = manager.getHandle(*createAction.m_popSource);

					movementPlan.m_explorationPlan = explorePlan;
					movementPlan.m_movementPerDay = createAction.m_movementSpeed;

					auto& graphics = m_managerRef.addComponent<ECS_Core::Components::C_SFMLDrawable>(unitHandle);
					auto unitGraphic = std::make_shared<sf::CircleShape>(2.5f, 5);
					unitGraphic->setFillColor({ 240, 30, 0 });
					graphics.m_drawables[ECS_Core::Components::DrawLayer::UNIT][0].push_back({ unitGraphic, {} });
				}
			}
			return ecs::IterationBehavior::CONTINUE;
		});
		break;
	case GameLoopPhase::ACTION_RESPONSE:
		UpdateAgendas();
		GainIncomes();
		m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_MovingUnit>(
			[&manager = m_managerRef, this](
				const ecs::EntityIndex& e,
				const ECS_Core::Components::C_TilePosition&,
				const ECS_Core::Components::C_MovingUnit& mover,
				ECS_Core::Components::C_Population& population,
				const ECS_Core::Components::C_Vision&)
		{
			if (mover.m_explorationPlan)
			{
				if (!mover.m_currentMovement && mover.m_explorationPlan->m_explorationComplete)
				{
					// Scout has returned, their population can return to the fold
					if (manager.isHandleValid(mover.m_explorationPlan->m_homeBase) &&
						manager.hasComponent<ECS_Core::Components::C_Population>(mover.m_explorationPlan->m_homeBase))
					{
						MoveFullPopulation(population, manager.getComponent<ECS_Core::Components::C_Population>(mover.m_explorationPlan->m_homeBase));
						manager.addTag<ECS_Core::Tags::T_Dead>(e);
					}
				}
			}
			return ecs::IterationBehavior::CONTINUE;
		});
		break;
	case GameLoopPhase::RENDER:
		break;
	case GameLoopPhase::CLEANUP:
		using namespace ECS_Core;
		m_managerRef.forEntitiesMatching<Signatures::S_Governor>(
			[&manager = m_managerRef](
				const ecs::EntityIndex& entity,
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