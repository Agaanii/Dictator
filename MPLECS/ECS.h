#pragma once

#include "ecs/ecs.hpp"

#include "Core/typedef.h"

#include <SFML/Graphics.hpp>
#include <memory>

namespace ECS_Core
{
	namespace Components
	{
		struct C_PositionCartesian
		{
			CartesianVector m_position;
		};

		struct C_VelocityCartesian
		{
			CartesianVector m_velocity;
		};

		struct C_AccelerationCartesian
		{
			CartesianVector m_acceleration;
		};

		struct C_SFMLShape
		{
			std::unique_ptr<sf::Shape> m_shape;
		};

		struct C_Health
		{
			C_Health() {}
			C_Health(int maxHealth) : m_maxHealth(maxHealth), m_currentHealth(maxHealth) {}
			f64 m_maxHealth{ 0 };
			f64 m_currentHealth{ 0 };
		};

		struct C_Healable
		{
			f64 m_healingThisFrame;
			struct HealOverTime
			{
				f64 m_secondsRemaining;
				f64 m_healingPerFrame;
			};
			std::vector<HealOverTime> m_hots;
		};

		struct C_Damageable
		{
			f64 m_damageThisFrame;
			struct DamageOverTime
			{
				DamageOverTime(f64 frames, f64 damagePerFrame)
					: m_secondsRemaining(frames)
					, m_damagePerFrame(damagePerFrame)
				{ }
				f64 m_secondsRemaining;
				f64 m_damagePerFrame;
			};
			std::vector<DamageOverTime> m_dots;
		};

		struct C_UserInputs
		{

		};
	}

	namespace Tags
	{
		struct T_NoAcceleration {};
	}

	namespace Signatures
	{
		using S_ApplyConstantMotion = ecs::Signature<Components::C_PositionCartesian, Components::C_VelocityCartesian, Tags::T_NoAcceleration>;
		using S_ApplyNewtonianMotion = ecs::Signature<Components::C_PositionCartesian, Components::C_VelocityCartesian, Components::C_AccelerationCartesian>;
		using S_Drawable = ecs::Signature<Components::C_PositionCartesian, Components::C_SFMLShape>;
		using S_Living = ecs::Signature<Components::C_Health>;
		using S_Health = ecs::Signature<Components::C_Health, Components::C_Healable, Components::C_Damageable>;
	}

	using MasterComponentList = ecs::ComponentList<
		Components::C_PositionCartesian,
		Components::C_VelocityCartesian,
		Components::C_AccelerationCartesian,
		Components::C_SFMLShape,
		Components::C_Health,
		Components::C_Healable,
		Components::C_Damageable
	>;

	using MasterTagList = ecs::TagList<Tags::T_NoAcceleration>;

	using MasterSignatureList = ecs::SignatureList<
		Signatures::S_ApplyConstantMotion,
		Signatures::S_ApplyNewtonianMotion,
		Signatures::S_Drawable,
		Signatures::S_Living,
		Signatures::S_Health
	>;

	using MasterSettings = ecs::Settings<MasterComponentList, MasterTagList, MasterSignatureList>;
	using Manager = ecs::Manager<MasterSettings>;
}
