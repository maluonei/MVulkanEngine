#ifndef MVULKANENGINE_RAYTRACINGPIPELINETEST_HPP
#define MVULKANENGINE_RAYTRACINGPIPELINETEST_HPP

#include <MRenderApplication.hpp>
#include <memory>
#include <vector>
#include "MVulkanRHI/MVulkanSampler.hpp"
#include "MVulkanRHI/MVulkanBuffer.hpp"
#include "MVulkanRHI/MVulkanRaytracing.hpp"

const uint16_t WIDTH = 1280;
const uint16_t HEIGHT = 800;

class Scene;
class RenderPass;
class Camera;
class Light;

class RayTracingPipelineTest : public MRenderApplication {
public:
	virtual void SetUp();
	virtual void ComputeAndDraw(uint32_t imageIndex);

	virtual void RecreateSwapchainAndRenderPasses();
	virtual void CreateRenderPass();

	virtual void PreComputes();
	virtual void Clean();
private:
	void loadScene();
	
private:
	std::shared_ptr<Scene> m_scene;

	MVulkanRaytracing rayTracing;
};

#endif