#include "ComputePipelineTest.hpp"
#include "ComputePass.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"

void ComputePipelineTest::SetUp()
{
	createSampler();
	createTestTexture();
}

void ComputePipelineTest::UpdatePerFrame(uint32_t imageIndex)
{
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
