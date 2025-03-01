#ifndef RAYQUERYSHADOWS_HPP
#define RAYQUERYSHADOWS_HPP

#include <MRenderApplication.hpp>
#include <memory>
#include <vector>
#include "MVulkanRHI/MVulkanSampler.hpp"
#include "MVulkanRHI/MVulkanBuffer.hpp"
#include "MVulkanRHI/MVulkanRayTracing.hpp"
#include "Shaders/ShaderModule.hpp"

const uint16_t WIDTH = 1280;
const uint16_t HEIGHT = 800;

class Scene;
class RenderPass;
class ComputePass;
class Camera;
class Light;

class LightingPbrRayQueryShader :public ShaderModule {
public:
	LightingPbrRayQueryShader();

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;

	virtual void SetUBO(uint8_t binding, void* data);

	virtual void* GetData(uint32_t binding, uint32_t index = 0);

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
	};

private:
	UniformBuffer0 ubo0;
};

class RayQueryGBufferShader :public ShaderModule {
public:
	RayQueryGBufferShader();

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;

	virtual void SetUBO(uint8_t binding, void* data);

	virtual void* GetData(uint32_t binding, uint32_t index = 0);
public:
	struct TexBuffer
	{
		int diffuseTextureIdx;
		int metallicAndRoughnessTextureIdx;
		int matId;
		int padding2;
	};

	struct UniformBuffer0
	{
		TexBuffer texBuffer[512];
	};

	struct UniformBuffer1
	{
		glm::vec3 cameraPosition;
		int    padding0;

		glm::vec3 cameraDirection;
		int    padding1;

		glm::ivec2   screenResolution;
		float  fovY;
		float  zNear;
	};

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

	VertexBuffer   vertexBuffer;
	IndexBuffer    indexBuffer;
	NormalBuffer   normalBuffer;
	UVBuffer       uvBuffer;
	InstanceOffset instanceOffsetBuffer;
private:

	UniformBuffer0 ubo0;
	UniformBuffer1 ubo1;
};

class RayQueryPbr : public MRenderApplication {
public:
	virtual void SetUp();
	virtual void ComputeAndDraw(uint32_t imageIndex);
	//virtual void UpdatePerFrame(uint32_t imageIndex);

	virtual void RecreateSwapchainAndRenderPasses();
	virtual void CreateRenderPass();

	virtual void PreComputes();
	virtual void Clean();
private:
	void createGbufferPass();
	void createLightingPass();

	void loadScene();
	void createAS();
	void createLight();
	void createCamera();
	void createSamplers();

private:
	std::shared_ptr<RenderPass> m_gbufferPass;
	std::shared_ptr<RenderPass> m_lightingPass;

	MVulkanSampler				m_linearSampler;

	std::shared_ptr<Scene>		m_scene;
	std::shared_ptr<Scene>		m_squad;

	std::shared_ptr<Light>		m_directionalLight;

	std::shared_ptr<Camera>		m_camera;
	MVulkanRaytracing			m_rayTracing;
};

#endif // RAYQUERYTEST_HPP