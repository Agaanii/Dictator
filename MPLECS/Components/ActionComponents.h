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
#include <optional>
#include <variant>
#include <vector>

namespace Action
{
	// Note: Camera manipulation is not an action for these purposes
	// Maybe it should be? Only local player would use it, but would let
	// players control which keys are used for pan/zoom.
	// Worth consideration

	namespace LocalPlayer
	{
		// only local player will use Ghost-related buildings
		// When they click, will take info from this to create a PlaceBuilding
		struct CreateBuildingGhost
		{
			TilePosition m_position;
			int m_buildingClassId{ 0 };
		};

		struct CreateBuildingFromGhost
		{};


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

		struct PlanMotion
		{
			PlanMotion(const ecs::EntityIndex& moverIndex) : m_moverIndex(moverIndex) {}
			ecs::EntityIndex m_moverIndex;
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
		TilePosition m_spawningPosition;
		ecs::EntityIndex m_popSource;
		int m_movementSpeed;
	};

	struct CreateCaravan : CreateUnit
	{
		TilePosition m_deliveryPosition;
		std::map<s32, s64> m_inventory;
	};

	struct CreateExplorationUnit : CreateUnit
	{
	};

	struct CreateBuildingUnit : CreateUnit
	{
		int m_buildingTypeId;
	};

	struct SettleBuildingUnit
	{
		SettleBuildingUnit(const ecs::EntityIndex& builderIndex) : m_builderIndex(builderIndex) {}
		ecs::EntityIndex m_builderIndex;
	};

	// Would love to use a Named Union instead
	// This'll do for now
	// @Herb - Metaclasses when
	using Variant = std::variant<
		LocalPlayer::CreateBuildingGhost,
		LocalPlayer::CreateBuildingFromGhost,
		LocalPlayer::TimeManipulation,
		LocalPlayer::CloseUIFrame,
		LocalPlayer::SelectTile,
		LocalPlayer::PlanMotion,
		PlaceBuilding, 
		CreateBuildingUnit,
		CreateCaravan,
		CreateExplorationUnit,
		SettleBuildingUnit>;
	using Plan = std::vector<Variant>;
}