#ifndef RTAO_HPP
#define RTAO_HPP

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

class RTAO : public MRenderApplication {
public:
	virtual void SetUp();
	virtual void ComputeAndDraw(uint32_t imageIndex);
	//virtual void UpdatePerFrame(uint32_t imageIndex);

	virtual void RecreateSwapchainAndRenderPasses();
	virtual void CreateRenderPass();

	virtual void PreComputes();
	virtual void Clean();
private:
	void loadScene();
	void createAS();
	void createLight();
	void createCamera();
	void createSamplers();

	void createTextures();

private:
	std::shared_ptr<RenderPass> m_gbufferPass;
	std::shared_ptr<RenderPass> m_rtao_lightingPass;

	std::shared_ptr<MVulkanTexture> m_acculatedAOTexture = nullptr;
	MVulkanSampler				m_linearSampler;

	std::shared_ptr<Scene>		m_scene;
	std::shared_ptr<Scene>		m_squad;

	std::shared_ptr<Light>		m_directionalLight;

	std::shared_ptr<Camera>		m_camera;
	MVulkanRaytracing			m_rayTracing;
};

class RTAOShader :public ShaderModule {
public:
	RTAOShader();

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;

	virtual void SetUBO(uint8_t index, void* data);

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

		int resetAccumulatedBuffer;
		int gbufferWidth;
		int gbufferHeight;
		float padding2;
	};

private:
	UniformBuffer0 ubo0;
};



#endif // RTAO_HPP