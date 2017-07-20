#pragma once

#include "../ecs/ecs.hpp"

#include "../Core/typedef.h"

#include "../Components/GraphicsComponents.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <optional>
#include <set>
#include <variant>

namespace ECS_Core
{
	namespace Components
	{
		struct C_PositionCartesian
		{
			C_PositionCartesian() {}
			C_PositionCartesian(f64 X, f64 Y, f64 Z) : m_position({ X, Y, Z }) {}
			CartesianVector3 m_position;
		};

		struct C_VelocityCartesian
		{
			CartesianVector3 m_velocity;
		};

		struct C_AccelerationCartesian
		{
			CartesianVector3 m_acceleration;
		};
		struct C_Health
		{
			C_Health() {}
			C_Health(int maxHealth) : m_maxHealth(maxHealth), m_currentHealth(maxHealth) {}
			f64 m_maxHealth{ 0 };
			f64 m_currentHealth{ 0 };
		};

		struct C_Healable
		{
			f64 m_healingThisFrame{ 0 };
			struct HealOverTime
			{
				f64 m_secondsRemaining{ 0 };
				f64 m_healingPerFrame{ 0 };
			};
			std::vector<HealOverTime> m_hots;
		};

		struct C_Damageable
		{
			f64 m_damageThisFrame{ 0 };
			struct DamageOverTime
			{
				DamageOverTime(f64 frames, f64 damagePerFrame)
					: m_secondsRemaining(frames)
					, m_damagePerFrame(damagePerFrame)
				{ }
				f64 m_secondsRemaining{ 0 };
				f64 m_damagePerFrame{ 0 };
			};
			std::vector<DamageOverTime> m_dots;
		};

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
			LEFT    = 1 << 0,
			RIGHT   = 1 << 1,
			MIDDLE  = 1 << 2,
			FOUR    = 1 << 3, // Mouse button indices make sense...
			FIVE    = 1 << 4,

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
				CartesianVector2 m_screenPosition;
				CartesianVector2 m_worldPosition;
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
			}

		private:
			std::set<InputKeys> m_processedCurrentKeys;

			u8 m_processedThisFrameDownMouseButtonFlags{ 0 };
			u8 m_processedThisFrameUpMouseButtonFlags{ 0 };
		};

		struct C_QuadrantPosition
		{
			int m_quadrantX{ 0 };
			int m_quadrantY{ 0 };
		};

		struct C_SectorPosition
		{
			// Quadrant parent
			int m_quadrantX{ 0 };
			int m_quadrantY{ 0 };

			int m_x{ 1 };
			int m_y{ 1 };
		};

		struct C_TilePosition 
		{
			C_TilePosition() {}
			C_TilePosition(int qx, int qy, int sx, int sy, int x, int y)
				: m_quadrantX(qx)
				, m_quadrantY(qy)
				, m_sectorX(sx)
				, m_sectorY(sy)
				, m_x(x)
				, m_y(y)
			{

			}

			// Default values are the center of the world
			// Quadrant parent
			int m_quadrantX{ 0 };
			int m_quadrantY{ 0 };

			// Sector within quadrant
			int m_sectorX{ 1 };
			int m_sectorY{ 1 };

			// Position within sector
			int m_x{ 0 };
			int m_y{ 0 };
		};

		struct C_TileProperties
		{
			int m_tileType;
			std::optional<int> m_movementCost; // If notset, unpathable
		};
	}

	namespace Tags
	{
		struct T_NoAcceleration {};
	}

	namespace Signatures
	{
		using S_ApplyConstantMotion = ecs::Signature<Components::C_PositionCartesian, Components::C_VelocityCartesian, Tags::T_NoAcceleration>;
		using S_ApplyNewtonianMotion = ecs::Signature<Components::C_PositionCartesian, Components::C_VelocityCartesian, Components::C_AccelerationCartesian>;
		using S_Drawable = ecs::Signature<Components::C_PositionCartesian, Components::C_SFMLDrawable>;
		using S_Living = ecs::Signature<Components::C_Health>;
		using S_Health = ecs::Signature<Components::C_Health, Components::C_Healable, Components::C_Damageable>;
	}

	using MasterComponentList = ecs::ComponentList<
		Components::C_PositionCartesian,
		Components::C_VelocityCartesian,
		Components::C_AccelerationCartesian,
		Components::C_SFMLDrawable,
		Components::C_Health,
		Components::C_Healable,
		Components::C_Damageable,
		Components::C_UserInputs,
		Components::C_TilePosition,
		Components::C_TileProperties
	>;

	using MasterTagList = ecs::TagList<Tags::T_NoAcceleration>;

	using MasterSignatureList = ecs::SignatureList<
		Signatures::S_ApplyConstantMotion,
		Signatures::S_ApplyNewtonianMotion,
		Signatures::S_Drawable,
		Signatures::S_Living,
		Signatures::S_Health
	>;

	using MasterSettings = ecs::Settings<MasterComponentList, MasterTagList, MasterSignatureList>;
	using Manager = ecs::Manager<MasterSettings>;
}
