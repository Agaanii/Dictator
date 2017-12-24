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
	case GameLoopPhase::ACTION_RESPONSE:
	{
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
		}
		break;
	}
	case GameLoopPhase::PREPARATION:
	case GameLoopPhase::INPUT:
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