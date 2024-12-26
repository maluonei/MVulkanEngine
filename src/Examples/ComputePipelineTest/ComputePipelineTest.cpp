#include "ComputePipelineTest.hpp"
#include "ComputePass.hpp"
#include "RenderPass.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"
#include "Scene/SceneLoader.hpp"
#include "Scene/Scene.hpp"

void ComputePipelineTest::SetUp()
{
	createSampler();
	createTestTexture();

	loadScene();
}

void ComputePipelineTest::ComputeAndDraw(uint32_t imageIndex)
{
	auto computeList = Singleton<MVulkanEngine>::instance().GetComputeCommandList();
	auto computeQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::COMPUTE);

	computeList.Reset();
	computeList.Begin();

	{
		TestCompShader::Constants constant;
		constant.Width = 16;
		constant.Height = 16;

		auto shader = m_testComputePass->GetShader();
		shader->SetUBO(0, &constant);

		TestCompShader::InputBuffer input;
		for (auto i = 0; i < 16 * 16; i+=4) {
			input.data[i] = 0;
			input.data[i+1] = 1;
			input.data[i+2] = 2;
			input.data[i+3] = 3;
		}

		shader->SetUBO(1, &input);
	}

	Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_testComputePass, 16, 16, 1);

	computeList.End();

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &computeList.GetBuffer();

	computeQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
	computeQueue.WaitForQueueComplete();

	auto graphicsList = Singleton<MVulkanEngine>::instance().GetGraphicsList(m_currentFrame);
	auto graphicsQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::GRAPHICS);

	graphicsList.Reset();
	graphicsList.Begin();

	Singleton<MVulkanEngine>::instance().RecordCommandBuffer(imageIndex, m_testRenderPass, m_currentFrame, m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size());

	graphicsList.End();
	Singleton<MVulkanEngine>::instance().SubmitGraphicsCommands(imageIndex, m_currentFrame);
}

void ComputePipelineTest::RecreateSwapchainAndRenderPasses()
{

}

void ComputePipelineTest::CreateRenderPass()
{
	auto device = Singleton<MVulkanEngine>::instance().GetDevice();
	{
		std::shared_ptr<ComputeShaderModule> testCompShader = std::make_shared<TestCompShader>();
		m_testComputePass = std::make_shared<ComputePass>(device);

		std::vector<uint32_t> storageBufferSizes(2);
		storageBufferSizes[0] = 16 * 16 * sizeof(float);
		storageBufferSizes[1] = 16 * 16 * sizeof(float);

		std::vector<std::vector<StorageImageCreateInfo>> storageImageCreateInfo(1);
		storageImageCreateInfo[0].resize(1);
		storageImageCreateInfo[0][0].width = 16;
		storageImageCreateInfo[0][0].height = 16;
		storageImageCreateInfo[0][0].depth = 1;
		storageImageCreateInfo[0][0].format = VK_FORMAT_R32G32B32A32_SFLOAT;

		std::vector<VkSampler> samplers(1, m_linearSampler.GetSampler());

		std::vector<std::vector<VkImageView>> seperateImageViews(1);
		seperateImageViews[0].resize(1);
		seperateImageViews[0][0] = m_testTexture->GetImageView();

		Singleton<MVulkanEngine>::instance().CreateComputePass(m_testComputePass, testCompShader, 
			storageBufferSizes, storageImageCreateInfo, seperateImageViews, samplers);
		spdlog::info("m_testComputePass create success!");
	}

	{
        std::vector<VkFormat> testSquadPassFormats;
		testSquadPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());

        RenderPassCreateInfo info{};
        info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
        info.depthFormat = device.FindDepthFormat();
        info.frambufferCount = Singleton<MVulkanEngine>::instance().GetSwapchainImageCount();
        info.useSwapchainImages = true;
        info.imageAttachmentFormats = testSquadPassFormats;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        info.initialDepthLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
        info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
        //info.depthView = m_testRenderPass->GetFrameBuffer(0).GetDepthImageView();

		m_testRenderPass = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<ShaderModule> testSquadShader = std::make_shared<TestSquadShader>();
        std::vector<std::vector<VkImageView>> gbufferViews(0);

        std::vector<VkSampler> samplers(0);

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
			m_testRenderPass, testSquadShader, gbufferViews, samplers);
    }
}

void ComputePipelineTest::PreComputes()
{

}

void ComputePipelineTest::Clean()
{
	m_testTexture->Clean();
	m_linearSampler.Clean();
}

void ComputePipelineTest::createTestTexture()
{
	auto generalGraphicList = Singleton<MVulkanEngine>::instance().GetCommandList(MQueueType::GRAPHICS);
	auto graphicsQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::GRAPHICS);

	fs::path projectRootPath = PROJECT_ROOT;
	fs::path texturePath = projectRootPath.append("resources").append("textures").append("texture.jpg");

	//std::vector<MImage<unsigned char>*> image(1);
	MImage<unsigned char> image;
	if (!image.Load(texturePath)) {
		spdlog::error("Failed to load texture: {}", texturePath.string());
	}

	std::vector<MImage<unsigned char>*> images(1);
	images[0] = &image;

	m_testTexture = std::make_shared<MVulkanTexture>();

	Singleton<MVulkanEngine>::instance().CreateImage(m_testTexture, images, true);
}

void ComputePipelineTest::createSampler()
{
	{
		MVulkanSamplerCreateInfo info{};
		info.minFilter = VK_FILTER_LINEAR;
		info.magFilter = VK_FILTER_LINEAR;
		info.mipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		m_linearSampler.Create(Singleton<MVulkanEngine>::instance().GetDevice(), info);
	}
}

void ComputePipelineTest::loadScene()
{
	fs::path projectRootPath = PROJECT_ROOT;
	fs::path resourcePath = projectRootPath.append("resources").append("models");

	m_squad = std::make_shared<Scene>();
	fs::path squadPath = resourcePath / "squad.obj";
	Singleton<SceneLoader>::instance().Load(squadPath.string(), m_squad.get());

	m_squad->GenerateIndirectDataAndBuffers();
}
