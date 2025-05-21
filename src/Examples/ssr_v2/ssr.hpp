#ifndef SSR_HPP
#define SSR_HPP

#include "MRenderApplication.hpp"
#include "UIRenderer.hpp"
#include <memory>
#include <vector>
#include "MVulkanRHI/MVulkanSampler.hpp"
#include "MVulkanRHI/MVulkanBuffer.hpp"
#include "MVulkanRHI/MVulkanSyncObject.hpp"
#include "Hiz.hpp"

const uint16_t WIDTH = 1280;
const uint16_t HEIGHT = 800;

class Scene;
class RenderPass;
class ComputePass;
class DynamicRenderPass;
class Camera;
class Light;

#define NOT_DO_HIZ 0
#define DO_HIZ_MODE_0 1

class SSR : public MRenderApplication {
public:
	virtual void SetUp();
	virtual void ComputeAndDraw(uint32_t imageIndex);
	//virtual void UpdatePerFrame(uint32_t imageIndex);

	virtual void RecreateSwapchainAndRenderPasses();
	virtual void CreateRenderPass();

	virtual void PreComputes();
	virtual void Clean();

	inline std::shared_ptr<Camera> GetMainCamera() {
		return m_camera;
	}

	inline std::shared_ptr<Camera> GetLightCamera() {
		return m_directionalLightCamera;
	}


	int numDrawInstances = 0;
	int numTotalInstances = 0;

	float gbufferTime = 0.f;
	float shadowmapTime = 0.f;
	float shadingTime = 0.f;
	float cullingTime = 0.f;
	float compositeTime = 0.f;
	float hizTime = 0.f;
	float ssrTime = 0.f;
	//std::vector<float> hizTimes;
	//int m_outputContext = 0;
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
	void createSSRPass();
	void createCompositePass();
	void createFrustumCullingPass();
	void createResetDidirectDrawBufferPass();
	void initHiz();

	void loadShaders();
	void createStorageBuffers();

	void createSyncObjs();
protected:
	virtual void initUIRenderer();

private:
	std::shared_ptr<RenderPass> m_gbufferPass;
	std::shared_ptr<RenderPass> m_shadowPass;
	std::shared_ptr<RenderPass> m_lightingPass;
	std::shared_ptr<RenderPass> m_compositePass;
	//std::shared_ptr<RenderPass> m_ssrPass2;
	std::shared_ptr<ComputePass> m_ssrPass;
	std::shared_ptr<ComputePass> m_frustumCullingPass;
	std::shared_ptr<ComputePass> m_resetDidirectDrawBufferPass;

	std::vector<std::shared_ptr<MVulkanTexture>> swapchainDepthViews;
	std::shared_ptr<MVulkanTexture> shadowMap;
	std::shared_ptr<MVulkanTexture> shadowMapDepth;
	std::shared_ptr<MVulkanTexture> gBuffer0;
	std::shared_ptr<MVulkanTexture> gBuffer1;
	std::shared_ptr<MVulkanTexture> gBufferDepth;
	std::shared_ptr<MVulkanTexture> m_basicShadingTexture;
	std::shared_ptr<MVulkanTexture> m_ssrTexture;
	//std::shared_ptr<MVulkanTexture> m_ssrTexture_frag;

	std::shared_ptr<StorageBuffer> m_instanceBoundsBuffer;
	std::shared_ptr<StorageBuffer> m_indirectDrawBuffer;
	std::shared_ptr<StorageBuffer> m_numIndirectDrawBuffer;
	std::shared_ptr<StorageBuffer> m_culledIndirectDrawBuffer;
	std::shared_ptr<StorageBuffer> m_modelBuffer;
	std::shared_ptr<StorageBuffer> m_materialBuffer;
	std::shared_ptr<StorageBuffer> m_materialIdBuffer;
	std::shared_ptr<StorageBuffer> m_indirectInstanceBuffer;

	VkExtent2D shadowmapExtent = { 2048, 2048 };

	Hiz							m_hiz;
	MVulkanSampler				m_linearSampler;

	std::shared_ptr<Scene>		m_scene;
	std::shared_ptr<Scene>		m_squad;

	std::shared_ptr<Light>		m_directionalLight;

	std::shared_ptr<Camera>		m_directionalLightCamera;
	std::shared_ptr<Camera>		m_camera;

	glm::mat4					m_prevView;
	glm::mat4					m_prevProj;

	int							m_queryIndex = 0;

	MVulkanSemaphore			m_cullingSemaphore;
	MVulkanSemaphore			m_basicShadingSemaphore;
	MVulkanSemaphore			m_ssrSemaphore;
};

class SSRUI :public UIRenderer {
public:
	virtual void  RenderContext();

private:
	bool shouleRenderUI = true;
	int a;

public:
	int cullingMode = 0;
	//int hizMode = DO_HIZ_MODE_0;
	//int showMotionVector = 0;
	bool doSSR = 1;
	int outputContext = 0;
	bool showPassTime = 0;
	//bool copyHiz = true;
	//int hizLayer = 1;
};


#endif

