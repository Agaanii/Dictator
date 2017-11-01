//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Systems/BuildingCreation.cpp
// Tracks pending buildings which the user is trying to place (ghost)
// Places the buildings when the user chooses to do so (placement)
// Applies progress to buildings currently in progress (construction)

// During Action phase advances the construction of buildings, 
//	and creates any ghosts or placements as needed
// During Render phase, updates any building visuals as needed
// During the cleanup phase, checks any buildings that have health to see if they need to die

#include "../Core/typedef.h"

#include "Systems.h"

#include "../ECS/System.h"
#include "../ECS/ECS.h"

namespace Buildings
{
	std::optional<ecs::EntityIndex> s_ghostEntity;
	void AdvanceBuildingConstruction(ECS_Core::Manager& manager, const timeuS& frameDuration);
	void CheckCreatePlacements(ECS_Core::Manager& manager);
	void CheckCreateGhosts(ECS_Core::Manager& manager);
}

template<class T>
T min(T&& a, T&& b)
{
	return a < b ? a : b;
}

void Buildings::AdvanceBuildingConstruction(ECS_Core::Manager& manager, const timeuS& frameDuration)
{
	manager.forEntitiesMatching<ECS_Core::Signatures::S_DrawableConstructingBuilding>(
		[&manager, frameDuration](
			ecs::EntityIndex mI,
			ECS_Core::Components::C_BuildingDescription& building,
			ECS_Core::Components::C_TilePosition& buildingTilePosition,
			ECS_Core::Components::C_SFMLDrawable& drawable)
	{
		building.m_buildingProgress += (1.0 * frameDuration / (30 * 1000000));

		if (building.m_buildingProgress >= 1.0)
		{
			building.m_buildingProgress = 1.0;
			// We're done building, remove the construction tag and give it territory
			manager.delTag<ECS_Core::Tags::T_BuildingConstruction>(mI);
			manager.addComponent<ECS_Core::Components::C_Territory>(mI).m_ownedTiles.insert(buildingTilePosition.m_position);
		}

		auto alphaFloat = min(1.0, 0.1 + (0.9 * building.m_buildingProgress));
		auto& buildingDrawables = drawable.m_drawables[ECS_Core::Components::DrawLayer::BUILDING];
		for (auto& prio : buildingDrawables)
		{
			for (auto&& drawable : prio.second)
			{
				sf::Shape* shapeDrawable = dynamic_cast<sf::Shape*>(drawable.m_graphic.get());
				if (shapeDrawable)
				{
					shapeDrawable->setFillColor({ 128, 128, 128, static_cast<sf::Uint8>(alphaFloat * 255) });
				}
			}
		}
	});
}

void Buildings::CheckCreatePlacements(ECS_Core::Manager& manager)
{
	if (!s_ghostEntity) return;

	auto inputEntities = manager.entitiesMatching<ECS_Core::Signatures::S_Input>();
	if (inputEntities.size() == 0) return;
	ECS_Core::Components::C_UserInputs& inputComponent = manager.getComponent<ECS_Core::Components::C_UserInputs>(inputEntities.front());

	// Update position of the ghost
	manager.getComponent<ECS_Core::Components::C_TilePosition>(*s_ghostEntity) = *inputComponent.m_currentMousePosition.m_tilePosition;

	if (inputComponent.m_unprocessedThisFrameDownMouseButtonFlags & (u8)ECS_Core::Components::MouseButtons::LEFT)
	{
		auto& ghost = manager.getComponent<ECS_Core::Components::C_BuildingGhost>(*s_ghostEntity);
		if (!ghost.m_currentPlacementValid)
		{
			// TODO: Surface error
			return;
		}
		manager.delComponent<ECS_Core::Components::C_BuildingGhost>(*s_ghostEntity);
		manager.addTag<ECS_Core::Tags::T_BuildingConstruction>(*s_ghostEntity);
		auto& drawable = manager.getComponent<ECS_Core::Components::C_SFMLDrawable>(*s_ghostEntity);
		for (auto& prio : drawable.m_drawables[ECS_Core::Components::DrawLayer::BUILDING])
		{
			for (auto&& building : prio.second)
			{
				sf::Shape* shapeDrawable = dynamic_cast<sf::Shape*>(building.m_graphic.get());
				if (shapeDrawable)
					shapeDrawable->setFillColor(sf::Color(128, 128, 128, 26));
			}
		}
		s_ghostEntity.reset();
		
		inputComponent.ProcessMouseDown(ECS_Core::Components::MouseButtons::LEFT);
	}
}

void Buildings::CheckCreateGhosts(ECS_Core::Manager& manager)
{
	if (s_ghostEntity) return;
	auto inputEntities = manager.entitiesMatching<ECS_Core::Signatures::S_Input>();
	if (inputEntities.size() == 0) return;
	ECS_Core::Components::C_UserInputs& inputComponent = manager.getComponent<ECS_Core::Components::C_UserInputs>(inputEntities.front());

	if (inputComponent.m_newKeyDown.count(ECS_Core::Components::InputKeys::B))
	{
		s_ghostEntity = manager.createIndex();
		manager.addComponent<ECS_Core::Components::C_BuildingGhost>(*s_ghostEntity);
		manager.addComponent<ECS_Core::Components::C_TilePosition>(*s_ghostEntity) = *inputComponent.m_currentMousePosition.m_tilePosition;
		manager.addComponent<ECS_Core::Components::C_PositionCartesian>(*s_ghostEntity);
		manager.addComponent<ECS_Core::Components::C_BuildingDescription>(*s_ghostEntity); // TODO: Mapping input key to building type
		auto& drawable = manager.addComponent<ECS_Core::Components::C_SFMLDrawable>(*s_ghostEntity);
		auto shape = std::make_shared<sf::CircleShape>(2.5f);
		shape->setOutlineColor(sf::Color(128, 128, 128, 255));
		shape->setFillColor(sf::Color(128, 128, 128, 128));
		drawable.m_drawables[ECS_Core::Components::DrawLayer::UNIT][0].push_back({ shape, {0,0} });

		inputComponent.ProcessKey(ECS_Core::Components::InputKeys::B);
	}
}

void BuildingCreation::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	switch (phase)
	{
	case GameLoopPhase::PREPARATION:
	case GameLoopPhase::INPUT:
	case GameLoopPhase::ACTION_RESPONSE:
		// no-op
		break;

	case GameLoopPhase::ACTION:
		// Advance construction, then create ghosts and placements
		// Construction does not advance on the cycle of creation

		// Advance Construction
		Buildings::AdvanceBuildingConstruction(m_managerRef, frameDuration);

		// Create ghosts and placements
		Buildings::CheckCreatePlacements(m_managerRef);
		Buildings::CheckCreateGhosts(m_managerRef);
		break;
	case GameLoopPhase::RENDER:
		// Update building visuals

		break;
	case GameLoopPhase::CLEANUP:
		// Kill any buildings that have run out of health
		break;
	}
}

bool BuildingCreation::ShouldExit()
{
	return false;
}

DEFINE_SYSTEM_INSTANTIATION(BuildingCreation);