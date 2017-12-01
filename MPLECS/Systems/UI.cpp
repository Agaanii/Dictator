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
#include "../ECS/ECS.h"

template <typename COMPONENT_TYPE, typename VALUE_TYPE> 
class UIDataBind
{
public:
	UIDataBind(VALUE_TYPE COMPONENT_TYPE::* memberPtr) : m_memberPtr(memberPtr) {}

	VALUE_TYPE ReadValue(const COMPONENT_TYPE& component)
	{
		return component.*m_memberPtr;
	}
protected:
	VALUE_TYPE COMPONENT_TYPE::* m_memberPtr;
};

#define DataBinding(Type, MemberName) UIDataBind<Type, decltype(Type::MemberName)>(&Type::MemberName)
// Example: auto dataBinding = DataBinding(ECS_Core::Components::C_BuildingGhost, m_currentPlacementValid);

template<typename ...DataBindings>
class UIFrameDefinition : ECS_Core::Components::C_UIFrame
{
	using BindingTuple = typename std::tuple<DataBindings...>;
	static const int c_tupleCount = static_cast<int>(std::tuple_size<BindingTuple>::value);
public:
	// Title gets printed at top of frame
	// Description gets printed at bottom of frame, filling in values where {#} tokens are
	// Example: "This unit has {0} hp and {1} mana"
	UIFrameDefinition(std::string title, std::string description, DataBindings&&... bindings)
		:m_title(title)
		, m_description(description)
		, m_bindings(bindings...)
	{
	}

protected:

	std::map<int, std::string> ReadData() const
	{
		std::map<int, std::string> result;
		for (int i = 0; i < c_tupleCount; ++i)
		{
			result[i] = std::to_string(m_bindings.get<i>());
		}
	}

	std::string m_title;
	std::string m_description;
	BindingTuple m_bindings;
};

auto thing = UIFrameDefinition<UIDataBind<ECS_Core::Components::C_BuildingGhost, bool>, UIDataBind<ECS_Core::Components::C_BuildingDescription, u64>>(
	"Title",
	"Descri{0}ption{1}",
	DataBinding(ECS_Core::Components::C_BuildingGhost, m_currentPlacementValid),
	DataBinding(ECS_Core::Components::C_BuildingDescription, m_buildingType));

void UI::SetupGameplay() {

}

void UI::Operate(GameLoopPhase phase, const timeuS& frameDuration)
{
	switch (phase)
	{
	case GameLoopPhase::PREPARATION:
	case GameLoopPhase::INPUT:
	case GameLoopPhase::ACTION:
	case GameLoopPhase::ACTION_RESPONSE:
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