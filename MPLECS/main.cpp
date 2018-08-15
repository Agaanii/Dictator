//-----------------------------------------------------------------------------
// All code is property of Dictator Developers Inc
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2018

// main.cpp 
// entry point and core loop (obviously)

#include <iostream>

#include "Systems/Systems.h"
#include "ECS/System.h"
#include "ECS/ECS.h"
#include "Core/typedef.h"

#include <chrono>
#include <memory>
#include <set>
#include <vector>

using namespace std;

ECS_Core::Manager s_manager;

std::vector<std::unique_ptr<SystemBase>> s_systems;

SystemBase::SystemBase()
	: m_managerRef(s_manager)
{

}

template <typename SYSTEM>
void RegisterSystem()
{
	s_systems.emplace_back(std::move(InstantiateSystem<SYSTEM>()));
}

int main()
{
	srand(static_cast<unsigned int>(chrono::high_resolution_clock::now().time_since_epoch().count()));
	// Systems registered in processing order
	
	// Give the UI first shot at any inputs. Draw order is separate from processing order
	// And we'll read input before the Action phase
	RegisterSystem<UI>();

	// Translate non-UI inputs into reality before time advances
	RegisterSystem<InputTranslation>();

	// Adjust timescale before running any other system
	RegisterSystem<Time>();

	RegisterSystem<DamageApplication>();
	RegisterSystem<NewtonianMovement>();
	RegisterSystem<Government>();
	RegisterSystem<PopulationGrowth>();
	RegisterSystem<BuildingCreation>();
	RegisterSystem<WorldTile>();
	RegisterSystem<CaravanTrade>();
	RegisterSystem<Education>();

	// Draw last
	RegisterSystem<SFMLManager>();

	// Kill units last, other things may want to refer to them
	RegisterSystem<UnitDeath>();

	for (auto&& system : s_systems)
	{
		system->ProgramInit();
	}
	s_manager.refresh();

	auto loopStart = chrono::high_resolution_clock::now();

	for (auto&& system : s_systems)
	{
		system->SetupGameplay();
	}
	s_manager.refresh();

	while(true)
	{
		auto now = chrono::high_resolution_clock::now();
		auto loopDuration = chrono::duration_cast<chrono::microseconds>(now - loopStart).count();
		static const vector<GameLoopPhase> c_phaseOrder = {
			GameLoopPhase::PREPARATION,
			GameLoopPhase::INPUT,
			GameLoopPhase::ACTION,
			GameLoopPhase::ACTION_RESPONSE,
			GameLoopPhase::RENDER,
			GameLoopPhase::CLEANUP
		};
		for (auto&& phase : c_phaseOrder)
		{
			for (auto&& system : s_systems)
			{
				system->Operate(phase, loopDuration);
				if (system->ShouldExit())
				{
					return 0;
				}
			}
		}
		s_manager.refresh();
		loopStart = now;
	}
}
