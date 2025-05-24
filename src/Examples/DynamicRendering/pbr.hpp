#ifndef MVULKANENGINE_PBR_HPP
#define MVULKANENGINE_PBR_HPP

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

class PBR: public MRenderApplication {
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
	int numShadowmapDrawInstances = 0;
	int numTotalInstances = 0;

	float gbufferTime = 0.f;
	float shadowmapTime = 0.f;
	float shadingTime = 0.f;
	float cullingTime = 0.f;
	float reprojectDepthTime = 0.f;
	float hizTime = 0.f;
	std::vector<float> hizTimes;
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
	void createFrustumCullingPass();
	void createReprojectDepthPass();
	void createResetDidirectDrawBufferPass();
	void createGenHizPass();
	void createUpdateHizBufferPass();
	void createResetHizBufferPass();

	void loadShaders();
	void createStorageBuffers();

	void createSyncObjs();
	void initHiz();

	std::shared_ptr<MVulkanTexture> getDepthCurrent();
	std::shared_ptr<MVulkanTexture> getDepthPrev();

	std::shared_ptr<MVulkanTexture> getShadowDepthCurrent();
	std::shared_ptr<MVulkanTexture> getShadowDepthPrev();

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
	//std::shared_ptr<ComputePass> m_frustumCullingPass;
	std::shared_ptr<ComputePass> m_cullingPass;
	std::shared_ptr<ComputePass> m_shadowmapCullingPass;
	std::shared_ptr<ComputePass> m_resetIndirectDrawBufferPass;
	std::shared_ptr<ComputePass> m_resetShadowmapIndirectDrawBufferPass;
	std::shared_ptr<ComputePass> m_reprojectDepthPass;
	std::shared_ptr<ComputePass> m_reprojectShadowmapDepthPass;
	//std::shared_ptr<ComputePass> m_hizPass;
	//std::shared_ptr<ComputePass> m_updateHizBufferPass;
	//std::shared_ptr<ComputePass> m_resetHizBufferPass;

	std::vector<std::shared_ptr<MVulkanTexture>> swapchainDepthViews;
	std::shared_ptr<MVulkanTexture> shadowMap;
	std::shared_ptr<MVulkanTexture> shadowMapDepth[2];
	std::shared_ptr<MVulkanTexture> gBuffer0;
	std::shared_ptr<MVulkanTexture> gBuffer1;
	std::shared_ptr<MVulkanTexture> gBufferDepth[2];

	//std::vector<std::shared_ptr<MVulkanTexture>> m_hizTextures;
	std::shared_ptr<StorageBuffer> m_instanceBoundsBuffer;
	std::shared_ptr<StorageBuffer> m_indirectDrawBuffer;
	std::shared_ptr<StorageBuffer> m_numIndirectDrawBuffer;
	std::shared_ptr<StorageBuffer> m_culledIndirectDrawBuffer;
	std::shared_ptr<StorageBuffer> m_indirectInstanceBuffer;
	std::shared_ptr<StorageBuffer> m_shadowmapIndirectDrawBuffer;
	std::shared_ptr<StorageBuffer> m_shadowmapNumIndirectDrawBuffer;
	std::shared_ptr<StorageBuffer> m_shadowmapCulledIndirectDrawBuffer;
	std::shared_ptr<StorageBuffer> m_shadowmapIndirectInstanceBuffer;
	//std::shared_ptr<StorageBuffer> m_hizBuffer;
	//std::shared_ptr<StorageBuffer> m_hizDimensionsBuffer;
	std::shared_ptr<StorageBuffer> m_modelBuffer;
	std::shared_ptr<StorageBuffer> m_materialBuffer;
	std::shared_ptr<StorageBuffer> m_materialIdBuffer;
	//std::shared_ptr<StorageBuffer> m_depthDebugBuffer;
	//std::shared_ptr<StorageBuffer> m_depthDebugBuffer1;

	VkExtent2D shadowmapExtent = { 2048, 2048 };

	Hiz							m_hiz;
	Hiz							m_shadowmapHiz;
	MVulkanSampler				m_linearSampler;

	std::shared_ptr<Scene>		m_scene;
	std::shared_ptr<Scene>		m_squad;

	std::shared_ptr<Light>		m_directionalLight;

	std::shared_ptr<Camera>		m_directionalLightCamera;
	std::shared_ptr<Camera>		m_camera;

	glm::mat4					m_prevView;
	glm::mat4					m_prevProj;
	glm::mat4					m_prevShadowView;
	glm::mat4					m_prevShadowProj;

	int							m_queryIndex = 0;
	int							m_depthIndex = 0;
	bool						m_firstFrame = true;

	MVulkanSemaphore			m_cullingSemaphore;
	MVulkanSemaphore			m_shadingSemaphore;
};

class DRUI :public UIRenderer {
public:
	virtual void  RenderContext();

private:
	bool shouleRenderUI = true;
	int a;

public:
	int frustumCullingMode = 2;
	bool doHizCulling = false;
	//int hizMode = DO_HIZ_MODE_0;
	int outputContext = 0;
	bool showPassTime = 0;
	bool cullShadowmap = true;
	bool calculateDrawNums = false;
	bool shadowmapCulling = false;
};


#endif

