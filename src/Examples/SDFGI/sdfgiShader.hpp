#ifndef SDFGI_SHADER_HPP
#define SDFGI_SHADER_HPP

#include "Shaders/ddgiShader.hpp"

class SDFProbeTracingShader :public ShaderModule {
public:
	SDFProbeTracingShader() :ShaderModule(
		"hlsl/ddgi/fullScreen.vert.hlsl",
		"hlsl/sdfgi/probeTrace.frag.hlsl",
		"main",
		"main",
		m_compileEveryTime = false)
	{
	}

	virtual size_t GetBufferSizeBinding(uint32_t binding) const {
		switch (binding) {
		case 0:
			return sizeof(UniformBuffer1);
		case 1:
			return sizeof(UniformBuffer2);
		case 2:
			return sizeof(SDFBuffer);
		}
	}

	virtual void SetUBO(uint8_t binding, void* data) {
		switch (binding) {
		case 0:
			ubo1 = *reinterpret_cast<UniformBuffer1*>(data);
			return;
		case 1:
			ubo2 = *reinterpret_cast<SDFProbeTracingShader::UniformBuffer2*>(data);
			return;
		case 2:
			ubo3 = *reinterpret_cast<SDFProbeTracingShader::SDFBuffer*>(data);
			return;
		}
	}

	virtual void* GetData(uint32_t binding, uint32_t index = 0) {
		switch (binding) {
		case 0:
			return (void*)&ubo1;
		case 1:
			return (void*)&ubo2;
		case 2:
			return (void*)&ubo3;
		}
	}

public:
	struct UniformBuffer2 {
		LightStruct lights[4];

		int    lightNum;
		int    frameCount;
		int		padding1;
		float	t;

		glm::vec3 cameraPos;
		float padding2;

		glm::vec4 probeRotateQuaternion;
	};
	//ProbeBuffer	   probeBuffer;

	struct SDFBuffer {
		glm::vec3 aabbMin;
		uint32_t maxRaymarchSteps;
		glm::vec3 aabbMax;
		float padding1;
		glm::ivec3 sdfResolution;
		float padding2;
	};

private:
	UniformBuffer1 ubo1;
	UniformBuffer2 ubo2;
	SDFBuffer	   ubo3;
};

//class SDFProbeTracingShader2 :public ShaderModule {
//public:
//	SDFProbeTracingShader2() :ShaderModule(
//		"hlsl/ddgi/fullScreen.vert.hlsl",
//		"hlsl/sdfgi/probeTrace2.frag.hlsl",
//		"main",
//		"main",
//		m_compileEveryTime = false)
//	{
//	}
//
//	virtual size_t GetBufferSizeBinding(uint32_t binding) const {
//		switch (binding) {
//		case 0:
//			return sizeof(UniformBuffer1);
//		case 1:
//			return sizeof(UniformBuffer2);
//		case 2:
//			return sizeof(SDFBuffer);
//		}
//	}
//
//	virtual void SetUBO(uint8_t binding, void* data) {
//		switch (binding) {
//		case 0:
//			ubo1 = *reinterpret_cast<UniformBuffer1*>(data);
//			return;
//		case 1:
//			ubo2 = *reinterpret_cast<SDFProbeTracingShader2::UniformBuffer2*>(data);
//			return;
//		case 2:
//			ubo3 = *reinterpret_cast<SDFProbeTracingShader2::SDFBuffer*>(data);
//			return;
//		}
//	}
//
//	virtual void* GetData(uint32_t binding, uint32_t index = 0) {
//		switch (binding) {
//		case 0:
//			return (void*)&ubo1;
//		case 1:
//			return (void*)&ubo2;
//		case 2:
//			return (void*)&ubo3;
//		}
//	}
//
//public:
//	struct UniformBuffer2 {
//		LightStruct lights[4];
//
//		int    lightNum;
//		int    frameCount;
//		int		padding1;
//		float	t;
//
//		glm::vec3 cameraPos;
//		float padding2;
//
//		glm::vec4 probeRotateQuaternion;
//	};
//	//ProbeBuffer	   probeBuffer;
//
//	struct SDFBuffer {
//		glm::vec3 aabbMin;
//		uint32_t maxRaymarchSteps;
//		glm::vec3 aabbMax;
//		float padding1;
//		glm::ivec3 sdfResolution;
//		float padding2;
//	};
//
//private:
//	UniformBuffer1 ubo1;
//	UniformBuffer2 ubo2;
//	SDFBuffer	   ubo3;
//};

class SDFGILightingShader :public ShaderModule {
public:
	SDFGILightingShader() :ShaderModule(
		"hlsl/ddgi/fullScreen.vert.hlsl",
		"hlsl/sdfgi/sdfgiLighting.frag.hlsl",
		"main",
		"main",
		m_compileEveryTime = false)
	{
	}

	virtual size_t GetBufferSizeBinding(uint32_t binding) const {
		switch (binding) {
		case 0:
			return sizeof(SDFGILightingShader::UniformBuffer0);
		case 1:
			return sizeof(UniformBuffer1);
			//case 2:
			//	return sizeof(probeBuffer.GetSize());
		}
	}

	virtual void SetUBO(uint8_t binding, void* data) {
		switch (binding) {
		case 0:
			ubo0 = *reinterpret_cast<SDFGILightingShader::UniformBuffer0*>(data);
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

class SDFGICompositeShader :public ShaderModule {
public:
	SDFGICompositeShader() :ShaderModule(
		"hlsl/ddgi/fullScreen.vert.hlsl",
		"hlsl/sdfgi/composite.frag.hlsl")
	{
	}

	virtual size_t GetBufferSizeBinding(uint32_t binding) const {
		switch (binding) {
		case 0:
			return sizeof(SDFGICompositeShader::UniformBuffer0);
		}
	}

	virtual void SetUBO(uint8_t binding, void* data) {
		switch (binding) {
		case 0:
			ubo0 = *reinterpret_cast<SDFGICompositeShader::UniformBuffer0*>(data);
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
		int padding0;
		int padding1;
		int padding2;
	};

private:

	UniformBuffer0 ubo0;

};

#endif