#ifndef MVULKANENGINE_PBR_HPP
#define MVULKANENGINE_PBR_HPP

#include "MRenderApplication.hpp"
#include "UIRenderer.hpp"
#include <memory>
#include <vector>
#include "MVulkanRHI/MVulkanSampler.hpp"
#include "MVulkanRHI/MVulkanBuffer.hpp"
#include "Hiz.hpp"

const uint16_t WIDTH = 1280;
const uint16_t HEIGHT = 800;

class Scene;
class RenderPass;
class ComputePass;
class DynamicRenderPass;
class Camera;
class Light;

class PBR: public MRenderApplication {
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
	void createLight();
	void createCamera();
	void createSamplers();

	void createLightCamera();

	void createTextures();
	void createGbufferPass();
	void createShadowPass();
	void createShadingPass();
	void createFrustumCullingPass();
	void createGenHizPass();

	void loadShaders();
	void createStorageBuffers();

	//void 

	//void ImageLayoutToShaderRead(int currentFrame);
	//void ImageLayoutToAttachment(int imageIndex, int currentFrame);
	//void ImageLayoutToPresent(int imageIndex, int currentFrame);
protected:
	virtual void initUIRenderer();

private:
	//std::shared_ptr<RenderPass> m_gbufferPass;
	//std::shared_ptr<RenderPass> m_shadowPass;
	//std::shared_ptr<RenderPass> m_lightingPass;
	std::shared_ptr<RenderPass> m_gbufferPass;
	std::shared_ptr<RenderPass> m_shadowPass;
	std::shared_ptr<RenderPass> m_lightingPass;
	std::shared_ptr<ComputePass> m_frustumCullingPass;
	std::shared_ptr<ComputePass> m_hizPass;

	std::vector<std::shared_ptr<MVulkanTexture>> swapchainDepthViews;
	std::shared_ptr<MVulkanTexture> shadowMap;
	std::shared_ptr<MVulkanTexture> shadowMapDepth;
	std::shared_ptr<MVulkanTexture> gBuffer0;
	std::shared_ptr<MVulkanTexture> gBuffer1;
	std::shared_ptr<MVulkanTexture> gBufferDepth;

	std::vector<std::shared_ptr<MVulkanTexture>> m_hizTextures;
	std::shared_ptr<StorageBuffer> m_instanceBoundsBuffer;
	std::shared_ptr<StorageBuffer> m_indirectDrawBuffer;
	std::shared_ptr<StorageBuffer> m_culledIndirectDrawBuffer;

	VkExtent2D shadowmapExtent = { 4096, 4096 };

	Hiz							m_hiz;
	MVulkanSampler				m_linearSampler;

	std::shared_ptr<Scene>		m_scene;
	std::shared_ptr<Scene>		m_squad;

	std::shared_ptr<Light>		m_directionalLight;

	std::shared_ptr<Camera>		m_directionalLightCamera;
	std::shared_ptr<Camera>		m_camera;
};

class DRUI :public UIRenderer {
public:
	virtual void  RenderContext();

private:
	bool shouleRenderUI = true;
	int a;
public:
	int cullingMode = 0;
};


#endif

