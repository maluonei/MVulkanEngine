#ifndef DDGI_SHADER_HPP
#define DDGI_SHADER_HPP

#include "Shaders/ShaderModule.hpp"
#include "Scene/ddgi.hpp"


struct UniformBuffer1
{
	//Probe		probes[2040];
	glm::ivec3	probeDim;
	int			raysPerProbe;

	glm::vec3	probePos0;
	float		minFrontFaceDistance;
	glm::vec3	probePos1;
	//int		  farthestFrontfaceDistance;
	
	int			probeRelocationEnbled = true;
	//int			padding0;
	//int			padding1;
	//int			padding2;
};

struct LightStruct {
	glm::mat4 shadowViewProjMat;

	glm::vec3 direction;
	float intensity;

	glm::vec3 color;
	int padding0;
};


class RTAOShader :public ShaderModule {
public:
	RTAOShader() :ShaderModule(
		"hlsl/ddgi/fullscreen.vert.hlsl", 
		"hlsl/ddgi/rtao.frag.hlsl")
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
	struct UniformBuffer0 {
		int resetAccumulatedBuffer;
		int gbufferWidth;
		int gbufferHeight;
		int padding0;
	};

private:
	UniformBuffer0 ubo0;
};

class ProbeTracingShader :public ShaderModule {
public:
	ProbeTracingShader() :ShaderModule(
		"hlsl/ddgi/fullScreen.vert.hlsl", 
		"hlsl/ddgi/probeTrace.frag.hlsl", 
		"main",
		"main",
		m_compileEveryTime = false)
	{}

	virtual size_t GetBufferSizeBinding(uint32_t binding) const {
		switch (binding) {
		case 0:
			return sizeof(UniformBuffer0);
		case 1:
			return sizeof(UniformBuffer1);
		case 2:
			return sizeof(UniformBuffer2);
		}
	}

	virtual void SetUBO(uint8_t binding, void* data) {
		switch (binding) {
		case 0:
			ubo0 = *reinterpret_cast<ProbeTracingShader::UniformBuffer0*>(data);
			return;
		case 1:
			ubo1 = *reinterpret_cast<UniformBuffer1*>(data);
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
		}
	}

public:
	struct TexBuffer {
		int diffuseTextureIdx;
		int metallicAndRoughnessTextureIdx;
		int matId;
		int normalMapIdx;
	};

	struct UniformBuffer0 {
		TexBuffer texBuffer[256];
	};

	struct UniformBuffer2 {
		LightStruct lights[4];

		int    lightNum;
		int    frameCount;
		int		padding1;
		float	t;

		glm::vec4 probeRotateQuaternion;
	};
	//ProbeBuffer	   probeBuffer;

private:
	UniformBuffer0 ubo0;
	UniformBuffer1 ubo1;
	UniformBuffer2 ubo2;
};


class ProbeBlendShader :public ComputeShaderModule {
public:
	ProbeBlendShader(std::string entry) :ComputeShaderModule(
		"hlsl/ddgi/probeBlend.comp.hlsl", 
		entry,
		false)
	{
	}

	virtual size_t GetBufferSizeBinding(uint32_t binding) const {
		switch (binding) {
		case 0:
			return sizeof(UniformBuffer0);
		case 1:
			return sizeof(UniformBuffer1);
		//case 2:
		//	return probeBuffer.GetSize();
		}
	}

	virtual void SetUBO(uint8_t binding, void* data) {
		switch (binding) {
		case 0:
			ubo0 = *reinterpret_cast<ProbeBlendShader::UniformBuffer0*>(data);
			return;
		case 1:
			ubo1 = *reinterpret_cast<UniformBuffer1*>(data);
			return;
		}
	}

	virtual void* GetData(uint32_t binding, uint32_t index = 0) {
		switch (binding) {
		case 0:
			return (void*)&ubo0;
		case 1:
			return (void*)&ubo1;
		//case 2:
		//	return (void*)probeBuffer.probes.data();
		}
	}

public:
	struct UniformBuffer0 {
		float sharpness;
		int padding0;
		int padding1;
		int padding2;
	};

	//ProbeBuffer probeBuffer;

private:
	UniformBuffer0 ubo0;
	UniformBuffer1 ubo1;
};

class DDGILightingShader :public ShaderModule {
public:
	DDGILightingShader() :ShaderModule(
		"hlsl/ddgi/fullScreen.vert.hlsl", 
		"hlsl/ddgi/ddgiLighting.frag.hlsl",
		"main",
		"main", 
		m_compileEveryTime = false)
	{
	}

	virtual size_t GetBufferSizeBinding(uint32_t binding) const {
		switch (binding) {
		case 0:
			return sizeof(DDGILightingShader::UniformBuffer0);
		case 1:
			return sizeof(UniformBuffer1);
		//case 2:
		//	return sizeof(probeBuffer.GetSize());
		}
	}

	virtual void SetUBO(uint8_t binding, void* data) {
		switch (binding) {
		case 0:
			ubo0 = *reinterpret_cast<DDGILightingShader::UniformBuffer0*>(data);
			return;
		case 1:
			ubo1 = *reinterpret_cast<UniformBuffer1*>(data);
			return;
		}
	}

	virtual void* GetData(uint32_t binding, uint32_t index = 0) {
		switch (binding) {
		case 0:
			return (void*)&ubo0;
		case 1:
			return (void*)&ubo1;
		//case 2:
		//	return (void*)probeBuffer.probes.data();
		}
	}

public:
	struct UniformBuffer0 {
		LightStruct lights[4];

		glm::vec3 cameraPos;
		int lightNum;
	};

	//ProbeBuffer	   probeBuffer;

private:
	UniformBuffer0 ubo0;
	UniformBuffer1 ubo1;
};

class CompositeShader :public ShaderModule {
public:
	CompositeShader() :ShaderModule(
		"hlsl/ddgi/fullScreen.vert.hlsl", 
		"hlsl/ddgi/composite.frag.hlsl")
	{
	}

	virtual size_t GetBufferSizeBinding(uint32_t binding) const {
		switch (binding) {
		case 0:
			return sizeof(CompositeShader::UniformBuffer0);
		}
	}

	virtual void SetUBO(uint8_t binding, void* data) {
		switch (binding) {
		case 0:
			ubo0 = *reinterpret_cast<CompositeShader::UniformBuffer0*>(data);
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
	struct UniformBuffer0 {
		int visulizeProbe = 0;
		int useAO = 1;
		int padding1;
		int padding2;
	};

private:

	UniformBuffer0 ubo0;

};

class DDGIProbeVisulizeShader :public ShaderModule {
public:
	DDGIProbeVisulizeShader() :ShaderModule(
		"hlsl/ddgi/probeVisulize.vert.hlsl", 
		"hlsl/ddgi/probeVisulize.frag.hlsl")
	{
	}

	virtual size_t GetBufferSizeBinding(uint32_t binding) const {
		switch (binding) {
		case 0:
			return sizeof(DDGIProbeVisulizeShader::UniformBuffer0);
		case 1:
			return sizeof(UniformBuffer1);
		//case 2:
		//	return modelBuffer.GetSize();
		//case 3:
		//	return probeBuffer.GetSize();
		}
	}

	virtual void SetUBO(uint8_t binding, void* data) {
		switch (binding) {
		case 0:
			ubo0 = *reinterpret_cast<DDGIProbeVisulizeShader::UniformBuffer0*>(data);
			return;
		case 1:
			ubo1 = *reinterpret_cast<UniformBuffer1*>(data);
			return;
		}
	}

	virtual void* GetData(uint32_t binding, uint32_t index = 0) {
		switch (binding) {
		case 0:
			return (void*)&ubo0;
		case 1:
			return (void*)&ubo1;
		//case 2:
		//	return (void*)modelBuffer.models.data();
		//case 3:
		//	return (void*)probeBuffer.probes.data();
		}
	}

public:
	struct VPBuffer {
		glm::mat4 View;
		glm::mat4 Projection;
	};

	struct UniformBuffer0 {
		VPBuffer vp;
	};

public:
	//ProbeBuffer		probeBuffer;
	//ModelBuffer		modelBuffer;

private:
	UniformBuffer0 ubo0;
	UniformBuffer1 ubo1;
};

class ProbeClassficationShader :public ComputeShaderModule {
public:
	ProbeClassficationShader() :ComputeShaderModule(
		"hlsl/ddgi/probeClassfication.comp.hlsl", 
		"main",
		false)
	{
	}

	virtual size_t GetBufferSizeBinding(uint32_t binding) const {
		switch (binding) {
		case 0:
			return sizeof(UniformBuffer0);
		case 1:
			return sizeof(UniformBuffer1);
		//case 2:
		//	return probeBuffer.GetSize();
		}
	}

	virtual void SetUBO(uint8_t binding, void* data) {
		switch (binding) {
		case 0:
			ubo0 = *reinterpret_cast<ProbeClassficationShader::UniformBuffer0*>(data);
			return;
		case 1:
			ubo1 = *reinterpret_cast<UniformBuffer1*>(data);
			return;
		}
	}

	virtual void* GetData(uint32_t binding, uint32_t index = 0) {
		switch (binding) {
		case 0:
			return (void*)&ubo0;
		case 1:
			return (void*)&ubo1;
		//case 2:
		//	return (void*)probeBuffer.probes.data();
		}
	}

public:
	struct UniformBuffer0 {
		float maxRayDistance;
		int padding0;
		int padding1;
		int padding2;
	};

	//ProbeBuffer probeBuffer;

private:
	UniformBuffer0 ubo0;
	UniformBuffer1 ubo1;
};

class ProbeRelocationShader :public ComputeShaderModule {
public:
	ProbeRelocationShader() :ComputeShaderModule(
		"hlsl/ddgi/probeRelocation.comp.hlsl",
		"main",
		false)
	{
	}

	virtual size_t GetBufferSizeBinding(uint32_t binding) const {
		switch (binding) {
		case 0:
			return sizeof(UniformBuffer1);
		}
	}

	virtual void SetUBO(uint8_t binding, void* data) {
		switch (binding) {
		case 0:
			ubo0 = *reinterpret_cast<UniformBuffer1*>(data);
			return;

		}
	}

	virtual void* GetData(uint32_t binding, uint32_t index = 0) {
		switch (binding) {
		case 0:
			return (void*)&ubo0;
			//case 2:
			//	return (void*)probeBuffer.probes.data();
		}
	}

private:
	UniformBuffer1 ubo0;
};



#endif