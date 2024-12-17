#ifndef MVULKANENGINE_PBR_HPP
#define MVULKANENGINE_PBR_HPP

#include <MRenderApplication.hpp>
#include <memory>
#include <vector>
#include "MVulkanRHI/MVulkanSampler.hpp"
#include "MVulkanRHI/MVulkanBuffer.hpp"

const uint16_t WIDTH = 1280;
const uint16_t HEIGHT = 800;

class Scene;
class RenderPass;
class Camera;
class Light;

class PBR: public MRenderApplication {
public:
	virtual void SetUp();
	virtual void UpdatePerFrame(uint32_t imageIndex);

	virtual void RecreateSwapchainAndRenderPasses();
	virtual void CreateRenderPass();

	virtual void PreComputes();
private:
	void preComputeIrradianceCubemap();
	void preFilterEnvmaps();
	void preComputeLUT();

	void createSkyboxTexture();
	void createIrradianceCubemapTexture();

	void loadScene();
	void createLight();
	void createCamera();
	void createSamplers();

	void createLightCamera();

private:
	std::shared_ptr<RenderPass> irradianceConvolutionPass;
	std::vector<std::shared_ptr<RenderPass>> prefilterEnvmapPasses;
	std::shared_ptr<RenderPass> brdfLUTPass;

	std::shared_ptr<RenderPass> gbufferPass;
	std::shared_ptr<RenderPass> shadowPass;
	std::shared_ptr<RenderPass> lightingPass;
	std::shared_ptr<RenderPass> skyboxPass;

	MVulkanSampler linearSampler;
	MVulkanSampler nearestSampler;

	MVulkanTexture skyboxTexture;
	MVulkanTexture irradianceTexture;
	MVulkanTexture preFilteredEnvTexture;

	std::shared_ptr<Scene> scene;
	std::shared_ptr<Scene> cube;
	std::shared_ptr<Scene> sphere;
	std::shared_ptr<Scene> squad;

	std::shared_ptr<Light> directionalLight;

	std::shared_ptr<Camera> directionalLightCamera;
	std::shared_ptr<Camera> camera;
};




#endif

