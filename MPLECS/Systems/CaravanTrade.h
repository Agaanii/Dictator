//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// ECS/CaravanTrade.h
// Template for the declaration of a System
// This is where any other functions would get put if needed

#include "../ECS/System.h"

enum class TradeType
{
	PREFER_EXCHANGE,
	HIGHEST_AVAILABLE
};

class CaravanTrade : public SystemBase
{
public:
	CaravanTrade() : SystemBase() { }
	virtual ~CaravanTrade() {}
	virtual void ProgramInit() override;
	virtual void SetupGameplay() override;
	virtual void Operate(GameLoopPhase phase, const timeuS& frameDuration) override;
	virtual bool ShouldExit() override;
protected:
	void PerformTrade(
		ECS_Core::Components::C_ResourceInventory& caravanInventory,
		ECS_Core::Components::C_ResourceInventory& buildingInventory,
		TradeType tradeType);
};
template <> std::unique_ptr<CaravanTrade> InstantiateSystem();