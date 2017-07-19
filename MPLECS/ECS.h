#pragma once

#include "ecs/ecs.hpp"

#include "Core/typedef.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <set>

namespace ECS_Core
{
	namespace Components
	{
		struct C_PositionCartesian
		{
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

		struct C_Health
		{
			C_Health() {}
			C_Health(int maxHealth) : m_maxHealth(maxHealth), m_currentHealth(maxHealth) {}
			f64 m_maxHealth{ 0 };
			f64 m_currentHealth{ 0 };
		};

		struct C_Healable
		{
			f64 m_healingThisFrame;
			struct HealOverTime
			{
				f64 m_secondsRemaining;
				f64 m_healingPerFrame;
			};
			std::vector<HealOverTime> m_hots;
		};

		struct C_Damageable
		{
			f64 m_damageThisFrame;
			struct DamageOverTime
			{
				DamageOverTime(f64 frames, f64 damagePerFrame)
					: m_secondsRemaining(frames)
					, m_damagePerFrame(damagePerFrame)
				{ }
				f64 m_secondsRemaining;
				f64 m_damagePerFrame;
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
		struct C_UserInputs
		{
			u8 m_activeModifiers;
			std::set<InputKeys> m_unprocessedCurrentKeys;
			std::set<InputKeys> m_newKeyDown;
			std::set<InputKeys> m_newKeyUp;

			// std::optional<

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

			void Reset()
			{
				m_newKeyDown.clear();
				m_newKeyUp.clear();
				for (auto& key : m_processedCurrentKeys)
				{
					m_unprocessedCurrentKeys.emplace(key);
				}
				m_processedCurrentKeys.clear();
			}

		private:
			std::set<InputKeys> m_processedCurrentKeys;
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
		Components::C_UserInputs
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
