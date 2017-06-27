//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// main.cpp 
// entry point and core loop (obviously)

#include <iostream>

#include "System.h"
#include "ECS.h"
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

void RegisterSystem(GameLoopPhase phase, unique_ptr<SystemBase>&& system)
{
	s_systems.emplace_back(move(system));
}

int main()
{
	auto loopStart = chrono::high_resolution_clock::now();
	while(true)
	{
		auto now = chrono::high_resolution_clock::now();
		auto loopDuration = chrono::duration_cast<chrono::microseconds>(now - loopStart).count();
		for (auto& system : s_systems)
		{
			system->Operate(loopDuration);
			if (system->ShouldExit())
			{
				return 0;
			}
		}
		s_manager.refresh();
		loopStart = now;
	}
}
