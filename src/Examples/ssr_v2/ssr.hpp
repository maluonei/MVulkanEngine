#pragma once
#ifndef SSR_HPP
#define SSR_HPP

#include <MRenderApplication.hpp>
#include <memory>
#include <vector>
#include "MVulkanRHI/MVulkanSampler.hpp"
#include "MVulkanRHI/MVulkanBuffer.hpp"
#include "Shaders/ShaderModule.hpp"

const uint16_t WIDTH = 1280;
const uint16_t HEIGHT = 800;

class Scene;
class RenderPass;
class ComputePass;
class Camera;
class Light;

#define USE_SPD

class DownSampleDepthShader :public ComputeShaderModule{
public:
	struct Constants {
		int u_previousLevel;
		uint32_t u_previousLevelDimensionsWidth;
		uint32_t u_previousLevelDimensionsHeight;
		uint32_t padding0;
	};

	std::vector<VkExtent2D>					depthExtents;
public:
	DownSampleDepthShader():ComputeShaderModule("hlsl/ssr_v2/DownSampleDepth.comp.hlsl"){
		depthExtents.resize(13);
	}

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;
	virtual void SetUBO(uint8_t binding, void* data);
	virtual void* GetData(uint32_t binding, uint32_t index = 0);

private:
	Constants constant;
};

class DownSampleDepthShader2 :public ComputeShaderModule {
public:
	struct Constants {
		glm::uvec4 u_previousLevelDimensions[13];
	};

	//std::vector<VkExtent2D>	depthExtents;
public:
	DownSampleDepthShader2() :ComputeShaderModule("hlsl/ssr_v2/DownSampleDepth2.comp.hlsl") {
		//depthExtents.resize(13);
	}

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;
	virtual void SetUBO(uint8_t binding, void* data);
	virtual void* GetData(uint32_t binding, uint32_t index = 0);

private:
	Constants constant;
};


class SSR2Shader :public ShaderModule {
public:
	SSR2Shader();

	virtual size_t GetBufferSizeBinding(uint32_t binding) const;

	virtual void SetUBO(uint8_t binding, void* data);

	virtual void* GetData(uint32_t binding, uint32_t index = 0);

public:
	struct UniformBuffer0 {
		glm::mat4 viewProj;

		glm::ivec4 mipResolutions[13];

		glm::vec3 cameraPos;
		uint32_t min_traversal_occupancy;

		int GbufferWidth;
		int GbufferHeight;
		int maxMipLevel;
		uint32_t g_max_traversal_intersections;
	};

private:
	UniformBuffer0 ubo0;
};

class SSR : public MRenderApplication {
public:
	virtual void SetUp();
	virtual void ComputeAndDraw(uint32_t imageIndex);

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

private:
	std::shared_ptr<RenderPass> m_gbufferPass;
	std::shared_ptr<RenderPass> m_shadowPass;
	std::shared_ptr<RenderPass> m_lightingPass;
	std::shared_ptr<RenderPass> m_ssrPass;

	std::shared_ptr<ComputePass> m_downsampleDepthPass;
	std::shared_ptr<ComputePass> m_downsampleDepthPass2;

	MVulkanSampler				m_linearSampler;
	MVulkanSampler				m_nearestSampler;

	std::shared_ptr<Scene>		m_scene;
	std::shared_ptr<Scene>		m_squad;

	std::shared_ptr<Light>		m_directionalLight;

	std::shared_ptr<Camera>		m_directionalLightCamera;
	std::shared_ptr<Camera>		m_camera;

	//std::shared_ptr<MVulkanTexture>	m_mipmapDepthBuffer;
};

#endif // SSR_HPP