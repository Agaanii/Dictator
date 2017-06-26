//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// main.cpp 
// entry point and core loop (obviously)

#include <iostream>

#include "ECS/Entity.h"
#include "ECS/System.h"

#include "Core/typedef.h"

#include <chrono>
#include <vector>

using namespace std;

vector<Entity> s_entities;
vector<System> s_systems;

int main()
{
	auto loopStart = chrono::high_resolution_clock::now();
	bool shouldTerminate = false;
	while(true)
	{
		auto now = chrono::high_resolution_clock::now();
		auto loopDuration = now - loopStart;
		for (auto& system : s_systems)
		{
			if (!system.Operate(loopDuration, s_entities))
			{
				return 0;
			}
		}
		loopStart = now;
	}
}
