//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Components/UIComponents.h
// Components involved in UI Data Bindings

#pragma once

#include "../ECS/ECS.h"

template <typename COMPONENT_TYPE, typename VALUE_TYPE>
class UIDataBind
{
public:
	using ComponentType = COMPONENT_TYPE;
	using ValueType = VALUE_TYPE;
	UIDataBind(VALUE_TYPE COMPONENT_TYPE::* memberPtr) : m_memberPtr(memberPtr) {}

	VALUE_TYPE ReadValue(const COMPONENT_TYPE& component) const
	{
		return component.*m_memberPtr;
	}
protected:
	VALUE_TYPE COMPONENT_TYPE::* m_memberPtr;
};

#define DataBinding(Type, MemberName) UIDataBind<Type, decltype(Type::MemberName)>(&Type::MemberName)
// Example: auto dataBinding = DataBinding(ECS_Core::Components::C_BuildingGhost, m_currentPlacementValid);

template<typename ...DataBindings>
class UIFrameDefinition : public ECS_Core::Components::UIFrame
{
	using BindingTuple = typename std::tuple<DataBindings...>;
	static const int c_tupleCount = static_cast<int>(std::tuple_size<BindingTuple>::value);

	static_assert(c_tupleCount > 0, "Don't Use Data Bindings if you don't need data bindings");
protected:

	template <int i> void ReadFields(FieldStrings& result, ecs::EntityIndex mI, ECS_Core::Manager& manager) const
	{
		auto& component = manager.getComponent<std::tuple_element<i, BindingTuple>::type::ComponentType>(mI);
		result[i] = std::to_string(std::get<i>(m_bindings).ReadValue(component));
		ReadFields<i - 1>(result, mI, manager);
	}

	template <> void ReadFields<0>(FieldStrings& result, ecs::EntityIndex mI, ECS_Core::Manager& manager) const
	{
		auto& component = manager.getComponent<std::tuple_element<0, BindingTuple>::type::ComponentType>(mI);
		result[0] = std::to_string(std::get<0>(m_bindings).ReadValue(component));
	}

public:
	// Title gets printed at top of frame
	// Description gets printed at bottom of frame, filling in values where {#} tokens are
	// Example: "This unit has {0} hp and {1} mana"
	UIFrameDefinition(std::string title, std::string description, DataBindings&&... bindings)
		: ECS_Core::Components::UIFrame()
		, m_title(title)
		, m_description(description)
		, m_bindings(bindings...)
	{
	}

	virtual FieldStrings ReadData(ecs::EntityIndex mI, ECS_Core::Manager& manager) const override
	{
		std::map<int, std::string> result;
		// Recursively go through the fields, put string in map
		ReadFields<c_tupleCount - 1>(result, mI, manager);
		return result;
	}

protected:

	std::string m_title;
	std::string m_description;
	BindingTuple m_bindings;
};