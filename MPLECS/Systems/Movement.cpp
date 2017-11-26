//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Systems/SystemTemplate.cpp
// Moves all entities that can move and have a velocity

#include "../Core/typedef.h"

#include "Systems.h"

#include "../ECS/System.h"
#include "../ECS/ECS.h"

void NewtonianMovement::SetupGameplay() {}

void NewtonianMovement::Operate(GameLoopPhase phase, const timeuS& frameDuration) 
{
	if (phase != GameLoopPhase::ACTION)
	{
		// We only act during the action phase
		return;
	}
	f64 frameSeconds = (1. * frameDuration / 1000000);
	m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_ApplyConstantMotion>(
		[&frameSeconds](
			ecs::EntityIndex mI,
			ECS_Core::Components::C_PositionCartesian& position,
			const ECS_Core::Components::C_VelocityCartesian& velocity) {
		position.m_position += velocity.m_velocity * frameSeconds;
	});

	m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_ApplyNewtonianMotion>(
		[&frameSeconds](
			ecs::EntityIndex mI,
			ECS_Core::Components::C_PositionCartesian& position,
			ECS_Core::Components::C_VelocityCartesian& velocity,
			const ECS_Core::Components::C_AccelerationCartesian& acceleration) {
		position.m_position += (velocity.m_velocity * frameSeconds) + (acceleration.m_acceleration * frameSeconds * frameSeconds);
	});
}

bool NewtonianMovement::ShouldExit()
{
	return false;
}

DEFINE_SYSTEM_INSTANTIATION(NewtonianMovement);
