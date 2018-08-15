//-----------------------------------------------------------------------------
// All code is property of Dictator Developers Inc
// Contact at Loesby.dev@gmail.com for permission to use
// Or to discuss ideas
// (c) 2018

#include "WorkerStructs.h"

bool WorkerSkillKey::operator< (const WorkerSkillKey& other) const
{
	if (m_level < other.m_level) return true;
	if (m_level > other.m_level) return false;

	if (m_xp < other.m_xp) return true;
	if (m_xp > other.m_xp) return false;
	return false;

}