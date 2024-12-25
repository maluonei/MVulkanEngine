#ifndef MVULKANENGINE_COMPUTEPIPELINETEST_HPP
#define MVULKANENGINE_COMPUTEPIPELINETEST_HPP

#include <MRenderApplication.hpp>
#include <memory>
#include <vector>
#include "MVulkanRHI/MVulkanBuffer.hpp"
#include "MVulkanRHI/MVulkanSampler.hpp"

const uint16_t WIDTH = 1280;
const uint16_t HEIGHT = 800;

class Scene;
class ComputePass;
class Camera;
class Light;

class ComputePipelineTest : public MRenderApplication {
public:
	virtual void SetUp();
	virtual void UpdatePerFrame(uint32_t imageIndex);

	virtual void RecreateSwapchainAndRenderPasses();
	virtual void CreateRenderPass();

	virtual void PreComputes();
	virtual void Clean();
private:
	void createTestTexture();
	void createSampler();


private:
	std::shared_ptr<ComputePass>	m_testComputePass;

	std::shared_ptr<MVulkanTexture>	m_testTexture;
	MVulkanSampler					m_linearSampler;
};

#endif