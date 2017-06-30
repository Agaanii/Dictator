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
	}

	using MasterComponentList = ecs::ComponentList<
		Components::C_PositionCartesian,
		Components::C_VelocityCartesian,
		Components::C_AccelerationCartesian,
		Components::C_SFMLShape
	>;

	using MasterTagList = ecs::TagList<Tags::T_NoAcceleration>;

	using MasterSignatureList = ecs::SignatureList<
		Signatures::S_ApplyConstantMotion,
		Signatures::S_ApplyNewtonianMotion,
		Signatures::S_Drawable
	>;

	using MasterSettings = ecs::Settings<MasterComponentList, MasterTagList, MasterSignatureList>;
	using Manager = ecs::Manager<MasterSettings>;
}
