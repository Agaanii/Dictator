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
			UNIT,
			EFFECT,
			MENU,

			__COUNT
		};
		struct C_SFMLDrawable
		{
			C_SFMLDrawable() { }
			C_SFMLDrawable(
				std::unique_ptr<sf::Drawable>&& drawable,
				DrawLayer layer,
				u64 priority)
				: m_drawable(std::move(drawable))
				, m_drawLayer(layer)
				, m_priority(priority)
			{ }

			std::unique_ptr<sf::Drawable> m_drawable;
			DrawLayer m_drawLayer{ DrawLayer::BACKGROUND };
			u64 m_priority{ 0 };
		};
	}
}