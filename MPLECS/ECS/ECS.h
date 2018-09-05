#pragma once

#include "../ecs/ecs.hpp"

#include "../Core/typedef.h"

#include "../Components/ActionComponents.h"
#include "../Components/GraphicsComponents.h"
#include "../Components/InputComponents.h"

#include <SFML/Graphics.hpp>
#include <functional>
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
		};
		struct C_BuildingConstruction {
			ecs::Impl::Handle m_placingGovernor;
			f64 m_buildingProgress{ 0 };
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
		using SpecialtyId = s32;
		using SpecialtyLevel = s32;
		using SpecialtyExperience = f64;

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
			f64 m_womensHealth{ 1. };
			s32 m_numMen{ 1 };
			f64 m_mensHealth{ 1. };
			SpecialtyMap m_specialties;
			PopulationClass m_class{ PopulationClass::CHILDREN };
		};

		// Age (months)
		using PopulationKey = s32;
		struct C_Population
		{
			std::map<PopulationKey, PopulationSegment> m_populations;
		};

		struct C_Territory
		{
			std::set<TilePosition> m_ownedTiles;
			std::optional<GrowthTile> m_nextGrowthTile;
		};

		using YieldType = s32;
		namespace Yields
		{
			enum Enum
			{
				FOOD = 1,
				WOOD = 2,
				STONE = 3,
			};
		}
		using YieldBuckets = std::map<YieldType, f64>;
		struct TileProduction
		{
			f64 m_productionInterval{ 120 }; // # of Days of 1 level 1 worker = 1 produced
			f64 m_productionProgress{ 0 };
			s32 m_workableTiles{ 0 };
			YieldBuckets m_productionYield;
		};
		using TileType = s32;
		using TileProductionMap = std::map<TileType, TileProduction>;
		struct C_TileProductionPotential
		{
			TileProductionMap m_availableYields;
		};

		struct C_ResourceInventory
		{
			YieldBuckets m_collectedYields;
		};

		struct C_BuildingGhost
		{
			bool m_currentPlacementValid{ false };
			ecs::Impl::Handle m_placingGovernor;
			YieldBuckets m_paidYield;
		};

		struct C_Realm
		{
			std::set<ecs::Impl::Handle> m_subordinates;
			std::set<ecs::Impl::Handle> m_territories;
			std::optional<ecs::Impl::Handle> m_capitol;
		};

		struct UIFrame;

		struct DataString
		{
			CartesianVector2<f64> m_relativePosition;
			std::shared_ptr<sf::Text> m_text;
		};
		struct Button
		{
			std::function<Action::Variant(const ecs::EntityIndex& /*clicker*/, const ecs::EntityIndex& /*clickedEntity*/)> m_onClick;
			// Position relative to containing frame
			CartesianVector2<f64> m_topLeftCorner;
			CartesianVector2<f64> m_size;
		};
		struct C_UIFrame
		{
			std::unique_ptr<UIFrame> m_frame;
			std::map<std::vector<int>, DataString> m_dataStrings;
			std::vector<Button> m_buttons;
			CartesianVector2<f64> m_topLeftCorner;
			CartesianVector2<f64> m_size;
			std::optional<CartesianVector2<f64>> m_currentDragPosition;
			bool m_focus{ false };
			bool m_global{ false };
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
			bool IsNewDay() const
			{
				return m_dayProgress < m_frameDuration;
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

		struct C_ActionPlan
		{
			Action::Plan m_plan;
		};

		struct ExplorationPlan
		{
			Direction m_direction;
			int m_leavingYear;
			int m_leavingMonth;
			int m_leavingDay;
			// Once time matches days to explore + leaving time, turn around and return home
			s64 m_daysToExplore;
		};
		struct MovementTilePosition
		{
			MovementTilePosition(TilePosition tile, int movementCost)
				: m_tile(tile)
				, m_movementCost(movementCost)
			{ }
			TilePosition m_tile;
			int m_movementCost{ 1 };
		};
		struct MoveToPoint
		{
			TilePosition m_targetPosition;
			std::vector<MovementTilePosition> m_path;
			int m_currentPathIndex{ 0 };
			f64 m_currentMovementProgress{ 0 };
			s64 m_totalPathCost{ 0 };
		};

		struct C_MovingUnit
		{
			// # of tiles of movement cost 1 the unit can move in a day
			s32 m_movementPerDay{ 1 };
			std::optional<MoveToPoint> m_currentMovement;
			std::optional<ExplorationPlan> m_explorationPlan;
		};

		struct C_Selection
		{
			ecs::Impl::Handle m_selector;
		};

		struct C_MovementTarget
		{
			ecs::Impl::Handle m_moverHandle;
			ecs::Impl::Handle m_governorHandle;
		};

		struct C_CaravanPlan
		{
			ecs::Impl::Handle m_sourceBuildingHandle;
			ecs::Impl::Handle m_governorHandle;
		};

		struct C_CaravanPath
		{
			ecs::Impl::Handle m_originBuildingHandle;
			ecs::Impl::Handle m_targetBuildingHandle;
			MoveToPoint m_basePath;
			bool m_isReturning{ false };
		};

		struct C_ScoutingPlan
		{
			ecs::Impl::Handle m_sourceBuildingHandle;
			ecs::Impl::Handle m_governorHandle;
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
		using S_DrawableConstructingBuilding = ecs::Signature<Components::C_BuildingDescription, Components::C_TilePosition, Components::C_SFMLDrawable, Components::C_BuildingConstruction>;
		using S_InProgressBuilding = ecs::Signature<Components::C_BuildingDescription, Components::C_TilePosition, Components::C_BuildingConstruction>;
		using S_CompleteBuilding = ecs::Signature<Components::C_BuildingDescription, Components::C_TilePosition, Components::C_Territory, Components::C_TileProductionPotential, Components::C_ResourceInventory>;
		using S_Population = ecs::Signature<Components::C_Population, Components::C_ResourceInventory>;
		using S_DestroyedBuilding = ecs::Signature<Components::C_BuildingDescription, Components::C_TilePosition, Tags::T_Dead>;
		using S_Governor = ecs::Signature<Components::C_Realm, Components::C_Agenda>;
		using S_PlayerGovernor = ecs::Signature<Components::C_Realm, Tags::T_LocalPlayer>;
		using S_UIFrame = ecs::Signature<Components::C_UIFrame>;
		using S_TimeTracker = ecs::Signature<Components::C_TimeTracker>;
		using S_WindowInfo = ecs::Signature<Components::C_WindowInfo>;
		using S_UserIO = ecs::Signature<Components::C_UserInputs, Components::C_ActionPlan>;
		using S_Planner = ecs::Signature<Components::C_ActionPlan>;
		using S_WealthPlanner = ecs::Signature<Components::C_ActionPlan, Components::C_Realm>;
		using S_MovingUnit = ecs::Signature<Components::C_TilePosition, Components::C_MovingUnit, Components::C_Population>;
		using S_SelectedMovingUnit = ecs::Signature<Components::C_TilePosition, Components::C_MovingUnit, Components::C_Population, Components::C_Selection>;
		using S_BuilderUnit = ecs::Signature<Components::C_TilePosition, Components::C_MovingUnit, Components::C_BuildingDescription, Components::C_Population>;
		using S_CaravanUnit = ecs::Signature<Components::C_TilePosition, Components::C_MovingUnit, Components::C_ResourceInventory, Components::C_Population, Components::C_CaravanPath>;
		using S_MovementPlanIndicator = ecs::Signature<Components::C_MovementTarget, Components::C_TilePosition>;
		using S_CaravanPlanIndicator = ecs::Signature<Components::C_CaravanPlan, Components::C_TilePosition>;
		using S_ScoutPlanner = ecs::Signature<Components::C_ScoutingPlan>;
		using S_Dead = ecs::Signature<Tags::T_Dead>;
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
		Components::C_Population,
		Components::C_TileProductionPotential,
		Components::C_ResourceInventory,
		Components::C_Realm,
		Components::C_BuildingConstruction,
		Components::C_UIFrame,
		Components::C_TimeTracker,
		Components::C_Agenda,
		Components::C_WindowInfo,
		Components::C_ActionPlan,
		Components::C_MovingUnit,
		Components::C_Selection,
		Components::C_MovementTarget,
		Components::C_CaravanPlan,
		Components::C_CaravanPath,
		Components::C_ScoutingPlan
	>;

	using MasterTagList = ecs::TagList<
		Tags::T_NoAcceleration,
		Tags::T_Dead,
		Tags::T_LocalPlayer
	>;

	using MasterSignatureList = ecs::SignatureList <
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
		Signatures::S_Population,
		Signatures::S_PlannedBuildingPlacement,
		Signatures::S_DestroyedBuilding,
		Signatures::S_Governor,
		Signatures::S_PlayerGovernor,
		Signatures::S_UIFrame,
		Signatures::S_UIDrawable,
		Signatures::S_TimeTracker,
		Signatures::S_WindowInfo,
		Signatures::S_UserIO,
		Signatures::S_Planner,
		Signatures::S_WealthPlanner,
		Signatures::S_BuilderUnit,
		Signatures::S_CaravanUnit,
		Signatures::S_MovingUnit,
		Signatures::S_SelectedMovingUnit,
		Signatures::S_MovementPlanIndicator,
		Signatures::S_CaravanPlanIndicator,
		Signatures::S_ScoutPlanner,
		Signatures::S_Dead
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
