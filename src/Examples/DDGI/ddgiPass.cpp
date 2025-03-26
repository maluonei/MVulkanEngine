#include "ddgiPass.hpp"

#include "MVulkanRHI/MVulkanEngine.hpp"
#include "Scene/SceneLoader.hpp"
#include "Scene/Scene.hpp"
#include "Managers/TextureManager.hpp"
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

    //prepare gbufferPass ubo
    {
        GbufferShader::UniformBufferObject0 ubo0{};
        ubo0.Model = glm::mat4(1.f);
        ubo0.View = m_camera->GetViewMatrix();
        ubo0.Projection = m_camera->GetProjMatrix();
        m_gbufferPass->GetShader()->SetUBO(0, &ubo0);

        GbufferShader::UniformBufferObject1 ubo1 = GbufferShader::GetFromScene(m_scene);
        m_gbufferPass->GetShader()->SetUBO(1, &ubo1);
    }

    //prepare probeTracingPass ubo
    {
        ProbeTracingShader::UniformBuffer0 ubo0{};
    
        //auto meshNames = m_scene->GetMeshNames();
        auto numPrims = m_scene->GetNumPrimInfos();
        auto drawIndexedIndirectCommands = m_scene->GetIndirectDrawCommands();
    
        for (auto i = 0; i < numPrims; i++) {
            //auto name = meshNames[m_scene->m_primInfos[i].meshId];
            auto mesh = m_scene->GetMesh(m_scene->m_primInfos[i].mesh_id);
            auto mat = m_scene->GetMaterial(m_scene->m_primInfos[i].material_id);
            auto indirectCommand = drawIndexedIndirectCommands[i];
            if (mat->diffuseTexture != "") {
                auto diffuseTexId = Singleton<TextureManager>::instance().GetTextureId(mat->diffuseTexture);
                ubo0.texBuffer[indirectCommand.firstInstance].diffuseTextureIdx = diffuseTexId;
            }
            else {
                ubo0.texBuffer[indirectCommand.firstInstance].diffuseTextureIdx = -1;
            }
    
            if (mat->metallicAndRoughnessTexture != "") {
                auto metallicAndRoughnessTexId = Singleton<TextureManager>::instance().GetTextureId(mat->metallicAndRoughnessTexture);
                ubo0.texBuffer[indirectCommand.firstInstance].metallicAndRoughnessTextureIdx = metallicAndRoughnessTexId;
            }
            else {
                ubo0.texBuffer[indirectCommand.firstInstance].metallicAndRoughnessTextureIdx = -1;
            }
        }
        m_probeTracingPass->GetShader()->SetUBO(0, &ubo0);
    
        ProbeTracingShader::UniformBuffer2 ubo2{};
        ubo2.lightNum = 1;
        ubo2.lights[0].direction = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetDirection();
        ubo2.lights[0].intensity = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetIntensity();
        ubo2.lights[0].color = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetColor();
        
        auto now = std::chrono::system_clock::now();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count();
        ubo2.t = (millis - m_start) / 1000.f;

        m_volume->SetRandomRotation();
        ubo2.probeRotateQuaternion = m_volume->GetQuaternion();
        
        m_probeTracingPass->GetShader()->SetUBO(2, &ubo2);
    }

    //prepare probeBlendRadiancePass ubo
    {
        ProbeBlendShader::UniformBuffer0 ubo0{};
        //ubo0.raysPerProbe = m_raysPerProbe;
        ubo0.sharpness = 1;
        m_probeBlendingRadiancePass->GetShader()->SetUBO(0, &ubo0);
    }

    //prepare probeBlendDepthPass ubo
    {
        ProbeBlendShader::UniformBuffer0 ubo0{};
        //ubo0.raysPerProbe = m_raysPerProbe;
        ubo0.sharpness = 50;
        m_probeBlendingDepthPass->GetShader()->SetUBO(0, &ubo0);
    }

    //prepare rtaoPass ubo
    {
        RTAOShader::UniformBuffer0 ubo0{};
        auto extent2D = m_gbufferPass->GetFrameBuffer(0).GetExtent2D();
        ubo0.gbufferWidth = extent2D.width;
        ubo0.gbufferHeight = extent2D.height;

        //spdlog::info("GetCameraMoved():{0}", int(GetCameraMoved()));
        ubo0.resetAccumulatedBuffer = GetCameraMoved() ? 1 : 0;
        
        m_rtaoPass->GetShader()->SetUBO(0, &ubo0);
    }

    //prepare lightingPass ubo
    {
        DDGILightingShader::UniformBuffer0 ubo0{};
        ubo0.lightNum = 1;
        ubo0.lights[0].direction = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetDirection();
        ubo0.lights[0].intensity = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetIntensity();
        ubo0.lights[0].color = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetColor();
        ubo0.cameraPos = m_camera->GetPosition();
        //ubo0.probePos0 = m_volume->GetProbePosition(0, 0, 0);
        //ubo0.probePos1 = m_volume->GetProbePosition(7, 7, 7);

        m_lightingPass->GetShader()->SetUBO(0, &ubo0);
    }

    //prepare visulizePass ubo
    {
        DDGIProbeVisulizeShader::UniformBuffer0 ubo0{};
        ubo0.vp.View = m_camera->GetViewMatrix();
        ubo0.vp.Projection = m_camera->GetProjMatrix();

        m_probeVisulizePass->GetShader()->SetUBO(0, &ubo0);
    }

    {
        CompositeShader::UniformBuffer0 ubo0{};
        ubo0.visulizeProbe = (int)m_visualizeProbes;
        ubo0.useAO = (int)m_addAO;

        m_compositePass->GetShader()->SetUBO(0, &ubo0);
    }
    
    graphicsList.Reset();
    graphicsList.Begin();
    
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(
        0, m_gbufferPass, m_currentFrame,
        m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), m_scene->GetIndirectBuffer(), m_scene->GetIndirectDrawCommands().size(),
        std::string("Generate Gbuffer"));
    if (m_sceneChange) {
        Singleton<MVulkanEngine>::instance().RecordCommandBuffer(
            0, m_probeTracingPass, m_currentFrame, 
            m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size(),
            std::string("Probe Tracing"));
        graphicsList.End();

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &graphicsList.GetBuffer();

        graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
        graphicsQueue.WaitForQueueComplete();

        changeTextureLayoutToRWTexture();

        computeList.Reset();
        computeList.Begin();

        //Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_probeBlendingRadiancePass, 512, 64, 1);
        auto probeDim = m_volume->GetProbeDim();
        //if(m_probeRelocationEnabled)
        //    Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_probeRelocationPass, probeDim.x * probeDim.y * probeDim.z, 1, 1);
        //if(m_probeClassfication)
        //    Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_probeClassficationPass, probeDim.x * probeDim.y * probeDim.z, 1, 1);
        //Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_probeBlendingDepthPass, 16 * probeDim.x * probeDim.y, 16 * probeDim.z, 1);
        //Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_probeBlendingRadiancePass, 8 * probeDim.x * probeDim.y, 8 * probeDim.z, 1);
        if (m_probeRelocationEnabled)
            Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_probeRelocationPass, (probeDim.x * probeDim.y * probeDim.z + 31) / 32, 1, 1,
                std::string("Probe Reloction"));
        if (m_probeClassfication)
            Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_probeClassficationPass, (probeDim.x * probeDim.y * probeDim.z+31) / 32, 1, 1,
                std::string("Probe Classfication"));
        Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_probeBlendingDepthPass, probeDim.x * probeDim.y, probeDim.z, 1,
            std::string("Probe Blend Depth"));
        Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_probeBlendingRadiancePass, probeDim.x * probeDim.y, probeDim.z, 1,
            std::string("Probe Blend Radiance"));

        computeList.End();

        //VkSubmitInfo submitInfo{};
        submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &computeList.GetBuffer();

        computeQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
        computeQueue.WaitForQueueComplete();

        changeRWTextureLayoutToTexture();

        graphicsList.Reset();
        graphicsList.Begin();
       // m_sceneChange = false;
    }
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(
        0, m_lightingPass, m_currentFrame, 
        m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size(),
        std::string("DDGI Lighting"));
    if (m_addAO) {
        Singleton<MVulkanEngine>::instance().RecordCommandBuffer(
            0, m_rtaoPass, m_currentFrame,
            m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size(),
            std::string("RTAO"));
    }
    if (m_visualizeProbes) {
        Singleton<MVulkanEngine>::instance().RecordCommandBuffer(
            0, m_probeVisulizePass, m_currentFrame, 
            m_sphere->GetIndirectVertexBuffer(), m_sphere->GetIndirectIndexBuffer(), m_sphere->GetIndirectBuffer(), m_sphere->GetIndirectDrawCommands().size(),
            std::string("Visulize Probe"));
    }
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(
        imageIndex, m_compositePass, m_currentFrame, 
        m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size(),
        std::string("Composite"));
    
    graphicsList.End();
    Singleton<MVulkanEngine>::instance().SubmitGraphicsCommands(imageIndex, m_currentFrame);
  

}

void DDGIApplication::RecreateSwapchainAndRenderPasses()
{
    if (Singleton<MVulkanEngine>::instance().RecreateSwapchain()) {
        Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_gbufferPass);

        createTextures();

        {
            //m_rtaoPass->GetRenderPassCreateInfo().depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();
            Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_rtaoPass);

            std::vector<std::vector<VkImageView>> rtaoViews(2);
            for (auto i = 0; i < 2; i++) {
                rtaoViews[i].resize(1);
                rtaoViews[i][0] = m_gbufferPass->GetFrameBuffer(0).GetImageView(i);
            }

            std::vector<VkSampler> samplers(1);
            samplers[0] = m_linearSamplerWithAnisotropy.GetSampler();

            std::vector<std::vector<VkImageView>> storageTextureViews(1);
            storageTextureViews[0].resize(1, m_acculatedAOTexture->GetImageView());

            auto tlas = m_rayTracing.GetTLAS();
            std::vector<VkAccelerationStructureKHR> accelerationStructures(1, tlas);

            m_rtaoPass->UpdateDescriptorSetWrite(rtaoViews, storageTextureViews, samplers, accelerationStructures);
        }

        {
            m_probeVisulizePass->GetRenderPassCreateInfo().depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();
            Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_probeVisulizePass);
        }

        {
            Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_lightingPass);

            std::vector<std::vector<VkImageView>> views(6);
            for (auto i = 0; i < 4; i++) {
                views[i].resize(1);
                views[i][0] = m_gbufferPass->GetFrameBuffer(0).GetImageView(i);
            }
            views[4] = std::vector<VkImageView>(1, m_volumeProbeDatasRadiance->GetImageView());
            views[5] = std::vector<VkImageView>(1, m_volumeProbeDatasDepth->GetImageView());

            std::vector<VkSampler> samplers(1);
            samplers[0] = m_linearSamplerWithoutAnisotropy.GetSampler();

            auto tlas = m_rayTracing.GetTLAS();
            std::vector<VkAccelerationStructureKHR> accelerationStructures(1, tlas);

            m_lightingPass->UpdateDescriptorSetWrite(views, samplers, accelerationStructures);
        }

        {
            //m_compositePass->GetRenderPassCreateInfo().depthView = m_compositePass->GetFrameBuffer(0).GetDepthImageView();
            Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_compositePass);

            std::vector<std::vector<VkImageView>> views(4);
            views[0].resize(1);
            views[0][0] = m_lightingPass->GetFrameBuffer(0).GetImageView(0);
            views[1].resize(1);
            views[1][0] = m_lightingPass->GetFrameBuffer(0).GetImageView(1);
            views[2].resize(1);
            views[2][0] = m_rtaoPass->GetFrameBuffer(0).GetImageView(0);
            views[3].resize(1);
            views[3][0] = m_probeVisulizePass->GetFrameBuffer(0).GetImageView(0);

            std::vector<VkSampler> samplers(1);
            samplers[0] = m_linearSamplerWithAnisotropy.GetSampler();

            std::vector<VkAccelerationStructureKHR> accelerationStructures(0);

            m_compositePass->UpdateDescriptorSetWrite(views, samplers, accelerationStructures);
        }
    }
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
    createCompositePass();
    
    return;
}

void DDGIApplication::PreComputes()
{

}

void DDGIApplication::Clean()
{
    m_gbufferPass->Clean();
    m_rtaoPass->Clean();
    m_probeTracingPass->Clean();
    m_lightingPass->Clean();
    m_probeVisulizePass->Clean();
    m_compositePass->Clean();
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
    m_sphere->GenerateIndirectDataAndBuffers(m_probeDim.x * m_probeDim.y * m_probeDim.z);
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

    bool translationLayout = false;
    auto extent2D = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();

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

    auto probeDim = m_volume->GetProbeDim();

    if (!m_volumeProbeDatasRadiance)
    {
        m_volumeProbeDatasRadiance = std::make_shared<MVulkanTexture>();
        translationLayout = true;

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

    if (!m_testTexture)
    {
        m_testTexture = std::make_shared<MVulkanTexture>();

        ImageCreateInfo imageInfo;
        ImageViewCreateInfo viewInfo;
        imageInfo.arrayLength = 1;
        imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.width = 16 * probeDim.z * probeDim.x * probeDim.y;
        imageInfo.height = 1;
        imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;

        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = imageInfo.format;
        viewInfo.flag = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.baseMipLevel = 0;
        viewInfo.levelCount = 1;
        viewInfo.baseArrayLayer = 0;
        viewInfo.layerCount = 1;

        Singleton<MVulkanEngine>::instance().CreateImage(m_testTexture, imageInfo, viewInfo, VK_IMAGE_LAYOUT_GENERAL);
    }

    if(translationLayout)
        changeRWTextureLayoutToTexture();
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
        auto probeBuffer = m_volume->GetProbeBuffer();

        auto numVertices = m_scene->GetNumVertices();
        auto numIndices = m_scene->GetNumTriangles() * 3;
        auto numMeshes = m_scene->GetNumMeshes();

        BufferCreateInfo vertexBufferCreateInfo{};
        BufferCreateInfo indexBufferCreateInfo{};
        BufferCreateInfo normalBufferCreateInfo{};
        BufferCreateInfo uvBufferCreateInfo{};
        BufferCreateInfo geometryBufferCreateInfo{};

        vertexBufferCreateInfo.size = numVertices * sizeof(glm::vec3);
        indexBufferCreateInfo.size = numIndices * sizeof(int);
        normalBufferCreateInfo.size = vertexBufferCreateInfo.size;
        uvBufferCreateInfo.size = numVertices * sizeof(glm::vec2);
        geometryBufferCreateInfo.size = numMeshes * sizeof(GeometryInfo);
        //storageBufferSizes[5] = probeBuffer.GetSize();

        //auto shader = std::static_pointer_cast<ProbeTracingShader>(probeTracingShader);
        //auto meshNames = m_scene->GetMeshNames();
        auto primitiveNum = m_scene->GetNumPrimInfos();

        VertexBuffer   vertexBuffer;
        IndexBuffer    indexBuffer;
        NormalBuffer   normalBuffer;
        UVBuffer       uvBuffer;
        InstanceOffset instanceOffsetBuffer;

        vertexBuffer.position.resize(numVertices);
        indexBuffer.index.resize(numIndices);
        normalBuffer.normal.resize(numVertices);
        uvBuffer.uv.resize(numVertices);
        instanceOffsetBuffer.geometryInfos.resize(numMeshes);
        probeBuffer = probeBuffer;

        int vertexBufferIndex = 0;
        int indexBufferIndex = 0;
        int instanceBufferIndex = 0;

        for (auto j = 0; j < primitiveNum;j++) {
            auto mesh = m_scene->GetMesh(m_scene->m_primInfos[j].mesh_id);
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

            instanceOffsetBuffer.geometryInfos[instanceBufferIndex] =
                GeometryInfo{
                    .vertexOffset = vertexBufferIndex * 3,
                    .indexOffset = indexBufferIndex,
                    .uvOffset = vertexBufferIndex * 2,
                    .normalOffset = vertexBufferIndex * 3,
                    .materialIdx = int(m_scene->m_primInfos[j].material_id)
            };

            vertexBufferIndex += meshVertexNum;
            indexBufferIndex += meshIndexNum;
            instanceBufferIndex += 1;
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

    m_volume = std::make_shared<DDGIVolume>(sceneAABB.pMin + 0.3f * scale / (glm::vec3)m_probeDim, sceneAABB.pMax - glm::vec3(0.1f), probeDim);


    {
        //UniformBuffer1 ubo1{};
        glm::ivec3 probeDim = m_volume->GetProbeDim();

        m_uniformBuffer1.probeDim = m_volume->GetProbeDim();
        m_uniformBuffer1.raysPerProbe = m_raysPerProbe;

        m_uniformBuffer1.probePos0 = m_volume->GetProbePosition(0, 0, 0);
        m_uniformBuffer1.probePos1 = m_volume->GetProbePosition(probeDim.x-1, probeDim.y-1, probeDim.z-1);
    

        m_uniformBuffer1.minFrontFaceDistance = 0.4f;
        //m_uniformBuffer1.farthestFrontfaceDistance = 0.8f;
        m_uniformBuffer1.probeRelocationEnbled = (int)m_probeRelocationEnabled;
    }
}

void DDGIApplication::createGbufferPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

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
        std::vector<VkSampler> samplers(1, m_linearSamplerWithAnisotropy.GetSampler());

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_gbufferPass, gbufferShader, bufferTextureViews, samplers);
    }
}

void DDGIApplication::createProbeTracingPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        std::vector<VkFormat> probeTracingPassFormats;
        probeTracingPassFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        probeTracingPassFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        probeTracingPassFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        probeTracingPassFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        //probeTracingPassFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        //probeTracingPassFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        //probeTracingPassFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);

        VkExtent2D extent2D = { m_raysPerProbe, m_volume->GetNumProbes()};

        RenderPassCreateInfo info{};
        info.depthFormat = device.FindDepthFormat();
        info.frambufferCount = 1;
        info.extent = extent2D;
        info.useAttachmentResolve = false;
        info.useSwapchainImages = false;
        info.imageAttachmentFormats = probeTracingPassFormats;
        info.colorAttachmentResolvedViews = nullptr;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalDepthLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.pipelineCreateInfo.depthTestEnable = VK_TRUE;
        info.pipelineCreateInfo.depthWriteEnable = VK_TRUE;

        m_probeTracingPass = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<ShaderModule> probeTracingShader = std::make_shared<ProbeTracingShader>();
        std::vector<std::vector<VkImageView>> bufferTextureViews(3);
        auto wholeTextures = Singleton<TextureManager>::instance().GenerateTextureVector();
        auto wholeTextureSize = wholeTextures.size();

        if (wholeTextureSize == 0) {
            bufferTextureViews[0].resize(1);
            bufferTextureViews[0][0] = Singleton<MVulkanEngine>::instance().GetPlaceHolderTexture().GetImageView();
        }
        else {
            bufferTextureViews[0].resize(wholeTextureSize);
            for (auto j = 0; j < wholeTextureSize; j++) {
                bufferTextureViews[0][j] = wholeTextures[j]->GetImageView();
            }
        }

        bufferTextureViews[1] = std::vector<VkImageView>(1, m_volumeProbeDatasRadiance->GetImageView());
        bufferTextureViews[2] = std::vector<VkImageView>(1, m_volumeProbeDatasDepth->GetImageView());


        std::vector<std::vector<VkImageView>> storageTextureViews(0);
        //storageTextureViews[0].resize(1, m_volumeProbeDatasRadiance->GetImageView());
        //storageTextureViews[1].resize(1, m_volumeProbeDatasDepth->GetImageView());

        std::vector<VkSampler> samplers(1);
        samplers[0] = m_linearSamplerWithoutAnisotropy.GetSampler();

        auto tlas = m_rayTracing.GetTLAS();
        std::vector<VkAccelerationStructureKHR> accelerationStructures(1, tlas);

        std::vector<StorageBuffer> storageBuffers(0);
        storageBuffers.push_back(*m_tlasVertexBuffer);
        storageBuffers.push_back(*m_tlasIndexBuffer);
        storageBuffers.push_back(*m_tlasNormalBuffer);
        storageBuffers.push_back(*m_tlasUVBuffer);
        storageBuffers.push_back(*m_geometryInfo);
        storageBuffers.push_back(*m_probesDataBuffer);

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_probeTracingPass, probeTracingShader,
            storageBuffers, bufferTextureViews, storageTextureViews, samplers, accelerationStructures);
    
        {
            probeTracingShader->SetUBO(1, &m_uniformBuffer1);
        }
    }
}

void DDGIApplication::createLightingPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        std::vector<VkFormat> lightingPassFormats;
        lightingPassFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        lightingPassFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);

        RenderPassCreateInfo info{};
        info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
        info.depthFormat = device.FindDepthFormat();
        info.frambufferCount = 1;
        info.useSwapchainImages = false;
        info.imageAttachmentFormats = lightingPassFormats;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalDepthLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        //info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
        info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
        //info.depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();

        m_lightingPass = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<DDGILightingShader> lightingShader = std::make_shared<DDGILightingShader>();

        auto probeBuffer = m_volume->GetProbeBuffer();
        std::vector<StorageBuffer> storageBuffers(1);
        storageBuffers[0] = *m_probesDataBuffer;
        //lightingShader->probeBuffer = probeBuffer;

        std::vector<std::vector<VkImageView>> gbufferViews(6);
        for (auto i = 0; i < 4; i++) {
            gbufferViews[i].resize(1);
            gbufferViews[i][0] = m_gbufferPass->GetFrameBuffer(0).GetImageView(i);
        }
        gbufferViews[4] = std::vector<VkImageView>(1, m_volumeProbeDatasRadiance->GetImageView());
        gbufferViews[5] = std::vector<VkImageView>(1, m_volumeProbeDatasDepth->GetImageView());

        std::vector<std::vector<VkImageView>> storageTextureViews(0);

        std::vector<VkSampler> samplers(1);
        samplers[0] = m_linearSamplerWithoutAnisotropy.GetSampler();

        auto tlas = m_rayTracing.GetTLAS();
        std::vector<VkAccelerationStructureKHR> accelerationStructures(1, tlas);

        //std::vector<uint32_t> storageBufferSizes(0);
        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_lightingPass, lightingShader,
            storageBuffers, gbufferViews, storageTextureViews, samplers, accelerationStructures);
    
        {
            lightingShader->SetUBO(1, &m_uniformBuffer1);
        }
    
    }
}

void DDGIApplication::createRTAOPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        std::vector<VkFormat> rtaoPassFormats;
        rtaoPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());

        RenderPassCreateInfo info{};
        info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
        info.depthFormat = device.FindDepthFormat();
        info.frambufferCount = 1;
        info.useSwapchainImages = false;
        info.imageAttachmentFormats = rtaoPassFormats;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalDepthLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        //info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
        info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
        //info.depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();

        m_rtaoPass = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<ShaderModule> rtaoShader = std::make_shared<RTAOShader>();
        std::vector<std::vector<VkImageView>> rtaoViews(4);
        for (auto i = 0; i < 2; i++) {
            rtaoViews[i].resize(1);
            rtaoViews[i][0] = m_gbufferPass->GetFrameBuffer(0).GetImageView(i);
        }

        std::vector<std::vector<VkImageView>> storageTextureViews(1);
        storageTextureViews[0].resize(1, m_acculatedAOTexture->GetImageView());

        std::vector<VkSampler> samplers(1);
        samplers[0] = m_linearSamplerWithAnisotropy.GetSampler();

        auto tlas = m_rayTracing.GetTLAS();
        std::vector<VkAccelerationStructureKHR> accelerationStructures(1, tlas);

        std::vector<uint32_t> storageBufferSizes(0);
        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_rtaoPass, rtaoShader,
            storageBufferSizes, rtaoViews, storageTextureViews, samplers, accelerationStructures);
    }
}

void DDGIApplication::createProbeBlendingRadiancePass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    std::shared_ptr<ProbeBlendShader> probeBlendingRadianceShader = std::make_shared<ProbeBlendShader>("main_radiance");
    m_probeBlendingRadiancePass = std::make_shared<ComputePass>(device);

    auto probeBuffer = m_volume->GetProbeBuffer();
    std::vector<StorageBuffer> storageBuffers(1);
    storageBuffers[0] = *m_probesDataBuffer;
    //probeBlendingRadianceShader->probeBuffer = probeBuffer;

    //std::vector<uint32_t> storageBufferSizes(0);

    std::vector<VkSampler> samplers(1, m_linearSamplerWithAnisotropy.GetSampler());

    std::vector<std::vector<VkImageView>> seperateImageViews(2);
    seperateImageViews[0].resize(1);
    seperateImageViews[0][0] = m_probeTracingPass->GetFrameBuffer(0).GetImageView(0);
    seperateImageViews[1].resize(1);
    seperateImageViews[1][0] = m_probeTracingPass->GetFrameBuffer(0).GetImageView(3);

    std::vector<std::vector<VkImageView>> storageImageViews(2);
    storageImageViews[0].resize(1);
    storageImageViews[0][0] = m_volumeProbeDatasRadiance->GetImageView();
    storageImageViews[1].resize(1);
    storageImageViews[1][0] = m_volumeProbeDatasDepth->GetImageView();

    Singleton<MVulkanEngine>::instance().CreateComputePass(m_probeBlendingRadiancePass, probeBlendingRadianceShader,
        storageBuffers, seperateImageViews, storageImageViews, samplers);

    {
        probeBlendingRadianceShader->SetUBO(1, &m_uniformBuffer1);
    }
}

void DDGIApplication::createProbeBlendingDepthPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    std::shared_ptr<ProbeBlendShader> probeBlendingDepthShader = std::make_shared<ProbeBlendShader>("main_depth");
    m_probeBlendingDepthPass = std::make_shared<ComputePass>(device);

    auto probeBuffer = m_volume->GetProbeBuffer();
    std::vector<StorageBuffer> storageBuffers(1);
    storageBuffers[0] = *m_probesDataBuffer;
    //probeBlendingRadianceShader->probeBuffer = probeBuffer;

    //std::vector<uint32_t> storageBufferSizes(0);

    std::vector<VkSampler> samplers(1, m_linearSamplerWithAnisotropy.GetSampler());

    std::vector<std::vector<VkImageView>> seperateImageViews(2);
    seperateImageViews[0].resize(1);
    seperateImageViews[0][0] = m_probeTracingPass->GetFrameBuffer(0).GetImageView(0);
    seperateImageViews[1].resize(1);
    seperateImageViews[1][0] = m_probeTracingPass->GetFrameBuffer(0).GetImageView(3);

    std::vector<std::vector<VkImageView>> storageImageViews(2);
    storageImageViews[0].resize(1);
    storageImageViews[0][0] = m_volumeProbeDatasRadiance->GetImageView();
    storageImageViews[1].resize(1);
    storageImageViews[1][0] = m_volumeProbeDatasDepth->GetImageView();

    Singleton<MVulkanEngine>::instance().CreateComputePass(m_probeBlendingDepthPass, probeBlendingDepthShader,
        storageBuffers, seperateImageViews, storageImageViews, samplers);

    {
        probeBlendingDepthShader->SetUBO(1, &m_uniformBuffer1);
    }
}

void DDGIApplication::createProbeClassficationPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    std::shared_ptr<ProbeClassficationShader> probeclassficationShader = std::make_shared<ProbeClassficationShader>();
    m_probeClassficationPass = std::make_shared<ComputePass>(device);

    auto probeBuffer = m_volume->GetProbeBuffer();
    std::vector<StorageBuffer> storageBuffers(1);
    storageBuffers[0] = *m_probesDataBuffer;
    //probeclassficationShader->probeBuffer = probeBuffer;

    //std::vector<uint32_t> storageBufferSizes(0);

    std::vector<VkSampler> samplers(1, m_linearSamplerWithAnisotropy.GetSampler());

    std::vector<std::vector<VkImageView>> seperateImageViews(2);
    seperateImageViews[0].resize(1);
    seperateImageViews[0][0] = m_probeTracingPass->GetFrameBuffer(0).GetImageView(0);
    seperateImageViews[1].resize(1);
    seperateImageViews[1][0] = m_probeTracingPass->GetFrameBuffer(0).GetImageView(3);

    std::vector<std::vector<VkImageView>> storageImageViews(1);
    storageImageViews[0].resize(1);
    storageImageViews[0][0] = m_testTexture->GetImageView();

    Singleton<MVulkanEngine>::instance().CreateComputePass(m_probeClassficationPass, probeclassficationShader,
        storageBuffers, seperateImageViews, storageImageViews, samplers);

    {
        ProbeClassficationShader::UniformBuffer0 ubo0;
        ubo0.maxRayDistance = 9999.f;

        probeclassficationShader->SetUBO(0, &ubo0);
        probeclassficationShader->SetUBO(1, &m_uniformBuffer1);
    }
}


void DDGIApplication::createProbeRelocationPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    std::shared_ptr<ProbeRelocationShader> probeRelocationShader = std::make_shared<ProbeRelocationShader>();
    m_probeRelocationPass = std::make_shared<ComputePass>(device);

    auto probeBuffer = m_volume->GetProbeBuffer();
    std::vector<StorageBuffer> storageBuffers(2);
    storageBuffers[0] = *m_probesDataBuffer;
    storageBuffers[1] = *m_probesModelBuffer;
    //probeclassficationShader->probeBuffer = probeBuffer;

    //std::vector<uint32_t> storageBufferSizes(0);

    std::vector<VkSampler> samplers(1, m_linearSamplerWithAnisotropy.GetSampler());

    std::vector<std::vector<VkImageView>> seperateImageViews(2);
    seperateImageViews[0].resize(1);
    seperateImageViews[0][0] = m_probeTracingPass->GetFrameBuffer(0).GetImageView(0);
    seperateImageViews[1].resize(1);
    seperateImageViews[1][0] = m_probeTracingPass->GetFrameBuffer(0).GetImageView(3);

    std::vector<std::vector<VkImageView>> storageImageViews(0);
    //storageImageViews[0].resize(1);
    //storageImageViews[0][0] = m_testTexture->GetImageView();

    Singleton<MVulkanEngine>::instance().CreateComputePass(m_probeRelocationPass, probeRelocationShader,
        storageBuffers, seperateImageViews, storageImageViews, samplers);

    {
        probeRelocationShader->SetUBO(0, &m_uniformBuffer1);
    }
}

void DDGIApplication::createProbeVisulizePass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        std::vector<VkFormat> ddgiProbeVisulizePassFormats;
        ddgiProbeVisulizePassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());

        RenderPassCreateInfo info{};
        info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
        info.depthFormat = device.FindDepthFormat();
        info.frambufferCount = 1;
        info.useSwapchainImages = false;
        info.imageAttachmentFormats = ddgiProbeVisulizePassFormats;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.initialDepthLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.finalDepthLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        info.pipelineCreateInfo.depthTestEnable = VK_TRUE;
        info.pipelineCreateInfo.depthWriteEnable = VK_TRUE;
        info.depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();

        m_probeVisulizePass = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<DDGIProbeVisulizeShader> visulizeShader = std::make_shared<DDGIProbeVisulizeShader>();
        
        auto modelBuffer = m_volume->GetModelBuffer();
        auto probeBuffer = m_volume->GetProbeBuffer();
        std::vector<StorageBuffer> storageBuffers(2);
        storageBuffers[0] = *m_probesModelBuffer;
        storageBuffers[1] = *m_probesDataBuffer;
        
        std::vector<std::vector<VkImageView>> views(1);
        views[0] = std::vector<VkImageView>(1, m_volumeProbeDatasRadiance->GetImageView());

        std::vector<std::vector<VkImageView>> storageTextureViews(0);

        std::vector<VkSampler> samplers(1);
        samplers[0] = m_linearSamplerWithAnisotropy.GetSampler();

        std::vector<VkAccelerationStructureKHR> accelerationStructures(0);

        //std::vector<uint32_t> storageBufferSizes(0);
        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_probeVisulizePass, visulizeShader,
            storageBuffers, views, storageTextureViews, samplers, accelerationStructures);


        //visulizeShader->SetUBO(0, &m_ProbeVisulizeShaderUniformBuffer0);
        visulizeShader->SetUBO(1, &m_uniformBuffer1);
    }

    if (m_visualizeProbes == 0) {
        transitionProbeVisulizeTextureLayoutToShaderRead();
    }
}

void DDGIApplication::createCompositePass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        std::vector<VkFormat> compositePassFormats;
        compositePassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());

        RenderPassCreateInfo info{};
        info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
        info.depthFormat = device.FindDepthFormat();
        info.frambufferCount = Singleton<MVulkanEngine>::instance().GetSwapchainImageCount();
        info.useSwapchainImages = true;
        info.imageAttachmentFormats = compositePassFormats;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        info.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        //info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
        info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
        //info.depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();

        m_compositePass = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<ShaderModule> compositeShader = std::make_shared<CompositeShader>();
        std::vector<std::vector<VkImageView>> views(4);
        views[0].resize(1);
        views[0][0] = m_lightingPass->GetFrameBuffer(0).GetImageView(0);
        views[1].resize(1);
        views[1][0] = m_lightingPass->GetFrameBuffer(0).GetImageView(1);
        views[2].resize(1);
        views[2][0] = m_rtaoPass->GetFrameBuffer(0).GetImageView(0);
        views[3].resize(1);
        views[3][0] = m_probeVisulizePass->GetFrameBuffer(0).GetImageView(0);

        std::vector<std::vector<VkImageView>> storageTextureViews(0);

        std::vector<VkSampler> samplers(1);
        samplers[0] = m_linearSamplerWithAnisotropy.GetSampler();

        std::vector<VkAccelerationStructureKHR> accelerationStructures(0);

        std::vector<uint32_t> storageBufferSizes(0);
        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_compositePass, compositeShader,
            storageBufferSizes, views, storageTextureViews, samplers, accelerationStructures);
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

        //barrier.image = m_volumeProbeDatasDepth->GetImage();
        //barriers.push_back(barrier);
    }
    Singleton<MVulkanEngine>::instance().TransitionImageLayout(barriers, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}
