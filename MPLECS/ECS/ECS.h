#pragma once

#include "../ecs/ecs.hpp"

#include "../Core/typedef.h"

#include "../Components/GraphicsComponents.h"
#include "../Components/InputComponents.h"

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

		struct C_TilePosition
		{
			C_TilePosition() = default;
			C_TilePosition(const TilePosition& position) : m_position(position) {}
			TilePosition m_position;
		};

		struct C_BuildingDescription
		{
			u64 m_buildingType{ 0 };
			f64 m_buildingProgress{ 0 };
		};

		struct C_BuildingGhost {
			bool m_currentPlacementValid{ false };
			ecs::Impl::Handle m_placingGovernor;
		};
		struct C_BuildingConstruction {
			ecs::Impl::Handle m_placingGovernor;
		};

		struct GrowthTile
		{
			f64 m_progress{ 0 };
			TilePosition m_tile;
		};

		struct C_Territory
		{
			std::set<TilePosition> m_ownedTiles;
			std::optional<GrowthTile> m_nextGrowthTile;
		};

		using YieldType = s32;
		struct Yield
		{
			f64 m_productionInterval{ 1 };
			f64 m_productionProgress{ 0 };
			s32 m_value{ 0 };
		};
		struct C_YieldPotential
		{
			std::map<YieldType, Yield> m_availableYields;
		};

		struct C_ResourceInventory
		{
			std::map<YieldType, s64> m_collectedYields;
		};

		struct C_Realm
		{
			std::set<ecs::Impl::Handle> m_subordinates;
			std::set<ecs::Impl::Handle> m_territories;
		};
	}

	namespace Tags
	{
		struct T_NoAcceleration {};
		struct T_Dead {};
		struct T_LocalPlayer {};
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
		using S_DrawableConstructingBuilding = ecs::Signature <Components::C_BuildingDescription, Components::C_TilePosition, Components::C_SFMLDrawable, Components::C_BuildingConstruction>;
		using S_InProgressBuilding = ecs::Signature <Components::C_BuildingDescription, Components::C_TilePosition, Components::C_BuildingConstruction>;
		using S_CompleteBuilding = ecs::Signature <Components::C_BuildingDescription, Components::C_TilePosition, Components::C_Territory, Components::C_YieldPotential>;
		using S_DestroyedBuilding = ecs::Signature<Components::C_BuildingDescription, Components::C_TilePosition, Tags::T_Dead>;
		using S_Governor = ecs::Signature<Components::C_Realm, Components::C_ResourceInventory>;
		using S_PlayerGovernor = ecs::Signature<Components::C_Realm, Components::C_ResourceInventory, Tags::T_LocalPlayer>;
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
		Components::C_Territory,
		Components::C_YieldPotential,
		Components::C_ResourceInventory,
		Components::C_Realm,
		Components::C_BuildingConstruction
	>;

	using MasterTagList = ecs::TagList<
		Tags::T_NoAcceleration,
		Tags::T_Dead,
		Tags::T_LocalPlayer
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
		Signatures::S_PlannedBuildingPlacement,
		Signatures::S_DestroyedBuilding,
		Signatures::S_Governor,
		Signatures::S_PlayerGovernor
	>;

	using MasterSettings = ecs::Settings<MasterComponentList, MasterTagList, MasterSignatureList>;
	using Manager = ecs::Manager<MasterSettings>;
}
