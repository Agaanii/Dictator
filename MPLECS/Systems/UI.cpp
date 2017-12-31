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

#include <iostream>

void UI::SetupGameplay() {

}

void UI::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	switch (phase)
	{
	case GameLoopPhase::ACTION:
	{
		for (auto&& inputEntity : m_managerRef.entitiesMatching<ECS_Core::Signatures::S_Input>())
		{
			auto& inputComponent = m_managerRef.getComponent<ECS_Core::Components::C_UserInputs>(inputEntity);^
			if (inputComponent.m_unprocessedThisFrameDownMouseButtonFlags & static_cast<u8>(ECS_Core::Components::MouseButtons::LEFT))
			{
				// Click happened this frame. See whether it's on any UI frame
				for (auto&& uiEntity : m_managerRef.entitiesMatching<ECS_Core::Signatures::S_UIFrame>())
				{
					auto& uiFrame = m_managerRef.getComponent<ECS_Core::Components::C_UIFrame>(uiEntity);
					auto& bottomRightCorner = uiFrame.m_topLeftCorner + uiFrame.m_size;
					if (inputComponent.m_currentMousePosition.m_screenPosition.m_x < uiFrame.m_topLeftCorner.m_x
						|| inputComponent.m_currentMousePosition.m_screenPosition.m_y < uiFrame.m_topLeftCorner.m_y
						|| inputComponent.m_currentMousePosition.m_screenPosition.m_x > bottomRightCorner.m_x
						|| inputComponent.m_currentMousePosition.m_screenPosition.m_y > bottomRightCorner.m_y)
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
				for (auto&& uiEntity : m_managerRef.entitiesMatching<ECS_Core::Signatures::S_UIFrame>())
				{
					auto& uiFrame = m_managerRef.getComponent<ECS_Core::Components::C_UIFrame>(uiEntity);
					if (uiFrame.m_currentDragPosition)
					{
						uiFrame.m_currentDragPosition.reset();
						inputComponent.ProcessMouseUp(ECS_Core::Components::MouseButtons::LEFT);
					}
				}
			}
		}
	}
		break;
	case GameLoopPhase::ACTION_RESPONSE:
	{
		auto inputEntities = m_managerRef.entitiesMatching<ECS_Core::Signatures::S_Input>();
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