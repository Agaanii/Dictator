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

template<typename T>
bool IsInRectangle(
	CartesianVector2<T> point,
	CartesianVector2<T> corner,
	CartesianVector2<T> oppositeCorner)
{
	T lowXBound = min(corner.m_x, oppositeCorner.m_x);
	T highXBound = max(corner.m_x, oppositeCorner.m_x);
	T lowYBound = min(corner.m_y, oppositeCorner.m_y);
	T highYBound = max(corner.m_y, oppositeCorner.m_y);

	return point.m_x <= highXBound
		&& point.m_x >= lowXBound
		&& point.m_y <= highYBound
		&& point.m_y >= lowYBound;
}

void UI::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	switch (phase)
	{
	case GameLoopPhase::ACTION:
	{
		for (auto&& inputEntity : m_managerRef.entitiesMatching<ECS_Core::Signatures::S_Input>())
		{
			auto& inputComponent = m_managerRef.getComponent<ECS_Core::Components::C_UserInputs>(inputEntity);
			if (inputComponent.m_unprocessedThisFrameDownMouseButtonFlags & static_cast<u8>(ECS_Core::Components::MouseButtons::LEFT))
			{
				// Click happened this frame. See whether it's on any UI frame
				for (auto&& uiEntity : m_managerRef.entitiesMatching<ECS_Core::Signatures::S_UIFrame>())
				{
					auto& uiFrame = m_managerRef.getComponent<ECS_Core::Components::C_UIFrame>(uiEntity);
					auto& bottomRightCorner = uiFrame.m_topLeftCorner + uiFrame.m_size;
					if (!IsInRectangle(inputComponent.m_currentMousePosition.m_screenPosition,
						uiFrame.m_topLeftCorner,
						bottomRightCorner))
					{
						continue;
					}

					uiFrame.m_currentDragPosition = inputComponent.m_currentMousePosition.m_screenPosition - uiFrame.m_topLeftCorner;
					inputComponent.ProcessMouseDown(ECS_Core::Components::MouseButtons::LEFT);
					break;
				}
			}

			if (inputComponent.m_unprocessedThisFrameUpMouseButtonFlags & static_cast<u8>(ECS_Core::Components::MouseButtons::LEFT))
			{
				bool mouseUpCaptured = false;
				for (auto&& uiEntity : m_managerRef.entitiesMatching<ECS_Core::Signatures::S_UIFrame>())
				{
					auto& uiFrame = m_managerRef.getComponent<ECS_Core::Components::C_UIFrame>(uiEntity);
					if (uiFrame.m_currentDragPosition)
					{
						uiFrame.m_currentDragPosition.reset();
						uiFrame.m_focus = true;
						inputComponent.ProcessMouseUp(ECS_Core::Components::MouseButtons::LEFT);
						mouseUpCaptured = true;
						break;
					}
				}
				if (!mouseUpCaptured)
				{
					// We weren't dragging a UI frame
					// Check if we should close one
					for (auto&& uiEntity : m_managerRef.entitiesMatching<ECS_Core::Signatures::S_UIFrame>())
					{
						auto& uiFrame = m_managerRef.getComponent<ECS_Core::Components::C_UIFrame>(uiEntity);
						if (!uiFrame.m_closable)
						{
							continue;
						}
						auto topRightCorner = uiFrame.m_topLeftCorner;
						topRightCorner.m_x += uiFrame.m_size.m_x;
						auto closeButtonOppositeCorner = topRightCorner - CartesianVector2<f64>(75, 75);
						if (IsInRectangle(
								inputComponent.m_currentMousePosition.m_screenPosition,
								topRightCorner,
								closeButtonOppositeCorner)
							&& IsInRectangle(
								inputComponent.m_heldMouseButtonInitialPositions[ECS_Core::Components::MouseButtons::LEFT].m_position.m_screenPosition,
								topRightCorner,
								closeButtonOppositeCorner))
						{
							m_managerRef.delComponent<ECS_Core::Components::C_UIFrame>(uiEntity);
							m_managerRef.delComponent<ECS_Core::Components::C_SFMLDrawable>(uiEntity);
							inputComponent.ProcessMouseUp(ECS_Core::Components::MouseButtons::LEFT);
							break;
						}
					}
				}
			}
		}
	}
	break;
	case GameLoopPhase::ACTION_RESPONSE:
	{
		auto inputEntities = m_managerRef.entitiesMatching<ECS_Core::Signatures::S_Input>();
		// Will crash if the windowInfo entity hasn't been created
		auto windowInfoIndex = m_managerRef.entitiesMatching<ECS_Core::Signatures::S_WindowInfo>().front();
		auto& windowInfo = m_managerRef.getComponent<ECS_Core::Components::C_WindowInfo>(windowInfoIndex);

		for (auto&& entityIndex : m_managerRef.entitiesMatching<ECS_Core::Signatures::S_UIFrame>())
		{
			auto& uiEntity = m_managerRef.getComponent<ECS_Core::Components::C_UIFrame>(entityIndex);
			for (auto&& str : uiEntity.m_frame->ReadData(entityIndex, m_managerRef))
			{
				auto displayIter = uiEntity.m_dataStrings.find(str.first);
				if (displayIter != uiEntity.m_dataStrings.end())
				{
					displayIter->second.m_text->setString(str.second);
				}
			}
			if (uiEntity.m_currentDragPosition && inputEntities.size())
			{
				auto& input = m_managerRef.getComponent<ECS_Core::Components::C_UserInputs>(inputEntities.front());
				uiEntity.m_topLeftCorner = input.m_currentMousePosition.m_screenPosition - *uiEntity.m_currentDragPosition;
				uiEntity.m_topLeftCorner.m_x = max<f64>(uiEntity.m_topLeftCorner.m_x, 0);
				uiEntity.m_topLeftCorner.m_y = max<f64>(uiEntity.m_topLeftCorner.m_y, 0);

				auto maxTopLeft = windowInfo.m_windowSize - uiEntity.m_size;
				uiEntity.m_topLeftCorner.m_x = min<f64>(uiEntity.m_topLeftCorner.m_x, maxTopLeft.m_x);
				uiEntity.m_topLeftCorner.m_y = min<f64>(uiEntity.m_topLeftCorner.m_y, maxTopLeft.m_y);
			}
		}
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