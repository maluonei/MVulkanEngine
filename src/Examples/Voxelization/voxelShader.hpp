#ifndef VOXEL_SHADER_HPP
#define VOXEL_SHADER_HPP

#include "Shaders/ShaderModule.hpp"

struct DrawCommand {
	uint32_t    indexCount;
	uint32_t    instanceCount;
	uint32_t    firstIndex;
	int			vertexOffset;
	uint32_t    firstInstance;
	float		paddings[3];
};

struct ModelBuffer
{
	glm::mat4 Model;
};

class VoxelShader : public ShaderModule {
public:
	VoxelShader() :ShaderModule(
		"hlsl/vx/voxel.vert.hlsl",
		"hlsl/vx/voxel.geom.hlsl",
		"hlsl/vx/voxel.frag.hlsl",
		"main", "main", "main")
	{
	}

	virtual size_t GetBufferSizeBinding(uint32_t binding) const {
		switch (binding) {
		case 0:
			return sizeof(VoxelShader::UniformBuffer0);
		case 1:
			return sizeof(VoxelShader::UniformBuffer1);
		case 2:
			return sizeof(VoxelShader::UniformBufferObject2);
		}
	}

	virtual void SetUBO(uint8_t binding, void* data) {
		switch (binding) {
		case 0:
			ubo0 = *reinterpret_cast<VoxelShader::UniformBuffer0*>(data);
			return;
		case 1:
			ubo1 = *reinterpret_cast<VoxelShader::UniformBuffer1*>(data);
			return;
		case 2:
			ubo2 = *reinterpret_cast<VoxelShader::UniformBufferObject2*>(data);
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
	struct UniformBuffer0
	{
		glm::mat4 Model;
	};

	struct UniformBuffer1
	{
		glm::mat4 Projection;
		glm::ivec3 volumeResolution;
		int padding2;
	};

	struct UniformBuffer2 {
		int diffuseTextureIdx;
		int metallicAndRoughnessTexIdx;
		int matId;
		int normalMapIdx;

		glm::vec3 diffuseColor;
		int padding0;
	};

	struct UniformBufferObject2
	{
		UniformBuffer2 texBuffer[512];
	};

private:
	UniformBuffer0 ubo0;
	UniformBuffer1 ubo1;
	UniformBufferObject2 ubo2;
};

class VoxelVisulizeShader : public ShaderModule {
public:
	VoxelVisulizeShader() :ShaderModule(
		"hlsl/vx/voxelVisulize.vert.hlsl",
		"hlsl/vx/voxelVisulize.frag.hlsl",
		"main", "main")
	{
	}

};

class VoxeVisulizeTestShader : public ShaderModule {
public:
	VoxeVisulizeTestShader() :ShaderModule(
		"hlsl/vx/voxelVisulize.vert.hlsl",
		"hlsl/vx/voxelVisulize.frag.hlsl",
		"main", "main")
	{
	}

	virtual size_t GetBufferSizeBinding(uint32_t binding) const {
		switch (binding) {
		case 0:
			return sizeof(VoxeVisulizeTestShader::UniformBuffer0);
		}
	}

	virtual void SetUBO(uint8_t binding, void* data) {
		switch (binding) {
		case 0:
			ubo0 = *reinterpret_cast<VoxeVisulizeTestShader::UniformBuffer0*>(data);
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
	struct UniformBuffer0
	{
		glm::mat4 Model;
		glm::mat4 View;
		glm::mat4 Projection;
	};

private:
	UniformBuffer0 ubo0;
};

class VoxeVisulizeShader : public ShaderModule {
public:
	VoxeVisulizeShader() :ShaderModule(
		"hlsl/vx/voxelVisulize.vert.hlsl",
		"hlsl/vx/voxelVisulize.frag.hlsl",
		"main", "main")
	{
	}

	virtual size_t GetBufferSizeBinding(uint32_t binding) const {
		switch (binding) {
		case 0:
			return sizeof(VoxeVisulizeShader::UniformBuffer0);
		}
	}

	virtual void SetUBO(uint8_t binding, void* data) {
		switch (binding) {
		case 0:
			ubo0 = *reinterpret_cast<VoxeVisulizeShader::UniformBuffer0*>(data);
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
	struct VPBuffer {
		glm::mat4 View;
		glm::mat4 Projection;
	};

	struct UniformBuffer0
	{
		VPBuffer vp;
	};


private:
	UniformBuffer0 ubo0;
};

class GenIndirectDrawShader :public ComputeShaderModule {
public:
	GenIndirectDrawShader(std::string entry) :ComputeShaderModule(
		"hlsl/vx/genVoxelVisulizeIndirectBuffer.comp.hlsl",
		entry,
		false) 
	{
	}

	virtual size_t GetBufferSizeBinding(uint32_t binding) const {
		switch (binding) {
		case 0:
			return sizeof(UniformBuffer0);
			//case 2:
			//	return probeBuffer.GetSize();
		}
	}

	virtual void SetUBO(uint8_t binding, void* data) {
		switch (binding) {
		case 0:
			ubo0 = *reinterpret_cast<GenIndirectDrawShader::UniformBuffer0*>(data);
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

public:
	struct UniformBuffer0
	{
		glm::vec3 aabbMin;
		float padding0;
		glm::vec3 aabbMax;
		float padding1;
		glm::ivec3 voxelResolusion;
		float padding2;
	};

	//ProbeBuffer probeBuffer;

private:
	UniformBuffer0 ubo0;
};

class JFAShader :public ComputeShaderModule {
public:
	JFAShader(std::string entry) :ComputeShaderModule(
		"hlsl/vx/JFA3D.comp.hlsl",
		entry,
		false)
	{
	}

	virtual size_t GetBufferSizeBinding(uint32_t binding) const {
		switch (binding) {
		case 0:
			return sizeof(UniformBuffer0);
			//case 2:
			//	return probeBuffer.GetSize();
		}
	}

	virtual void SetUBO(uint8_t binding, void* data) {
		switch (binding) {
		case 0:
			ubo0 = *reinterpret_cast<JFAShader::UniformBuffer0*>(data);
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

public:
	struct UniformBuffer0
	{
		glm::vec3 aabbMin;
		float padding0;
		glm::vec3 aabbMax;
		float padding1;
		glm::ivec3 voxelResolusion;
		uint32_t stepSize;
	};

	//ProbeBuffer probeBuffer;

private:
	UniformBuffer0 ubo0;
};

class ClearTexturesShader :public ComputeShaderModule {
public:
	ClearTexturesShader(std::string entry) :ComputeShaderModule(
		"hlsl/vx/clearTextures.comp.hlsl",
		entry,
		false)
	{}
};

class SDFTraceShader : public ShaderModule {
public:
	SDFTraceShader() :ShaderModule(
		"hlsl/lighting_pbr.vert.hlsl",
		"hlsl/vx/rayMarchingDebug2.frag.hlsl",
		"main", "main")
	{}

	virtual size_t GetBufferSizeBinding(uint32_t binding) const {
		switch (binding) {
		case 0:
			return sizeof(SDFTraceShader::UniformBuffer0);
		case 1:
			return sizeof(SDFTraceShader::UniformBuffer1);
		case 2:
			return sizeof(SDFTraceShader::UniformBuffer2);
		}
	}

	virtual void SetUBO(uint8_t binding, void* data) {
		switch (binding) {
		case 0:
			ubo0 = *reinterpret_cast<SDFTraceShader::UniformBuffer0*>(data);
			return;
		case 1:
			ubo1 = *reinterpret_cast<SDFTraceShader::UniformBuffer1*>(data);
			return;
		case 2:
			ubo2 = *reinterpret_cast<SDFTraceShader::UniformBuffer2*>(data);
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
	struct UniformBuffer0
	{
		glm::vec3 aabbMin;
		float padding0;
		glm::vec3 aabbMax;
		float padding1;
		glm::ivec3 voxelResolusion;
		float padding3;
	};

	struct UniformBuffer1
	{
		glm::vec3 cameraPos;
		float padding;

		glm::vec3 cameraDir;
		uint32_t maxRaymarchSteps;

		glm::ivec2 ScreenResolution;
		float fovY;
		float zNear;
	};

	struct UniformBuffer2
	{
		glm::vec3 lightDir;
		float padding2;

		glm::vec3 lightColor;
		float lightIntensity;
	};


private:
	SDFTraceShader::UniformBuffer0 ubo0;
	SDFTraceShader::UniformBuffer1 ubo1;
	SDFTraceShader::UniformBuffer2 ubo2;
};

#endif