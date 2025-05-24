#include "pbr.hpp"

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

void PBR::SetUp()
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

void PBR::ComputeAndDraw(uint32_t imageIndex)
{
    //spdlog::info("draw start");

    m_depthIndex++;
    {
        std::vector<PassResources> resources;
        resources.push_back(PassResources::SetSampledImageResource(3, 0, getDepthPrev()));
        resources.push_back(PassResources::SetStorageImageResource(4, 0, getDepthCurrent()));
        m_reprojectDepthPass->UpdateDescriptorSetWrite(resources);
    
        m_hiz.SetSourceDepth2(getDepthCurrent());
    }

    {
        std::vector<PassResources> resources;
        resources.push_back(PassResources::SetSampledImageResource(3, 0, getShadowDepthPrev()));
        resources.push_back(PassResources::SetStorageImageResource(4, 0, getShadowDepthCurrent()));
        m_reprojectShadowmapDepthPass->UpdateDescriptorSetWrite(resources);

        m_shadowmapHiz.SetSourceDepth2(getShadowDepthCurrent());
    }

    {
        std::vector<PassResources> resources;
        resources.push_back(PassResources::SetBufferResource("screenBuffer", 2, 0, imageIndex));
        m_reprojectDepthPass->UpdateDescriptorSetWrite(resources);
    }

    auto frustumCullingMode = std::static_pointer_cast<DRUI>(m_uiRenderer)->frustumCullingMode;
    auto doHizCulling = std::static_pointer_cast<DRUI>(m_uiRenderer)->doHizCulling;
    auto cullShadowmap = std::static_pointer_cast<DRUI>(m_uiRenderer)->cullShadowmap;

    m_queryIndex = 0;
    int shadowmapQueryIndex = 0;
    int gbufferQueryIndex = 0;
    int shadingQueryIndex = 0;
    int cullingQueryIndex = 0;
    //int cullingQueryInedxEnd;
    int reprojectDepthIndex = 0;
    int hizQueryIndexStart = 0;
    int hizQueryIndexEnd = 0;
    std::vector<int> hizQueryIndex(0);
    //Singleton<MVulkanEngine>::instance().ResetTimeStampQueryPool();

    auto& graphicsList = Singleton<MVulkanEngine>::instance().GetGraphicsList(m_currentFrame);
    auto& graphicsQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::GRAPHICS);

    auto& computeList = Singleton<MVulkanEngine>::instance().GetComputeCommandList();
    auto& computeQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::COMPUTE);

    //ImageLayoutToAttachment(imageIndex);
    auto swapchainExtent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();

    //if()

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
    }
    {
        VPBuffer vpBuffer{};
        vpBuffer.View = m_directionalLightCamera->GetViewMatrix();
        vpBuffer.Projection = m_directionalLightCamera->GetOrthoMatrix();
        Singleton<ShaderResourceManager>::instance().LoadData("shadowmapVpBuffer", 0, &vpBuffer, 0);

        VPBuffer vpBuffer_p{};
        vpBuffer_p.View = m_prevShadowView;
        vpBuffer_p.Projection = m_prevShadowProj;
        Singleton<ShaderResourceManager>::instance().LoadData("shadowmapVpBuffer_p", 0, &vpBuffer_p, 0);
    }
    {
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

        MScreenBuffer screenBuffer{};
        screenBuffer.WindowRes = int2(swapchainExtent.width, swapchainExtent.height);

        int outputContext = std::static_pointer_cast<DRUI>(m_uiRenderer)->outputContext;

        Singleton<ShaderResourceManager>::instance().LoadData("lightBuffer", imageIndex, &lightBuffer, 0);
        Singleton<ShaderResourceManager>::instance().LoadData("cameraBuffer", imageIndex, &cameraBuffer, 0);
        Singleton<ShaderResourceManager>::instance().LoadData("screenBuffer", imageIndex, &screenBuffer, 0);
        Singleton<ShaderResourceManager>::instance().LoadData("intBuffer", imageIndex, &outputContext, 0);
        //m_lightingPass->GetShader()->SetUBO(0, &ubo0);
    }

    //prepare lightingPass ubo
    {
        MScreenBuffer screenBuffer{};
        screenBuffer.WindowRes = int2(shadowmapExtent.width, shadowmapExtent.height);

        Singleton<ShaderResourceManager>::instance().LoadData("shadowmapScreenBuffer", 0, &screenBuffer, 0);
    }

    {
        auto frustum = m_camera->GetFrustum();
        FrustumBuffer buffer;
        for (int i = 0; i < 6; i++) {
            buffer.planes[i] = frustum.planes[i];
        }

        //MSceneBuffer sceneBuffer;
        //sceneBuffer.numInstances = m_scene->GetIndirectDrawCommands().size();
        //sceneBuffer.targetIndex = 0;

        MCullingBuffer cullingBuffer;
        cullingBuffer.doFrustumCulling = frustumCullingMode > 0 ? 1 : 0;
        cullingBuffer.doHizCulling = doHizCulling ? 1 : 0;
        cullingBuffer.frustumCullingMode = frustumCullingMode;
        cullingBuffer.cullingCountCalculate = std::static_pointer_cast<DRUI>(m_uiRenderer)->calculateDrawNums ? 1 : 0;

        MCullingBuffer shadowmapCullingBuffer;
        //if (m_firstFrame) {
        //    shadowmapCullingBuffer.doFrustumCulling = 0;
        //    shadowmapCullingBuffer.doHizCulling = 0;
        //    shadowmapCullingBuffer.frustumCullingMode = 0;
        //    shadowmapCullingBuffer.cullingCountCalculate = 0;
        //}
        //else {
        shadowmapCullingBuffer.doFrustumCulling = 0;
        shadowmapCullingBuffer.doHizCulling = 1;
        shadowmapCullingBuffer.frustumCullingMode = 0;
        shadowmapCullingBuffer.cullingCountCalculate = std::static_pointer_cast<DRUI>(m_uiRenderer)->calculateDrawNums ? 1 : 0;
        //}

        Singleton<ShaderResourceManager>::instance().LoadData("FrustumBuffer", 0, &buffer, 0);
        //Singleton<ShaderResourceManager>::instance().LoadData("sceneBuffer", 0, &sceneBuffer, 0);
        Singleton<ShaderResourceManager>::instance().LoadData("cullingBuffer", 0, &cullingBuffer, 0);
        Singleton<ShaderResourceManager>::instance().LoadData("shadowmapCullingBuffer", 0, &shadowmapCullingBuffer, 0);
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

        Singleton<ShaderResourceManager>::instance().LoadData("hizDimensionBuffer", 0, &hizDimensionBuffer, 0);
    }

    {
        HizDimensionBuffer shadowHizDimensionBuffer;
        auto hizLayers = m_shadowmapHiz.GetHizLayers();
        for (auto i = 0; i < hizLayers; i++) {
            auto res = m_shadowmapHiz.GetHizRes(i);
            shadowHizDimensionBuffer.hizDimensions[i].x = res.x;
            shadowHizDimensionBuffer.hizDimensions[i].y = res.y;
        }
        shadowHizDimensionBuffer.hizDimensions[0].z = hizLayers;

        Singleton<ShaderResourceManager>::instance().LoadData("shadowmapHizDimensionBuffer", 0, &shadowHizDimensionBuffer, 0);
    }

    RenderingInfo gbufferRenderInfo, shadowmapRenderInfo, shadingRenderInfo; 
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
        //gbufferRenderInfo.colorAttachments.push_back(
        //    RenderingAttachment{
        //        .texture = gBuffer2,
        //        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        //    }
        //);
        //gbufferRenderInfo.colorAttachments.push_back(
        //    RenderingAttachment{
        //        .texture = gBuffer3,
        //        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        //    }
        //);
        //gbufferRenderInfo.colorAttachments.push_back(
        //    RenderingAttachment{
        //        .texture = gBuffer4,
        //        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        //    }
        //);

        gbufferRenderInfo.depthAttachment = RenderingAttachment{
                .texture = getDepthCurrent(),
                .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                .view = nullptr,
                //.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
                //.storeOp = VK_ATTACHMENT_STORE_OP_STORE
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
                .texture = getShadowDepthCurrent(),
                .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };
    }

    {
        //auto swapChainImages = Singleton<MVulkanEngine>::instance().GetSwa
        auto swapChain = Singleton<MVulkanEngine>::instance().GetSwapchain();

        shadingRenderInfo.colorAttachments.push_back(
            RenderingAttachment{
                .texture = nullptr,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .view = swapChain.GetImageView(imageIndex),
            }
        );
        shadingRenderInfo.depthAttachment = RenderingAttachment{
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

    //culling
    {
        computeList.GetFence().WaitForSignal();
        computeList.GetFence().Reset();
        computeList.Reset();
        computeList.Begin();
        Singleton<MVulkanEngine>::instance().CmdResetTimeStampQueryPool(computeList);

        if (doHizCulling || frustumCullingMode > 0) {
            reprojectDepthIndex = m_queryIndex;
            Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_reprojectDepthPass, computeList, (swapchainExtent.width + 15) / 16, (swapchainExtent.height + 15) / 16, 1, std::string("ReprojectDepth Pass"), m_queryIndex++);

            m_hiz.Generate2(computeList, m_queryIndex);
            hizQueryIndexStart = m_hiz.GetHizQueryIndexStart();
            hizQueryIndexEnd = m_hiz.GetHizQueryIndexEnd();
        }

        Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_resetIndirectDrawBufferPass, computeList, 1, 1, 1, std::string("Reset NumIndirectDrawBuffer Pass"), m_queryIndex++);
        auto numInstances = m_scene->GetIndirectDrawCommands().size();
        cullingQueryIndex = m_queryIndex;

        Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_cullingPass, computeList, numInstances, 1, 1, std::string("Culling Pass"), m_queryIndex++);
        //computeList.End();

        //shadowmap culling
        if (cullShadowmap)
        {
            Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_reprojectShadowmapDepthPass, computeList, (shadowmapExtent.width + 15) / 16, (shadowmapExtent.height + 15) / 16, 1, std::string("Reproject Shadowmap Depth Pass"), m_queryIndex++);

            m_shadowmapHiz.Generate2(computeList, m_queryIndex);

            Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_resetShadowmapIndirectDrawBufferPass, computeList, 1, 1, 1, std::string("Reset Shadowmap NumIndirectDrawBuffer Pass"), m_queryIndex++);
            auto numInstances = m_scene->GetIndirectDrawCommands().size();
            Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_shadowmapCullingPass, computeList, numInstances, 1, 1, std::string("Shadowmap Culling Pass"), m_queryIndex++);
        }

        computeList.End();

        std::vector<MVulkanSemaphore> waitSemaphores2(0);
        std::vector<MVulkanSemaphore> signalSemaphores2(1, m_cullingSemaphore);
        std::vector<VkPipelineStageFlags> waitFlags2(0);
        Singleton<MVulkanEngine>::instance().SubmitCommands(computeList, computeQueue, waitSemaphores2, waitFlags2, signalSemaphores2);
    }


    numDrawInstances = *(uint32_t*)m_numIndirectDrawBuffer->GetMappedData();
    numShadowmapDrawInstances = *(uint32_t*)m_shadowmapNumIndirectDrawBuffer->GetMappedData();

    graphicsList.GetFence().WaitForSignal();
    graphicsList.GetFence().Reset();
    graphicsList.Reset();
    graphicsList.Begin();

    shadowmapQueryIndex = m_queryIndex;
    //Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_shadowPass, m_currentFrame, shadowmapRenderInfo, m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), m_scene->GetIndirectBuffer(), m_scene->GetIndirectDrawCommands().size(), std::string("Shadowmap Pass"), m_queryIndex++);
    if (cullShadowmap)
        Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_shadowPass, m_currentFrame, shadowmapRenderInfo, m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), m_shadowmapCulledIndirectDrawBuffer, m_scene->GetIndirectDrawCommands().size(), std::string("Shadowmap Pass"), m_queryIndex++);
    else
        Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_shadowPass, m_currentFrame, shadowmapRenderInfo, m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), m_scene->GetIndirectBuffer(), m_scene->GetIndirectDrawCommands().size(), std::string("Shadowmap Pass"), m_queryIndex++);
    gbufferQueryIndex = m_queryIndex;
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_gbufferPass, m_currentFrame, gbufferRenderInfo, m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), m_culledIndirectDrawBuffer, m_scene->GetIndirectDrawCommands().size(), std::string("Gbuffer Pass"), m_queryIndex++);
    shadingQueryIndex = m_queryIndex;
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(imageIndex, m_lightingPass, m_currentFrame, shadingRenderInfo, m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size(), std::string("Shading Pass"), m_queryIndex++);

    graphicsList.End();

    std::vector<MVulkanSemaphore> waitSemaphores1(1, m_cullingSemaphore);
    std::vector<MVulkanSemaphore> signalSemaphores1(0);
    std::vector<VkPipelineStageFlags> waitFlags1(1, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    Singleton<MVulkanEngine>::instance().SubmitGraphicsCommands(imageIndex, m_currentFrame, waitSemaphores1, waitFlags1, signalSemaphores1);
   
    m_prevView = m_camera->GetViewMatrix();
    m_prevProj = m_camera->GetProjMatrix();

    m_prevShadowView = m_directionalLightCamera->GetViewMatrix();
    m_prevShadowProj = m_directionalLightCamera->GetOrthoMatrix();

    //return;
    auto showPassTime = std::static_pointer_cast<DRUI>(m_uiRenderer)->showPassTime;
    if (showPassTime) {
        auto queryResults = Singleton<MVulkanEngine>::instance().GetTimeStampQueryResults(0, 2 * m_queryIndex);

        auto timestampPeriod = Singleton<MVulkanEngine>::instance().GetTimeStampPeriod();

        cullingTime = (queryResults[2 * cullingQueryIndex + 1] - queryResults[2 * cullingQueryIndex]) * timestampPeriod / 1000000.f;
        gbufferTime = (queryResults[2 * gbufferQueryIndex + 1] - queryResults[2 * gbufferQueryIndex]) * timestampPeriod / 1000000.f;
        shadowmapTime = (queryResults[2 * shadowmapQueryIndex + 1] - queryResults[2 * shadowmapQueryIndex]) * timestampPeriod / 1000000.f;
        shadingTime = (queryResults[2 * shadingQueryIndex + 1] - queryResults[2 * shadingQueryIndex]) * timestampPeriod / 1000000.f;
        if (doHizCulling) {
            hizTime = (queryResults[2 * hizQueryIndexEnd - 1] - queryResults[2 * hizQueryIndexStart]) * timestampPeriod / 1000000.f;
            reprojectDepthTime = (queryResults[2 * reprojectDepthIndex + 1] - queryResults[2 * reprojectDepthIndex]) * timestampPeriod / 1000000.f;
        }
        else {
            hizTime = 0.f;
            reprojectDepthTime = (queryResults[2 * reprojectDepthIndex + 1] - queryResults[2 * reprojectDepthIndex]) * timestampPeriod / 1000000.f;
        }
    }
    else {
        reprojectDepthTime = 0.f;
        cullingTime = 0.f;
        gbufferTime = 0.f;
        shadowmapTime = 0.f;
        shadingTime = 0.f;
        hizTime = 0.f;
    }

    if (m_firstFrame) {
        m_firstFrame = false;
    }

    //spdlog::info("draw end");
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
    createResetDidirectDrawBufferPass();
    createFrustumCullingPass();
    createReprojectDepthPass();
    //createGenHizPass();
    //createResetHizBufferPass();
    //createUpdateHizBufferPass();
    

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
    fs::path arcadePath = resourcePath / "Arcade" / "Arcade.gltf";
    fs::path bistroPath = resourcePath / "Bistro" / "bistro.gltf";
    //fs::path modelPath = resourcePath / "San_Miguel" / "san-miguel-low-poly.obj";
    //fs::path modelPath = resourcePath / "shapespark_example_room" / "shapespark_example_room.gltf";

    Singleton<SceneLoader>::instance().Load(bistroPath.string(), m_scene.get());

    //split Image
    {
        //auto wholeTextures2 = Singleton<TextureManager>::instance().GenerateTextureVector();
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

    //createStorageBuffers();
    
    //{
    //    std::vector<PassResources> resources;
    //
    //    //PassResources resource;
    //    //resources.push_back(PassResources::SetBufferResource("FrustumBuffer", 0, 0, 0));
    //    resources.push_back(PassResources::SetBufferResource(0, 1, m_instanceBoundsBuffer));
    //    resources.push_back(PassResources::SetBufferResource(0, 2, m_indirectDrawBuffer));
    //    resources.push_back(PassResources::SetBufferResource(0, 3, m_culledIndirectDrawBuffer));
    //
    //    m_frustumCullingPass->UpdateDescriptorSetWrite(resources);
    //}
}

void PBR::createLight()
{
    glm::vec3 direction = glm::normalize(glm::vec3(-1.f, -6.f, -1.f));
    //glm::vec3 direction = glm::normalize(glm::vec3(-2.f, -1.f, 1.f));
    glm::vec3 color = glm::vec3(1.f, 1.f, 1.f);
    //float intensity = 10.f;
    float intensity = 10.f;
    m_directionalLight = std::make_shared<DirectionalLight>(direction, color, intensity);
}

void PBR::createCamera()
{
    //glm::vec3 position(-1.f, 0.f, 4.f);
    //glm::vec3 center(0.f);
    //glm::vec3 direction = glm::normalize(center - position);

    //-4.9944386, 2.9471996, -5.8589
    //glm::vec3 position(-4.9944386, 2.9471996, -5.8589);
    //glm::vec3 direction = glm::normalize(glm::vec3(2.f, -1.f, 2.f));
    //
    //bistro
    glm::vec3 position(-20.f, 3.0f, 19.f);
    glm::vec3 direction = glm::normalize(glm::vec3(0.12f, -0.06f, -0.99f));


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

    //sponza
    //glm::vec3 direction = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetDirection();
    //glm::vec3 position = -50.f * direction;

    //bistro
    glm::vec3 direction = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetDirection();
    glm::vec3 position = glm::vec3(16.015f, 79.726f, 22.634f);

    float fov = 60.f;
    float aspect = (float)extent.width / (float)extent.height;
    float zNear = 0.001f;
    float zFar = 100.f;

    float xMin = -42.f;
    float xMax = 42.f;
    float yMin = -70.f;
    float yMax = 70.f;

    //sponza
    //m_directionalLightCamera = std::make_shared<Camera>(position, direction, fov, aspect, zNear, zFar);
    
    //bistro
    m_directionalLightCamera = std::make_shared<Camera>(position, direction, float3(0.f, 1.f, 0.f), aspect, xMin, xMax, yMin, yMax, zNear, zFar);
    m_directionalLightCamera->SetOrth(true);

    //Singleton<MVulkanEngine>::instance().SetCamera(m_directionalLightCamera);
}

void PBR::createTextures()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();
    auto swapchainExtent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
    auto shadowMapExtent = shadowmapExtent;

    auto depthFormat = device.FindDepthFormat();
    auto shadowMapFormat = VK_FORMAT_R32G32B32A32_UINT;

    {
        shadowMap = std::make_shared<MVulkanTexture>();
        shadowMapDepth[0] = std::make_shared<MVulkanTexture>();
        shadowMapDepth[1] = std::make_shared<MVulkanTexture>();

        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            shadowMap, shadowMapExtent, shadowMapFormat
        );

        Singleton<MVulkanEngine>::instance().CreateDepthAttachmentImage(
            shadowMapDepth[0], shadowMapExtent, depthFormat
        );

        Singleton<MVulkanEngine>::instance().CreateDepthAttachmentImage(
            shadowMapDepth[1], shadowMapExtent, depthFormat
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
        auto motionVectorFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

        gBuffer0 = std::make_shared<MVulkanTexture>();
        gBuffer1 = std::make_shared<MVulkanTexture>();
        //gBuffer2 = std::make_shared<MVulkanTexture>();
        //gBuffer3 = std::make_shared<MVulkanTexture>();
        //gBuffer4 = std::make_shared<MVulkanTexture>();
        gBufferDepth[0] = std::make_shared<MVulkanTexture>();
        gBufferDepth[1] = std::make_shared<MVulkanTexture>();
        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            gBuffer0, swapchainExtent, gbufferFormat
        );

        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            gBuffer1, swapchainExtent, gbufferFormat
        );

        //Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
        //    gBuffer2, swapchainExtent, motionVectorFormat
        //);
        //
        //Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
        //    gBuffer3, swapchainExtent, motionVectorFormat
        //);
        //
        //Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
        //    gBuffer4, swapchainExtent, motionVectorFormat
        //);
        
        Singleton<MVulkanEngine>::instance().CreateDepthAttachmentImage(
            gBufferDepth[0], swapchainExtent, depthFormat
        );
        Singleton<MVulkanEngine>::instance().CreateDepthAttachmentImage(
            gBufferDepth[1], swapchainExtent, depthFormat
        );
    }
}

void PBR::loadShaders() 
{
    //Singleton<ShaderManager>::instance().AddShader("GBuffer Shader", { "hlsl/gbuffer.vert.hlsl", "hlsl/gbuffer/gbuffer.frag.hlsl", "main", "main" });
    Singleton<ShaderManager>::instance().AddShader("GBuffer Shader", { "hlsl/gbuffer/gbuffer2.vert.hlsl", "hlsl/gbuffer/gbuffer2.frag.hlsl", "main", "main" }, false);
    //Singleton<ShaderManager>::instance().AddShader("Shadow Shader", { "glsl/shadow.vert.glsl", "glsl/shadow.frag.glsl" });
    Singleton<ShaderManager>::instance().AddShader("Shadow Shader", { "hlsl/shadow.vert.hlsl", "hlsl/shadow.frag.hlsl" }, false);
    //Singleton<ShaderManager>::instance().AddShader("Shading Shader", { "hlsl/lighting_pbr.vert.hlsl", "hlsl/lighting_pbr_packed.frag.hlsl" }, true);
    Singleton<ShaderManager>::instance().AddShader("Shading Shader", { "hlsl/lighting_pbr.vert.hlsl", "hlsl/lighting_pbr_packed2.frag.hlsl" }, false);
    //Singleton<ShaderManager>::instance().AddShader("FrustumCulling Shader", { "hlsl/culling/FrustumCulling.comp.hlsl" }, false);
    Singleton<ShaderManager>::instance().AddShader("Culling Shader", { "hlsl/culling/Culling.comp.hlsl" }, true);
    //Singleton<ShaderManager>::instance().AddShader("ShadowmapCulling Shader", { "hlsl/culling/ShadowmapCulling.comp.hlsl" }, true);
    Singleton<ShaderManager>::instance().AddShader("ResetIndirectDrawBuffer Shader", { "hlsl/culling/ResetIndirectDrawBuffer.comp.hlsl" }, false);

    Singleton<ShaderManager>::instance().AddShader("ReprojectDepth Shader", { "hlsl/culling/ReprojectDepth.comp.hlsl" }, true);
}

void PBR::createStorageBuffers()
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

        

        info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
        info.arrayLength = 1;
        info.size = sizeof(IndirectDrawArgs) * indirectDrawCommands.size();
        m_shadowmapIndirectDrawBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, indirectDrawArgs.data());

        info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
        m_shadowmapCulledIndirectDrawBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info);

        info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        info.size = sizeof(uint32_t);
        m_shadowmapNumIndirectDrawBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info);

        info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        info.size = sizeof(uint32_t) * indirectInstances.size();
        m_shadowmapIndirectInstanceBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, indirectInstances.data());
    }
}

void PBR::createSyncObjs()
{
    m_cullingSemaphore.Create(Singleton<MVulkanEngine>::instance().GetDevice().GetDevice());
    m_shadingSemaphore.Create(Singleton<MVulkanEngine>::instance().GetDevice().GetDevice());
}

void PBR::initHiz()
{
    m_hiz = Hiz();
    m_shadowmapHiz = Hiz();

    auto gbufferExtent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
    glm::ivec2 res = { gbufferExtent.width, gbufferExtent.height };
    glm::ivec2 shadowmapRes = { shadowmapExtent.width, shadowmapExtent.height };

    m_hiz.Init(res, getDepthCurrent());
    m_shadowmapHiz.Init(shadowmapRes, getShadowDepthCurrent());
}

std::shared_ptr<MVulkanTexture> PBR::getDepthCurrent()
{
    return gBufferDepth[m_depthIndex % 2];
}

std::shared_ptr<MVulkanTexture> PBR::getDepthPrev()
{
    return gBufferDepth[(m_depthIndex + 1) % 2];
}

std::shared_ptr<MVulkanTexture> PBR::getShadowDepthCurrent()
{
    return shadowMapDepth[m_depthIndex % 2];
}

std::shared_ptr<MVulkanTexture> PBR::getShadowDepthPrev()
{
    return shadowMapDepth[(m_depthIndex + 1) % 2];
}

void PBR::initUIRenderer()
{
    m_uiRenderer = std::make_shared<DRUI>();
}

void PBR::createGbufferPass()
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
        //        9, 0, m_depthDebugBuffer));
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

void PBR::createShadowPass()
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

void PBR::createShadingPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        std::vector<VkFormat> lightingPassFormats;
        lightingPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());

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

        m_lightingPass = std::make_shared<RenderPass>(device, info);

        auto shader = Singleton<ShaderManager>::instance().GetShader<ShaderModule>("Shading Shader");

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_lightingPass, shader);


        std::vector<std::shared_ptr<MVulkanTexture>> shadowViews{
            getShadowDepthCurrent(),
            getShadowDepthCurrent()
        };

        for (int i = 0; i < info.frambufferCount; i++) {
            std::vector<PassResources> resources;

            resources.push_back(
                PassResources::SetBufferResource(
                    "lightBuffer", 0, 0, i));
            resources.push_back(
                PassResources::SetBufferResource(
                    "cameraBuffer", 1, 0, i));
            resources.push_back(
                PassResources::SetBufferResource(
                    "screenBuffer", 2, 0, i));
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
                    "intBuffer", 7, 0, i));


            m_lightingPass->UpdateDescriptorSetWrite(i, resources);
        }
    }
}

void PBR::createFrustumCullingPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        m_cullingPass = std::make_shared<ComputePass>(device);

        auto shader = Singleton<ShaderManager>::instance().GetShader<ComputeShaderModule>("Culling Shader");

        Singleton<MVulkanEngine>::instance().CreateComputePass(
            m_cullingPass, shader);

        std::vector<PassResources> resources;

        //PassResources resource;
        resources.push_back(PassResources::SetBufferResource("FrustumBuffer", 0, 0, 0));
        resources.push_back(PassResources::SetBufferResource("cullingBuffer", 1, 0, 0));
        resources.push_back(PassResources::SetBufferResource("vpBuffer", 2, 0, 0));
        resources.push_back(PassResources::SetBufferResource("hizDimensionBuffer", 3, 0, 0));
        resources.push_back(PassResources::SetBufferResource(4, 0, m_instanceBoundsBuffer));
        resources.push_back(PassResources::SetBufferResource(5, 0, m_indirectDrawBuffer));
        resources.push_back(PassResources::SetSampledImageResource(6, 0, m_hiz.GetHizTexture()));
        resources.push_back(PassResources::SetBufferResource(7, 0, m_culledIndirectDrawBuffer));
        resources.push_back(PassResources::SetBufferResource(8, 0, m_indirectInstanceBuffer));
        resources.push_back(PassResources::SetBufferResource(9, 0, m_numIndirectDrawBuffer));

        m_cullingPass->UpdateDescriptorSetWrite(resources);
    }

    {
        m_shadowmapCullingPass = std::make_shared<ComputePass>(device);

        auto shader = Singleton<ShaderManager>::instance().GetShader<ComputeShaderModule>("Culling Shader");

        Singleton<MVulkanEngine>::instance().CreateComputePass(
            m_shadowmapCullingPass, shader);

        std::vector<PassResources> resources;

        //PassResources resource;
        resources.push_back(PassResources::SetBufferResource("FrustumBuffer", 0, 0, 0));
        resources.push_back(PassResources::SetBufferResource("shadowmapCullingBuffer", "cullingBuffer", 1, 0, 0));
        resources.push_back(PassResources::SetBufferResource("shadowmapVpBuffer", "vpBuffer", 2, 0, 0));
        resources.push_back(PassResources::SetBufferResource("shadowmapHizDimensionBuffer", "hizDimensionBuffer", 3, 0, 0));
        resources.push_back(PassResources::SetBufferResource(4, 0, m_instanceBoundsBuffer));
        resources.push_back(PassResources::SetBufferResource(5, 0, m_shadowmapIndirectDrawBuffer));
        resources.push_back(PassResources::SetSampledImageResource(6, 0, m_shadowmapHiz.GetHizTexture()));
        resources.push_back(PassResources::SetBufferResource(7, 0, m_shadowmapCulledIndirectDrawBuffer));
        resources.push_back(PassResources::SetBufferResource(8, 0, m_shadowmapIndirectInstanceBuffer));
        resources.push_back(PassResources::SetBufferResource(9, 0, m_shadowmapNumIndirectDrawBuffer));

        m_shadowmapCullingPass->UpdateDescriptorSetWrite(resources);
    }
}

void PBR::createReprojectDepthPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        m_reprojectDepthPass = std::make_shared<ComputePass>(device);

        auto shader = Singleton<ShaderManager>::instance().GetShader<ComputeShaderModule>("ReprojectDepth Shader");

        Singleton<MVulkanEngine>::instance().CreateComputePass(
            m_reprojectDepthPass, shader);

        std::vector<PassResources> resources;

        //PassResources resource;
        resources.push_back(PassResources::SetBufferResource("vpBuffer", 0, 0, 0));
        resources.push_back(PassResources::SetBufferResource("vpBuffer_p", 1, 0, 0));
        resources.push_back(PassResources::SetBufferResource("screenBuffer", 2, 0, 0));
        //resources.push_back(PassResources::SetSampledImageResource(3, 0, getDepthPrev()));
        //resources.push_back(PassResources::SetStorageImageResource(4, 0, getDepthCurrent()));

        m_reprojectDepthPass->UpdateDescriptorSetWrite(resources);
    }

    {
        m_reprojectShadowmapDepthPass = std::make_shared<ComputePass>(device);

        auto shader = Singleton<ShaderManager>::instance().GetShader<ComputeShaderModule>("ReprojectDepth Shader");

        Singleton<MVulkanEngine>::instance().CreateComputePass(
            m_reprojectShadowmapDepthPass, shader);

        std::vector<PassResources> resources;

        //PassResources resource;
        resources.push_back(PassResources::SetBufferResource("shadowmapVpBuffer", "vpBuffer", 0, 0, 0));
        resources.push_back(PassResources::SetBufferResource("shadowmapVpBuffer_p", "vpBuffer_p", 1, 0, 0));
        resources.push_back(PassResources::SetBufferResource("shadowmapScreenBuffer", "screenBuffer", 2, 0, 0));
        //resources.push_back(PassResources::SetSampledImageResource(3, 0, get()));
        //resources.push_back(PassResources::SetStorageImageResource(4, 0, getDepthCurrent()));

        m_reprojectShadowmapDepthPass->UpdateDescriptorSetWrite(resources);
    }
}

void PBR::createResetDidirectDrawBufferPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        m_resetIndirectDrawBufferPass = std::make_shared<ComputePass>(device);

        auto shader = Singleton<ShaderManager>::instance().GetShader<ComputeShaderModule>("ResetIndirectDrawBuffer Shader");

        Singleton<MVulkanEngine>::instance().CreateComputePass(
            m_resetIndirectDrawBufferPass, shader);

        std::vector<PassResources> resources;

        //PassResources resource;
        resources.push_back(PassResources::SetBufferResource(0, 0, m_numIndirectDrawBuffer));

        m_resetIndirectDrawBufferPass->UpdateDescriptorSetWrite(resources);
    }

    {
        m_resetShadowmapIndirectDrawBufferPass = std::make_shared<ComputePass>(device);

        auto shader = Singleton<ShaderManager>::instance().GetShader<ComputeShaderModule>("ResetIndirectDrawBuffer Shader");

        Singleton<MVulkanEngine>::instance().CreateComputePass(
            m_resetShadowmapIndirectDrawBufferPass, shader);

        std::vector<PassResources> resources;

        //PassResources resource;
        resources.push_back(PassResources::SetBufferResource(0, 0, m_shadowmapNumIndirectDrawBuffer));

        m_resetShadowmapIndirectDrawBufferPass->UpdateDescriptorSetWrite(resources);
    }
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

void DRUI::RenderContext() {
    ImGui::Begin("DR_UI Window", &shouleRenderUI);
    ImGui::Text("Hello from another window!");

    double fps = this->m_app->GetFPS();
    ImGui::Text("FPS: %.2f", fps);

    ImGui::RadioButton("frustum culling mode null", &frustumCullingMode, CULLINGMODE_NULL); ImGui::SameLine();
    ImGui::RadioButton("frustum culling mode sphere", &frustumCullingMode, CULLINGMODE_SPHERE); ImGui::SameLine();
    ImGui::RadioButton("frustum culling mode bbx", &frustumCullingMode, CULLINGMODE_AABB);

    ImGui::Checkbox("do hiz culling", &doHizCulling);
    ImGui::Checkbox("cull shadowmap", &cullShadowmap);

    ImGui::Combo("outputContext", &outputContext, OutputBufferContents, IM_ARRAYSIZE(OutputBufferContents));

    auto app = (PBR*)(this->m_app);
    auto camera = app->GetMainCamera();
    //auto camera = app->GetLightCamera();
    auto& cameraPosition = camera->GetPositionRef();
    auto& cameraDirection = camera->GetDirectionRef();

    ImGui::DragFloat3("Camera Position:", &cameraPosition[0]);
    ImGui::DragFloat3("Camera Direction:", &cameraDirection[0]);

    auto& cameraVelocity = Singleton<InputManager>::instance().m_cameraVelocity;
    ImGui::InputFloat("Camera Velocity:", &cameraVelocity, 0.001f, 0, "%.4f");

    //auto numTotalInstances = ((PBR*)(this->m_app))->numTotalInstances;
    //auto numDrawInstances = ((PBR*)(this->m_app))->numDrawInstances;
    //ImGui::Text("Total Instance Count: %d, Drawed Instance Count: %d", numTotalInstances, numDrawInstances);

    ImGui::Checkbox("showPassTime", &showPassTime);
    if (showPassTime) {
        ImGui::Text("culling time: %.3f ms", ((PBR*)(this->m_app))->cullingTime);
        ImGui::Text("shadowmap time: %.3f ms", ((PBR*)(this->m_app))->shadowmapTime);
        ImGui::Text("gbuffer time: %.3f ms", ((PBR*)(this->m_app))->gbufferTime);
        ImGui::Text("shading time: %.3f ms", ((PBR*)(this->m_app))->shadingTime);
        ImGui::Text("hiz time: %.3f ms", ((PBR*)(this->m_app))->hizTime);
        ImGui::Text("reproject depth time: %.3f ms", ((PBR*)(this->m_app))->reprojectDepthTime);
        //auto hizTimes = ((PBR*)(this->m_app))->hizTimes;
        //for (auto i = 0; i < hizTimes.size(); i++) {
        //    ImGui::Text("hiz layer %d time: %.3f ms", i, ((PBR*)(this->m_app))->hizTimes[i]);
        //}
    }

    ImGui::Checkbox("show instance draw count:", &calculateDrawNums);
    if (calculateDrawNums) {
        auto numTotalInstances = ((PBR*)(this->m_app))->numTotalInstances;
        auto numDrawInstances = ((PBR*)(this->m_app))->numDrawInstances;
        auto numShadowmapDrawInstances = ((PBR*)(this->m_app))->numShadowmapDrawInstances;
        ImGui::Text("Total Instance Count: %d, Drawed Instance Count: %d", numTotalInstances, numDrawInstances);
        ImGui::Text("Total Instance Count: %d, Shadowmap Drawed Instance Count: %d", numTotalInstances, numShadowmapDrawInstances);
    }
        //ImGui::InputInt("Hiz Layer:", &hizLayer, 1, 0);

    ImGui::End();
}