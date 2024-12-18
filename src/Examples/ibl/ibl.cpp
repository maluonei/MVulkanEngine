#include "ibl.hpp"

#include "MVulkanRHI/MVulkanEngine.hpp"
#include "scene/SceneLoader.hpp"
#include "scene/scene.hpp"
#include "Managers/TextureManager.hpp"
#include "RenderPass.hpp"
#include "Camera.hpp"
#include "scene/Light/DirectionalLight.hpp"
#include "Shaders/ShaderModule.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>

glm::vec3 directions[6] = {
    glm::vec3(1.f, 0.f, 0.f),
    glm::vec3(-1.f, 0.f, 0.f),
    glm::vec3(0.f, 1.f, 0.f),
    glm::vec3(0.f, -1.f, 0.f),
    glm::vec3(0.f, 0.f, 1.f),
    glm::vec3(0.f, 0.f, -1.f)
};

glm::vec3 ups[6] = {
    glm::vec3(0.f, -1.f, 0.f),
    glm::vec3(0.f, -1.f, 0.f),
    glm::vec3(0.f, 0.f, 1.f),
    glm::vec3(0.f, 0.f, -1.f),
    glm::vec3(0.f, -1.f, 0.f),
    glm::vec3(0.f, -1.f, 0.f)
};

void IBL::SetUp()
{
    createSkyboxTexture();
    createIrradianceCubemapTexture();

    createSamplers();
    createCamera();

    loadScene();
}

void IBL::UpdatePerFrame(uint32_t imageIndex)
{
    //prepare m_gbufferPass ubo
    {
        GbufferShader::UniformBufferObject0 ubo0{};
        ubo0.Model = glm::mat4(1.f);
        ubo0.View = m_camera->GetViewMatrix();
        ubo0.Projection = m_camera->GetProjMatrix();
        m_gbufferPass->GetShader()->SetUBO(0, &ubo0);

        GbufferShader::UniformBufferObject1 ubo1[256];

        auto meshNames = m_sphere->GetMeshNames();
        auto drawIndexedIndirectCommands = m_sphere->GetIndirectDrawCommands();

        for (auto i = 0; i < meshNames.size(); i++) {
            auto name = meshNames[i];
            auto mesh = m_sphere->GetMesh(name);
            auto mat = m_sphere->GetMaterial(mesh->matId);
            auto indirectCommand = drawIndexedIndirectCommands[i];
            if (mat->diffuseTexture != "") {
                auto diffuseTexId = Singleton<TextureManager>::instance().GetTextureId(mat->diffuseTexture);
                ubo1[indirectCommand.firstInstance].diffuseTextureIdx = diffuseTexId;
            }
            else {
                ubo1[indirectCommand.firstInstance].diffuseTextureIdx = -1;
            }

            if (mat->metallicAndRoughnessTexture != "") {
                auto metallicAndRoughnessTexId = Singleton<TextureManager>::instance().GetTextureId(mat->metallicAndRoughnessTexture);
                ubo1[indirectCommand.firstInstance].metallicAndRoughnessTexIdx = metallicAndRoughnessTexId;
            }
            else {
                ubo1[indirectCommand.firstInstance].metallicAndRoughnessTexIdx = -1;
            }

            if (i == 25) {
                ubo1[indirectCommand.firstInstance].matId = 1;
            }
            else {
                ubo1[indirectCommand.firstInstance].matId = 0;
            }
        }
        m_gbufferPass->GetShader()->SetUBO(1, &ubo1);
    }

    //prepare m_lightingPass ubo
    {
        LightingIBLShader::UniformBuffer0 ubo0{};
        ubo0.lightNum = 0;
        ubo0.cameraPos = m_camera->GetPosition();
        ubo0.ResolusionWidth = 0;
        ubo0.ResolusionHeight = 0;

        m_lightingPass->GetShader()->SetUBO(0, &ubo0);
    }

    {
        SkyboxShader::UniformBuffer0 ubo0{};
        ubo0.View = m_camera->GetViewMatrix();
        ubo0.Projection = m_camera->GetProjMatrix();

        m_skyboxPass->GetShader()->SetUBO(0, &ubo0);
    }

    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_gbufferPass, m_currentFrame, m_sphere->GetIndirectVertexBuffer(), m_sphere->GetIndirectIndexBuffer(), m_sphere->GetIndirectBuffer(), m_sphere->GetIndirectDrawCommands().size());
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(imageIndex, m_lightingPass, m_currentFrame, m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size());
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(imageIndex, m_skyboxPass, m_currentFrame, m_cube->GetIndirectVertexBuffer(), m_cube->GetIndirectIndexBuffer(), m_cube->GetIndirectBuffer(), m_cube->GetIndirectDrawCommands().size());
}

void IBL::RecreateSwapchainAndRenderPasses()
{
    if (Singleton<MVulkanEngine>::instance().RecreateSwapchain()) {
        Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_gbufferPass);

        m_lightingPass->GetRenderPassCreateInfo().depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();
        Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_lightingPass);

        m_skyboxPass->GetRenderPassCreateInfo().depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();
        Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_skyboxPass);

        std::vector<std::vector<VkImageView>> gbufferViews(7);
        for (auto i = 0; i < 4; i++) {
            gbufferViews[i].resize(1);
            gbufferViews[i][0] = m_gbufferPass->GetFrameBuffer(0).GetImageView(i);
        }
        gbufferViews[4] = std::vector<VkImageView>(1);
        gbufferViews[4][0] = m_irradianceTexture.GetImageView();
        gbufferViews[5] = std::vector<VkImageView>(1);
        gbufferViews[5][0] = m_preFilteredEnvTexture.GetImageView();
        gbufferViews[6] = std::vector<VkImageView>(1);
        gbufferViews[6][0] = m_brdfLUTPass->GetFrameBuffer(0).GetImageView(0);

        std::vector<VkSampler> samplers(1);
        samplers[0] = m_linearSampler.GetSampler();

        m_lightingPass->UpdateDescriptorSetWrite(gbufferViews, samplers);
    }
}

void IBL::CreateRenderPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();
    {
        RenderPassCreateInfo irradianceConvolusionPassInfo{};
        std::vector<VkFormat> irradianceConvolutionPassFormats(0);
        irradianceConvolutionPassFormats.push_back(VK_FORMAT_R8G8B8A8_SRGB);

        irradianceConvolusionPassInfo.extent = VkExtent2D(512, 512);
        irradianceConvolusionPassInfo.depthFormat = device.FindDepthFormat();
        irradianceConvolusionPassInfo.frambufferCount = 1;
        irradianceConvolusionPassInfo.useSwapchainImages = false;
        irradianceConvolusionPassInfo.imageAttachmentFormats = irradianceConvolutionPassFormats;
        irradianceConvolusionPassInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        irradianceConvolusionPassInfo.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        irradianceConvolusionPassInfo.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        irradianceConvolusionPassInfo.finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        irradianceConvolusionPassInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        irradianceConvolusionPassInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        irradianceConvolusionPassInfo.depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        irradianceConvolusionPassInfo.depthStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        irradianceConvolusionPassInfo.pipelineCreateInfo.depthCompareOp = VkCompareOp::VK_COMPARE_OP_LESS_OR_EQUAL;
        irradianceConvolusionPassInfo.pipelineCreateInfo.cullmode = VK_CULL_MODE_NONE;
        irradianceConvolusionPassInfo.depthView = nullptr;

        m_irradianceConvolutionPass = std::make_shared<RenderPass>(device, irradianceConvolusionPassInfo);

        std::shared_ptr<ShaderModule> irradianceConvolusionShader = std::make_shared<IrradianceConvolutionShader>();
        std::vector<std::vector<VkImageView>> irradianceConvolusionPassViews(1);
        irradianceConvolusionPassViews[0] = std::vector<VkImageView>(1);
        irradianceConvolusionPassViews[0][0] = m_skyboxTexture.GetImageView();

        std::vector<VkSampler> samplers(1, m_linearSampler.GetSampler());

        Singleton<MVulkanEngine>::instance().CreateRenderPass(m_irradianceConvolutionPass, irradianceConvolusionShader, irradianceConvolusionPassViews, samplers);
    }

    {
        m_prefilterEnvmapPasses.resize(5);
        for (auto mipLevel = 0; mipLevel < 5; mipLevel++) {
            RenderPassCreateInfo prefilterEnvmapPassInfo{};
            std::vector<VkFormat> prefilterEnvmapPassFormats(0);
            prefilterEnvmapPassFormats.push_back(VK_FORMAT_R8G8B8A8_SRGB);

            unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mipLevel));
            unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mipLevel));
            prefilterEnvmapPassInfo.extent = VkExtent2D(mipWidth, mipHeight);
            prefilterEnvmapPassInfo.depthFormat = device.FindDepthFormat();
            prefilterEnvmapPassInfo.frambufferCount = 1;
            prefilterEnvmapPassInfo.useSwapchainImages = false;
            prefilterEnvmapPassInfo.imageAttachmentFormats = prefilterEnvmapPassFormats;
            prefilterEnvmapPassInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            prefilterEnvmapPassInfo.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            prefilterEnvmapPassInfo.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            prefilterEnvmapPassInfo.finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            prefilterEnvmapPassInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            prefilterEnvmapPassInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            prefilterEnvmapPassInfo.depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            prefilterEnvmapPassInfo.depthStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
            prefilterEnvmapPassInfo.pipelineCreateInfo.depthCompareOp = VkCompareOp::VK_COMPARE_OP_LESS_OR_EQUAL;
            prefilterEnvmapPassInfo.pipelineCreateInfo.cullmode = VK_CULL_MODE_NONE;
            prefilterEnvmapPassInfo.depthView = nullptr;

            m_prefilterEnvmapPasses[mipLevel] = std::make_shared<RenderPass>(device, prefilterEnvmapPassInfo);

            std::shared_ptr<ShaderModule> prefilterEnvmapShader;
            if (mipLevel == 0) {
                prefilterEnvmapShader = std::make_shared<PreFilterEnvmapShader>();
            }
            else {
                prefilterEnvmapShader = m_prefilterEnvmapPasses[0]->GetShader();
            }
            std::vector<std::vector<VkImageView>> prefilterEnvmapPassViews(1);
            prefilterEnvmapPassViews[0] = std::vector<VkImageView>(1);
            prefilterEnvmapPassViews[0][0] = m_skyboxTexture.GetImageView();

            std::vector<VkSampler> samplers(1, m_linearSampler.GetSampler());

            Singleton<MVulkanEngine>::instance().CreateRenderPass(
                m_prefilterEnvmapPasses[mipLevel], prefilterEnvmapShader, prefilterEnvmapPassViews, samplers);
        }
    }

    {
        RenderPassCreateInfo brdfLUTPassInfo{};
        std::vector<VkFormat> brdfLUTPassFormats(0);
        brdfLUTPassFormats.push_back(VK_FORMAT_R8G8B8A8_SRGB);

        brdfLUTPassInfo.extent = VkExtent2D(512, 512);
        brdfLUTPassInfo.depthFormat = device.FindDepthFormat();
        brdfLUTPassInfo.frambufferCount = 1;
        brdfLUTPassInfo.useSwapchainImages = false;
        brdfLUTPassInfo.imageAttachmentFormats = brdfLUTPassFormats;
        brdfLUTPassInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        brdfLUTPassInfo.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        brdfLUTPassInfo.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        brdfLUTPassInfo.finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        brdfLUTPassInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        brdfLUTPassInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        brdfLUTPassInfo.depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        brdfLUTPassInfo.depthStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        brdfLUTPassInfo.pipelineCreateInfo.depthCompareOp = VkCompareOp::VK_COMPARE_OP_LESS_OR_EQUAL;
        brdfLUTPassInfo.pipelineCreateInfo.cullmode = VK_CULL_MODE_NONE;
        brdfLUTPassInfo.depthView = nullptr;

        m_brdfLUTPass = std::make_shared<RenderPass>(device, brdfLUTPassInfo);

        std::shared_ptr<ShaderModule> brdfLUTShader = std::make_shared<IBLBrdfShader>();
        std::vector<std::vector<VkImageView>> brdfLUTPassViews(0);

        std::vector<VkSampler> samplers(0);

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_brdfLUTPass, brdfLUTShader, brdfLUTPassViews, samplers);
    }

    {
        std::vector<VkFormat> gbufferFormats;
        gbufferFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        gbufferFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        gbufferFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        gbufferFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        gbufferFormats.push_back(VK_FORMAT_R32G32B32A32_UINT);

        RenderPassCreateInfo info{};
        info.depthFormat = device.FindDepthFormat();
        info.frambufferCount = 1;
        info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
        info.useAttachmentResolve = false;
        info.useSwapchainImages = false;
        info.imageAttachmentFormats = gbufferFormats;
        info.colorAttachmentResolvedViews = nullptr;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalDepthLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.pipelineCreateInfo.depthTestEnable = VK_TRUE;
        info.pipelineCreateInfo.depthWriteEnable = VK_TRUE;
        info.pipelineCreateInfo.depthCompareOp = VkCompareOp::VK_COMPARE_OP_LESS;

        m_gbufferPass = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<ShaderModule> gbufferShader = std::make_shared<GbufferShader>();
        std::vector<std::vector<VkImageView>> bufferTextureViews(1);
        auto wholeTextures = Singleton<TextureManager>::instance().GenerateTextureVector();
        auto wholeTextureSize = wholeTextures.size();
        for (auto i = 0; i < bufferTextureViews.size(); i++) {
            if (wholeTextureSize == 0) {
                bufferTextureViews[i].resize(1);
                bufferTextureViews[i][0] = Singleton<MVulkanEngine>::instance().GetPlaceHolderTexture().GetImageView();
            }
            else {
                bufferTextureViews[i].resize(wholeTextureSize);
                for (auto j = 0; j < wholeTextureSize; j++) {
                    bufferTextureViews[i][j] = wholeTextures[j]->GetImageView();
                }
            }
        }

        std::vector<VkSampler> samplers(1, m_linearSampler.GetSampler());

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_gbufferPass, gbufferShader, bufferTextureViews, samplers);
    }

    {
        std::vector<VkFormat> lightingPassFormats;
        lightingPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());

        RenderPassCreateInfo info{};
        info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
        info.depthFormat = device.FindDepthFormat();
        info.frambufferCount = Singleton<MVulkanEngine>::instance().GetSwapchainImageCount();
        info.useSwapchainImages = true;
        info.imageAttachmentFormats = lightingPassFormats;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        info.initialDepthLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
        info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
        //info.reuseDepthView = true;
        info.depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();

        m_lightingPass = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<ShaderModule> lightingShader = std::make_shared<LightingIBLShader>();
        std::vector<std::vector<VkImageView>> gbufferViews(7);
        for (auto i = 0; i < 4; i++) {
            gbufferViews[i].resize(1);
            gbufferViews[i][0] = m_gbufferPass->GetFrameBuffer(0).GetImageView(i);
        }
        gbufferViews[4] = std::vector<VkImageView>(1);
        gbufferViews[4][0] = m_irradianceTexture.GetImageView();
        gbufferViews[5] = std::vector<VkImageView>(1);
        gbufferViews[5][0] = m_preFilteredEnvTexture.GetImageView();
        gbufferViews[6] = std::vector<VkImageView>(1);
        gbufferViews[6][0] = m_brdfLUTPass->GetFrameBuffer(0).GetImageView(0);

        std::vector<VkSampler> samplers(1);
        samplers[0] = m_linearSampler.GetSampler();

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_lightingPass, lightingShader, gbufferViews, samplers);
    }

    {
        std::vector<VkFormat> skyboxPassFormats(0);
        skyboxPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());

        RenderPassCreateInfo info{};
        info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
        info.depthFormat = device.FindDepthFormat();
        info.frambufferCount = Singleton<MVulkanEngine>::instance().GetSwapchainImageCount();
        info.useSwapchainImages = true;
        info.imageAttachmentFormats = skyboxPassFormats;
        info.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        info.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        info.initialDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        info.finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        info.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        info.pipelineCreateInfo.depthTestEnable = VK_TRUE;
        info.pipelineCreateInfo.depthWriteEnable = VK_TRUE;
        info.pipelineCreateInfo.depthCompareOp = VkCompareOp::VK_COMPARE_OP_LESS_OR_EQUAL;
        info.pipelineCreateInfo.cullmode = VK_CULL_MODE_NONE;
        info.depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();

        m_skyboxPass = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<ShaderModule> skyboxShader = std::make_shared<SkyboxShader>();
        std::vector<std::vector<VkImageView>> skyboxPassViews(1);
        skyboxPassViews[0] = std::vector<VkImageView>(1);
        skyboxPassViews[0][0] = m_skyboxTexture.GetImageView();

        std::vector<VkSampler> samplers(1, m_linearSampler.GetSampler());

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_skyboxPass, skyboxShader, skyboxPassViews, samplers);
    }
}

void IBL::PreComputes()
{
    preComputeIrradianceCubemap();
    preFilterEnvmaps();
    preComputeLUT();
}

void IBL::Clean()
{
    m_irradianceConvolutionPass->Clean();
    for (auto pass : m_prefilterEnvmapPasses) {
        pass->Clean();
    }
    m_brdfLUTPass->Clean();

    m_gbufferPass->Clean();
    m_lightingPass->Clean();
    m_skyboxPass->Clean();

    m_linearSampler.Clean();
    m_skyboxTexture.Clean();
    m_irradianceTexture.Clean();
    m_preFilteredEnvTexture.Clean();
}

void IBL::loadScene()
{ 
    fs::path projectRootPath = PROJECT_ROOT;
    fs::path resourcePath = projectRootPath.append("resources").append("models");

    m_cube = std::make_shared<Scene>();
    fs::path cubePath = resourcePath / "cube.obj";
    Singleton<SceneLoader>::instance().Load(cubePath.string(), m_cube.get());

    m_sphere = std::make_shared<Scene>();
    fs::path spherePath = resourcePath / "sphere.obj";
    Singleton<SceneLoader>::instance().Load(spherePath.string(), m_sphere.get());

    m_squad = std::make_shared<Scene>();
    fs::path squadPath = resourcePath / "squad.obj";
    Singleton<SceneLoader>::instance().Load(squadPath.string(), m_squad.get());

    m_squad->GenerateIndirectDataAndBuffers();
    m_cube->GenerateIndirectDataAndBuffers();
    m_sphere->GenerateIndirectDataAndBuffers();
}

void IBL::createCamera()
{
    glm::vec3 position(-1.f, 0.f, 4.f);
    glm::vec3 center(0.f);
    glm::vec3 direction = glm::normalize(center - position);

    float fov = 60.f;
    float aspectRatio = (float)WIDTH / (float)HEIGHT;
    float zNear = 0.01f;
    float zFar = 1000.f;

    m_camera = std::make_shared<Camera>(position, direction, fov, aspectRatio, zNear, zFar);
    //spdlog::info("create m_camera");
    Singleton<MVulkanEngine>::instance().SetCamera(m_camera);
}

void IBL::createSamplers()
{
    {
        MVulkanSamplerCreateInfo info{};
        info.minFilter = VK_FILTER_LINEAR;
        info.magFilter = VK_FILTER_LINEAR;
        info.mipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        m_linearSampler.Create(Singleton<MVulkanEngine>::instance().GetDevice(), info);
    }
}

void IBL::preComputeIrradianceCubemap()
{
    auto generalGraphicList = Singleton<MVulkanEngine>::instance().GetCommandList(MQueueType::GRAPHICS);
    auto graphicsQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::GRAPHICS);

    for (int i = 0; i < 6; i++) {
        glm::vec3 position(0.f, 0.f, 0.f);

        float fov = 90.f;
        float aspectRatio = 1.;
        float zNear = 0.01f;
        float zFar = 1000.f;

        std::shared_ptr<Camera> cam = std::make_shared<Camera>(position, directions[i], ups[i], fov, aspectRatio, zNear, zFar);

        {
            IrradianceConvolutionShader::UniformBuffer0 ubo0{};
            ubo0.View = cam->GetViewMatrix();
            ubo0.Projection = cam->GetProjMatrix();

            m_irradianceConvolutionPass->GetShader()->SetUBO(0, &ubo0);
        }

        generalGraphicList.Reset();
        generalGraphicList.Begin();

        Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_irradianceConvolutionPass, m_cube->GetIndirectVertexBuffer(), m_cube->GetIndirectIndexBuffer(), m_cube->GetIndirectBuffer(), m_cube->GetIndirectDrawCommands().size(), false);
        //recordCommandBuffer(0, m_irradianceConvolutionPass, generalGraphicList, m_cube->GetIndirectVertexBuffer(), m_cube->GetIndirectIndexBuffer(), m_cube->GetIndirectBuffer(), m_cube->GetIndirectDrawCommands().size(), false);

        MVulkanImageCopyInfo copyInfo{};
        copyInfo.srcAspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyInfo.dstAspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyInfo.srcMipLevel = 0;
        copyInfo.dstMipLevel = 0;
        copyInfo.srcArrayLayer = 0;
        copyInfo.dstArrayLayer = i;
        copyInfo.layerCount = 1;

        generalGraphicList.CopyImage(m_irradianceConvolutionPass->GetFrameBuffer(0).GetImage(0), m_irradianceTexture.GetImage(), 512, 512, copyInfo);

        generalGraphicList.End();

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &generalGraphicList.GetBuffer();

        graphicsQueue.SubmitCommands(1, &submitInfo, nullptr);
        graphicsQueue.WaitForQueueComplete();
    }

    {
        generalGraphicList.Reset();
        generalGraphicList.Begin();

        MVulkanImageMemoryBarrier barrier{};
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.baseArrayLayer = 0;
        barrier.layerCount = 6;
        barrier.baseMipLevel = 0;
        barrier.levelCount = 1;
        barrier.image = m_irradianceTexture.GetImage();

        std::vector<MVulkanImageMemoryBarrier> barriers(1, barrier);
        generalGraphicList.TransitionImageLayout(barriers, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        generalGraphicList.End();

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &generalGraphicList.GetBuffer();

        graphicsQueue.SubmitCommands(1, &submitInfo, nullptr);
        graphicsQueue.WaitForQueueComplete();
    }
}

void IBL::preFilterEnvmaps()
{
    auto generalGraphicList = Singleton<MVulkanEngine>::instance().GetCommandList(MQueueType::GRAPHICS);
    auto graphicsQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::GRAPHICS);

    for (auto mipLevel = 0; mipLevel < 5; mipLevel++) {
        float roughness = mipLevel / 4.0f;
        unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mipLevel));
        unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mipLevel));

        for (int i = 0; i < 6; i++) {
            glm::vec3 position(0.f, 0.f, 0.f);

            float fov = 90.f;
            float aspectRatio = 1.;
            float zNear = 0.01f;
            float zFar = 1000.f;

            std::shared_ptr<Camera> cam = std::make_shared<Camera>(position, directions[i], ups[i], fov, aspectRatio, zNear, zFar);
            {
                PreFilterEnvmapShader::UniformBuffer0 ubo0{};
                ubo0.View = cam->GetViewMatrix();
                ubo0.Projection = cam->GetProjMatrix();

                PreFilterEnvmapShader::UniformBuffer1 ubo1{};
                ubo1.roughness = roughness;

                m_prefilterEnvmapPasses[mipLevel]->GetShader()->SetUBO(0, &ubo0);
                m_prefilterEnvmapPasses[mipLevel]->GetShader()->SetUBO(1, &ubo1);
            }

            generalGraphicList.Reset();
            generalGraphicList.Begin();

            Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_prefilterEnvmapPasses[mipLevel], m_cube->GetIndirectVertexBuffer(), m_cube->GetIndirectIndexBuffer(), m_cube->GetIndirectBuffer(), m_cube->GetIndirectDrawCommands().size(), false);

            MVulkanImageCopyInfo copyInfo{};
            copyInfo.srcAspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyInfo.dstAspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyInfo.srcMipLevel = 0;
            copyInfo.dstMipLevel = mipLevel;
            copyInfo.srcArrayLayer = 0;
            copyInfo.dstArrayLayer = i;
            copyInfo.layerCount = 1;

            generalGraphicList.CopyImage(m_prefilterEnvmapPasses[mipLevel]->GetFrameBuffer(0).GetImage(0), m_preFilteredEnvTexture.GetImage(), mipWidth, mipHeight, copyInfo);

            generalGraphicList.End();

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &generalGraphicList.GetBuffer();

            graphicsQueue.SubmitCommands(1, &submitInfo, nullptr);
            graphicsQueue.WaitForQueueComplete();
        }
    }

    {
        generalGraphicList.Reset();
        generalGraphicList.Begin();

        MVulkanImageMemoryBarrier barrier{};
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.baseArrayLayer = 0;
        barrier.layerCount = 6;
        barrier.baseMipLevel = 0;
        barrier.levelCount = 5;
        barrier.image = m_preFilteredEnvTexture.GetImage();

        std::vector<MVulkanImageMemoryBarrier> barriers(1, barrier);
        generalGraphicList.TransitionImageLayout(barriers, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        generalGraphicList.End();

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &generalGraphicList.GetBuffer();

        graphicsQueue.SubmitCommands(1, &submitInfo, nullptr);
        graphicsQueue.WaitForQueueComplete();
    }
}

void IBL::preComputeLUT()
{
    auto generalGraphicList = Singleton<MVulkanEngine>::instance().GetCommandList(MQueueType::GRAPHICS);
    auto graphicsQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::GRAPHICS);

    generalGraphicList.Reset();
    generalGraphicList.Begin();

    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_brdfLUTPass, m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size(), true);

    generalGraphicList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &generalGraphicList.GetBuffer();

    graphicsQueue.SubmitCommands(1, &submitInfo, nullptr);
    graphicsQueue.WaitForQueueComplete();
}

void IBL::createSkyboxTexture()
{
    auto generalGraphicList = Singleton<MVulkanEngine>::instance().GetCommandList(MQueueType::GRAPHICS);
    auto graphicsQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::GRAPHICS);

    fs::path projectRootPath = PROJECT_ROOT;
    fs::path resourcePath = projectRootPath.append("resources").append("textures").append("noon_grass");

    std::vector<MImage<unsigned char>*> images(6);

    std::vector<std::shared_ptr<MImage<unsigned char>>> imagesPtrs(6);
    for (auto i = 0; i < 6; i++) {
        imagesPtrs[i] = std::make_shared<MImage<unsigned char>>();

        fs::path imagePath = resourcePath / (std::to_string(i) + ".png");
        imagesPtrs[i]->Load(imagePath);

        images[i] = imagesPtrs[i].get();
    }

    ImageCreateInfo imageinfo{};
    imageinfo.width = imagesPtrs[0]->Width();
    imageinfo.height = imagesPtrs[0]->Height();
    imageinfo.format = imagesPtrs[0]->Format();
    imageinfo.mipLevels = imagesPtrs[0]->MipLevels();
    imageinfo.arrayLength = 6;
    imageinfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    imageinfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageinfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageinfo.flag = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    ImageViewCreateInfo viewInfo{};
    viewInfo.format = imagesPtrs[0]->Format();
    viewInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.flag = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.levelCount = 1;
    viewInfo.layerCount = 6;

    generalGraphicList.Reset();
    generalGraphicList.Begin();

    m_skyboxTexture.CreateAndLoadData(&generalGraphicList, Singleton<MVulkanEngine>::instance().GetDevice(), imageinfo, viewInfo, images);
    generalGraphicList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &generalGraphicList.GetBuffer();

    graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    graphicsQueue.WaitForQueueComplete();
}

void IBL::createIrradianceCubemapTexture()
{
    auto generalGraphicList = Singleton<MVulkanEngine>::instance().GetCommandList(MQueueType::GRAPHICS);
    auto graphicsQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::GRAPHICS);

    {
        ImageCreateInfo imageinfo{};
        imageinfo.width = 512;
        imageinfo.height = 512;
        imageinfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageinfo.mipLevels = 1;
        imageinfo.arrayLength = 6;
        imageinfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageinfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageinfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageinfo.flag = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        ImageViewCreateInfo viewInfo{};
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.flag = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
        //viewInfo.levelCount = m_image->MipLevels();
        viewInfo.levelCount = 1;
        viewInfo.layerCount = 6;

        generalGraphicList.Reset();
        generalGraphicList.Begin();

        m_irradianceTexture.Create(&generalGraphicList, Singleton<MVulkanEngine>::instance().GetDevice(), imageinfo, viewInfo, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        generalGraphicList.End();

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &generalGraphicList.GetBuffer();

        graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
        graphicsQueue.WaitForQueueComplete();
    }

    {
        ImageCreateInfo imageinfo{};
        imageinfo.width = 128;
        imageinfo.height = 128;
        imageinfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageinfo.mipLevels = 5;
        imageinfo.arrayLength = 6;
        imageinfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageinfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageinfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageinfo.flag = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        ImageViewCreateInfo viewInfo{};
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.flag = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.levelCount = 5;
        viewInfo.layerCount = 6;

        generalGraphicList.Reset();
        generalGraphicList.Begin();

        m_preFilteredEnvTexture.Create(&generalGraphicList, Singleton<MVulkanEngine>::instance().GetDevice(), imageinfo, viewInfo, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        generalGraphicList.End();

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &generalGraphicList.GetBuffer();

        graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
        graphicsQueue.WaitForQueueComplete();
    }
}
