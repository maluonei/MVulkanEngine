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
	virtual void Clean();
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
	std::shared_ptr<RenderPass>					m_irradianceConvolutionPass;
	std::vector<std::shared_ptr<RenderPass>>	m_prefilterEnvmapPasses;
	std::shared_ptr<RenderPass>					m_brdfLUTPass;

	std::shared_ptr<RenderPass> m_gbufferPass;
	std::shared_ptr<RenderPass> m_lightingPass;
	std::shared_ptr<RenderPass> m_skyboxPass;

	MVulkanSampler				m_linearSampler;

	MVulkanTexture				m_skyboxTexture;
	MVulkanTexture				m_irradianceTexture;
	MVulkanTexture				m_preFilteredEnvTexture;

	std::shared_ptr<Scene>		m_cube;
	std::shared_ptr<Scene>		m_sphere;
	std::shared_ptr<Scene>		m_squad;

	std::shared_ptr<Camera>		m_camera;
};

#endif