//-----------------------------------------------------------------------------
// All code is property of Matthew Loesby
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2017

// Systems/SystemTemplate.cpp
// The boilerplate system code, to ease new system creation

#include "System.h"

#include "ECS.h"

namespace Graphics
{
	class SystemTemplate : public SystemBase
	{
	public:
		SystemTemplate() : SystemBase() { }
		virtual ~SystemTemplate() {}
		virtual void Operate(GameLoopPhase phase, const timeuS& frameDuration) override
		{
			switch (phase)
			{
			case GameLoopPhase::PREPARATION:
			case GameLoopPhase::ACTION:
			case GameLoopPhase::INPUT:
			case GameLoopPhase::RENDER:
			case GameLoopPhase::CLEANUP:
				return;
			}
		}

		virtual bool ShouldExit() override
		{
			return false;
		}
	};

	// Uncomment to have the system created at program startup and registered in the main system registry
	// SystemRegistrar<SystemTemplate> registration;
}
