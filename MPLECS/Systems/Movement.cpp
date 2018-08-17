//-----------------------------------------------------------------------------
// All code is property of Dictator Developers Inc
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2018

// Systems/SystemTemplate.cpp
// Moves all entities that can move and have a velocity

#include "../Core/typedef.h"

#include "Systems.h"

#include "../ECS/System.h"
#include "../ECS/ECS.h"

void Movement::ProgramInit() {}
void Movement::SetupGameplay() {}

void Movement::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	if (phase != GameLoopPhase::ACTION)
	{
		// We only act during the action phase
		return;
	}
	// Get current time
	// Assume the first entity is the one that has a valid time
	auto timeEntities = m_managerRef.entitiesMatching<ECS_Core::Signatures::S_TimeTracker>();
	if (timeEntities.size() == 0)
	{
		return;
	}
	const auto& time = m_managerRef.getComponent<ECS_Core::Components::C_TimeTracker>(timeEntities.front());
	m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_ApplyConstantMotion>(
		[&time](
			ecs::EntityIndex mI,
			ECS_Core::Components::C_PositionCartesian& position,
			const ECS_Core::Components::C_VelocityCartesian& velocity) {
		position.m_position += velocity.m_velocity * time.m_frameDuration;
		return ecs::IterationBehavior::CONTINUE;
	});

	m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_ApplyNewtonianMotion>(
		[&time](
			ecs::EntityIndex mI,
			ECS_Core::Components::C_PositionCartesian& position,
			ECS_Core::Components::C_VelocityCartesian& velocity,
			const ECS_Core::Components::C_AccelerationCartesian& acceleration) {
		position.m_position += ((velocity.m_velocity) + (acceleration.m_acceleration * time.m_frameDuration)) * time.m_frameDuration;
		return ecs::IterationBehavior::CONTINUE;
	});

	m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_MovingUnit>(
		[&manager = m_managerRef, &time](
			ecs::EntityIndex mI,
			ECS_Core::Components::C_TilePosition& tilePosition,
			ECS_Core::Components::C_MovingUnit& mover,
			const ECS_Core::Components::C_Population&)
	{
		if (mover.m_currentMovement && std::holds_alternative<ECS_Core::Components::MoveToPoint>(*mover.m_currentMovement))
		{
			auto& pointMovement = std::get<ECS_Core::Components::MoveToPoint>(*mover.m_currentMovement);
			pointMovement.m_currentMovementProgress += time.m_frameDuration * mover.m_movementPerDay;
			while (pointMovement.m_currentMovementProgress >= pointMovement.m_path[pointMovement.m_currentPathIndex].m_movementCost)
			{
				pointMovement.m_currentMovementProgress -= pointMovement.m_path[pointMovement.m_currentPathIndex].m_movementCost;
				pointMovement.m_currentPathIndex = min<int>(++pointMovement.m_currentPathIndex, static_cast<int>(pointMovement.m_path.size()) - 1);
			}
			tilePosition.m_position = pointMovement.m_path[pointMovement.m_currentPathIndex].m_tile;
		}
		return ecs::IterationBehavior::CONTINUE;
	});
}

bool Movement::ShouldExit()
{
	return false;
}

DEFINE_SYSTEM_INSTANTIATION(Movement);