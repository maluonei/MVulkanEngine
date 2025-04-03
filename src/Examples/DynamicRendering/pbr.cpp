#include "pbr.hpp"

#include "MVulkanRHI/MVulkanEngine.hpp"
#include "Scene/SceneLoader.hpp"
#include "Scene/Scene.hpp"
#include "Managers/TextureManager.hpp"
#include "RenderPass.hpp"
#include "Camera.hpp"
#include "Scene/Light/DirectionalLight.hpp"
#include "Shaders/ShaderModule.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>


void PBR::SetUp()
{
    createSamplers();
    createLight();
    createCamera();
    createTextures();

    loadScene();
}

void PBR::ComputeAndDraw(uint32_t imageIndex)
{
    auto graphicsList = Singleton<MVulkanEngine>::instance().GetGraphicsList(m_currentFrame);
    auto graphicsQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::GRAPHICS);

    //ImageLayoutToAttachment(imageIndex);

    graphicsList.Reset();
    graphicsList.Begin();

    //auto shadowmapExtent = shadowmapExtent

    //prepare gbufferPass ubo
    {
        GbufferShader2::UniformBufferObject0 ubo0{};
        ubo0.Model = glm::mat4(1.f);
        ubo0.View = m_camera->GetViewMatrix();
        ubo0.Projection = m_camera->GetProjMatrix();
        m_gbufferPass->GetShader()->SetUBO(0, &ubo0);

        GbufferShader2::UniformBufferObject1 ubo1 = GbufferShader2::GetFromScene(m_scene);
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
        LightingPbrShader2::UniformBuffer0 ubo0{};
        ubo0.lightNum = 1;
        ubo0.lights[0].direction = m_directionalLightCamera->GetDirection();
        ubo0.lights[0].intensity = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetIntensity();
        ubo0.lights[0].color = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetColor();
        ubo0.lights[0].shadowMapIndex = 0;
        ubo0.lights[0].shadowViewProj = m_directionalLightCamera->GetOrthoMatrix() * m_directionalLightCamera->GetViewMatrix();
        ubo0.lights[0].cameraZnear = m_directionalLightCamera->GetZnear();
        ubo0.lights[0].cameraZfar = m_directionalLightCamera->GetZfar();
        ubo0.cameraPos = m_camera->GetPosition();

        auto swapChainExtent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();

        ubo0.ResolusionWidth = shadowmapExtent.width;
        ubo0.ResolusionHeight = shadowmapExtent.height;
        ubo0.WindowResWidth = swapChainExtent.width;
        ubo0.WindowResHeight = swapChainExtent.height;

        m_lightingPass->GetShader()->SetUBO(0, &ubo0);
    }

    RenderingInfo gbufferRenderInfo, shadowmapRenderInfo, shadingRenderInfo; 
    {
        gbufferRenderInfo.colorAttachments.push_back(
            RenderingAttachment{
                .view = gBuffer0->GetImageView(),
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            }
        );
        gbufferRenderInfo.colorAttachments.push_back(
            RenderingAttachment{
                .view = gBuffer1->GetImageView(),
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            }
        );
        gbufferRenderInfo.depthAttachment = RenderingAttachment{
                .view = gBufferDepth->GetImageView(),
                .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };
    }

    {
        shadowmapRenderInfo.colorAttachments.push_back(
            RenderingAttachment{
                .view = shadowMap->GetImageView(),
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            }
        );
        shadowmapRenderInfo.depthAttachment = RenderingAttachment{
                .view = shadowMapDepth->GetImageView(),
                .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };
    }

    {
        //auto swapChainImages = Singleton<MVulkanEngine>::instance().GetSwa
        auto swapChain = Singleton<MVulkanEngine>::instance().GetSwapchain();

        shadingRenderInfo.colorAttachments.push_back(
            RenderingAttachment{
                .view = swapChain.GetImageView(imageIndex),
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            }
        );
        shadingRenderInfo.depthAttachment = RenderingAttachment{
                .view = swapchainDepthViews[imageIndex]->GetImageView(),
                .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        };
    }
    gbufferRenderInfo.offset = { 0, 0 };
    gbufferRenderInfo.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();

    shadowmapRenderInfo.offset = { 0, 0 };
    shadowmapRenderInfo.extent = shadowmapExtent;

    shadingRenderInfo.offset = { 0, 0 };
    shadingRenderInfo.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();


    //todo:  ÷∂Ø±‰ªªimage layout
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer2(0, m_shadowPass, m_currentFrame, shadowmapRenderInfo, m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), m_scene->GetIndirectBuffer(), m_scene->GetIndirectDrawCommands().size(), std::string("Shadowmap Pass"));
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer2(0, m_gbufferPass, m_currentFrame, gbufferRenderInfo, m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), m_scene->GetIndirectBuffer(), m_scene->GetIndirectDrawCommands().size(), std::string("Gbuffer Pass"));
    
    //graphicsList.End();
    //
    //VkSubmitInfo submitInfo{};
    //submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    //submitInfo.commandBufferCount = 1;
    //submitInfo.pCommandBuffers = &graphicsList.GetBuffer();
    //
    //graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    //graphicsQueue.WaitForQueueComplete();
    
    ImageLayoutToShaderRead(m_currentFrame);
   

    //graphicsList.Reset();
    //graphicsList.Begin();
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer2(imageIndex, m_lightingPass, m_currentFrame, shadingRenderInfo, m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size(), std::string("Shading Pass"));

    ImageLayoutToAttachment(imageIndex, m_currentFrame);
    ImageLayoutToPresent(imageIndex, m_currentFrame);

    graphicsList.End();
    Singleton<MVulkanEngine>::instance().SubmitGraphicsCommands(imageIndex, m_currentFrame);

    //ImageLayoutToAttachment(imageIndex);
    //ImageLayoutToPresent(imageIndex);
    //spdlog::info("camera.pos: {0}, {1}, {2}", m_camera->GetPosition().x, m_camera->GetPosition().y, m_camera->GetPosition().z);
}

void PBR::RecreateSwapchainAndRenderPasses()
{
    //if (Singleton<MVulkanEngine>::instance().RecreateSwapchain()) {
    //    Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_gbufferPass);
    //    Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_shadowPass);
    //
    //    m_lightingPass->GetRenderPassCreateInfo().depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();
    //    Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_lightingPass);
    //
    //    std::vector<std::vector<VkImageView>> gbufferViews(5);
    //    for (auto i = 0; i < 4; i++) {
    //        gbufferViews[i].resize(1);
    //        gbufferViews[i][0] = m_gbufferPass->GetFrameBuffer(0).GetImageView(i);
    //    }
    //    gbufferViews[4] = std::vector<VkImageView>(2);
    //    gbufferViews[4][0] = m_shadowPass->GetFrameBuffer(0).GetDepthImageView();
    //    gbufferViews[4][1] = m_shadowPass->GetFrameBuffer(0).GetDepthImageView();
    //
    //    std::vector<VkSampler> samplers(1);
    //    samplers[0] = m_linearSampler.GetSampler();
    //
    //    m_lightingPass->UpdateDescriptorSetWrite(gbufferViews, samplers);
    //}
}

void PBR::CreateRenderPass()
{
    createGbufferPass();
    createShadowPass();
    createShadingPass();

    createLightCamera();
}

void PBR::PreComputes()
{

}

void PBR::Clean()
{
    //m_gbufferPass->Clean();
    //m_lightingPass->Clean();
    //m_shadowPass->Clean();

    m_linearSampler.Clean();

    m_squad->Clean();
    m_scene->Clean();

    MRenderApplication::Clean();
}

void PBR::loadScene()
{
    m_scene = std::make_shared<Scene>();

    fs::path projectRootPath = PROJECT_ROOT;
    fs::path resourcePath = projectRootPath.append("resources").append("models");
    fs::path modelPath = resourcePath / "Sponza" / "glTF" / "Sponza.gltf";
    //fs::path modelPath = resourcePath / "San_Miguel" / "san-miguel-low-poly.obj";
    //fs::path modelPath = resourcePath / "shapespark_example_room" / "shapespark_example_room.gltf";

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

void PBR::createLight()
{
    glm::vec3 direction = glm::normalize(glm::vec3(-1.f, -6.f, -1.f));
    //glm::vec3 direction = glm::normalize(glm::vec3(-2.f, -1.f, 1.f));
    glm::vec3 color = glm::vec3(1.f, 1.f, 1.f);
    float intensity = 10.f;
    m_directionalLight = std::make_shared<DirectionalLight>(direction, color, intensity);
}

void PBR::createCamera()
{
    //glm::vec3 position(-1.f, 0.f, 4.f);
    //glm::vec3 center(0.f);
    //glm::vec3 direction = glm::normalize(center - position);

    //-4.9944386, 2.9471996, -5.8589
    glm::vec3 position(-4.9944386, 2.9471996, -5.8589);
    glm::vec3 direction = glm::normalize(glm::vec3(2.f, -1.f, 2.f));

    //volumn pmin : -6.512552, 0.31353754, -6.434822
    // volumn pmax : 2.139489, 3.020252, 6.2549577
    //
    float fov = 60.f;
    float aspectRatio = (float)WIDTH / (float)HEIGHT;
    float zNear = 0.01f;
    float zFar = 1000.f;

    m_camera = std::make_shared<Camera>(position, direction, fov, aspectRatio, zNear, zFar);
    Singleton<MVulkanEngine>::instance().SetCamera(m_camera);
}

void PBR::createSamplers()
{
    {
        MVulkanSamplerCreateInfo info{};
        info.minFilter = VK_FILTER_LINEAR;
        info.magFilter = VK_FILTER_LINEAR;
        info.mipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        m_linearSampler.Create(Singleton<MVulkanEngine>::instance().GetDevice(), info);
    }
}

void PBR::createLightCamera()
{
    VkExtent2D extent = shadowmapExtent;

    glm::vec3 direction = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetDirection();
    glm::vec3 position = -50.f * direction;

    float fov = 60.f;
    float aspect = (float)extent.width / (float)extent.height;
    float zNear = 0.001f;
    float zFar = 60.f;
    m_directionalLightCamera = std::make_shared<Camera>(position, direction, fov, aspect, zNear, zFar);
    m_directionalLightCamera->SetOrth(true);
}

void PBR::createTextures()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();
    auto swapchainExtent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
    auto shadowMapExtent = VkExtent2D{ 4096, 4096 };

    auto depthFormat = device.FindDepthFormat();
    auto shadowMapFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

    {
        shadowMap = std::make_shared<MVulkanTexture>();
        shadowMapDepth = std::make_shared<MVulkanTexture>();

        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            shadowMap, shadowMapExtent, shadowMapFormat
        );

        Singleton<MVulkanEngine>::instance().CreateDepthAttachmentImage(
            shadowMapDepth, shadowMapExtent, depthFormat
        );
    }

    {
        swapchainDepthViews.resize(Singleton<MVulkanEngine>::instance().GetSwapchainImageCount());

        for (auto i = 0; i < swapchainDepthViews.size(); i++) {
            //Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            //    shadowMap, shadowMapExtent, shadowMapFormat
            //);
            swapchainDepthViews[i] = std::make_shared<MVulkanTexture>();

            Singleton<MVulkanEngine>::instance().CreateDepthAttachmentImage(
                swapchainDepthViews[i], swapchainExtent, depthFormat
            );
        }
    }

    {
        auto gbufferFormat = VK_FORMAT_R32G32B32A32_UINT;

        gBuffer0 = std::make_shared<MVulkanTexture>();
        gBuffer1 = std::make_shared<MVulkanTexture>();
        gBufferDepth = std::make_shared<MVulkanTexture>();
        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            gBuffer0, swapchainExtent, gbufferFormat
        );

        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            gBuffer1, swapchainExtent, gbufferFormat
        );

        Singleton<MVulkanEngine>::instance().CreateDepthAttachmentImage(
            gBufferDepth, shadowMapExtent, depthFormat
        );
    }

}

void PBR::createGbufferPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        //std::vector<VkFormat> gbufferFormats;
        //gbufferFormats.push_back(VK_FORMAT_R32G32B32A32_UINT);
        //gbufferFormats.push_back(VK_FORMAT_R32G32B32A32_UINT);

        DynamicRenderPassCreateInfo info{};
        //info.depthFormat = device.FindDepthFormat();
        //info.frambufferCount = 1;
        //info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
        //info.useAttachmentResolve = false;
        //info.useSwapchainImages = false;
        //info.imageAttachmentFormats = gbufferFormats;
        //info.colorAttachmentResolvedViews = nullptr;
        //info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        //info.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        //info.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        //info.finalDepthLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        //info.numAttachments = 2;
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_UINT);
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_UINT);
        info.pipelineCreateInfo.depthAttachmentFormats = device.FindDepthFormat();

        info.numFrameBuffers = 1;
        info.pipelineCreateInfo.depthTestEnable = VK_TRUE;
        info.pipelineCreateInfo.depthWriteEnable = VK_TRUE;
        info.pipelineCreateInfo.depthCompareOp = VkCompareOp::VK_COMPARE_OP_LESS;

        m_gbufferPass = std::make_shared<DynamicRenderPass>(device, info);

        std::shared_ptr<ShaderModule> gbufferShader = std::make_shared<GbufferShader2>();
        
        std::vector<StorageBuffer> storageBuffers(0);
        std::vector<std::vector<VkImageView>> seperateImageViews(1);
        std::vector<std::vector<VkImageView>> storageImageViews(0);
        std::vector<VkSampler> samplers(1);
        
        seperateImageViews[0] = Singleton<TextureManager>::instance().GenerateTextureViews();
        samplers[0] = m_linearSampler.GetSampler();

        Singleton<MVulkanEngine>::instance().CreateDynamicRenderPass(
            m_gbufferPass, gbufferShader, storageBuffers, seperateImageViews, storageImageViews, samplers);
    }
}

void PBR::createShadowPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        //std::vector<VkFormat> shadowFormats(0);
        //shadowFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);

        DynamicRenderPassCreateInfo info{};
        //info.depthFormat = VK_FORMAT_D32_SFLOAT;
        //info.frambufferCount = 1;
        //info.extent = VkExtent2D(4096, 4096);
        //info.useAttachmentResolve = false;
        //info.useSwapchainImages = false;
        //info.imageAttachmentFormats = shadowFormats;
        //info.colorAttachmentResolvedViews = nullptr;
        //info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        //info.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        //info.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        //info.finalDepthLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        info.pipelineCreateInfo.depthAttachmentFormats = device.FindDepthFormat();
        info.numFrameBuffers = 1;

        m_shadowPass = std::make_shared<DynamicRenderPass>(device, info);

        std::shared_ptr<ShaderModule> shadowShader = std::make_shared<ShadowShader>();

        std::vector<StorageBuffer> storageBuffers(0);
        std::vector<std::vector<VkImageView>> seperateImageViews(0);
        std::vector<std::vector<VkImageView>> storageImageViews(0);
        std::vector<VkSampler> samplers(0);

        Singleton<MVulkanEngine>::instance().CreateDynamicRenderPass(
            m_shadowPass, shadowShader, storageBuffers, seperateImageViews, storageImageViews, samplers);
    }
}

void PBR::createShadingPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        std::vector<VkFormat> lightingPassFormats;
        lightingPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());

        DynamicRenderPassCreateInfo info{};
        //info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
        //info.depthFormat = device.FindDepthFormat();
        //info.frambufferCount = Singleton<MVulkanEngine>::instance().GetSwapchainImageCount();
        //info.useSwapchainImages = true;
        //info.imageAttachmentFormats = lightingPassFormats;
        //info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        //info.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        //info.initialDepthLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        //info.finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        //info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());
        info.pipelineCreateInfo.depthAttachmentFormats = device.FindDepthFormat();
        //info.numFrameBuffers = 1;
        info.numFrameBuffers = Singleton<MVulkanEngine>::instance().GetSwapchainImageCount();
        info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
        info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
        //info.depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();

        m_lightingPass = std::make_shared<DynamicRenderPass>(device, info);

        std::shared_ptr<ShaderModule> lightingShader = std::make_shared<LightingPbrShader2>();

        std::vector<StorageBuffer> storageBuffers(0);
        std::vector<std::vector<VkImageView>> seperateImageViews(3);
        std::vector<std::vector<VkImageView>> storageImageViews(0);
        std::vector<VkSampler> samplers(1);

        //std::vector<std::vector<VkImageView>> gbufferViews(3);
        //for (auto i = 0; i < 2; i++) {
            seperateImageViews[0].resize(1);
            seperateImageViews[0][0] = gBuffer0->GetImageView();
            seperateImageViews[1].resize(1);
            seperateImageViews[1][0] = gBuffer1->GetImageView();
        //}
        seperateImageViews[2] = std::vector<VkImageView>(2);
        seperateImageViews[2][0] = shadowMapDepth->GetImageView();
        seperateImageViews[2][1] = shadowMapDepth->GetImageView();
        samplers[0] = m_linearSampler.GetSampler();

        Singleton<MVulkanEngine>::instance().CreateDynamicRenderPass(
            m_lightingPass, lightingShader, storageBuffers, seperateImageViews, storageImageViews, samplers);
    }
}

void PBR::ImageLayoutToShaderRead(int currentFrame)
{
    std::vector<MVulkanImageMemoryBarrier> barriers;
    {
        MVulkanImageMemoryBarrier barrier{};
        barrier.image = gBuffer0->GetImage();
        barrier.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.baseArrayLayer = 0;
        barrier.layerCount = 1;
        barrier.levelCount = 1;
        barrier.srcAccessMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barriers.push_back(barrier);

        barrier.image = gBuffer1->GetImage();
        barriers.push_back(barrier);

        barrier.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        barrier.image = shadowMapDepth->GetImage();
        barriers.push_back(barrier);
    }
    Singleton<MVulkanEngine>::instance().TransitionImageLayout2(currentFrame, barriers, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}

void PBR::ImageLayoutToAttachment(int imageIndex, int currentFrame)
{
    std::vector<MVulkanImageMemoryBarrier> barriers;
    {
        MVulkanImageMemoryBarrier barrier{};
        barrier.image = gBuffer0->GetImage();
        barrier.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.baseArrayLayer = 0;
        barrier.layerCount = 1;
        barrier.levelCount = 1;
        barrier.srcAccessMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        barrier.dstAccessMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        barriers.push_back(barrier);

        barrier.image = gBuffer1->GetImage();
        barriers.push_back(barrier);

        barrier.image = shadowMap->GetImage();
        barriers.push_back(barrier);

        barrier.image = Singleton<MVulkanEngine>::instance().GetSwapchain().GetImage(imageIndex);
        barriers.push_back(barrier);

        barrier.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        barrier.image = shadowMapDepth->GetImage();
        barrier.srcAccessMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        barrier.dstAccessMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        //barrier.dst
        barriers.push_back(barrier);
    }
    Singleton<MVulkanEngine>::instance().TransitionImageLayout2(currentFrame, barriers, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}

void PBR::ImageLayoutToPresent(int imageIndex, int currentFrame)
{
    std::vector<MVulkanImageMemoryBarrier> barriers;
    {
        MVulkanImageMemoryBarrier barrier{};
        barrier.image = Singleton<MVulkanEngine>::instance().GetSwapchain().GetImage(imageIndex);
        barrier.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.baseArrayLayer = 0;
        barrier.layerCount = 1;
        barrier.levelCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;
        barrier.oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barriers.push_back(barrier);
    }
    Singleton<MVulkanEngine>::instance().TransitionImageLayout2(currentFrame, barriers, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_NONE);
}
