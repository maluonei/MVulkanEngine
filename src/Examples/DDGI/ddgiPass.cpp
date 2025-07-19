#include "ddgiPass.hpp"

#include "MVulkanRHI/MVulkanEngine.hpp"
#include "Scene/SceneLoader.hpp"
#include "Scene/Scene.hpp"
#include "Managers/TextureManager.hpp"
#include "Managers/ShaderManager.hpp"
//#include "Managers/InputManager.hpp"
#include "RenderPass.hpp"
#include "ComputePass.hpp"
#include "Camera.hpp"
#include "Scene/Light/DirectionalLight.hpp"

#include <glm/glm.hpp>
#include "Scene/ddgi.hpp"
#include <chrono>

#include "JsonLoader.hpp"

//#define raysPerProbe


void DDGIApplication::SetUp()
{
    fs::path projectRootPath = PROJECT_ROOT;
    fs::path resourcePath = projectRootPath.append("resources").append("DDGIScene");
    fs::path jsonPath = resourcePath / "sponza.json";
    //fs::path jsonPath = resourcePath / "Arcade.json";
    m_jsonLoader = std::make_shared<JsonFileLoader>(jsonPath.string());

    createSamplers();
    createLight();
    createCamera();

    loadScene();
    initDDGIVolumn();

    createTextures();

    createStorageBuffers();
    createAS();

    loadShaders();
    createSyncObjs();

    auto now = std::chrono::system_clock::now();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    m_start = millis;
}

void DDGIApplication::ComputeAndDraw(uint32_t imageIndex)
{
    auto graphicsList = Singleton<MVulkanEngine>::instance().GetGraphicsList(m_currentFrame);
    auto graphicsQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::GRAPHICS);

    auto computeList = Singleton<MVulkanEngine>::instance().GetComputeCommandList();
    auto computeQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::COMPUTE);

    auto swapchainExtent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();

    auto m_visulizeMode = std::static_pointer_cast<DDGIUI>(m_uiRenderer)->m_visulizeMode;
    auto m_probeClassfication = std::static_pointer_cast<DDGIUI>(m_uiRenderer)->m_probeClassfication;
    auto m_probeRelocationEnabled = std::static_pointer_cast<DDGIUI>(m_uiRenderer)->m_probeRelocationEnabled;

    m_queryIndex = 0;
    int gbufferQueryIndex = -1;
    int probeTraceQueryIndex = -1;
    int probeRelocationQueryIndex = -1;
    int probeClassficationQueryIndex = -1;
    int probeBlendDepthQueryIndex = -1;
    int probeBlendRadianceQueryIndex = -1;
    int lightningQueryIndex = -1;
    int rtaoQueryQueryIndex = -1;
    int compositeQueryIndex = -1;

    //prepare gbufferPass ubo
    {
        VPBuffer vpBuffer{};
        vpBuffer.View = m_camera->GetViewMatrix();
        vpBuffer.Projection = m_camera->GetProjMatrix();
        Singleton<ShaderResourceManager>::instance().LoadData("vpBuffer", 0, &vpBuffer, 0);
 
        LightBuffer lightBuffer{};
        lightBuffer.lightNum = 1;
        lightBuffer.lights[0].direction = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetDirection();
        lightBuffer.lights[0].intensity = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetIntensity();
        lightBuffer.lights[0].color = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetColor();

        DDGILightBuffer ddgiLightBuffer{};
        ddgiLightBuffer.lightNum = 1;
        ddgiLightBuffer.lights[0].direction = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetDirection();
        ddgiLightBuffer.lights[0].intensity = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetIntensity();
        ddgiLightBuffer.lights[0].color = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetColor();
        auto now = std::chrono::system_clock::now();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count();
        ddgiLightBuffer.t = (millis - m_start) / 1000.f;

        m_volume->SetRandomRotation();
        ddgiLightBuffer.probeRotateQuaternion = m_volume->GetQuaternion();

        MCameraBuffer cameraBuffer{};
        cameraBuffer.cameraPos = m_camera->GetPosition();
        cameraBuffer.cameraDir = m_camera->GetDirection();
        cameraBuffer.zNear = m_camera->GetZnear();
        cameraBuffer.zFar = m_camera->GetZfar();
        cameraBuffer.fovY = glm::radians(m_camera->GetFov());

        MScreenBuffer screenBuffer{};
        screenBuffer.WindowRes = int2(swapchainExtent.width, swapchainExtent.height);

        Singleton<ShaderResourceManager>::instance().LoadData("lightBuffer", 0, &lightBuffer, 0);
        Singleton<ShaderResourceManager>::instance().LoadData("ddgiLightBuffer", 0, &ddgiLightBuffer, 0);
        Singleton<ShaderResourceManager>::instance().LoadData("cameraBuffer", 0, &cameraBuffer, 0);
        Singleton<ShaderResourceManager>::instance().LoadData("screenBuffer", 0, &screenBuffer, 0);
    }

    {
        auto probeDim = m_volume->GetProbeDim();
        DDGIBuffer ddgiBuffer{};

        ddgiBuffer.probeDim = probeDim;
        ddgiBuffer.raysPerProbe = m_raysPerProbe;
        ddgiBuffer.probePos0 = m_volume->GetProbePosition(0, 0, 0);
        ddgiBuffer.probePos1 = m_volume->GetProbePosition(probeDim.x - 1, probeDim.y - 1, probeDim.z - 1);
        ddgiBuffer.minFrontFaceDistance = 0.3f;
        ddgiBuffer.probeRelocationEnabled = 1;

        ddgiBuffer.reAccumulate = 0;
        ddgiBuffer.maxRayDistance = 1e+4f;
        ddgiBuffer.sharpness = 1.f;

        Singleton<ShaderResourceManager>::instance().LoadData("ddgiBuffer", 0, &ddgiBuffer, 0);
    }

    {
        RTAOBuffer rtaoBuffer;
        rtaoBuffer.resetAccumulatedBuffer = GetCameraMoved() ? 1 : 0;
        Singleton<ShaderResourceManager>::instance().LoadData("RtaoBuffer", 0, &rtaoBuffer, 0);
    }

    {
        DDGICompositeBuffer compositeBuffer{};
        compositeBuffer.useAO = 1;
        compositeBuffer.visulizeProbe = m_visualizeProbes;
        compositeBuffer.visulizeMode = m_visulizeMode;
        Singleton<ShaderResourceManager>::instance().LoadData("ddgiCompositeBuffer", m_currentFrame, &compositeBuffer, 0);
    }
    
    RenderingInfo gbufferRenderInfo;
    {
        gbufferRenderInfo.colorAttachments.push_back(
            RenderingAttachment{
                .texture = gBuffer0,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .view = nullptr,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearColor = glm::vec4(0.f, 0.f, 0.f, 0.f)
            }
            );
        gbufferRenderInfo.colorAttachments.push_back(
            RenderingAttachment{
                .texture = gBuffer1,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .view = nullptr,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearColor = glm::vec4(0.f, 0.f, 0.f, 0.f)
            }
            );
        gbufferRenderInfo.colorAttachments.push_back(
            RenderingAttachment{
                .texture = gBuffer2,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .view = nullptr,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearColor = glm::vec4(0.f, 0.f, 0.f, 0.f)
            }
            );
        gbufferRenderInfo.colorAttachments.push_back(
            RenderingAttachment{
                .texture = gBuffer3,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .view = nullptr,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearColor = glm::vec4(0.f, 0.f, 0.f, 0.f)
            }
            );

        gbufferRenderInfo.depthAttachment = RenderingAttachment{
                .texture = gBufferDepth,
                .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                .view = nullptr,
        };

        gbufferRenderInfo.offset = { 0, 0 };
        gbufferRenderInfo.extent = swapchainExtent;
    }

    RenderingInfo probeTracingInfo{};
    {
        probeTracingInfo.colorAttachments.push_back(
            RenderingAttachment{
                .texture = m_probePositions,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            }
            );
        probeTracingInfo.colorAttachments.push_back(
            RenderingAttachment{
                .texture = m_probeNormals,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            }
            );
        probeTracingInfo.colorAttachments.push_back(
            RenderingAttachment{
                .texture = m_probeAlbedo,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            }
            );
        probeTracingInfo.colorAttachments.push_back(
            RenderingAttachment{
                .texture = m_probeRadiance,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            }
            );

        probeTracingInfo.depthAttachment = RenderingAttachment{
            .texture = m_probeDepth,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .view = nullptr,
        };

        probeTracingInfo.offset = { 0, 0 };
        probeTracingInfo.extent = { 64, (unsigned int)m_volume->GetNumProbes() };
    }

    RenderingInfo LighteningRenderInfo;
    {
        LighteningRenderInfo.colorAttachments.push_back(
            RenderingAttachment{
                .texture = m_diTexture,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            }
            );
        
        LighteningRenderInfo.colorAttachments.push_back(
            RenderingAttachment{
                .texture = m_giTexture,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            }
            );

        LighteningRenderInfo.offset = { 0, 0 };
        LighteningRenderInfo.extent = swapchainExtent;
        LighteningRenderInfo.useDepth = false;
    }

    RenderingInfo RTAORenderInfo;
    {
        RTAORenderInfo.colorAttachments.push_back(
            RenderingAttachment{
                .texture = m_aoTexture,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            }
            );

        RTAORenderInfo.offset = { 0, 0 };
        RTAORenderInfo.extent = swapchainExtent;
        RTAORenderInfo.useDepth = false;
    }

    RenderingInfo CompositeRenderingInfo{}, ProbeVisulizeRenderingInfo{};

    auto swapChain = Singleton<MVulkanEngine>::instance().GetSwapchain();

    CompositeRenderingInfo.colorAttachments.push_back(
        RenderingAttachment{
            .texture = nullptr,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .view = swapChain.GetImageView(imageIndex),
        }
        );
    CompositeRenderingInfo.depthAttachment = RenderingAttachment{
            .texture = swapchainDepthViews[imageIndex],
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };


    CompositeRenderingInfo.offset = { 0, 0 };
    CompositeRenderingInfo.extent = swapchainExtent;
    CompositeRenderingInfo.useDepth = false;
    if (m_visualizeProbes) {
        ProbeVisulizeRenderingInfo.colorAttachments.push_back(
            RenderingAttachment{
                .texture = nullptr,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .view = swapChain.GetImageView(imageIndex),
                .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE
            }
            );
        ProbeVisulizeRenderingInfo.depthAttachment = RenderingAttachment{
                .texture = swapchainDepthViews[imageIndex],
                .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE
        };


        ProbeVisulizeRenderingInfo.offset = { 0, 0 };
        ProbeVisulizeRenderingInfo.extent = swapchainExtent;
        ProbeVisulizeRenderingInfo.useDepth = true;
    }

    auto probeDim = m_volume->GetProbeDim();



    graphicsList.GetFence().WaitForSignal();
    graphicsList.GetFence().Reset();
    
    graphicsList.Reset();
    graphicsList.Begin();
    Singleton<MVulkanEngine>::instance().CmdResetTimeStampQueryPool(graphicsList);

    gbufferQueryIndex = m_queryIndex;
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(
        0, 
        m_gbufferPass,
        m_currentFrame, 
        gbufferRenderInfo, 
        m_scene->GetIndirectVertexBuffer(), 
        m_scene->GetIndirectIndexBuffer(), 
        m_scene->GetIndirectBuffer(),
        m_scene->GetIndirectDrawCommands().size(), 
        std::string("Gbuffer Pass"),
        m_queryIndex++);

    probeTraceQueryIndex = m_queryIndex;
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(
        0,
        m_probeTracingRenderPass,
        m_currentFrame,
        probeTracingInfo,
        m_squad->GetIndirectVertexBuffer(),
        m_squad->GetIndirectIndexBuffer(),
        m_squad->GetIndirectBuffer(),
        m_squad->GetIndirectDrawCommands().size(),
        std::string("ProbeTracing Pass"),
        m_queryIndex++);

    graphicsList.End();


    if (m_sceneChange) {
        std::vector<MVulkanSemaphore> waitSemaphores(0);
        std::vector<MVulkanSemaphore> signalSemaphores(1, m_shadingSemaphore);
        std::vector<VkPipelineStageFlags> waitFlags(0);
        Singleton<MVulkanEngine>::instance().SubmitCommands(graphicsList, graphicsQueue, waitSemaphores, waitFlags, signalSemaphores);

        computeList.GetFence().WaitForSignal();
        computeList.GetFence().Reset();

        computeList.Reset();
        computeList.Begin();

        if (m_probeRelocationEnabled) {
            probeRelocationQueryIndex = m_queryIndex;
            Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_probeRelocationPass, computeList, (probeDim.x * probeDim.y * probeDim.z + 31) / 32, 1, 1,
                std::string("Probe Reloction"), m_queryIndex++);
            
            //m_uniformBuffer1.reAccumulate = 1;
            m_probeRelocationEnabled = false;
        }
        if (m_probeClassfication) {
            probeClassficationQueryIndex = m_queryIndex;
            Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_probeClassficationPass, computeList, (probeDim.x * probeDim.y * probeDim.z + 31) / 32, 1, 1,
                std::string("Probe Classfication"), m_queryIndex++);
        }
        probeBlendDepthQueryIndex = m_queryIndex;
        Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_probeBlendingDepthPass, computeList, probeDim.x * probeDim.y, probeDim.z, 1,
            std::string("Probe Blend Depth"), m_queryIndex++);
        probeBlendRadianceQueryIndex = m_queryIndex;
        Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_probeBlendingRadiancePass, computeList, probeDim.x * probeDim.y, probeDim.z, 1,
            std::string("Probe Blend Radiance"), m_queryIndex++);

        computeList.End();

        std::vector<MVulkanSemaphore> waitSemaphores2(1, m_shadingSemaphore);
        std::vector<MVulkanSemaphore> signalSemaphores2(1, m_ddgiSemephore);
        std::vector<VkPipelineStageFlags> waitFlags2(1, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        Singleton<MVulkanEngine>::instance().SubmitCommands(computeList, computeQueue, waitSemaphores2, waitFlags2, signalSemaphores2);
    }
    else {
        std::vector<MVulkanSemaphore> waitSemaphores(0);
        std::vector<MVulkanSemaphore> signalSemaphores(0);
        std::vector<VkPipelineStageFlags> waitFlags(0);
        Singleton<MVulkanEngine>::instance().SubmitCommands(graphicsList, graphicsQueue, waitSemaphores, waitFlags, signalSemaphores);
    }

    graphicsList.GetFence().WaitForSignal();
    graphicsList.GetFence().Reset();

    graphicsList.Reset();
    graphicsList.Begin();

    if (m_visulizeMode != VisulizeDDGI_AO) {
        lightningQueryIndex = m_queryIndex;
        Singleton<MVulkanEngine>::instance().RecordCommandBuffer(
            0,
            m_lightingPass,
            m_currentFrame,
            LighteningRenderInfo,
            m_squad->GetIndirectVertexBuffer(),
            m_squad->GetIndirectIndexBuffer(),
            m_squad->GetIndirectBuffer(),
            m_squad->GetIndirectDrawCommands().size(),
            std::string("DDGI Lighting"),
            m_queryIndex++);
    }
    
    if (m_visulizeMode == VisulizeDDGI_AO || m_visulizeMode == VisulizeDDGI_ALL) {
        rtaoQueryQueryIndex = m_queryIndex;
        Singleton<MVulkanEngine>::instance().RecordCommandBuffer(
            0, 
            m_rtaoPass,
            m_currentFrame,
            RTAORenderInfo,
            m_squad->GetIndirectVertexBuffer(),
            m_squad->GetIndirectIndexBuffer(),
            m_squad->GetIndirectBuffer(), 
            m_squad->GetIndirectDrawCommands().size(),
            std::string("RTAO"), 
            m_queryIndex++);
    }

    //if (m_visualizeProbes) {
    //    MVulkanImageCopyInfo copyInfo{};
    //    copyInfo.extent = {swapchainExtent.width, swapchainExtent.height, 1};
    //    copyInfo.srcOffset = { 0, 0, 0 };
    //    copyInfo.dstOffset = { 0, 0, 0 };
    //    copyInfo.srcAspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    //    copyInfo.dstAspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    //    copyInfo.layerCount = 1;
    //
    //    Singleton<MVulkanEngine>::instance().CopyImage(graphicsList, swapchainDepthViews[imageIndex], gBufferDepth, copyInfo);
    //
    //    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(
    //        imageIndex,
    //        m_compositeScenePass,
    //        m_currentFrame,
    //        CompositeRenderingInfo,
    //        m_squad->GetIndirectVertexBuffer(),
    //        m_squad->GetIndirectIndexBuffer(),
    //        m_squad->GetIndirectBuffer(),
    //        m_squad->GetIndirectDrawCommands().size(),
    //        std::string("Composite"));
    //
    //    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(
    //        imageIndex,
    //        m_probeVisulizePass,
    //        m_currentFrame,
    //        ProbeVisulizeRenderingInfo,
    //        m_sphere->GetIndirectVertexBuffer(),
    //        m_sphere->GetIndirectIndexBuffer(),
    //        m_sphere->GetIndirectBuffer(),
    //        m_sphere->GetIndirectDrawCommands().size(),
    //        std::string("Visulize Probe"));
    //}
    //else {
        compositeQueryIndex = m_queryIndex;
        Singleton<MVulkanEngine>::instance().RecordCommandBuffer(
            imageIndex,
            m_compositeScenePass,
            m_currentFrame,
            CompositeRenderingInfo,
            m_squad->GetIndirectVertexBuffer(),
            m_squad->GetIndirectIndexBuffer(),
            m_squad->GetIndirectBuffer(),
            m_squad->GetIndirectDrawCommands().size(),
            std::string("Composite"),
            m_queryIndex++);
    //}

    
    graphicsList.End();

    if (m_sceneChange) {
        std::vector<MVulkanSemaphore> waitSemaphores1(1, m_ddgiSemephore);
        std::vector<MVulkanSemaphore> signalSemaphores1(0);
        std::vector<VkPipelineStageFlags> waitFlags1(1, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        Singleton<MVulkanEngine>::instance().SubmitGraphicsCommands(imageIndex, m_currentFrame, waitSemaphores1, waitFlags1, signalSemaphores1);
    }
    else {
        std::vector<MVulkanSemaphore> waitSemaphores1(0);
        std::vector<MVulkanSemaphore> signalSemaphores1(0);
        std::vector<VkPipelineStageFlags> waitFlags1(0);
        Singleton<MVulkanEngine>::instance().SubmitGraphicsCommands(imageIndex, m_currentFrame, waitSemaphores1, waitFlags1, signalSemaphores1);
    }

    auto showPassTime = std::static_pointer_cast<DDGIUI>(m_uiRenderer)->m_showPassTime;
    if (showPassTime) {
        auto queryResults = Singleton<MVulkanEngine>::instance().GetTimeStampQueryResults(0, 2 * m_queryIndex);
        float timestampPeriod = Singleton<MVulkanEngine>::instance().GetTimeStampPeriod();

        m_gbufferTime = (queryResults[gbufferQueryIndex * 2 + 1] - queryResults[gbufferQueryIndex * 2]) * timestampPeriod / 1000000.f;
        m_probeTracingTime = (queryResults[probeTraceQueryIndex * 2 + 1] - queryResults[probeTraceQueryIndex * 2]) * timestampPeriod / 1000000.f;
        m_probeRelocationTime = probeRelocationQueryIndex == -1 ? 0.f : (queryResults[probeRelocationQueryIndex * 2 + 1] - queryResults[probeRelocationQueryIndex * 2]) * timestampPeriod / 1000000.f;
        m_probeClassficationTime = probeClassficationQueryIndex == -1? 0.f : (queryResults[probeClassficationQueryIndex * 2 + 1] - queryResults[probeClassficationQueryIndex * 2]) * timestampPeriod / 1000000.f;
        m_probeBlendDepthTime = (queryResults[probeBlendDepthQueryIndex * 2 + 1] - queryResults[probeBlendDepthQueryIndex * 2]) * timestampPeriod / 1000000.f;
        m_probeBlendRadianceTime = (queryResults[probeBlendRadianceQueryIndex * 2 + 1] - queryResults[probeBlendRadianceQueryIndex * 2]) * timestampPeriod / 1000000.f;
        m_lightingTime = lightningQueryIndex == -1 ? 0.f : (queryResults[lightningQueryIndex * 2 + 1] - queryResults[lightningQueryIndex * 2]) * timestampPeriod / 1000000.f;
        m_rtaoTime = rtaoQueryQueryIndex == -1 ? 0.f : (queryResults[rtaoQueryQueryIndex * 2 + 1] - queryResults[rtaoQueryQueryIndex * 2]) * timestampPeriod / 1000000.f;
        m_compositeTime = (queryResults[compositeQueryIndex * 2 + 1] - queryResults[compositeQueryIndex * 2]) * timestampPeriod / 1000000.f;
    }
}

void DDGIApplication::RecreateSwapchainAndRenderPasses()
{
    //if (Singleton<MVulkanEngine>::instance().RecreateSwapchain()) {
    //    Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_gbufferPass);
    //
    //    createTextures();
    //
    //    {
    //        //m_rtaoPass->GetRenderPassCreateInfo().depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();
    //        Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_rtaoPass);
    //
    //        std::vector<std::vector<VkImageView>> rtaoViews(2);
    //        for (auto i = 0; i < 2; i++) {
    //            rtaoViews[i].resize(1);
    //            rtaoViews[i][0] = m_gbufferPass->GetFrameBuffer(0).GetImageView(i);
    //        }
    //
    //        std::vector<VkSampler> samplers(1);
    //        samplers[0] = m_linearSamplerWithAnisotropy.GetSampler();
    //
    //        std::vector<std::vector<VkImageView>> storageTextureViews(1);
    //        storageTextureViews[0].resize(1, m_acculatedAOTexture->GetImageView());
    //
    //        auto tlas = m_rayTracing.GetTLAS();
    //        std::vector<VkAccelerationStructureKHR> accelerationStructures(1, tlas);
    //
    //        m_rtaoPass->UpdateDescriptorSetWrite(rtaoViews, storageTextureViews, samplers, accelerationStructures);
    //    }
    //
    //    {
    //        m_probeVisulizePass->GetRenderPassCreateInfo().depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();
    //        Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_probeVisulizePass);
    //    }
    //
    //    {
    //        Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_lightingPass);
    //
    //        std::vector<std::vector<VkImageView>> views(6);
    //        for (auto i = 0; i < 4; i++) {
    //            views[i].resize(1);
    //            views[i][0] = m_gbufferPass->GetFrameBuffer(0).GetImageView(i);
    //        }
    //        views[4] = std::vector<VkImageView>(1, m_volumeProbeDatasRadiance->GetImageView());
    //        views[5] = std::vector<VkImageView>(1, m_volumeProbeDatasDepth->GetImageView());
    //
    //        std::vector<VkSampler> samplers(1);
    //        samplers[0] = m_linearSamplerWithoutAnisotropy.GetSampler();
    //
    //        auto tlas = m_rayTracing.GetTLAS();
    //        std::vector<VkAccelerationStructureKHR> accelerationStructures(1, tlas);
    //
    //        m_lightingPass->UpdateDescriptorSetWrite(views, samplers, accelerationStructures);
    //    }
    //
    //    {
    //        //m_compositePass->GetRenderPassCreateInfo().depthView = m_compositePass->GetFrameBuffer(0).GetDepthImageView();
    //        Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_compositePass);
    //
    //        std::vector<std::vector<VkImageView>> views(4);
    //        views[0].resize(1);
    //        views[0][0] = m_lightingPass->GetFrameBuffer(0).GetImageView(0);
    //        views[1].resize(1);
    //        views[1][0] = m_lightingPass->GetFrameBuffer(0).GetImageView(1);
    //        views[2].resize(1);
    //        views[2][0] = m_rtaoPass->GetFrameBuffer(0).GetImageView(0);
    //        views[3].resize(1);
    //        views[3][0] = m_probeVisulizePass->GetFrameBuffer(0).GetImageView(0);
    //
    //        std::vector<VkSampler> samplers(1);
    //        samplers[0] = m_linearSamplerWithAnisotropy.GetSampler();
    //
    //        std::vector<VkAccelerationStructureKHR> accelerationStructures(0);
    //
    //        m_compositePass->UpdateDescriptorSetWrite(views, samplers, accelerationStructures);
    //    }
    //}
}

void DDGIApplication::CreateRenderPass()
{
    createGbufferPass();
    createProbeTracingPass();
    createProbeBlendingRadiancePass();
    createProbeBlendingDepthPass();
    createProbeClassficationPass();
    createProbeRelocationPass();
    createLightingPass();
    createRTAOPass();
    createProbeVisulizePass();
    //createCompositePass();
    createCompositeScenePass();
    //createRayQueryTestPass();
    
    return;
}

void DDGIApplication::PreComputes()
{

}

void DDGIApplication::Clean()
{
    m_gbufferPass->Clean();
    m_rtaoPass->Clean();
    //m_probeTracingPass->Clean();
    m_lightingPass->Clean();
    m_probeVisulizePass->Clean();
    m_compositeScenePass->Clean();
    m_probeBlendingRadiancePass->Clean();

    m_acculatedAOTexture->Clean();
    m_volumeProbeDatasDepth->Clean();
    m_volumeProbeDatasRadiance->Clean();

    m_linearSamplerWithAnisotropy.Clean();
    m_linearSamplerWithoutAnisotropy.Clean();

    m_rayTracing.Clean();

    m_sphere->Clean();
    m_squad->Clean();
    m_scene->Clean();

    MRenderApplication::Clean();
}

void DDGIApplication::loadScene()
{
    m_scene = std::make_shared<Scene>();

    fs::path projectRootPath = PROJECT_ROOT;
    fs::path resourcePath = projectRootPath.append("resources").append("models");
    ////fs::path modelPath = resourcePath / "Sponza" / "glTF" / "Sponza.gltf";
    ////fs::path modelPath = resourcePath / "Arcade" / "Arcade.gltf";
    //fs::path modelPath = resourcePath / "shapespark_example_room" / "shapespark_example_room.gltf";

    std::string scenePath = m_jsonLoader->GetScenePath();
    fs::path modelPath = resourcePath / scenePath;

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

    m_sphere = std::make_shared<Scene>();
    fs::path spherePath = resourcePath / "sphere.obj";
    Singleton<SceneLoader>::instance().Load(spherePath.string(), m_sphere.get());

    m_squad->GenerateIndirectDataAndBuffers();
    auto probeDim = m_jsonLoader->GetDDGIProbeDim();
    m_sphere->GenerateIndirectDataAndBuffers(probeDim.x * probeDim.y * probeDim.z);
    m_scene->GenerateIndirectDataAndBuffers();

    m_scene->GenerateMeshBuffers();
}

void DDGIApplication::createAS()
{
    m_rayTracing.Create(Singleton<MVulkanEngine>::instance().GetDevice());
    m_rayTracing.TestCreateAccelerationStructure(m_scene);
}

void DDGIApplication::createLight()
{
    //glm::vec3 direction = glm::normalize(glm::vec3(-1.f, -6.f, -1.f));
    //glm::vec3 direction = glm::normalize(glm::vec3(-3.f, -6.f, -1.f));
    //glm::vec3 direction = glm::normalize(glm::vec3(-2.f, -1.f, 1.f));
    ////glm::vec3 direction = glm::normalize(glm::vec3(-1.f, -1.f, -1.f));
    //glm::vec3 color = glm::vec3(1.f, 1.f, 1.f);
    ////float intensity = 20.f;
    //float intensity = 100.f;
    //m_directionalLight = std::make_shared<DirectionalLight>(direction, color, intensity);

    m_directionalLight = m_jsonLoader->GetLights();
}

void DDGIApplication::createCamera()
{
    //glm::vec3 position(1.2925221, 3.7383504, -0.29563048);
    //glm::vec3 center = position + glm::vec3(-0.8f, -0.3f, -0.1f);
    //glm::vec3 direction = glm::normalize(center - position);
  
    //glm::vec3 position(-4.9944386, 2.9471996, -5.8589);
    //glm::vec3 direction = glm::normalize(glm::vec3(2.f, -1.f, 2.f));

    //glm::vec3 position(-4.6, 4.9, -9.0);
    //glm::vec3 direction = glm::normalize(glm::vec3(2.f, -1.f, -2.f));

    //glm::vec3 position(0.f, 1.f, 2.f);
    //glm::vec3 direction = glm::normalize(glm::vec3(0.f, -1.f, -2.f));
    //
    //float fov = 60.f;
    //float aspectRatio = (float)WIDTH / (float)HEIGHT;
    //float zNear = 0.01f;
    //float zFar = 1000.f;
    //
    //m_camera = std::make_shared<Camera>(position, direction, fov, aspectRatio, zNear, zFar);
    m_camera = m_jsonLoader->GetCamera();
    Singleton<MVulkanEngine>::instance().SetCamera(m_camera);
}

void DDGIApplication::createSamplers()
{
    {
        MVulkanSamplerCreateInfo info{};
        info.minFilter = VK_FILTER_LINEAR;
        info.magFilter = VK_FILTER_LINEAR;
        info.mipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        //info.minFilter = VK_FILTER_NEAREST;
        //info.magFilter = VK_FILTER_NEAREST;
        //info.mipMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        info.maxLod = 0.f;
        info.anisotropyEnable = false;
        m_linearSamplerWithoutAnisotropy.Create(Singleton<MVulkanEngine>::instance().GetDevice(), info);
    }

    {
        MVulkanSamplerCreateInfo info{};
        info.minFilter = VK_FILTER_LINEAR;
        info.magFilter = VK_FILTER_LINEAR;
        info.mipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        //info.minFilter = VK_FILTER_NEAREST;
        //info.magFilter = VK_FILTER_NEAREST;
        //info.mipMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        info.maxLod = 4.f;
        info.anisotropyEnable = true;
        m_linearSamplerWithAnisotropy.Create(Singleton<MVulkanEngine>::instance().GetDevice(), info);
    }
}

void DDGIApplication::createTextures()
{
    //if (!m_volumeProbeDatasDepth) {
    //    m_volumeProbeDatasDepth = std::make_shared<MVulkanTexture>();
    //}
    //else {
    //    m_volumeProbeDatasDepth->Clean();
    //}
    //
    //if (!m_volumeProbeDatasRadiance) {
    //    m_volumeProbeDatasRadiance = std::make_shared<MVulkanTexture>();
    //}
    //else {
    //    m_volumeProbeDatasRadiance->Clean();
    //}

    if (!m_acculatedAOTexture) {
      m_acculatedAOTexture = std::make_shared<MVulkanTexture>();
    }
    else {
        m_acculatedAOTexture->Clean();
    }

    //bool translationLayout = false;
    auto extent2D = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
    //
    {
        ImageCreateInfo imageInfo;
        ImageViewCreateInfo viewInfo;
        imageInfo.arrayLength = 1;
        imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.width = extent2D.width;
        imageInfo.height = extent2D.height;
        imageInfo.format = VK_FORMAT_R32G32_SFLOAT;
    
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = imageInfo.format;
        viewInfo.flag = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.baseMipLevel = 0;
        viewInfo.levelCount = 1;
        viewInfo.baseArrayLayer = 0;
        viewInfo.layerCount = 1;
    
        Singleton<MVulkanEngine>::instance().CreateImage(m_acculatedAOTexture, imageInfo, viewInfo, VK_IMAGE_LAYOUT_GENERAL);
    }
    //
    auto probeDim = m_volume->GetProbeDim();
    //
    if (!m_volumeProbeDatasRadiance)
    {
        m_volumeProbeDatasRadiance = std::make_shared<MVulkanTexture>();
        //translationLayout = true;
    
        ImageCreateInfo imageInfo;
        ImageViewCreateInfo viewInfo;
        imageInfo.arrayLength = 1;
        imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.width = 8 * probeDim.x * probeDim.y;
        imageInfo.height = 8 * probeDim.z;
        imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = imageInfo.format;
        viewInfo.flag = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.baseMipLevel = 0;
        viewInfo.levelCount = 1;
        viewInfo.baseArrayLayer = 0;
        viewInfo.layerCount = 1;
    
        Singleton<MVulkanEngine>::instance().CreateImage(m_volumeProbeDatasRadiance, imageInfo, viewInfo, VK_IMAGE_LAYOUT_GENERAL);
    }
    
    if (!m_volumeProbeDatasDepth)
    {
        m_volumeProbeDatasDepth = std::make_shared<MVulkanTexture>();
    
        ImageCreateInfo imageInfo;
        ImageViewCreateInfo viewInfo;
        imageInfo.arrayLength = 1;
        imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.width = 16 * probeDim.x * probeDim.y;
        imageInfo.height = 16 * probeDim.z;
        imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = imageInfo.format;
        viewInfo.flag = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.baseMipLevel = 0;
        viewInfo.levelCount = 1;
        viewInfo.baseArrayLayer = 0;
        viewInfo.layerCount = 1;
    
        Singleton<MVulkanEngine>::instance().CreateImage(m_volumeProbeDatasDepth, imageInfo, viewInfo, VK_IMAGE_LAYOUT_GENERAL);
    }

    //if (!m_testTexture)
    //{
    //    m_testTexture = std::make_shared<MVulkanTexture>();
    //
    //    ImageCreateInfo imageInfo;
    //    ImageViewCreateInfo viewInfo;
    //    imageInfo.arrayLength = 1;
    //    imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    //    imageInfo.width = 16 * probeDim.z * probeDim.x * probeDim.y;
    //    imageInfo.height = 1;
    //    imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    //
    //    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    //    viewInfo.format = imageInfo.format;
    //    viewInfo.flag = VK_IMAGE_ASPECT_COLOR_BIT;
    //    viewInfo.baseMipLevel = 0;
    //    viewInfo.levelCount = 1;
    //    viewInfo.baseArrayLayer = 0;
    //    viewInfo.layerCount = 1;
    //
    //    Singleton<MVulkanEngine>::instance().CreateImage(m_testTexture, imageInfo, viewInfo, VK_IMAGE_LAYOUT_GENERAL);
    //}

    //if(translationLayout)
    //    changeRWTextureLayoutToTexture();
    //auto probeDim = m_volume->GetProbeDim();
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();
    auto swapchainExtent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
    //auto shadowMapExtent = shadowmapExtent;
    VkExtent2D probeTextureExtent = {512, 64};

    auto depthFormat = device.FindDepthFormat();
    //auto shadowMapFormat = VK_FORMAT_R32G32B32A32_UINT;

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
        //auto gbufferFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
        //auto motionVectorFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

        gBuffer0 = std::make_shared<MVulkanTexture>();
        gBuffer1 = std::make_shared<MVulkanTexture>();
        gBuffer2 = std::make_shared<MVulkanTexture>();
        gBuffer3 = std::make_shared<MVulkanTexture>();
        //gBuffer4 = std::make_shared<MVulkanTexture>();
        //gBuffer5 = std::make_shared<MVulkanTexture>();
        gBufferDepth = std::make_shared<MVulkanTexture>();

        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            gBuffer0, swapchainExtent, VK_FORMAT_R32G32B32A32_SFLOAT
        );

        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            gBuffer1, swapchainExtent, VK_FORMAT_R32G32B32A32_SFLOAT
        );

        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            gBuffer2, swapchainExtent, VK_FORMAT_R32G32B32A32_SFLOAT
        );
        
        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            gBuffer3, swapchainExtent, VK_FORMAT_R32G32B32A32_SFLOAT
        );
        //Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
        //    gBuffer4, swapchainExtent, VK_FORMAT_R32G32B32A32_SFLOAT
        //);
        //
        //Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
        //    gBuffer5, swapchainExtent, VK_FORMAT_R32G32B32A32_SFLOAT
        //);

        Singleton<MVulkanEngine>::instance().CreateDepthAttachmentImage(
            gBufferDepth, swapchainExtent, depthFormat
        );

        auto format = VK_FORMAT_R32G32B32A32_SFLOAT;

        m_diTexture = std::make_shared<MVulkanTexture>();
        m_giTexture = std::make_shared<MVulkanTexture>();
        m_aoTexture = std::make_shared<MVulkanTexture>();
        //m_acculatedAOTexture = std::make_shared<MVulkanTexture>();
        m_probeVisulizTexture = std::make_shared<MVulkanTexture>();
        m_testTexture = std::make_shared<MVulkanTexture>();

        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            m_diTexture, swapchainExtent, format
        );

        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            m_giTexture, swapchainExtent, format
        );

        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            m_probeVisulizTexture, swapchainExtent, format
        );

        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            m_testTexture, swapchainExtent, format
        );

        format = VK_FORMAT_R32_SFLOAT;
        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            m_aoTexture, swapchainExtent, format
        );
    }

    {
        auto format = VK_FORMAT_R32G32B32A32_SFLOAT;
        VkExtent2D extent = { 64, m_volume->GetNumProbes() };

        m_probePositions = std::make_shared<MVulkanTexture>();
        m_probeNormals = std::make_shared<MVulkanTexture>();
        m_probeDepth = std::make_shared<MVulkanTexture>();
        m_probeAlbedo = std::make_shared<MVulkanTexture>();
        m_probeRadiance = std::make_shared<MVulkanTexture>();
        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            m_probePositions, extent, format
        );
        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            m_probeNormals, extent, format
        );
        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            m_probeAlbedo, extent, format
        );
        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            m_probeRadiance, extent, format
        );
        Singleton<MVulkanEngine>::instance().CreateDepthAttachmentImage(
            m_probeDepth, extent, depthFormat
        );
    }

    {
        auto format = VK_FORMAT_R32_UINT;
        m_matIdTexture = std::make_shared<MVulkanTexture>();

        ImageCreateInfo imageInfo;
        ImageViewCreateInfo viewInfo;
        imageInfo.arrayLength = 1;
        imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.width = m_raysPerProbe;
        imageInfo.height = m_volume->GetNumProbes();
        imageInfo.format = format;

        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = imageInfo.format;
        viewInfo.flag = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.baseMipLevel = 0;
        viewInfo.levelCount = 1;
        viewInfo.baseArrayLayer = 0;
        viewInfo.layerCount = 1;

        Singleton<MVulkanEngine>::instance().CreateImage(m_matIdTexture, imageInfo, viewInfo, VK_IMAGE_LAYOUT_GENERAL);
    
        m_texCoordsTexture = std::make_shared<MVulkanTexture>();
        format = VK_FORMAT_R32G32_SFLOAT;
        imageInfo.format = format;
        viewInfo.format = imageInfo.format;
        Singleton<MVulkanEngine>::instance().CreateImage(m_texCoordsTexture, imageInfo, viewInfo, VK_IMAGE_LAYOUT_GENERAL);

    }

}

void DDGIApplication::createStorageBuffers()
{
    {
        auto modelBuffer = m_volume->GetModelBuffer();

        BufferCreateInfo info{};
        info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        info.arrayLength = 1;
        info.size = modelBuffer.GetSize();

        m_probesModelBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, modelBuffer.models.data());
    }

    {
        auto probeBuffer = m_volume->GetProbeBuffer();

        BufferCreateInfo info{};
        info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        info.arrayLength = 1;
        info.size = probeBuffer.GetSize();

        m_probesDataBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, probeBuffer.probes.data());
    }

    {
        BufferCreateInfo info{};
        info.arrayLength = 1;

        std::vector<glm::mat4> models = m_scene->GetTransforms();
        info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        info.size = sizeof(glm::mat4) * models.size();
        m_modelBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, models.data());

    }

    {
        BufferCreateInfo info{};
        info.arrayLength = 1;

        std::vector<int> materialIds = m_scene->GetMaterialsIds();
        info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        info.size = sizeof(int) * materialIds.size();
        m_materialIdBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, materialIds.data());

        std::vector<MaterialBuffer> materials = m_scene->GetMaterials();
        info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        info.size = sizeof(MaterialBuffer) * materials.size();
        m_materialBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, materials.data());
    }

    {
        auto probeBuffer = m_volume->GetProbeBuffer();

        auto numVertices = m_scene->GetNumVertices();
        auto numIndices = m_scene->GetNumTriangles() * 3;
        auto numMeshes = m_scene->GetNumMeshes();
        auto numInstances = m_scene->GetTotalPrimInfos();

        BufferCreateInfo vertexBufferCreateInfo{};
        BufferCreateInfo indexBufferCreateInfo{};
        BufferCreateInfo normalBufferCreateInfo{};
        BufferCreateInfo uvBufferCreateInfo{};
        BufferCreateInfo geometryBufferCreateInfo{};

        vertexBufferCreateInfo.size = numVertices * sizeof(glm::vec3);
        indexBufferCreateInfo.size = numIndices * sizeof(int);
        normalBufferCreateInfo.size = vertexBufferCreateInfo.size;
        uvBufferCreateInfo.size = numVertices * sizeof(glm::vec2);
        geometryBufferCreateInfo.size = numInstances * sizeof(GeometryInfo);

        VertexBuffer   vertexBuffer;
        IndexBuffer    indexBuffer;
        NormalBuffer   normalBuffer;
        UVBuffer       uvBuffer;
        InstanceOffset instanceOffsetBuffer;

        vertexBuffer.position.resize(numVertices);
        indexBuffer.index.resize(numIndices);
        normalBuffer.normal.resize(numVertices);
        uvBuffer.uv.resize(numVertices);
        instanceOffsetBuffer.geometryInfos.resize(numInstances);
        probeBuffer = probeBuffer;

        int vertexBufferIndex = 0;
        int indexBufferIndex = 0;
        int instanceBufferIndex = 0;

        //auto numInstances = m_scene->GetTotalPrimInfos();
        auto primInfos = m_scene->m_primInfos;
        auto numMeshs = primInfos.size();

        int instanceIndex = 0;
        for (auto k = 0; k < numMeshs; k++) {
            auto numMeshInstances = primInfos[k].size();
            auto mesh = m_scene->GetMesh(primInfos[k][0].mesh_id);

            auto meshVertexNum = mesh->vertices.size();
            for (auto i = 0; i < meshVertexNum; i++) {
                vertexBuffer.position[vertexBufferIndex + i] = mesh->vertices[i].position;
                normalBuffer.normal[vertexBufferIndex + i] = mesh->vertices[i].normal;
                uvBuffer.uv[vertexBufferIndex + i] = mesh->vertices[i].texcoord;
            }

            auto meshIndexNum = mesh->indices.size();
            for (auto i = 0; i < meshIndexNum; i++) {
                indexBuffer.index[indexBufferIndex + i] = mesh->indices[i];
            }

            for (auto j = 0; j < numMeshInstances; j++) {
                instanceOffsetBuffer.geometryInfos[instanceBufferIndex] =
                    GeometryInfo{
                        .transform = primInfos[k][j].transform,
                        .vertexOffset = vertexBufferIndex * 3,
                        .indexOffset = indexBufferIndex,
                        .uvOffset = vertexBufferIndex * 2,
                        .normalOffset = vertexBufferIndex * 3,
                        .materialIdx = int(primInfos[k][j].material_id)
                };
                instanceBufferIndex += 1;
            }

            vertexBufferIndex += meshVertexNum;
            indexBufferIndex += meshIndexNum;
        }

        vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        indexBufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        normalBufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        uvBufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        geometryBufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

        m_tlasVertexBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(vertexBufferCreateInfo, vertexBuffer.position.data());
        m_tlasIndexBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(indexBufferCreateInfo, indexBuffer.index.data());
        m_tlasNormalBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(normalBufferCreateInfo, normalBuffer.normal.data());
        m_tlasUVBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(uvBufferCreateInfo, uvBuffer.uv.data());
        m_geometryInfo = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(geometryBufferCreateInfo, instanceOffsetBuffer.geometryInfos.data());
    }
}

void DDGIApplication::initDDGIVolumn()
{
    //glm::vec3 startPosition = glm::vec3(-5.902552, 0.11353754, -6.484822);
    //glm::vec3 endPosition = glm::vec3(2.109489, 3.620252, 6.8349577);

    //glm::vec3 startPosition = glm::vec3(-5.6f, -1.1f, -9.3f);
    //glm::vec3 endPosition = glm::vec3(4.8f, 7.4f, 4.5f);

    //glm::vec3 startPosition = glm::vec3(-11.7112f, -0.678682f, -5.10776f);
    //glm::vec3 endPosition = glm::vec3(10.7607f, 11.0776f, 5.90468f);
    //glm::vec3 scale = endPosition - startPosition;
    //glm::ivec3 probeDim = glm::ivec3(8, 8, 8);
    //glm::ivec3 probeDim = m_probeDim;
    glm::ivec3 probeDim = m_jsonLoader->GetDDGIProbeDim();
    //glm::vec3 offset = glm::vec3(3.0f, 1.5f, 1.5f);

    auto sceneAABB = m_scene->GetBoundingBox();
    glm::vec3 scale = sceneAABB.pMax - sceneAABB.pMin;

    m_volume = std::make_shared<DDGIVolume>(sceneAABB.pMin + 0.3f * scale / (glm::vec3)probeDim, sceneAABB.pMax - glm::vec3(0.1f), probeDim);
}

void DDGIApplication::createGbufferPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        RenderPassCreateInfo info{};
        //info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_UINT);
        //info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_UINT);
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        info.pipelineCreateInfo.depthAttachmentFormats = device.FindDepthFormat();
        info.frambufferCount = 1;
        info.dynamicRender = 1;

        m_gbufferPass = std::make_shared<RenderPass>(device, info);

        auto gbufferShader = Singleton<ShaderManager>::instance().GetShader<ShaderModule>("GBuffer Shader");
        
        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_gbufferPass, gbufferShader);

        std::vector<std::shared_ptr<MVulkanTexture>> gbufferTextures = Singleton<TextureManager>::instance().GenerateTextures();

        std::vector<PassResources> resources;
        resources.push_back(
            PassResources::SetBufferResource(
                "vpBuffer", 0, 0));

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
            PassResources::SetSampledImageResource(
                4, 0, gbufferTextures));
        resources.push_back(
            PassResources::SetSamplerResource(
                5, 0, m_linearSamplerWithAnisotropy.GetSampler()));


        m_gbufferPass->UpdateDescriptorSetWrite(0, resources);
    }
}

void DDGIApplication::createProbeTracingPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    //{
    //    auto device = Singleton<MVulkanEngine>::instance().GetDevice();
    //
    //    m_probeTracingPass = std::make_shared<ComputePass>(device);
    //
    //    auto shader = Singleton<ShaderManager>::instance().GetShader<ComputeShaderModule>("ProbeTracing Shader");
    //
    //    Singleton<MVulkanEngine>::instance().CreateComputePass(
    //        m_probeTracingPass, shader);
    //
    //    std::vector<PassResources> resources;
    //    std::vector<std::shared_ptr<MVulkanTexture>> bufferTextures = Singleton<TextureManager>::instance().GenerateTextures();
    //
    //    //resources.push_back(
    //    //    PassResources::SetBufferResource(
    //    //        "texBuffer", 0, 0));
    //
    //    resources.push_back(
    //        PassResources::SetBufferResource(
    //            "ddgiBuffer", 0, 0));
    //
    //    resources.push_back(
    //        PassResources::SetBufferResource(
    //            "ddgiLightBuffer", 1, 0));
    //
    //    resources.push_back(
    //        PassResources::SetBufferResource(
    //            "probeTraceDispatchDimBuffer", 20, 0));
    //
    //    resources.push_back(
    //        PassResources::SetBufferResource(
    //            2, 0, m_tlasVertexBuffer));
    //    resources.push_back(
    //        PassResources::SetBufferResource(
    //            3, 0, m_tlasIndexBuffer));
    //    resources.push_back(
    //        PassResources::SetBufferResource(
    //            4, 0, m_tlasNormalBuffer));
    //    resources.push_back(
    //        PassResources::SetBufferResource(
    //            5, 0, m_tlasUVBuffer));
    //    resources.push_back(
    //        PassResources::SetBufferResource(
    //            6, 0, m_geometryInfo));
    //    resources.push_back(
    //        PassResources::SetBufferResource(
    //            7, 0, m_materialBuffer));
    //    resources.push_back(
    //        PassResources::SetBufferResource(
    //            8, 0, m_probesDataBuffer));
    //
    //    resources.push_back(
    //        PassResources::SetSampledImageResource(
    //            9, 0, bufferTextures));
    //    resources.push_back(
    //        PassResources::SetSampledImageResource(
    //            10, 0, m_volumeProbeDatasRadiance));
    //    resources.push_back(
    //        PassResources::SetSampledImageResource(
    //            11, 0, m_volumeProbeDatasDepth));
    //
    //    resources.push_back(
    //        PassResources::SetSamplerResource(
    //            12, 0, m_linearSamplerWithAnisotropy.GetSampler()));
    //
    //    auto tlas = m_rayTracing.GetTLAS();
    //    resources.push_back(
    //        PassResources::SetAccelerationStructureResource(
    //            13, 0, &tlas));
    //
    //    resources.push_back(
    //        PassResources::SetStorageImageResource(
    //            14, 0, m_probePositions));
    //    resources.push_back(
    //        PassResources::SetStorageImageResource(
    //            15, 0, m_probeNormals));
    //    resources.push_back(
    //        PassResources::SetStorageImageResource(
    //            16, 0, m_probeDepth));
    //    resources.push_back(
    //        PassResources::SetStorageImageResource(
    //            17, 0, m_probeRadiance));
    //    resources.push_back(
    //        PassResources::SetStorageImageResource(
    //            18, 0, m_matIdTexture));
    //    resources.push_back(
    //        PassResources::SetStorageImageResource(
    //            19, 0, m_texCoordsTexture));
    //
    //    m_probeTracingPass->UpdateDescriptorSetWrite(resources);
    //}

    {
        RenderPassCreateInfo info{};
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        info.pipelineCreateInfo.depthAttachmentFormats = device.FindDepthFormat();

        info.frambufferCount = 1;
        info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
        info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
        info.dynamicRender = 1;
        info.useDepthBuffer = true;

        m_probeTracingRenderPass = std::make_shared<RenderPass>(device, info);

        //std::shared_ptr<DDGILightingShader> lightingShader = std::make_shared<DDGILightingShader>();
        auto shader = Singleton<ShaderManager>::instance().GetShader<ShaderModule>("ProbeTracing Shader2");

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_probeTracingRenderPass, shader);

        std::vector<PassResources> resources;
        std::vector<std::shared_ptr<MVulkanTexture>> bufferTextures = Singleton<TextureManager>::instance().GenerateTextures();

        resources.push_back(
            PassResources::SetBufferResource(
                "ddgiBuffer", 0, 0));

        resources.push_back(
            PassResources::SetBufferResource(
                "ddgiLightBuffer", 1, 0));

        resources.push_back(
            PassResources::SetBufferResource(
                2, 0, m_tlasVertexBuffer));
        resources.push_back(
            PassResources::SetBufferResource(
                3, 0, m_tlasIndexBuffer));
        resources.push_back(
            PassResources::SetBufferResource(
                4, 0, m_tlasNormalBuffer));
        resources.push_back(
            PassResources::SetBufferResource(
                5, 0, m_tlasUVBuffer));
        resources.push_back(
            PassResources::SetBufferResource(
                6, 0, m_geometryInfo));
        resources.push_back(
            PassResources::SetBufferResource(
                7, 0, m_materialBuffer));
        resources.push_back(
            PassResources::SetBufferResource(
                8, 0, m_probesDataBuffer));

        resources.push_back(
            PassResources::SetSampledImageResource(
                9, 0, bufferTextures));
        resources.push_back(
            PassResources::SetSampledImageResource(
                10, 0, m_volumeProbeDatasRadiance));
        resources.push_back(
            PassResources::SetSampledImageResource(
                11, 0, m_volumeProbeDatasDepth));

        resources.push_back(
            PassResources::SetSamplerResource(
                12, 0, m_linearSamplerWithoutAnisotropy.GetSampler()));

        auto tlas = m_rayTracing.GetTLAS();
        resources.push_back(
            PassResources::SetAccelerationStructureResource(
                13, 0, &tlas));

        m_probeTracingRenderPass->UpdateDescriptorSetWrite(0, resources);
    }
}

void DDGIApplication::createLightingPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        RenderPassCreateInfo info{};
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        info.pipelineCreateInfo.depthAttachmentFormats = device.FindDepthFormat();

        info.frambufferCount = 1;
        info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
        info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
        info.dynamicRender = 1;
        info.useDepthBuffer = false;

        m_lightingPass = std::make_shared<RenderPass>(device, info);

        auto shader = Singleton<ShaderManager>::instance().GetShader<ShaderModule>("Lighting Shader");

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_lightingPass, shader);

        std::vector<PassResources> resources;

        resources.push_back(
            PassResources::SetBufferResource(
                "lightBuffer", 0, 0));
        resources.push_back(
            PassResources::SetBufferResource(
                "cameraBuffer", 1, 0));

        resources.push_back(
            PassResources::SetBufferResource(
                "ddgiBuffer", 3, 0));
        resources.push_back(
            PassResources::SetBufferResource(
                4, 0, m_probesDataBuffer));
        resources.push_back(
            PassResources::SetSampledImageResource(
                5, 0, gBuffer0));
        resources.push_back(
            PassResources::SetSampledImageResource(
                6, 0, gBuffer1));
        resources.push_back(
            PassResources::SetSampledImageResource(
                7, 0, gBuffer3));
        resources.push_back(
            PassResources::SetSampledImageResource(
                8, 0, gBuffer2));
        resources.push_back(
            PassResources::SetSampledImageResource(
                9, 0, m_volumeProbeDatasRadiance));
        resources.push_back(
            PassResources::SetSampledImageResource(
                10, 0, m_volumeProbeDatasDepth));
        resources.push_back(
            PassResources::SetSamplerResource(
                11, 0, m_linearSamplerWithoutAnisotropy.GetSampler()));
        auto tlas = m_rayTracing.GetTLAS();
        resources.push_back(
            PassResources::SetAccelerationStructureResource(
                12, 0, &tlas));

        m_lightingPass->UpdateDescriptorSetWrite(0, resources);
    }
}

void DDGIApplication::createRTAOPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        std::vector<VkFormat> rtaoPassFormats;
        rtaoPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());

        RenderPassCreateInfo info{};
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32_SFLOAT);
        info.pipelineCreateInfo.depthAttachmentFormats = device.FindDepthFormat();
        info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
        info.depthFormat = device.FindDepthFormat();
        info.frambufferCount = 1;

        info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
        info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
        info.dynamicRender = 1;
        info.useDepthBuffer = false;

        m_rtaoPass = std::make_shared<RenderPass>(device, info);
        auto rtaoShader = Singleton<ShaderManager>::instance().GetShader<ShaderModule>("RTAO Shader");

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_rtaoPass, rtaoShader);

        std::vector<PassResources> resources;

        resources.push_back(
            PassResources::SetBufferResource(
                "RtaoBuffer", 0, 0));
        resources.push_back(
            PassResources::SetBufferResource(
                "screenBuffer", 1, 0));
        resources.push_back(
            PassResources::SetSampledImageResource(
                2, 0, gBuffer0));
        resources.push_back(
            PassResources::SetSampledImageResource(
                3, 0, gBuffer1));
        resources.push_back(
            PassResources::SetStorageImageResource(
                4, 0, m_acculatedAOTexture));
        resources.push_back(
            PassResources::SetSamplerResource(
                5, 0, m_linearSamplerWithAnisotropy.GetSampler()));
        auto tlas = m_rayTracing.GetTLAS();
        resources.push_back(
            PassResources::SetAccelerationStructureResource(
                6, 0, &tlas));

        m_rtaoPass->UpdateDescriptorSetWrite(0, resources);
    }
}

void DDGIApplication::createProbeBlendingRadiancePass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    m_probeBlendingRadiancePass = std::make_shared<ComputePass>(device);

    auto shader = Singleton<ShaderManager>::instance().GetShader<ComputeShaderModule>("ProbeBlendingRadiance Shader");

    Singleton<MVulkanEngine>::instance().CreateComputePass(
        m_probeBlendingRadiancePass, shader);

    std::vector<PassResources> resources;

    //PassResources resource;
    resources.push_back(PassResources::SetBufferResource("ddgiBuffer", 1, 0, 0));
    resources.push_back(PassResources::SetBufferResource(2, 0, m_probesDataBuffer));
    resources.push_back(PassResources::SetSampledImageResource(3, 0, m_probePositions));
    resources.push_back(PassResources::SetSampledImageResource(4, 0, m_probeRadiance));
    resources.push_back(PassResources::SetStorageImageResource(5, 0, m_volumeProbeDatasRadiance));
    resources.push_back(PassResources::SetStorageImageResource(6, 0, m_volumeProbeDatasDepth));
    resources.push_back(
        PassResources::SetSamplerResource(
            7, 0, m_linearSamplerWithAnisotropy.GetSampler()));

    m_probeBlendingRadiancePass->UpdateDescriptorSetWrite(resources);
}

void DDGIApplication::createProbeBlendingDepthPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    m_probeBlendingDepthPass = std::make_shared<ComputePass>(device);

    auto shader = Singleton<ShaderManager>::instance().GetShader<ComputeShaderModule>("ProbeBlendingDepth Shader");

    Singleton<MVulkanEngine>::instance().CreateComputePass(
        m_probeBlendingDepthPass, shader);

    std::vector<PassResources> resources;

    //PassResources resource;
    resources.push_back(PassResources::SetBufferResource("ddgiBuffer", 1, 0, 0));
    resources.push_back(PassResources::SetBufferResource(2, 0, m_probesDataBuffer));
    resources.push_back(PassResources::SetSampledImageResource(3, 0, m_probePositions));
    resources.push_back(PassResources::SetSampledImageResource(4, 0, m_probeRadiance));
    resources.push_back(PassResources::SetStorageImageResource(5, 0, m_volumeProbeDatasRadiance));
    resources.push_back(PassResources::SetStorageImageResource(6, 0, m_volumeProbeDatasDepth));
    resources.push_back(
        PassResources::SetSamplerResource(
            7, 0, m_linearSamplerWithAnisotropy.GetSampler()));

    m_probeBlendingDepthPass->UpdateDescriptorSetWrite(resources);
}

void DDGIApplication::createProbeClassficationPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    m_probeClassficationPass = std::make_shared<ComputePass>(device);

    auto shader = Singleton<ShaderManager>::instance().GetShader<ComputeShaderModule>("ProbeClassfication Shader");

    Singleton<MVulkanEngine>::instance().CreateComputePass(
        m_probeClassficationPass, shader);

    std::vector<PassResources> resources;

    //PassResources resource;
    resources.push_back(PassResources::SetBufferResource("ddgiBuffer", 1, 0, 0));
    resources.push_back(PassResources::SetBufferResource(2, 0, m_probesDataBuffer));
    resources.push_back(PassResources::SetSampledImageResource(3, 0, m_probePositions));
    resources.push_back(PassResources::SetSampledImageResource(4, 0, m_probeRadiance));
    resources.push_back(PassResources::SetStorageImageResource(5, 0, m_testTexture));

    m_probeClassficationPass->UpdateDescriptorSetWrite(resources);
}


void DDGIApplication::createProbeRelocationPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    m_probeRelocationPass = std::make_shared<ComputePass>(device);

    auto shader = Singleton<ShaderManager>::instance().GetShader<ComputeShaderModule>("ProbeRelocation Shader");

    Singleton<MVulkanEngine>::instance().CreateComputePass(
        m_probeRelocationPass, shader);

    std::vector<PassResources> resources;

    //PassResources resource;
    resources.push_back(PassResources::SetBufferResource("ddgiBuffer", 0, 0, 0));
    resources.push_back(PassResources::SetBufferResource(1, 0, m_probesDataBuffer));
    resources.push_back(PassResources::SetBufferResource(2, 0, m_probesModelBuffer));
    resources.push_back(PassResources::SetSampledImageResource(3, 0, m_probePositions));
    resources.push_back(PassResources::SetSampledImageResource(4, 0, m_probeRadiance));

    m_probeRelocationPass->UpdateDescriptorSetWrite(resources);
}

void DDGIApplication::createProbeVisulizePass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        RenderPassCreateInfo info{};
        info.useSwapchainImages = true;
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());
        info.pipelineCreateInfo.depthAttachmentFormats = device.FindDepthFormat();

        info.frambufferCount = Singleton<MVulkanEngine>::instance().GetSwapchainImageCount();;
        info.dynamicRender = true;
        info.pipelineCreateInfo.depthTestEnable = VK_TRUE;
        info.pipelineCreateInfo.depthWriteEnable = VK_TRUE;

        m_probeVisulizePass = std::make_shared<RenderPass>(device, info);
        auto shader = Singleton<ShaderManager>::instance().GetShader<ShaderModule>("ProbeVisulize Shader");

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_probeVisulizePass, shader);


        for (int i = 0; i < info.frambufferCount; i++) {
            std::vector<PassResources> resources;

            resources.push_back(
                PassResources::SetBufferResource(
                    "vpBuffer", 0, 0, i));
            resources.push_back(
                PassResources::SetBufferResource(
                    "ddgiBuffer", 1, 0, i));
            resources.push_back(
                PassResources::SetBufferResource(
                    2, 0, m_probesModelBuffer));
            resources.push_back(
                PassResources::SetBufferResource(
                    3, 0, m_probesDataBuffer));
            resources.push_back(
                PassResources::SetSampledImageResource(
                    4, 0, m_volumeProbeDatasRadiance));
            resources.push_back(
                PassResources::SetSamplerResource(
                    5, 0, m_linearSamplerWithAnisotropy.GetSampler()));

            m_probeVisulizePass->UpdateDescriptorSetWrite(i, resources);
        }
    }
}

void DDGIApplication::createCompositeScenePass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        RenderPassCreateInfo info{};
        info.useSwapchainImages = true;
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());
        info.pipelineCreateInfo.depthAttachmentFormats = device.FindDepthFormat();

        info.frambufferCount = Singleton<MVulkanEngine>::instance().GetSwapchainImageCount();
        info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
        info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
        info.dynamicRender = 1;
        info.useDepthBuffer = false;

        m_compositeScenePass = std::make_shared<RenderPass>(device, info);

        auto shader = Singleton<ShaderManager>::instance().GetShader<ShaderModule>("CompositeScene Shader");

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_compositeScenePass, shader);

        for (int i = 0; i < info.frambufferCount; i++) {
            std::vector<PassResources> resources;

            resources.push_back(
                PassResources::SetBufferResource(
                    "ddgiCompositeBuffer", 0, 0, i));
            resources.push_back(
                PassResources::SetSampledImageResource(
                    1, 0, m_diTexture));
            resources.push_back(
                PassResources::SetSampledImageResource(
                    2, 0, m_giTexture));
            resources.push_back(
                PassResources::SetSampledImageResource(
                    3, 0, m_acculatedAOTexture));

            resources.push_back(
                PassResources::SetSamplerResource(
                    4, 0, m_linearSamplerWithAnisotropy.GetSampler()));

            m_compositeScenePass->UpdateDescriptorSetWrite(i, resources);
        }
    }
}

void DDGIApplication::changeTextureLayoutToRWTexture()
{
    std::vector<MVulkanImageMemoryBarrier> barriers;
    {
        MVulkanImageMemoryBarrier barrier{};
        barrier.image = m_volumeProbeDatasRadiance->GetImage();
        barrier.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.baseArrayLayer = 0;
        barrier.layerCount = 1;
        barrier.levelCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barriers.push_back(barrier);

        barrier.image = m_volumeProbeDatasDepth->GetImage();
        barriers.push_back(barrier);
    }
    Singleton<MVulkanEngine>::instance().TransitionImageLayout(barriers, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
}

void DDGIApplication::changeRWTextureLayoutToTexture()
{
    std::vector<MVulkanImageMemoryBarrier> barriers;
    {
        MVulkanImageMemoryBarrier barrier{};
        barrier.image = m_volumeProbeDatasRadiance->GetImage();
        barrier.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.baseArrayLayer = 0;
        barrier.layerCount = 1;
        barrier.levelCount = 1;
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = 0;
        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barriers.push_back(barrier);

        barrier.image = m_volumeProbeDatasDepth->GetImage();
        barriers.push_back(barrier);
    }
    Singleton<MVulkanEngine>::instance().TransitionImageLayout(barriers, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

}

void DDGIApplication::transitionProbeVisulizeTextureLayoutToShaderRead()
{
    std::vector<MVulkanImageMemoryBarrier> barriers;
    {
        MVulkanImageMemoryBarrier barrier{};
        barrier.image = m_probeVisulizePass->GetFrameBuffer(0).GetImage(0);
        barrier.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.baseArrayLayer = 0;
        barrier.layerCount = 1;
        barrier.levelCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barriers.push_back(barrier);

        //barrier.image = m_volumeProbeDatasDepth->GetImage();
        //barriers.push_back(barrier);
    }
    Singleton<MVulkanEngine>::instance().TransitionImageLayout(barriers, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}

void DDGIApplication::transitionProbeVisulizeTextureLayoutToUndifined()
{
    std::vector<MVulkanImageMemoryBarrier> barriers;
    {
        MVulkanImageMemoryBarrier barrier{};
        barrier.image = m_probeVisulizePass->GetFrameBuffer(0).GetImage(0);
        barrier.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.baseArrayLayer = 0;
        barrier.layerCount = 1;
        barrier.levelCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barriers.push_back(barrier);
    }
    Singleton<MVulkanEngine>::instance().TransitionImageLayout(barriers, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}

void DDGIApplication::loadShaders()
{
    Singleton<ShaderManager>::instance().AddShader("GBuffer Shader", { "hlsl/gbuffer/gbuffer.vert.hlsl", "hlsl/gbuffer/gbuffer.frag.hlsl", "main", "main" });
    Singleton<ShaderManager>::instance().AddShader("ProbeTracing Shader", { "hlsl/ddgi/probeTrace.comp.hlsl", "main"});
    Singleton<ShaderManager>::instance().AddShader("ProbeTracing Shader2", { "hlsl/ddgi/fullScreen.vert.hlsl", "hlsl/ddgi/probeTrace.frag.hlsl", "main", "main" });
    Singleton<ShaderManager>::instance().AddShader("RTAO Shader", { "hlsl/ddgi/fullscreen.vert.hlsl", "hlsl/ddgi/rtao.frag.hlsl", "main", "main" });
    Singleton<ShaderManager>::instance().AddShader("Lighting Shader", { "hlsl/ddgi/fullScreen.vert.hlsl", "hlsl/ddgi/ddgiLighting.frag.hlsl", "main", "main" });
    Singleton<ShaderManager>::instance().AddShader("ProbeRelocation Shader", { "hlsl/ddgi/probeRelocation.comp.hlsl", "main" });
    Singleton<ShaderManager>::instance().AddShader("ProbeBlendingRadiance Shader", { "hlsl/ddgi/probeBlend.comp.hlsl", "main_radiance" });
    Singleton<ShaderManager>::instance().AddShader("ProbeBlendingDepth Shader", { "hlsl/ddgi/probeBlend.comp.hlsl", "main_depth" });
    Singleton<ShaderManager>::instance().AddShader("ProbeClassfication Shader", { "hlsl/ddgi/probeClassfication.comp.hlsl", "main" });
    Singleton<ShaderManager>::instance().AddShader("ProbeVisulize Shader", { "hlsl/ddgi/probeVisulize.vert.hlsl", "hlsl/ddgi/probeVisulize.frag.hlsl", "main", "main" });
    Singleton<ShaderManager>::instance().AddShader("CompositeScene Shader", { "hlsl/ddgi/fullScreen.vert.hlsl", "hlsl/ddgi/ddgiSceneComposite.frag.hlsl", "main", "main" });
}

void DDGIApplication::createSyncObjs()
{
    m_ddgiSemephore.Create(Singleton<MVulkanEngine>::instance().GetDevice().GetDevice());
    m_shadingSemaphore.Create(Singleton<MVulkanEngine>::instance().GetDevice().GetDevice());
}

void DDGIApplication::initUIRenderer()
{
    m_uiRenderer = std::make_shared<DDGIUI>();
}

static const char* VisualizeModes[] = {
    "DI",
    "GI",
    "AO",
    "DDGI(Without AO)",
    "DDGI"
};

void DDGIUI::RenderContext()
{
    ImGui::Begin("DDGI_UI Window", &shouleRenderUI);

    double fps = this->m_app->GetFPS();
    ImGui::Text("FPS: %.2f", fps);

    ImGui::Checkbox("enable probe relocation", &m_probeRelocationEnabled);
    ImGui::Checkbox("enable probe classfication", &m_probeClassfication);

    ImGui::Combo("outputContext", &m_visulizeMode, VisualizeModes, IM_ARRAYSIZE(VisualizeModes));

    ImGui::Checkbox("showPassTime", &m_showPassTime);
    if (m_showPassTime) {
        ImGui::Text("Gbuffer Pass Time: %.3f ms", ((DDGIApplication*)(this->m_app))->m_gbufferTime);
        ImGui::Text("ProbeTracing Pass Time: %.3f ms", ((DDGIApplication*)(this->m_app))->m_probeTracingTime);
        ImGui::Text("ProbeClassfication Pass Time: %.3f ms", ((DDGIApplication*)(this->m_app))->m_probeClassficationTime);
        ImGui::Text("ProbeRlelocation Pass Time: %.3f ms", ((DDGIApplication*)(this->m_app))->m_probeRelocationTime);
        ImGui::Text("ProbeBlendDepth Pass Time: %.3f ms", ((DDGIApplication*)(this->m_app))->m_probeBlendDepthTime);
        ImGui::Text("ProbeBlendRadiance Pass Time: %.3f ms", ((DDGIApplication*)(this->m_app))->m_probeBlendRadianceTime);
        ImGui::Text("Lightening Pass Time: %.3f ms", ((DDGIApplication*)(this->m_app))->m_lightingTime);
        ImGui::Text("Rtao Pass Time: %.3f ms", ((DDGIApplication*)(this->m_app))->m_rtaoTime);
        ImGui::Text("Composite Pass Time: %.3f ms", ((DDGIApplication*)(this->m_app))->m_compositeTime);
    }

    ImGui::End();
}
