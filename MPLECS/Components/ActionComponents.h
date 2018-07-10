//-----------------------------------------------------------------------------
// All code is property of Dictator Developers Inc
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2018

// Components/InputComponents.h
// Components involved with interpreting user inputs

#pragma once

#include "../Core/typedef.h"

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
			ecs::EntityIndex m_frameIndex;
		};

		struct SelectTile
		{
			TilePosition m_position;
		};
	}

	// AI governors choose position and place in the same turn
	struct PlaceBuilding
	{
		TilePosition m_position;
		int m_buildingClassId{ 0 };
	};

	// Would love to use a Named Union instead
	// This'll do for now
	// @Herb - Metaclasses when
	using Variant = std::variant<
		LocalPlayer::CreateBuildingGhost,
		LocalPlayer::CreateBuildingFromGhost,
		PlaceBuilding,
		LocalPlayer::TimeManipulation,
		LocalPlayer::CloseUIFrame,
		LocalPlayer::SelectTile>;
	using Plan = std::vector<Variant>;
}