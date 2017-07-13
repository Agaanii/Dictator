//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Systems/SystemTemplate.cpp
// System to spawn a bunch of squares, for testing rendering.

#include "System.h"
#include "Systems.h"

#include "ECS.h"

#include <SFML/Graphics.hpp>
#include <random>

#include <Windows.h>

struct RandomInitializer
{
	RandomInitializer()
	{
		LPSYSTEMTIME st;
		GetSystemTime(st);
		srand(st->wMilliseconds);
	}
};

timeuS usSinceLastSpawn = 0;

void ObjectSpammer::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	switch (phase)
	{
	case GameLoopPhase::PREPARATION:
	case GameLoopPhase::INPUT:
	case GameLoopPhase::RENDER:
	case GameLoopPhase::CLEANUP:
	case GameLoopPhase::ACTION_RESPONSE:
		return;
	case GameLoopPhase::ACTION:
		if ((usSinceLastSpawn += frameDuration) > 2000000)
		{
			usSinceLastSpawn = 0;
			auto newObject = m_managerRef.createIndex();
			auto& position = m_managerRef.addComponent<ECS_Core::Components::C_PositionCartesian>(newObject);
			position.m_position.m_x = 650;
			position.m_position.m_y = 300;
			auto& velocity = m_managerRef.addComponent<ECS_Core::Components::C_VelocityCartesian>(newObject);
			velocity.m_velocity.m_x = (rand() % 101) - 50;
			velocity.m_velocity.m_y = (rand() % 101) - 50;
			auto rect = std::make_unique<sf::RectangleShape>(sf::Vector2f(300, 300));
			rect->setFillColor({ (sf::Uint8)rand(), (sf::Uint8)rand(), (sf::Uint8)rand() });
			rect->setOutlineColor({ (sf::Uint8)rand(), (sf::Uint8)rand(), (sf::Uint8)rand() });
			rect->setOutlineThickness(4);
			auto& shape = m_managerRef.addComponent<ECS_Core::Components::C_SFMLDrawable>(
				newObject,
				std::move(rect),
				ECS_Core::Components::DrawLayer::TERRAIN,
				rand());
			m_managerRef.addTag<ECS_Core::Tags::T_NoAcceleration>(newObject);
			m_managerRef.addComponent<ECS_Core::Components::C_Health>(newObject, 10);
			m_managerRef.addComponent<ECS_Core::Components::C_Healable>(newObject);
			auto& damage = m_managerRef.addComponent<ECS_Core::Components::C_Damageable>(newObject);
			damage.m_dots.push_back({ 10, 1 });
		}
		break;
	}
}

bool ObjectSpammer::ShouldExit()
{
	return false;
}

DEFINE_SYSTEM_INSTANTIATION(ObjectSpammer);
