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
	void AdvanceBuildingConstruction(ECS_Core::Manager& manager, const timeuS& frameDuration);
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
			manager.addComponent<ECS_Core::Components::C_YieldPotential>(mI);
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
		// Advance construction
		Buildings::AdvanceBuildingConstruction(m_managerRef, frameDuration);
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