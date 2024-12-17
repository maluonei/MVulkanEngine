#ifndef MVULKANENGINE_IBL_HPP
#define MVULKANENGINE_IBL_HPP

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

class IBL : public MRenderApplication {
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
	void createCamera();
	void createSamplers();
private:
	std::shared_ptr<RenderPass> irradianceConvolutionPass;
	std::vector<std::shared_ptr<RenderPass>> prefilterEnvmapPasses;
	std::shared_ptr<RenderPass> brdfLUTPass;

	std::shared_ptr<RenderPass> gbufferPass;
	std::shared_ptr<RenderPass> lightingPass;
	std::shared_ptr<RenderPass> skyboxPass;

	MVulkanSampler linearSampler;

	MVulkanTexture skyboxTexture;
	MVulkanTexture irradianceTexture;
	MVulkanTexture preFilteredEnvTexture;

	std::shared_ptr<Scene> cube;
	std::shared_ptr<Scene> sphere;
	std::shared_ptr<Scene> squad;

	std::shared_ptr<Camera> camera;
};

#endif