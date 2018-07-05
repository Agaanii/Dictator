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
		struct C_BuildingConstruction {
			ecs::Impl::Handle m_placingGovernor;
		};

		struct GrowthTile
		{
			f64 m_progress{ 0 };
			TilePosition m_tile;
		};
		enum PopulationClass
		{
			CHILDREN,
			WORKERS,
			ELDERS
		};
		using SpecialtyId = u32;
		using SpecialtyLevel = s32;
		using SpecialtyExperience = s32;

		struct SpecialtyProgress
		{
			SpecialtyLevel m_level{ 1 };
			SpecialtyExperience m_experience{ 0 };
		};

		using SpecialtyMap = std::map<SpecialtyId, SpecialtyProgress>;

		struct PopulationSegment
		{
			// Age in months
			s32 m_numWomen{ 1 };
			s32 m_numMen{ 1 };
			SpecialtyMap m_specialties;
			PopulationClass m_class{ PopulationClass::CHILDREN };
		};

		// Age (months)
		using PopulationKey = s32;
		
		struct C_Territory
		{
			std::set<TilePosition> m_ownedTiles;
			std::optional<GrowthTile> m_nextGrowthTile;
			std::map<PopulationKey, PopulationSegment> m_populations;
		};

		using YieldType = s32;
		struct Yield
		{
			f64 m_productionInterval{ 0.01 }; // 100 Days of 1 level 1 worker = 1 produced
			f64 m_productionProgress{ 0 };
			s32 m_value{ 1 };	
		};
		using YieldMap = std::map<YieldType, Yield>;
		struct C_YieldPotential
		{
			YieldMap m_availableYields;
		};

		struct C_ResourceInventory
		{
			std::map<YieldType, s64> m_collectedYields;
		};

		struct C_BuildingGhost
		{
			bool m_currentPlacementValid{ false };
			ecs::Impl::Handle m_placingGovernor;
			std::map<YieldType, s64> m_paidYield;
		};

		struct C_Realm
		{
			std::set<ecs::Impl::Handle> m_subordinates;
			std::set<ecs::Impl::Handle> m_territories;
		};

		struct UIFrame;

		struct DataString
		{
			CartesianVector2<f64> m_relativePosition;
			std::shared_ptr<sf::Text> m_text;
		};

		struct C_UIFrame
		{
			std::unique_ptr<UIFrame> m_frame;
			std::map<std::vector<int>, DataString> m_dataStrings;
			CartesianVector2<f64> m_topLeftCorner;
			CartesianVector2<f64> m_size;
			std::optional<CartesianVector2<f64>> m_currentDragPosition;
			bool m_focus{ false };
			bool m_global{ false };
			bool m_closable{ false };
		};

		struct C_TimeTracker
		{
			int m_year{ 0 };
			int m_month{ 0 };
			int m_day{ 0 };
			f64 m_dayProgress{ 0 };

			// Amount of game time between previous frame and current
			f64 m_frameDuration{ 0 };

			int m_gameSpeed{ 1 };
			bool m_paused{ true };

			bool IsNewMonth() const
			{
				return m_day == 1
					&& m_dayProgress < m_frameDuration;
			}
		};

		enum class PopulationAgenda
		{
			TRAINING,
			PRODUCTION
		};
		struct C_Agenda
		{
			PopulationAgenda m_popAgenda{ PopulationAgenda::TRAINING };
			std::vector<YieldType> m_yieldPriority;
		};
		struct C_WindowInfo
		{
			CartesianVector2<f64> m_windowSize;
		};
}

	namespace Tags
	{
		struct T_NoAcceleration {};

		struct T_Dead {};

		// Controlled/Owned by local player
		struct T_LocalPlayer {};

		// Marks a territory as capitol of the empire
		struct T_Capitol {};
	}

	namespace Signatures
	{
		using S_ApplyConstantMotion = ecs::Signature<Components::C_PositionCartesian, Components::C_VelocityCartesian, Tags::T_NoAcceleration>;
		using S_ApplyNewtonianMotion = ecs::Signature<Components::C_PositionCartesian, Components::C_VelocityCartesian, Components::C_AccelerationCartesian>;
		using S_Drawable = ecs::Signature<Components::C_PositionCartesian, Components::C_SFMLDrawable>;
		using S_UIDrawable = ecs::Signature<Components::C_UIFrame, Components::C_SFMLDrawable>;
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
		using S_Governor = ecs::Signature<Components::C_Realm, Components::C_ResourceInventory, Components::C_Agenda>;
		using S_PlayerGovernor = ecs::Signature<Components::C_Realm, Components::C_ResourceInventory, Tags::T_LocalPlayer>;
		using S_UIFrame = ecs::Signature<Components::C_UIFrame>;
		using S_TimeTracker = ecs::Signature<Components::C_TimeTracker>;
		using S_WindowInfo = ecs::Signature<Components::C_WindowInfo>;
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
		Components::C_BuildingConstruction,
		Components::C_UIFrame,
		Components::C_TimeTracker,
		Components::C_Agenda,
		Components::C_WindowInfo
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
		Signatures::S_PlayerGovernor,
		Signatures::S_UIFrame,
		Signatures::S_UIDrawable,
		Signatures::S_TimeTracker,
		Signatures::S_WindowInfo
	>;

	using MasterSettings = ecs::Settings<MasterComponentList, MasterTagList, MasterSignatureList>;
	using Manager = ecs::Manager<MasterSettings>;

	namespace Components
	{
		struct UIFrame
		{
			using FieldStrings = std::map<std::vector<int> /*key, separated in description by colons*/, std::string>;
			virtual FieldStrings ReadData(ecs::EntityIndex mI, ECS_Core::Manager& manager) const = 0;
		};
	}
}
