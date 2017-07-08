//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Systems/SFMLManager.cpp
// Manages the drawing of all objects, and the SFML window in which the game lives.
// Future: Split into layers?

#include "System.h"
#include "Systems.h"

#include "ECS.h"

#include <SFML/Graphics.hpp>

sf::RenderWindow s_window(sf::VideoMode(1600, 900), "Loesby is good at this shit.");
bool closingTriggered = false;
bool close = false;


void ReadSFMLInput(ECS_Core::Manager& manager, const timeuS& frameDuration);
void RenderWorld(ECS_Core::Manager& manager, const timeuS& frameDuration);

void SFMLManager::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	switch (phase)
	{
	case GameLoopPhase::PREPARATION:
	case GameLoopPhase::ACTION:
	case GameLoopPhase::ACTION_RESPONSE:
		return;
	case GameLoopPhase::INPUT:
		ReadSFMLInput(m_managerRef, frameDuration);
		break;
	case GameLoopPhase::RENDER:
		RenderWorld(m_managerRef, frameDuration);
		break;
	case GameLoopPhase::CLEANUP:
		close = closingTriggered;
		break;
	}
}

void ReadSFMLInput(ECS_Core::Manager& manager, const timeuS& frameDuration)
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

void RenderWorld(ECS_Core::Manager& manager, const timeuS& frameDuration)
{
	s_window.clear();
	manager.forEntitiesMatching<ECS_Core::Signatures::S_Drawable>(
		[](
			ecs::EntityIndex mI,
			const ECS_Core::Components::C_PositionCartesian& position,
			ECS_Core::Components::C_SFMLShape& shape)
	{
		if (shape.m_shape)
		{
			shape.m_shape->setPosition({ static_cast<float>(position.m_position.m_x), static_cast<float>(position.m_position.m_y) });
			s_window.draw(*shape.m_shape);
		}
	});
	s_window.display();
}

bool SFMLManager::ShouldExit()
{
	return close;
}

DEFINE_SYSTEM_INSTANTIATION(SFMLManager);
