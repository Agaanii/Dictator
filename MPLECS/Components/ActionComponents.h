//-----------------------------------------------------------------------------
// All code is property of Dictator Developers Inc
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2018

// Components/InputComponents.h
// Components involved with interpreting user inputs

#pragma once

#include "../Core/typedef.h"

#include <deque>
#include <map>
#include <optional>
#include <variant>
#include <vector>

namespace Pathing
{
	struct Path
	{
		std::deque<CoordinateVector2> m_path;
		int m_totalPathCost;
	};

	struct MacroPathNode
	{
		MacroPathNode(const CoordinateVector2& node,
			 PathingDirection entry,
			 PathingDirection exit)
			: m_node(node)
			, m_entryDirection(entry)
			, m_exitDirection(exit)
		{

		}
		CoordinateVector2 m_node;
		PathingDirection m_entryDirection;
		PathingDirection m_exitDirection;
	};
	struct MacroPath
	{
		std::vector<MacroPathNode> m_path;
		int m_totalPathCost;
	};
}

namespace Action
{
	// Note: Camera manipulation is not an action for these purposes
	// Maybe it should be? Only local player would use it, but would let
	// players control which keys are used for pan/zoom.
	// Worth consideration

	namespace LocalPlayer
	{
		enum class PauseAction
		{
			PAUSE,
			UNPAUSE,
			TOGGLE_PAUSE
		};
		enum class GameSpeedAction
		{
			SPEED_UP,
			SLOW_DOWN
		};
		struct TimeManipulation
		{
			std::optional<PauseAction> m_pauseAction;
			std::optional<GameSpeedAction> m_gameSpeedAction;
		};

		struct CloseUIFrame
		{
			CloseUIFrame(const ecs::EntityIndex& uiFrameIndex) : m_frameIndex(uiFrameIndex) {}
			ecs::EntityIndex m_frameIndex;
		};

		struct SelectTile
		{
			SelectTile(const TilePosition& position) : m_position(position) { }
			TilePosition m_position;
		};

		struct PlanTargetedMotion
		{
			PlanTargetedMotion(const ecs::Impl::Handle& mover) : m_moverHandle(mover) {}
			ecs::Impl::Handle m_moverHandle;
		};

		struct PlanDirectionScout
		{
			PlanDirectionScout(const ecs::Impl::Handle& scoutSource) : m_scoutSource(scoutSource) {}
			ecs::Impl::Handle m_scoutSource;
		};

		struct PlanCaravan
		{
			PlanCaravan(const ecs::Impl::Handle& caravanSource) : m_sourceHandle(caravanSource) {}
			ecs::Impl::Handle m_sourceHandle;
		};

		struct CancelMovementPlan{};

		struct CenterCamera
		{
			CenterCamera(const CoordinateVector2& worldPosition) : m_worldPosition(worldPosition) {}
			CoordinateVector2 m_worldPosition;
		};
	}

	// AI governors choose position and place in the same turn
	struct PlaceBuilding
	{
		TilePosition m_position;
		int m_buildingClassId{ 0 };
	};

	struct CreateUnit
	{
		CreateUnit() = default;
		CreateUnit(const TilePosition& spawn,
			const std::optional<ecs::EntityIndex>& popSource,
			int movementSpeed)
			: m_spawningPosition(spawn)
			, m_popSource(popSource)
			, m_movementSpeed(movementSpeed)
		{}
		TilePosition m_spawningPosition;
		std::optional<ecs::EntityIndex> m_popSource;
		int m_movementSpeed;
	};

	struct CreateCaravan : CreateUnit
	{
		CreateCaravan(
			const TilePosition& delivery,
			const std::optional<ecs::Impl::Handle>& target,
			const TilePosition& spawn,
			const ecs::EntityIndex& popSource,
			int movementSpeed)
			: m_deliveryPosition(delivery)
			, m_targetingIcon(target)
			, CreateUnit(spawn, popSource, movementSpeed)
		{ }
		TilePosition m_deliveryPosition;
		std::optional<ecs::Impl::Handle> m_targetingIcon;
		std::optional<std::map<s32, s64>> m_inventory;
	};

	struct CreateExplorationUnit : CreateUnit
	{
		CreateExplorationUnit(const TilePosition& spawn,
			const std::optional<ecs::EntityIndex>& popSource,
			int movementSpeed,
			int daysToExplore,
			Direction exploreDirection)
			: CreateUnit(spawn, popSource, movementSpeed)
			, m_daysToExplore(daysToExplore)
			, m_exploreDirection(exploreDirection)
		{ }

		int m_daysToExplore;
		Direction m_exploreDirection;
	};

	struct CreateBuildingUnit : CreateUnit
	{
		int m_buildingTypeId{ 0 };
	};

	struct SettleBuildingUnit
	{
		SettleBuildingUnit(const ecs::EntityIndex& builderIndex) : m_builderIndex(builderIndex) {}
		ecs::EntityIndex m_builderIndex;
	};

	struct SetTargetedMovement
	{
		SetTargetedMovement(
			const ecs::Impl::Handle& mover,
			const std::optional<ecs::Impl::Handle>& targetingIcon,
			const TilePosition& position)
			: m_mover(mover)
			, m_targetingIcon(targetingIcon)
			, m_targetPosition(position)
		{}
		ecs::Impl::Handle m_mover;
		std::optional<ecs::Impl::Handle> m_targetingIcon;
		TilePosition m_targetPosition;
		// If set, int indicates error, Path indicates reachable
		std::optional<std::variant<Pathing::Path, int>> m_path;
	};

	// Would love to use a Named Union instead
	// This'll do for now
	// @Herb - Metaclasses when
	using Variant = std::variant<
		LocalPlayer::TimeManipulation,
		LocalPlayer::CloseUIFrame,
		LocalPlayer::SelectTile,
		LocalPlayer::PlanTargetedMotion,
		LocalPlayer::PlanCaravan,
		LocalPlayer::PlanDirectionScout,
		LocalPlayer::CenterCamera,
		LocalPlayer::CancelMovementPlan,
		PlaceBuilding, 
		CreateBuildingUnit,
		CreateCaravan,
		CreateExplorationUnit,
		SettleBuildingUnit,
		SetTargetedMovement
	>;

	struct Command
	{
		Command() {}
		Command(const Variant& v) : m_command(v) {}
		Command(const Variant& v, bool d) : m_command(v), m_delivered(d) {}
		Variant m_command;
		bool m_delivered{ false };
	};
	using Plan = std::vector<Command>;
}