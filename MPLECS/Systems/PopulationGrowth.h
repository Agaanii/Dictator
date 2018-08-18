//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// ECS/PopulationGrowth.h
// Template for the declaration of a System
// This is where any other functions would get put if needed

#include "../ECS/System.h"
class PopulationGrowth : public SystemBase
{
public:
	PopulationGrowth() : SystemBase() { }
	virtual ~PopulationGrowth() {}
	virtual void ProgramInit() override;
	virtual void SetupGameplay() override;
	virtual void Operate(GameLoopPhase phase, const timeuS& frameDuration) override;
	virtual bool ShouldExit() override;
protected:
	void GainLevels();
	void AgePopulations();
	void BirthChildren();
	void FeedWomen(
		const ECS_Core::Components::C_TimeTracker& time,
		f64& foodAmount,
		ECS_Core::Components::PopulationSegment& pop);
	void FeedMen(
		const ECS_Core::Components::C_TimeTracker& time,
		f64& foodAmount,
		ECS_Core::Components::PopulationSegment& pop);

	void ConsumeResources();
	void CauseNaturalDeaths();
};
template <> std::unique_ptr<PopulationGrowth> InstantiateSystem();