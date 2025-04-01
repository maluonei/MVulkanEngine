#ifndef MVULKANENGINE_MESHSHADER_HPP
#define MVULKANENGINE_MESHSHADER_HPP

#include "ShaderModule.hpp"
#include "Camera.hpp"

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

class MeshletShader2 : public MeshShaderModule {
public:
	MeshletShader2();

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;
	virtual void SetUBO(uint8_t binding, void* data);
	virtual void* GetData(uint32_t binding, uint32_t index = 0);

	struct CameraProperties {
		glm::mat4 Model;
		glm::mat4 View;
		glm::mat4 Projection;
		glm::mat4 MVP;
	};

	struct SceneProperties {
		uint32_t     InstanceCount;
		uint32_t     MeshletCount;
	};

private:
	SceneProperties ubo0;
	CameraProperties ubo1;
};


class VBufferShader : public MeshShaderModule {
public:
	VBufferShader();

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;
	virtual void SetUBO(uint8_t binding, void* data);
	virtual void* GetData(uint32_t binding, uint32_t index = 0);

	struct CameraProperties {
		glm::mat4 Model;
		glm::mat4 View;
		glm::mat4 Projection;
		glm::mat4 MVP;
	};

	struct SceneProperties {
		//uint32_t     InstanceCount;
		uint32_t     MeshletCount;
	};

private:
	SceneProperties ubo0;
	CameraProperties ubo1;
};

class VBufferCullingShader : public MeshShaderModule {
public:
	VBufferCullingShader();

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;
	virtual void SetUBO(uint8_t binding, void* data);
	virtual void* GetData(uint32_t binding, uint32_t index = 0);

	struct CameraProperties {
		//glm::mat4 Model;
		glm::mat4 View;
		glm::mat4 Projection;
		//glm::mat4 MVP;
	};

	//struct SceneProperties {
	//	//uint32_t     InstanceCount;
	//	uint32_t     MeshletCount;
	//};


	struct FrustumPlane
	{
		glm::vec3  Normal;
		float __pad0;
		glm::vec3  Position;
		float __pad1;
	};

	struct FrustumData
	{
		FrustumPlane Planes[6];
		glm::vec4    Sphere;
		FrustumCone  Cone;
	};

	struct SceneProperties {
		//uint     InstanceCount;
		VBufferCullingShader::FrustumData	Frustum;
		uint32_t			MeshletCount;
		uint32_t			VisibilityFunc = 4;
	};

private:
	SceneProperties ubo0;
	CameraProperties ubo1;
};

class VBufferShadingShader :public ShaderModule {
public:
	VBufferShadingShader();

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;
	virtual void SetUBO(uint8_t binding, void* data);
	virtual void* GetData(uint32_t binding, uint32_t index = 0);

public:
	struct Light
	{
		glm::mat4 shadowViewProj;

		glm::vec3 direction;
		float intensity;

		glm::vec3 color;
		int shadowMapIndex;

		float cameraZnear;
		float cameraZfar;
		float padding6;
		float padding7;
	};

	struct UnifomBuffer0
	{
		Light lights[2];

		glm::vec3 cameraPos;
		int lightNum;

		int ShadowmapResWidth;
		int ShadowmapResHeight;
		int WindowResWidth;
		int WindowResHeight;
	};

	struct TexBuffer
	{
		int diffuseTextureIdx;
		int metallicAndRoughnessTextureIdx;
		int matId;
		int normalTextureIdx;

		glm::vec3 diffuseColor;
		int padding0;
	};

	struct UnifomBuffer1
	{
		TexBuffer tex[512];
	};


private:
	UnifomBuffer0 ubo0;
	UnifomBuffer1 ubo1;
};

#endif