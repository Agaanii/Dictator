//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Systems/SystemTemplate.cpp
// The boilerplate system code, to ease new system creation

#include "System.h"
#include "Systems.h"

#include "ECS.h"

void TakeDamage(ECS_Core::Manager& manager);
void ClearPendingDamage(ECS_Core::Manager& manager);

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
		TakeDamage(m_managerRef);
		break;
	case GameLoopPhase::CLEANUP:
		ClearPendingDamage(m_managerRef);
		break;
	}
}

bool DamageApplication::ShouldExit()
{
	return false;
}

void TakeDamage(ECS_Core::Manager& manager)
{
	manager.forEntitiesMatching<ECS_Core::Signatures::S_Health>(
		[](ecs::EntityIndex mI,
			ECS_Core::Components::C_Health& health,
			ECS_Core::Components::C_Healable& healing,
			ECS_Core::Components::C_Damageable& damage)
	{
		health.m_currentHealth += healing.m_healingThisFrame - damage.m_damageThisFrame;
		for (auto& hot : healing.m_hots)
		{
			health.m_currentHealth += hot.m_healingPerFrame;
			--hot.m_framesRemaining;
		}
		for (auto& dot : damage.m_dots)
		{
			health.m_currentHealth -= dot.m_damagePerFrame;
			--dot.m_framesRemaining;
		}
		health.m_currentHealth = (health.m_currentHealth < health.m_maxHealth) ? health.m_currentHealth : health.m_maxHealth;
	});
}

void ClearPendingDamage(ECS_Core::Manager& manager)
{
	manager.forEntitiesMatching<ECS_Core::Signatures::S_Health>(
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
			[](const auto& leftHot, const auto& rightHot) { return leftHot.m_framesRemaining > rightHot.m_framesRemaining; });
		while (healing.m_hots.size() > 0
			&& healing.m_hots.back().m_framesRemaining <= 0)
		{
			healing.m_hots.pop_back();
		}

		std::sort(
			damage.m_dots.begin(),
			damage.m_dots.end(),
			[](const auto& leftDot, const auto& rightDot) { return leftDot.m_framesRemaining > rightDot.m_framesRemaining; });
		while (damage.m_dots.size() > 0
			&& damage.m_dots.back().m_framesRemaining <= 0)
		{
			damage.m_dots.pop_back();
		}
	});
}
DEFINE_SYSTEM_INSTANTIATION(DamageApplication);