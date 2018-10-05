//-----------------------------------------------------------------------------
// All code is property of Dictator Developers Inc
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2018

#include "../Core/typedef.h"
#include "../ECS/ECS.h"

#include <map>

struct WorkerProductionValue
{
	WorkerProductionValue() = default;
	WorkerProductionValue(const ECS_Core::Components::PopulationKey& workerKey, const f64& productionValue)
		: m_workerKey(workerKey)
		, m_productiveValue(productionValue)
	{ }
	ECS_Core::Components::PopulationKey m_workerKey;
	f64 m_productiveValue{ 0 };
};

struct WorkerAssignment
{
	using AssignmentMap = std::map<ECS_Core::Components::SpecialtyId, WorkerProductionValue>;
	AssignmentMap m_assignments;
};

using WorkerAssignmentMap = std::map<ECS_Core::Components::PopulationKey, WorkerAssignment>;

struct WorkerSkillKey
{
	ECS_Core::Components::SpecialtyLevel m_level;
	ECS_Core::Components::SpecialtyExperience m_xp;

	bool operator< (const WorkerSkillKey& other) const;
};

using WorkerSkillMap = std::map<WorkerSkillKey, std::vector<WorkerProductionValue>>;
using SkillMap = std::map<ECS_Core::Components::SpecialtyId, WorkerSkillMap>;