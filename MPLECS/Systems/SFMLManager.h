//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// ECS/SFMLManager.h
// Template for the declaration of a System
// This is where any other functions would get put if needed

#include "../ECS/System.h"

#include <SFML/Graphics.hpp>

namespace std
{
	bool operator<(const std::weak_ptr<sf::Drawable>& left, const std::weak_ptr<sf::Drawable>& right);
	bool operator<(const ecs::Impl::HandleData& left, const ecs::Impl::HandleData& right);
}

class SFMLManager : public SystemBase
{
public:
	SFMLManager()
		: SystemBase()
		, m_window(sf::VideoMode(1600, 900), "Loesby is good at this.")
	{ }
	virtual ~SFMLManager() {}
	virtual void ProgramInit() override;
	virtual void SetupGameplay() override;
	virtual void Operate(GameLoopPhase phase, const timeuS& frameDuration) override;
	virtual bool ShouldExit() override;
protected:
	void ReadSFMLInput();
	void ReceiveInput(const timeuS& frameDuration);
	void RenderWorld(const timeuS& frameDuration);
	void DisplayCurrentInputs(
		const ECS_Core::Components::C_UserInputs& inputComponent,
		const timeuS& frameDuration);

	void OnWindowResize(const sf::Event::SizeEvent& size);

	void OnLoseFocus(ECS_Core::Components::C_UserInputs& input);
	void OnGainFocus();

	void OnKeyDown(ECS_Core::Components::C_UserInputs& input, const sf::Event::KeyEvent& key);
	void OnKeyUp(ECS_Core::Components::C_UserInputs& input, const sf::Event::KeyEvent& key);

	void OnTextEntered(const sf::Event::TextEvent& text);

	void OnMouseMove(ECS_Core::Components::C_UserInputs& input, const sf::Event::MouseMoveEvent& move);

	void UpdateMouseWorldPosition(ECS_Core::Components::C_UserInputs & input);

	void OnMouseEnter();
	void OnMouseLeave(ECS_Core::Components::C_UserInputs& input);

	void OnMouseButtonDown(ECS_Core::Components::C_UserInputs& input, const sf::Event::MouseButtonEvent& button);
	void OnMouseButtonUp(ECS_Core::Components::C_UserInputs& input, const sf::Event::MouseButtonEvent& button);

	void OnMouseWheelMove(ECS_Core::Components::C_UserInputs& input, const sf::Event::MouseWheelEvent& wheel);
	void OnMouseWheelScroll(ECS_Core::Components::C_UserInputs& input, const sf::Event::MouseWheelScrollEvent& scroll);

	void OnTouchBegin(const sf::Event::TouchEvent& touch);
	void OnTouchMove(const sf::Event::TouchEvent& touch);
	void OnTouchEnd(const sf::Event::TouchEvent& touch);

	void OnSensor(const sf::Event::SensorEvent& touch);

	void OnJoystickMove(const sf::Event::JoystickMoveEvent& joyMove);

	void OnJoystickButtonDown(const sf::Event::JoystickButtonEvent& joyButton);
	void OnJoystickButtonUp(const sf::Event::JoystickButtonEvent& joyButton);

	void OnJoystickConnect(const sf::Event::JoystickConnectEvent& joyConnect);
	void OnJoystickDisconnect(const sf::Event::JoystickConnectEvent& joyDisconnect);

	struct TaggedDrawable
	{
		bool m_drawnThisFrame;
		std::set<std::weak_ptr<sf::Drawable>> m_drawable;
	};

	sf::RenderWindow m_window;
	bool m_closingTriggered{ false };
	bool m_close{ false };
	sf::Vector2u m_mostRecentWindowSize;

	sf::View m_worldView{ { 0, 0, 1600, 900 } };
	sf::View m_UIView{ {0, 0, 1600, 900 } };

	std::map<ECS_Core::Components::DrawLayer, 
		std::map<u64, 
			std::map<ecs::Impl::HandleData, TaggedDrawable>>> 
		m_drawablesByLayer;
};
template <> std::unique_ptr<SFMLManager> InstantiateSystem();