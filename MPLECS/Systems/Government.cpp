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

void CreateBuildingGhost(ECS_Core::Manager & manager, ecs::Impl::Handle &governor, TilePosition & position)
{
	auto ghostEntity = manager.createIndex();
	manager.addComponent<ECS_Core::Components::C_BuildingGhost>(ghostEntity).m_placingGovernor = governor;
	manager.addComponent<ECS_Core::Components::C_TilePosition>(ghostEntity) = position;
	manager.addComponent<ECS_Core::Components::C_PositionCartesian>(ghostEntity);
	manager.addComponent<ECS_Core::Components::C_BuildingDescription>(ghostEntity);

	auto& drawable = manager.addComponent<ECS_Core::Components::C_SFMLDrawable>(ghostEntity);
	auto shape = std::make_shared<sf::CircleShape>(2.5f);
	shape->setOutlineColor(sf::Color(128, 128, 128, 255));
	shape->setFillColor(sf::Color(128, 128, 128, 128));
	drawable.m_drawables[ECS_Core::Components::DrawLayer::UNIT][0].push_back({ shape,{ 0,0 } });
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
		CreateBuildingGhost(manager, playerGovernorHandle, *inputComponent.m_currentMousePosition.m_tilePosition);

		inputComponent.ProcessKey(ECS_Core::Components::InputKeys::B);
	}

	for (auto&& ghost : manager.entitiesMatching<ECS_Core::Signatures::S_PlannedBuildingPlacement>())
	{
		if (manager.getComponent<ECS_Core::Components::C_BuildingGhost>(ghost).m_placingGovernor == playerGovernorHandle)
		{
			manager.getComponent<ECS_Core::Components::C_TilePosition>(ghost) = *inputComponent.m_currentMousePosition.m_tilePosition;
		}
	}
}

void GainIncomes(ECS_Core::Manager& manager, const timeuS& frameDuration)
{
	manager.forEntitiesMatching<ECS_Core::Signatures::S_Governor>(
		[&manager, &frameDuration](
			ecs::EntityIndex mI,
			ECS_Core::Components::C_Realm& realm,
			ECS_Core::Components::C_ResourceInventory& inventory)
	{
		for (auto&& territoryHandle : realm.m_territories)
		{
			if (!manager.isHandleValid(territoryHandle) || !manager.hasComponent<ECS_Core::Components::C_YieldPotential>(territoryHandle))
			{
				continue;
			}

			auto& territoryYield = manager.getComponent<ECS_Core::Components::C_YieldPotential>(territoryHandle);
			for (auto&& yield : territoryYield.m_availableYields)
			{
				yield.second.m_productionProgress += 1. * frameDuration / 1000000;
				if (yield.second.m_productionProgress > yield.second.m_productionInterval)
				{
					yield.second.m_productionProgress -= yield.second.m_productionInterval;
					inventory.m_collectedYields[yield.first] += yield.second.m_value;
				}
			}
			inventory.m_yield0 = inventory.m_collectedYields[1];
		}
	});
}

void Government::SetupGameplay()
{
	auto localPlayerGovernment = m_managerRef.createHandle();
	m_managerRef.addComponent<ECS_Core::Components::C_Realm>(localPlayerGovernment);
	m_managerRef.addComponent<ECS_Core::Components::C_ResourceInventory>(localPlayerGovernment);

	m_managerRef.addComponent<ECS_Core::Components::C_UIFrame>(localPlayerGovernment).m_frame
		= DefineUIFrame(
			"Inventory",
			"Food: {0:0}\n"
			"Lumber: {0:1}\n"
			"Fresh Water: {0:2}",
			DataBinding(ECS_Core::Components::C_ResourceInventory, m_collectedYields));
	m_managerRef.addTag<ECS_Core::Tags::T_LocalPlayer>(localPlayerGovernment);
}

void Government::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	switch (phase)
	{
	case GameLoopPhase::INPUT:
		InterpretLocalInput(m_managerRef);
		break;
	case GameLoopPhase::ACTION:
		GainIncomes(m_managerRef, frameDuration);
		break;
	case GameLoopPhase::PREPARATION:
	case GameLoopPhase::ACTION_RESPONSE:
	case GameLoopPhase::RENDER:
		break;
	case GameLoopPhase::CLEANUP:
		for (auto&& governor : m_managerRef.entitiesMatching<ECS_Core::Signatures::S_Governor>())
		{
			auto& realm = m_managerRef.getComponent<ECS_Core::Components::C_Realm>(governor);
			auto& managerRef = m_managerRef;
			for (auto iter = realm.m_territories.begin(); iter != realm.m_territories.end();)
			{
				if (managerRef.isHandleValid(*iter))
				{
					++iter;
				}
				else
				{
					realm.m_territories.erase(iter++);
				}
			}
		}
		return;
	}
}

bool Government::ShouldExit()
{
	return false;
}

DEFINE_SYSTEM_INSTANTIATION(Government);