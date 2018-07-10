//-----------------------------------------------------------------------------
// All code is property of Dictator Developers Inc
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2018

// Components/InputComponents.h
// Components involved with interpreting user inputs

#pragma once

#include "../Core/typedef.h"

#include <map>
#include <memory>
#include <optional>
#include <set>

namespace ECS_Core
{
	namespace Components
	{
		enum class Modifiers
		{
			CTRL = 1 << 0,
			ALT = 1 << 1,
			SHIFT = 1 << 2
		};

		enum class InputKeys
		{
			GRAVE,
			ONE,
			TWO,
			THREE,
			FOUR,
			FIVE,
			SIX,
			SEVEN,
			EIGHT,
			NINE,
			ZERO,
			DASH,
			EQUAL,
			A,
			B,
			C,
			D,
			E,
			F,
			G,
			H,
			I,
			J,
			K,
			L,
			M,
			N,
			O,
			P,
			Q,
			R,
			S,
			T,
			U,
			V,
			W,
			X,
			Y,
			Z,
			BACKSPACE,
			LEFT_SQUARE,
			RIGHT_SQUARE,
			BACKSLASH,
			PERIOD,
			COMMA,
			SLASH,
			TAB,
			CAPS_LOCK,
			ENTER,
			SPACE,
			QUOTE,
			COLON,
			ESCAPE,
			F1,
			F2,
			F3,
			F4,
			F5,
			F6,
			F7,
			F8,
			F9,
			F10,
			F11,
			F12,
			F13,
			F14,
			F15,
			PRINT_SCREEN,
			SCROLL_LOCK,
			PAUSE_BREAK,
			INSERT,
			DELETE,
			HOME,
			END,
			PAGE_UP,
			PAGE_DOWN,
			ARROW_LEFT,
			ARROW_RIGHT,
			ARROW_UP,
			ARROW_DOWN,
			NUM_LOCK,
			NUM_SLASH,
			NUM_STAR,
			NUM_DASH,
			NUM_PLUS,
			NUM_ENTER,
			NUM_0,
			NUM_1,
			NUM_2,
			NUM_3,
			NUM_4,
			NUM_5,
			NUM_6,
			NUM_7,
			NUM_8,
			NUM_9,
			NUM_PERIOD,
			WINDOWS,
			MENU,

			__COUNT // Keep last
		};
		enum class MouseButtons
		{
			LEFT = 1 << 0,
			RIGHT = 1 << 1,
			MIDDLE = 1 << 2,
			FOUR = 1 << 3, // Mouse button indices make sense...
			FIVE = 1 << 4,

			_COUNT = 5
		};
		struct C_UserInputs
		{
			u8 m_activeModifiers{ 0 };
			std::set<InputKeys> m_unprocessedCurrentKeys;
			std::set<InputKeys> m_newKeyDown;
			std::set<InputKeys> m_newKeyUp;

			struct MousePosition
			{
				CartesianVector2<f64> m_screenPosition;
				CartesianVector2<f64> m_worldPosition;
				std::optional<TilePosition> m_tilePosition;
			};

			u8 m_unprocessedThisFrameDownMouseButtonFlags{ 0 };
			u8 m_unprocessedThisFrameUpMouseButtonFlags{ 0 };

			MousePosition m_currentMousePosition;
			struct MouseInitialClick
			{
				MousePosition m_position;
				u8 m_initialActiveModifiers{ 0 };
			};
			std::map<MouseButtons, MouseInitialClick> m_heldMouseButtonInitialPositions;

			void ActivateModifier(Modifiers modifier)
			{
				m_activeModifiers |= static_cast<u8>(modifier);
			}

			void DeactivateModifier(Modifiers modifier)
			{
				m_activeModifiers &= ~static_cast<u8>(modifier);
			}

			void ProcessKey(InputKeys key)
			{
				if (!m_unprocessedCurrentKeys.erase(key))
				{
					return;
				}
				m_processedCurrentKeys.emplace(key);
			}

			void ProcessMouseDown(MouseButtons button)
			{
				if (m_unprocessedThisFrameDownMouseButtonFlags & (u8)button)
				{
					m_processedThisFrameDownMouseButtonFlags |= (u8)button;
					m_unprocessedThisFrameDownMouseButtonFlags &= ~(u8)button;
				}
			}

			void ProcessMouseUp(MouseButtons button)
			{
				if (m_unprocessedThisFrameUpMouseButtonFlags & (u8)button)
				{
					m_processedThisFrameUpMouseButtonFlags |= (u8)button;
					m_unprocessedThisFrameUpMouseButtonFlags &= ~(u8)button;
				}
			}

			void Reset()
			{
				m_newKeyDown.clear();
				m_newKeyUp.clear();
				for (auto& key : m_processedCurrentKeys)
				{
					m_unprocessedCurrentKeys.emplace(key);
				}
				m_processedCurrentKeys.clear();

				for (int i = 0; i < (int)MouseButtons::_COUNT; ++i)
				{
					if ((m_unprocessedThisFrameUpMouseButtonFlags & (1 << i)) ||
						(m_processedThisFrameUpMouseButtonFlags & (1 << i)))
					{
						m_heldMouseButtonInitialPositions.erase(static_cast<MouseButtons>(1 << i));
					}
				}

				m_unprocessedThisFrameDownMouseButtonFlags = 0;
				m_processedThisFrameDownMouseButtonFlags = 0;
				m_unprocessedThisFrameUpMouseButtonFlags = 0;
				m_processedThisFrameUpMouseButtonFlags = 0;
			}

		private:
			std::set<InputKeys> m_processedCurrentKeys;

			u8 m_processedThisFrameDownMouseButtonFlags{ 0 };
			u8 m_processedThisFrameUpMouseButtonFlags{ 0 };
		};
	}
}