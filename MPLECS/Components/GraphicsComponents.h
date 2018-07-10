//-----------------------------------------------------------------------------
// All code is property of Dictator Developers Inc
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2018

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
		struct AttachedDrawable
		{
			AttachedDrawable(const std::shared_ptr<sf::Drawable>& graphic, CartesianVector2<f64> offset)
				: m_graphic(graphic)
				, m_offset(offset)
			{}
			AttachedDrawable() = default;
			std::shared_ptr<sf::Drawable> m_graphic;
			CartesianVector2<f64> m_offset;
		};
		struct C_SFMLDrawable
		{
			// Each layer may have a set of drawables
			// They'll be drawn in priority order, low to high
			std::map<DrawLayer, std::map<u64 /*priority*/, std::vector<AttachedDrawable>>> m_drawables;
		};
	}
}