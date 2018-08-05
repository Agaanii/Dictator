//-----------------------------------------------------------------------------
// All code is property of Dictator Developers Inc
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2018

// Systems/CaravanTrade.cpp
// When a Caravan is at the end of a route, perform the trade and turn around

#include "../Core/typedef.h"

#include "Systems.h"

#include "../ECS/System.h"
#include "../ECS/ECS.h"

#include <algorithm>

void CaravanTrade::ProgramInit() {}
void CaravanTrade::SetupGameplay() {}

enum class TradeType
{
	PREFER_EXCHANGE,
	HIGHEST_AVAILABLE
};

void PerformTrade(
	ECS_Core::Components::C_ResourceInventory& caravanInventory,
	ECS_Core::Components::C_ResourceInventory& buildingInventory, 
	ECS_Core::Manager& manager,
	TradeType tradeType)
{
	using namespace ECS_Core;
	// Drop off everything but some food
	for (auto&& resource : caravanInventory.m_collectedYields)
	{
		auto amountToMove = resource.first == Components::Yields::FOOD ?
			(resource.second > 100 ? (resource.second - 100) : 0)
			: resource.second;
		buildingInventory.m_collectedYields[resource.first] += amountToMove;
		resource.second -= amountToMove;
	}
	// Then take the most available yields in the inventory of the base, preferring those not carried back this way
	std::vector<std::pair<ECS_Core::Components::YieldType, f64>> heldResources;
	for (auto&& resource : buildingInventory.m_collectedYields)
	{
		heldResources.push_back(resource);
	}
	if (tradeType == TradeType::PREFER_EXCHANGE)
	{
		std::sort(
			heldResources.begin(),
			heldResources.end(),
			[&caravanInventory](auto& left, auto& right) -> bool {
				if (caravanInventory.m_collectedYields.count(left.first) && !caravanInventory.m_collectedYields.count(right.first))
				{
					return false;
				}
				if (!caravanInventory.m_collectedYields.count(left.first) && caravanInventory.m_collectedYields.count(right.first))
				{
					return true;
				}
				if (left.second < right.second) return false;
				if (left.second > right.second) return true;
				return left.first < right.first;
			});
	}
	else
	{
		std::sort(
			heldResources.begin(),
			heldResources.end(),
			[](auto& left, auto& right) -> bool {
				if (left.second < right.second) return false;
				if (left.second > right.second) return true;
				return left.first < right.first;
			});
	}
	// Reset inventory
	auto remainingFood = caravanInventory.m_collectedYields[Components::Yields::FOOD];
	caravanInventory.m_collectedYields.clear();
	auto foodToTake = min<f64>(100 - remainingFood, buildingInventory.m_collectedYields[Components::Yields::FOOD]);
	buildingInventory.m_collectedYields[Components::Yields::FOOD] -= foodToTake;
	caravanInventory.m_collectedYields[Components::Yields::FOOD] = remainingFood + foodToTake;
	auto resourcesMoved{ 0 };
	for (auto&& resource : heldResources)
	{
		auto amountToMove = min<f64>(100, resource.second);
		caravanInventory.m_collectedYields[resource.first] = amountToMove;
		buildingInventory.m_collectedYields[resource.first] -= amountToMove;
		if (++resourcesMoved >= 3)
		{
			break;
		}
	}
}

void CaravanTrade::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	using namespace ECS_Core;
	switch (phase)
	{
	case GameLoopPhase::ACTION_RESPONSE:
		m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_CaravanUnit>([&manager = m_managerRef](
			const ecs::EntityIndex&,
			const Components::C_TilePosition& position,
			Components::C_MovingUnit& mover,
			Components::C_ResourceInventory& inventory,
			const Components::C_Population&,
			Components::C_CaravanPath& path)
		{
			if (position.m_position == path.m_basePath.m_path.front().m_tile
				&& path.m_isReturning)
			{
				// Trade with home base and turn around
				if (!mover.m_currentMovement
					|| !std::holds_alternative<Components::MoveToPoint>(*mover.m_currentMovement))
				{
					return ecs::IterationBehavior::CONTINUE;
				}
				if (!manager.hasComponent<Components::C_ResourceInventory>(path.m_originBuildingHandle))
				{
					return ecs::IterationBehavior::CONTINUE;
				}
				auto& baseInventory = manager.getComponent<Components::C_ResourceInventory>(path.m_originBuildingHandle);
				PerformTrade(inventory, baseInventory, manager, TradeType::HIGHEST_AVAILABLE);

				auto& moveToPoint = std::get<Components::MoveToPoint>(*mover.m_currentMovement);
				std::reverse(moveToPoint.m_path.begin(), moveToPoint.m_path.end());
				moveToPoint.m_currentPathIndex = 0;
				moveToPoint.m_currentMovementProgress = 0;
				moveToPoint.m_targetPosition = moveToPoint.m_path.back().m_tile;
				path.m_isReturning = false;
			}
			else if (position.m_position == path.m_basePath.m_path.back().m_tile
				&& !path.m_isReturning)
			{
				// Trade with target and turn around
				if (!mover.m_currentMovement
					|| !std::holds_alternative<Components::MoveToPoint>(*mover.m_currentMovement))
				{
					return ecs::IterationBehavior::CONTINUE;
				}
				if (!manager.hasComponent<Components::C_ResourceInventory>(path.m_targetBuildingHandle))
				{
					return ecs::IterationBehavior::CONTINUE;
				}
				PerformTrade(inventory, 
					manager.getComponent<Components::C_ResourceInventory>(path.m_targetBuildingHandle),
					manager,
					TradeType::PREFER_EXCHANGE);
				
				auto& moveToPoint = std::get<Components::MoveToPoint>(*mover.m_currentMovement);
				std::reverse(moveToPoint.m_path.begin(), moveToPoint.m_path.end());
				moveToPoint.m_currentPathIndex = 0;
				moveToPoint.m_currentMovementProgress = 0;
				moveToPoint.m_targetPosition = moveToPoint.m_path.back().m_tile;
				path.m_isReturning = true;
			}
			return ecs::IterationBehavior::CONTINUE;
		});
		break;
	case GameLoopPhase::PREPARATION:
	case GameLoopPhase::INPUT:
	case GameLoopPhase::ACTION:
	case GameLoopPhase::RENDER:
	case GameLoopPhase::CLEANUP:
		return;
	}
}

bool CaravanTrade::ShouldExit()
{
	return false;
}

DEFINE_SYSTEM_INSTANTIATION(CaravanTrade);