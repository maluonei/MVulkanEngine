#include "RayQueryShadows.hpp"

#include "MVulkanRHI/MVulkanEngine.hpp"
#include "Scene/SceneLoader.hpp"
#include "Scene/Scene.hpp"
#include "Managers/TextureManager.hpp"
#include "RenderPass.hpp"
#include "ComputePass.hpp"
#include "Camera.hpp"
#include "Scene/Light/DirectionalLight.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>

void RayQueryPbr::SetUp()
{
    createSamplers();
    createLight();
    createCamera();

    loadScene();
    createAS();
}

void RayQueryPbr::ComputeAndDraw(uint32_t imageIndex)
{
    auto graphicsList = Singleton<MVulkanEngine>::instance().GetGraphicsList(m_currentFrame);
    auto graphicsQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::GRAPHICS);

    graphicsList.Reset();
    graphicsList.Begin();

    //prepare gbufferPass ubo
    {
        RayQueryGBufferShader::UniformBuffer0 ubo0;

        auto numPrims = m_scene->m_primInfos.size();
        //auto meshNames = m_scene->GetMeshNames();
        auto drawIndexedIndirectCommands = m_scene->GetIndirectDrawCommands();

        for (auto i = 0; i < numPrims; i++) {
            //auto name = meshNames[i];
            auto mesh = m_scene->GetMesh(m_scene->m_primInfos[i].mesh_id);
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


            if (mat->normalMap != "") {
                auto normalmapIdx = Singleton<TextureManager>::instance().GetTextureId(mat->normalMap);
                ubo0.texBuffer[indirectCommand.firstInstance].normalmapIdx = normalmapIdx;
            }
            else {
                ubo0.texBuffer[indirectCommand.firstInstance].normalmapIdx = -1;
            }
        }
        m_gbufferPass->GetShader()->SetUBO(0, &ubo0);


        auto extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
        RayQueryGBufferShader::UniformBuffer1 ubo1;

        ubo1.cameraPosition = m_camera->GetPosition();
        ubo1.cameraDirection = m_camera->GetDirection();
        ubo1.screenResolution = glm::ivec2(extent.width, extent.height);
        ubo1.fovY = glm::radians(m_camera->GetFov());
        ubo1.zNear = m_camera->GetZnear();
        m_gbufferPass->GetShader()->SetUBO(1, &ubo1);
    }

    //prepare lightingPass ubo
    {
        LightingPbrRayQueryShader::UniformBuffer0 ubo0{};
        ubo0.lightNum = 1;
        ubo0.lights[0].direction = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetDirection();
        ubo0.lights[0].intensity = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetIntensity();
        ubo0.lights[0].color = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetColor();
        ubo0.lightNum = 1;
        ubo0.cameraPos = m_camera->GetPosition();

        m_lightingPass->GetShader()->SetUBO(0, &ubo0);
    }
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_gbufferPass, m_currentFrame, m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size());
    //spdlog::info("gbuffer generation donw");
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(imageIndex, m_lightingPass, m_currentFrame, m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size());

    graphicsList.End();
    Singleton<MVulkanEngine>::instance().SubmitGraphicsCommands(imageIndex, m_currentFrame);
}

void RayQueryPbr::RecreateSwapchainAndRenderPasses()
{
    if (Singleton<MVulkanEngine>::instance().RecreateSwapchain()) {
        Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_gbufferPass);

        m_lightingPass->GetRenderPassCreateInfo().depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();
        Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_lightingPass);

        std::vector<std::vector<VkImageView>> gbufferViews(4);
        for (auto i = 0; i < 4; i++) {
            gbufferViews[i].resize(1);
            gbufferViews[i][0] = m_gbufferPass->GetFrameBuffer(0).GetImageView(i);
        }

        std::vector<VkSampler> samplers(1);
        samplers[0] = m_linearSampler.GetSampler();

        auto tlas = m_rayTracing.GetTLAS();
        std::vector<VkAccelerationStructureKHR> accelerationStructures(1, tlas);

        m_lightingPass->UpdateDescriptorSetWrite(gbufferViews, samplers, accelerationStructures);
    }
}

void RayQueryPbr::CreateRenderPass()
{
    createGbufferPass();
    createLightingPass();
}

void RayQueryPbr::PreComputes()
{

}

void RayQueryPbr::Clean()
{
    m_gbufferPass->Clean();
    m_lightingPass->Clean();
    //m_shadowPass->Clean();

    m_linearSampler.Clean();

    m_squad->Clean();
    m_scene->Clean();

    MRenderApplication::Clean();
}

void RayQueryPbr::createGbufferPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        std::vector<VkFormat> probeTracingPassFormats;
        probeTracingPassFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        probeTracingPassFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        probeTracingPassFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
        probeTracingPassFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);

        VkExtent2D extent2D = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();

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

        m_gbufferPass = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<ShaderModule> rayQueryGBufferShader = std::make_shared<RayQueryGBufferShader>();
        std::vector<std::vector<VkImageView>> bufferTextureViews(1);
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

        std::vector<std::vector<VkImageView>> storageTextureViews(0);

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
            storageBufferSizes[4] = numMeshes * sizeof(RayQueryGBufferShader::GeometryInfo);

            auto shader = std::static_pointer_cast<RayQueryGBufferShader>(rayQueryGBufferShader);
            
            //auto numMeshes = m_scene->GetNumMeshes();
            //auto meshNames = m_scene->GetMeshNames();

            shader->vertexBuffer.position.resize(numVertices);
            shader->indexBuffer.index.resize(numIndices);
            shader->normalBuffer.normal.resize(numVertices);
            shader->uvBuffer.uv.resize(numVertices);
            shader->instanceOffsetBuffer.geometryInfos.resize(numMeshes);

            int vertexBufferIndex = 0;
            int indexBufferIndex = 0;
            int instanceBufferIndex = 0;

            auto numPrims = m_scene->GetNumPrimInfos();

            for (auto i = 0; i < numPrims;i++) {
                auto mesh = m_scene->GetMesh(m_scene->m_primInfos[i].mesh_id);

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
                    RayQueryGBufferShader::GeometryInfo{
                        .vertexOffset = vertexBufferIndex * 3,
                        .indexOffset = indexBufferIndex,
                        .uvOffset = vertexBufferIndex * 2,
                        .normalOffset = vertexBufferIndex * 3,
                        .materialIdx = int(mesh->matId)
                };

                vertexBufferIndex += meshVertexNum;
                indexBufferIndex += meshIndexNum;
                instanceBufferIndex += 1;
            }
        }

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_gbufferPass, rayQueryGBufferShader,
            storageBufferSizes, bufferTextureViews, storageTextureViews, samplers, accelerationStructures);
    }
}

void RayQueryPbr::createLightingPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

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

        m_lightingPass = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<ShaderModule> lightingShader = std::make_shared<LightingPbrRayQueryShader>();
        std::vector<std::vector<VkImageView>> gbufferViews(4);
        for (auto i = 0; i < 4; i++) {
            gbufferViews[i].resize(1);
            gbufferViews[i][0] = m_gbufferPass->GetFrameBuffer(0).GetImageView(i);
        }

        std::vector<VkSampler> samplers(1);
        samplers[0] = m_linearSampler.GetSampler();

        auto tlas = m_rayTracing.GetTLAS();
        std::vector<VkAccelerationStructureKHR> accelerationStructures(1, tlas);

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_lightingPass, lightingShader, gbufferViews, samplers, accelerationStructures);
    }
}

void RayQueryPbr::loadScene()
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

void RayQueryPbr::createAS()
{
    m_rayTracing.Create(Singleton<MVulkanEngine>::instance().GetDevice());
    m_rayTracing.TestCreateAccelerationStructure(m_scene);
}

void RayQueryPbr::createLight()
{
    glm::vec3 direction = glm::normalize(glm::vec3(-1.f, -6.f, -1.f));
    glm::vec3 color = glm::vec3(1.f, 1.f, 1.f);
    float intensity = 10.f;
    m_directionalLight = std::make_shared<DirectionalLight>(direction, color, intensity);
}

void RayQueryPbr::createCamera()
{
    glm::vec3 position(-1.f, 0.f, 4.f);
    glm::vec3 center(0.f);
    glm::vec3 direction = glm::normalize(center - position);

    float fov = 60.f;
    float aspectRatio = (float)WIDTH / (float)HEIGHT;
    float zNear = 0.01f;
    float zFar = 1000.f;

    m_camera = std::make_shared<Camera>(position, direction, fov, aspectRatio, zNear, zFar);
    Singleton<MVulkanEngine>::instance().SetCamera(m_camera);
}

void RayQueryPbr::createSamplers()
{
    {
        MVulkanSamplerCreateInfo info{};
        info.minFilter = VK_FILTER_LINEAR;
        info.magFilter = VK_FILTER_LINEAR;
        info.mipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        m_linearSampler.Create(Singleton<MVulkanEngine>::instance().GetDevice(), info);
    }
}

RayQueryGBufferShader::RayQueryGBufferShader() :ShaderModule("hlsl/rayquery/rayqueryGbuffer.vert.hlsl", "hlsl/rayquery/rayqueryGbuffer.frag.hlsl")
{
}

size_t RayQueryGBufferShader::GetBufferSizeBinding(uint32_t binding) const
{
    switch (binding) {
    case 0:
        return sizeof(RayQueryGBufferShader::UniformBuffer0);
    case 1:
        return sizeof(RayQueryGBufferShader::UniformBuffer1);
    case 2:
        return vertexBuffer.GetSize();;
    case 3:
        return indexBuffer.GetSize();
    case 4:
        return normalBuffer.GetSize();
    case 5:
        return uvBuffer.GetSize();
    case 6:
        return instanceOffsetBuffer.GetSize();
    }
}

void RayQueryGBufferShader::SetUBO(uint8_t binding, void* data)
{
    switch (binding) {
    case 0:
        ubo0 = *reinterpret_cast<RayQueryGBufferShader::UniformBuffer0*>(data);
        return;
    case 1:
        ubo1 = *reinterpret_cast<RayQueryGBufferShader::UniformBuffer1*>(data);
        return;
    }
}

void* RayQueryGBufferShader::GetData(uint32_t binding, uint32_t index)
{
    switch (binding) {
    case 0:
        return (void*)&ubo0;
    case 1:
        return (void*)&ubo1;
    case 2:
        return (void*)vertexBuffer.position.data();
    case 3:
        return (void*)indexBuffer.index.data();
    case 4:
        return (void*)normalBuffer.normal.data();
    case 5:
        return (void*)uvBuffer.uv.data();
    case 6:
        return (void*)instanceOffsetBuffer.geometryInfos.data();
    }
}

LightingPbrRayQueryShader::LightingPbrRayQueryShader() :ShaderModule("hlsl/lighting_pbr.vert.hlsl", "hlsl/rayquery/lighting_pbr_rayquery.frag.hlsl")
{

}

size_t LightingPbrRayQueryShader::GetBufferSizeBinding(uint32_t binding) const
{
    switch (binding) {
    case 0:
        return sizeof(LightingPbrRayQueryShader::UniformBuffer0);
    }
}

void LightingPbrRayQueryShader::SetUBO(uint8_t binding, void* data)
{
    switch (binding) {
    case 0:
        ubo0 = *reinterpret_cast<LightingPbrRayQueryShader::UniformBuffer0*>(data);
        return;
    }
}

void* LightingPbrRayQueryShader::GetData(uint32_t binding, uint32_t index)
{
    switch (binding) {
    case 0:
        return (void*)&ubo0;
    }
}
