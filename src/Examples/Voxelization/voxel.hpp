#ifndef RTAO_HPP
#define RTAO_HPP

#include <MRenderApplication.hpp>
#include <memory>
#include <vector>
#include "MVulkanRHI/MVulkanSampler.hpp"
#include "MVulkanRHI/MVulkanBuffer.hpp"
#include "MVulkanRHI/MVulkanRayTracing.hpp"
#include "Shaders/ShaderModule.hpp"

const uint16_t WIDTH = 1280;
const uint16_t HEIGHT = 800;

class Scene;
class RenderPass;
class ComputePass;
class Camera;
class Light;

class VOXEL : public MRenderApplication {
public:
	virtual void SetUp();
	virtual void ComputeAndDraw(uint32_t imageIndex);

	virtual void RecreateSwapchainAndRenderPasses();
	virtual void CreateRenderPass();

	virtual void PreComputes();
	virtual void Clean();
private:
	void loadScene();
	void createCube();
	//void createAS();
	void createLight();
	void createCamera();

	void setOrthCamera(int axis);
	//void 
	void createSamplers();

	void createVoxelPass();
	void createPostPass();
	void createVoxelizeTestPass();
	void createClearTexturesPass();
	void createVoxeVisulizePass();
	void createGenVoxelVisulizeIndirectComandsPass();
	void createJFAPass();
	void createSDFDebugPass();

	void createTextures();
	void initVoxelVisulizeBuffers();

	void transitionVoxelTextureLayoutToComputeShaderRead();
	void transitionVoxelTextureLayoutToGeneral();

	void transitionSDFTextureLayoutToComputeShaderWrite();
	void transitionSDFTextureLayoutToFragmentShaderRead();

	BoundingBox GetSameSizeAABB(std::shared_ptr<Scene> scene);

private:
	std::shared_ptr<RenderPass> m_gbufferPass;
	std::shared_ptr<RenderPass> m_voxelPass;
	std::shared_ptr<RenderPass> m_voxelizeTestPass;
	std::shared_ptr<RenderPass> m_voxelVisulizePass;
	std::shared_ptr<ComputePass> m_genVoxelVisulizeIndirectComandsPass;
	std::shared_ptr<ComputePass> m_JFAPass;
	std::shared_ptr<ComputePass> m_clearTexturesPass;
	std::shared_ptr<RenderPass> m_postPass;
	std::shared_ptr<RenderPass> m_sdfDebugPass;

	MVulkanSampler				m_linearSampler;

	std::shared_ptr<Scene>		m_scene;
	std::shared_ptr<Scene>		m_squad;
	//std::shared_ptr<Scene>		m_cube;

	std::shared_ptr<Buffer>		m_cube_vertexBuffer;
	std::shared_ptr<Buffer>		m_cube_indexBuffer;
	std::shared_ptr<Buffer>		m_cube_indirectBuffer;
	std::shared_ptr<StorageBuffer>		m_voxelVisulizeBufferModel;
	std::shared_ptr<StorageBuffer>		m_voxelDrawCommandBuffer;
	//VkDrawIndirectCommand		m_cube_indirectCommand;

	std::shared_ptr<Light>		m_directionalLight;

	std::shared_ptr<Camera>		m_camera;
	std::shared_ptr<Camera>		m_orthCamera;

	std::shared_ptr<MVulkanTexture>		m_voxelTexture;
	std::shared_ptr<MVulkanTexture>		m_JFATexture0;
	std::shared_ptr<MVulkanTexture>		m_JFATexture1;
	std::shared_ptr<MVulkanTexture>		m_SDFTexture;
	std::shared_ptr<MVulkanTexture>		m_SDFAlbedoTexture;
	std::shared_ptr<MVulkanTexture>		m_SDFNormalTexture;
	//glm::ivec3 m_voxelResolution = { 64, 64, 64 };
	//int m_maxRaymarchSteps = 16;

	glm::ivec3 m_voxelResolution = { 128, 128, 128 };
	int m_maxRaymarchSteps = 100;
	
	//MVulkanRaytracing			m_rayTracing;
};



#endif // RTAO_HPP