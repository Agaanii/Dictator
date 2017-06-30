//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Systems/SFMLManager.cpp
// Manages the drawing of all objects, and the SFML window in which the game lives.
// Future: Split into layers?

#include "System.h"

#include "ECS.h"

#include <SFML/Graphics.hpp>

namespace Graphics
{
	sf::RenderWindow s_window(sf::VideoMode(1600, 900), "Loesby is good at this shit.");
	bool closingTriggered = false;
	bool close = false;

	class SFMLManager : public SystemBase
	{
	public:
		SFMLManager() : SystemBase() { }
		virtual ~SFMLManager() {}
		virtual void Operate(GameLoopPhase phase, const timeuS& frameDuration) override
		{
			switch (phase)
			{
			case GameLoopPhase::PREPARATION:
			case GameLoopPhase::ACTION:
				return;
			case GameLoopPhase::INPUT:
				ReadSFMLInput(frameDuration);
				break;
			case GameLoopPhase::RENDER:
				RenderWorld(frameDuration);
				break;
			case GameLoopPhase::CLEANUP:
				close = closingTriggered;
				break;
			}
		}

		void ReadSFMLInput(const timeuS& frameDuration)
		{
			sf::Event event;
			if (s_window.pollEvent(event))
			{
				// TODO: Record inputs etc. into components
				if (event.type == sf::Event::Closed)
				{
					closingTriggered = true;
				}
			}
		}

		void RenderWorld(const timeuS& frameDuration)
		{
			s_window.clear();
			m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_Drawable>(
				[](
					ecs::EntityIndex mI,
					const ECS_Core::Components::C_SFMLShape& shape)
			{
				if (shape.m_shape)
				{
					s_window.draw(*shape.m_shape);
				}
			});
			s_window.display();
		}

		virtual bool ShouldExit() override
		{
			return close;
		}
	};

	SystemRegistrar<SFMLManager> registration;
}
