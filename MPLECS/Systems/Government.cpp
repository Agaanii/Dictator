//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Systems/Government.cpp
// The best system of government ever created

#include "../Core/typedef.h"

#include "Systems.h"

#include "../Components/UIComponents.h"

#include "../ECS/System.h"
#include "../ECS/ECS.h"

#include <iostream>

void BeginBuildingConstruction(ECS_Core::Manager & manager, ecs::EntityIndex & ghostEntity)
{
	manager.addComponent<ECS_Core::Components::C_BuildingConstruction>(ghostEntity).m_placingGovernor =
		manager.getComponent<ECS_Core::Components::C_BuildingGhost>(ghostEntity).m_placingGovernor;
	manager.delComponent<ECS_Core::Components::C_BuildingGhost>(ghostEntity);
	auto& drawable = manager.getComponent<ECS_Core::Components::C_SFMLDrawable>(ghostEntity);
	for (auto& prio : drawable.m_drawables[ECS_Core::Components::DrawLayer::BUILDING])
	{
		for (auto&& building : prio.second)
		{
			sf::Shape* shapeDrawable = dynamic_cast<sf::Shape*>(building.m_graphic.get());
			if (shapeDrawable)
				shapeDrawable->setFillColor(sf::Color(128, 128, 128, 26));
		}
	}
}

std::map<int, std::map<ECS_Core::Components::YieldType, s64>> s_buildingCosts
{
	{
		0, 
		{
			{1, 50},
			{2, 50}
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
	for (auto&& cost : buildingCostIter->second)
	{
		auto govResourceIter = governorInventory.m_collectedYields.find(cost.first);
		if (govResourceIter == governorInventory.m_collectedYields.end())
		{
			if (cost.second != 0)
			{
				insufficientResources.insert(cost.first);
			}
			continue;
		}
		if (govResourceIter->second < cost.second)
		{
			insufficientResources.insert(cost.first);
		}
	}
	if (insufficientResources.size())
	{
		return false;
	}

	auto ghostEntity = manager.createIndex();
	auto& ghost = manager.addComponent<ECS_Core::Components::C_BuildingGhost>(ghostEntity);
	ghost.m_placingGovernor = governor;
	ghost.m_paidYield = buildingCostIter->second;
	for (auto&& cost : buildingCostIter->second)
	{
		governorInventory.m_collectedYields[cost.first] -= cost.second;
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

void InterpretLocalInput(ECS_Core::Manager& manager)
{
	auto inputEntities = manager.entitiesMatching<ECS_Core::Signatures::S_Input>();
	auto playerGovernorEntities = manager.entitiesMatching<ECS_Core::Signatures::S_PlayerGovernor>();
	if (playerGovernorEntities.size() != 1)
	{
		std::cout << "Pain and suffering: there are " << playerGovernorEntities.size() << " player governors";
		return;
	}
	auto playerGovernorEntity = playerGovernorEntities.front();
	auto playerGovernorHandle = manager.getHandle(playerGovernorEntity);
	if (inputEntities.size() == 0) return;
	// Should only be one input component, only one computer running this
	ECS_Core::Components::C_UserInputs& inputComponent = manager.getComponent<ECS_Core::Components::C_UserInputs>(inputEntities.front());
	
	using namespace ECS_Core;
	// First try to place a pending building from local player
	if (inputComponent.m_unprocessedThisFrameDownMouseButtonFlags & (u8)ECS_Core::Components::MouseButtons::LEFT)
	{
		for (auto&& ghostEntity : manager.entitiesMatching<ECS_Core::Signatures::S_PlannedBuildingPlacement>())
		{
			auto& ghost = manager.getComponent<ECS_Core::Components::C_BuildingGhost>(ghostEntity);
			if (ghost.m_placingGovernor != playerGovernorHandle)
			{
				continue;
			}
			if (!ghost.m_currentPlacementValid)
			{
				// TODO: Surface error
				continue;
			}
			BeginBuildingConstruction(manager, ghostEntity);
			inputComponent.ProcessMouseDown(ECS_Core::Components::MouseButtons::LEFT);
			break;
		}
	}

	if (inputComponent.m_newKeyDown.count(ECS_Core::Components::InputKeys::B))
	{
		if (CreateBuildingGhost(manager, playerGovernorHandle, *inputComponent.m_currentMousePosition.m_tilePosition, 0))
		{
			inputComponent.ProcessKey(ECS_Core::Components::InputKeys::B);
		}
	}

	manager.forEntitiesMatching<Signatures::S_PlannedBuildingPlacement>(
		[&inputComponent, &playerGovernorHandle](
			const ecs::EntityIndex& entity,
			const Components::C_BuildingDescription&,
			Components::C_TilePosition& position,
			const Components::C_BuildingGhost& ghost)
	{
		if (ghost.m_placingGovernor == playerGovernorHandle)
		{
			position.m_position = *inputComponent.m_currentMousePosition.m_tilePosition;
		}
		return ecs::IterationBehavior::CONTINUE;
	});
}


using WorkerKey = s32;

struct WorkerAssignment
{
	using AssignmentMap = std::map<ECS_Core::Components::SpecialtyId, s32>;
	AssignmentMap m_assignments;
};

using WorkerAssignmentMap = std::map<WorkerKey, WorkerAssignment>;

struct WorkerSkillKey
{
	ECS_Core::Components::SpecialtyLevel m_level;
	ECS_Core::Components::SpecialtyExperience m_xp;

	bool operator< (const WorkerSkillKey& other) const
	{
		if (m_level < other.m_level) return true;
		if (m_level > other.m_level) return false;

		if (m_xp < other.m_xp) return true;
		if (m_xp > other.m_xp) return false;
		return false;
	}
};
using WorkerSkillMap = std::map<WorkerSkillKey, std::vector<WorkerKey>>;
using SkillMap = std::map<ECS_Core::Components::SpecialtyId, WorkerSkillMap>;
int AssignYieldWorkers(
	std::pair<const WorkerSkillKey, std::vector<int>> & skillLevel,
	WorkerAssignmentMap &assignments,
	s32 &amountToWork,
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
			ECS_Core::Components::C_ResourceInventory& inventory,
			const ECS_Core::Components::C_Agenda& agenda)
	{
		for (auto&& territoryHandle : realm.m_territories)
		{
			if (!manager.isHandleValid(territoryHandle) || !manager.hasComponent<ECS_Core::Components::C_YieldPotential>(territoryHandle))
			{
				continue;
			}
			WorkerAssignmentMap assignments;

			SkillMap skillMap;

			auto& territoryYield = manager.getComponent<ECS_Core::Components::C_YieldPotential>(territoryHandle);
			auto& territory = manager.getComponent<ECS_Core::Components::C_Territory>(territoryHandle);
			// Choose which yields will be worked based on government priorities
			// If production is the focus, highest skill works first
			// If training is the focus, lowest skill works first
			// After yields are determined, increase experience for the workers
			// Experience advances more slowly for higher skill
			s32 totalWorkerCount{ 0 };
			for (auto&& pop : territory.m_populations)
			{
				if (pop.second.m_class == ECS_Core::Components::PopulationClass::WORKERS)
				{
					auto totalPeople = pop.second.m_numWomen + pop.second.m_numMen;
					totalWorkerCount += totalPeople;
					assignments[pop.first].m_assignments[-1] = totalPeople;
					for (auto&& yieldType : agenda.m_yieldPriority)
					{
						// Make sure there are entries in the specialties for every skill needed to work the region
						pop.second.m_specialties[yieldType];
					}
					for (auto&& skill : pop.second.m_specialties)
					{
						skillMap[skill.first][{skill.second.m_level, skill.second.m_experience}].push_back(pop.first);
					}
				}
			}
			
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

				auto amountToWork = availableYield->second.m_value;

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

			for (auto&& yield : territoryYield.m_availableYields)
			{
				s32 gainAmount = static_cast<s32>(yield.second.m_productionProgress / yield.second.m_productionInterval);
				inventory.m_collectedYields[yield.first] += gainAmount;
				yield.second.m_productionProgress -= yield.second.m_productionInterval * gainAmount;
			}

			// And assign experience to workers
			for (auto&& workers : assignments)
			{
				for (auto& assignment : workers.second.m_assignments)
				{
					territory.m_populations[workers.first].m_specialties[assignment.first]
						.m_experience += assignment.second;
				}
			}
		}
		return ecs::IterationBehavior::CONTINUE;
	});
}

void Government::ProgramInit() {}

int AssignYieldWorkers(
	std::pair<const WorkerSkillKey, std::vector<int>> & skillLevel,
	WorkerAssignmentMap &assignments,
	s32 &amountToWork,
	const int & yield)
{
	int totalYieldAmount = 0;
	for (auto&& workerKey : skillLevel.second)
	{
		auto assignment = assignments.find(workerKey);
		if (assignment == assignments.end())
		{
			continue;
		}
		// Find how many people are still unassigned
		auto countWorkingYield = min<s32>(amountToWork, assignment->second.m_assignments[-1]);
		assignment->second.m_assignments[yield] += countWorkingYield;
		assignment->second.m_assignments[-1] -= countWorkingYield;
		amountToWork -= countWorkingYield;
		totalYieldAmount += countWorkingYield * static_cast<s32>(sqrt(skillLevel.first.m_level));
	}
	return totalYieldAmount;
}

extern sf::Font s_font;
void Government::SetupGameplay()
{
	auto localPlayerGovernment = m_managerRef.createHandle();
	m_managerRef.addComponent<ECS_Core::Components::C_Realm>(localPlayerGovernment);
	m_managerRef.addComponent<ECS_Core::Components::C_ResourceInventory>(localPlayerGovernment).m_collectedYields = { {1, 100},{2,100} };
	m_managerRef.addComponent<ECS_Core::Components::C_Agenda>(localPlayerGovernment).m_yieldPriority = { 0,1,2,3,4,5,6,7 };

	auto& uiFrameComponent = m_managerRef.addComponent<ECS_Core::Components::C_UIFrame>(localPlayerGovernment);
	uiFrameComponent.m_frame
		= DefineUIFrame(
			"Inventory",
			DataBinding(ECS_Core::Components::C_ResourceInventory, m_collectedYields));
	uiFrameComponent.m_dataStrings[{0, 0}] = { { 40,0 }, std::make_shared<sf::Text>() };
	uiFrameComponent.m_dataStrings[{0, 1}] = {{ 40,30 }, std::make_shared<sf::Text>() };
	uiFrameComponent.m_dataStrings[{0, 2}] = {{ 40,60 }, std::make_shared<sf::Text>() };
	uiFrameComponent.m_dataStrings[{0, 3}] = {{ 40,90 }, std::make_shared<sf::Text>() };
	uiFrameComponent.m_dataStrings[{0, 4}] = {{ 40,120 }, std::make_shared<sf::Text>() };
	uiFrameComponent.m_dataStrings[{0, 5}] = {{ 40,150 }, std::make_shared<sf::Text>() };
	uiFrameComponent.m_dataStrings[{0, 6}] = {{ 40,180 }, std::make_shared<sf::Text>() };
	uiFrameComponent.m_dataStrings[{0, 7}] = {{ 40,210 }, std::make_shared<sf::Text>() };

	uiFrameComponent.m_topLeftCorner = { 50,50 };
	uiFrameComponent.m_size = { 200, 240 };
	uiFrameComponent.m_global = true;
	m_managerRef.addTag<ECS_Core::Tags::T_LocalPlayer>(localPlayerGovernment);

	auto& drawable = m_managerRef.addComponent<ECS_Core::Components::C_SFMLDrawable>(localPlayerGovernment);
	auto resourceWindowBackground = std::make_shared<sf::RectangleShape>(sf::Vector2f(200, 240));
	resourceWindowBackground->setFillColor({});
	drawable.m_drawables[ECS_Core::Components::DrawLayer::MENU][0].push_back({ resourceWindowBackground, {} });
	for (auto&& dataStr : uiFrameComponent.m_dataStrings)
	{
		dataStr.second.m_text->setFillColor({ 255,255,255 });
		dataStr.second.m_text->setOutlineColor({ 128,128,128 });
		dataStr.second.m_text->setFont(s_font);
		drawable.m_drawables[ECS_Core::Components::DrawLayer::MENU][255].push_back({dataStr.second.m_text, dataStr.second.m_relativePosition});
	}
}

void Government::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	switch (phase)
	{
	case GameLoopPhase::INPUT:
		InterpretLocalInput(m_managerRef);
		break;
	case GameLoopPhase::ACTION:
		GainIncomes(m_managerRef);
		break;
	case GameLoopPhase::PREPARATION:
	case GameLoopPhase::ACTION_RESPONSE:
	case GameLoopPhase::RENDER:
		break;
	case GameLoopPhase::CLEANUP:
		using namespace ECS_Core;
		m_managerRef.forEntitiesMatching<Signatures::S_Governor>([&manager = m_managerRef](const ecs::EntityIndex& entity,
			Components::C_Realm& realm,
			const Components::C_ResourceInventory&,
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