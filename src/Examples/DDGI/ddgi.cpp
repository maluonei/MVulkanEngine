#include "ddgi.hpp"

DDGIVolume::DDGIVolume(glm::vec3 startPosition, glm::vec3 offset):
	m_startPosition(startPosition), m_offset(offset)
{

}

glm::vec3 DDGIVolume::GetProbePosition(int x, int y, int z)
{
	return m_startPosition + glm::vec3(x, y, z) * m_offset;
}

glm::vec3 DDGIVolume::GetProbePosition(int index)
{
	int x = index / 64;
	int y = index % 64 / 8;
	int z = index % 8;
	return GetProbePosition(x, y, z);
}
