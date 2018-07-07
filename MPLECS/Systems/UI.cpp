//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Systems/UI.cpp
// Manages all UI elements, 

#include "../Core/typedef.h"

#include "Systems.h"

#include "../ECS/System.h"

void UI::ProgramInit() {}
void UI::SetupGameplay() {}

void UI::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	switch (phase)
	{
	case GameLoopPhase::ACTION:
	{
		m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_Input>([&manager = m_managerRef](
			const ecs::EntityIndex&,
			ECS_Core::Components::C_UserInputs& inputs) -> ecs::IterationBehavior
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
					// Check if we should close one
					manager.forEntitiesMatching<ECS_Core::Signatures::S_UIFrame>([&manager, &inputs](
						const ecs::EntityIndex& entityIndex,
						ECS_Core::Components::C_UIFrame& uiFrame) -> ecs::IterationBehavior
					{
						if (!uiFrame.m_closable)
						{
							return ecs::IterationBehavior::CONTINUE;
						}
						auto topRightCorner = uiFrame.m_topLeftCorner;
						topRightCorner.m_x += uiFrame.m_size.m_x;
						auto closeButtonOppositeCorner = topRightCorner - CartesianVector2<f64>(75, 75);
						if (IsInRectangle(
							inputs.m_currentMousePosition.m_screenPosition,
								topRightCorner,
								closeButtonOppositeCorner)
							&& IsInRectangle(
								inputs.m_heldMouseButtonInitialPositions[ECS_Core::Components::MouseButtons::LEFT].m_position.m_screenPosition,
								topRightCorner,
								closeButtonOppositeCorner))
						{
							manager.delComponent<ECS_Core::Components::C_UIFrame>(entityIndex);
							manager.delComponent<ECS_Core::Components::C_SFMLDrawable>(entityIndex);
							inputs.ProcessMouseUp(ECS_Core::Components::MouseButtons::LEFT);
							return ecs::IterationBehavior::BREAK;
						}
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

		m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_UIFrame>([&manager = m_managerRef, &inputEntities, &windowInfo](
			const ecs::EntityIndex& entityIndex,
			ECS_Core::Components::C_UIFrame& uiEntity)
		{
			for (auto&& str : uiEntity.m_frame->ReadData(entityIndex, manager))
			{
				auto displayIter = uiEntity.m_dataStrings.find(str.first);
				if (displayIter != uiEntity.m_dataStrings.end())
				{
					displayIter->second.m_text->setString(str.second);
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
	case GameLoopPhase::INPUT:
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