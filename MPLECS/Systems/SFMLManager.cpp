//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Systems/SFMLManager.cpp
// Manages the drawing of all objects, and the SFML window in which the game lives.
// Also responsible for recording the SFML inputs for other modules to use.

#include "../Core/typedef.h"

#include "Systems.h"

#include "../ECS/System.h"
#include "../ECS/ECS.h"

#include <SFML/Graphics.hpp>
#include <optional>

sf::RenderWindow s_window(sf::VideoMode(1600, 900), "Loesby is good at this.");
bool closingTriggered = false;
bool close = false;
sf::Font s_font;
sf::Vector2u s_mostRecentWindowSize;

sf::View s_worldView({ 0, 0, 1600, 900 });
sf::View s_UIView({ 0, 0, 1600, 900 });

void ReadSFMLInput(ECS_Core::Manager& manager);
void ReceiveInput(ECS_Core::Manager& manager, const timeuS& frameDuration);
void RenderWorld(ECS_Core::Manager& manager, const timeuS& frameDuration);

namespace EventResponse
{
	using namespace sf;
	void OnWindowResize(const Event::SizeEvent& size, ECS_Core::Manager& manager);

	void OnLoseFocus(ECS_Core::Components::C_UserInputs& input);
	void OnGainFocus();
	
	void OnKeyDown(ECS_Core::Components::C_UserInputs& input, const Event::KeyEvent& key);
	void OnKeyUp(ECS_Core::Components::C_UserInputs& input, const Event::KeyEvent& key);
	
	void OnTextEntered(const Event::TextEvent& text);
	
	void OnMouseMove(ECS_Core::Components::C_UserInputs& input, const Event::MouseMoveEvent& move);

	void UpdateMouseWorldPosition(ECS_Core::Components::C_UserInputs & input);

	void OnMouseEnter();
	void OnMouseLeave(ECS_Core::Components::C_UserInputs& input);
	
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

ECS_Core::Components::MouseButtons GetMouseButton(sf::Mouse::Button button)
{
	switch (button)
	{
	case sf::Mouse::Button::Left: return ECS_Core::Components::MouseButtons::LEFT;
	case sf::Mouse::Button::Right: return ECS_Core::Components::MouseButtons::RIGHT;
	case sf::Mouse::Button::Middle: return ECS_Core::Components::MouseButtons::MIDDLE;
	case sf::Mouse::Button::XButton1: return ECS_Core::Components::MouseButtons::FOUR;
	case sf::Mouse::Button::XButton2: return ECS_Core::Components::MouseButtons::FIVE;
	}
	return ECS_Core::Components::MouseButtons::_COUNT;
}

namespace sf
{
	Vector2f operator/(const Vector2f& vector, int divisor)
	{
		auto returnVal = vector;
		returnVal.x /= divisor;
		returnVal.y /= divisor;
		return returnVal;
	}
}

void EventResponse::OnWindowResize(const sf::Event::SizeEvent& size, ECS_Core::Manager& manager)
{ 
	auto& currentViewSize = s_worldView.getSize();
	auto newViewSize = currentViewSize;
	newViewSize.x *= 1.f * size.width / s_mostRecentWindowSize.x;
	newViewSize.y *= 1.f * size.height / s_mostRecentWindowSize.y;

	auto currentViewOrigin = s_worldView.getCenter() - (currentViewSize / 2);
	s_worldView = sf::View({ currentViewOrigin.x, currentViewOrigin.y, newViewSize.x, newViewSize.y});

	s_UIView = sf::View({ 0, 0, static_cast<float>(size.width), static_cast<float>(size.height) });

	// update window info
	for (auto&& index : manager.entitiesMatching<ECS_Core::Signatures::S_WindowInfo>())
	{
		auto& windowInfo = manager.getComponent<ECS_Core::Components::C_WindowInfo>(index);

		windowInfo.m_windowSize = CartesianVector2<unsigned int>{ s_window.getSize().x, s_window.getSize().y }.cast<f64>();
	}
}

void EventResponse::OnLoseFocus(ECS_Core::Components::C_UserInputs& input)
{
	input.m_activeModifiers = 0;
	input.m_newKeyUp = input.m_unprocessedCurrentKeys;
	input.m_unprocessedCurrentKeys.clear();
}

void EventResponse::OnGainFocus()
{
}

void EventResponse::OnKeyDown(
	ECS_Core::Components::C_UserInputs& input,
	const sf::Event::KeyEvent& key)
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

void EventResponse::OnKeyUp(
	ECS_Core::Components::C_UserInputs& input,
	const sf::Event::KeyEvent& key)
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

void EventResponse::OnMouseMove(
	ECS_Core::Components::C_UserInputs& input, 
	const sf::Event::MouseMoveEvent& move)
{
	input.m_currentMousePosition.m_screenPosition.m_x = move.x;
	input.m_currentMousePosition.m_screenPosition.m_y = move.y;

	UpdateMouseWorldPosition(input);
}

void EventResponse::UpdateMouseWorldPosition(ECS_Core::Components::C_UserInputs & input)
{
	auto worldViewCenter = s_worldView.getCenter();
	auto worldViewSize = s_worldView.getSize();

	// UI view matches window size in pixels
	auto xPercent = 1.0 * input.m_currentMousePosition.m_screenPosition.m_x / s_UIView.getSize().x;
	auto yPercent = 1.0 * input.m_currentMousePosition.m_screenPosition.m_y / s_UIView.getSize().y;
	input.m_currentMousePosition.m_worldPosition.m_x = ((xPercent - 0.5f) * worldViewSize.x) + worldViewCenter.x;
	input.m_currentMousePosition.m_worldPosition.m_y = ((yPercent - 0.5f) * worldViewSize.y) + worldViewCenter.y;
}

void EventResponse::OnMouseEnter()
{
}

void EventResponse::OnMouseLeave(
	ECS_Core::Components::C_UserInputs& input)
{
	// When mouse leaves the window, treat all mouse buttons as lifting
	// This will avoid annoying snapping for anything that uses mouse position change
	// And replace it with annoying "god dammit I accidentally left the screen"
	// I should probably implement fullscreen and mouse bounding
	for (auto& mouseButton : input.m_heldMouseButtonInitialPositions)
	{
		input.m_unprocessedThisFrameUpMouseButtonFlags |=
			static_cast<u8>(mouseButton.first);
	}
}

void EventResponse::OnMouseButtonDown(
	ECS_Core::Components::C_UserInputs& input,
	const sf::Event::MouseButtonEvent& button)
{
	// Position and Active Modifiers technically have a race condition here
	// Since we don't know which order the events will come in
	// But it's not noticeable to the user
	// Since we clear mouse inputs when the mouse leaves the screen
	auto mouseButton = GetMouseButton(button.button);
	input.m_unprocessedThisFrameDownMouseButtonFlags |= static_cast<u8>(mouseButton);
	auto& initialPosition = input.m_heldMouseButtonInitialPositions[mouseButton];
	initialPosition.m_position = input.m_currentMousePosition;
	initialPosition.m_initialActiveModifiers = input.m_activeModifiers;
}

void EventResponse::OnMouseButtonUp(
	ECS_Core::Components::C_UserInputs& input,
	const sf::Event::MouseButtonEvent& button)
{
	auto mouseButton = GetMouseButton(button.button);
	input.m_unprocessedThisFrameUpMouseButtonFlags |= static_cast<u8>(mouseButton);
}

void EventResponse::OnMouseWheelMove(
	ECS_Core::Components::C_UserInputs& input,
	const sf::Event::MouseWheelEvent& wheel)
{
}

void EventResponse::OnMouseWheelScroll(
	ECS_Core::Components::C_UserInputs& input,
	const sf::Event::MouseWheelScrollEvent& scroll)
{
	s_worldView.zoom(1 - (scroll.delta / 20));
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

void SFMLManager::ProgramInit() 
{
	auto windowInfoIndex = m_managerRef.createIndex();
	auto& windowInfo = m_managerRef.addComponent<ECS_Core::Components::C_WindowInfo>(windowInfoIndex);
	windowInfo.m_windowSize = CartesianVector2<unsigned int>{ s_window.getSize().x, s_window.getSize().y }.cast<f64>();
}

void SFMLManager::SetupGameplay()
{
	sf::Font tempFont;
	if (tempFont.loadFromFile("Assets/cour.ttf"))
	{
		s_font = tempFont;
	}
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
		ReadSFMLInput(m_managerRef);
	case GameLoopPhase::ACTION:
	case GameLoopPhase::ACTION_RESPONSE:
		return;
	case GameLoopPhase::INPUT:
		ReceiveInput(m_managerRef, frameDuration);
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
	s_mostRecentWindowSize = s_window.getSize();
}

void ReadSFMLInput(ECS_Core::Manager& manager)
{
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
			EventResponse::OnWindowResize(event.size, manager); break;
		case sf::Event::LostFocus:
			EventResponse::OnLoseFocus(inputComponent); break;
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
			EventResponse::OnMouseLeave(inputComponent); break;

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

void ReceiveInput(ECS_Core::Manager& manager, const timeuS& frameDuration)
{
	auto& inputComponent = manager.getComponent<ECS_Core::Components::C_UserInputs>(*EventResponse::s_inputObject);
	if (inputComponent.m_unprocessedCurrentKeys.count(ECS_Core::Components::InputKeys::ARROW_DOWN))
	{
		s_worldView.move({ 0, 150.f * frameDuration / 1000000 });
		inputComponent.ProcessKey(ECS_Core::Components::InputKeys::ARROW_DOWN);
	}
	if (inputComponent.m_unprocessedCurrentKeys.count(ECS_Core::Components::InputKeys::ARROW_UP))
	{
		s_worldView.move({ 0, -150.f * frameDuration / 1000000 });
		inputComponent.ProcessKey(ECS_Core::Components::InputKeys::ARROW_UP);
	}
	if (inputComponent.m_unprocessedCurrentKeys.count(ECS_Core::Components::InputKeys::ARROW_LEFT))
	{
		s_worldView.move({ -150.f * frameDuration / 1000000, 0 });
		inputComponent.ProcessKey(ECS_Core::Components::InputKeys::ARROW_LEFT);
	}
	if (inputComponent.m_unprocessedCurrentKeys.count(ECS_Core::Components::InputKeys::ARROW_RIGHT))
	{
		s_worldView.move({ 150.f * frameDuration / 1000000, 0 });
		inputComponent.ProcessKey(ECS_Core::Components::InputKeys::ARROW_RIGHT);
	}
	EventResponse::UpdateMouseWorldPosition(inputComponent);
}

void DisplayCurrentInputs(const ECS_Core::Components::C_UserInputs& inputComponent, const timeuS& frameDuration)
{
	sf::Text modifierText, newDownText, newUpText, currentDepressedText, windowPositionText, worldPositionText, worldCoordinatesText, frameDurationText;
	std::vector<sf::Text*> texts{
		&modifierText,
		&newDownText,
		&newUpText,
		&currentDepressedText,
		&windowPositionText,
		&worldPositionText,
		&worldCoordinatesText,
		&frameDurationText
	};
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

	std::string windowMousePosStr =
		"Window: X=" + std::to_string(inputComponent.m_currentMousePosition.m_screenPosition.m_x) +
		", Y=" + std::to_string(inputComponent.m_currentMousePosition.m_screenPosition.m_y);
	windowPositionText.setString(windowMousePosStr);

	std::string worldMousePosStr =
		"World: X=" + std::to_string(inputComponent.m_currentMousePosition.m_worldPosition.m_x) +
		", Y=" + std::to_string(inputComponent.m_currentMousePosition.m_worldPosition.m_y);
	worldPositionText.setString(worldMousePosStr);

	if (inputComponent.m_currentMousePosition.m_tilePosition)
	{
		std::string mouseWorldCoordinatesStr = "Tile: " + std::to_string(inputComponent.m_currentMousePosition.m_tilePosition->m_quadrantCoords.m_x)
			+ "." + std::to_string(inputComponent.m_currentMousePosition.m_tilePosition->m_quadrantCoords.m_y)
			+ ":" + std::to_string(inputComponent.m_currentMousePosition.m_tilePosition->m_sectorCoords.m_x)
			+ "." + std::to_string(inputComponent.m_currentMousePosition.m_tilePosition->m_sectorCoords.m_y)
			+ ":" + std::to_string(inputComponent.m_currentMousePosition.m_tilePosition->m_coords.m_x)
			+ "." + std::to_string(inputComponent.m_currentMousePosition.m_tilePosition->m_coords.m_y);
		worldCoordinatesText.setString(mouseWorldCoordinatesStr);
	}

	std::string frameDurationStr = "FrameDuration: " + std::to_string(frameDuration) + " uS. FPS = " + std::to_string(1000000. / frameDuration);
	frameDurationText.setString(frameDurationStr);

	int row = 0;
	for (auto* text : texts)
	{
		text->setFont(s_font);
		text->setPosition(0, 300.f + 45.f * row++);
		text->setFillColor(sf::Color(255, 255, 255));
		text->setOutlineColor(sf::Color(15, 15, 15));
		s_window.draw(*text);
	}
}

namespace std
{
	bool operator<(const std::weak_ptr<sf::Drawable>& left, const std::weak_ptr<sf::Drawable>& right)
	{
		if (left.expired() && !right.expired())
		{
			return true;
		}
		if (right.expired() && !left.expired())
		{
			return false;
		}
		return left.owner_before(right);
	}
}

struct TaggedDrawable
{
	bool m_drawnThisFrame;
	std::set<std::weak_ptr<sf::Drawable>> m_drawable;
};

namespace std
{
	bool operator<(const ecs::Impl::HandleData& left, const ecs::Impl::HandleData& right)
	{
		if (left.entityIndex < right.entityIndex) return true;
		if (right.entityIndex < left.entityIndex) return false;

		if (left.counter < right.counter) return true;
		return false;
	}
}

std::map<ECS_Core::Components::DrawLayer, std::map<u64, std::map<ecs::Impl::HandleData, TaggedDrawable>>> drawablesByLayer;
void RenderWorld(ECS_Core::Manager& manager, const timeuS& frameDuration)
{
	s_window.clear();

	// Get current time
	// Assume the first entity is the one that has a valid time
	auto timeEntities = manager.entitiesMatching<ECS_Core::Signatures::S_TimeTracker>();
	if (timeEntities.size() == 0)
	{
		return;
	}
	const auto& time = manager.getComponent<ECS_Core::Components::C_TimeTracker>(timeEntities.front());

	manager.forEntitiesMatching<ECS_Core::Signatures::S_Drawable>(
		[&manager](
			ecs::EntityIndex mI,
			const ECS_Core::Components::C_PositionCartesian& position,
			ECS_Core::Components::C_SFMLDrawable& drawables)
	{
		auto handle = manager.getHandleData(mI);
		for (auto&& layer : drawables.m_drawables)
		{
			for (auto&& priority : layer.second)
			{
				for (auto&& drawable : priority.second)
				{
					auto* transform = dynamic_cast<sf::Transformable*>(drawable.m_graphic.get());
					if (transform)
					{
						transform->setPosition({
							static_cast<float>(position.m_position.m_x + drawable.m_offset.m_x),
							static_cast<float>(position.m_position.m_y + drawable.m_offset.m_y) });
					}
					auto& taggedDrawable = drawablesByLayer[layer.first][priority.first][handle];
					taggedDrawable.m_drawable.insert(drawable.m_graphic);
					taggedDrawable.m_drawnThisFrame = true;
				}
			}
		}
	});
	manager.forEntitiesMatching<ECS_Core::Signatures::S_UIDrawable>([&manager](
		ecs::EntityIndex mI,
		ECS_Core::Components::C_UIFrame& uiFrame,
		ECS_Core::Components::C_SFMLDrawable& drawables)
	{
		auto handle = manager.getHandleData(mI);
		for (auto&& layer : drawables.m_drawables)
		{
			for (auto&& priority : layer.second)
			{
				for (auto&& drawable : priority.second)
				{
					auto* transform = dynamic_cast<sf::Transformable*>(drawable.m_graphic.get());
					if (transform)
					{
						transform->setPosition({
							static_cast<float>(uiFrame.m_topLeftCorner.m_x + drawable.m_offset.m_x),
							static_cast<float>(uiFrame.m_topLeftCorner.m_y + drawable.m_offset.m_y) });
					}
					auto& taggedDrawable = drawablesByLayer[layer.first][priority.first][handle];
					taggedDrawable.m_drawable.insert(drawable.m_graphic);
					taggedDrawable.m_drawnThisFrame = true;
				}
			}
		}
	});
	for (auto& layerMap : drawablesByLayer)
	{
		if (layerMap.first == ECS_Core::Components::DrawLayer::MENU)
		{
			s_window.setView(s_UIView);
		}
		else
		{
			s_window.setView(s_worldView);
		}
		for (auto& handleMap : layerMap.second)
		{
			std::vector<ecs::Impl::HandleData> handlesToRemove;
			for (auto& drawable : handleMap.second)
			{
				if (drawable.second.m_drawnThisFrame)
				{
					for (auto&& weakGraphicIter = drawable.second.m_drawable.begin(); weakGraphicIter != drawable.second.m_drawable.end();)
					{
						if (weakGraphicIter->expired())
						{
							drawable.second.m_drawable.erase(weakGraphicIter++);
						}
						else
						{
							auto graphic = weakGraphicIter->lock();
							s_window.draw(*graphic);
							++weakGraphicIter;
						}
					}
				}
				else
				{
					handlesToRemove.push_back(drawable.first);
				}
			}
			for (auto& handle : handlesToRemove)
			{
				handleMap.second.erase(handle);
			}
		}
	}

	s_window.setView(s_UIView);
	DisplayCurrentInputs(manager.getComponent<ECS_Core::Components::C_UserInputs>(*EventResponse::s_inputObject), frameDuration);
	s_window.display();
}

bool SFMLManager::ShouldExit()
{
	return close;
}

DEFINE_SYSTEM_INSTANTIATION(SFMLManager);
