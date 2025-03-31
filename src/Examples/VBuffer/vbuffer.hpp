#ifndef MVULKANENGINE_VISIBILITY_HPP
#define MVULKANENGINE_VISIBILITY_HPP

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

class Meshlet;

class MSTestApplication : public MRenderApplication {
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
	//void createLight();
	void createCamera();
	//void createSamplers();
	//
	//void createLightCamera();

	void createStorageBuffers();

	void createVBufferPass();

	//void createTestPass();
	//void createTestPass2();
	//void createTestPass3();
	//void createMeshletPass();
	//void createMeshletPass2();

private:
	//std::shared_ptr<RenderPass> m_meshShaderTestPass;
	//std::shared_ptr<RenderPass> m_meshShaderTestPass2;
	//std::shared_ptr<RenderPass> m_meshShaderTestPass3;
	//std::shared_ptr<RenderPass> m_meshletPass;
	//std::shared_ptr<RenderPass> m_meshletPass2;
	std::shared_ptr<RenderPass> m_vbufferPass;

	//MVulkanSampler				m_linearSampler;

	std::shared_ptr<Scene>		m_scene;
	//std::shared_ptr<Scene>			m_suzanne;
	//
	//std::shared_ptr<Light>		m_directionalLight;
	//
	//std::shared_ptr<Camera>		m_directionalLightCamera;
	std::shared_ptr<Camera>		m_camera;

	std::shared_ptr<Meshlet>	m_meshLet;

	std::shared_ptr<StorageBuffer> m_verticesBuffer;
	std::shared_ptr<StorageBuffer> m_meshletsBuffer;
	std::shared_ptr<StorageBuffer> m_meshletVerticesBuffer;
	std::shared_ptr<StorageBuffer> m_meshletTrianglesBuffer;
	std::shared_ptr<StorageBuffer> m_meshletAddonBuffer;
	//std::shared_ptr<StorageBuffer> m_modelsBuffer;
	std::shared_ptr<StorageBuffer> m_meshletBoundsBuffer;

	//std::shared_ptr<StorageBuffer> m_testVerticesBuffer;
	//std::shared_ptr<StorageBuffer> m_testVerticesBuffer2;
	//std::shared_ptr<StorageBuffer> m_testIndexBuffer;
	//std::shared_ptr<StorageBuffer> m_meshletTrianglesU32Buffer;
};




#endif

