#ifndef DDGI_SHADER_HPP
#define DDGI_SHADER_HPP

#include "Shaders/ShaderModule.hpp"


class RTAOShader :public ShaderModule {
public:
	RTAOShader() :ShaderModule("hlsl/rtao/rtao.vert.hlsl", "hlsl/rtao/rtao.frag.hlsl")
	{ }

	virtual size_t GetBufferSizeBinding(uint32_t binding) const {
		switch (binding) {
		case 0:
			return sizeof(RTAOShader::UniformBuffer0);
		}
	}

	virtual void SetUBO(uint8_t binding, void* data) {
		switch (binding) {
		case 0:
			ubo0 = *reinterpret_cast<RTAOShader::UniformBuffer0*>(data);
			return;
		}
	}

	virtual void* GetData(uint32_t binding, uint32_t index = 0) {
		switch (binding) {
		case 0:
			return (void*)&ubo0;
		}
	}

public:
	struct Light {
		glm::vec3 direction;
		float intensity;

		glm::vec3 color;
		int shadowMapIndex;
	};

	struct UniformBuffer0 {
		Light lights[2];

		glm::vec3 cameraPos;
		int lightNum;

		int resetAccumulatedBuffer;
		int gbufferWidth;
		int gbufferHeight;
		float padding2;
	};

private:
	UniformBuffer0 ubo0;
};

class ProbeTracingShader :public ShaderModule {
public:
	ProbeTracingShader() :ShaderModule("hlsl/ddgi/fullScreen.vert.hlsl", "hlsl/ddgi/probeTrace.frag.hlsl")
	{}

	virtual size_t GetBufferSizeBinding(uint32_t binding) const {
		switch (binding) {
		case 0:
			return sizeof(UniformBuffer0);
		case 1:
			return sizeof(UniformBuffer1);
		case 2:
			return sizeof(UniformBuffer2);
		case 3:
			return vertexBuffer.GetSize();;
		case 4:
			return indexBuffer.GetSize();
		case 5:
			return normalBuffer.GetSize();
		case 6:
			return uvBuffer.GetSize();
		case 7:
			return instanceOffsetBuffer.GetSize();
		}
	}

	virtual void SetUBO(uint8_t binding, void* data) {
		switch (binding) {
		case 0:
			ubo0 = *reinterpret_cast<ProbeTracingShader::UniformBuffer0*>(data);
			return;
		case 1:
			ubo1 = *reinterpret_cast<ProbeTracingShader::UniformBuffer1*>(data);
			return;
		case 2:
			ubo2 = *reinterpret_cast<ProbeTracingShader::UniformBuffer2*>(data);
			return;
		}
	}

	virtual void* GetData(uint32_t binding, uint32_t index = 0) {
		switch (binding) {
		case 0:
			return (void*)&ubo0;
		case 1:
			return (void*)&ubo1;
		case 2:
			return (void*)&ubo2;
		case 3:
			return (void*)vertexBuffer.position.data();
		case 4:
			return (void*)indexBuffer.index.data();
		case 5:
			return (void*)normalBuffer.normal.data();
		case 6:
			return (void*)uvBuffer.uv.data();
		case 7:
			return (void*)instanceOffsetBuffer.geometryInfos.data();
		}
	}

public:
	struct Probe {
		glm::vec3 position;
		int padding0;
	};

	struct UniformBuffer1 {
		Probe probes[512];
	};

	struct TexBuffer {
		int diffuseTextureIdx;
		int metallicAndRoughnessTextureIdx;
		int matId;
		int padding0;
	};

	struct UniformBuffer0 {
		TexBuffer texBuffer[256];
	};

	struct Light {
		glm::vec3 direction;
		float intensity;

		glm::vec3 color;
		int padding0;
	};

	struct UniformBuffer2 {
		Light lights[4];
		glm::vec3 probePos0;
		int    lightNum;
		glm::vec3 probePos1;
		int    frameCount;
	};

	struct VertexBuffer {
		std::vector<glm::vec3> position;

		inline int GetSize() const {return position.size() * sizeof(glm::vec3);}
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

	VertexBuffer   vertexBuffer;
	IndexBuffer    indexBuffer;
	NormalBuffer   normalBuffer;
	UVBuffer       uvBuffer;
	InstanceOffset instanceOffsetBuffer;

private:
	UniformBuffer0 ubo0;
	UniformBuffer1 ubo1;
	UniformBuffer2 ubo2;
};


class ProbeBlendRadianceShader :public ComputeShaderModule {
public:
	ProbeBlendRadianceShader() :ComputeShaderModule("hlsl/ddgi/probeBlendRadiance.comp.hlsl")
	{
	}

	virtual size_t GetBufferSizeBinding(uint32_t binding) const {
		switch (binding) {
		case 0:
			return sizeof(UniformBuffer0);
		}
	}

	virtual void SetUBO(uint8_t binding, void* data) {
		switch (binding) {
		case 0:
			ubo0 = *reinterpret_cast<ProbeBlendRadianceShader::UniformBuffer0*>(data);
			return;
		}
	}

	virtual void* GetData(uint32_t binding, uint32_t index = 0) {
		switch (binding) {
		case 0:
			return (void*)&ubo0;
		}
	}

public:
	struct Probe {
		glm::vec3 position;
		int padding0;
	};

	struct UniformBuffer0 {
		Probe probes[512];
	};

private:
	UniformBuffer0 ubo0;
};

class DDGILightingShader :public ShaderModule {
public:
	DDGILightingShader() :ShaderModule("hlsl/ddgi/fullScreen.vert.hlsl", "hlsl/ddgi/ddgiLighting.frag.hlsl")
	{
	}

	virtual size_t GetBufferSizeBinding(uint32_t binding) const {
		switch (binding) {
		case 0:
			return sizeof(DDGILightingShader::UniformBuffer0);
		case 1:
			return sizeof(DDGILightingShader::UniformBuffer1);
		}
	}

	virtual void SetUBO(uint8_t binding, void* data) {
		switch (binding) {
		case 0:
			ubo0 = *reinterpret_cast<DDGILightingShader::UniformBuffer0*>(data);
			return;
		case 1:
			ubo1 = *reinterpret_cast<DDGILightingShader::UniformBuffer1*>(data);
			return;
		}
	}

	virtual void* GetData(uint32_t binding, uint32_t index = 0) {
		switch (binding) {
		case 0:
			return (void*)&ubo0;
		case 1:
			return (void*)&ubo1;
		}
	}

public:
	struct Light {
		glm::vec3 direction;
		float intensity;

		glm::vec3 color;
		int shadowMapIndex;
	};

	struct UniformBuffer0 {
		Light lights[4];

		glm::vec3 cameraPos;
		int lightNum;

		glm::vec3 probePos0;
		int    padding0;
		glm::vec3 probePos1;
		int    padding1;
	};

	struct Probe {
		glm::vec3 position;
		int    padding0;
	};

	struct UniformBuffer1
	{
		Probe	probes[512];
	};

private:
	UniformBuffer0 ubo0;
	UniformBuffer1 ubo1;
};

class CompositeShader :public ShaderModule {
public:
	CompositeShader() :ShaderModule("hlsl/ddgi/fullScreen.vert.hlsl", "hlsl/ddgi/composite.frag.hlsl")
	{
	}

public:
};

class DDGIProbeVisulizeShader :public ShaderModule {
public:
	DDGIProbeVisulizeShader() :ShaderModule("hlsl/ddgi/probeVisulize.vert.hlsl", "hlsl/ddgi/probeVisulize.frag.hlsl")
	{
	}

	virtual size_t GetBufferSizeBinding(uint32_t binding) const {
		switch (binding) {
		case 0:
			return sizeof(DDGIProbeVisulizeShader::UniformBuffer0);
		case 1:
			return sizeof(DDGIProbeVisulizeShader::UniformBuffer1);
		}
	}

	virtual void SetUBO(uint8_t binding, void* data) {
		switch (binding) {
		case 0:
			ubo0 = *reinterpret_cast<DDGIProbeVisulizeShader::UniformBuffer0*>(data);
			return;
		case 1:
			ubo1 = *reinterpret_cast<DDGIProbeVisulizeShader::UniformBuffer1*>(data);
			return;
		}
	}

	virtual void* GetData(uint32_t binding, uint32_t index = 0) {
		switch (binding) {
		case 0:
			return (void*)&ubo0;
		case 1:
			return (void*)&ubo1;
		}
	}

public:
	struct ModelBuffer {
		glm::mat4 Model;
	};

	struct VPBuffer {
		glm::mat4 View;
		glm::mat4 Projection;
	};

	struct UniformBuffer0 {
		ModelBuffer models[512];
	};

	struct UniformBuffer1 {
		VPBuffer vp;
	};

private:
	UniformBuffer0 ubo0;
	UniformBuffer1 ubo1;
};

#endif