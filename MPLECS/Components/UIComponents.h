//-----------------------------------------------------------------------------
// All code is property of Dictator Developers Inc
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2018

// Components/UIComponents.h
// Components involved in UI Data Bindings

#pragma once

#include "../ECS/ECS.h"

#include <functional>

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
	const VALUE_TYPE COMPONENT_TYPE::* m_memberPtr;
};

#define DataBinding(Type, MemberName) UIDataBind<Type, decltype(Type::MemberName)>(&Type::MemberName)
// Example: auto dataBinding = DataBinding(ECS_Core::Components::C_BuildingGhost, m_currentPlacementValid);

template<class T> 
struct is_map
{
	static constexpr bool value = false;
};

template<class Key, class Value>
struct is_map<std::map<Key, Value>> 
{
	static constexpr bool value = true;
};

template<typename COMPONENT_TYPE, typename VALUE_TYPE>
class UIDataReader
{
public:
	using ComponentType = COMPONENT_TYPE;
	using ValueType = VALUE_TYPE;
	using ValueFunction = std::function<ValueType(const ComponentType&)>;
	UIDataReader(const ValueFunction& function) : m_func(function) {}

	ValueType ReadValue(const ComponentType& component) const { return m_func(component); }
protected:
	ValueFunction m_func;
};

template<typename ...DataBindings>
class UIFrameDefinition : public ECS_Core::Components::UIFrame
{
	using BindingTuple = typename std::tuple<DataBindings...>;
	static constexpr int c_tupleCount = static_cast<int>(std::tuple_size<BindingTuple>::value);

	static_assert(c_tupleCount > 0, "Don't Use Data Bindings if you don't need data bindings");
protected:

	template <typename FieldType> 
	void GetFieldEntry(
		int fieldIndex,
		const FieldType& value,
		FieldStrings& resultOut) const
	{
		resultOut[{fieldIndex}] = std::to_string(value);
	}

	template<typename FieldType>
	void GetFieldEntry_Recurse(
		std::vector<int>& key,
		const FieldType& mapOrValue,
		FieldStrings& resultOut) const
	{
		if constexpr(is_map<FieldType>::value)
		{
			for (auto&& entry : mapOrValue)
			{
				key.push_back(static_cast<int>(entry.first));
				GetFieldEntry_Recurse(key, entry.second, resultOut);
				key.pop_back();
			}
		}
		else
		{
			// End case
			resultOut[key] = std::to_string(mapOrValue);
		}
	}

	template <typename KeyType, typename MapEntryType>
	void GetFieldEntry(
		int fieldIndex,
		const std::map<KeyType, MapEntryType>& map,
		FieldStrings& resultOut) const
	{
		std::vector<int> key{ fieldIndex };
		for (auto&& entry : map)
		{
			key.push_back(static_cast<int>(entry.first));
			GetFieldEntry_Recurse(key, entry.second, resultOut);
			key.pop_back();
		}
	}

	template <int i> void ReadFields(
		FieldStrings& result, 
		ecs::EntityIndex mI, 
		ECS_Core::Manager& manager) const
	{
		auto& component = manager.getComponent
			<std::tuple_element<i, BindingTuple>::type::ComponentType>(mI);
		GetFieldEntry(i, std::get<i>(m_bindings).ReadValue(component), result);
		ReadFields<i - 1>(result, mI, manager);
	}

	template <> void ReadFields<0>(
		FieldStrings& result,
		ecs::EntityIndex mI,
		ECS_Core::Manager& manager) const
	{
		auto& component = manager.getComponent
			<std::tuple_element<0, BindingTuple>::type::ComponentType>(mI);
		GetFieldEntry(0, std::get<0>(m_bindings).ReadValue(component), result);
	}

public:
	// Title gets printed at top of frame
	// Description gets printed at bottom of frame, filling in values where {#} tokens are
	// Example: "This unit has {0} hp and {1} mana"
	UIFrameDefinition(std::string title, DataBindings&&... bindings)
		: ECS_Core::Components::UIFrame()
		, m_title(title)
		, m_bindings(bindings...)
	{
	}
	
	virtual FieldStrings ReadData(
		ecs::EntityIndex mI,
		ECS_Core::Manager& manager) const override
	{
		FieldStrings result;
		// Recursively go through the fields, put string in map
		ReadFields<c_tupleCount - 1>(result, mI, manager);
		return result;
	}

protected:
	std::string m_title;
	BindingTuple m_bindings;
};

// Thanks to reddit user /u/YouFeedTheFish
template<typename ...Args>
auto DefineUIFrame(std::string&& title, Args&&... args)
{
	return std::make_unique<UIFrameDefinition<Args...>>(
		std::forward<std::string>(title), 
		std::forward<Args>(args)...);
}
