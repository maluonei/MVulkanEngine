#include "Hiz.hpp"
#include "MVulkanRHI/MVulkanBuffer.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"
#include "Shaders/share/Common.h"
#include "ComputePass.hpp"
#include "Managers/ShaderManager.hpp"

void Hiz::updateHizRes(glm::ivec2 basicResolution)
{
	glm::ivec2 res = basicResolution;

	int numLayers = 0;
	while (res.x >= 1 && res.y >= 1)
	{
		//hizRes[layer] = res;
		res /= 2;
		numLayers++;
	}

	m_hizRes.resize(numLayers);
	int layer = 0;
	res = basicResolution;
	while (res.x >= 1 && res.y >= 1)
	{
		m_hizRes[layer] = res;
		res /= 2;
		layer++;
	}

	m_numHizLayers = numLayers;
}

void Hiz::initHizTextures()
{
	m_hizTextures.clear();
	m_hizTextures.resize(m_numHizLayers);

	auto device = Singleton<MVulkanEngine>::instance().GetDevice();
	auto depthFormat = device.FindDepthFormat();

	ImageCreateInfo imageInfo;
	ImageViewCreateInfo viewInfo;
	imageInfo.arrayLength = 1;
	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	imageInfo.format = depthFormat;

	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = imageInfo.format;
	viewInfo.flag = VK_IMAGE_ASPECT_DEPTH_BIT;
	viewInfo.baseMipLevel = 0;
	viewInfo.levelCount = 1;
	viewInfo.baseArrayLayer = 0;
	viewInfo.layerCount = 1;

	for (auto i = 0; i < m_numHizLayers; i++) {
		auto res = m_hizRes[i];

		//auto depthFormat = device.FindDepthFormat();
		imageInfo.width = res.x;
		imageInfo.height = res.y;

		m_hizTextures[i] = std::make_shared<MVulkanTexture>();

		if (i == 0) {
			viewInfo.levelCount = m_numHizLayers;
			imageInfo.mipLevels = m_numHizLayers;
		}
		else {
			viewInfo.levelCount = 1;
			imageInfo.mipLevels = 1;
		}
		Singleton<MVulkanEngine>::instance().CreateImage(m_hizTextures[i], imageInfo, viewInfo);
	}
}

void Hiz::initHizBuffers()
{
	auto device = Singleton<MVulkanEngine>::instance().GetDevice();

	BufferCreateInfo info{};
	info.arrayLength = 1;

	std::vector<HizDimension> hizDimensions(MAX_HIZ_LAYERS);
	for (int i = 0; i < m_numHizLayers; i++) {
		hizDimensions[i].u_previousLevelDimensions = m_hizRes[i];
	}

	info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	info.size = sizeof(HizDimension) * MAX_HIZ_LAYERS;
	m_hizDimensionsBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, hizDimensions.data());

	info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	info.size = sizeof(HIZBuffer);
	m_hizBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info);
}

void Hiz::initComputePass()
{
	initResetHizBufferPass();
	initUpdateHizBufferPass();
	initGenHizPass();
}


void Hiz::initResetHizBufferPass()
{
	auto device = Singleton<MVulkanEngine>::instance().GetDevice();

	{
		m_resetHizBufferPass = std::make_shared<ComputePass>(device);

		auto shader = Singleton<ShaderManager>::instance().GetShader<ComputeShaderModule>("ResetHizBuffer Shader");

		Singleton<MVulkanEngine>::instance().CreateComputePass(
			m_resetHizBufferPass, shader);

		std::vector<PassResources> resources;

		//PassResources resource;
		resources.push_back(PassResources::SetBufferResource(0, 0, m_hizBuffer));

		m_resetHizBufferPass->UpdateDescriptorSetWrite(resources);
	}
}

void Hiz::initUpdateHizBufferPass()
{
	auto device = Singleton<MVulkanEngine>::instance().GetDevice();

	{
		m_updateHizBufferPass = std::make_shared<ComputePass>(device);

		auto shader = Singleton<ShaderManager>::instance().GetShader<ComputeShaderModule>("UpdateHizBuffer Shader");

		Singleton<MVulkanEngine>::instance().CreateComputePass(
			m_updateHizBufferPass, shader);

		std::vector<PassResources> resources;

		//PassResources resource;
		//resources.push_back(PassResources::SetBufferResource("hizBuffer", 0, 0, 0));
		//resources.push_back(PassResources::SetSampledImageResource(1, 0, gBufferDepth));
		//resources.push_back(PassResources::SetStorageImageResource(2, 0, m_hizTextures));
		resources.push_back(PassResources::SetBufferResource(0, 0, m_hizDimensionsBuffer));
		resources.push_back(PassResources::SetBufferResource(1, 0, m_hizBuffer));

		m_updateHizBufferPass->UpdateDescriptorSetWrite(resources);
	}
}

void Hiz::initGenHizPass()
{
	auto device = Singleton<MVulkanEngine>::instance().GetDevice();

	{
		m_genHizPass = std::make_shared<ComputePass>(device);

		auto shader = Singleton<ShaderManager>::instance().GetShader<ComputeShaderModule>("GenHiz Shader");

		Singleton<MVulkanEngine>::instance().CreateComputePass(
			m_genHizPass, shader);

		std::vector<PassResources> resources;

		//PassResources resource;
		//resources.push_back(PassResources::SetBufferResource("hizBuffer", 0, 0, 0));
		resources.push_back(PassResources::SetBufferResource(0, 0, m_hizBuffer));
		//resources.push_back(PassResources::SetSampledImageResource(1, 0, gBufferDepth));
		resources.push_back(PassResources::SetStorageImageResource(2, 0, m_hizTextures));

		m_genHizPass->UpdateDescriptorSetWrite(resources);
	}
}

Hiz::Hiz()
{
	//Singleton<ShaderManager>::instance().AddShader("GenHiz Shader", { "hlsl/culling/GenHiz.comp.hlsl" });
	//Singleton<ShaderManager>::instance().AddShader("UpdateHizBuffer Shader", { "hlsl/culling/UpdateHizBuffer.comp.hlsl" });
	//Singleton<ShaderManager>::instance().AddShader("ResetHizBuffer Shader", { "hlsl/culling/ResetHizBuffer.comp.hlsl" });
}

void Hiz::Clean()
{
}

void Hiz::UpdateHiz(glm::ivec2 basicResolution)
{
	updateHizRes(basicResolution);

	for (auto i = 0; i < m_hizTextures.size(); i++) {
		m_hizTextures[i]->Clean();
	}

	
}

const int Hiz::GetHizLayers() const
{
	return m_numHizLayers;
}

const glm::ivec2 Hiz::GetHizRes(int layer) const
{
	return m_hizRes[layer];
}

std::shared_ptr<MVulkanTexture> Hiz::GetHizTexture(int layer)
{
	return m_hizTextures[layer];
}

std::vector<std::shared_ptr<MVulkanTexture>> Hiz::GetHizTextures()
{
	return m_hizTextures;
}

void Hiz::Init(glm::ivec2 basicResolution)
{
	Singleton<ShaderManager>::instance().AddShader("GenHiz Shader", { "hlsl/culling/GenHiz.comp.hlsl" });
	Singleton<ShaderManager>::instance().AddShader("UpdateHizBuffer Shader", { "hlsl/culling/UpdateHizBuffer.comp.hlsl" });
	Singleton<ShaderManager>::instance().AddShader("ResetHizBuffer Shader", { "hlsl/culling/ResetHizBuffer.comp.hlsl" });

	updateHizRes(basicResolution);
	initHizTextures();
	initHizBuffers();
	initComputePass();
}

void Hiz::Init(glm::ivec2 basicResolution, std::shared_ptr<MVulkanTexture> depth) {
	Singleton<ShaderManager>::instance().AddShader("GenHiz Shader", { "hlsl/culling/GenHiz.comp.hlsl" });
	Singleton<ShaderManager>::instance().AddShader("UpdateHizBuffer Shader", { "hlsl/culling/UpdateHizBuffer.comp.hlsl" });
	Singleton<ShaderManager>::instance().AddShader("ResetHizBuffer Shader", { "hlsl/culling/ResetHizBuffer.comp.hlsl" });
	
	updateHizRes(basicResolution);
	initHizTextures();
	initHizBuffers();
	initComputePass();
	SetSourceDepth(depth);
}


void Hiz::SetSourceDepth(std::shared_ptr<MVulkanTexture> depth)
{
	std::vector<PassResources> resources;

	resources.push_back(PassResources::SetSampledImageResource(1, 0, depth));

	m_genHizPass->UpdateDescriptorSetWrite(resources);
}

void Hiz::Generate(MComputeCommandList commandList)
{
	//hizTimes.resize(numHizLayers);
	for (auto layer = 0; layer < m_numHizLayers; layer++) {
		addHizBufferWriteBarrier();
		if (layer == 0) {
			Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_resetHizBufferPass, commandList, 1, 1, 1, std::string("ResetHizBuffer Pass"));
		}
		else {
			Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_updateHizBufferPass, commandList, 1, 1, 1, std::string("UpdateHizBuffer Pass"));
			addHizImageBarrier(layer);
		}
		addHizBufferReadBarrier();
		Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_genHizPass, commandList, ((m_hizRes[layer].x + 15) / 16), ((m_hizRes[layer].y + 15) / 16), 1, std::string("GenHiz Pass"));
	
	}
	
	//copy hiz
	{
		copyHizToDepth();
	}
}

void Hiz::Generate(MComputeCommandList commandList, int& queryIndex)
{
	hizQueryIndexStart = queryIndex;
	auto numHizLayers = m_hizRes.size();

	for (auto layer = 0; layer < m_numHizLayers; layer++) {
		//hizQueryIndex[layer] = queryIndex;
		addHizBufferWriteBarrier();
		if (layer == 0) {
			Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_resetHizBufferPass, commandList, 1, 1, 1, std::string("ResetHizBuffer Pass"), queryIndex++);
		}
		else {
			Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_updateHizBufferPass, commandList, 1, 1, 1, std::string("UpdateHizBuffer Pass"), queryIndex++);
			addHizImageBarrier(layer);
		}
		addHizBufferReadBarrier();
		Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_genHizPass, commandList, ((m_hizRes[layer].x + 15) / 16), ((m_hizRes[layer].y + 15) / 16), 1, std::string("GenHiz Pass"), queryIndex++);

	}
	hizQueryIndexEnd = queryIndex;

	//copy hiz
	{
		copyHizToDepth();
	}
}

const int Hiz::GetHizQueryIndexStart() const
{
	return hizQueryIndexStart;
}

const int Hiz::GetHizQueryIndexEnd() const
{
	return hizQueryIndexEnd;
}

void Hiz::addHizImageBarrier(int layer)
{
	auto device = Singleton<MVulkanEngine>::instance().GetDevice();

	{
		auto& computeList = Singleton<MVulkanEngine>::instance().GetComputeCommandList();
		std::vector<MVulkanImageMemoryBarrier> barriers;
		MVulkanImageMemoryBarrier barrier;

		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_hizTextures[layer - 1]->GetImage();  // 指 tex[1] 对应的 VkImage
		barrier.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		barrier.baseMipLevel = 0;
		barrier.levelCount = 1;
		barrier.baseArrayLayer = 0;
		barrier.layerCount = 1;
		barriers.push_back(barrier);

		barrier.srcAccessMask = VK_ACCESS_NONE;
		barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.image = m_hizTextures[layer]->GetImage();  // 指 tex[1] 对应的 VkImage
		barrier.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		barriers.push_back(barrier);

		Singleton<MVulkanEngine>::instance().TransitionImageLayout2(computeList, barriers, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	}
}

void Hiz::addHizBufferReadBarrier()
{
	auto device = Singleton<MVulkanEngine>::instance().GetDevice();

	{
		auto& computeList = Singleton<MVulkanEngine>::instance().GetComputeCommandList();
		std::vector<VkBufferMemoryBarrier> barriers;
		VkBufferMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;

		//barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		//barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		//barrier.image = m_hizTextures[layer - 1]->GetImage();  // 指 tex[1] 对应的 VkImage
		barrier.buffer = m_hizBuffer->GetBuffer();
		barrier.offset = 0;
		barrier.size = VK_WHOLE_SIZE;

		barriers.push_back(barrier);

		Singleton<MVulkanEngine>::instance().TransitionBufferLayout2(computeList, barriers, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	}
}

void Hiz::addHizBufferWriteBarrier()
{
	auto device = Singleton<MVulkanEngine>::instance().GetDevice();

	{
		auto& computeList = Singleton<MVulkanEngine>::instance().GetComputeCommandList();
		std::vector<VkBufferMemoryBarrier> barriers;
		VkBufferMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;

		//barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		//barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		//barrier.image = m_hizTextures[layer - 1]->GetImage();  // 指 tex[1] 对应的 VkImage
		barrier.buffer = m_hizBuffer->GetBuffer();
		barrier.offset = 0;
		barrier.size = VK_WHOLE_SIZE;

		barriers.push_back(barrier);

		Singleton<MVulkanEngine>::instance().TransitionBufferLayout2(computeList, barriers, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	}
}

void Hiz::copyHizToDepth()
{
	auto device = Singleton<MVulkanEngine>::instance().GetDevice();

	{
		auto& computeList = Singleton<MVulkanEngine>::instance().GetComputeCommandList();

		auto numMips = m_hizRes.size();

		std::vector<std::shared_ptr<MVulkanTexture>> srcs;
		std::vector<std::shared_ptr<MVulkanTexture>> dsts;
		std::vector<MVulkanImageCopyInfo> infos;
		for (auto i = 1; i < numMips; i++) {
			MVulkanImageCopyInfo copyInfo{};

			copyInfo.srcAspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			copyInfo.dstAspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			copyInfo.srcMipLevel = 0;
			copyInfo.dstMipLevel = i;

			copyInfo.extent = { (uint32_t)m_hizRes[i].x, (uint32_t)m_hizRes[i].y, 1 };

			srcs.push_back(m_hizTextures[i]);
			dsts.push_back(m_hizTextures[0]);
			infos.push_back(copyInfo);
		}
		Singleton<MVulkanEngine>::instance().CopyImage2(computeList, dsts, srcs, infos);
	}
}