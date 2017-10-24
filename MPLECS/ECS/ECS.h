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
			CartesianVector3<f64> m_position;
		};

		struct C_VelocityCartesian
		{
			CartesianVector3<f64> m_velocity;
		};

		struct C_AccelerationCartesian
		{
			CartesianVector3<f64> m_acceleration;
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

		using CoordinateVector2 = CartesianVector2<s64>;
		struct C_QuadrantPosition
		{
			C_QuadrantPosition() {}
			C_QuadrantPosition(const CoordinateVector2& c) : m_coords(c) { }
			template<typename NUM_TYPE>
			C_QuadrantPosition(NUM_TYPE x, NUM_TYPE y)
				: m_coords(x, y)
			{}
			CoordinateVector2 m_coords;
		};

		struct C_SectorPosition
		{
			C_SectorPosition() {}
			C_SectorPosition(const CoordinateVector2& qc, const CoordinateVector2& c)
				: m_quadrantCoords(qc)
				, m_coords(c)
			{ }

			template<typename NUM_TYPE>
			C_SectorPosition(NUM_TYPE qx, NUM_TYPE qy, NUM_TYPE x, NUM_TYPE y)
				: m_quadrantCoords(qx, qy)
				, m_coords(x, y)
			{}
			CoordinateVector2 m_quadrantCoords;
			CoordinateVector2 m_coords;
		};

		struct TilePosition
		{
			TilePosition() {}
			TilePosition(const CoordinateVector2& qc, const CoordinateVector2& sc, const CoordinateVector2& c)
				: m_quadrantCoords(qc)
				, m_sectorCoords(sc)
				, m_coords(c)
			{ }

			template<typename NUM_TYPE>
			TilePosition(NUM_TYPE qx, NUM_TYPE qy, NUM_TYPE sx, NUM_TYPE sy, NUM_TYPE x, NUM_TYPE y)
				: m_quadrantCoords(qx, qy)
				, m_sectorCoords(sx, sy)
				, m_coords(x, y)
			{}
			CoordinateVector2 m_quadrantCoords;
			CoordinateVector2 m_sectorCoords;
			CoordinateVector2 m_coords;

			bool operator==(const TilePosition& other) const
			{
				return m_quadrantCoords == other.m_quadrantCoords
					&& m_sectorCoords == other.m_sectorCoords
					&& m_coords == other.m_coords;
			}

			bool operator<(const TilePosition& other) const
			{
				if (m_quadrantCoords < other.m_quadrantCoords) return true;
				if (m_sectorCoords < other.m_sectorCoords) return true;
				if (m_coords < other.m_coords) return true;
				return false;
			}
		};

		struct C_TilePosition
		{
			C_TilePosition() = default;
			C_TilePosition(const TilePosition& position) : m_position(position) {}
			TilePosition m_position;
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
				std::optional<C_TilePosition> m_tilePosition;
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

		struct C_BuildingDescription
		{
			u64 m_buildingType{ 0 };
			f64 m_buildingProgress{ 0 };
		};

		struct C_BuildingGhost {
			bool m_currentPlacementValid{ false };
		};

		struct C_Territory
		{
			std::set<TilePosition> m_ownedTiles;
		};
	}

	namespace Tags
	{
		struct T_NoAcceleration {};
		struct T_BuildingConstruction {};
	}

	namespace Signatures
	{
		using S_ApplyConstantMotion = ecs::Signature<Components::C_PositionCartesian, Components::C_VelocityCartesian, Tags::T_NoAcceleration>;
		using S_ApplyNewtonianMotion = ecs::Signature<Components::C_PositionCartesian, Components::C_VelocityCartesian, Components::C_AccelerationCartesian>;
		using S_Drawable = ecs::Signature<Components::C_PositionCartesian, Components::C_SFMLDrawable>;
		using S_Living = ecs::Signature<Components::C_Health>;
		using S_Health = ecs::Signature<Components::C_Health, Components::C_Healable, Components::C_Damageable>;
		using S_Input = ecs::Signature<Components::C_UserInputs>;
		using S_TilePositionable = ecs::Signature<Components::C_PositionCartesian, Components::C_TilePosition>;
		using S_DrawableBuilding = ecs::Signature <Components::C_BuildingDescription, Components::C_SFMLDrawable>;
		using S_PlannedBuildingPlacement = ecs::Signature<Components::C_BuildingDescription, Components::C_TilePosition, Components::C_BuildingGhost>;
		using S_DrawableConstructingBuilding = ecs::Signature <Components::C_BuildingDescription, Components::C_TilePosition, Components::C_SFMLDrawable, Tags::T_BuildingConstruction>;
		using S_InProgressBuilding = ecs::Signature <Components::C_BuildingDescription, Components::C_TilePosition, Tags::T_BuildingConstruction>;
		using S_CompleteBuilding = ecs::Signature <Components::C_BuildingDescription, Components::C_TilePosition, Components::C_Territory>;
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
		Components::C_QuadrantPosition,
		Components::C_SectorPosition,
		Components::C_TilePosition,
		Components::C_BuildingGhost,
		Components::C_BuildingDescription,
		Components::C_Territory
	>;

	using MasterTagList = ecs::TagList<
		Tags::T_NoAcceleration,
		Tags::T_BuildingConstruction
	>;

	using MasterSignatureList = ecs::SignatureList<
		Signatures::S_ApplyConstantMotion,
		Signatures::S_ApplyNewtonianMotion,
		Signatures::S_Drawable,
		Signatures::S_Living,
		Signatures::S_Health,
		Signatures::S_Input,
		Signatures::S_TilePositionable,
		Signatures::S_DrawableBuilding,
		Signatures::S_DrawableConstructingBuilding,
		Signatures::S_InProgressBuilding,
		Signatures::S_CompleteBuilding,
		Signatures::S_PlannedBuildingPlacement
	>;

	using MasterSettings = ecs::Settings<MasterComponentList, MasterTagList, MasterSignatureList>;
	using Manager = ecs::Manager<MasterSettings>;
}
