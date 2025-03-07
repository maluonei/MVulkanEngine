#include "ddgi.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>

DDGIVolume::DDGIVolume(glm::vec3 startPosition, glm::vec3 offset):
	m_startPosition(startPosition), m_offset(offset), m_probeNum(glm::ivec3(8, 8, 8))
{
	InitProbeBufferAndModelBuffer();
}

DDGIVolume::DDGIVolume(glm::vec3 startPosition, glm::vec3 endPosition, glm::ivec3 probeNum):
	m_startPosition(startPosition), m_offset((endPosition - startPosition) / glm::vec3(probeNum - glm::ivec3(1.f))), m_probeNum(probeNum)
{
	InitProbeBufferAndModelBuffer();
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

void DDGIVolume::InitProbeBufferAndModelBuffer()
{
	auto numProbes = m_probeNum.x * m_probeNum.y * m_probeNum.z;

	m_probeBuffer.probes.resize(numProbes);
	m_modelBuffer.models.resize(numProbes);
	for (auto i = 0; i < numProbes; i++) {
		m_probeBuffer.probes[i].position = GetProbePosition(i);

		glm::mat4 translation = glm::translate(glm::mat4(1.f), GetProbePosition(i));
		glm::mat4 scale = glm::scale(glm::mat4(1.f), glm::vec3(0.12f));
		m_modelBuffer.models[i].Model = translation * scale;
	}
}
