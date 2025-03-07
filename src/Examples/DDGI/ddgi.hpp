#ifndef DDGI_HPP
#define DDGI_HPP

//#include "MVulkanRHI/MVulkanBuffer.hpp"
#include <vector>
#include <glm/glm.hpp>
#include "ddgiShader.hpp"

//9.920306, 0.7, 3.9827316
//-11.015438, 6.619673, -4.404496
//            10.615406

//-11.015438, 0.7, -4.404496
//2.5, 1.3, 1.05


class DDGIVolume {
public:
	DDGIVolume(glm::vec3 startPosition, glm::vec3 offset);
	DDGIVolume(glm::vec3 startPosition, glm::vec3 endPosition, glm::ivec3 probeNum);

	glm::vec3 GetProbePosition(int x, int y, int z);
	glm::vec3 GetProbePosition(int index);

	inline int GetNumProbes() const {
		return m_probeNum.x * m_probeNum.y * m_probeNum.z;
	}

	inline glm::ivec3 GetProbeDim() const {
		return m_probeNum;
	}

	inline ProbeBuffer& GetProbeBuffer() {
		return m_probeBuffer;
	}

	inline ModelBuffer& GetModelBuffer() {
		return m_modelBuffer;
	}

private:
	void InitProbeBufferAndModelBuffer();

private:
	glm::vec3	m_startPosition;
	glm::vec3	m_offset;
	glm::ivec3	m_probeNum;

	ProbeBuffer m_probeBuffer;
	ModelBuffer m_modelBuffer;
};

#endif