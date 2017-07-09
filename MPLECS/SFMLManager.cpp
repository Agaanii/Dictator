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
std::optional<sf::Font> s_font;

void ReadSFMLInput(ECS_Core::Manager& manager, const timeuS& frameDuration);
void ReceiveInput(ECS_Core::Manager& manager);
void RenderWorld(ECS_Core::Manager& manager, const timeuS& frameDuration);

namespace EventResponse
{
	using namespace sf;
	void OnWindowResize(const Event::SizeEvent& size);

	void OnLoseFocus();
	void OnGainFocus();
	
	void OnKeyDown(ECS_Core::Components::C_UserInputs& input, const Event::KeyEvent& key);
	void OnKeyUp(ECS_Core::Components::C_UserInputs& input, const Event::KeyEvent& key);
	
	void OnTextEntered(const Event::TextEvent& text);
	
	void OnMouseMove(ECS_Core::Components::C_UserInputs& input, const Event::MouseMoveEvent& move);

	void OnMouseEnter();
	void OnMouseLeave();
	
	void OnMouseButtonDown(ECS_Core::Components::C_UserInputs& input, const Event::MouseButtonEvent& button);
	void OnMouseButtonUp(ECS_Core::Components::C_UserInputs& input, const Event::MouseButtonEvent& button);

	void OnMouseWheelMove(ECS_Core::Components::C_UserInputs& input, const Event::MouseWheelEvent& wheel);
	void OnMouseWheelScroll(ECS_Core::Components::C_UserInputs& input, const Event::MouseWheelScrollEvent& scroll);

	void OnTouchBegin(const Event::TouchEvent& touch);
	void OnTouchMove(const Event::TouchEvent& touch);
	void OnTouchEnd(const Event::TouchEvent& touch);

	void OnSensor(const Event::SensorEvent& touch);

	void OnJoystickMove(const Event::JoystickMoveEvent& joyMove);

	void OnJoystickButtonDown(const Event::JoystickButtonEvent& joyButton);
	void OnJoystickButtonUp(const Event::JoystickButtonEvent& joyButton);

	void OnJoystickConnect(const Event::JoystickConnectEvent& joyConnect);
	void OnJoystickDisconnect(const Event::JoystickConnectEvent& joyDisconnect);

	std::optional<ecs::EntityIndex> s_inputObject;
}

std::optional<ECS_Core::Components::InputKeys> GetInputKey(sf::Keyboard::Key sfKey)
{
	using namespace ECS_Core::Components;
	switch (sfKey)
	{
	case sf::Keyboard::A: return InputKeys::A;
	case sf::Keyboard::B: return InputKeys::B;
	case sf::Keyboard::C: return InputKeys::C;
	case sf::Keyboard::D: return InputKeys::D;
	case sf::Keyboard::E: return InputKeys::E;
	case sf::Keyboard::F: return InputKeys::F;
	case sf::Keyboard::G: return InputKeys::G;
	case sf::Keyboard::H: return InputKeys::H;
	case sf::Keyboard::I: return InputKeys::I;
	case sf::Keyboard::J: return InputKeys::J;
	case sf::Keyboard::K: return InputKeys::K;
	case sf::Keyboard::L: return InputKeys::L;
	case sf::Keyboard::M: return InputKeys::M;
	case sf::Keyboard::N: return InputKeys::N;
	case sf::Keyboard::O: return InputKeys::O;
	case sf::Keyboard::P: return InputKeys::P;
	case sf::Keyboard::Q: return InputKeys::Q;
	case sf::Keyboard::R: return InputKeys::R;
	case sf::Keyboard::S: return InputKeys::S;
	case sf::Keyboard::T: return InputKeys::T;
	case sf::Keyboard::U: return InputKeys::U;
	case sf::Keyboard::V: return InputKeys::V;
	case sf::Keyboard::W: return InputKeys::W;
	case sf::Keyboard::X: return InputKeys::X;
	case sf::Keyboard::Y: return InputKeys::Y;
	case sf::Keyboard::Z: return InputKeys::Z;
	case sf::Keyboard::Num0: return InputKeys::ZERO;
	case sf::Keyboard::Num1: return InputKeys::ONE;
	case sf::Keyboard::Num2: return InputKeys::TWO;
	case sf::Keyboard::Num3: return InputKeys::THREE;
	case sf::Keyboard::Num4: return InputKeys::FOUR;
	case sf::Keyboard::Num5: return InputKeys::FIVE;
	case sf::Keyboard::Num6: return InputKeys::SIX;
	case sf::Keyboard::Num7: return InputKeys::SEVEN;
	case sf::Keyboard::Num8: return InputKeys::EIGHT;
	case sf::Keyboard::Num9: return InputKeys::NINE;
	case sf::Keyboard::Escape: return InputKeys::ESCAPE;
	case sf::Keyboard::LSystem: return InputKeys::WINDOWS;
	case sf::Keyboard::RSystem: return InputKeys::WINDOWS;
	case sf::Keyboard::Menu: return InputKeys::MENU;
	case sf::Keyboard::LBracket: return InputKeys::LEFT_SQUARE;
	case sf::Keyboard::RBracket: return InputKeys::RIGHT_SQUARE;
	case sf::Keyboard::SemiColon: return InputKeys::COLON;
	case sf::Keyboard::Comma: return InputKeys::COMMA;
	case sf::Keyboard::Period: return InputKeys::PERIOD;
	case sf::Keyboard::Quote: return InputKeys::QUOTE;
	case sf::Keyboard::Slash: return InputKeys::SLASH;
	case sf::Keyboard::BackSlash: return InputKeys::BACKSLASH;
	case sf::Keyboard::Tilde: return InputKeys::GRAVE;
	case sf::Keyboard::Equal: return InputKeys::EQUAL;
	case sf::Keyboard::Dash: return InputKeys::DASH;
	case sf::Keyboard::Space: return InputKeys::SPACE;
	case sf::Keyboard::Return: return InputKeys::ENTER;
	case sf::Keyboard::BackSpace: return InputKeys::BACKSPACE;
	case sf::Keyboard::Tab: return InputKeys::TAB;
	case sf::Keyboard::PageUp: return InputKeys::PAGE_UP;
	case sf::Keyboard::PageDown: return InputKeys::PAGE_DOWN;
	case sf::Keyboard::End: return InputKeys::END;
	case sf::Keyboard::Home: return InputKeys::HOME;
	case sf::Keyboard::Insert: return InputKeys::INSERT;
	case sf::Keyboard::Delete: return InputKeys::DELETE;
	case sf::Keyboard::Add: return InputKeys::NUM_PLUS;
	case sf::Keyboard::Subtract: return InputKeys::NUM_DASH;
	case sf::Keyboard::Multiply: return InputKeys::NUM_STAR;
	case sf::Keyboard::Divide: return InputKeys::NUM_SLASH;
	case sf::Keyboard::Left: return InputKeys::ARROW_LEFT;
	case sf::Keyboard::Right: return InputKeys::ARROW_RIGHT;
	case sf::Keyboard::Up: return InputKeys::ARROW_UP;
	case sf::Keyboard::Down: return InputKeys::ARROW_DOWN;
	case sf::Keyboard::Numpad0: return InputKeys::NUM_0;
	case sf::Keyboard::Numpad1: return InputKeys::NUM_1;
	case sf::Keyboard::Numpad2: return InputKeys::NUM_2;
	case sf::Keyboard::Numpad3: return InputKeys::NUM_3;
	case sf::Keyboard::Numpad4: return InputKeys::NUM_4;
	case sf::Keyboard::Numpad5: return InputKeys::NUM_5;
	case sf::Keyboard::Numpad6: return InputKeys::NUM_6;
	case sf::Keyboard::Numpad7: return InputKeys::NUM_7;
	case sf::Keyboard::Numpad8: return InputKeys::NUM_8;
	case sf::Keyboard::Numpad9: return InputKeys::NUM_9;
	case sf::Keyboard::F1: return InputKeys::F1;
	case sf::Keyboard::F2: return InputKeys::F2;
	case sf::Keyboard::F3: return InputKeys::F3;
	case sf::Keyboard::F4: return InputKeys::F4;
	case sf::Keyboard::F5: return InputKeys::F5;
	case sf::Keyboard::F6: return InputKeys::F6;
	case sf::Keyboard::F7: return InputKeys::F7;
	case sf::Keyboard::F8: return InputKeys::F8;
	case sf::Keyboard::F9: return InputKeys::F9;
	case sf::Keyboard::F10: return InputKeys::F10;
	case sf::Keyboard::F11: return InputKeys::F11;
	case sf::Keyboard::F12: return InputKeys::F12;
	case sf::Keyboard::F13: return InputKeys::F13;
	case sf::Keyboard::F14: return InputKeys::F14;
	case sf::Keyboard::F15: return InputKeys::F15;
	case sf::Keyboard::Pause: return InputKeys::PAUSE_BREAK;

	default:
		return std::nullopt;
	}
}

std::optional<ECS_Core::Components::Modifiers> GetInputModifier(sf::Keyboard::Key key)
{
	using namespace ECS_Core::Components;
	switch (key)
	{
	case sf::Keyboard::LControl:
	case sf::Keyboard::RControl:
		return Modifiers::CTRL;
	case sf::Keyboard::LAlt:
	case sf::Keyboard::RAlt:
		return Modifiers::ALT;
	case sf::Keyboard::LShift:
	case sf::Keyboard::RShift:
		return Modifiers::SHIFT;
	default:
		return std::nullopt;
	}
}

std::string GetInputKeyString(ECS_Core::Components::InputKeys key)
{
	using namespace ECS_Core::Components;
	switch (key)
	{
	case InputKeys::GRAVE:        return "GRAVE";
	case InputKeys::ONE:		  return "ONE";
	case InputKeys::TWO:		  return "TWO";
	case InputKeys::THREE:		  return "THREE";
	case InputKeys::FOUR:		  return "FOUR";
	case InputKeys::FIVE:		  return "FIVE";
	case InputKeys::SIX:		  return "SIX";
	case InputKeys::SEVEN:		  return "SEVEN";
	case InputKeys::EIGHT:		  return "EIGHT";
	case InputKeys::NINE:		  return "NINE";
	case InputKeys::ZERO:		  return "ZERO";
	case InputKeys::DASH:		  return "DASH";
	case InputKeys::EQUAL:		  return "EQUAL";
	case InputKeys::A:			  return "A";
	case InputKeys::B:			  return "B";
	case InputKeys::C:			  return "C";
	case InputKeys::D:			  return "D";
	case InputKeys::E:			  return "E";
	case InputKeys::F:			  return "F";
	case InputKeys::G:			  return "G";
	case InputKeys::H:			  return "H";
	case InputKeys::I:			  return "I";
	case InputKeys::J:			  return "J";
	case InputKeys::K:			  return "K";
	case InputKeys::L:			  return "L";
	case InputKeys::M:			  return "M";
	case InputKeys::N:			  return "N";
	case InputKeys::O:			  return "O";
	case InputKeys::P:			  return "P";
	case InputKeys::Q:			  return "Q";
	case InputKeys::R:			  return "R";
	case InputKeys::S:			  return "S";
	case InputKeys::T:			  return "T";
	case InputKeys::U:			  return "U";
	case InputKeys::V:			  return "V";
	case InputKeys::W:			  return "W";
	case InputKeys::X:			  return "X";
	case InputKeys::Y:			  return "Y";
	case InputKeys::Z:			  return "Z";
	case InputKeys::BACKSPACE:	  return "BACKSPACE";
	case InputKeys::LEFT_SQUARE:  return "LEFT_SQUARE";
	case InputKeys::RIGHT_SQUARE: return "RIGHT_SQUARE";
	case InputKeys::BACKSLASH:	  return "BACKSLASH";
	case InputKeys::PERIOD:		  return "PERIOD";
	case InputKeys::COMMA:		  return "COMMA";
	case InputKeys::SLASH:		  return "SLASH";
	case InputKeys::TAB:		  return "TAB";
	case InputKeys::CAPS_LOCK:	  return "CAPS_LOCK";
	case InputKeys::ENTER:		  return "ENTER";
	case InputKeys::SPACE:		  return "SPACE";
	case InputKeys::QUOTE:		  return "QUOTE";
	case InputKeys::COLON:		  return "COLON";
	case InputKeys::ESCAPE:		  return "ESCAPE";
	case InputKeys::F1:			  return "F1";
	case InputKeys::F2:			  return "F2";
	case InputKeys::F3:			  return "F3";
	case InputKeys::F4:			  return "F4";
	case InputKeys::F5:			  return "F5";
	case InputKeys::F6:			  return "F6";
	case InputKeys::F7:			  return "F7";
	case InputKeys::F8:			  return "F8";
	case InputKeys::F9:			  return "F9";
	case InputKeys::F10:		  return "F10";
	case InputKeys::F11:		  return "F11";
	case InputKeys::F12:		  return "F12";
	case InputKeys::F13:		  return "F13";
	case InputKeys::F14:		  return "F14";
	case InputKeys::F15:		  return "F15";
	case InputKeys::PRINT_SCREEN: return "PRINT_SCREEN";
	case InputKeys::SCROLL_LOCK:  return "SCROLL_LOCK";
	case InputKeys::PAUSE_BREAK:  return "PAUSE_BREAK";
	case InputKeys::INSERT:		  return "INSERT";
	case InputKeys::DELETE:		  return "DELETE";
	case InputKeys::HOME:		  return "HOME";
	case InputKeys::END:		  return "END";
	case InputKeys::PAGE_UP:	  return "PAGE_UP";
	case InputKeys::PAGE_DOWN:	  return "PAGE_DOWN";
	case InputKeys::ARROW_LEFT:	  return "ARROW_LEFT";
	case InputKeys::ARROW_RIGHT:  return "ARROW_RIGHT";
	case InputKeys::ARROW_UP:	  return "ARROW_UP";
	case InputKeys::ARROW_DOWN:	  return "ARROW_DOWN";
	case InputKeys::NUM_LOCK:	  return "NUM_LOCK";
	case InputKeys::NUM_SLASH:	  return "NUM_SLASH";
	case InputKeys::NUM_STAR:	  return "NUM_STAR";
	case InputKeys::NUM_DASH:	  return "NUM_DASH";
	case InputKeys::NUM_PLUS:	  return "NUM_PLUS";
	case InputKeys::NUM_ENTER:	  return "NUM_ENTER";
	case InputKeys::NUM_0:		  return "NUM_0";
	case InputKeys::NUM_1:		  return "NUM_1";
	case InputKeys::NUM_2:		  return "NUM_2";
	case InputKeys::NUM_3:		  return "NUM_3";
	case InputKeys::NUM_4:		  return "NUM_4";
	case InputKeys::NUM_5:		  return "NUM_5";
	case InputKeys::NUM_6:		  return "NUM_6";
	case InputKeys::NUM_7:		  return "NUM_7";
	case InputKeys::NUM_8:		  return "NUM_8";
	case InputKeys::NUM_9:		  return "NUM_9";
	case InputKeys::NUM_PERIOD:	  return "NUM_PERIOD";
	case InputKeys::WINDOWS:	  return "WINDOWS";
	case InputKeys::MENU:		  return "MENU";

	default:
		return "";
	}
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

void EventResponse::OnKeyDown(ECS_Core::Components::C_UserInputs& input, const sf::Event::KeyEvent& key)
{
	auto modifier = GetInputModifier(key.code);
	if (modifier)
	{
		input.ActivateModifier(*modifier);
		return;
	}
	auto inputKey = GetInputKey(key.code);
	input.m_newKeyDown.emplace(*inputKey);
	input.m_unprocessedCurrentKeys.emplace(*inputKey);
}

void EventResponse::OnKeyUp(ECS_Core::Components::C_UserInputs& input, const sf::Event::KeyEvent& key)
{
	auto modifier = GetInputModifier(key.code);
	if (modifier)
	{
		input.DeactivateModifier(*modifier);
		return;
	}
	auto inputKey = GetInputKey(key.code);
	input.m_newKeyUp.emplace(*inputKey);
	input.m_unprocessedCurrentKeys.erase(*inputKey);
}

void EventResponse::OnTextEntered(const sf::Event::TextEvent& text)
{
}

void EventResponse::OnMouseMove(ECS_Core::Components::C_UserInputs& input, const sf::Event::MouseMoveEvent& move)
{
}

void EventResponse::OnMouseEnter()
{
}

void EventResponse::OnMouseLeave()
{
}

void EventResponse::OnMouseButtonDown(ECS_Core::Components::C_UserInputs& input, const sf::Event::MouseButtonEvent& button)
{
}

void EventResponse::OnMouseButtonUp(ECS_Core::Components::C_UserInputs& input, const sf::Event::MouseButtonEvent& button)
{
}

void EventResponse::OnMouseWheelMove(ECS_Core::Components::C_UserInputs& input, const sf::Event::MouseWheelEvent& wheel)
{
}

void EventResponse::OnMouseWheelScroll(ECS_Core::Components::C_UserInputs& input, const sf::Event::MouseWheelScrollEvent& scroll)
{
	auto view = s_window.getView();
	view.zoom(1 - (scroll.delta / 20));
	s_window.setView(view);
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
	if (!EventResponse::s_inputObject)
	{
		EventResponse::s_inputObject = m_managerRef.createIndex();
		m_managerRef.addComponent<ECS_Core::Components::C_UserInputs>(*EventResponse::s_inputObject);
	}
	switch (phase)
	{
	case GameLoopPhase::PREPARATION:
		ReadSFMLInput(m_managerRef, frameDuration);
	case GameLoopPhase::ACTION:
	case GameLoopPhase::ACTION_RESPONSE:
		return;
	case GameLoopPhase::INPUT:
		ReceiveInput(m_managerRef);
		break;
	case GameLoopPhase::RENDER:
		RenderWorld(m_managerRef, frameDuration);
		break;
	case GameLoopPhase::CLEANUP:
	{
		auto& inputComponent = m_managerRef.getComponent<ECS_Core::Components::C_UserInputs>(*EventResponse::s_inputObject);
		inputComponent.Reset();
		close = closingTriggered;
	}
		break;
	}
}

void ReadSFMLInput(ECS_Core::Manager& manager, const timeuS& frameDuration)
{
	if (!s_font)
	{
		sf::Font tempFont;
		if (tempFont.loadFromFile("Assets/cour.ttf"))
		{
			s_font = tempFont;
		}
	}

	auto& inputComponent = manager.getComponent<ECS_Core::Components::C_UserInputs>(*EventResponse::s_inputObject);
	sf::Event event;
	while (s_window.pollEvent(event))
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
			EventResponse::OnKeyDown(inputComponent, event.key); break;
		case sf::Event::KeyReleased:
			EventResponse::OnKeyUp(inputComponent, event.key); break;

		case sf::Event::MouseWheelMoved:
			EventResponse::OnMouseWheelMove(inputComponent, event.mouseWheel); break;
		case sf::Event::MouseWheelScrolled:
			EventResponse::OnMouseWheelScroll(inputComponent, event.mouseWheelScroll); break;
		case sf::Event::MouseButtonPressed:
			EventResponse::OnMouseButtonDown(inputComponent, event.mouseButton); break;
		case sf::Event::MouseButtonReleased:
			EventResponse::OnMouseButtonUp(inputComponent, event.mouseButton); break;

		case sf::Event::MouseMoved:
			EventResponse::OnMouseMove(inputComponent, event.mouseMove); break;

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

void ReceiveInput(ECS_Core::Manager& manager)
{
	auto& inputComponent = manager.getComponent<ECS_Core::Components::C_UserInputs>(*EventResponse::s_inputObject);
	auto view = s_window.getView();
	if (inputComponent.m_unprocessedCurrentKeys.count(ECS_Core::Components::InputKeys::ARROW_DOWN))
	{
		view.move({ 0, 1 });
	}
	if (inputComponent.m_unprocessedCurrentKeys.count(ECS_Core::Components::InputKeys::ARROW_UP))
	{
		view.move({ 0, -1 });
	}
	if (inputComponent.m_unprocessedCurrentKeys.count(ECS_Core::Components::InputKeys::ARROW_LEFT))
	{
		view.move({ -1, 0 });
	}
	if (inputComponent.m_unprocessedCurrentKeys.count(ECS_Core::Components::InputKeys::ARROW_RIGHT))
	{
		view.move({ 1, 0 });
	}
	s_window.setView(view);
}

void DisplayCurrentInputs(const ECS_Core::Components::C_UserInputs& inputComponent)
{
	if (s_font)
	{
		sf::Text modifierText, newDownText, newUpText, currentDepressedText;
		std::vector<sf::Text*> texts{ &modifierText, &newDownText, &newUpText, &currentDepressedText };
		std::string modifierString = "";
		if (inputComponent.m_activeModifiers & (u8)ECS_Core::Components::Modifiers::CTRL)
		{
			modifierString += "Control ";
		}
		if (inputComponent.m_activeModifiers & (u8)ECS_Core::Components::Modifiers::ALT)
		{
			modifierString += "Alt ";
		}
		if (inputComponent.m_activeModifiers & (u8)ECS_Core::Components::Modifiers::SHIFT)
		{
			modifierString += "Shift";
		}
		modifierText.setString(modifierString);

		std::string newUpStr = "";
		for (auto&& inputKey : inputComponent.m_newKeyUp)
		{
			if (newUpStr.size()) newUpStr += " ";
			newUpStr += GetInputKeyString(inputKey);
		}
		newUpText.setString(newUpStr);

		std::string newDownStr = "";
		for (auto&& inputKey : inputComponent.m_newKeyDown)
		{
			if (newDownStr.size()) newDownStr += " ";
			newDownStr += GetInputKeyString(inputKey);
		}
		newDownText.setString(newDownStr);

		std::string currentDepressedStr = "";
		for (auto&& inputKey : inputComponent.m_unprocessedCurrentKeys)
		{
			if (currentDepressedStr.size()) currentDepressedStr += " ";
			currentDepressedStr += GetInputKeyString(inputKey);
		}
		currentDepressedText.setString(currentDepressedStr);

		int row = 0;
		for (auto* text : texts)
		{
			text->setFont(*s_font);
			text->setPosition(0, 45.f * row++);
			text->setFillColor(sf::Color(255, 255, 255));
			text->setOutlineColor(sf::Color(15, 15, 15));
			s_window.draw(*text);
		}
	}
}

void RenderWorld(ECS_Core::Manager& manager, const timeuS& frameDuration)
{
	s_window.clear();
	auto& inputComponent = manager.getComponent<ECS_Core::Components::C_UserInputs>(*EventResponse::s_inputObject);
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
	DisplayCurrentInputs(inputComponent);
	s_window.display();
}

bool SFMLManager::ShouldExit()
{
	return close;
}

DEFINE_SYSTEM_INSTANTIATION(SFMLManager);
