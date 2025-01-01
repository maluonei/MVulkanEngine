#ifndef MVULKANENGINE_COMPUTEPIPELINETEST_HPP
#define MVULKANENGINE_COMPUTEPIPELINETEST_HPP

#include <MRenderApplication.hpp>
#include <memory>
#include <vector>
#include "MVulkanRHI/MVulkanBuffer.hpp"
#include "MVulkanRHI/MVulkanSampler.hpp"
#include "Shaders/ShaderModule.hpp"

const uint16_t WIDTH = 1280;
const uint16_t HEIGHT = 800;

class Scene;
class ComputePass;
class RenderPass;
class Camera;
class Light;

class ComputePipelineTest : public MRenderApplication {
public:
	virtual void SetUp();
	virtual void ComputeAndDraw(uint32_t imageIndex);

	virtual void RecreateSwapchainAndRenderPasses();
	virtual void CreateRenderPass();

	virtual void PreComputes();
	virtual void Clean();
private:
	void createTestTexture();
	void createSampler();

	void loadScene();

private:
	TestCompShader::InputBuffer		input;

	std::shared_ptr<ComputePass>	m_testComputePass;
	std::shared_ptr<RenderPass>		m_testRenderPass;

	std::shared_ptr<MVulkanTexture>	m_testTexture;
	MVulkanSampler					m_linearSampler;

	std::shared_ptr<Scene>			m_squad;
};

#endif