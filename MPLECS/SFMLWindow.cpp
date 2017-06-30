//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Systems/SFMLWindow.cpp
// Manages the drawing of all 2D objects
// Future: Split into layers?

#include "System.h"

#include <SFML/Graphics.hpp>

namespace Graphics
{
	// TODO: Move window creation and configuration out of this system?

	sf::RenderWindow s_window(sf::VideoMode(1600, 900), "Loesby is good at this shit.");

	class Draw2D : public SystemBase
	{
	public:
		Draw2D() : SystemBase() { }
		virtual ~Draw2D() {}
		virtual void Operate(const timeuS& frameDuration) override
		{

		}

		virtual bool ShouldExit() override
		{
			return false;
		}
	};

	SystemRegistrar<Draw2D, GameLoopPhase::RENDER> registration;
}