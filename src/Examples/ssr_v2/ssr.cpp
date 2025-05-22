#include "ssr.hpp"

#include "MVulkanRHI/MVulkanEngine.hpp"
#include "Scene/SceneLoader.hpp"
#include "Scene/Scene.hpp"
#include "Managers/TextureManager.hpp"
#include "Managers/ShaderManager.hpp"
#include "Managers/InputManager.hpp"
#include "RenderPass.hpp"
#include "ComputePass.hpp"
#include "Camera.hpp"
#include "Scene/Light/DirectionalLight.hpp"
#include "Shaders/ShaderModule.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shaders/share/Common.h"
#include "Shaders/share/SSR.h"

void SSR::SetUp()
{
    createSyncObjs();

    createSamplers();
    createLight();
    createCamera();
    createTextures();
    loadShaders();

    loadScene();
    createStorageBuffers();

    initHiz();
}

void SSR::ComputeAndDraw(uint32_t imageIndex)
{
    //auto hizMode = std::static_pointer_cast<SSRUI>(m_uiRenderer)->hizMode;
    //auto copyHiz = std::static_pointer_cast<SSRUI>(m_uiRenderer)->copyHiz;
    auto doSSR = std::static_pointer_cast<SSRUI>(m_uiRenderer)->doSSR;

    m_queryIndex = 0;
    int shadowmapQueryIndex = 0;
    int gbufferQueryIndex = 0;
    int shadingQueryIndex = 0;
    int cullingQueryIndex = 0;
    //int cullingQueryInedxEnd;
    int hizQueryIndexStart = 0;
    int hizQueryIndexEnd = 0;
    int ssrQueryIndex = 0;
    int compositeQueryIndex = 0;
    //std::vector<int> hizQueryIndex(0);
    //Singleton<MVulkanEngine>::instance().ResetTimeStampQueryPool();

    auto& graphicsList = Singleton<MVulkanEngine>::instance().GetGraphicsList(m_currentFrame);
    auto& graphicsQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::GRAPHICS);

    auto& computeList = Singleton<MVulkanEngine>::instance().GetComputeCommandList();
    auto& computeQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::COMPUTE);

    //ImageLayoutToAttachment(imageIndex);
    auto swapchainExtent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();


    //prepare gbufferPass ubo
    {
        VPBuffer vpBuffer{};
        vpBuffer.View = m_camera->GetViewMatrix();
        vpBuffer.Projection = m_camera->GetProjMatrix();
        Singleton<ShaderResourceManager>::instance().LoadData("vpBuffer", 0, &vpBuffer, 0);

        VPBuffer vpBuffer_p{};
        vpBuffer_p.View = m_prevView;
        vpBuffer_p.Projection = m_prevProj;
        Singleton<ShaderResourceManager>::instance().LoadData("vpBuffer_p", 0, &vpBuffer_p, 0);

        auto gbufferExtent = swapchainExtent;
        MScreenBuffer gBufferInfoBuffer{};
        gBufferInfoBuffer.WindowRes = int2(gbufferExtent.width, gbufferExtent.height);

        Singleton<ShaderResourceManager>::instance().LoadData("gBufferInfoBuffer", 0, &gBufferInfoBuffer, 0);

    }

    //prepare shadowPass ubo
    {
        glm::mat4 shadowVP{};
        shadowVP = m_directionalLightCamera->GetOrthoMatrix() * m_directionalLightCamera->GetViewMatrix();
        Singleton<ShaderResourceManager>::instance().LoadData("shadowVpBuffer", 0, &shadowVP, 0);
    }

    //prepare lightingPass ubo
    {

        LightBuffer lightBuffer{};
        lightBuffer.lightNum = 1;
        lightBuffer.lights[0].direction = m_directionalLightCamera->GetDirection();
        lightBuffer.lights[0].intensity = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetIntensity();
        lightBuffer.lights[0].color = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetColor();
        lightBuffer.lights[0].shadowMapIndex = 0;
        lightBuffer.lights[0].shadowViewProj = m_directionalLightCamera->GetOrthoMatrix() * m_directionalLightCamera->GetViewMatrix();
        lightBuffer.lights[0].shadowCameraZnear = m_directionalLightCamera->GetZnear();
        lightBuffer.lights[0].shadowCameraZfar = m_directionalLightCamera->GetZfar();
        lightBuffer.lights[0].shadowmapResolution = int2(shadowmapExtent.width, shadowmapExtent.height);

        MCameraBuffer cameraBuffer{};
        cameraBuffer.cameraPos = m_camera->GetPosition();
        cameraBuffer.cameraDir = m_camera->GetDirection();

        MScreenBuffer screenBuffer{};
        screenBuffer.WindowRes = int2(swapchainExtent.width, swapchainExtent.height);

        int outputContext = std::static_pointer_cast<SSRUI>(m_uiRenderer)->outputContext;

        Singleton<ShaderResourceManager>::instance().LoadData("lightBuffer", 0, &lightBuffer, 0);
        Singleton<ShaderResourceManager>::instance().LoadData("cameraBuffer", 0, &cameraBuffer, 0);
        Singleton<ShaderResourceManager>::instance().LoadData("screenBuffer", m_currentFrame, &screenBuffer, 0);
        Singleton<ShaderResourceManager>::instance().LoadData("intBuffer", 0, &outputContext, 0);
    }

    {
        HizDimensionBuffer hizDimensionBuffer;
        auto hizLayers = m_hiz.GetHizLayers();
        for (auto i = 0; i < hizLayers; i++) {
            auto res = m_hiz.GetHizRes(i);
            hizDimensionBuffer.hizDimensions[i].x = res.x;
            hizDimensionBuffer.hizDimensions[i].y = res.y;
        }
        hizDimensionBuffer.hizDimensions[0].z = hizLayers;

        SSRBuffer ssrBuffer;
        ssrBuffer.g_max_traversal_intersections = 256;
        ssrBuffer.min_traversal_occupancy = 50;

        SSRConfigBuffer ssrConfig;
        ssrConfig.doSSR = doSSR ? 1 : 0;

        Singleton<ShaderResourceManager>::instance().LoadData("ssrBuffer", 0, &ssrBuffer, 0);
        Singleton<ShaderResourceManager>::instance().LoadData("hizDimensionBuffer", 0, &hizDimensionBuffer, 0);
        
        Singleton<ShaderResourceManager>::instance().LoadData("ssrConfigBuffer", m_currentFrame, &ssrConfig, 0);
    }

    {
        auto frustum = m_camera->GetFrustum();
        FrustumBuffer buffer;
        for (int i = 0; i < 6; i++) {
            buffer.planes[i] = frustum.planes[i];
        }

        MSceneBuffer sceneBuffer;
        sceneBuffer.numInstances = m_scene->GetIndirectDrawCommands().size();
        sceneBuffer.targetIndex = 0;
        sceneBuffer.cullingMode = std::static_pointer_cast<SSRUI>(m_uiRenderer)->cullingMode;

        Singleton<ShaderResourceManager>::instance().LoadData("FrustumBuffer", 0, &buffer, 0);
        Singleton<ShaderResourceManager>::instance().LoadData("sceneBuffer", 0, &sceneBuffer, 0);
    }

    RenderingInfo gbufferRenderInfo, shadowmapRenderInfo, shadingRenderInfo, compositingRenderInfo, ssrRenderInfo;
    {
        gbufferRenderInfo.colorAttachments.push_back(
            RenderingAttachment{
                .texture = gBuffer0,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            }
            );
        gbufferRenderInfo.colorAttachments.push_back(
            RenderingAttachment{
                .texture = gBuffer1,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            }
            );

        gbufferRenderInfo.depthAttachment = RenderingAttachment{
                .texture = gBufferDepth,
                .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };
    }

    {
        shadowmapRenderInfo.colorAttachments.push_back(
            RenderingAttachment{
                .texture = shadowMap,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            }
            );
        shadowmapRenderInfo.depthAttachment = RenderingAttachment{
                .texture = shadowMapDepth,
                .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };
    }

    {
        auto swapChain = Singleton<MVulkanEngine>::instance().GetSwapchain();

        shadingRenderInfo.colorAttachments.push_back(
            RenderingAttachment{
                .texture = m_basicShadingTexture,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            }
            );
        shadingRenderInfo.useDepth = false;
    }

    //{
    //    auto swapChain = Singleton<MVulkanEngine>::instance().GetSwapchain();
    //
    //    ssrRenderInfo.colorAttachments.push_back(
    //        RenderingAttachment{
    //            .texture = m_ssrTexture_frag,
    //            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    //        }
    //        );
    //    ssrRenderInfo.useDepth = false;
    //}

    {
        auto swapChain = Singleton<MVulkanEngine>::instance().GetSwapchain();

        compositingRenderInfo.colorAttachments.push_back(
            RenderingAttachment{
                .texture = nullptr,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .view = swapChain.GetImageView(imageIndex),
            }
            );
        compositingRenderInfo.depthAttachment = RenderingAttachment{
                .texture = swapchainDepthViews[imageIndex],
                .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };
    }

    gbufferRenderInfo.offset = { 0, 0 };
    gbufferRenderInfo.extent = swapchainExtent;

    shadowmapRenderInfo.offset = { 0, 0 };
    shadowmapRenderInfo.extent = shadowmapExtent;

    shadingRenderInfo.offset = { 0, 0 };
    shadingRenderInfo.extent = swapchainExtent;

    compositingRenderInfo.offset = { 0, 0 };
    compositingRenderInfo.extent = swapchainExtent;

    ssrRenderInfo.offset = { 0, 0 };
    ssrRenderInfo.extent = swapchainExtent;

    {
        computeList.GetFence().WaitForSignal();
        computeList.GetFence().Reset();
        computeList.Reset();
        computeList.Begin();
        Singleton<MVulkanEngine>::instance().CmdResetTimeStampQueryPool(computeList);

        Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_resetDidirectDrawBufferPass, computeList, 1, 1, 1, std::string("Reset NumIndirectDrawBuffer Pass"), m_queryIndex++);
        auto numInstances = m_scene->GetIndirectDrawCommands().size();
        cullingQueryIndex = m_queryIndex;
        Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_frustumCullingPass, computeList, numInstances, 1, 1, std::string("FrustumCulling Pass"), m_queryIndex++);

        computeList.End();

        std::vector<MVulkanSemaphore> waitSemaphores(0);
        std::vector<MVulkanSemaphore> signalSemaphores(1, m_cullingSemaphore);
        std::vector<VkPipelineStageFlags> waitFlags(0);
        Singleton<MVulkanEngine>::instance().SubmitCommands(computeList, computeQueue, waitSemaphores, waitFlags, signalSemaphores);

    }
    
    numDrawInstances = *(uint32_t*)m_numIndirectDrawBuffer->GetMappedData();

    {
        graphicsList.GetFence().WaitForSignal();
        graphicsList.GetFence().Reset();
        graphicsList.Reset();
        graphicsList.Begin();
        
        shadowmapQueryIndex = m_queryIndex;
        Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_shadowPass, m_currentFrame, shadowmapRenderInfo, m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), m_scene->GetIndirectBuffer(), m_scene->GetIndirectDrawCommands().size(), std::string("Shadowmap Pass"), m_queryIndex++);
        gbufferQueryIndex = m_queryIndex;
        Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_gbufferPass, m_currentFrame, gbufferRenderInfo, m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), m_culledIndirectDrawBuffer, m_scene->GetIndirectDrawCommands().size(), std::string("Gbuffer Pass"), m_queryIndex++);
        shadingQueryIndex = m_queryIndex;
        Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_lightingPass, m_currentFrame, shadingRenderInfo, m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size(), std::string("Shading Pass"), m_queryIndex++);
        
        graphicsList.End();
        std::vector<MVulkanSemaphore> waitSemaphores0(1, m_cullingSemaphore);
        std::vector<MVulkanSemaphore> signalSemaphores0(1, m_basicShadingSemaphore);
        std::vector<VkPipelineStageFlags> waitFlags0(1, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        Singleton<MVulkanEngine>::instance().SubmitCommands(graphicsList, graphicsQueue, waitSemaphores0, waitFlags0, signalSemaphores0);
    }

    {
        if (doSSR) {
            computeList.GetFence().WaitForSignal();
            computeList.GetFence().Reset();
            computeList.Reset();
            computeList.Begin();
        
            m_hiz.Generate(computeList, m_queryIndex);
            hizQueryIndexStart = m_hiz.GetHizQueryIndexStart();
            hizQueryIndexEnd = m_hiz.GetHizQueryIndexEnd();
        
            ssrQueryIndex = m_queryIndex;
            Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_ssrPass, computeList, (swapchainExtent.width + 15)/16, (swapchainExtent.height + 15) / 16, 1, std::string("SSR Pass"), m_queryIndex++);
        
            computeList.End();
        
            std::vector<MVulkanSemaphore> waitSemaphores2(1, m_basicShadingSemaphore);
            std::vector<MVulkanSemaphore> signalSemaphores2(1, m_ssrSemaphore);
            std::vector<VkPipelineStageFlags> waitFlags2(1, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
            Singleton<MVulkanEngine>::instance().SubmitCommands(computeList, computeQueue, waitSemaphores2, waitFlags2, signalSemaphores2);
        }
    }


    {
        graphicsList.GetFence().WaitForSignal();
        graphicsList.GetFence().Reset();
        graphicsList.Reset();
        graphicsList.Begin();

        compositeQueryIndex = m_queryIndex;
        Singleton<MVulkanEngine>::instance().RecordCommandBuffer(imageIndex, m_compositePass, m_currentFrame, compositingRenderInfo, m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size(), std::string("Composite Pass"), m_queryIndex++);

        graphicsList.End();

        if (doSSR) {
            std::vector<MVulkanSemaphore> waitSemaphores1(1, m_ssrSemaphore);
            std::vector<MVulkanSemaphore> signalSemaphores1(0);
            std::vector<VkPipelineStageFlags> waitFlags1(1, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            Singleton<MVulkanEngine>::instance().SubmitGraphicsCommands(imageIndex, m_currentFrame, waitSemaphores1, waitFlags1, signalSemaphores1);
        }
        else {
            std::vector<MVulkanSemaphore> waitSemaphores1(1, m_basicShadingSemaphore);
            std::vector<MVulkanSemaphore> signalSemaphores1(0);
            std::vector<VkPipelineStageFlags> waitFlags1(1, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
            //std::vector<MVulkanSemaphore> waitSemaphores1(0);
            //std::vector<MVulkanSemaphore> signalSemaphores1(0);
            //std::vector<VkPipelineStageFlags> waitFlags1(0);
            Singleton<MVulkanEngine>::instance().SubmitGraphicsCommands(imageIndex, m_currentFrame, waitSemaphores1, waitFlags1, signalSemaphores1);
            //graphicsQueue.WaitForQueueComplete();
        }
    }

    m_prevView = m_camera->GetViewMatrix();
    m_prevProj = m_camera->GetProjMatrix();

    //return;
    auto showPassTime = std::static_pointer_cast<SSRUI>(m_uiRenderer)->showPassTime;
    if (showPassTime) {
        auto queryResults = Singleton<MVulkanEngine>::instance().GetTimeStampQueryResults(0, 2 * m_queryIndex);

        auto timestampPeriod = Singleton<MVulkanEngine>::instance().GetTimeStampPeriod();

        cullingTime = (queryResults[2 * cullingQueryIndex + 1] - queryResults[2 * cullingQueryIndex]) * timestampPeriod / 1000000.f;
        gbufferTime = (queryResults[2 * gbufferQueryIndex + 1] - queryResults[2 * gbufferQueryIndex]) * timestampPeriod / 1000000.f;
        shadowmapTime = (queryResults[2 * shadowmapQueryIndex + 1] - queryResults[2 * shadowmapQueryIndex]) * timestampPeriod / 1000000.f;
        shadingTime = (queryResults[2 * shadingQueryIndex + 1] - queryResults[2 * shadingQueryIndex]) * timestampPeriod / 1000000.f;
        compositeTime = (queryResults[2 * compositeQueryIndex + 1] - queryResults[2 * compositeQueryIndex]) * timestampPeriod / 1000000.f;

        if (doSSR) {
            hizTime = (queryResults[2 * hizQueryIndexEnd - 1] - queryResults[2 * hizQueryIndexStart]) * timestampPeriod / 1000000.f;
            ssrTime = (queryResults[2 * ssrQueryIndex + 1] - queryResults[2 * ssrQueryIndex]) * timestampPeriod / 1000000.f;
            
        }
        else {
            hizTime = 0.f;
            ssrTime = 0.f;

        }
    }
    else {
        cullingTime = 0.f;
        gbufferTime = 0.f;
        shadowmapTime = 0.f;
        shadingTime = 0.f;
        hizTime = 0.f;
        ssrTime = 0.f;
        compositeTime = 0.f;
    }
}

void SSR::RecreateSwapchainAndRenderPasses()
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

void SSR::CreateRenderPass()
{
    createGbufferPass();
    createShadowPass();
    createShadingPass();
    createResetDidirectDrawBufferPass();
    createFrustumCullingPass();

    createSSRPass();
    createCompositePass();

    createLightCamera();
}

void SSR::PreComputes()
{

}

void SSR::Clean()
{
    //m_gbufferPass->Clean();
    //m_lightingPass->Clean();
    //m_shadowPass->Clean();

    m_linearSampler.Clean();

    m_squad->Clean();
    m_scene->Clean();

    MRenderApplication::Clean();
}

void SSR::loadScene()
{
    m_scene = std::make_shared<Scene>();

    fs::path projectRootPath = PROJECT_ROOT;
    fs::path resourcePath = projectRootPath.append("resources").append("models");
    fs::path sponzaPath = resourcePath / "Sponza" / "glTF" / "Sponza.gltf";
    fs::path arcadePath = resourcePath / "Arcade" / "Arcade.gltf";
    fs::path bistroPath = resourcePath / "Bistro" / "bistro.gltf";
    //fs::path modelPath = resourcePath / "San_Miguel" / "san-miguel-low-poly.obj";
    //fs::path modelPath = resourcePath / "shapespark_example_room" / "shapespark_example_room.gltf";

    Singleton<SceneLoader>::instance().Load(sponzaPath.string(), m_scene.get());

    //split Image
    {
        auto wholeTextures = Singleton<TextureManager>::instance().GenerateUnmipedTextureVector();
        if (wholeTextures.size() > 0) {
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
    //glm::vec3 direction = glm::normalize(glm::vec3(-2.f, -1.f, 1.f));
    glm::vec3 color = glm::vec3(1.f, 1.f, 1.f);
    //float intensity = 10.f;
    float intensity = 10.f;
    m_directionalLight = std::make_shared<DirectionalLight>(direction, color, intensity);
}

void SSR::createCamera()
{
    //glm::vec3 position(-1.f, 0.f, 4.f);
    //glm::vec3 center(0.f);
    //glm::vec3 direction = glm::normalize(center - position);

    //-4.9944386, 2.9471996, -5.8589
    glm::vec3 position(-6.0, 3.3, -0.2);
    glm::vec3 direction = glm::normalize(glm::vec3(0.8f, -0.36f, -0.3f));
    //
    //bistro
    //glm::vec3 position(-20.f, 3.0f, 19.f);
    //glm::vec3 direction = glm::normalize(glm::vec3(0.12f, -0.06f, -0.99f));


    //volumn pmin : -6.512552, 0.31353754, -6.434822
    // volumn pmax : 2.139489, 3.020252, 6.2549577
    //
    float fov = 60.f;
    float aspectRatio = (float)WIDTH / (float)HEIGHT;
    float zNear = 0.01f;
    float zFar = 100.f;

    //m_camera = std::make_shared<Camera>(position, direction, fov, aspectRatio, zNear, zFar);
    //m_camera = std::make_shared<Camera>(position, direction, fov, aspectRatio, zNear, zFar, xMin, xMax, yMin, yMax, zMin, zMax);
    //Singleton<MVulkanEngine>::instance().SetCamera(m_camera);
    m_camera = std::make_shared<Camera>(position, direction, fov, aspectRatio, zNear, zFar);
    Singleton<MVulkanEngine>::instance().SetCamera(m_camera);

    m_prevView = m_camera->GetViewMatrix();
    m_prevProj = m_camera->GetProjMatrix();
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
}

void SSR::createLightCamera()
{
    VkExtent2D extent = shadowmapExtent;

    //sponza
    glm::vec3 direction = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetDirection();
    glm::vec3 position = -50.f * direction;
    float zFar = 60.f;

    //bistro
    //glm::vec3 direction = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetDirection();
    //glm::vec3 position = glm::vec3(16.015f, 79.726f, 22.634f);
    //float zFar = 100.f;

    float fov = 60.f;
    float aspect = (float)extent.width / (float)extent.height;
    float zNear = 0.001f;
    //float zFar = 100.f;

    float xMin = -42.f;
    float xMax = 42.f;
    float yMin = -70.f;
    float yMax = 70.f;

    //sponza
    m_directionalLightCamera = std::make_shared<Camera>(position, direction, fov, aspect, zNear, zFar);

    //bistro
    //m_directionalLightCamera = std::make_shared<Camera>(position, direction, float3(0.f, 1.f, 0.f), aspect, xMin, xMax, yMin, yMax, zNear, zFar);
    m_directionalLightCamera->SetOrth(true);

    //Singleton<MVulkanEngine>::instance().SetCamera(m_directionalLightCamera);
}

void SSR::createTextures()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();
    auto swapchainExtent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
    auto shadowMapExtent = shadowmapExtent;

    auto depthFormat = device.FindDepthFormat();
    auto shadowMapFormat = VK_FORMAT_R32G32B32A32_UINT;
    auto swapchainImageFormat = Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat();

    auto renderingFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

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
            swapchainDepthViews[i] = std::make_shared<MVulkanTexture>();

            Singleton<MVulkanEngine>::instance().CreateDepthAttachmentImage(
                swapchainDepthViews[i], swapchainExtent, depthFormat
            );
        }
    }

    {
        auto gbufferFormat = VK_FORMAT_R32G32B32A32_UINT;
        auto motionVectorFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

        gBuffer0 = std::make_shared<MVulkanTexture>();
        gBuffer1 = std::make_shared<MVulkanTexture>();
        //gBuffer2 = std::make_shared<MVulkanTexture>();

        gBufferDepth = std::make_shared<MVulkanTexture>();
        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            gBuffer0, swapchainExtent, gbufferFormat
        );

        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            gBuffer1, swapchainExtent, gbufferFormat
        );
        //Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
        //    gBuffer2, swapchainExtent, VK_FORMAT_R32G32B32A32_SFLOAT
        //);

        //auto numHizLayers = m_hiz.hizRes.size();
        Singleton<MVulkanEngine>::instance().CreateDepthAttachmentImage(
            gBufferDepth, swapchainExtent, depthFormat
        );
    }

    {
        m_basicShadingTexture = std::make_shared<MVulkanTexture>();
        //m_ssrTexture_frag = std::make_shared<MVulkanTexture>();
    
        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            m_basicShadingTexture, swapchainExtent, renderingFormat
        );
    
        //Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
        //    m_ssrTexture_frag, swapchainExtent, VK_FORMAT_R32G32B32A32_SFLOAT
        //);
    }

    {
        ImageCreateInfo imageInfo;
        ImageViewCreateInfo viewInfo;
        imageInfo.arrayLength = 1;
        imageInfo.usage =  VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
        imageInfo.format = renderingFormat;

        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = imageInfo.format;
        viewInfo.flag = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.baseMipLevel = 0;

        viewInfo.baseArrayLayer = 0;
        viewInfo.layerCount = 1;

        //auto depthFormat = device.FindDepthFormat();
        imageInfo.width = swapchainExtent.width;
        imageInfo.height = swapchainExtent.height;

        m_ssrTexture = std::make_shared<MVulkanTexture>();
        Singleton<MVulkanEngine>::instance().CreateImage(m_ssrTexture, imageInfo, viewInfo);
    }

}

void SSR::loadShaders()
{
    //Singleton<ShaderManager>::instance().AddShader("GBuffer Shader", { "hlsl/gbuffer.vert.hlsl", "hlsl/gbuffer/gbuffer.frag.hlsl", "main", "main" });
    Singleton<ShaderManager>::instance().AddShader("GBuffer Shader", { "hlsl/gbuffer/gbuffer2.vert.hlsl", "hlsl/gbuffer/gbuffer2.frag.hlsl", "main", "main" });
    //Singleton<ShaderManager>::instance().AddShader("Shadow Shader", { "glsl/shadow.vert.glsl", "glsl/shadow.frag.glsl" });
    Singleton<ShaderManager>::instance().AddShader("Shadow Shader", { "hlsl/shadow.vert.hlsl", "hlsl/shadow.frag.hlsl" });
    //Singleton<ShaderManager>::instance().AddShader("Shading Shader", { "hlsl/lighting_pbr.vert.hlsl", "hlsl/lighting_pbr_packed.frag.hlsl" }, true);
    //Singleton<ShaderManager>::instance().AddShader("Shading Shader", { "hlsl/lighting_pbr.vert.hlsl", "hlsl/lighting_pbr_packed2.frag.hlsl" }, true);
    Singleton<ShaderManager>::instance().AddShader("Shading Shader", { "hlsl/lighting_pbr.vert.hlsl", "hlsl/ssr_v2/pbr.frag.hlsl" });
    Singleton<ShaderManager>::instance().AddShader("SSR Shader", { "hlsl/ssr_v2/ssr.comp.hlsl" }, true);
    Singleton<ShaderManager>::instance().AddShader("SSR2 Shader", { "hlsl/lighting_pbr.vert.hlsl", "hlsl/ssr_v2/ssr.frag.hlsl" });
    Singleton<ShaderManager>::instance().AddShader("Composite Shader", { "hlsl/lighting_pbr.vert.hlsl", "hlsl/ssr_v2/composite.frag.hlsl" }, true);

    Singleton<ShaderManager>::instance().AddShader("FrustumCulling Shader", { "hlsl/culling/FrustumCulling.comp.hlsl" });
    Singleton<ShaderManager>::instance().AddShader("ResetIndirectDrawBuffer Shader", { "hlsl/culling/ResetIndirectDrawBuffer.comp.hlsl" });}

void SSR::createStorageBuffers()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        auto indirectDrawCommands = m_scene->GetIndirectDrawCommands();
        std::vector<IndirectDrawArgs> indirectDrawArgs(indirectDrawCommands.size());
        for (auto i = 0; i < indirectDrawCommands.size(); i++) {
            indirectDrawArgs[i].indexCount = indirectDrawCommands[i].indexCount;
            indirectDrawArgs[i].instanceCount = indirectDrawCommands[i].instanceCount;
            indirectDrawArgs[i].firstIndex = indirectDrawCommands[i].firstIndex;
            indirectDrawArgs[i].vertexOffset = indirectDrawCommands[i].vertexOffset;
            indirectDrawArgs[i].firstInstance = indirectDrawCommands[i].firstInstance;
        }

        BufferCreateInfo info{};
        info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
        info.arrayLength = 1;
        info.size = sizeof(IndirectDrawArgs) * indirectDrawCommands.size();

        m_indirectDrawBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, indirectDrawArgs.data());

        info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
        m_culledIndirectDrawBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info);

        std::vector<InstanceBound> bounds = m_scene->GetBounds();
        info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        info.size = sizeof(InstanceBound) * bounds.size();
        m_instanceBoundsBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, bounds.data());

        //std::vector<HizDimension> hizDimensions(m_hiz.hizRes.size());
        //for (int i = 0; i < hizDimensions.size(); i++) {
        //    hizDimensions[i].u_previousLevelDimensions = m_hiz.hizRes[i];
        //}
        //
        //info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        //info.size = sizeof(HizDimension) * hizDimensions.size();
        //m_hizDimensionsBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, hizDimensions.data());
        //
        //info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        //info.size = sizeof(HIZBuffer);
        //m_hizBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info);

        //std::vector<glm::mat4> models(m_scene->GetIndirectDrawCommands().size());
        //for (auto i = 0; i < models.size(); i++) {
        //    models[i] = glm::mat4(1.0f);
        //}
        std::vector<glm::mat4> models = m_scene->GetTransforms();
        info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        info.size = sizeof(glm::mat4) * models.size();
        m_modelBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, models.data());

        std::vector<int> materialIds = m_scene->GetMaterialsIds();
        info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        info.size = sizeof(int) * materialIds.size();
        m_materialIdBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, materialIds.data());

        std::vector<MaterialBuffer> materials = m_scene->GetMaterials();
        info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        info.size = sizeof(MaterialBuffer) * materials.size();
        m_materialBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, materials.data());

        info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        info.size = sizeof(uint32_t);
        m_numIndirectDrawBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info);

        auto numInstances = models.size();
        std::vector<int> indirectInstances(numInstances);
        for (auto i = 0; i < numInstances; i++) {
            indirectInstances[i] = i;
        }
        info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        info.size = sizeof(uint32_t) * indirectInstances.size();
        m_indirectInstanceBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, indirectInstances.data());

        numTotalInstances = numInstances;
    }
}

void SSR::createSyncObjs()
{
    m_cullingSemaphore.Create(Singleton<MVulkanEngine>::instance().GetDevice().GetDevice());
    //m_genHizSemaphore.Create(Singleton<MVulkanEngine>::instance().GetDevice().GetDevice());
    m_ssrSemaphore.Create(Singleton<MVulkanEngine>::instance().GetDevice().GetDevice());
    m_basicShadingSemaphore.Create(Singleton<MVulkanEngine>::instance().GetDevice().GetDevice());
    //m_shadowMapSemaphore.Create(Singleton<MVulkanEngine>::instance().GetDevice().GetDevice());
}

void SSR::initUIRenderer()
{
    m_uiRenderer = std::make_shared<SSRUI>();
}

void SSR::createGbufferPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        RenderPassCreateInfo info{};
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_UINT);
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_UINT);
        //info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        //info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        //info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        info.pipelineCreateInfo.depthAttachmentFormats = device.FindDepthFormat();
        info.frambufferCount = 1;
        info.dynamicRender = 1;

        m_gbufferPass = std::make_shared<RenderPass>(device, info);
        auto shader = Singleton<ShaderManager>::instance().GetShader<ShaderModule>("GBuffer Shader");

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_gbufferPass, shader);

        //std::vector<VkImageView> bufferTextureViews = Singleton<TextureManager>::instance().GenerateTextureViews();
        std::vector<std::shared_ptr<MVulkanTexture>> gbufferTextures = Singleton<TextureManager>::instance().GenerateTextures();

        std::vector<PassResources> resources;
        resources.push_back(
            PassResources::SetBufferResource(
                "vpBuffer", 0, 0));
        resources.push_back(
            PassResources::SetBufferResource(
                "vpBuffer_p", 1, 0));
        resources.push_back(
            PassResources::SetBufferResource(
                2, 0, m_modelBuffer));
        resources.push_back(
            PassResources::SetBufferResource(
                3, 0, m_materialBuffer));
        resources.push_back(
            PassResources::SetBufferResource(
                7, 0, m_materialIdBuffer));
        resources.push_back(
            PassResources::SetBufferResource(
                8, 0, m_indirectInstanceBuffer));
        //resources.push_back(
        //    PassResources::SetBufferResource(
        //        "texBuffer", 3, 0));
        resources.push_back(
            PassResources::SetBufferResource(
                "gBufferInfoBuffer", 6, 0, 0));

        resources.push_back(
            PassResources::SetSampledImageResource(
                4, 0, gbufferTextures));
        resources.push_back(
            PassResources::SetSamplerResource(
                5, 0, m_linearSampler.GetSampler()));

        m_gbufferPass->UpdateDescriptorSetWrite(0, resources);
    }
}

void SSR::createShadowPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        RenderPassCreateInfo info{};
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_UINT);
        info.pipelineCreateInfo.depthAttachmentFormats = device.FindDepthFormat();
        info.frambufferCount = 1;
        info.dynamicRender = 1;

        m_shadowPass = std::make_shared<RenderPass>(device, info);

        auto shadowShader = Singleton<ShaderManager>::instance().GetShader<ShaderModule>("Shadow Shader");

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_shadowPass, shadowShader);

        std::vector<PassResources> resources;
        resources.push_back(
            PassResources::SetBufferResource(
                "shadowVpBuffer", 0, 0));
        resources.push_back(
            PassResources::SetBufferResource(
                1, 0, m_modelBuffer));

        m_shadowPass->UpdateDescriptorSetWrite(0, resources);
    }
}

void SSR::createShadingPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        RenderPassCreateInfo info{};
        //info.pipelineCreateInfo.colorAttachmentFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        //info.pipelineCreateInfo.depthAttachmentFormats = device.FindDepthFormat();
        //info.numFrameBuffers = 1;
        //info.frambufferCount = Singleton<MVulkanEngine>::instance().GetSwapchainImageCount();
        info.frambufferCount = 1;
        info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
        info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
        info.dynamicRender = 1;
        //info.depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();
        info.useDepthBuffer = false;

        m_lightingPass = std::make_shared<RenderPass>(device, info);

        auto shader = Singleton<ShaderManager>::instance().GetShader<ShaderModule>("Shading Shader");

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_lightingPass, shader);


        std::vector<std::shared_ptr<MVulkanTexture>> shadowViews{
            shadowMapDepth,
            shadowMapDepth
        };
        //std::vector<VkImageView> shadowViews{
        //    shadowMapDepth->GetImageView(),
        //    shadowMapDepth->GetImageView()
        //};

        //for (int i = 0; i < info.frambufferCount; i++) {
        std::vector<PassResources> resources;

        resources.push_back(
            PassResources::SetBufferResource(
                "lightBuffer", 0, 0, 0));
        resources.push_back(
            PassResources::SetBufferResource(
                "cameraBuffer", 1, 0, 0));
        resources.push_back(
            PassResources::SetBufferResource(
                "screenBuffer", 2, 0, 0));
        resources.push_back(
            PassResources::SetSampledImageResource(
                3, 0, gBuffer0));
        resources.push_back(
            PassResources::SetSampledImageResource(
                4, 0, gBuffer1));
        //resources.push_back(
        //    PassResources::SetSampledImageResource(
        //        8, 0, gBuffer2));
        //
        resources.push_back(
            PassResources::SetSamplerResource(
                6, 0, m_linearSampler.GetSampler()));

        resources.push_back(
            PassResources::SetSampledImageResource(
                5, 0, shadowViews));

        resources.push_back(
            PassResources::SetBufferResource(
                "intBuffer", 7, 0, 0));


        m_lightingPass->UpdateDescriptorSetWrite(0, resources);
        //}
    }
}

void SSR::createSSRPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        m_ssrPass = std::make_shared<ComputePass>(device);

        auto shader = Singleton<ShaderManager>::instance().GetShader<ComputeShaderModule>("SSR Shader");

        Singleton<MVulkanEngine>::instance().CreateComputePass(
            m_ssrPass, shader);

        std::vector<PassResources> resources;

        //PassResources resource;
        resources.push_back(PassResources::SetBufferResource("cameraBuffer", 0, 0, 0));
        resources.push_back(PassResources::SetBufferResource("vpBuffer", 1, 0, 0));
        resources.push_back(PassResources::SetBufferResource("hizDimensionBuffer", 2, 0, 0));
        resources.push_back(PassResources::SetBufferResource("ssrBuffer", 3, 0, 0));
        resources.push_back(PassResources::SetBufferResource("screenBuffer", 4, 0, 0));
        resources.push_back(PassResources::SetSampledImageResource(5, 0, gBuffer0));
        resources.push_back(PassResources::SetSampledImageResource(6, 0, gBuffer1));
        //resources.push_back(PassResources::SetSampledImageResource(7, 0, m_hizTextures[0]));
        resources.push_back(PassResources::SetSampledImageResource(7, 0, m_hiz.GetHizTexture()));
        resources.push_back(PassResources::SetSampledImageResource(8, 0, m_basicShadingTexture));
        resources.push_back(PassResources::SetStorageImageResource(9, 0, m_ssrTexture));
        resources.push_back(PassResources::SetSamplerResource(10, 0, m_linearSampler.GetSampler()));

        m_ssrPass->UpdateDescriptorSetWrite(resources);
    }

    //{
    //    RenderPassCreateInfo info{};
    //    info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
    //    info.frambufferCount = 1;
    //    info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
    //    info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
    //    info.dynamicRender = 1;
    //    info.useDepthBuffer = false;
    //
    //    m_ssrPass2 = std::make_shared<RenderPass>(device, info);
    //
    //    auto shader = Singleton<ShaderManager>::instance().GetShader<ShaderModule>("SSR2 Shader");
    //
    //    Singleton<MVulkanEngine>::instance().CreateRenderPass(
    //        m_ssrPass2, shader);
    //
    //    std::vector<PassResources> resources;
    //
    //    //PassResources resource;
    //    resources.push_back(PassResources::SetBufferResource("cameraBuffer", 0, 0, 0));
    //    resources.push_back(PassResources::SetBufferResource("vpBuffer", 1, 0, 0));
    //    resources.push_back(PassResources::SetBufferResource("hizDimensionBuffer", 2, 0, 0));
    //    resources.push_back(PassResources::SetBufferResource("ssrBuffer", 3, 0, 0));
    //    resources.push_back(PassResources::SetBufferResource("screenBuffer", 4, 0, 0));
    //    resources.push_back(PassResources::SetSampledImageResource(5, 0, gBuffer0));
    //    resources.push_back(PassResources::SetSampledImageResource(6, 0, gBuffer1));
    //    //resources.push_back(PassResources::SetSampledImageResource(7, 0, m_hizTextures[0]));
    //    resources.push_back(PassResources::SetSampledImageResource(7, 0, m_hiz.GetHizTexture(0)));
    //    resources.push_back(PassResources::SetSampledImageResource(8, 0, m_basicShadingTexture));
    //    resources.push_back(PassResources::SetStorageImageResource(9, 0, m_ssrTexture));
    //    resources.push_back(PassResources::SetSamplerResource(10, 0, m_linearSampler.GetSampler()));
    //    //resources.push_back(PassResources::SetSampledImageResource(11, 0, m_hizTextures));
    //    //std::vector<std::shared_ptr<MVulkanTexture>> textures = m_hiz.GetHizTextures();
    //    //resources.push_back(PassResources::SetSampledImageResource(11, 0, textures));
    //
    //    m_ssrPass2->UpdateDescriptorSetWrite(0, resources);
    //}
}

void SSR::createCompositePass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {

        //RenderPassCreateInfo info{};
        RenderPassCreateInfo info{};
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());
        info.pipelineCreateInfo.depthAttachmentFormats = device.FindDepthFormat();
        //info.numFrameBuffers = 1;
        info.frambufferCount = Singleton<MVulkanEngine>::instance().GetSwapchainImageCount();
        info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
        info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
        info.dynamicRender = 1;
        //info.depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();

        m_compositePass = std::make_shared<RenderPass>(device, info);

        auto shader = Singleton<ShaderManager>::instance().GetShader<ShaderModule>("Composite Shader");

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_compositePass, shader);

        for (int i = 0; i < info.frambufferCount; i++) {
            std::vector<PassResources> resources;

            resources.push_back(
                PassResources::SetBufferResource(
                    "screenBuffer", 0, 0, i));
            resources.push_back(
                PassResources::SetBufferResource(
                    "ssrConfigBuffer", 1, 0, i));
            resources.push_back(
                PassResources::SetSampledImageResource(
                    2, 0, m_basicShadingTexture));
            resources.push_back(
                PassResources::SetSampledImageResource(
                    3, 0, m_ssrTexture));
            //resources.push_back(
            //    PassResources::SetSampledImageResource(
            //        3, 0, m_ssrTexture_frag));
            
            m_compositePass->UpdateDescriptorSetWrite(i, resources);
        }
    }
}

void SSR::createFrustumCullingPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        m_frustumCullingPass = std::make_shared<ComputePass>(device);

        auto shader = Singleton<ShaderManager>::instance().GetShader<ComputeShaderModule>("FrustumCulling Shader");

        Singleton<MVulkanEngine>::instance().CreateComputePass(
            m_frustumCullingPass, shader);

        std::vector<PassResources> resources;

        //PassResources resource;
        resources.push_back(PassResources::SetBufferResource("FrustumBuffer", 0, 0, 0));
        resources.push_back(PassResources::SetBufferResource("sceneBuffer", 4, 0, 0));
        resources.push_back(PassResources::SetBufferResource(1, 0, m_instanceBoundsBuffer));
        resources.push_back(PassResources::SetBufferResource(2, 0, m_indirectDrawBuffer));
        resources.push_back(PassResources::SetBufferResource(3, 0, m_culledIndirectDrawBuffer));
        resources.push_back(PassResources::SetBufferResource(5, 0, m_numIndirectDrawBuffer));
        resources.push_back(PassResources::SetBufferResource(6, 0, m_indirectInstanceBuffer));

        m_frustumCullingPass->UpdateDescriptorSetWrite(resources);
    }
}

void SSR::createResetDidirectDrawBufferPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        m_resetDidirectDrawBufferPass = std::make_shared<ComputePass>(device);

        auto shader = Singleton<ShaderManager>::instance().GetShader<ComputeShaderModule>("ResetIndirectDrawBuffer Shader");

        Singleton<MVulkanEngine>::instance().CreateComputePass(
            m_resetDidirectDrawBufferPass, shader);

        std::vector<PassResources> resources;

        //PassResources resource;
        resources.push_back(PassResources::SetBufferResource(0, 0, m_numIndirectDrawBuffer));

        m_resetDidirectDrawBufferPass->UpdateDescriptorSetWrite(resources);
    }
}

void SSR::initHiz()
{
    m_hiz = Hiz();

    auto gbufferExtent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
    glm::ivec2 res = { gbufferExtent.width, gbufferExtent.height };

    m_hiz.Init(res, gBufferDepth);
}

static const char* OutputBufferContents[] = {
    "Render",
    "Depth",
    "Albedo",
    "Normal",
    "Position",
    "Roughness",
    "Metallic",
    "MotionVector",
    "ShadowMap",
};

void SSRUI::RenderContext() {
    ImGui::Begin("DR_UI Window", &shouleRenderUI);
    ImGui::Text("Hello from another window!");

    double fps = this->m_app->GetFPS();
    ImGui::Text("FPS: %.2f", fps);

    ImGui::RadioButton("cullingmode null", &cullingMode, CULLINGMODE_NULL); ImGui::SameLine();
    ImGui::RadioButton("cullingmode sphere", &cullingMode, CULLINGMODE_SPHERE); ImGui::SameLine();
    ImGui::RadioButton("cullingmode bbx", &cullingMode, CULLINGMODE_AABB);

    //ImGui::RadioButton("not do hiz", &hizMode, NOT_DO_HIZ); ImGui::SameLine();
    //ImGui::RadioButton("hiz mode 0", &hizMode, DO_HIZ_MODE_0);// ImGui::SameLine();
    //ImGui::RadioButton("hiz mode 1", &hizMode, DO_HIZ_MODE_1);

    ImGui::Combo("outputContext", &outputContext, OutputBufferContents, IM_ARRAYSIZE(OutputBufferContents));

    auto app = (SSR*)(this->m_app);
    auto camera = app->GetMainCamera();
    //auto camera = app->GetLightCamera();
    auto& cameraPosition = camera->GetPositionRef();
    auto& cameraDirection = camera->GetDirectionRef();

    ImGui::DragFloat3("Camera Position:", &cameraPosition[0]);
    ImGui::DragFloat3("Camera Direction:", &cameraDirection[0]);

    auto& cameraVelocity = Singleton<InputManager>::instance().m_cameraVelocity;
    ImGui::InputFloat("Camera Velocity:", &cameraVelocity, 0.001f, 0, "%.4f");

    auto numTotalInstances = ((SSR*)(this->m_app))->numTotalInstances;
    auto numDrawInstances = ((SSR*)(this->m_app))->numDrawInstances;
    ImGui::Text("Total Instance Count: %d, Drawed Instance Count: %d", numTotalInstances, numDrawInstances);

    ImGui::Checkbox("doSSR", &doSSR);

    ImGui::Checkbox("showPassTime", &showPassTime);
    if (showPassTime) {
        ImGui::Text("culling time: %.3f ms", ((SSR*)(this->m_app))->cullingTime);
        ImGui::Text("shadowmap time: %.3f ms", ((SSR*)(this->m_app))->shadowmapTime);
        ImGui::Text("gbuffer time: %.3f ms", ((SSR*)(this->m_app))->gbufferTime);
        ImGui::Text("shading time: %.3f ms", ((SSR*)(this->m_app))->shadingTime);
        ImGui::Text("hiz time: %.3f ms", ((SSR*)(this->m_app))->hizTime);
        ImGui::Text("ssr time: %.3f ms", ((SSR*)(this->m_app))->ssrTime);
        ImGui::Text("composite time: %.3f ms", ((SSR*)(this->m_app))->compositeTime);
        //auto hizTimes = ((SSR*)(this->m_app))->hizTimes;
        //for (auto i = 0; i < hizTimes.size(); i++) {
        //    ImGui::Text("hiz layer %d time: %.3f ms", i, ((SSR*)(this->m_app))->hizTimes[i]);
        //}
    }
    //ImGui::InputInt("Hiz Layer:", &hizLayer, 1, 0);

    ImGui::End();
}