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