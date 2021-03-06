//-----------------------------------------------------------------------------
// All code is property of Dictator Developers Inc
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2018

// Systems/BuildingCreation.cpp
// Tracks pending buildings which the user is trying to place (ghost)
// Places the buildings when the user chooses to do so (placement)
// Applies progress to buildings currently in progress (construction)

// During Action phase advances the construction of buildings, 
//	and creates any ghosts or placements as needed
// During Render phase, updates any building visuals as needed
// During the cleanup phase, checks any buildings that have health to see if they need to die

#include "../Core/typedef.h"

#include "BuildingCreation.h"

void BuildingCreation::AdvanceBuildingConstruction()
{
	// Get current time
	// Assume the first entity is the one that has a valid time
	auto timeEntities = m_managerRef.entitiesMatching<ECS_Core::Signatures::S_TimeTracker>();
	if (timeEntities.size() == 0)
	{
		return;
	}
	const auto& time = m_managerRef.getComponent<ECS_Core::Components::C_TimeTracker>(timeEntities.front());
	
	m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_DrawableConstructingBuilding>(
		[&manager = m_managerRef, &time](
			ecs::EntityIndex mI,
			ECS_Core::Components::C_BuildingDescription& building,
			ECS_Core::Components::C_TilePosition& buildingTilePosition,
			ECS_Core::Components::C_SFMLDrawable& drawable,
			ECS_Core::Components::C_BuildingConstruction& construction) -> ecs::IterationBehavior
	{
		if (!manager.isHandleValid(construction.m_placingGovernor))
		{
			manager.addTag<ECS_Core::Tags::T_Dead>(mI);
			return ecs::IterationBehavior::CONTINUE;
		}
		construction.m_buildingProgress += time.m_frameDuration / 30; 

		if (construction.m_buildingProgress >= 1.0)
		{
			construction.m_buildingProgress = 1.0;
			// We're done building, give it territory, put it in a realm, and, at the end of this function, remove the construction tag
			if (!manager.hasComponent<ECS_Core::Components::C_ResourceInventory>(mI))
			{
				manager.addComponent<ECS_Core::Components::C_ResourceInventory>(mI);
			}
			manager.addComponent<ECS_Core::Components::C_TileProductionPotential>(mI);
			auto& territory = manager.addComponent<ECS_Core::Components::C_Territory>(mI);
			territory.m_ownedTiles.insert(buildingTilePosition.m_position);
			if (!manager.hasComponent<ECS_Core::Components::C_Population>(mI))
			{
				auto& population = manager.addComponent<ECS_Core::Components::C_Population>(mI);
				auto& foundingPopulation = population.m_populations[-12 * (time.m_year - 15) - time.m_month];
				foundingPopulation.m_numMen = 25;
				foundingPopulation.m_numWomen = 25;
				foundingPopulation.m_class = ECS_Core::Components::PopulationClass::WORKERS;
			}
			auto& creatorRealm = manager.getComponent<ECS_Core::Components::C_Realm>(construction.m_placingGovernor);
			creatorRealm.m_territories.insert(manager.getHandle(mI));
			if (!creatorRealm.m_capitol)
			{
				creatorRealm.m_capitol = manager.getHandle(mI);
			}

			// Must delete components last, since this invalidates the reference
			manager.delComponent<ECS_Core::Components::C_BuildingConstruction>(mI);
		}

		auto alphaFloat = min(1.0, 0.1 + (0.9 * construction.m_buildingProgress));
		auto& buildingDrawables = drawable.m_drawables[ECS_Core::Components::DrawLayer::BUILDING];
		for (auto& [priority, graphics] : buildingDrawables)
		{
			for (auto&& drawable : graphics)
			{
				sf::Shape* shapeDrawable = dynamic_cast<sf::Shape*>(drawable.m_graphic.get());
				if (shapeDrawable)
				{
					shapeDrawable->setFillColor({ 128, 128, 128, static_cast<sf::Uint8>(alphaFloat * 255) });
				}
			}
		}
		return ecs::IterationBehavior::CONTINUE;
	});
}

void BuildingCreation::ProgramInit() {}
void BuildingCreation::SetupGameplay() {}

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
		AdvanceBuildingConstruction();
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