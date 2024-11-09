#pragma once
#ifndef RENDERPASS_H
#define RENDERPASS_H

#include "MVulkanRHI/MVulkanPipeline.hpp"
#include "MVulkanRHI/MVulkanRenderPass.hpp"
#include "MVulkanRHI/MVulkanFrameBuffer.hpp"
#include "MVulkanRHI/MVulkanDevice.hpp"

class ShaderModule;

class RenderPass {
public:
	RenderPass(MVulkanDevice device);
	void Create();



private:
	void CreatePipeline(const std::string& vertPath, const std::string& fragPath);
	void CreateRenderPass();
	void CreateFrameBuffer();

	void loadShader(const std::string& vertPath, const std::string& fragPath);

private:
	MVulkanDevice m_device;

	std::shared_ptr<ShaderModule> shader;
	MVulkanRenderPass renderPass;
	MVulkanPipeline pipeline;
	MVulkanFrameBuffer frameBuffer;

};


#endif