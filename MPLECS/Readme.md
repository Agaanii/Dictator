Using the Template Metaprogramming-based Entity/Component/System architecture designed by Vittori Romeo.
Implementation of the library pulled from https://github.com/netgusto/ecs

Works like a standard ECS architecture. Each system is registered, and they operate in order each frame.
Currently, single-threaded. That will likely need to change in the future as world tile generation is fairly time-consuming.

Each game loop is divided into several phases: Preparation, Input, Action, Action-Response, Render, and Cleanup.
Preparation sets up any objects which will only live for the current frame; currently, it's not much used, but optimization will likely change that.
Input uses the SFML library to translate OS events into usable input formats. It then translates UI interactions into planned game actions.
Action is when most planned actions are carried out
Action response is reserved for the world to react to user actions
Render is exactly what it sounds like, drawing the world.
Cleanup is the phase wherein any unit that needs to be deleted will get its destructor called. Dying units will exist until the Cleanup phase completes.

Systems are all registered in the main.cpp file.
Components are defined in ECS.h, which also defines all the template magic that the NetGusto library uses.
Entities may be created and marked for deletion by any system.

No system should ever call a function in another system. Instead, an RPC-like communication can take place by generating an Entity with the proper Components to form a request, and then another system can replace the Request component with a Response.
As much as possible, data should be kept within Entities and Components; some cached data for optimization outside of this is all right, but it's a risk for bugs.
