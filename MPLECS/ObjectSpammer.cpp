//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Systems/SystemTemplate.cpp
// System to spawn a bunch of squares, for testing rendering.

#include "System.h"

#include "ECS.h"

#include <SFML/Graphics.hpp>
#include <random>

#include <Windows.h>

namespace Graphics
{
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

	class ObjectSpammer : public SystemBase
	{
	public:
		ObjectSpammer() : SystemBase() { }
		virtual ~ObjectSpammer() {}
		virtual void Operate(GameLoopPhase phase, const timeuS& frameDuration) override
		{
			switch (phase)
			{
			case GameLoopPhase::PREPARATION:
			case GameLoopPhase::INPUT:
			case GameLoopPhase::RENDER:
			case GameLoopPhase::CLEANUP:
				return;
			case GameLoopPhase::ACTION:
				if ((usSinceLastSpawn += frameDuration) > 2000000)
				{
					usSinceLastSpawn = 0;
					auto newObject = m_managerRef.createIndex();
					auto& position = m_managerRef.addComponent<ECS_Core::Components::C_PositionCartesian>(newObject);
					position.m_position.m_x = 800;
					position.m_position.m_y = 450;
					auto& velocity = m_managerRef.addComponent<ECS_Core::Components::C_VelocityCartesian>(newObject);
					velocity.m_velocity.m_x = (rand() % 51) - 25;
					velocity.m_velocity.m_y = (rand() % 51) - 25;
					auto& shape = m_managerRef.addComponent<ECS_Core::Components::C_SFMLShape>(newObject);
					shape.m_shape = std::make_unique<sf::RectangleShape>(sf::Vector2f(300, 300));
					m_managerRef.addTag<ECS_Core::Tags::T_NoAcceleration>(newObject);
				}
				break;
			}
		}

		virtual bool ShouldExit() override
		{
			return false;
		}
	};

	// Uncomment to have the system created at program startup and registered in the main system registry
	SystemRegistrar<ObjectSpammer> registration;
}
