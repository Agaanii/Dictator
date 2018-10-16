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

InputTranslation::InputTranslation()
	: SystemBase()
{
	m_functions[ECS_Core::Components::InputKeys::BACKSPACE] = [this](auto& _1, auto& _2) {TogglePause(_1, _2); };
	m_functions[ECS_Core::Components::InputKeys::EQUAL] = [this](auto& _1, auto& _2) {IncreaseGameSpeed(_1, _2); };
	m_functions[ECS_Core::Components::InputKeys::DASH] = [this](auto& _1, auto& _2) {DecreaseGameSpeed(_1, _2); };
}

void InputTranslation::ProgramInit() {}
void InputTranslation::SetupGameplay() {}

bool InputTranslation::CheckStartTargetedMovement(
	ECS_Core::Components::C_UserInputs& inputs,
	ECS_Core::Components::C_ActionPlan& actionPlan)
{
	using namespace ECS_Core;
	bool movementFound{ false };
	m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_MovementPlanIndicator>(
		[&manager = m_managerRef, &inputs, &actionPlan, &movementFound](
			const ecs::EntityIndex& targetEntity,
			const Components::C_MovementTarget& movement,
			const Components::C_TilePosition& position)
	{
		actionPlan.m_plan.push_back({ Action::SetTargetedMovement(
			movement.m_moverHandle,
			manager.getHandle(targetEntity),
			position.m_position) });
		inputs.ProcessMouseDown(ECS_Core::Components::MouseButtons::LEFT);
		movementFound = true;
		return ecs::IterationBehavior::BREAK;
	});
	m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_CaravanPlanIndicator>(
		[&manager = m_managerRef, &inputs, &actionPlan, &movementFound](
			const ecs::EntityIndex& targetEntity,
			const Components::C_CaravanPlan& movement,
			const Components::C_TilePosition& position)
	{
		if (!manager.hasComponent<Components::C_TilePosition>(movement.m_sourceBuildingHandle))
		{
			return ecs::IterationBehavior::CONTINUE;
		}
		auto& sourcePosition = manager.getComponent<Components::C_TilePosition>(movement.m_sourceBuildingHandle);
		actionPlan.m_plan.push_back({ Action::CreateCaravan(
			position.m_position,
			manager.getHandle(targetEntity),
			sourcePosition.m_position,
			manager.getEntityIndex(movement.m_sourceBuildingHandle),
			5) });
		inputs.ProcessMouseDown(ECS_Core::Components::MouseButtons::LEFT);
		movementFound = true;
		return ecs::IterationBehavior::BREAK;
	});
	return movementFound;
}

void InputTranslation::TranslateDownClicks(
	ECS_Core::Components::C_UserInputs& inputs,
	ECS_Core::Components::C_ActionPlan& actionPlan)
{
	using namespace ECS_Core;
	if (inputs.m_unprocessedThisFrameDownMouseButtonFlags & (u8)ECS_Core::Components::MouseButtons::LEFT)
	{	
		if (!CheckStartTargetedMovement(inputs, actionPlan))
		{
			actionPlan.m_plan.push_back({ Action::LocalPlayer::SelectTile(*inputs.m_currentMousePosition.m_tilePosition) });
		}
	}
	if (inputs.m_unprocessedThisFrameDownMouseButtonFlags & (u8)ECS_Core::Components::MouseButtons::RIGHT)
	{
		actionPlan.m_plan.push_back({ Action::LocalPlayer::CancelMovementPlan() });
	}
}

void InputTranslation::TogglePause(
	ECS_Core::Components::C_UserInputs& inputs,
	ECS_Core::Components::C_ActionPlan& actionPlan)
{
	Action::LocalPlayer::TimeManipulation action;
	action.m_pauseAction = Action::LocalPlayer::PauseAction::TOGGLE_PAUSE;
	actionPlan.m_plan.push_back({ action });
}

void InputTranslation::IncreaseGameSpeed(
	ECS_Core::Components::C_UserInputs& inputs,
	ECS_Core::Components::C_ActionPlan& actionPlan)
{
	Action::LocalPlayer::TimeManipulation action;
	action.m_gameSpeedAction = Action::LocalPlayer::GameSpeedAction::SPEED_UP;
	actionPlan.m_plan.push_back({ action });
}

void InputTranslation::DecreaseGameSpeed(
	ECS_Core::Components::C_UserInputs& inputs,
	ECS_Core::Components::C_ActionPlan& actionPlan)
{
	Action::LocalPlayer::TimeManipulation action;
	action.m_gameSpeedAction = Action::LocalPlayer::GameSpeedAction::SLOW_DOWN;
	actionPlan.m_plan.push_back({ action });
}

void InputTranslation::CancelMovementPlan(
	ECS_Core::Components::C_UserInputs& inputs,
	ECS_Core::Components::C_ActionPlan& actionPlan)
{
	actionPlan.m_plan.push_back({ Action::LocalPlayer::CancelMovementPlan() });
}

void InputTranslation::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	using namespace ECS_Core;
	switch (phase)
	{
	case GameLoopPhase::INPUT:
		m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_UserIO>(
			[&manager = m_managerRef, this](
				const ecs::EntityIndex& governorEntity,
				ECS_Core::Components::C_UserInputs& inputs,
				ECS_Core::Components::C_ActionPlan& actionPlan)
		{
			auto governorHandle = manager.getHandle(governorEntity);
			// UI actions will take place before we get here (see ordering in main.cpp)
			// So we only have to deal with the direct keybinds here
			if (inputs.m_unprocessedThisFrameDownMouseButtonFlags != 0)
			{
				TranslateDownClicks(inputs, actionPlan);
			}

			// Individual button actions - onRelease
			// TODO: Make options for onKeyDown vs OnKeyUp
			for (auto&& button : inputs.m_newKeyUp)
			{
				auto functionIter = m_functions.find(button);
				if (functionIter != m_functions.end())
				{
					functionIter->second(inputs, actionPlan);
					inputs.ProcessKey(button);
				}
			}

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