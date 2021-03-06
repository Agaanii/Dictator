//-----------------------------------------------------------------------------
// All code is property of Dictator Developers Inc
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2018

// Systems/DamageApplication.cpp
// Reads pending damage on all units that have health
// During the Action_Response phase, applies the damage to the units
// During the cleanup phase, clears any pending damage that should go away

#include "../Core/typedef.h"

#include "DamageApplication.h"

void DamageApplication::ProgramInit() {}
void DamageApplication::SetupGameplay() {}

void DamageApplication::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	switch (phase)
	{
	case GameLoopPhase::PREPARATION:
	case GameLoopPhase::INPUT:
	case GameLoopPhase::ACTION:
	case GameLoopPhase::RENDER:
		return;
	case GameLoopPhase::ACTION_RESPONSE:
		TakeDamage();
		break;
	case GameLoopPhase::CLEANUP:
		ClearPendingDamage();
		break;
	}
}

bool DamageApplication::ShouldExit()
{
	return false;
}

void DamageApplication::TakeDamage()
{
	// Get current time
	// Assume the first entity is the one that has a valid time
	auto timeEntities = m_managerRef.entitiesMatching<ECS_Core::Signatures::S_TimeTracker>();
	if (timeEntities.size() == 0)
	{
		return;
	}
	const auto& time = m_managerRef.getComponent<ECS_Core::Components::C_TimeTracker>(timeEntities.front());

	m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_Health>(
		[&time](ecs::EntityIndex mI,
			ECS_Core::Components::C_Health& health,
			ECS_Core::Components::C_Healable& healing,
			ECS_Core::Components::C_Damageable& damage)
	{
		f64 secondFraction = time.m_frameDuration;
		health.m_currentHealth += healing.m_healingThisFrame - damage.m_damageThisFrame;
		for (auto& hot : healing.m_hots)
		{
			auto hotSecondFraction = secondFraction;
			if (hot.m_secondsRemaining < secondFraction) hotSecondFraction = hot.m_secondsRemaining;
			health.m_currentHealth += hotSecondFraction * hot.m_healingPerFrame;
			hot.m_secondsRemaining -= hotSecondFraction;
		}
		for (auto& dot : damage.m_dots)
		{
			auto dotSecondFraction = secondFraction;
			if (dot.m_secondsRemaining < secondFraction) dotSecondFraction = dot.m_secondsRemaining;
			health.m_currentHealth -= dotSecondFraction * dot.m_damagePerFrame;
			dot.m_secondsRemaining -= dotSecondFraction;
		}
		health.m_currentHealth = (health.m_currentHealth < health.m_maxHealth) ? health.m_currentHealth : health.m_maxHealth;
		return ecs::IterationBehavior::CONTINUE;
	});
}

void DamageApplication::ClearPendingDamage()
{
	m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_Health>(
		[](ecs::EntityIndex mI,
			ECS_Core::Components::C_Health& health,
			ECS_Core::Components::C_Healable& healing,
			ECS_Core::Components::C_Damageable& damage)
	{
		healing.m_healingThisFrame = 0;
		damage.m_damageThisFrame = 0;
		std::sort(
			healing.m_hots.begin(),
			healing.m_hots.end(),
			[](const auto& leftHot, const auto& rightHot) { return leftHot.m_secondsRemaining > rightHot.m_secondsRemaining; });
		while (healing.m_hots.size() > 0
			&& healing.m_hots.back().m_secondsRemaining <= 0)
		{
			healing.m_hots.pop_back();
		}

		std::sort(
			damage.m_dots.begin(),
			damage.m_dots.end(),
			[](const auto& leftDot, const auto& rightDot) { return leftDot.m_secondsRemaining  > rightDot.m_secondsRemaining; });
		while (damage.m_dots.size() > 0
			&& damage.m_dots.back().m_secondsRemaining <= 0)
		{
			damage.m_dots.pop_back();
		}
		return ecs::IterationBehavior::CONTINUE;
	});
}
DEFINE_SYSTEM_INSTANTIATION(DamageApplication);