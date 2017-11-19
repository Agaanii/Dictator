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

void BeginBuildingConstruction(ECS_Core::Manager & manager, ecs::EntityIndex & ghostEntity)
{
	manager.delComponent<ECS_Core::Components::C_BuildingGhost>(ghostEntity);
	manager.addTag<ECS_Core::Tags::T_BuildingConstruction>(ghostEntity);
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

void InterpretLocalInput(ECS_Core::Manager& manager, ecs::Impl::Handle localGovernor)
{
	auto inputEntities = manager.entitiesMatching<ECS_Core::Signatures::S_Input>();
	if (inputEntities.size() == 0) return;
	// Should only be one input component, only one computer running this
	ECS_Core::Components::C_UserInputs& inputComponent = manager.getComponent<ECS_Core::Components::C_UserInputs>(inputEntities.front());
	
	// First try to place a pending building from local player
	if (inputComponent.m_unprocessedThisFrameDownMouseButtonFlags & (u8)ECS_Core::Components::MouseButtons::LEFT)
	{
		for (auto&& ghostEntity : manager.entitiesMatching<ECS_Core::Signatures::S_PlannedBuildingPlacement>())
		{
			auto& ghost = manager.getComponent<ECS_Core::Components::C_BuildingGhost>(ghostEntity);
			if (ghost.m_placingGovernor != localGovernor)
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
		CreateBuildingGhost(manager, localGovernor, *inputComponent.m_currentMousePosition.m_tilePosition);

		inputComponent.ProcessKey(ECS_Core::Components::InputKeys::B);
	}

	for (auto&& ghost : manager.entitiesMatching<ECS_Core::Signatures::S_PlannedBuildingPlacement>())
	{
		if (manager.getComponent<ECS_Core::Components::C_BuildingGhost>(ghost).m_placingGovernor == localGovernor)
		{
			manager.getComponent<ECS_Core::Components::C_TilePosition>(ghost) = *inputComponent.m_currentMousePosition.m_tilePosition;
		}
	}
}

void Government::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	switch (phase)
	{
	case GameLoopPhase::INPUT:
		InterpretLocalInput(m_managerRef, m_localPlayerGovernment);
		break;
	case GameLoopPhase::PREPARATION:
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