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

void NewtonianMovement::ProgramInit() {}
void NewtonianMovement::SetupGameplay() {}

void NewtonianMovement::Operate(GameLoopPhase phase, const timeuS& frameDuration) 
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
	});

	m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_ApplyNewtonianMotion>(
		[&time](
			ecs::EntityIndex mI,
			ECS_Core::Components::C_PositionCartesian& position,
			ECS_Core::Components::C_VelocityCartesian& velocity,
			const ECS_Core::Components::C_AccelerationCartesian& acceleration) {
		position.m_position += ((velocity.m_velocity) + (acceleration.m_acceleration * time.m_frameDuration)) * time.m_frameDuration;
	});
}

bool NewtonianMovement::ShouldExit()
{
	return false;
}

DEFINE_SYSTEM_INSTANTIATION(NewtonianMovement);