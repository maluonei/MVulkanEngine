#include "ssr.hpp"

#include "MVulkanRHI/MVulkanEngine.hpp"
#include "Scene/SceneLoader.hpp"
#include "Scene/Scene.hpp"
#include "Managers/TextureManager.hpp"
#include "RenderPass.hpp"
#include "ComputePass.hpp"
#include "Camera.hpp"
#include "Scene/Light/DirectionalLight.hpp"
#include "Shaders/ShaderModule.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>


void SSR::SetUp()
{
    createSamplers();
    createLight();
    createCamera();

    loadScene();
}

void SSR::ComputeAndDraw(uint32_t imageIndex)
{
    auto graphicsList = Singleton<MVulkanEngine>::instance().GetGraphicsList(m_currentFrame);
    auto graphicsQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::GRAPHICS);

    graphicsList.Reset();
    graphicsList.Begin();

    //prepare gbufferPass ubo
    {
        GbufferShader::UniformBufferObject0 ubo0{};
        ubo0.Model = glm::mat4(1.f);
        ubo0.View = m_camera->GetViewMatrix();
        ubo0.Projection = m_camera->GetProjMatrix();
        GbufferShader::UniformBufferObject1 ubo1 = GbufferShader::GetFromScene(m_scene);

        //GbufferShader::UniformBufferObject1 ubo1[256];
        //
        //auto meshNames = m_scene->GetMeshNames();
        //auto drawIndexedIndirectCommands = m_scene->GetIndirectDrawCommands();
        //
        //for (auto i = 0; i < meshNames.size(); i++) {
        //    auto name = meshNames[i];
        //    auto mesh = m_scene->GetMesh(name);
        //    auto mat = m_scene->GetMaterial(mesh->matId);
        //    auto indirectCommand = drawIndexedIndirectCommands[i];
        //    if (mat->diffuseTexture != "") {
        //        auto diffuseTexId = Singleton<TextureManager>::instance().GetTextureId(mat->diffuseTexture);
        //        ubo1[indirectCommand.firstInstance].diffuseTextureIdx = diffuseTexId;
        //    }
        //    else {
        //        ubo1[indirectCommand.firstInstance].diffuseTextureIdx = -1;
        //    }
        //
        //    if (mat->metallicAndRoughnessTexture != "") {
        //        auto metallicAndRoughnessTexId = Singleton<TextureManager>::instance().GetTextureId(mat->metallicAndRoughnessTexture);
        //        ubo1[indirectCommand.firstInstance].metallicAndRoughnessTexIdx = metallicAndRoughnessTexId;
        //    }
        //    else {
        //        ubo1[indirectCommand.firstInstance].metallicAndRoughnessTexIdx = -1;
        //    }
        //
        //    if (i == 12) {
        //        ubo1[indirectCommand.firstInstance].matId = 1;
        //    }
        //    else {
        //        ubo1[indirectCommand.firstInstance].matId = 0;
        //    }
        //}
        m_gbufferPass->GetShader()->SetUBO(1, &ubo1);
    }

    //prepare shadowPass ubo
    {
        ShadowShader::ShadowUniformBuffer ubo0{};
        ubo0.shadowMVP = m_directionalLightCamera->GetOrthoMatrix() * m_directionalLightCamera->GetViewMatrix();
        m_shadowPass->GetShader()->SetUBO(0, &ubo0);
    }

    //prepare lightingPass ubo
    {
        LightingPbrSSRShader::UniformBuffer0 ubo0{};
        ubo0.lightNum = 1;
        ubo0.lights[0].direction = m_directionalLightCamera->GetDirection();
        ubo0.lights[0].intensity = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetIntensity();
        ubo0.lights[0].color = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetColor();
        ubo0.lights[0].shadowMapIndex = 0;
        ubo0.lights[0].shadowViewProj = m_directionalLightCamera->GetOrthoMatrix() * m_directionalLightCamera->GetViewMatrix();
        ubo0.lights[0].cameraZnear = m_directionalLightCamera->GetZnear();
        ubo0.lights[0].cameraZfar = m_directionalLightCamera->GetZfar();
        ubo0.cameraPos = m_camera->GetPosition();

        ubo0.ResolusionWidth = m_shadowPass->GetFrameBuffer(0).GetExtent2D().width;
        ubo0.ResolusionHeight = m_shadowPass->GetFrameBuffer(0).GetExtent2D().height;

        m_lightingPass->GetShader()->SetUBO(0, &ubo0);
    }

    //prepare SSR ubo
    {
        uint32_t maxMipLevel = CalculateMipLevels(m_gbufferPass->GetFrameBuffer(0).GetExtent2D());

        SSR2Shader::UniformBuffer0 ubo0{};
        ubo0.viewProj = m_camera->GetProjMatrix() * m_camera->GetViewMatrix();
        ubo0.cameraPos = m_camera->GetPosition();

        ubo0.min_traversal_occupancy = 50;
        ubo0.GbufferWidth = m_gbufferPass->GetFrameBuffer(0).GetExtent2D().width;
        ubo0.GbufferHeight = m_gbufferPass->GetFrameBuffer(0).GetExtent2D().height;
        ubo0.maxMipLevel = maxMipLevel - 1;
        ubo0.g_max_traversal_intersections = 60;

        uint32_t width = ubo0.GbufferWidth;
        uint32_t height = ubo0.GbufferHeight;

        for (auto i = 0; i < maxMipLevel; i++) {
            ubo0.mipResolutions[i] = glm::ivec4(int(width), int(height), 0, 0);
            width = width / 2;
            height = height / 2;
        }

        m_ssrPass->GetShader()->SetUBO(0, &ubo0);
    }

    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_shadowPass, m_currentFrame, m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), m_scene->GetIndirectBuffer(), m_scene->GetIndirectDrawCommands().size());
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_gbufferPass, m_currentFrame, m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), m_scene->GetIndirectBuffer(), m_scene->GetIndirectDrawCommands().size());

    
    graphicsList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &graphicsList.GetBuffer();

    graphicsQueue.SubmitCommands(1, &submitInfo, nullptr);
    graphicsQueue.WaitForQueueComplete();

#ifdef USE_SPD
    {
        auto computeQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::COMPUTE);
        auto computeList = Singleton<MVulkanEngine>::instance().GetComputeCommandList();

        auto maxMipLevels = CalculateMipLevels(m_gbufferPass->GetFrameBuffer(0).GetExtent2D());
        auto extent = m_gbufferPass->GetFrameBuffer(0).GetExtent2D();
        auto width = extent.width;
        auto height = extent.height;

        DownSampleDepthShader2::Constants constant{};

        constant.u_previousLevelDimensions[0].w = maxMipLevels;
        for (auto j = 0; j < maxMipLevels; j++) {
            constant.u_previousLevelDimensions[j].x = width;
            constant.u_previousLevelDimensions[j].y = height;
            constant.u_previousLevelDimensions[j].z = width * height;

            width /= 2;
            height /= 2;
        }

        m_downsampleDepthPass2->GetShader()->SetUBO(0, &constant);

        computeList.Reset();
        computeList.Begin();

        int dispatchX = (extent.width + 15) / 16;
        int dispatchY = (extent.height + 15) / 16;
        int dispatchZ = 1;
        Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_downsampleDepthPass2, dispatchX, dispatchY, dispatchZ);

        computeList.End();
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &computeList.GetBuffer();

        computeQueue.SubmitCommands(1, &submitInfo, nullptr);
        computeQueue.WaitForQueueComplete();
    }
#else
   //do downsample depth
   {
       auto computeQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::COMPUTE);
       auto computeList = Singleton<MVulkanEngine>::instance().GetComputeCommandList();

       auto maxMipLevels = CalculateMipLevels(m_gbufferPass->GetFrameBuffer(0).GetExtent2D());
       auto extent = m_gbufferPass->GetFrameBuffer(0).GetExtent2D();
       auto width = extent.width * 2;
       auto height = extent.height * 2;
       for (auto i = 0; i < maxMipLevels; i++) {
           DownSampleDepthShader::Constants constant{};
           constant.u_previousLevel = i - 1;

           constant.u_previousLevelDimensionsWidth = width;
           constant.u_previousLevelDimensionsHeight = height;

           m_downsampleDepthPass->GetShader()->SetUBO(0, &constant);

           computeList.Reset();
           computeList.Begin();

           int dispatchX = (width + 15) / 16;
           int dispatchY = (height + 15) / 16;
           int dispatchZ = 1;
           Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_downsampleDepthPass, dispatchX, dispatchY, dispatchZ);

           computeList.End();
           VkSubmitInfo submitInfo{};
           submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

           submitInfo.commandBufferCount = 1;
           submitInfo.pCommandBuffers = &computeList.GetBuffer();

           computeQueue.SubmitCommands(1, &submitInfo, nullptr);
           computeQueue.WaitForQueueComplete();

           width = int(width / 2);
           height = int(height / 2);
       }
   }
#endif

    graphicsList.Reset();
    graphicsList.Begin();
   

    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_lightingPass, m_currentFrame, m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size());
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(imageIndex, m_ssrPass, m_currentFrame, m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size());

    graphicsList.End();
    Singleton<MVulkanEngine>::instance().SubmitGraphicsCommands(imageIndex, m_currentFrame);
}

void SSR::RecreateSwapchainAndRenderPasses()
{
    if (Singleton<MVulkanEngine>::instance().RecreateSwapchain()) {
        Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_gbufferPass);
        Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_shadowPass);
        Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_ssrPass);

        {
            m_lightingPass->GetRenderPassCreateInfo().depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();
            Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_lightingPass);

            std::vector<std::vector<VkImageView>> gbufferViews(6);
            for (auto i = 0; i < 5; i++) {
                gbufferViews[i].resize(1);
                gbufferViews[i][0] = m_gbufferPass->GetFrameBuffer(0).GetImageView(i);
            }
            gbufferViews[5] = std::vector<VkImageView>(2);
            gbufferViews[5][0] = m_shadowPass->GetFrameBuffer(0).GetDepthImageView();
            gbufferViews[5][1] = m_shadowPass->GetFrameBuffer(0).GetDepthImageView();

            std::vector<VkSampler> samplers(1);
            samplers[0] = m_linearSampler.GetSampler();

            m_lightingPass->UpdateDescriptorSetWrite(gbufferViews, samplers);
        }


        {
            std::vector<uint32_t> storageBufferSizes(0);

            auto maxMipLevels = CalculateMipLevels(m_gbufferPass->GetFrameBuffer(0).GetExtent2D());

            uint32_t width = m_gbufferPass->GetFrameBuffer(0).GetExtent2D().width;
            uint32_t height = m_gbufferPass->GetFrameBuffer(0).GetExtent2D().height;

            std::vector<std::vector<StorageImageCreateInfo>> storageImageCreateInfos(1);
            storageImageCreateInfos[0].resize(maxMipLevels);

#ifdef USE_SPD
            auto downSampleDepthShader = m_downsampleDepthPass2->GetShader();
#else
            auto downSampleDepthShader = m_downsampleDepthPass->GetShader();
#endif

            for (int i = 0; i < maxMipLevels; i++) {
                std::static_pointer_cast<DownSampleDepthShader>(downSampleDepthShader)->depthExtents[i].width = width;
                std::static_pointer_cast<DownSampleDepthShader>(downSampleDepthShader)->depthExtents[i].height = height;

                storageImageCreateInfos[0][i].width = width;
                storageImageCreateInfos[0][i].height = height;
                storageImageCreateInfos[0][i].depth = 1;
                storageImageCreateInfos[0][i].format = VK_FORMAT_R32_SFLOAT;

                width = int(width / 2);
                height = int(height / 2);
            }

            std::vector<VkSampler> samplers(0);

            std::vector<std::vector<VkImageView>> seperateImageViews(1);
            seperateImageViews[0].resize(1, m_gbufferPass->GetFrameBuffer(0).GetDepthImageView());
            //seperateImageViews[0][0] = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();

#ifdef USE_SPD
            m_downsampleDepthPass2->RecreateStorageImages(storageImageCreateInfos);
            m_downsampleDepthPass2->UpdateDescriptorSetWrite(seperateImageViews, samplers);
#else
            m_downsampleDepthPass->RecreateStorageImages(storageImageCreateInfos);
            m_downsampleDepthPass->UpdateDescriptorSetWrite(seperateImageViews, samplers);
#endif
        }

        {
            Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_ssrPass);

            std::vector<std::vector<VkImageView>> ssrViews(5);
            ssrViews[0] = std::vector<VkImageView>(1, m_gbufferPass->GetFrameBuffer(0).GetImageView(0));
            ssrViews[1] = std::vector<VkImageView>(1, m_gbufferPass->GetFrameBuffer(0).GetImageView(1));
            ssrViews[2] = std::vector<VkImageView>(1, m_gbufferPass->GetFrameBuffer(0).GetImageView(4));
            ssrViews[3] = std::vector<VkImageView>(1, m_lightingPass->GetFrameBuffer(0).GetImageView(0));

            auto maxMipLevels = CalculateMipLevels(m_gbufferPass->GetFrameBuffer(0).GetExtent2D());
            ssrViews[4] = std::vector<VkImageView>(maxMipLevels);
            for (auto i = 0; i < maxMipLevels; i++) {
#ifdef USE_SPD
                ssrViews[4][i] = m_downsampleDepthPass2->GetStorageImageViewByBinding(0, i);
#else
                ssrViews[4][i] = m_downsampleDepthPass->GetStorageImageViewByBinding(0, i);
#endif
            }

            std::vector<VkSampler> samplers(2);
            samplers[0] = m_linearSampler.GetSampler();
            samplers[1] = m_nearestSampler.GetSampler();

            m_ssrPass->UpdateDescriptorSetWrite(ssrViews, samplers);
        }
    }
}

void SSR::CreateRenderPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        std::vector<VkFormat> gbufferFormats;
        gbufferFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        gbufferFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        gbufferFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        gbufferFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        //gbufferFormats.push_back(VK_FORMAT_R32G32B32A32_UINT);

        RenderPassCreateInfo info{};
        info.depthFormat = device.FindDepthFormat();
        //info.generateDepthMipmap = true;
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
        std::vector<VkFormat> shadowFormats(0);
        shadowFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);

        RenderPassCreateInfo info{};
        info.depthFormat = VK_FORMAT_D32_SFLOAT;
        info.frambufferCount = 1;
        info.extent = VkExtent2D(4096, 4096);
        info.useAttachmentResolve = false;
        info.useSwapchainImages = false;
        info.imageAttachmentFormats = shadowFormats;
        info.colorAttachmentResolvedViews = nullptr;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalDepthLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        m_shadowPass = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<ShaderModule> shadowShader = std::make_shared<ShadowShader>();
        std::vector<std::vector<VkImageView>> shadowShaderTextures(1);
        shadowShaderTextures[0].resize(0);

        std::vector<VkSampler> samplers(0);

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_shadowPass, shadowShader, shadowShaderTextures, samplers);
    }

    {
        std::vector<VkFormat> lightingPassFormats;
        lightingPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());

        RenderPassCreateInfo info{};
        info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
        info.depthFormat = device.FindDepthFormat();
        info.frambufferCount = 1;
        info.useSwapchainImages = false;
        info.imageAttachmentFormats = lightingPassFormats;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.initialDepthLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.finalDepthLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
        info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
        info.depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();

        m_lightingPass = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<ShaderModule> lightingShader = std::make_shared<LightingPbrSSRShader>();
        std::vector<std::vector<VkImageView>> gbufferViews(6);
        for (auto i = 0; i < 5; i++) {
            gbufferViews[i].resize(1);
            gbufferViews[i][0] = m_gbufferPass->GetFrameBuffer(0).GetImageView(i);
        }
        gbufferViews[5] = std::vector<VkImageView>(2);
        gbufferViews[5][0] = m_shadowPass->GetFrameBuffer(0).GetDepthImageView();
        gbufferViews[5][1] = m_shadowPass->GetFrameBuffer(0).GetDepthImageView();

        std::vector<VkSampler> samplers(1);
        samplers[0] = m_linearSampler.GetSampler();

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_lightingPass, lightingShader, gbufferViews, samplers);
    }

#ifndef USE_SPD
    {
        std::shared_ptr<ComputeShaderModule> downSampleDepthShader = std::make_shared<DownSampleDepthShader>();
        m_downsampleDepthPass = std::make_shared<ComputePass>(device);

        std::vector<uint32_t> storageBufferSizes(0);

        auto maxMipLevels = CalculateMipLevels(m_gbufferPass->GetFrameBuffer(0).GetExtent2D());

        uint32_t width = m_gbufferPass->GetFrameBuffer(0).GetExtent2D().width;
        uint32_t height = m_gbufferPass->GetFrameBuffer(0).GetExtent2D().height;

        std::vector<std::vector<StorageImageCreateInfo>> storageImageCreateInfos(1);
        storageImageCreateInfos[0].resize(maxMipLevels);

        for (int i = 0; i < maxMipLevels; i++) {
            std::static_pointer_cast<DownSampleDepthShader>(downSampleDepthShader)->depthExtents[i].width = width;
            std::static_pointer_cast<DownSampleDepthShader>(downSampleDepthShader)->depthExtents[i].height = height;

            storageImageCreateInfos[0][i].width = width;
            storageImageCreateInfos[0][i].height = height;
            storageImageCreateInfos[0][i].depth = 1;
            storageImageCreateInfos[0][i].format = VK_FORMAT_R32_SFLOAT;

            width = int(width / 2);
            height = int(height / 2);
        }

        std::vector<VkSampler> samplers(0);

        std::vector<std::vector<VkImageView>> seperateImageViews(1);
        seperateImageViews[0].resize(1, m_gbufferPass->GetFrameBuffer(0).GetDepthImageView());

        Singleton<MVulkanEngine>::instance().CreateComputePass(m_downsampleDepthPass, downSampleDepthShader,
            storageBufferSizes, storageImageCreateInfos, seperateImageViews, samplers);
    }
#else
    {
        std::shared_ptr<ComputeShaderModule> downSampleDepthShader = std::make_shared<DownSampleDepthShader2>();
        m_downsampleDepthPass2 = std::make_shared<ComputePass>(device);

        std::vector<uint32_t> storageBufferSizes(0);

        auto maxMipLevels = CalculateMipLevels(m_gbufferPass->GetFrameBuffer(0).GetExtent2D());

        uint32_t width = m_gbufferPass->GetFrameBuffer(0).GetExtent2D().width;
        uint32_t height = m_gbufferPass->GetFrameBuffer(0).GetExtent2D().height;

        std::vector<std::vector<StorageImageCreateInfo>> storageImageCreateInfos(1);
        storageImageCreateInfos[0].resize(maxMipLevels);

        for (int i = 0; i < maxMipLevels; i++) {
            storageImageCreateInfos[0][i].width = width;
            storageImageCreateInfos[0][i].height = height;
            storageImageCreateInfos[0][i].depth = 1;
            storageImageCreateInfos[0][i].format = VK_FORMAT_R32_SFLOAT;

            width = int(width / 2);
            height = int(height / 2);
        }

        std::vector<VkSampler> samplers(0);

        std::vector<std::vector<VkImageView>> seperateImageViews(1);
        seperateImageViews[0].resize(1, m_gbufferPass->GetFrameBuffer(0).GetDepthImageView());

        Singleton<MVulkanEngine>::instance().CreateComputePass(m_downsampleDepthPass2, downSampleDepthShader,
            storageBufferSizes, storageImageCreateInfos, seperateImageViews, samplers);
    }
#endif


    {
        std::vector<VkFormat> SSRPassFormats;
        SSRPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());

        RenderPassCreateInfo info{};
        info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
        info.depthFormat = device.FindDepthFormat();
        info.frambufferCount = Singleton<MVulkanEngine>::instance().GetSwapchainImageCount();
        info.useSwapchainImages = true;
        info.imageAttachmentFormats = SSRPassFormats;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        info.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

        m_ssrPass = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<ShaderModule> ssrShader = std::make_shared<SSR2Shader>();
        std::vector<std::vector<VkImageView>> ssrViews(5);
        ssrViews[0] = std::vector<VkImageView>(1, m_gbufferPass->GetFrameBuffer(0).GetImageView(0));
        ssrViews[1] = std::vector<VkImageView>(1, m_gbufferPass->GetFrameBuffer(0).GetImageView(1));
        ssrViews[2] = std::vector<VkImageView>(1, m_gbufferPass->GetFrameBuffer(0).GetImageView(4));
        ssrViews[3] = std::vector<VkImageView>(1, m_lightingPass->GetFrameBuffer(0).GetImageView(0));
        
        auto maxMipLevels = CalculateMipLevels(m_gbufferPass->GetFrameBuffer(0).GetExtent2D());
        ssrViews[4] = std::vector<VkImageView>(maxMipLevels);
        for (auto i = 0; i < maxMipLevels; i++) {
#ifdef USE_SPD
            ssrViews[4][i] = m_downsampleDepthPass2->GetStorageImageViewByBinding(0, i);
#else
            ssrViews[4][i] = m_downsampleDepthPass->GetStorageImageViewByBinding(0, i);
#endif
        }

        std::vector<VkSampler> samplers(2);
        samplers[0] = m_linearSampler.GetSampler();
        samplers[1] = m_nearestSampler.GetSampler();

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_ssrPass, ssrShader, ssrViews, samplers);
    }

    createLightCamera();
}

void SSR::PreComputes()
{

}

void SSR::Clean()
{
    m_gbufferPass->Clean();
    m_lightingPass->Clean();
    m_shadowPass->Clean();
    m_ssrPass->Clean();

#ifdef USE_SPD
    m_downsampleDepthPass2->Clean();
#else
    m_downsampleDepthPass->Clean();
#endif

    m_linearSampler.Clean();
    m_nearestSampler.Clean();

    m_squad->Clean();
    m_scene->Clean();

    MRenderApplication::Clean();
}

void SSR::loadScene()
{
    m_scene = std::make_shared<Scene>();

    fs::path projectRootPath = PROJECT_ROOT;
    fs::path resourcePath = projectRootPath.append("resources").append("models");
    fs::path modelPath = resourcePath / "Sponza" / "glTF" / "Sponza.gltf";

    Singleton<SceneLoader>::instance().Load(modelPath.string(), m_scene.get());

    //split Image
    {
        auto wholeTextures = Singleton<TextureManager>::instance().GenerateTextureVector();

        auto& transferList = Singleton<MVulkanEngine>::instance().GetCommandList(MQueueType::TRANSFER);

        transferList.Reset();
        transferList.Begin();
        std::vector<MVulkanImageMemoryBarrier> barriers(wholeTextures.size());
        for (auto i = 0; i < wholeTextures.size(); i++) {
            MVulkanImageMemoryBarrier barrier{};
            barrier.image = wholeTextures[i]->GetImage();

            barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.levelCount = wholeTextures[i]->GetImageInfo().mipLevels;

            barriers[i] = barrier;
        }

        transferList.TransitionImageLayout(barriers, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

        transferList.End();

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &transferList.GetBuffer();

        auto& transferQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::TRANSFER);

        transferQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
        transferQueue.WaitForQueueComplete();

        for (auto item : wholeTextures) {
            auto texture = item;
            Singleton<MVulkanEngine>::instance().GenerateMipMap(*texture);
        }
    }

    m_squad = std::make_shared<Scene>();
    fs::path squadPath = resourcePath / "squad.obj";
    Singleton<SceneLoader>::instance().Load(squadPath.string(), m_squad.get());

    m_squad->GenerateIndirectDataAndBuffers();
    m_scene->GenerateIndirectDataAndBuffers();
}

void SSR::createLight()
{
    glm::vec3 direction = glm::normalize(glm::vec3(-1.f, -6.f, -1.f));
    glm::vec3 color = glm::vec3(1.f, 1.f, 1.f);
    float intensity = 20.f;
    m_directionalLight = std::make_shared<DirectionalLight>(direction, color, intensity);
}

void SSR::createCamera()
{
    glm::vec3 position(-1.f, 0.f, 4.f);
    glm::vec3 center(0.f);
    glm::vec3 direction = glm::normalize(center - position);

    float fov = 60.f;
    float aspectRatio = (float)WIDTH / (float)HEIGHT;
    float zNear = 0.01f;
    float zFar = 100.f;

    m_camera = std::make_shared<Camera>(position, direction, fov, aspectRatio, zNear, zFar);
    Singleton<MVulkanEngine>::instance().SetCamera(m_camera);
}

void SSR::createSamplers()
{
    {
        MVulkanSamplerCreateInfo info{};
        info.minFilter = VK_FILTER_LINEAR;
        info.magFilter = VK_FILTER_LINEAR;
        info.mipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        m_linearSampler.Create(Singleton<MVulkanEngine>::instance().GetDevice(), info);
    }

    {
        MVulkanSamplerCreateInfo info{};
        info.minFilter = VK_FILTER_NEAREST;
        info.magFilter = VK_FILTER_NEAREST;
        info.mipMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        m_nearestSampler.Create(Singleton<MVulkanEngine>::instance().GetDevice(), info);
    }
}

void SSR::createLightCamera()
{
    VkExtent2D extent = m_shadowPass->GetFrameBuffer(0).GetExtent2D();

    glm::vec3 direction = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetDirection();
    glm::vec3 position = -50.f * direction;

    float fov = 60.f;
    float aspect = (float)extent.width / (float)extent.height;
    float zNear = 0.001f;
    float zFar = 60.f;
    m_directionalLightCamera = std::make_shared<Camera>(position, direction, fov, aspect, zNear, zFar);
    m_directionalLightCamera->SetOrth(true);
}

size_t DownSampleDepthShader::GetBufferSizeBinding(uint32_t binding) const
{
    return sizeof(constant);
}

void DownSampleDepthShader::SetUBO(uint8_t index, void* data)
{
    switch (index) {
    case 0:
        constant = *reinterpret_cast<DownSampleDepthShader::Constants*>(data);
        return;
    }
}

void* DownSampleDepthShader::GetData(uint32_t binding, uint32_t index)
{
    return (void*)&constant;
}



SSR2Shader::SSR2Shader() :ShaderModule("hlsl/lighting_pbr.vert.hlsl", "hlsl/ssr2.frag.hlsl")
{

}

size_t SSR2Shader::GetBufferSizeBinding(uint32_t binding) const
{
    switch (binding) {
    case 0:
        return sizeof(SSR2Shader::UniformBuffer0);
    }
}

void SSR2Shader::SetUBO(uint8_t binding, void* data)
{
    switch (binding) {
    case 0:
        ubo0 = *reinterpret_cast<SSR2Shader::UniformBuffer0*>(data);
        return;
    }
}

void* SSR2Shader::GetData(uint32_t binding, uint32_t index)
{
    switch (binding) {
    case 0:
        return (void*)&ubo0;
    }
}


size_t DownSampleDepthShader2::GetBufferSizeBinding(uint32_t binding) const
{
    return sizeof(constant);
}

void DownSampleDepthShader2::SetUBO(uint8_t binding, void* data)
{
    switch (binding) {
    case 0:
        constant = *reinterpret_cast<DownSampleDepthShader2::Constants*>(data);
        return;
    }
}

void* DownSampleDepthShader2::GetData(uint32_t binding, uint32_t index)
{
    return (void*)&constant;
}