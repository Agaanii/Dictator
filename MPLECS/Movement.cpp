#include "System.h"

namespace Experiment
{
	class NewtonianMovement : public SystemBase
	{
	public:
		NewtonianMovement() : SystemBase() { }
		virtual ~NewtonianMovement() {}
		virtual void Operate(GameLoopPhase phase, const timeuS& frameDuration) override
		{
			if (phase != GameLoopPhase::ACTION)
			{
				// We only act during the action phase
				return;
			}
			f64 frameSeconds = (1. * frameDuration / 1000000);
			m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_ApplyConstantMotion>(
				[&frameSeconds](
					ecs::EntityIndex mI,
					ECS_Core::Components::C_PositionCartesian& position,
					const ECS_Core::Components::C_VelocityCartesian& velocity) {
				position.m_position += velocity.m_velocity * frameSeconds;
			});

			m_managerRef.forEntitiesMatching<ECS_Core::Signatures::S_ApplyNewtonianMotion>(
				[&frameSeconds](
					ecs::EntityIndex mI,
					ECS_Core::Components::C_PositionCartesian& position,
					ECS_Core::Components::C_VelocityCartesian& velocity,
					const ECS_Core::Components::C_AccelerationCartesian& acceleration) {
				position.m_position += (velocity.m_velocity * frameSeconds) + (acceleration.m_acceleration * frameSeconds * frameSeconds);
			});
		}

		virtual bool ShouldExit() override
		{
			return false;
		}
	};

	SystemRegistrar<NewtonianMovement> registration;
}
