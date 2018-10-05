#include "ECS.h"

bool ECS_Core::Components::PopulationKey::operator<(const PopulationKey& other) const
{
	if (other.m_birthMonthIndex < m_birthMonthIndex) return false;
	if (other.m_birthMonthIndex > m_birthMonthIndex) return true;
	return m_segmentIndex < other.m_segmentIndex;
}