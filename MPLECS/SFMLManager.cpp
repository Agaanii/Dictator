//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Systems/SFMLManager.cpp
// Manages the drawing of all objects, and the SFML window in which the game lives.
// Also responsible for recording the SFML inputs for other modules to use.
// Future: Split into layers?

#include "System.h"
#include "Systems.h"

#include "ECS.h"

#include <SFML/Graphics.hpp>
#include <optional>

sf::RenderWindow s_window(sf::VideoMode(1600, 900), "Loesby is good at this shit.");
bool closingTriggered = false;
bool close = false;

void ReadSFMLInput(ECS_Core::Manager& manager, const timeuS& frameDuration);
void RenderWorld(ECS_Core::Manager& manager, const timeuS& frameDuration);

namespace EventResponse
{
	using namespace sf;
	void OnWindowResize(const Event::SizeEvent& size);

	void OnLoseFocus();
	void OnGainFocus();
	
	void OnKeyDown(const Event::KeyEvent& key);
	void OnKeyUp(const Event::KeyEvent& key);
	
	void OnTextEntered(const Event::TextEvent& text);
	
	void OnMouseMove(const Event::MouseMoveEvent& move);

	void OnMouseEnter();
	void OnMouseLeave();
	
	void OnMouseButtonDown(const Event::MouseButtonEvent& button);
	void OnMouseButtonUp(const Event::MouseButtonEvent& button);

	void OnMouseWheelMove(const Event::MouseWheelEvent& wheel);
	void OnMouseWheelScroll(const Event::MouseWheelScrollEvent& scroll);

	void OnTouchBegin(const Event::TouchEvent& touch);
	void OnTouchMove(const Event::TouchEvent& touch);
	void OnTouchEnd(const Event::TouchEvent& touch);

	void OnSensor(const Event::SensorEvent& touch);

	void OnJoystickMove(const Event::JoystickMoveEvent& joyMove);

	void OnJoystickButtonDown(const Event::JoystickButtonEvent& joyButton);
	void OnJoystickButtonUp(const Event::JoystickButtonEvent& joyButton);

	void OnJoystickConnect(const Event::JoystickConnectEvent& joyConnect);
	void OnJoystickDisconnect(const Event::JoystickConnectEvent& joyDisconnect);

	std::optional<ecs::HandleDataIndex> s_inputObjectHandle;
}

void EventResponse::OnWindowResize(const sf::Event::SizeEvent& size)
{ 
}

void EventResponse::OnLoseFocus()
{
}

void EventResponse::OnGainFocus()
{
}

void EventResponse::OnKeyDown(const sf::Event::KeyEvent& key)
{
}

void EventResponse::OnKeyUp(const sf::Event::KeyEvent& key)
{
}

void EventResponse::OnTextEntered(const sf::Event::TextEvent& text)
{
}

void EventResponse::OnMouseMove(const sf::Event::MouseMoveEvent& move)
{
}

void EventResponse::OnMouseEnter()
{
}

void EventResponse::OnMouseLeave()
{
}

void EventResponse::OnMouseButtonDown(const sf::Event::MouseButtonEvent& button)
{
}

void EventResponse::OnMouseButtonUp(const sf::Event::MouseButtonEvent& button)
{
}

void EventResponse::OnMouseWheelMove(const sf::Event::MouseWheelEvent& wheel)
{
}

void EventResponse::OnMouseWheelScroll(const sf::Event::MouseWheelScrollEvent& scroll)
{
}

void EventResponse::OnTouchBegin(const sf::Event::TouchEvent& touch)
{
}

void EventResponse::OnTouchMove(const sf::Event::TouchEvent& touch)
{
}

void EventResponse::OnTouchEnd(const sf::Event::TouchEvent& touch)
{
}

void EventResponse::OnSensor(const sf::Event::SensorEvent& touch)
{
}

void EventResponse::OnJoystickMove(const sf::Event::JoystickMoveEvent& joyMove)
{
}

void EventResponse::OnJoystickButtonDown(const sf::Event::JoystickButtonEvent& joyButton)
{
}

void EventResponse::OnJoystickButtonUp(const sf::Event::JoystickButtonEvent& joyButton)
{
}

void EventResponse::OnJoystickConnect(const sf::Event::JoystickConnectEvent& joyConnect)
{
}

void EventResponse::OnJoystickDisconnect(const sf::Event::JoystickConnectEvent& joyDisconnect)
{
}

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
		switch (event.type)
		{
		case sf::Event::Closed:
			closingTriggered = true;
			break;

		// Window Management
		case sf::Event::Resized:
			EventResponse::OnWindowResize(event.size); break;
		case sf::Event::LostFocus:
			EventResponse::OnLoseFocus(); break;
		case sf::Event::GainedFocus:
			EventResponse::OnGainFocus(); break;

		case sf::Event::TextEntered:
			EventResponse::OnTextEntered(event.text); break;

		case sf::Event::KeyPressed:
			EventResponse::OnKeyDown(event.key); break;
		case sf::Event::KeyReleased:
			EventResponse::OnKeyUp(event.key); break;

		case sf::Event::MouseWheelMoved:
			EventResponse::OnMouseWheelMove(event.mouseWheel); break;
		case sf::Event::MouseWheelScrolled:
			EventResponse::OnMouseWheelScroll(event.mouseWheelScroll); break;
		case sf::Event::MouseButtonPressed:
			EventResponse::OnMouseButtonDown(event.mouseButton); break;
		case sf::Event::MouseButtonReleased:
			EventResponse::OnMouseButtonUp(event.mouseButton); break;

		case sf::Event::MouseMoved:
			EventResponse::OnMouseMove(event.mouseMove); break;

		case sf::Event::MouseEntered:
			EventResponse::OnMouseEnter(); break;
		case sf::Event::MouseLeft:
			EventResponse::OnMouseLeave(); break;

		case sf::Event::TouchBegan:
			EventResponse::OnTouchBegin(event.touch); break;
		case sf::Event::TouchMoved:
			EventResponse::OnTouchMove(event.touch); break;
		case sf::Event::TouchEnded:
			EventResponse::OnTouchEnd(event.touch); break;

		case sf::Event::SensorChanged:
			EventResponse::OnSensor(event.sensor); break;

		case sf::Event::JoystickButtonPressed:
			EventResponse::OnJoystickButtonDown(event.joystickButton); break;
		case sf::Event::JoystickButtonReleased:
			EventResponse::OnJoystickButtonUp(event.joystickButton); break;
		case sf::Event::JoystickMoved:
			EventResponse::OnJoystickMove(event.joystickMove); break;
		case sf::Event::JoystickConnected:
			EventResponse::OnJoystickConnect(event.joystickConnect); break;
		case sf::Event::JoystickDisconnected:
			EventResponse::OnJoystickDisconnect(event.joystickConnect); break;

		case sf::Event::Count:
			// wat. This shouldn't ever happen, this isn't an action but a utility
			break;
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
