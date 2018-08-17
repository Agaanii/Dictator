//-----------------------------------------------------------------------------
// All code is property of Dictator Developers Inc
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2018

// Systems/InputTranslation.cpp
// Centralized location for turning player inputs into
// planned actions

#include "../Core/typedef.h"

#include "InputTranslation.h"

#include <functional>

void InputTranslation::ProgramInit() {}
void InputTranslation::SetupGameplay() {}

bool CheckPlaceBuildingCommand(
	ECS_Core::Components::C_UserInputs& inputs,
	ECS_Core::Components::C_ActionPlan& actionPlan,
	ECS_Core::Manager& manager)
{
	using namespace ECS_Core;
	bool ghostFound{ false };
	manager.forEntitiesMatching<ECS_Core::Signatures::S_PlannedBuildingPlacement>([&manager, &inputs, &actionPlan, &ghostFound](
		const ecs::EntityIndex& ghostEntity,
		const Components::C_BuildingDescription&,
		const Components::C_TilePosition&,
		const Components::C_BuildingGhost& ghost)
	{
		if (!ghost.m_currentPlacementValid)
		{
			// TODO: Surface error
			return ecs::IterationBehavior::CONTINUE;
		}
		actionPlan.m_plan.push_back(Action::LocalPlayer::CreateBuildingFromGhost());
		inputs.ProcessMouseDown(ECS_Core::Components::MouseButtons::LEFT);
		ghostFound = true;
		return ecs::IterationBehavior::BREAK;
	});
	return ghostFound;
}

bool CheckStartTargetedMovement(
	ECS_Core::Components::C_UserInputs& inputs,
	ECS_Core::Components::C_ActionPlan& actionPlan,
	ECS_Core::Manager& manager)
{
	using namespace ECS_Core;
	bool movementFound{ false };
	manager.forEntitiesMatching<ECS_Core::Signatures::S_MovementPlanIndicator>([&manager, &inputs, &actionPlan, &movementFound](
		const ecs::EntityIndex& targetEntity,
		const Components::C_MovementTarget& movement,
		const Components::C_TilePosition& position)
	{
		actionPlan.m_plan.push_back(Action::SetTargetedMovement(
			movement.m_moverHandle,
			manager.getHandle(targetEntity),
			position.m_position));
		inputs.ProcessMouseDown(ECS_Core::Components::MouseButtons::LEFT);
		movementFound = true;
		return ecs::IterationBehavior::BREAK;
	});
	manager.forEntitiesMatching<ECS_Core::Signatures::S_CaravanPlanIndicator>([&manager, &inputs, &actionPlan, &movementFound](
		const ecs::EntityIndex& targetEntity,
		const Components::C_CaravanPlan& movement,
		const Components::C_TilePosition& position)
	{
		if (!manager.hasComponent<Components::C_TilePosition>(movement.m_sourceBuildingHandle))
		{
			return ecs::IterationBehavior::CONTINUE;
		}
		auto& sourcePosition = manager.getComponent<Components::C_TilePosition>(movement.m_sourceBuildingHandle);
		actionPlan.m_plan.push_back(Action::CreateCaravan(
			position.m_position,
			manager.getHandle(targetEntity),
			sourcePosition.m_position,
			manager.getEntityIndex(movement.m_sourceBuildingHandle),
			5));
		inputs.ProcessMouseDown(ECS_Core::Components::MouseButtons::LEFT);
		movementFound = true;
		return ecs::IterationBehavior::BREAK;
	});
	return movementFound;
}

void TranslateDownClicks(
	ECS_Core::Components::C_UserInputs& inputs,
	ECS_Core::Components::C_ActionPlan& actionPlan,
	ECS_Core::Manager& manager)
{
	using namespace ECS_Core;
	if (inputs.m_unprocessedThisFrameDownMouseButtonFlags & (u8)ECS_Core::Components::MouseButtons::LEFT)
	{	
		if (!CheckPlaceBuildingCommand(inputs, actionPlan, manager)
			&& !CheckStartTargetedMovement(inputs, actionPlan, manager))
		{
			actionPlan.m_plan.push_back(Action::LocalPlayer::SelectTile(*inputs.m_currentMousePosition.m_tilePosition));
		}
	}
}

void CreateBuildingGhost(
	ECS_Core::Components::C_UserInputs& inputs,
	ECS_Core::Components::C_ActionPlan& actionPlan,
	ECS_Core::Manager& manager)
{
	Action::LocalPlayer::CreateBuildingGhost ghost;
	ghost.m_buildingClassId = 0;
	ghost.m_position = *inputs.m_currentMousePosition.m_tilePosition;
	actionPlan.m_plan.push_back(ghost);
}

void TogglePause(
	ECS_Core::Components::C_UserInputs& inputs,
	ECS_Core::Components::C_ActionPlan& actionPlan,
	ECS_Core::Manager& manager)
{
	Action::LocalPlayer::TimeManipulation action;
	action.m_pauseAction = Action::LocalPlayer::PauseAction::TOGGLE_PAUSE;
	actionPlan.m_plan.push_back(action);
}

void IncreaseGameSpeed(
	ECS_Core::Components::C_UserInputs& inputs,
	ECS_Core::Components::C_ActionPlan& actionPlan,
	ECS_Core::Manager& manager)
{
	Action::LocalPlayer::TimeManipulation action;
	action.m_gameSpeedAction = Action::LocalPlayer::GameSpeedAction::SPEED_UP;
	actionPlan.m_plan.push_back(action);
}

void DecreaseGameSpeed(
	ECS_Core::Components::C_UserInputs& inputs,
	ECS_Core::Components::C_ActionPlan& actionPlan,
	ECS_Core::Manager& manager)
{
	Action::LocalPlayer::TimeManipulation action;
	action.m_gameSpeedAction = Action::LocalPlayer::GameSpeedAction::SLOW_DOWN;
	actionPlan.m_plan.push_back(action);
}

std::map < ECS_Core::Components::InputKeys, std::function<void(
	ECS_Core::Components::C_UserInputs&,
	ECS_Core::Components::C_ActionPlan&,
	ECS_Core::Manager&)>> functions
	=
{
	// {ECS_Core::Components::InputKeys::B, CreateBuildingGhost},
	{ECS_Core::Components::InputKeys::BACKSPACE, TogglePause},
	{ECS_Core::Components::InputKeys::EQUAL, IncreaseGameSpeed },
	{ECS_Core::Components::InputKeys::DASH, DecreaseGameSpeed} 
};

void InputTranslation::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	using namespace ECS_Core;
	switch (phase)
	{
	case GameLoopPhase::INPUT:
		m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_UserIO>([&manager = m_managerRef](
			const ecs::EntityIndex& governorEntity,
			ECS_Core::Components::C_UserInputs& inputs,
			ECS_Core::Components::C_ActionPlan& actionPlan)
		{
			auto governorHandle = manager.getHandle(governorEntity);
			// UI actions will take place before we get here (see ordering in main.cpp)
			// So we only have to deal with the direct keybinds here
			if (inputs.m_unprocessedThisFrameDownMouseButtonFlags != 0)
			{
				TranslateDownClicks(inputs, actionPlan, manager);
			}

			// Individual button actions - onRelease
			// TODO: Make options for onKeyDown vs OnKeyUp
			for (auto&& button : inputs.m_newKeyUp)
			{
				auto functionIter = functions.find(button);
				if (functionIter != functions.end())
				{
					functionIter->second(inputs, actionPlan, manager);
					inputs.ProcessKey(button);
				}
			}

			// If this governor owns a building ghost, update its position
			manager.forEntitiesMatching<Signatures::S_PlannedBuildingPlacement>(
				[&inputs, &governorHandle](
					const ecs::EntityIndex& entity,
					const Components::C_BuildingDescription&,
					Components::C_TilePosition& position,
					const Components::C_BuildingGhost& ghost)
			{
				if (ghost.m_placingGovernor == governorHandle)
				{
					position.m_position = *inputs.m_currentMousePosition.m_tilePosition;
				}
				return ecs::IterationBehavior::CONTINUE;
			});

			// Same for any planned motions
			manager.forEntitiesMatching<Signatures::S_MovementPlanIndicator>(
				[&inputs, &governorHandle](
					const ecs::EntityIndex& entity,
					const Components::C_MovementTarget& mover,
					Components::C_TilePosition& position)
			{
				if (mover.m_governorHandle == governorHandle)
				{
					position.m_position = *inputs.m_currentMousePosition.m_tilePosition;
				}
				return ecs::IterationBehavior::CONTINUE;
			});
			manager.forEntitiesMatching<Signatures::S_CaravanPlanIndicator>(
				[&inputs, &governorHandle](
					const ecs::EntityIndex& entity,
					const Components::C_CaravanPlan& mover,
					Components::C_TilePosition& position)
			{
				if (mover.m_governorHandle == governorHandle)
				{
					position.m_position = *inputs.m_currentMousePosition.m_tilePosition;
				}
				return ecs::IterationBehavior::CONTINUE;
			});
			return ecs::IterationBehavior::CONTINUE;
		});
		break;

	case GameLoopPhase::CLEANUP:
		m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_UserIO>([](
			const ecs::EntityIndex&,
			ECS_Core::Components::C_UserInputs&,
			ECS_Core::Components::C_ActionPlan& actionPlan)
		{
			actionPlan.m_plan.clear();
			return ecs::IterationBehavior::CONTINUE;
		});
		break;
	case GameLoopPhase::PREPARATION:
	case GameLoopPhase::ACTION:
	case GameLoopPhase::ACTION_RESPONSE:
	case GameLoopPhase::RENDER:
		return;
	}
}

bool InputTranslation::ShouldExit()
{
	return false;
}

DEFINE_SYSTEM_INSTANTIATION(InputTranslation);