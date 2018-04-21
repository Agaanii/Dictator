//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Systems/Time.cpp
// The boilerplate system code, to ease new system creation

#include "../Core/typedef.h"

#include "Systems.h"

#include "../ECS/System.h"
#include "../ECS/ECS.h"

#include "../Components/UIComponents.h"

void Time::ProgramInit() {}

extern sf::Font s_font;
void Time::SetupGameplay()
{
	auto index = m_managerRef.createIndex();
	m_managerRef.addComponent<ECS_Core::Components::C_TimeTracker>(index);

	auto& uiFrameComponent = m_managerRef.addComponent<ECS_Core::Components::C_UIFrame>(index);
	uiFrameComponent.m_frame
		= DefineUIFrame(
			"Calendar",
			DataBinding(ECS_Core::Components::C_TimeTracker, m_year),
			DataBinding(ECS_Core::Components::C_TimeTracker, m_month),
			DataBinding(ECS_Core::Components::C_TimeTracker, m_day));
	uiFrameComponent.m_dataStrings[{0}] = { { 0,0 }, std::make_shared<sf::Text>() };
	uiFrameComponent.m_dataStrings[{1}] = { { 0,35 }, std::make_shared<sf::Text>() };
	uiFrameComponent.m_dataStrings[{2}] = { { 50,35 }, std::make_shared<sf::Text>() };
	uiFrameComponent.m_topLeftCorner = { 1400, 100 };
	uiFrameComponent.m_size = { 100, 70 };
	uiFrameComponent.m_global = true;
	auto& drawable = m_managerRef.addComponent<ECS_Core::Components::C_SFMLDrawable>(index);
	auto timeBackground = std::make_shared<sf::RectangleShape>(sf::Vector2f(100, 70));
	timeBackground->setFillColor({});
	drawable.m_drawables[ECS_Core::Components::DrawLayer::MENU][0].push_back({ timeBackground,{} });
	for (auto&& dataStr : uiFrameComponent.m_dataStrings)
	{
		dataStr.second.m_text->setFillColor({ 255,255,255 });
		dataStr.second.m_text->setOutlineColor({ 128,128,128 });
		dataStr.second.m_text->setFont(s_font);
		drawable.m_drawables[ECS_Core::Components::DrawLayer::MENU][255].push_back({ dataStr.second.m_text, dataStr.second.m_relativePosition });
	}
}

void Time::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	switch (phase)
	{
	case GameLoopPhase::PREPARATION:
		m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_TimeTracker>([frameDuration](
			ecs::EntityIndex mI,
			ECS_Core::Components::C_TimeTracker& time)
		{
			if (time.m_paused)
			{
				time.m_frameDuration = 0;
			}
			else
			{
				time.m_frameDuration = 0.000001 * frameDuration * time.m_gameSpeed;
				time.m_dayProgress += time.m_frameDuration;
			}
			if (time.m_dayProgress >= 1)
			{
				time.m_dayProgress -= 1;
				if (++time.m_day > 30)
				{
					time.m_day -= 30;
					if (++time.m_month > 12)
					{
						time.m_month -= 12;
						++time.m_year;
					}
				}
			}
		});
		break;
	case GameLoopPhase::ACTION:
		// Adjust timescale, pause/unpause
	{
		for (auto&& inputEntity : m_managerRef.entitiesMatching<ECS_Core::Signatures::S_Input>())
		{
			auto& inputs = m_managerRef.getComponent<ECS_Core::Components::C_UserInputs>(inputEntity);
			if (inputs.m_newKeyUp.count(ECS_Core::Components::InputKeys::BACKSPACE))
			{
				m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_TimeTracker>([](
					ecs::EntityIndex mI,
					ECS_Core::Components::C_TimeTracker& time)
				{
					time.m_paused = !time.m_paused;
				});
				inputs.ProcessKey(ECS_Core::Components::InputKeys::BACKSPACE);
			}
			if (inputs.m_newKeyUp.count(ECS_Core::Components::InputKeys::EQUAL))
			{
				m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_TimeTracker>([](
					ecs::EntityIndex mI,
					ECS_Core::Components::C_TimeTracker& time)
				{
					time.m_gameSpeed = min<int>(5, ++time.m_gameSpeed);

				});
				inputs.ProcessKey(ECS_Core::Components::InputKeys::EQUAL);
			}
			if (inputs.m_newKeyUp.count(ECS_Core::Components::InputKeys::DASH))
			{
				m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_TimeTracker>([](
					ecs::EntityIndex mI,
					ECS_Core::Components::C_TimeTracker& time)
				{
					time.m_gameSpeed = max<int>(1, --time.m_gameSpeed);
				});
				inputs.ProcessKey(ECS_Core::Components::InputKeys::DASH);
			}
		}
	}
	break;

	case GameLoopPhase::INPUT:
	case GameLoopPhase::ACTION_RESPONSE:
	case GameLoopPhase::RENDER:
	case GameLoopPhase::CLEANUP:
		return;
	}
}

bool Time::ShouldExit()
{
	return false;
}

DEFINE_SYSTEM_INSTANTIATION(Time);