//-----------------------------------------------------------------------------
// All code is property of Dictator Developers Inc
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2018

// Systems/UI.cpp
// Manages all UI elements, 

#include "../Core/typedef.h"

#include "UI.h"

void UI::ProgramInit() {}
void UI::SetupGameplay() {}

void UI::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	switch (phase)
	{
	case GameLoopPhase::INPUT:
	{
		m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_UserIO>([&manager = m_managerRef](
			const ecs::EntityIndex& userEntity,
			ECS_Core::Components::C_UserInputs& inputs,
			ECS_Core::Components::C_ActionPlan& actions) -> ecs::IterationBehavior
		{
			if (inputs.m_unprocessedThisFrameDownMouseButtonFlags & static_cast<u8>(ECS_Core::Components::MouseButtons::LEFT))
			{
				// Click happened this frame. See whether it's on any UI frame
				manager.forEntitiesMatching<ECS_Core::Signatures::S_UIFrame>([&manager, &inputs](
					const ecs::EntityIndex&,
					ECS_Core::Components::C_UIFrame& uiFrame)
				{
					auto& bottomRightCorner = uiFrame.m_topLeftCorner + uiFrame.m_size;
					if (!IsInRectangle(inputs.m_currentMousePosition.m_screenPosition,
						uiFrame.m_topLeftCorner,
						bottomRightCorner))
					{
						return ecs::IterationBehavior::CONTINUE;
					}

					// Don't drag if click is on a button
					for (auto&& button : uiFrame.m_buttons)
					{
						auto absoluteButtonPosition = button.m_topLeftCorner + uiFrame.m_topLeftCorner;
						auto buttonBottomRight = absoluteButtonPosition + button.m_size;
						if (IsInRectangle(inputs.m_currentMousePosition.m_screenPosition,
							absoluteButtonPosition,
							buttonBottomRight)
							&& IsInRectangle(inputs.m_heldMouseButtonInitialPositions[ECS_Core::Components::MouseButtons::LEFT].m_position.m_screenPosition,
								absoluteButtonPosition,
								buttonBottomRight))
						{
							return ecs::IterationBehavior::CONTINUE;
						}
					}

					uiFrame.m_currentDragPosition = inputs.m_currentMousePosition.m_screenPosition - uiFrame.m_topLeftCorner;
					inputs.ProcessMouseDown(ECS_Core::Components::MouseButtons::LEFT);
					return ecs::IterationBehavior::BREAK;
				});
			}

			if (inputs.m_unprocessedThisFrameUpMouseButtonFlags & static_cast<u8>(ECS_Core::Components::MouseButtons::LEFT))
			{
				bool mouseUpCaptured = false;
				manager.forEntitiesMatching<ECS_Core::Signatures::S_UIFrame>([&manager, &inputs, &mouseUpCaptured](
					const ecs::EntityIndex& entityIndex,
					ECS_Core::Components::C_UIFrame& uiFrame) -> ecs::IterationBehavior
				{
					if (uiFrame.m_currentDragPosition)
					{
						uiFrame.m_currentDragPosition.reset();
						uiFrame.m_focus = true;
						inputs.ProcessMouseUp(ECS_Core::Components::MouseButtons::LEFT);
						mouseUpCaptured = true;
						return ecs::IterationBehavior::BREAK;
					}
					return ecs::IterationBehavior::CONTINUE;
				});
				if (!mouseUpCaptured)
				{
					// We weren't dragging a UI frame
					// Check if we should activate a button
					// This is also where button-checking should go
					manager.forEntitiesMatching<ECS_Core::Signatures::S_UIFrame>([&manager, &inputs, &userEntity, &actions](
						const ecs::EntityIndex& entityIndex,
						ECS_Core::Components::C_UIFrame& uiFrame) -> ecs::IterationBehavior
					{
						for (auto&& button : uiFrame.m_buttons)
						{
							auto absoluteButtonPosition = button.m_topLeftCorner + uiFrame.m_topLeftCorner;
							auto buttonBottomRight = absoluteButtonPosition + button.m_size;
							if (IsInRectangle(inputs.m_currentMousePosition.m_screenPosition,
								absoluteButtonPosition,
								buttonBottomRight)
								&& IsInRectangle(inputs.m_heldMouseButtonInitialPositions[ECS_Core::Components::MouseButtons::LEFT].m_position.m_screenPosition,
									absoluteButtonPosition,
									buttonBottomRight))
							{
								actions.m_plan.push_back({ button.m_onClick(userEntity, entityIndex) });
								inputs.ProcessMouseUp(ECS_Core::Components::MouseButtons::LEFT);
								return ecs::IterationBehavior::BREAK;
							}
						}
						return ecs::IterationBehavior::CONTINUE;
					});
				}
			}
			return ecs::IterationBehavior::CONTINUE;
		});
	}
	break;
	case GameLoopPhase::ACTION_RESPONSE:
	{
		auto inputEntities = m_managerRef.entitiesMatching<ECS_Core::Signatures::S_Input>();
		// Will crash if the windowInfo entity hasn't been created
		auto windowInfoIndex = m_managerRef.entitiesMatching<ECS_Core::Signatures::S_WindowInfo>().front();
		auto& windowInfo = m_managerRef.getComponent<ECS_Core::Components::C_WindowInfo>(windowInfoIndex);

		m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_Planner>([&manager = m_managerRef](
			const ecs::EntityIndex&,
			ECS_Core::Components::C_ActionPlan& plan)
		{
			for (auto&& action : plan.m_plan)
			{
				if (std::holds_alternative<Action::LocalPlayer::CloseUIFrame>(action.m_command))
				{
					auto& close = std::get<Action::LocalPlayer::CloseUIFrame>(action.m_command);
					manager.delComponent<ECS_Core::Components::C_UIFrame>(close.m_frameIndex);
					if (manager.hasComponent<ECS_Core::Components::C_SFMLDrawable>(close.m_frameIndex))
					{
						auto& drawableComponent = manager.getComponent<ECS_Core::Components::C_SFMLDrawable>(close.m_frameIndex);
						drawableComponent.m_drawables.erase(ECS_Core::Components::DrawLayer::MENU);
						if (drawableComponent.m_drawables.size() == 0)
						{
							manager.delComponent<ECS_Core::Components::C_SFMLDrawable>(close.m_frameIndex);
						}
					}
				}
			}
			return ecs::IterationBehavior::CONTINUE;
		});

		m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_UIFrame>([&manager = m_managerRef, &inputEntities, &windowInfo](
			const ecs::EntityIndex& entityIndex,
			ECS_Core::Components::C_UIFrame& uiEntity)
		{
			for (auto&& [key, str] : uiEntity.m_dataStrings)
			{
				str.m_text->setString("");
			}
			if (uiEntity.m_frame != nullptr)
			{
				for (auto&&[key, str] : uiEntity.m_frame->ReadData(entityIndex, manager))
				{
					auto displayIter = uiEntity.m_dataStrings.find(key);
					if (displayIter != uiEntity.m_dataStrings.end())
					{
						displayIter->second.m_text->setString(str);
					}
				}
			}
			if (uiEntity.m_currentDragPosition && inputEntities.size())
			{
				auto& input = manager.getComponent<ECS_Core::Components::C_UserInputs>(inputEntities.front());
				uiEntity.m_topLeftCorner = input.m_currentMousePosition.m_screenPosition - *uiEntity.m_currentDragPosition;
				uiEntity.m_topLeftCorner.m_x = max<f64>(uiEntity.m_topLeftCorner.m_x, 0);
				uiEntity.m_topLeftCorner.m_y = max<f64>(uiEntity.m_topLeftCorner.m_y, 0);

				auto maxTopLeft = windowInfo.m_windowSize - uiEntity.m_size;
				uiEntity.m_topLeftCorner.m_x = min<f64>(uiEntity.m_topLeftCorner.m_x, maxTopLeft.m_x);
				uiEntity.m_topLeftCorner.m_y = min<f64>(uiEntity.m_topLeftCorner.m_y, maxTopLeft.m_y);
			}
			return ecs::IterationBehavior::CONTINUE;
		});
		break;
	}
	case GameLoopPhase::PREPARATION:
	case GameLoopPhase::ACTION:
	case GameLoopPhase::RENDER:
	case GameLoopPhase::CLEANUP:
		return;
	}
}

bool UI::ShouldExit()
{
	return false;
}

DEFINE_SYSTEM_INSTANTIATION(UI);