//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// ECS/Education.h
// Template for the declaration of a System
// This is where any other functions would get put if needed

#include "../ECS/System.h"
class Education : public SystemBase
{
public:
	Education() : SystemBase() { }
	virtual ~Education() {}
	virtual void ProgramInit() override;
	virtual void SetupGameplay() override;
	virtual void Operate(GameLoopPhase phase, const timeuS& frameDuration) override;
	virtual bool ShouldExit() override;
protected:
	void TeachChildren();
};
template <> std::unique_ptr<Education> InstantiateSystem();