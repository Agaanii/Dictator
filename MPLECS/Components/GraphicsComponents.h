//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Components/GraphicsComponents.h
// All components needed to be able to draw an object to the screen in SFML

#pragma once

#include "../Core/typedef.h"

#include "SFML/Graphics.hpp"

#include <memory>

namespace ECS_Core
{
	namespace Components
	{
		enum class DrawLayer
		{
			BACKGROUND,
			TERRAIN,
			BUILDING,
			UNIT,
			EFFECT,
			MENU,

			__COUNT
		};
		struct C_SFMLDrawable
		{
			// Each layer may have a set of drawables
			// They'll be drawn in priority order, low to high
			std::map<DrawLayer, std::vector<std::pair<u64, std::shared_ptr<sf::Drawable>>>> m_drawables;
		};
	}
}