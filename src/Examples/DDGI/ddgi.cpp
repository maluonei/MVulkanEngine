#include "ddgi.hpp"

DDGIVolume::DDGIVolume(glm::vec3 startPosition, glm::vec3 offset):
	m_startPosition(startPosition), m_offset(offset), m_probeNum(glm::ivec3(8, 8, 8))
{

}

DDGIVolume::DDGIVolume(glm::vec3 startPosition, glm::vec3 endPosition, glm::ivec3 probeNum):
	m_startPosition(startPosition), m_offset((endPosition - startPosition) / glm::vec3(probeNum - glm::ivec3(1.f))), m_probeNum(probeNum)
{
}

glm::vec3 DDGIVolume::GetProbePosition(int x, int y, int z)
{
	return m_startPosition + glm::vec3(x, y, z) * m_offset;
}

glm::vec3 DDGIVolume::GetProbePosition(int index)
{
	auto probeNumYZ = m_probeNum.y * m_probeNum.z;

	int x = index / probeNumYZ;
	int y = (index % probeNumYZ) / m_probeNum.z;
	int z = index % m_probeNum.z;
	return GetProbePosition(x, y, z);
}
