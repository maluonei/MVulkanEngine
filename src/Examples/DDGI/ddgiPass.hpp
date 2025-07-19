#ifndef DDGI_PASS_HPP
#define DDGI_PASS_HPP

#include "MRenderApplication.hpp"
#include "UIRenderer.hpp"
#include <memory>
#include <vector>
#include "MVulkanRHI/MVulkanSampler.hpp"
#include "MVulkanRHI/MVulkanBuffer.hpp"
#include "MVulkanRHI/MVulkanRayTracing.hpp"
#include "Shaders/ShaderModule.hpp"
#include "Shaders/ddgiShader.hpp"

class DDGIVolume;

const uint16_t WIDTH = 1280;
const uint16_t HEIGHT = 800;

class Scene;
class RenderPass;
class ComputePass;
class Camera;
class Light;
class JsonFileLoader;

class DDGIApplication : public MRenderApplication {
public:
	virtual void SetUp();
	virtual void ComputeAndDraw(uint32_t imageIndex);

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
	void createStorageBuffers();
	void initDDGIVolumn();

	void createGbufferPass();
	void createProbeTracingPass();
	void createRTAOPass();
	void createLightingPass();
	void createProbeRelocationPass();
	void createProbeBlendingRadiancePass();
	void createProbeBlendingDepthPass();
	void createProbeClassficationPass();
	void createProbeVisulizePass();
	void createCompositeScenePass();

	void changeTextureLayoutToRWTexture();
	void changeRWTextureLayoutToTexture();
	void transitionProbeVisulizeTextureLayoutToShaderRead();
    void transitionProbeVisulizeTextureLayoutToUndifined();

	void loadShaders();
	void createSyncObjs();
protected:
	virtual void initUIRenderer();

private:
	std::shared_ptr<RenderPass> m_gbufferPass;
	std::shared_ptr<RenderPass> m_probeTracingRenderPass;
	//std::shared_ptr<ComputePass> m_probeTracingPass;
	std::shared_ptr<RenderPass> m_lightingPass;
	std::shared_ptr<RenderPass> m_rtaoPass;
	std::shared_ptr<RenderPass> m_probeVisulizePass;

	std::shared_ptr<RenderPass> m_compositeScenePass;
	std::shared_ptr<RenderPass> m_finalPass;

	std::shared_ptr<ComputePass> m_probeBlendingRadiancePass;
	std::shared_ptr<ComputePass> m_probeBlendingDepthPass;
	std::shared_ptr<ComputePass> m_probeClassficationPass;
	std::shared_ptr<ComputePass> m_probeRelocationPass;

	std::shared_ptr<MVulkanTexture> m_acculatedAOTexture = nullptr;
	std::shared_ptr<MVulkanTexture> m_volumeProbeDatasRadiance = nullptr;
	std::shared_ptr<MVulkanTexture> m_volumeProbeDatasDepth = nullptr;
	//std::shared_ptr<MVulkanTexture> m_testTexture = nullptr;

	std::vector<std::shared_ptr<MVulkanTexture>> swapchainDepthViews;
	std::shared_ptr<MVulkanTexture> gBuffer0 = nullptr;
	std::shared_ptr<MVulkanTexture> gBuffer1 = nullptr;
	std::shared_ptr<MVulkanTexture> gBuffer2 = nullptr;
	std::shared_ptr<MVulkanTexture> gBuffer3 = nullptr;
	std::shared_ptr<MVulkanTexture> gBufferDepth = nullptr;

	std::shared_ptr<MVulkanTexture> m_probePositions = nullptr;
	std::shared_ptr<MVulkanTexture> m_probeNormals = nullptr;
	std::shared_ptr<MVulkanTexture> m_probeDepth = nullptr;
	//std::shared_ptr<MVulkanTexture> m_probeAlbedo = nullptr;
	std::shared_ptr<MVulkanTexture> m_probeRadiance = nullptr;

	std::shared_ptr<MVulkanTexture> m_diTexture = nullptr;
	std::shared_ptr<MVulkanTexture> m_giTexture = nullptr;
	std::shared_ptr<MVulkanTexture> m_aoTexture = nullptr;
	std::shared_ptr<MVulkanTexture> m_probeVisulizTexture = nullptr;

	std::shared_ptr<MVulkanTexture> m_matIdTexture = nullptr;
	std::shared_ptr<MVulkanTexture> m_texCoordsTexture = nullptr;

	std::shared_ptr<MVulkanTexture> m_testRayQueryDepth = nullptr;

	std::shared_ptr<StorageBuffer> m_modelBuffer = nullptr;
	std::shared_ptr<StorageBuffer> m_probesDataBuffer = nullptr;
	std::shared_ptr<StorageBuffer> m_probesModelBuffer = nullptr;	
	std::shared_ptr<StorageBuffer> m_materialBuffer = nullptr;
	std::shared_ptr<StorageBuffer> m_materialIdBuffer = nullptr;
	std::shared_ptr<StorageBuffer> m_tlasVertexBuffer = nullptr;
	std::shared_ptr<StorageBuffer> m_tlasIndexBuffer = nullptr;
	std::shared_ptr<StorageBuffer> m_tlasNormalBuffer = nullptr;
	std::shared_ptr<StorageBuffer> m_tlasUVBuffer = nullptr;
	std::shared_ptr<StorageBuffer> m_geometryInfo = nullptr;

	MVulkanSampler				m_linearSamplerWithoutAnisotropy;
	MVulkanSampler				m_linearSamplerWithAnisotropy;

	std::shared_ptr<Scene>		m_scene;
	std::shared_ptr<Scene>		m_squad;
	std::shared_ptr<Scene>		m_sphere;

	std::shared_ptr<Light>		m_directionalLight;

	std::shared_ptr<Camera>		m_camera;
	MVulkanRaytracing			m_rayTracing;
	std::shared_ptr<DDGIVolume> m_volume = nullptr;

	std::shared_ptr<JsonFileLoader> m_jsonLoader = nullptr;

	int							m_raysPerProbe = 64;
	bool						m_sceneChange = true;
	bool						m_visualizeProbes = false;
	float						m_start = 0.f;
	int							m_queryIndex = 0;

	MVulkanSemaphore			m_shadingSemaphore;
	MVulkanSemaphore			m_ddgiSemephore;

public:
	float                       m_gbufferTime;
	float                       m_probeTracingTime;
	float                       m_probeRelocationTime;
	float                       m_probeClassficationTime;
	float                       m_probeBlendRadianceTime;
	float                       m_probeBlendDepthTime;
	float                       m_lightingTime;
	float                       m_rtaoTime;
	float                       m_probeVisulizeTime;
	float                       m_compositeTime;
};

class DDGIUI :public UIRenderer {
public:
	virtual void  RenderContext();

private:
	bool shouleRenderUI = true;

public:
	bool m_probeClassfication = true;
	bool m_probeRelocationEnabled = true;
	int m_visulizeMode = 4;
	bool m_showPassTime = true;
};



#endif // 