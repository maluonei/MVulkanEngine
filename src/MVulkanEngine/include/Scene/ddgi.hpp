#ifndef DDGI_HPP
#define DDGI_HPP

//#include "MVulkanRHI/MVulkanBuffer.hpp"
#include <vector>
#include <glm/glm.hpp>
//#include "ddgiShader.hpp"

//9.920306, 0.7, 3.9827316
//-11.015438, 6.619673, -4.404496
//            10.615406

//-11.015438, 0.7, -4.404496
//2.5, 1.3, 1.05

//struct Probe {
//	glm::vec3 position;
//	int    padding0;
//};

struct VertexBuffer {
	std::vector<glm::vec3> position;

	inline int GetSize() const { return position.size() * sizeof(glm::vec3); }
};

struct IndexBuffer {
	std::vector<int> index;

	inline int GetSize() const { return index.size() * sizeof(int); }
};

struct NormalBuffer {
	std::vector < glm::vec3> normal;

	inline int GetSize() const { return normal.size() * sizeof(glm::vec3); }
};

struct UVBuffer {
	std::vector < glm::vec2> uv;

	inline int GetSize() const { return uv.size() * sizeof(glm::vec2); }
};

struct GeometryInfo {
	int vertexOffset;
	int indexOffset;
	int uvOffset;
	int normalOffset;
	int materialIdx;
};

struct InstanceOffset {
	std::vector<GeometryInfo> geometryInfos;

	inline int GetSize() const { return geometryInfos.size() * sizeof(GeometryInfo); }
};

struct ProbeData {
	glm::vec3   position;
	int			probeState = 0;
	glm::vec3   offset = glm::vec3(0.f, 0.f, 0.f);

	float		padding0;
	//glm::vec3	closestFrontfaceDirection;
	//float		closestFrontfaceDistance;
	//glm::vec3	farthestFrontfaceDirection;
	//float		closestBackfaceDistance;
};

struct ProbeBuffer {
	std::vector<ProbeData> probes;

	inline int GetSize() const { return probes.size() * sizeof(ProbeData); }
};

struct Model {
	glm::mat4 Model;
};


struct ModelsBuffer {
	std::vector<Model> models;

	inline int GetSize() const { return models.size() * sizeof(Model); }
};


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

	inline ModelsBuffer& GetModelBuffer() {
		return m_modelBuffer;
	}

	void SetRandomRotation();

	inline glm::vec4 GetQuaternion() const {
		return m_quaternion;
	}

private:
	void InitProbeBufferAndModelBuffer();

private:
	glm::vec3	m_startPosition;
	glm::vec3	m_offset;
	glm::ivec3	m_probeNum;
	glm::vec4   m_quaternion;

	ProbeBuffer m_probeBuffer;
	ModelsBuffer m_modelBuffer;
};

#endif