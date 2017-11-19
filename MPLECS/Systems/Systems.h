//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Systems/Systems.h
// Listing of all System classes, just forward declarations
// This gives the main file access to instantiate them in the correct order

#pragma once

#include "../ECS/System.h"

template<typename SystemType>
std::unique_ptr<SystemType> InstantiateSystem();

#define DECLARE_SYSTEM(SystemName, ConstructorBody, Member)								\
class SystemName : public SystemBase													\
{																						\
public:																					\
	SystemName() : SystemBase() { ConstructorBody; }									\
	virtual ~SystemName() {}															\
	virtual void Operate(GameLoopPhase phase, const timeuS& frameDuration) override;	\
	virtual bool ShouldExit() override;													\
	Member;																				\
};																						\
template <> std::unique_ptr<SystemName> InstantiateSystem();

DECLARE_SYSTEM(DamageApplication,,);
DECLARE_SYSTEM(NewtonianMovement,,);
DECLARE_SYSTEM(SFMLManager,,);
DECLARE_SYSTEM(UnitDeath,,);
DECLARE_SYSTEM(BuildingCreation,,);
DECLARE_SYSTEM(WorldTile,,);
DECLARE_SYSTEM(Government,
	m_localPlayerGovernment = m_managerRef.createHandle(); 
	m_managerRef.addComponent<ECS_Core::Components::C_Realm>(m_localPlayerGovernment);
	m_managerRef.addComponent<ECS_Core::Components::C_ResourceInventory>(m_localPlayerGovernment),
	ecs::Impl::Handle m_localPlayerGovernment);

#define DEFINE_SYSTEM_INSTANTIATION(System)		\
	template<>									\
	std::unique_ptr<System> InstantiateSystem()	\
	{											\
		return std::make_unique<System>();		\
	}

// Make the compiler happy with our template
// No need to register this one (Though it won't be a lot of operations anyway)
DECLARE_SYSTEM(SystemTemplate,,);