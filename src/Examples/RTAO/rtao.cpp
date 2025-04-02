#include "rtao.hpp"

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

void RTAO::SetUp()
{
    //Singleton<InputManager>::instance().RegisterApplication(this);

    createSamplers();
    createLight();
    createCamera();
    createTextures();

    loadScene();
    createAS();
}

void RTAO::ComputeAndDraw(uint32_t imageIndex)
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
        m_gbufferPass->GetShader()->SetUBO(0, &ubo0);

        GbufferShader::UniformBufferObject1 ubo1 = GbufferShader::GetFromScene(m_scene);

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
        //}
        m_gbufferPass->GetShader()->SetUBO(1, &ubo1);
    }

    //prepare lightingPass ubo
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
        
        m_rtao_lightingPass->GetShader()->SetUBO(0, &ubo0);
    }
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_gbufferPass, m_currentFrame, m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), m_scene->GetIndirectBuffer(), m_scene->GetIndirectDrawCommands().size());
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(imageIndex, m_rtao_lightingPass, m_currentFrame, m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size());

    //auto camPos = m_camera->GetPosition();
    //spdlog::info("camera_position:({0}, {1}, {2})", camPos.x, camPos.y, camPos.z);

    graphicsList.End();
    Singleton<MVulkanEngine>::instance().SubmitGraphicsCommands(imageIndex, m_currentFrame);

    //auto cameraPos = m_camera->GetPosition();
    //spdlog::info("camera_position:({0}, {1}, {2})", cameraPos[0], cameraPos[1], cameraPos[2]);

}

void RTAO::RecreateSwapchainAndRenderPasses()
{
    if (Singleton<MVulkanEngine>::instance().RecreateSwapchain()) {
        Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_gbufferPass);

        createTextures();

        m_rtao_lightingPass->GetRenderPassCreateInfo().depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();
        Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_rtao_lightingPass);

        std::vector<std::vector<VkImageView>> gbufferViews(4);
        for (auto i = 0; i < 4; i++) {
            gbufferViews[i].resize(1);
            gbufferViews[i][0] = m_gbufferPass->GetFrameBuffer(0).GetImageView(i);
        }

        std::vector<VkSampler> samplers(1);
        samplers[0] = m_linearSampler.GetSampler();

        auto tlas = m_rayTracing.GetTLAS();
        std::vector<VkAccelerationStructureKHR> accelerationStructures(1, tlas);

        m_rtao_lightingPass->UpdateDescriptorSetWrite(gbufferViews, samplers, accelerationStructures);
    }
}

void RTAO::CreateRenderPass()
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
        info.depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();

        m_rtao_lightingPass = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<ShaderModule> lightingShader = std::make_shared<RTAOShader>();
        std::vector<std::vector<VkImageView>> gbufferViews(4);
        for (auto i = 0; i < 4; i++) {
            gbufferViews[i].resize(1);
            gbufferViews[i][0] = m_gbufferPass->GetFrameBuffer(0).GetImageView(i);
        }

        std::vector<uint32_t> storageBufferSizes(0);

        std::vector<std::vector<VkImageView>> storageTextureViews(1);
        storageTextureViews[0].resize(1, m_acculatedAOTexture->GetImageView());

        std::vector<VkSampler> samplers(1);
        samplers[0] = m_linearSampler.GetSampler();

        auto tlas = m_rayTracing.GetTLAS();
        std::vector<VkAccelerationStructureKHR> accelerationStructures(1, tlas);

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_rtao_lightingPass, lightingShader, storageBufferSizes, gbufferViews, storageTextureViews, samplers, accelerationStructures);
    }
}

void RTAO::PreComputes()
{

}

void RTAO::Clean()
{
    m_gbufferPass->Clean();
    m_rtao_lightingPass->Clean();

    m_acculatedAOTexture->Clean();

    m_linearSampler.Clean();

    m_rayTracing.Clean();

    m_squad->Clean();
    m_scene->Clean();

    MRenderApplication::Clean();
}

void RTAO::loadScene()
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

void RTAO::createAS()
{
    m_rayTracing.Create(Singleton<MVulkanEngine>::instance().GetDevice());
    m_rayTracing.TestCreateAccelerationStructure(m_scene);
}

void RTAO::createLight()
{
    glm::vec3 direction = glm::normalize(glm::vec3(-1.f, -6.f, -1.f));
    glm::vec3 color = glm::vec3(1.f, 1.f, 1.f);
    float intensity = 10.f;
    m_directionalLight = std::make_shared<DirectionalLight>(direction, color, intensity);
}

void RTAO::createCamera()
{
    glm::vec3 position(1.2925221, 3.7383504, -0.29563048);
    glm::vec3 center = position + glm::vec3(0.8f, -0.3f, -0.1f);
    glm::vec3 direction = glm::normalize(center - position);

    float fov = 60.f;
    float aspectRatio = (float)WIDTH / (float)HEIGHT;
    float zNear = 0.01f;
    float zFar = 1000.f;

    m_camera = std::make_shared<Camera>(position, direction, fov, aspectRatio, zNear, zFar);
    Singleton<MVulkanEngine>::instance().SetCamera(m_camera);
}

void RTAO::createSamplers()
{
    {
        MVulkanSamplerCreateInfo info{};
        info.minFilter = VK_FILTER_LINEAR;
        info.magFilter = VK_FILTER_LINEAR;
        info.mipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        m_linearSampler.Create(Singleton<MVulkanEngine>::instance().GetDevice(), info);
    }
}

void RTAO::createTextures()
{
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

}


RTAOShader::RTAOShader() :ShaderModule("hlsl/rtao/rtao.vert.hlsl", "hlsl/rtao/rtao.frag.hlsl")
{

}

size_t RTAOShader::GetBufferSizeBinding(uint32_t binding) const
{
    switch (binding) {
    case 0:
        return sizeof(RTAOShader::UniformBuffer0);
    }
}

void RTAOShader::SetUBO(uint8_t binding, void* data)
{
    switch (binding) {
    case 0:
        ubo0 = *reinterpret_cast<RTAOShader::UniformBuffer0*>(data);
        return;
    }
}

void* RTAOShader::GetData(uint32_t binding, uint32_t index)
{
    switch (binding) {
    case 0:
        return (void*)&ubo0;
    }
}