#ifndef MVULKANENGINE_PBR_HPP
#define MVULKANENGINE_PBR_HPP

#include <MRenderApplication.hpp>
#include <memory>
#include <vector>
#include "MVulkanRHI/MVulkanSampler.hpp"
#include "MVulkanRHI/MVulkanBuffer.hpp"
#include "Scene/Mesh/MeshDistanceField.hpp"

const uint16_t WIDTH = 1280;
const uint16_t HEIGHT = 800;

class Scene;
class RenderPass;
class DynamicRenderPass;
class Camera;
class Light;

class MDF: public MRenderApplication {
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
	//void createGbufferPass();
	//void createShadowPass();
	void createShadingPass();
	

	void loadShaders();

	//void testEmbreeScene();
	void createMDF();
	void createStorageBuffers();

	//void ImageLayoutToShaderRead(int currentFrame);
	//void ImageLayoutToAttachment(int imageIndex, int currentFrame);
	//void ImageLayoutToPresent(int imageIndex, int currentFrame);
private:
	//std::shared_ptr<RenderPass> m_gbufferPass;
	//std::shared_ptr<RenderPass> m_shadowPass;
	//std::shared_ptr<RenderPass> m_lightingPass;
	//std::shared_ptr<RenderPass> m_gbufferPass;
	//std::shared_ptr<RenderPass> m_shadowPass;
	std::shared_ptr<RenderPass> m_shadingPass;

	std::vector<std::shared_ptr<MVulkanTexture>> swapchainDepthViews;
	std::vector<std::vector<std::shared_ptr<MVulkanTexture>>> swapchainDebugViewss;
	//std::shared_ptr<MVulkanTexture> shadowMap;
	//std::shared_ptr<MVulkanTexture> shadowMapDepth;
	//std::shared_ptr<MVulkanTexture> gBuffer0;
	//std::shared_ptr<MVulkanTexture> gBuffer1;
	//std::shared_ptr<MVulkanTexture> gBufferDepth;

	std::shared_ptr<MVulkanTexture> mdfTexture;


	VkExtent2D shadowmapExtent = { 4096, 4096 };


	MVulkanSampler				m_linearSampler;

	std::shared_ptr<Scene>		m_scene;
	std::shared_ptr<Scene>		m_squad;
	std::shared_ptr<Scene>		m_sphere;

	std::shared_ptr<Light>		m_directionalLight;

	std::shared_ptr<Camera>		m_directionalLightCamera;
	std::shared_ptr<Camera>		m_camera;

	//SparseMeshDistanceField		m_mdf;
	MeshDistanceFieldAtlas		m_mdfAtlas;
	std::shared_ptr<StorageBuffer> m_mdfBuffers;
};




#endif

