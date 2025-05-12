#include "pbr.hpp"

#include "MVulkanRHI/MVulkanEngine.hpp"
#include "Scene/SceneLoader.hpp"
#include "Scene/Scene.hpp"
#include "Managers/TextureManager.hpp"
#include "Managers/ShaderManager.hpp"
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
    createSamplers();
    createLight();
    createCamera();
    createTextures();
    loadShaders();

    loadScene();
    createStorageBuffers(); 
}

void PBR::ComputeAndDraw(uint32_t imageIndex)
{
    auto& graphicsList = Singleton<MVulkanEngine>::instance().GetGraphicsList(m_currentFrame);
    auto& graphicsQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::GRAPHICS);

    auto& computeList = Singleton<MVulkanEngine>::instance().GetComputeCommandList();
    auto& computeQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::COMPUTE);

    //ImageLayoutToAttachment(imageIndex);
    auto swapchainExtent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();

    //if()

    //prepare gbufferPass ubo
    {
        //MVPBuffer mvpBuffer{};
        //mvpBuffer.Model = glm::mat4(1.f);
        VPBuffer vpBuffer{};
        vpBuffer.View = m_camera->GetViewMatrix();
        vpBuffer.Projection = m_camera->GetProjMatrix();
        Singleton<ShaderResourceManager>::instance().LoadData("vpBuffer", 0, &vpBuffer, 0);

        VPBuffer vpBuffer_p{};
        vpBuffer_p.View = m_prevView;
        vpBuffer_p.Projection = m_prevProj;
        Singleton<ShaderResourceManager>::instance().LoadData("vpBuffer_p", 0, &vpBuffer_p, 0);
        //m_gbufferPass->GetShader()->SetUBO(0, &ubo0);

        TexBuffer texBuffer = m_scene->GenerateTexBuffer();
        Singleton<ShaderResourceManager>::instance().LoadData("texBuffer", 0, &texBuffer, 0);
        //m_gbufferPass->GetShader()->SetUBO(1, &ubo1);

        auto gbufferExtent = swapchainExtent;
        MScreenBuffer gBufferInfoBuffer{};
        gBufferInfoBuffer.WindowRes = int2(gbufferExtent.width, gbufferExtent.height);

        Singleton<ShaderResourceManager>::instance().LoadData("gBufferInfoBuffer", 0, &gBufferInfoBuffer, 0);

    }

    //prepare shadowPass ubo
    {
        glm::mat4 shadowMVP{};
        shadowMVP = m_directionalLightCamera->GetOrthoMatrix() * m_directionalLightCamera->GetViewMatrix();
        Singleton<ShaderResourceManager>::instance().LoadData("ShadowMVPBuffer", 0, &shadowMVP, 0);
        //m_shadowPass->GetShader()->SetUBO(0, &ubo0);
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

    {
        auto frustum = m_camera->GetFrustum();
        FrustumBuffer buffer;
        for (int i = 0; i < 6; i++) {
            buffer.planes[i] = frustum.planes[i];
        }

        MSceneBuffer sceneBuffer;
        sceneBuffer.numInstances = m_scene->GetIndirectDrawCommands().size();
        sceneBuffer.targetIndex = 0;
        sceneBuffer.cullingMode = std::static_pointer_cast<DRUI>(m_uiRenderer)->cullingMode;

        Singleton<ShaderResourceManager>::instance().LoadData("FrustumBuffer", 0, &buffer, 0);
        Singleton<ShaderResourceManager>::instance().LoadData("sceneBuffer", 0, &sceneBuffer, 0);
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
                //.view = swapchainDepthViews[imageIndex]->GetImageView(),
        };
    }
    gbufferRenderInfo.offset = { 0, 0 };
    gbufferRenderInfo.extent = swapchainExtent;

    shadowmapRenderInfo.offset = { 0, 0 };
    shadowmapRenderInfo.extent = shadowmapExtent;

    shadingRenderInfo.offset = { 0, 0 };
    shadingRenderInfo.extent = swapchainExtent;



    computeList.Reset();
    computeList.Begin();

    auto numInstances = m_scene->GetIndirectDrawCommands().size();
    Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_frustumCullingPass, (numInstances+7)/8, 1, 1, std::string("FrustumCulling Pass"));

    computeList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &computeList.GetBuffer();
    computeQueue.SubmitCommands(1, &submitInfo, nullptr);
    computeQueue.WaitForQueueComplete();

    graphicsList.Reset();
    graphicsList.Begin();

    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_shadowPass, m_currentFrame, shadowmapRenderInfo, m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), m_scene->GetIndirectBuffer(), m_scene->GetIndirectDrawCommands().size(), std::string("Shadowmap Pass"));
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_gbufferPass, m_currentFrame, gbufferRenderInfo, m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), m_culledIndirectDrawBuffer, m_scene->GetIndirectDrawCommands().size(), std::string("Gbuffer Pass"));
    
    auto hizMode = std::static_pointer_cast<DRUI>(m_uiRenderer)->hizMode;
    if (hizMode == DO_HIZ_MODE_0) {

        graphicsList.End();
        submitInfo.pCommandBuffers = &graphicsList.GetBuffer();
        graphicsQueue.SubmitCommands(1, &submitInfo, nullptr);
        graphicsQueue.WaitForQueueComplete();

        computeList.Reset();
        computeList.Begin();

        auto numHizLayers = m_hiz.hizRes.size();
        for (auto layer = 0; layer < numHizLayers; layer++) {
            if (layer == 0) {
                Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_resetHizBufferPass, 1, 1, 1, std::string("ResetHizBuffer Pass"));
            }
            else {
                Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_updateHizBufferPass, 1, 1, 1, std::string("UpdateHizBuffer Pass"));
            }
            Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_hizPass, (m_hiz.hizRes[layer].x + 15) / 16, (m_hiz.hizRes[layer].y + 15) / 16, 1, std::string("GenHiz Pass"));
        }

        computeList.End();
        submitInfo.pCommandBuffers = &computeList.GetBuffer();
        computeQueue.SubmitCommands(1, &submitInfo, nullptr);
        computeQueue.WaitForQueueComplete();

        graphicsList.Reset();
        graphicsList.Begin();
    }
    
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(imageIndex, m_lightingPass, m_currentFrame, shadingRenderInfo, m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size(), std::string("Shading Pass"));

    graphicsList.End();
    Singleton<MVulkanEngine>::instance().SubmitGraphicsCommands(imageIndex, m_currentFrame);

    m_prevView = m_camera->GetViewMatrix();
    m_prevProj = m_camera->GetProjMatrix();
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
    createFrustumCullingPass();
    createGenHizPass();
    createResetHizBufferPass();
    createUpdateHizBufferPass();

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
    float zFar = 1000.f;

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
    auto shadowMapExtent = shadowmapExtent;

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
        auto motionVectorFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

        gBuffer0 = std::make_shared<MVulkanTexture>();
        gBuffer1 = std::make_shared<MVulkanTexture>();
        //gBuffer2 = std::make_shared<MVulkanTexture>();
        //gBuffer3 = std::make_shared<MVulkanTexture>();
        //gBuffer4 = std::make_shared<MVulkanTexture>();
        gBufferDepth = std::make_shared<MVulkanTexture>();
        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            gBuffer0, swapchainExtent, gbufferFormat
        );

        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            gBuffer1, swapchainExtent, gbufferFormat
        );

        //Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
        //    gBuffer2, swapchainExtent, motionVectorFormat
        //);

        //Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
        //    gBuffer3, swapchainExtent, motionVectorFormat
        //);
        //
        //Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
        //    gBuffer4, swapchainExtent, motionVectorFormat
        //);
        //
        Singleton<MVulkanEngine>::instance().CreateDepthAttachmentImage(
            gBufferDepth, swapchainExtent, depthFormat
        );
    }

    {
        m_hiz.UpdateHizRes(glm::ivec2(swapchainExtent.width, swapchainExtent.height));
        auto hizLayers = m_hiz.hizRes.size();
        m_hizTextures.resize(hizLayers);
        auto depthFormat = device.FindDepthFormat();

        ImageCreateInfo imageInfo;
        ImageViewCreateInfo viewInfo;
        imageInfo.arrayLength = 1;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageInfo.format = depthFormat;

        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = imageInfo.format;
        viewInfo.flag = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.baseMipLevel = 0;
        viewInfo.levelCount = 1;
        viewInfo.baseArrayLayer = 0;
        viewInfo.layerCount = 1;

        for (auto i = 0; i < hizLayers; i++) {
            auto res = m_hiz.hizRes[i];
            
            //auto depthFormat = device.FindDepthFormat();
            imageInfo.width = res.x;
            imageInfo.height = res.y;

            m_hizTextures[i] = std::make_shared<MVulkanTexture>();
            Singleton<MVulkanEngine>::instance().CreateImage(m_hizTextures[i], imageInfo, viewInfo);
        }
    }

}

void PBR::loadShaders() 
{
    //Singleton<ShaderManager>::instance().AddShader("GBuffer Shader", { "hlsl/gbuffer.vert.hlsl", "hlsl/gbuffer/gbuffer.frag.hlsl", "main", "main" });
    Singleton<ShaderManager>::instance().AddShader("GBuffer Shader", { "hlsl/gbuffer/gbuffer2.vert.hlsl", "hlsl/gbuffer/gbuffer2.frag.hlsl", "main", "main" });
    Singleton<ShaderManager>::instance().AddShader("Shadow Shader", { "glsl/shadow.vert.glsl", "glsl/shadow.frag.glsl" });
    //Singleton<ShaderManager>::instance().AddShader("Shading Shader", { "hlsl/lighting_pbr.vert.hlsl", "hlsl/lighting_pbr_packed.frag.hlsl" }, true);
    Singleton<ShaderManager>::instance().AddShader("Shading Shader", { "hlsl/lighting_pbr.vert.hlsl", "hlsl/lighting_pbr_packed2.frag.hlsl" }, true);
    Singleton<ShaderManager>::instance().AddShader("FrustumCulling Shader", { "hlsl/culling/FrustumCulling.comp.hlsl" }, true);
    Singleton<ShaderManager>::instance().AddShader("GenHiz Shader", { "hlsl/culling/GenHiz.comp.hlsl" }, true);
    Singleton<ShaderManager>::instance().AddShader("UpdateHizBuffer Shader", { "hlsl/culling/UpdateHizBuffer.comp.hlsl" }, true);
    Singleton<ShaderManager>::instance().AddShader("ResetHizBuffer Shader", { "hlsl/culling/ResetHizBuffer.comp.hlsl" }, true);
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

        std::vector<InstanceBound> bounds(indirectDrawCommands.size());
        for (auto i = 0; i < bounds.size(); i++) {
            auto indirectDrawCommand = indirectDrawCommands[i];
            bounds[i].center = m_scene->GetMesh(indirectDrawCommand.firstInstance)->m_box.GetCenter();
            bounds[i].extent = m_scene->GetMesh(indirectDrawCommand.firstInstance)->m_box.GetExtent();
        }

        info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        info.size = sizeof(InstanceBound) * indirectDrawCommands.size();
        m_instanceBoundsBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, bounds.data());
    
        std::vector<HizDimension> hizDimensions(m_hiz.hizRes.size());
        for (int i = 0; i < hizDimensions.size(); i++) {
            hizDimensions[i].u_previousLevelDimensions = m_hiz.hizRes[i];
        }

        info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        info.size = sizeof(HizDimension) * hizDimensions.size();
        m_hizDimensionsBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, hizDimensions.data());

        info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        info.size = sizeof(HIZBuffer);
        m_hizBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info);

        std::vector<glm::mat4> models(m_scene->GetIndirectDrawCommands().size());
        for (auto i = 0; i < models.size(); i++) {
            models[i] = glm::mat4(1.0f);
        }
        info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        info.size = sizeof(glm::mat4) * models.size();
        m_modelBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, models.data());
    }
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
        std::vector<std::shared_ptr<MVulkanTexture>> bufferTextures = Singleton<TextureManager>::instance().GenerateTextures();

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
                "texBuffer", 3, 0));
        resources.push_back(
            PassResources::SetBufferResource(
                "gBufferInfoBuffer", 6, 0, 0));

        resources.push_back(
            PassResources::SetSampledImageResource(
                4, 0, bufferTextures));
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
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
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
                "ShadowMVPBuffer", 0, 0));

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


        //std::vector<std::shared_ptr<MVulkanTexture>> shadowViews{
        //    m_shadowPass->GetFrameBuffer(0).GetDepthTexture(),
        //    m_shadowPass->GetFrameBuffer(0).GetDepthTexture()
        //};
        std::vector<std::shared_ptr<MVulkanTexture>> shadowViews{
            shadowMapDepth,
            shadowMapDepth
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

        m_frustumCullingPass->UpdateDescriptorSetWrite(resources);
    }
}

void PBR::createGenHizPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        m_hizPass = std::make_shared<ComputePass>(device);

        auto shader = Singleton<ShaderManager>::instance().GetShader<ComputeShaderModule>("GenHiz Shader");

        Singleton<MVulkanEngine>::instance().CreateComputePass(
            m_hizPass, shader);

        std::vector<PassResources> resources;

        //PassResources resource;
        //resources.push_back(PassResources::SetBufferResource("hizBuffer", 0, 0, 0));
        resources.push_back(PassResources::SetBufferResource(0, 0, m_hizBuffer));
        resources.push_back(PassResources::SetSampledImageResource(1, 0, gBufferDepth));
        resources.push_back(PassResources::SetStorageImageResource(2, 0, m_hizTextures));

        m_hizPass->UpdateDescriptorSetWrite(resources);
    }
}

void PBR::createUpdateHizBufferPass()
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

void PBR::createResetHizBufferPass()
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


static const char* OutputBufferContents[] = {
    "Render",
    "Depth",
    "Albedo",
    "Normal",
    "Position",
    "Roughness",
    "Metallic",
    "MotionVector"
};

void DRUI::RenderContext() {
    ImGui::Begin("DR_UI Window", &shouleRenderUI);
    ImGui::Text("Hello from another window!");

    ImGui::RadioButton("cullingmode null", &cullingMode, CULLINGMODE_NULL); ImGui::SameLine();
    ImGui::RadioButton("cullingmode sphere", &cullingMode, CULLINGMODE_SPHERE); ImGui::SameLine();
    ImGui::RadioButton("cullingmode bbx", &cullingMode, CULLINGMODE_AABB);

    ImGui::RadioButton("not do hiz", &hizMode, NOT_DO_HIZ); ImGui::SameLine();
    ImGui::RadioButton("hiz mode 0", &hizMode, DO_HIZ_MODE_0);// ImGui::SameLine();
    //ImGui::RadioButton("hiz mode 1", &hizMode, DO_HIZ_MODE_1);

    ImGui::Combo("outputContext", &outputContext, OutputBufferContents, IM_ARRAYSIZE(OutputBufferContents));
    //static int outputContext = 0;
    //ImGui::RadioButton("not show motionVector", &showMotionVector, 0); ImGui::SameLine();
    //ImGui::RadioButton("show motionVector", &showMotionVector, 1);// ImGui::SameLine();

    auto app = (PBR*)(this->m_app);
    auto camera = app->GetMainCamera();
    auto& cameraPosition = camera->GetPositionRef();
    auto& cameraDirection = camera->GetDirectionRef();

    ImGui::InputFloat3("Camera Position:", &cameraPosition[0], "%.3f");
    ImGui::InputFloat3("Camera Direction:", &cameraDirection[0], "%.3f");
    //ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", cameraPosition.x, cameraPosition.y, cameraPosition.z);
    //ImGui::Text("Camera Direction: (%.2f, %.2f, %.2f)", cameraDirection.x, cameraDirection.y, cameraDirection.z);

    //ImGui::RadioButton("show motionVector2", &showMotionVector, 2);
    //if (ImGui::Button("Close Me"))
    //    show_another_window = false;
    ImGui::End();
}