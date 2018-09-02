//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// ECS/InputTranslation.h
// Template for the declaration of a System
// This is where any other functions would get put if needed

#include "../ECS/System.h"

#include <functional>
#include <map>

class InputTranslation : public SystemBase
{
public:
	InputTranslation();
	virtual ~InputTranslation() {}
	virtual void ProgramInit() override;
	virtual void SetupGameplay() override;
	virtual void Operate(GameLoopPhase phase, const timeuS& frameDuration) override;
	virtual bool ShouldExit() override;
protected:
	bool CheckPlaceBuildingCommand(
		ECS_Core::Components::C_UserInputs& inputs,
		ECS_Core::Components::C_ActionPlan& actionPlan);
	bool CheckStartTargetedMovement(
		ECS_Core::Components::C_UserInputs& inputs,
		ECS_Core::Components::C_ActionPlan& actionPlan);
	void TranslateDownClicks(
		ECS_Core::Components::C_UserInputs& inputs,
		ECS_Core::Components::C_ActionPlan& actionPlan);
	void CreateBuildingGhost(
		ECS_Core::Components::C_UserInputs& inputs,
		ECS_Core::Components::C_ActionPlan& actionPlan);
	void TogglePause(
		ECS_Core::Components::C_UserInputs& inputs,
		ECS_Core::Components::C_ActionPlan& actionPlan);
	void IncreaseGameSpeed(
		ECS_Core::Components::C_UserInputs& inputs,
		ECS_Core::Components::C_ActionPlan& actionPlan);
	void DecreaseGameSpeed(
		ECS_Core::Components::C_UserInputs& inputs,
		ECS_Core::Components::C_ActionPlan& actionPlan);
	void CancelMovementPlan(
		ECS_Core::Components::C_UserInputs& inputs,
		ECS_Core::Components::C_ActionPlan& actionPlan);

	std::map<ECS_Core::Components::InputKeys, std::function<void(
		ECS_Core::Components::C_UserInputs&,
		ECS_Core::Components::C_ActionPlan&)>> m_functions;
};
template <> std::unique_ptr<InputTranslation> InstantiateSystem();