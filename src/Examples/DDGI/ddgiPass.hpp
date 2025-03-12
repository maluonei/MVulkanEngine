#ifndef DDGI_PASS_HPP
#define DDGI_PASS_HPP

#include <MRenderApplication.hpp>
#include <memory>
#include <vector>
#include "MVulkanRHI/MVulkanSampler.hpp"
#include "MVulkanRHI/MVulkanBuffer.hpp"
#include "MVulkanRHI/MVulkanRayTracing.hpp"
#include "Shaders/ShaderModule.hpp"
#include "ddgiShader.hpp"


class DDGIVolume;

const uint16_t WIDTH = 1280;
const uint16_t HEIGHT = 800;

class Scene;
class RenderPass;
class ComputePass;
class Camera;
class Light;

class DDGIApplication : public MRenderApplication {
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
	void createStorageBuffers();
	void initDDGIVolumn();

	void createGbufferPass();
	void createProbeTracingPass();
	void createRTAOPass();
	void createLightingPass();
	void createProbeBlendingRadiancePass();
	void createProbeBlendingDepthPass();
	void createProbeClassficationPass();
	void createProbeVisulizePass();
	void createCompositePass();

	void changeTextureLayoutToRWTexture();
	void changeRWTextureLayoutToTexture();
	void transitionProbeVisulizeTextureLayoutToShaderRead();
    void transitionProbeVisulizeTextureLayoutToUndifined();

private:
	std::shared_ptr<RenderPass> m_gbufferPass;
	std::shared_ptr<RenderPass> m_probeTracingPass;
	std::shared_ptr<RenderPass> m_lightingPass;
	std::shared_ptr<RenderPass> m_rtaoPass;
	std::shared_ptr<RenderPass> m_probeVisulizePass;
	std::shared_ptr<RenderPass> m_compositePass;

	std::shared_ptr<ComputePass> m_probeBlendingRadiancePass;
	std::shared_ptr<ComputePass> m_probeBlendingDepthPass;
	std::shared_ptr<ComputePass> m_probeClassficationPass;

	std::shared_ptr<MVulkanTexture> m_acculatedAOTexture = nullptr;
	std::shared_ptr<MVulkanTexture> m_volumeProbeDatasRadiance = nullptr;
	std::shared_ptr<MVulkanTexture> m_volumeProbeDatasDepth = nullptr;
	std::shared_ptr<MVulkanTexture> m_testTexture = nullptr;

	std::shared_ptr<StorageBuffer> m_probesDataBuffer = nullptr;
	std::shared_ptr<StorageBuffer> m_probesModelBuffer = nullptr;
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

	int							m_raysPerProbe = 64;
	bool						m_sceneChange = true;
	bool						m_visualizeProbes = true;

	DDGIProbeVisulizeShader::UniformBuffer0 m_ProbeVisulizeShaderUniformBuffer0;
	UniformBuffer1				m_uniformBuffer1;


};


#endif // 