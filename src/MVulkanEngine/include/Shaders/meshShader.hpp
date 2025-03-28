#ifndef MVULKANENGINE_MESHSHADER_HPP
#define MVULKANENGINE_MESHSHADER_HPP

#include "ShaderModule.hpp"

class TestMeshShader : public MeshShaderModule {
public:
	TestMeshShader();
};

class TestMeshShader2 : public MeshShaderModule {
public:
	TestMeshShader2();

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;
	virtual void SetUBO(uint8_t binding, void* data);
	virtual void* GetData(uint32_t binding, uint32_t index = 0);

	struct CameraProperties {
		glm::mat4 Model;
		glm::mat4 View;
		glm::mat4 Projection;
		glm::mat4 MVP;
	};

private:

	CameraProperties ubo0;
};

class TestMeshShader3 : public MeshShaderModule {
public:
	TestMeshShader3();

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;
	virtual void SetUBO(uint8_t binding, void* data);
	virtual void* GetData(uint32_t binding, uint32_t index = 0);

	struct CameraProperties {
		glm::mat4 Model;
		glm::mat4 View;
		glm::mat4 Projection;
		glm::mat4 MVP;

		int numVertices;
		int numIndices;
	};

private:

	CameraProperties ubo0;
};

class MeshletShader : public MeshShaderModule {
public:
	MeshletShader();

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;
	virtual void SetUBO(uint8_t binding, void* data);
	virtual void* GetData(uint32_t binding, uint32_t index = 0);

	struct CameraProperties {
		glm::mat4 Model;
		glm::mat4 View;
		glm::mat4 Projection;
		glm::mat4 MVP;
	};

private:

	CameraProperties ubo0;
};

#endif