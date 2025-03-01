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
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "ddgi.hpp"


void DDGIApplication::SetUp()
{
    //Singleton<InputManager>::instance().RegisterApplication(this);

    createSamplers();
    createLight();
    createCamera();
    createTextures();
    initDDGIVolumn();

    loadScene();
    createAS();
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

        GbufferShader::UniformBufferObject1 ubo1[256];

        auto meshNames = m_scene->GetMeshNames();
        auto drawIndexedIndirectCommands = m_scene->GetIndirectDrawCommands();

        for (auto i = 0; i < meshNames.size(); i++) {
            auto name = meshNames[i];
            auto mesh = m_scene->GetMesh(name);
            auto mat = m_scene->GetMaterial(mesh->matId);
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
        }
        m_gbufferPass->GetShader()->SetUBO(1, &ubo1);
    }

    //prepare gbufferPass ubo
    {
        ProbeTracingShader::UniformBuffer0 ubo0{};
    
        auto meshNames = m_scene->GetMeshNames();
        auto drawIndexedIndirectCommands = m_scene->GetIndirectDrawCommands();
    
        for (auto i = 0; i < meshNames.size(); i++) {
            auto name = meshNames[i];
            auto mesh = m_scene->GetMesh(name);
            auto mat = m_scene->GetMaterial(mesh->matId);
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
        ubo2.probePos0 = m_volume->GetProbePosition(0, 0, 0);
        ubo2.probePos1 = m_volume->GetProbePosition(7, 7, 7);
        m_probeTracingPass->GetShader()->SetUBO(2, &ubo2);
    }

    //prepare rtaoPass ubo
    {
        RTAOShader::UniformBuffer0 ubo0{};
        ubo0.lightNum = 1;
        ubo0.lights[0].direction = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetDirection();
        ubo0.lights[0].intensity = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetIntensity();
        ubo0.lights[0].color = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetColor();
        ubo0.cameraPos = m_camera->GetPosition();
        
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
        ubo0.probePos0 = m_volume->GetProbePosition(0, 0, 0);
        ubo0.probePos1 = m_volume->GetProbePosition(7, 7, 7);

        m_lightingPass->GetShader()->SetUBO(0, &ubo0);
    }

    //spdlog::info("1");

    {
        graphicsList.Reset();
        graphicsList.Begin();

        Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_gbufferPass, m_currentFrame, m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), m_scene->GetIndirectBuffer(), m_scene->GetIndirectDrawCommands().size());
        Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_probeTracingPass, m_currentFrame, m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size());
        graphicsList.End();

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &graphicsList.GetBuffer();

        graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
        graphicsQueue.WaitForQueueComplete();
    }

    //spdlog::info("2");

    {
        changeTextureLayoutToRWTexture();

        computeList.Reset();
        computeList.Begin();

        Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(m_probeBlendingRadiancePass, 512, 64, 1);

        computeList.End();

        VkSubmitInfo submitInfo{};
        submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &computeList.GetBuffer();

        computeQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
        computeQueue.WaitForQueueComplete();
    }

    //spdlog::info("3");

    {
        changeRWTextureLayoutToTexture();

        graphicsList.Reset();
        graphicsList.Begin();
        Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_lightingPass, m_currentFrame, m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size());
        Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_rtaoPass, m_currentFrame, m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size());
        Singleton<MVulkanEngine>::instance().RecordCommandBuffer(imageIndex, m_compositePass, m_currentFrame, m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size());
        //auto camPos = m_camera->GetPosition();
        //spdlog::info("camera_position:({0}, {1}, {2})", camPos.x, camPos.y, camPos.z);

        graphicsList.End();
        Singleton<MVulkanEngine>::instance().SubmitGraphicsCommands(imageIndex, m_currentFrame);
    }

    //auto cameraPos = m_camera->GetPosition();
    //spdlog::info("camera_position:({0}, {1}, {2})", cameraPos[0], cameraPos[1], cameraPos[2]);

    //spdlog::info("4");
}

void DDGIApplication::RecreateSwapchainAndRenderPasses()
{
    if (Singleton<MVulkanEngine>::instance().RecreateSwapchain()) {
        Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_gbufferPass);

        createTextures();

        m_rtaoPass->GetRenderPassCreateInfo().depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();
        Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_rtaoPass);

        std::vector<std::vector<VkImageView>> gbufferViews(4);
        for (auto i = 0; i < 4; i++) {
            gbufferViews[i].resize(1);
            gbufferViews[i][0] = m_gbufferPass->GetFrameBuffer(0).GetImageView(i);
        }

        std::vector<VkSampler> samplers(1);
        samplers[0] = m_linearSampler.GetSampler();

        auto tlas = m_rayTracing.GetTLAS();
        std::vector<VkAccelerationStructureKHR> accelerationStructures(1, tlas);

        m_rtaoPass->UpdateDescriptorSetWrite(gbufferViews, samplers, accelerationStructures);
    }
}

void DDGIApplication::CreateRenderPass()
{
    createGbufferPass();
    createProbeTracingPass();
    createProbeBlendingRadiancePass();
    createLightingPass();
    createRTAOPass();
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

    m_acculatedAOTexture->Clean();

    m_linearSampler.Clean();

    m_rayTracing.Clean();

    m_squad->Clean();
    m_scene->Clean();

    MRenderApplication::Clean();
}

void DDGIApplication::loadScene()
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
    m_scene->GenerateMeshBuffers();
}

void DDGIApplication::createAS()
{
    m_rayTracing.Create(Singleton<MVulkanEngine>::instance().GetDevice());
    m_rayTracing.TestCreateAccelerationStructure(m_scene);
}

void DDGIApplication::createLight()
{
    glm::vec3 direction = glm::normalize(glm::vec3(-1.f, -6.f, -1.f));
    glm::vec3 color = glm::vec3(1.f, 1.f, 1.f);
    float intensity = 20.f;
    m_directionalLight = std::make_shared<DirectionalLight>(direction, color, intensity);
}

void DDGIApplication::createCamera()
{
    glm::vec3 position(1.2925221, 3.7383504, -0.29563048);
    glm::vec3 center = position + glm::vec3(-0.8f, -0.3f, -0.1f);
    glm::vec3 direction = glm::normalize(center - position);

    float fov = 60.f;
    float aspectRatio = (float)WIDTH / (float)HEIGHT;
    float zNear = 0.01f;
    float zFar = 1000.f;

    m_camera = std::make_shared<Camera>(position, direction, fov, aspectRatio, zNear, zFar);
    Singleton<MVulkanEngine>::instance().SetCamera(m_camera);
}

void DDGIApplication::createSamplers()
{
    {
        MVulkanSamplerCreateInfo info{};
        info.minFilter = VK_FILTER_LINEAR;
        info.magFilter = VK_FILTER_LINEAR;
        info.mipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        m_linearSampler.Create(Singleton<MVulkanEngine>::instance().GetDevice(), info);
    }
}

void DDGIApplication::createTextures()
{
    if (!m_volumeProbeDatasDepth) {
        m_volumeProbeDatasDepth = std::make_shared<MVulkanTexture>();
    }
    else {
        m_volumeProbeDatasDepth->Clean();
    }

    if (!m_volumeProbeDatasRadiance) {
        m_volumeProbeDatasRadiance = std::make_shared<MVulkanTexture>();
    }
    else {
        m_volumeProbeDatasRadiance->Clean();
    }

    if (!m_acculatedAOTexture) {
        m_acculatedAOTexture = std::make_shared<MVulkanTexture>();
    }
    else {
        m_acculatedAOTexture->Clean();
    }

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

    {
        ImageCreateInfo imageInfo;
        ImageViewCreateInfo viewInfo;
        imageInfo.arrayLength = 1;
        imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.width = 512;
        imageInfo.height = 64;
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


    {
        ImageCreateInfo imageInfo;
        ImageViewCreateInfo viewInfo;
        imageInfo.arrayLength = 1;
        imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.width = 512;
        imageInfo.height = 64;
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

    changeRWTextureLayoutToTexture();
}

void DDGIApplication::initDDGIVolumn()
{
    glm::vec3 startPosition = glm::vec3(-11.f, 0.5f, -4.5f);
    glm::vec3 offset = glm::vec3(2.5f, 1.3f, 1.05f);
    //glm::vec3 startPosition = glm::vec3(-12.5f, 0.5f, -6.f);
    //glm::vec3 offset = glm::vec3(2.9f, 1.3f, 1.45f);
    m_volume = std::make_shared<DDGIVolume>(startPosition, offset);
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
        std::vector<VkSampler> samplers(1, m_linearSampler.GetSampler());

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

        VkExtent2D extent2D = { 64, 512 };

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
        samplers[0] = m_linearSampler.GetSampler();

        auto tlas = m_rayTracing.GetTLAS();
        std::vector<VkAccelerationStructureKHR> accelerationStructures(1, tlas);

        std::vector<uint32_t> storageBufferSizes(5);
        {
            auto numVertices = m_scene->GetNumVertices();
            auto numIndices = m_scene->GetNumTriangles() * 3;
            auto numMeshes = m_scene->GetNumMeshes();

            storageBufferSizes[0] = numVertices * sizeof(glm::vec3);
            storageBufferSizes[1] = numIndices * sizeof(int);
            storageBufferSizes[2] = storageBufferSizes[0];
            storageBufferSizes[3] = numVertices * sizeof(glm::vec2);
            storageBufferSizes[4] = numMeshes * sizeof(ProbeTracingShader::GeometryInfo);

            auto shader = std::static_pointer_cast<ProbeTracingShader>(probeTracingShader);
            auto meshNames = m_scene->GetMeshNames();

            shader->vertexBuffer.position.resize(numVertices);
            shader->indexBuffer.index.resize(numIndices);
            shader->normalBuffer.normal.resize(numVertices);
            shader->uvBuffer.uv.resize(numVertices);
            shader->instanceOffsetBuffer.geometryInfos.resize(numMeshes);

            int vertexBufferIndex = 0;
            int indexBufferIndex = 0;
            int instanceBufferIndex = 0;

            for (auto meshName : meshNames) {
                auto mesh = m_scene->GetMesh(meshName);
                auto meshVertexNum = mesh->vertices.size();
                for (auto i = 0; i < meshVertexNum; i++) {
                    shader->vertexBuffer.position[vertexBufferIndex + i] = mesh->vertices[i].position;
                    shader->normalBuffer.normal[vertexBufferIndex + i] = mesh->vertices[i].normal;
                    shader->uvBuffer.uv[vertexBufferIndex + i] = mesh->vertices[i].texcoord;
                }
                auto meshIndexNum = mesh->indices.size();
                for (auto i = 0; i < meshIndexNum; i++) {
                    shader->indexBuffer.index[indexBufferIndex + i] = mesh->indices[i];
                }

                shader->instanceOffsetBuffer.geometryInfos[instanceBufferIndex] =
                    ProbeTracingShader::GeometryInfo{
                        .vertexOffset = vertexBufferIndex,
                        .indexOffset = indexBufferIndex,
                        .uvOffset = vertexBufferIndex,
                        .normalOffset = vertexBufferIndex,
                        .materialIdx = int(mesh->matId)
                };

                vertexBufferIndex += meshVertexNum;
                indexBufferIndex += meshIndexNum;
                instanceBufferIndex += 1;
            }
        }

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_probeTracingPass, probeTracingShader,
            storageBufferSizes, bufferTextureViews, storageTextureViews, samplers, accelerationStructures);
    
        {
            ProbeTracingShader::UniformBuffer1 ubo1{};
            for (int x = 0; x < 8; x++) {
                for (int y = 0; y < 8; y++) {
                    for (int z = 0; z < 8; z++) {
                        int index = x * 64 + y * 8 + z;
                        ubo1.probes[index].position = m_volume->GetProbePosition(x, y, z);
                    }
                }
            }
            probeTracingShader->SetUBO(1, &ubo1);
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

        std::shared_ptr<ShaderModule> lightingShader = std::make_shared<DDGILightingShader>();
        std::vector<std::vector<VkImageView>> gbufferViews(6);
        for (auto i = 0; i < 4; i++) {
            gbufferViews[i].resize(1);
            gbufferViews[i][0] = m_gbufferPass->GetFrameBuffer(0).GetImageView(i);
        }
        gbufferViews[4] = std::vector<VkImageView>(1, m_volumeProbeDatasRadiance->GetImageView());
        gbufferViews[5] = std::vector<VkImageView>(1, m_volumeProbeDatasDepth->GetImageView());

        std::vector<std::vector<VkImageView>> storageTextureViews(0);
        //storageTextureViews[0].resize(1, m_volumeProbeDatasRadiance->GetImageView());
        //storageTextureViews[1].resize(1, m_volumeProbeDatasDepth->GetImageView());

        std::vector<VkSampler> samplers(1);
        samplers[0] = m_linearSampler.GetSampler();

        auto tlas = m_rayTracing.GetTLAS();
        std::vector<VkAccelerationStructureKHR> accelerationStructures(1, tlas);

        std::vector<uint32_t> storageBufferSizes(0);
        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_lightingPass, lightingShader,
            storageBufferSizes, gbufferViews, storageTextureViews, samplers, accelerationStructures);
    
        {
            DDGILightingShader::UniformBuffer1 ubo1{};
            for (int x = 0; x < 8; x++) {
                for (int y = 0; y < 8; y++) {
                    for (int z = 0; z < 8; z++) {
                        int index = x * 64 + y * 8 + z;

                        ubo1.probes[index].position = m_volume->GetProbePosition(index);
                    }
                }
            }

            lightingShader->SetUBO(1, &ubo1);
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
        std::vector<std::vector<VkImageView>> gbufferViews(4);
        for (auto i = 0; i < 4; i++) {
            gbufferViews[i].resize(1);
            gbufferViews[i][0] = m_gbufferPass->GetFrameBuffer(0).GetImageView(i);
        }

        std::vector<std::vector<VkImageView>> storageTextureViews(1);
        storageTextureViews[0].resize(1, m_acculatedAOTexture->GetImageView());

        std::vector<VkSampler> samplers(1);
        samplers[0] = m_linearSampler.GetSampler();

        auto tlas = m_rayTracing.GetTLAS();
        std::vector<VkAccelerationStructureKHR> accelerationStructures(1, tlas);

        std::vector<uint32_t> storageBufferSizes(0);
        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_rtaoPass, rtaoShader,
            storageBufferSizes, gbufferViews, storageTextureViews, samplers, accelerationStructures);
    }
}

void DDGIApplication::createProbeBlendingRadiancePass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    std::shared_ptr<ComputeShaderModule> probeBlendingRadianceShader = std::make_shared<ProbeBlendRadianceShader>();
    m_probeBlendingRadiancePass = std::make_shared<ComputePass>(device);

    std::vector<uint32_t> storageBufferSizes(0);

    std::vector<VkSampler> samplers(1, m_linearSampler.GetSampler());

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
        storageBufferSizes, seperateImageViews, storageImageViews, samplers);

    {
        ProbeBlendRadianceShader::UniformBuffer0 ubo0;
        for (int i = 0; i < 512; i++) {
            ubo0.probes[i].position = m_volume->GetProbePosition(i);
        }

        //auto shader = m_probeBlendingRadiancePass->GetShader();
        probeBlendingRadianceShader->SetUBO(0, &ubo0);
    }
}

void DDGIApplication::createProbeVisualizePass()
{

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
        std::vector<std::vector<VkImageView>> gbufferViews(3);
        gbufferViews[0].resize(1);
        gbufferViews[0][0] = m_lightingPass->GetFrameBuffer(0).GetImageView(0);
        gbufferViews[1].resize(1);
        gbufferViews[1][0] = m_lightingPass->GetFrameBuffer(0).GetImageView(1);
        gbufferViews[2].resize(1);
        gbufferViews[2][0] = m_rtaoPass->GetFrameBuffer(0).GetImageView(0);

        std::vector<std::vector<VkImageView>> storageTextureViews(0);

        std::vector<VkSampler> samplers(1);
        samplers[0] = m_linearSampler.GetSampler();

        std::vector<VkAccelerationStructureKHR> accelerationStructures(0);

        std::vector<uint32_t> storageBufferSizes(0);
        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_compositePass, compositeShader,
            storageBufferSizes, gbufferViews, storageTextureViews, samplers, accelerationStructures);
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
