//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// ECS/Government.h
// Template for the declaration of a System
// This is where any other functions would get put if needed

#include "../ECS/System.h"

#include "../Util/WorkerStructs.h"

class Government : public SystemBase
{
public:
	Government() : SystemBase() {
		m_buildingCosts[0] =
		{
			{ ECS_Core::Components::Yields::FOOD, 50 },
			{ ECS_Core::Components::Yields::WOOD, 50 }
		};
	}
	virtual ~Government() {}
	virtual void ProgramInit() override;
	virtual void SetupGameplay() override;
	void MovePopulations(
		ECS_Core::Components::C_Population& populationSource,
		ECS_Core::Components::C_Population& populationTarget,
		int totalMenToMove,
		int totalWomenToMove);
	void MoveFullPopulation(
		ECS_Core::Components::C_Population& populationSource,
		ECS_Core::Components::C_Population& populationTarget);
	virtual void Operate(GameLoopPhase phase, const timeuS& frameDuration) override;
	virtual bool ShouldExit() override;
protected:
	void BeginBuildingConstruction(const ecs::EntityIndex& ghostEntity);
	bool CreateBuildingGhost(ecs::Impl::Handle &governor, TilePosition & position, int buildingType);
	void ConstructRequestedBuildings();
	void UpdateAgendas();

	void GainIncomes();
	f64 AssignYieldWorkers(
		std::pair<const WorkerSkillKey, std::vector<WorkerProductionValue>> & skillLevel,
		WorkerAssignmentMap &assignments,
		f64 &amountToWork,
		const int& yield);

	std::map<int, ECS_Core::Components::YieldBuckets> m_buildingCosts;
};
template <> std::unique_ptr<Government> InstantiateSystem();