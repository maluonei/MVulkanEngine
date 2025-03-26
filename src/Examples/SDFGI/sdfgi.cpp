#include "sdfgi.hpp"

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

#include "Shaders/voxelShader.hpp"
#include "sdfgiShader.hpp"


//#define raysPerProbe


void SDFGIApplication::SetUp()
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
    //createAS();

    auto now = std::chrono::system_clock::now();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    m_start = millis;
}

void SDFGIApplication::ComputeAndDraw(uint32_t imageIndex)
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

        SDFProbeTracingShader::UniformBuffer2 ubo2{};
        ubo2.lightNum = 1;
        ubo2.lights[0].direction = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetDirection();
        ubo2.lights[0].intensity = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetIntensity();
        ubo2.lights[0].color = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetColor();
        //ubo2.lights[0].shadowMapIndex = 0;
        ubo2.lights[0].shadowViewProjMat = m_lightCamera->GetOrthoMatrix() * m_lightCamera->GetViewMatrix();

        auto now = std::chrono::system_clock::now();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count();
        ubo2.t = (millis - m_start) / 1000.f;

        m_volume->SetRandomRotation();
        ubo2.probeRotateQuaternion = m_volume->GetQuaternion();

        m_probeTracingPass->GetShader()->SetUBO(1, &ubo2);
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

    //prepare lightingPass ubo
    {
        SDFGILightingShader::UniformBuffer0 ubo0{};
        ubo0.lightNum = 1;
        ubo0.lights[0].direction = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetDirection();
        ubo0.lights[0].intensity = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetIntensity();
        ubo0.lights[0].color = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetColor();
        ubo0.lights[0].shadowViewProjMat = m_lightCamera->GetOrthoMatrix() * m_lightCamera->GetViewMatrix();
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

    //prepare compositePass ubo
    {
        SDFGICompositeShader::UniformBuffer0 ubo0{};
        ubo0.visulizeProbe = (int)m_visualizeProbes;

        m_compositePass->GetShader()->SetUBO(0, &ubo0);
    }

    //prepare voxelPass ubo
    {
        setOrthCamera(2);

        VoxelShader::UniformBuffer0 ubo0{};
        ubo0.Model = glm::mat4(1.f);
        m_voxelPass->GetShader()->SetUBO(0, &ubo0);

        auto aabb = GetSameSizeAABB(m_scene);
        VoxelShader::UniformBuffer1 ubo1{};
        ubo1.Projection = m_orthCamera->GetOrthoMatrix();
        ubo1.volumeResolution = m_voxelResolution;
        m_voxelPass->GetShader()->SetUBO(1, &ubo1);

        VoxelShader::UniformBufferObject2 ubo2{};
        auto numPrims = m_scene->GetNumPrimInfos();
        auto drawIndexedIndirectCommands = m_scene->GetIndirectDrawCommands();

        for (auto i = 0; i < numPrims; i++) {
            //auto name = meshNames[m_scene->m_primInfos[i].meshId];
            auto mesh = m_scene->GetMesh(m_scene->m_primInfos[i].mesh_id);
            auto mat = m_scene->GetMaterial(m_scene->m_primInfos[i].material_id);
            auto indirectCommand = drawIndexedIndirectCommands[i];
            if (mat->diffuseTexture != "") {
                auto diffuseTexId = Singleton<TextureManager>::instance().GetTextureId(mat->diffuseTexture);
                ubo2.texBuffer[indirectCommand.firstInstance].diffuseTextureIdx = diffuseTexId;
            }
            else {
                ubo2.texBuffer[indirectCommand.firstInstance].diffuseTextureIdx = -1;
            }

            if (mat->metallicAndRoughnessTexture != "") {
                auto metallicAndRoughnessTexId = Singleton<TextureManager>::instance().GetTextureId(mat->metallicAndRoughnessTexture);
                ubo2.texBuffer[indirectCommand.firstInstance].metallicAndRoughnessTexIdx = metallicAndRoughnessTexId;
            }
            else {
                ubo2.texBuffer[indirectCommand.firstInstance].metallicAndRoughnessTexIdx = -1;
            }
        }
        m_voxelPass->GetShader()->SetUBO(2, &ubo2);
    }

    //prepare shadowPass ubo
    {
        ShadowShader::ShadowUniformBuffer ubo0{};
        ubo0.shadowMVP = m_lightCamera->GetOrthoMatrix() * m_lightCamera->GetViewMatrix();
        m_shadowPass->GetShader()->SetUBO(0, &ubo0);
    }

    //clear Texture
    {
        computeList.Reset();
        computeList.Begin();

        Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(
            m_clearTexturesPass, m_voxelResolution.x / 8, m_voxelResolution.y / 8, m_voxelResolution.z / 8, "Clear Textures");

        computeList.End();

        VkSubmitInfo submitInfo{};
        submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &computeList.GetBuffer();

        computeQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
        computeQueue.WaitForQueueComplete();
    }

    //generate voxel
    {
        graphicsList.Reset();
        graphicsList.Begin();

        Singleton<MVulkanEngine>::instance().RecordCommandBuffer(
            0, m_voxelPass, m_currentFrame,  
            m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), m_scene->GetIndirectBuffer(), m_scene->GetIndirectDrawCommands().size(),
            std::string("Generate Voxel Texture"));

        graphicsList.End();

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &graphicsList.GetBuffer();

        graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
        graphicsQueue.WaitForQueueComplete();
    }

    //generate sdf
    {
        auto shader = m_JFAPass->GetShader();
        uint32_t stepSizeInit = m_voxelResolution.x / 2;
        uint32_t dispatchSize = m_voxelResolution.x / 8;

        auto aabb = GetSameSizeAABB(m_scene);

        JFAShader::UniformBuffer0 ubo0;
        ubo0.aabbMin = aabb.pMin;
        ubo0.aabbMax = aabb.pMax;
        ubo0.voxelResolusion = m_voxelResolution;

        std::vector<StorageBuffer> storageBuffers(0);
        std::vector<VkSampler> samplers(0);

        std::vector<std::vector<VkImageView>> seperateImageViews(0);
        bool swap = true;

        std::vector<std::vector<VkImageView>> storageImageViews(3);
        storageImageViews[0].resize(1);
        storageImageViews[0][0] = m_JFATexture0->GetImageView();
        storageImageViews[1].resize(1);
        storageImageViews[1][0] = m_JFATexture1->GetImageView();
        storageImageViews[2].resize(1);
        storageImageViews[2][0] = m_SDFTexture->GetImageView();

        for (auto i = stepSizeInit; i >= 1; i /= 2) {
            {
                ubo0.stepSize = i;
                shader->SetUBO(0, &ubo0);

                if (swap) {
                    storageImageViews[0][0] = m_JFATexture0->GetImageView();
                    storageImageViews[1][0] = m_JFATexture1->GetImageView();
                }
                else {
                    storageImageViews[1][0] = m_JFATexture0->GetImageView();
                    storageImageViews[0][0] = m_JFATexture1->GetImageView();
                }
                swap = !swap;
                //spdlog::info("swap:{0}", (int)swap);

                m_JFAPass->UpdateDescriptorSetWrite(
                    seperateImageViews, storageImageViews, samplers
                );

                computeList.Reset();
                computeList.Begin();

                Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(
                    m_JFAPass, dispatchSize, dispatchSize, dispatchSize, "JFA");

                computeList.End();

                //VkSubmitInfo submitInfo{};
                VkSubmitInfo submitInfo{};
                submitInfo = {};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &computeList.GetBuffer();

                computeQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
                computeQueue.WaitForQueueComplete();
            }

        }
    }

    graphicsList.Reset();
    graphicsList.Begin();

    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(
        0, m_shadowPass, m_currentFrame,
        m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), m_scene->GetIndirectBuffer(), m_scene->GetIndirectDrawCommands().size(),
        std::string("Generate ShadowMap"));

    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(
        0, m_gbufferPass, m_currentFrame, 
        m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), m_scene->GetIndirectBuffer(), m_scene->GetIndirectDrawCommands().size(),
        std::string("Generate GBuffer"));

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
        if (m_probeRelocationEnabled)
            Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_probeRelocationPass, (probeDim.x * probeDim.y * probeDim.z + 31) / 32, 1, 1, "Probe Relocation");
        if (m_probeClassfication)
            Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_probeClassficationPass, (probeDim.x * probeDim.y * probeDim.z + 31) / 32, 1, 1, "Probe Classfication");
        Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_probeBlendingDepthPass, probeDim.x * probeDim.y, probeDim.z, 1, "Probe Blending Depth");
        Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_probeBlendingRadiancePass, probeDim.x * probeDim.y, probeDim.z, 1, "Probe Blending Radiance");

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
        std::string("SDFGI Lighting Pass"));
    //Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_rtaoPass, m_currentFrame, m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size());
    if (m_visualizeProbes) {
        Singleton<MVulkanEngine>::instance().RecordCommandBuffer(
            0, m_probeVisulizePass, m_currentFrame, 
            m_sphere->GetIndirectVertexBuffer(), m_sphere->GetIndirectIndexBuffer(), m_sphere->GetIndirectBuffer(), m_sphere->GetIndirectDrawCommands().size(),
            std::string("Probe Visulize Pass"));
    }
    else {

    }
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(
        imageIndex, m_compositePass, m_currentFrame, 
        m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size(),
        std::string("Composite Pass"));

    graphicsList.End();
    Singleton<MVulkanEngine>::instance().SubmitGraphicsCommands(imageIndex, m_currentFrame);
}

void SDFGIApplication::RecreateSwapchainAndRenderPasses()
{
    if (Singleton<MVulkanEngine>::instance().RecreateSwapchain()) {
        Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_gbufferPass);

        createTextures();

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

            //auto tlas = m_rayTracing.GetTLAS();
            //std::vector<VkAccelerationStructureKHR> accelerationStructures(1, tlas);
            std::vector<VkAccelerationStructureKHR> accelerationStructures(0);

            m_lightingPass->UpdateDescriptorSetWrite(views, samplers, accelerationStructures);
        }

        {
            //m_compositePass->GetRenderPassCreateInfo().depthView = m_compositePass->GetFrameBuffer(0).GetDepthImageView();
            Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_compositePass);

            std::vector<std::vector<VkImageView>> views(3);
            views[0].resize(1);
            views[0][0] = m_lightingPass->GetFrameBuffer(0).GetImageView(0);
            views[1].resize(1);
            views[1][0] = m_lightingPass->GetFrameBuffer(0).GetImageView(1);
            //views[2].resize(1);
            //views[2][0] = m_rtaoPass->GetFrameBuffer(0).GetImageView(0);
            views[2].resize(1);
            views[2][0] = m_probeVisulizePass->GetFrameBuffer(0).GetImageView(0);

            std::vector<VkSampler> samplers(1);
            samplers[0] = m_linearSamplerWithAnisotropy.GetSampler();

            std::vector<VkAccelerationStructureKHR> accelerationStructures(0);

            m_compositePass->UpdateDescriptorSetWrite(views, samplers, accelerationStructures);
        }
    }
}

void SDFGIApplication::CreateRenderPass()
{
    createShadowMapPass();
    createVoxelPass();
    createJFAPass();
    createClearTexturesPass();

    createGbufferPass();
    createProbeTracingPass();
    createProbeBlendingRadiancePass();
    createProbeBlendingDepthPass();
    createProbeClassficationPass();
    createProbeRelocationPass();
    createLightingPass();
    //createRTAOPass();
    createProbeVisulizePass();
    createCompositePass();

    return;
}

void SDFGIApplication::PreComputes()
{

}

void SDFGIApplication::Clean()
{
    m_gbufferPass->Clean();
    //m_rtaoPass->Clean();
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

    //m_rayTracing.Clean();

    m_sphere->Clean();
    m_squad->Clean();
    m_scene->Clean();

    MRenderApplication::Clean();
}

void SDFGIApplication::loadScene()
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

void SDFGIApplication::createLight()
{
    m_directionalLight = m_jsonLoader->GetLights();
}

void SDFGIApplication::createCamera()
{
    m_camera = m_jsonLoader->GetCamera();
    Singleton<MVulkanEngine>::instance().SetCamera(m_camera);
}

void SDFGIApplication::createSamplers()
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

void SDFGIApplication::createTextures()
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

    {
        m_voxelTexture = std::make_shared<MVulkanTexture>();

        ImageCreateInfo imageInfo;
        ImageViewCreateInfo viewInfo;
        imageInfo.arrayLength = 1;
        imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.type = VK_IMAGE_TYPE_3D;
        imageInfo.width = m_voxelResolution.x;
        imageInfo.height = m_voxelResolution.y;
        imageInfo.depth = m_voxelResolution.z;
        imageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;

        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
        viewInfo.format = imageInfo.format;
        viewInfo.flag = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.baseMipLevel = 0;
        viewInfo.levelCount = 1;
        viewInfo.baseArrayLayer = 0;
        viewInfo.layerCount = 1;

        Singleton<MVulkanEngine>::instance().CreateImage(m_voxelTexture, imageInfo, viewInfo, VK_IMAGE_LAYOUT_GENERAL);
    }

    {
        m_SDFTexture = std::make_shared<MVulkanTexture>();

        ImageCreateInfo imageInfo;
        ImageViewCreateInfo viewInfo;
        imageInfo.arrayLength = 1;
        imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.type = VK_IMAGE_TYPE_3D;
        imageInfo.width = m_voxelResolution.x;
        imageInfo.height = m_voxelResolution.y;
        imageInfo.depth = m_voxelResolution.z;
        imageInfo.format = VK_FORMAT_R16_SFLOAT;

        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
        viewInfo.format = imageInfo.format;
        viewInfo.flag = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.baseMipLevel = 0;
        viewInfo.levelCount = 1;
        viewInfo.baseArrayLayer = 0;
        viewInfo.layerCount = 1;

        Singleton<MVulkanEngine>::instance().CreateImage(m_SDFTexture, imageInfo, viewInfo, VK_IMAGE_LAYOUT_GENERAL);
    }

    {
        m_SDFAlbedoTexture = std::make_shared<MVulkanTexture>();
        //m_SDFNormalTexture = std::make_shared<MVulkanTexture>();

        ImageCreateInfo imageInfo;
        ImageViewCreateInfo viewInfo;
        imageInfo.arrayLength = 1;
        imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.type = VK_IMAGE_TYPE_3D;
        imageInfo.width = m_voxelResolution.x;
        imageInfo.height = m_voxelResolution.y;
        imageInfo.depth = m_voxelResolution.z;
        imageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;

        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
        viewInfo.format = imageInfo.format;
        viewInfo.flag = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.baseMipLevel = 0;
        viewInfo.levelCount = 1;
        viewInfo.baseArrayLayer = 0;
        viewInfo.layerCount = 1;

        Singleton<MVulkanEngine>::instance().CreateImage(m_SDFAlbedoTexture, imageInfo, viewInfo, VK_IMAGE_LAYOUT_GENERAL);
        //Singleton<MVulkanEngine>::instance().CreateImage(m_SDFNormalTexture, imageInfo, viewInfo, VK_IMAGE_LAYOUT_GENERAL);
    }

    {
        m_JFATexture0 = std::make_shared<MVulkanTexture>();
        m_JFATexture1 = std::make_shared<MVulkanTexture>();

        ImageCreateInfo imageInfo;
        ImageViewCreateInfo viewInfo;
        imageInfo.arrayLength = 1;
        imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.type = VK_IMAGE_TYPE_3D;
        imageInfo.width = m_voxelResolution.x;
        imageInfo.height = m_voxelResolution.y;
        imageInfo.depth = m_voxelResolution.z;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UINT;

        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
        viewInfo.format = imageInfo.format;
        viewInfo.flag = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.baseMipLevel = 0;
        viewInfo.levelCount = 1;
        viewInfo.baseArrayLayer = 0;
        viewInfo.layerCount = 1;

        Singleton<MVulkanEngine>::instance().CreateImage(m_JFATexture0, imageInfo, viewInfo, VK_IMAGE_LAYOUT_GENERAL);
        Singleton<MVulkanEngine>::instance().CreateImage(m_JFATexture1, imageInfo, viewInfo, VK_IMAGE_LAYOUT_GENERAL);
    }

    if (translationLayout)
        changeRWTextureLayoutToTexture();
}

void SDFGIApplication::createStorageBuffers()
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
}

BoundingBox SDFGIApplication::GetSameSizeAABB(std::shared_ptr<Scene> scene)
{
    auto sceneAABB = scene->GetBoundingBox();

    auto aabbCenter = 0.5f * (sceneAABB.pMax + sceneAABB.pMin);
    auto aabbHalfScale = 0.5f * (sceneAABB.pMax - sceneAABB.pMin);
    float maxScale = 1.05f * std::max(std::max(aabbHalfScale.x, aabbHalfScale.y), aabbHalfScale.z);

    return BoundingBox(aabbCenter - glm::vec3(maxScale), aabbCenter + glm::vec3(maxScale));
}

void SDFGIApplication::createLightCamera()
{
    VkExtent2D extent = m_shadowPass->GetFrameBuffer(0).GetExtent2D();

    glm::vec3 direction = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetDirection();
    glm::vec3 position = -50.f * direction;

    float fov = 60.f;
    float aspect = (float)extent.width / (float)extent.height;
    float zNear = 0.001f;
    float zFar = 60.f;
    m_lightCamera = std::make_shared<Camera>(position, direction, fov, aspect, zNear, zFar);
    m_lightCamera->SetOrth(true);
}

void SDFGIApplication::setOrthCamera(int axis)
{
    auto aabb = GetSameSizeAABB(m_scene);
    glm::vec3 position;
    glm::vec3 direction;
    float xMin, xMax, yMin, yMax;
    float zNear = 0.f;
    float zFar;
    glm::vec3 up(0.f, 1.f, 0.f);

    switch (axis) {
    case 0:
        position = glm::vec3(aabb.pMax.x, (aabb.pMin.y + aabb.pMax.y) / 2.f, (aabb.pMin.z + aabb.pMax.z) / 2.f);
        direction = glm::vec3(-1.f, 0.f, 0.f);
        zFar = aabb.pMax.x - aabb.pMin.x;
        break;
    case 1:
        position = glm::vec3((aabb.pMin.x + aabb.pMax.x) / 2.f, aabb.pMax.y, (aabb.pMin.z + aabb.pMax.z) / 2.f);
        direction = glm::vec3(0.f, -1.f, 0.f);
        zFar = aabb.pMax.y - aabb.pMin.y;
        break;
    case 2:
        position = glm::vec3((aabb.pMin.x + aabb.pMax.x) / 2.f, (aabb.pMin.y + aabb.pMax.y) / 2.f, aabb.pMax.z);
        direction = glm::vec3(0.f, 0.f, -1.f);
        //zFar = aabb.pMax.z - aabb.pMin.z;

        glm::vec3 center = (aabb.pMin + aabb.pMax) / glm::vec3(2.f);
        xMin = aabb.pMin.x;
        xMax = aabb.pMax.x;
        yMin = aabb.pMin.y;
        yMax = aabb.pMax.y;

        zNear = aabb.pMin.z;
        zFar = aabb.pMax.z;

        break;
    }

    float fov = 60.f;
    float aspectRatio = 1.f;
    //float zNear = 0.01f;
    //float zFar = 1000.f;

    m_orthCamera = std::make_shared<Camera>(position, direction, up, aspectRatio, xMin, xMax, yMin, yMax, zNear, zFar);
    m_orthCamera->SetOrth(true);

}

void SDFGIApplication::initDDGIVolumn()
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
        m_uniformBuffer1.probePos1 = m_volume->GetProbePosition(probeDim.x - 1, probeDim.y - 1, probeDim.z - 1);


        m_uniformBuffer1.minFrontFaceDistance = 0.4f;
        //m_uniformBuffer1.farthestFrontfaceDistance = 0.8f;
        m_uniformBuffer1.probeRelocationEnbled = (int)m_probeRelocationEnabled;
    }
}

void SDFGIApplication::createGbufferPass()
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

void SDFGIApplication::createProbeTracingPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        std::vector<VkFormat> probeTracingPassFormats;
        probeTracingPassFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        probeTracingPassFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);

        VkExtent2D extent2D = { m_raysPerProbe, m_volume->GetNumProbes() };

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

        std::shared_ptr<ShaderModule> probeTracingShader = std::make_shared<SDFProbeTracingShader>();
        std::vector<std::vector<VkImageView>> bufferTextureViews(3);
        auto wholeTextures = Singleton<TextureManager>::instance().GenerateTextureVector();
        auto wholeTextureSize = wholeTextures.size();


        bufferTextureViews[0] = std::vector<VkImageView>(1, m_volumeProbeDatasRadiance->GetImageView());
        bufferTextureViews[1] = std::vector<VkImageView>(1, m_volumeProbeDatasDepth->GetImageView());
        bufferTextureViews[2] = std::vector<VkImageView>(4, m_shadowPass->GetFrameBuffer(0).GetDepthImageView());


        std::vector<std::vector<VkImageView>> storageTextureViews(2);
        storageTextureViews[0].resize(1, m_SDFTexture->GetImageView());
        storageTextureViews[1].resize(1, m_SDFAlbedoTexture->GetImageView());

        std::vector<VkSampler> samplers(1);
        samplers[0] = m_linearSamplerWithoutAnisotropy.GetSampler();

        //auto tlas = m_rayTracing.GetTLAS();
        //std::vector<VkAccelerationStructureKHR> accelerationStructures(1, tlas);
        std::vector<VkAccelerationStructureKHR> accelerationStructures(0);

        std::vector<StorageBuffer> storageBuffers(0);

        storageBuffers.push_back(*m_probesDataBuffer);

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_probeTracingPass, probeTracingShader,
            storageBuffers, bufferTextureViews, storageTextureViews, samplers, accelerationStructures);

        {
            probeTracingShader->SetUBO(0, &m_uniformBuffer1);

            auto aabb = GetSameSizeAABB(m_scene);

            SDFProbeTracingShader::SDFBuffer sdfBuffer;
            sdfBuffer.aabbMin = aabb.pMin;
            sdfBuffer.aabbMax = aabb.pMax;
            sdfBuffer.sdfResolution = m_voxelResolution;
            sdfBuffer.maxRaymarchSteps = m_maxRaymarchSteps;

            probeTracingShader->SetUBO(2, &sdfBuffer);
        }
    }
}

void SDFGIApplication::createLightingPass()
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

        std::shared_ptr<SDFGILightingShader> lightingShader = std::make_shared<SDFGILightingShader>();

        auto probeBuffer = m_volume->GetProbeBuffer();
        std::vector<StorageBuffer> storageBuffers(1);
        storageBuffers[0] = *m_probesDataBuffer;
        //lightingShader->probeBuffer = probeBuffer;

        std::vector<std::vector<VkImageView>> gbufferViews(7);
        for (auto i = 0; i < 4; i++) {
            gbufferViews[i].resize(1);
            gbufferViews[i][0] = m_gbufferPass->GetFrameBuffer(0).GetImageView(i);
        }
        gbufferViews[4] = std::vector<VkImageView>(1, m_volumeProbeDatasRadiance->GetImageView());
        gbufferViews[5] = std::vector<VkImageView>(1, m_volumeProbeDatasDepth->GetImageView());
        gbufferViews[6] = std::vector<VkImageView>(4, m_shadowPass->GetFrameBuffer(0).GetDepthImageView());

        std::vector<std::vector<VkImageView>> storageTextureViews(0);

        std::vector<VkSampler> samplers(1);
        samplers[0] = m_linearSamplerWithoutAnisotropy.GetSampler();

        //auto tlas = m_rayTracing.GetTLAS();
        //std::vector<VkAccelerationStructureKHR> accelerationStructures(1, tlas);
        std::vector<VkAccelerationStructureKHR> accelerationStructures(0);

        //std::vector<uint32_t> storageBufferSizes(0);
        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_lightingPass, lightingShader,
            storageBuffers, gbufferViews, storageTextureViews, samplers, accelerationStructures);

        {
            lightingShader->SetUBO(1, &m_uniformBuffer1);
        }

    }
}

void SDFGIApplication::createProbeBlendingRadiancePass()
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
    seperateImageViews[1][0] = m_probeTracingPass->GetFrameBuffer(0).GetImageView(1);

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

void SDFGIApplication::createProbeBlendingDepthPass()
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
    seperateImageViews[1][0] = m_probeTracingPass->GetFrameBuffer(0).GetImageView(1);

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

void SDFGIApplication::createProbeClassficationPass()
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
    seperateImageViews[1][0] = m_probeTracingPass->GetFrameBuffer(0).GetImageView(1);

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


void SDFGIApplication::createProbeRelocationPass()
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
    seperateImageViews[1][0] = m_probeTracingPass->GetFrameBuffer(0).GetImageView(1);

    std::vector<std::vector<VkImageView>> storageImageViews(0);
    //storageImageViews[0].resize(1);
    //storageImageViews[0][0] = m_testTexture->GetImageView();

    Singleton<MVulkanEngine>::instance().CreateComputePass(m_probeRelocationPass, probeRelocationShader,
        storageBuffers, seperateImageViews, storageImageViews, samplers);

    {
        probeRelocationShader->SetUBO(0, &m_uniformBuffer1);
    }
}

void SDFGIApplication::createProbeVisulizePass()
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

void SDFGIApplication::createCompositePass()
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

        std::shared_ptr<ShaderModule> compositeShader = std::make_shared<SDFGICompositeShader>();
        std::vector<std::vector<VkImageView>> views(3);
        views[0].resize(1);
        views[0][0] = m_lightingPass->GetFrameBuffer(0).GetImageView(0);
        views[1].resize(1);
        views[1][0] = m_lightingPass->GetFrameBuffer(0).GetImageView(1);
        //views[2].resize(1);
        //views[2][0] = m_rtaoPass->GetFrameBuffer(0).GetImageView(0);
        views[2].resize(1);
        views[2][0] = m_probeVisulizePass->GetFrameBuffer(0).GetImageView(0);

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

void SDFGIApplication::createShadowMapPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();
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
        std::vector<std::vector<VkImageView>> shadowShaderTextures(0);
        //shadowShaderTextures[0].resize(0);

        std::vector<VkSampler> samplers(0);

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_shadowPass, shadowShader, shadowShaderTextures, samplers);
    }

    createLightCamera();
}

void SDFGIApplication::createJFAPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    std::shared_ptr<JFAShader> jfaShader = std::make_shared<JFAShader>("main");
    m_JFAPass = std::make_shared<ComputePass>(device);

    std::vector<StorageBuffer> storageBuffers(0);
    std::vector<VkSampler> samplers(0);

    std::vector<std::vector<VkImageView>> seperateImageViews(0);


    std::vector<std::vector<VkImageView>> storageImageViews(3);
    storageImageViews[0].resize(1);
    storageImageViews[0][0] = m_JFATexture0->GetImageView();
    storageImageViews[1].resize(1);
    storageImageViews[1][0] = m_JFATexture1->GetImageView();
    storageImageViews[2].resize(1);
    storageImageViews[2][0] = m_SDFTexture->GetImageView();

    Singleton<MVulkanEngine>::instance().CreateComputePass(m_JFAPass, jfaShader,
        storageBuffers, seperateImageViews, storageImageViews, samplers);

    {
        auto aabb = GetSameSizeAABB(m_scene);

        JFAShader::UniformBuffer0 ubo0;
        ubo0.aabbMin = aabb.pMin;
        ubo0.aabbMax = aabb.pMax;
        ubo0.voxelResolusion = m_voxelResolution;

        jfaShader->SetUBO(0, &ubo0);
    }
}

void SDFGIApplication::createVoxelPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    std::vector<VkFormat> voxelPassFormats;
    voxelPassFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);

    RenderPassCreateInfo info{};
    //info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
    info.extent = { (uint32_t)m_voxelResolution.x, (uint32_t)m_voxelResolution.y };
    info.depthFormat = device.FindDepthFormat();
    info.frambufferCount = 1;
    info.useAttachmentResolve = false;
    info.useSwapchainImages = false;
    info.imageAttachmentFormats = voxelPassFormats;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    info.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
    info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
    info.pipelineCreateInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    info.useDepthBuffer = false;
    //info.pipelineCreateInfo.depthBoundsTestEnable = VK_FALSE; // ¹Ø±ÕÉî¶È·¶Î§²âÊÔ
    //info.pipelineCreateInfo.stencilTestEnable = VK_FALSE; // ¹Ø±ÕÄ£°å²âÊÔ
    info.pipelineCreateInfo.cullmode = VK_CULL_MODE_NONE;
    //info.pipelineCreateInfo.cullmode = VK_CULL_MODE_BACK_BIT;
    //info.depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();

    m_voxelPass = std::make_shared<RenderPass>(device, info);

    std::shared_ptr<ShaderModule> voxelShader = std::make_shared<VoxelShader>();
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

    std::vector<uint32_t> storageBufferSizes(0);

    std::vector<std::vector<VkImageView>> storageTextureViews(2);
    //storageTextureViews[0].resize(1, m_voxelTexture->GetImageView());
    storageTextureViews[0].resize(1, m_JFATexture0->GetImageView());
    storageTextureViews[1].resize(1, m_SDFAlbedoTexture->GetImageView());
    //storageTextureViews[2].resize(1, m_SDFNormalTexture->GetImageView());

    std::vector<VkSampler> samplers(1);
    samplers[0] = m_linearSamplerWithAnisotropy.GetSampler();

    //auto tlas = m_rayTracing.GetTLAS();
    std::vector<VkAccelerationStructureKHR> accelerationStructures(0);

    Singleton<MVulkanEngine>::instance().CreateRenderPass(
        m_voxelPass, voxelShader, storageBufferSizes, bufferTextureViews, storageTextureViews, samplers, accelerationStructures);
}

void SDFGIApplication::createClearTexturesPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    std::shared_ptr<ClearTexturesShader> clearShader = std::make_shared<ClearTexturesShader>("main");
    m_clearTexturesPass = std::make_shared<ComputePass>(device);

    std::vector<StorageBuffer> storageBuffers(0);
    std::vector<VkSampler> samplers(0);

    std::vector<std::vector<VkImageView>> seperateImageViews(0);

    std::vector<std::vector<VkImageView>> storageImageViews(3);
    storageImageViews[0].resize(1);
    storageImageViews[0][0] = m_JFATexture0->GetImageView();
    storageImageViews[1].resize(1);
    storageImageViews[1][0] = m_JFATexture1->GetImageView();
    storageImageViews[2].resize(1);
    storageImageViews[2][0] = m_SDFTexture->GetImageView();
    //storageImageViews[3].resize(1);
    //storageImageViews[3][0] = m_SDFAlbedoTexture->GetImageView();
    //storageImageViews[4].resize(1);
    //storageImageViews[4][0] = m_SDFNormalTexture->GetImageView();

    Singleton<MVulkanEngine>::instance().CreateComputePass(m_clearTexturesPass, clearShader,
        storageBuffers, seperateImageViews, storageImageViews, samplers);
}

void SDFGIApplication::changeTextureLayoutToRWTexture()
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

void SDFGIApplication::changeRWTextureLayoutToTexture()
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

void SDFGIApplication::transitionProbeVisulizeTextureLayoutToShaderRead()
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

void SDFGIApplication::transitionProbeVisulizeTextureLayoutToUndifined()
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

void SDFGIApplication::transitionVoxelTextureLayoutToComputeShaderRead()
{
    std::vector<MVulkanImageMemoryBarrier> barriers;
    {
        MVulkanImageMemoryBarrier barrier{};
        barrier.image = m_voxelTexture->GetImage();
        barrier.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.baseArrayLayer = 0;
        barrier.layerCount = 1;
        barrier.levelCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barriers.push_back(barrier);

        //barrier.image = m_volumeProbeDatasDepth->GetImage();
        //barriers.push_back(barrier);
    }
    Singleton<MVulkanEngine>::instance().TransitionImageLayout(barriers, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
}

void SDFGIApplication::transitionVoxelTextureLayoutToGeneral()
{
    std::vector<MVulkanImageMemoryBarrier> barriers;
    {
        MVulkanImageMemoryBarrier barrier{};
        barrier.image = m_voxelTexture->GetImage();
        barrier.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.baseArrayLayer = 0;
        barrier.layerCount = 1;
        barrier.levelCount = 1;
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = 0;
        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barriers.push_back(barrier);

        //barrier.image = m_volumeProbeDatasDepth->GetImage();
        //barriers.push_back(barrier);
    }
    Singleton<MVulkanEngine>::instance().TransitionImageLayout(barriers, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}
