#include "VOXEL.hpp"

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
#include "voxel.hpp"
#include "Shaders/voxelShader.hpp"
#include "Shaders/ddgiShader.hpp"

void VOXEL::SetUp()
{
    //Singleton<InputManager>::instance().RegisterApplication(this);

    createSamplers();
    createLight();
    createCamera();
    createTextures();

    loadScene();
    createCube();
    initVoxelVisulizeBuffers();
    //createAS();
}

void VOXEL::ComputeAndDraw(uint32_t imageIndex)
{
    auto graphicsList = Singleton<MVulkanEngine>::instance().GetGraphicsList(m_currentFrame);
    auto graphicsQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::GRAPHICS);

    auto computeList = Singleton<MVulkanEngine>::instance().GetComputeCommandList();
    auto computeQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::COMPUTE);

    //prepare gbufferPass ubo
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

    {
        auto aabb = GetSameSizeAABB(m_scene);
        auto resolution = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();

        SDFTraceShader::UniformBuffer0 ubo0;
        SDFTraceShader::UniformBuffer1 ubo1;
        SDFTraceShader::UniformBuffer2 ubo2;

        ubo0.aabbMin = aabb.pMin;
        ubo0.aabbMax = aabb.pMax;
        ubo0.voxelResolusion = m_voxelResolution;
        m_sdfDebugPass->GetShader()->SetUBO(0, &ubo0);

        ubo1.cameraPos = m_camera->GetPosition();
        ubo1.cameraDir = m_camera->GetDirection();
        ubo1.maxRaymarchSteps = m_maxRaymarchSteps;
        ubo1.ScreenResolution = { resolution.width, resolution.height };
        ubo1.fovY = glm::radians(m_camera->GetFov());
        ubo1.zNear = m_camera->GetZnear();
        m_sdfDebugPass->GetShader()->SetUBO(1, &ubo1);

        ubo2.lightDir = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetDirection();
        ubo2.lightColor = m_directionalLight->GetColor();
        ubo2.lightIntensity = m_directionalLight->GetIntensity();
        m_sdfDebugPass->GetShader()->SetUBO(2, &ubo2);
    }
    

    //{
    //
    //    VoxeVisulizeTestShader::UniformBuffer0 ubo0{};
    //    ubo0.Model = glm::mat4(1.f);
    //    ubo0.View = m_camera->GetViewMatrix();
    //    ubo0.Projection = m_camera->GetProjMatrix();
    //    m_voxelizeTestPass->GetShader()->SetUBO(0, &ubo0);
    //
    //}

    //{
    //    VoxeVisulizeShader::UniformBuffer0 ubo0{};
    //    //ubo0.Model = glm::mat4(1.f);
    //    ubo0.vp.View = m_camera->GetViewMatrix();
    //    ubo0.vp.Projection = m_camera->GetProjMatrix();
    //    m_voxelVisulizePass->GetShader()->SetUBO(0, &ubo0);
    //}
    {
        computeList.Reset();
        computeList.Begin();

        Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(
            m_clearTexturesPass, m_voxelResolution.x/8, m_voxelResolution.y/8, m_voxelResolution.z/8);

        computeList.End();

        VkSubmitInfo submitInfo{};
        submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &computeList.GetBuffer();

        computeQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
        computeQueue.WaitForQueueComplete();
    }

    graphicsList.Reset();
    graphicsList.Begin();

    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_voxelPass, m_currentFrame, m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), m_scene->GetIndirectBuffer(), m_scene->GetIndirectDrawCommands().size());
    
    graphicsList.End();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &graphicsList.GetBuffer();

    graphicsQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    graphicsQueue.WaitForQueueComplete();

    //transitionVoxelTextureLayoutToComputeShaderRead();

    //changeTextureLayoutToRWTexture();

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
                    m_JFAPass, dispatchSize, dispatchSize, dispatchSize);

                computeList.End();

                //VkSubmitInfo submitInfo{};
                submitInfo = {};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &computeList.GetBuffer();

                computeQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
                computeQueue.WaitForQueueComplete();
            }

        }
    }
    //transitionSDFTextureLayoutToFragmentShaderRead();
    //computeList.Reset();
    //computeList.Begin();
    //
    //Singleton<MVulkanEngine>::instance().RecordComputeCommandBuffer(
    //    m_genVoxelVisulizeIndirectComandsPass, (m_voxelResolution.x+7) / 8, (m_voxelResolution.y+7)/8, (m_voxelResolution.z+7)/8);
    //
    //computeList.End();
    //
    ////VkSubmitInfo submitInfo{};
    //submitInfo = {};
    //submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    //submitInfo.commandBufferCount = 1;
    //submitInfo.pCommandBuffers = &computeList.GetBuffer();
    //
    //computeQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    //computeQueue.WaitForQueueComplete();

    //changeRWTextureLayoutToTexture();
    //transitionVoxelTextureLayoutToGeneral();

    graphicsList.Reset();
    graphicsList.Begin();
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(imageIndex, m_sdfDebugPass, m_currentFrame, m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size());
    //Singleton<MVulkanEngine>::instance().RecordCommandBuffer(imageIndex, m_postPass, m_currentFrame, m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size());
    //Singleton<MVulkanEngine>::instance().RecordCommandBuffer(imageIndex, m_voxelizeTestPass, m_currentFrame, m_cube_vertexBuffer, m_cube_indexBuffer, m_cube_indirectBuffer, 1);
    //Buffer* indirectCommandBuffer = &(m_voxelDrawCommandBuffer->GetBuffer());
    //Singleton<MVulkanEngine>::instance().RecordCommandBuffer(imageIndex, m_voxelVisulizePass, m_currentFrame, m_cube_vertexBuffer, m_cube_indexBuffer, m_voxelDrawCommandBuffer, sizeof(DrawCommand),  m_voxelResolution.x * m_voxelResolution.y * m_voxelResolution.z);

    //auto camPos = m_camera->GetPosition();
    //spdlog::info("camera_position:({0}, {1}, {2})", camPos.x, camPos.y, camPos.z);

    graphicsList.End();
    
    Singleton<MVulkanEngine>::instance().SubmitGraphicsCommands(imageIndex, m_currentFrame);
    //transitionSDFTextureLayoutToComputeShaderWrite();

    auto cameraDir = m_camera->GetDirection();
    //spdlog::info("cameraDir:({0}, {1}, {2})", cameraDir[0], cameraDir[1], cameraDir[2]);
}

void VOXEL::RecreateSwapchainAndRenderPasses()
{
    if (Singleton<MVulkanEngine>::instance().RecreateSwapchain()) {
        //Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_gbufferPass);

        createTextures();

        ////m_rtao_lightingPass->GetRenderPassCreateInfo().depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();
        //Singleton<MVulkanEngine>::instance().RecreateRenderPassFrameBuffer(m_rtao_lightingPass);
        //
        //std::vector<std::vector<VkImageView>> gbufferViews(4);
        //for (auto i = 0; i < 4; i++) {
        //    gbufferViews[i].resize(1);
        //    gbufferViews[i][0] = m_gbufferPass->GetFrameBuffer(0).GetImageView(i);
        //}
        //
        //std::vector<VkSampler> samplers(1);
        //samplers[0] = m_linearSampler.GetSampler();
        //
        //auto tlas = m_rayTracing.GetTLAS();
        //std::vector<VkAccelerationStructureKHR> accelerationStructures(1, tlas);
        //
        //m_rtao_lightingPass->UpdateDescriptorSetWrite(gbufferViews, samplers, accelerationStructures);
    }
}

void VOXEL::CreateRenderPass()
{
    createVoxelPass();
    createJFAPass();
    //createPostPass();
    createClearTexturesPass();
    createSDFDebugPass();
    //createVoxelizeTestPass();

    //createGenVoxelVisulizeIndirectComandsPass();
    //createVoxeVisulizePass();
    return;
}

void VOXEL::PreComputes()
{

}

void VOXEL::Clean()
{
    //m_gbufferPass->Clean();
    //m_rtao_lightingPass->Clean();
    //
    //m_acculatedAOTexture->Clean();
    m_voxelPass->Clean();

    m_linearSampler.Clean();

    //m_rayTracing.Clean();

    m_squad->Clean();
    m_scene->Clean();

    MRenderApplication::Clean();
}

void VOXEL::loadScene()
{
    m_scene = std::make_shared<Scene>();

    fs::path projectRootPath = PROJECT_ROOT;
    fs::path resourcePath = projectRootPath.append("resources").append("models");
    //fs::path modelPath = resourcePath / "sphere.obj ";
    fs::path modelPath = resourcePath / "Sponza" / "glTF" / "Sponza.gltf";
    //fs::path modelPath = resourcePath / "suzanne.obj ";

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

void VOXEL::createCube()
{
    //float cubePoses[24] = {
    //    -1.f, -1.f, -1.f,
    //    1.f, -1.f, -1.f,
    //    -1.f, 1.f, -1.f, 
    //    1.f, 1.f, -1.f,
    //    -1.f, -1.f, 1.f,
    //    1.f, -1.f, 1.f,
    //    -1.f, 1.f, 1.f,
    //    1.f, 1.f, 1.f
    //};
    Vertex cubeVertices[8];
    cubeVertices[0].position = { -1.f, -1.f, -1.f };
    cubeVertices[1].position = { 1.f, -1.f, -1.f };
    cubeVertices[2].position = { -1.f, 1.f, -1.f };
    cubeVertices[3].position = { 1.f, 1.f, -1.f };
    cubeVertices[4].position = { -1.f, -1.f, 1.f };
    cubeVertices[5].position = { 1.f, -1.f, 1.f };
    cubeVertices[6].position = { -1.f, 1.f, 1.f };
    cubeVertices[7].position = { 1.f, 1.f, 1.f };
    
    int cubeIndices[24] = {
        0, 1,
        1, 3,
        3, 2,
        2, 0,
        4, 5,
        5, 7,
        7, 6,
        6, 4,
        0, 4,
        1, 5,
        2, 6,
        3, 7
    };

    m_cube_vertexBuffer = std::make_shared<Buffer>(BufferType::VERTEX_BUFFER);
    Singleton<MVulkanEngine>::instance().CreateBuffer(m_cube_vertexBuffer, (const void*)(&cubeVertices[0]), sizeof(cubeVertices));

    m_cube_indexBuffer = std::make_shared<Buffer>(BufferType::INDEX_BUFFER);
    Singleton<MVulkanEngine>::instance().CreateBuffer(m_cube_indexBuffer, (const void*)(&cubeIndices[0]), sizeof(cubeIndices));

    //m_cube_indirectCommand = {
    //    .vertexCount = 8,
    //    .instanceCount = 1,
    //    .firstVertex = 0,
    //    .firstInstance = 0
    //};

    VkDrawIndexedIndirectCommand indexedIndirectCommand = {
        .indexCount = 24,
        .instanceCount = 1,
        .firstIndex = 0,
        .vertexOffset = 0,
        .firstInstance = 0};


    m_cube_indirectBuffer = std::make_shared<Buffer>(BufferType::INDIRECT_BUFFER);
    Singleton<MVulkanEngine>::instance().CreateBuffer(m_cube_indirectBuffer, (const void*)(&indexedIndirectCommand), sizeof(indexedIndirectCommand));

}

//void VOXEL::createAS()
//{
//    m_rayTracing.Create(Singleton<MVulkanEngine>::instance().GetDevice());
//    m_rayTracing.TestCreateAccelerationStructure(m_scene);
//}

void VOXEL::createLight()
{
    glm::vec3 direction = glm::normalize(glm::vec3(-1.f, -6.f, -1.f));
    glm::vec3 color = glm::vec3(1.f, 1.f, 1.f);
    float intensity = 10.f;
    m_directionalLight = std::make_shared<DirectionalLight>(direction, color, intensity);
}

void VOXEL::createCamera()
{
    {
        glm::vec3 position(1.2925221, 3.7383504, -0.29563048);
        //glm::vec3 center = position + glm::vec3(0.f, 0.f, -1.f);
        glm::vec3 direction = glm::normalize(glm::vec3(-0.8, -0.3, -0.1));

        float fov = 60.f;
        float aspectRatio = (float)WIDTH / (float)HEIGHT;
        float zNear = 0.01f;
        float zFar = 1000.f;

        m_camera = std::make_shared<Camera>(position, direction, fov, aspectRatio, zNear, zFar);
        Singleton<MVulkanEngine>::instance().SetCamera(m_camera);
    }
}

void VOXEL::setOrthCamera(int axis)
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
        position = glm::vec3((aabb.pMin.x + aabb.pMax.x)/2.f, (aabb.pMin.y + aabb.pMax.y)/2.f, aabb.pMax.z);
        direction = glm::vec3(0.f, 0.f, -1.f);
        //zFar = aabb.pMax.z - aabb.pMin.z;

        glm::vec3 center = (aabb.pMin + aabb.pMax) / glm::vec3(2.f);
        xMin = aabb.pMin.x;
        xMax = aabb.pMax.x;
        yMin = aabb.pMin.y;
        yMax = aabb.pMax.y;

        zNear =aabb.pMin.z;
        zFar = aabb.pMax.z;

        break;
    }

    float fov = 60.f;
    float aspectRatio = 1.f;
    //float zNear = 0.01f;
    //float zFar = 1000.f;

    m_orthCamera = std::make_shared<Camera>(position, direction, up, aspectRatio, xMin, xMax, yMin, yMax, zNear, zFar);
    m_orthCamera->SetOrth(true);

    //Singleton<MVulkanEngine>::instance().SetCamera(m_camera);
}

void VOXEL::createSamplers()
{
    {
        MVulkanSamplerCreateInfo info{};
        info.minFilter = VK_FILTER_LINEAR;
        info.magFilter = VK_FILTER_LINEAR;
        info.mipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        m_linearSampler.Create(Singleton<MVulkanEngine>::instance().GetDevice(), info);
    }
}

void VOXEL::createVoxelPass()
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
    samplers[0] = m_linearSampler.GetSampler();

    //auto tlas = m_rayTracing.GetTLAS();
    std::vector<VkAccelerationStructureKHR> accelerationStructures(0);

    Singleton<MVulkanEngine>::instance().CreateRenderPass(
        m_voxelPass, voxelShader, storageBufferSizes, bufferTextureViews, storageTextureViews, samplers, accelerationStructures);
}

void VOXEL::createPostPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    std::vector<VkFormat> voxelVisulizePassFormats;
    voxelVisulizePassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());

    RenderPassCreateInfo info{};
    info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
    info.depthFormat = device.FindDepthFormat();
    info.frambufferCount = Singleton<MVulkanEngine>::instance().GetSwapchainImageCount();
    info.useSwapchainImages = true;
    info.imageAttachmentFormats = voxelVisulizePassFormats;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    info.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    //info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
    //info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
    //info.depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();

    m_postPass = std::make_shared<RenderPass>(device, info);

    std::shared_ptr<TestSquadShader> testSquadShader = std::make_shared<TestSquadShader>();
    std::vector<std::vector<VkImageView>> gbufferViews(0);
    //for (auto i = 0; i < 4; i++) {
    //    gbufferViews[i].resize(1);
    //    gbufferViews[i][0] = m_gbufferPass->GetFrameBuffer(0).GetImageView(i);
    //}

    std::vector<uint32_t> storageBufferSizes(0);

    std::vector<std::vector<VkImageView>> storageTextureViews(0);

    std::vector<VkSampler> samplers(0);
    //samplers[0] = m_linearSampler.GetSampler();

    //auto tlas = m_rayTracing.GetTLAS();
    std::vector<VkAccelerationStructureKHR> accelerationStructures(0);

    Singleton<MVulkanEngine>::instance().CreateRenderPass(
        m_postPass, testSquadShader, storageBufferSizes, gbufferViews, storageTextureViews, samplers, accelerationStructures);
}

void VOXEL::createVoxelizeTestPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    std::vector<VkFormat> voxelVisulizePassFormats;
    voxelVisulizePassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());

    RenderPassCreateInfo info{};
    info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
    info.depthFormat = device.FindDepthFormat();
    info.frambufferCount = Singleton<MVulkanEngine>::instance().GetSwapchainImageCount();
    info.useSwapchainImages = true;
    info.imageAttachmentFormats = voxelVisulizePassFormats;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    info.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

    info.pipelineCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    info.pipelineCreateInfo.lineWidth = 1.0f;
    //info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
    //info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
    //info.depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();

    m_voxelizeTestPass = std::make_shared<RenderPass>(device, info);

    std::shared_ptr<VoxeVisulizeTestShader> voxelVisulizeShader = std::make_shared<VoxeVisulizeTestShader>();
    std::vector<std::vector<VkImageView>> gbufferViews(0);
    //for (auto i = 0; i < 4; i++) {
    //    gbufferViews[i].resize(1);
    //    gbufferViews[i][0] = m_gbufferPass->GetFrameBuffer(0).GetImageView(i);
    //}

    std::vector<uint32_t> storageBufferSizes(0);

    std::vector<std::vector<VkImageView>> storageTextureViews(0);

    std::vector<VkSampler> samplers(0);
    //samplers[0] = m_linearSampler.GetSampler();

    //auto tlas = m_rayTracing.GetTLAS();
    std::vector<VkAccelerationStructureKHR> accelerationStructures(0);

    Singleton<MVulkanEngine>::instance().CreateRenderPass(
        m_voxelizeTestPass, voxelVisulizeShader, storageBufferSizes, gbufferViews, storageTextureViews, samplers, accelerationStructures);
}

void VOXEL::createVoxeVisulizePass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    std::vector<VkFormat> voxelVisulizePassFormats;
    voxelVisulizePassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());

    RenderPassCreateInfo info{};
    info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
    info.depthFormat = device.FindDepthFormat();
    info.frambufferCount = Singleton<MVulkanEngine>::instance().GetSwapchainImageCount();
    info.useSwapchainImages = true;
    info.imageAttachmentFormats = voxelVisulizePassFormats;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    info.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

    info.pipelineCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    info.pipelineCreateInfo.lineWidth = 1.0f;
    //info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
    //info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
    //info.depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();

    m_voxelVisulizePass = std::make_shared<RenderPass>(device, info);

    std::shared_ptr<VoxeVisulizeTestShader> voxelVisulizeShader = std::make_shared<VoxeVisulizeTestShader>();
    std::vector<std::vector<VkImageView>> gbufferViews(0);
    //for (auto i = 0; i < 4; i++) {
    //    gbufferViews[i].resize(1);
    //    gbufferViews[i][0] = m_gbufferPass->GetFrameBuffer(0).GetImageView(i);
    //}

    std::vector<StorageBuffer> storageBuffers(1, *m_voxelVisulizeBufferModel);

    std::vector<std::vector<VkImageView>> storageTextureViews(0);

    std::vector<VkSampler> samplers(0);
    //samplers[0] = m_linearSampler.GetSampler();

    //auto tlas = m_rayTracing.GetTLAS();
    std::vector<VkAccelerationStructureKHR> accelerationStructures(0);

    Singleton<MVulkanEngine>::instance().CreateRenderPass(
        m_voxelVisulizePass, voxelVisulizeShader, storageBuffers, gbufferViews, storageTextureViews, samplers, accelerationStructures);
}

void VOXEL::createGenVoxelVisulizeIndirectComandsPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    std::shared_ptr<GenIndirectDrawShader> genIndirectDrawShader = std::make_shared<GenIndirectDrawShader>("main");
    m_genVoxelVisulizeIndirectComandsPass = std::make_shared<ComputePass>(device);

    //auto probeBuffer = m_volume->GetProbeBuffer();
    std::vector<StorageBuffer> storageBuffers(2);
    storageBuffers[0] = *m_voxelDrawCommandBuffer;
    storageBuffers[1] = *m_voxelVisulizeBufferModel;
    //probeBlendingRadianceShader->probeBuffer = probeBuffer;

    //std::vector<uint32_t> storageBufferSizes(0);

    std::vector<VkSampler> samplers(0);

    std::vector<std::vector<VkImageView>> seperateImageViews(1);
    seperateImageViews[0].resize(1);
    seperateImageViews[0][0] = m_voxelTexture->GetImageView();


    std::vector<std::vector<VkImageView>> storageImageViews(0);
    //storageImageViews[0].resize(1);
    //storageImageViews[0][0] = m_voxelTexture->GetImageView();

    Singleton<MVulkanEngine>::instance().CreateComputePass(m_genVoxelVisulizeIndirectComandsPass, genIndirectDrawShader,
        storageBuffers, seperateImageViews, storageImageViews, samplers);

    {
        auto aabb = GetSameSizeAABB(m_scene);

        GenIndirectDrawShader::UniformBuffer0 ubo0;
        ubo0.aabbMin = aabb.pMin;
        ubo0.aabbMax = aabb.pMax;
        ubo0.voxelResolusion = m_voxelResolution;

        genIndirectDrawShader->SetUBO(0, &ubo0);
    }
}

void VOXEL::createJFAPass()
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

void VOXEL::createSDFDebugPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    std::vector<VkFormat> sdfDebugPassFormats;
    sdfDebugPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());
    //sdfDebugPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());
    //sdfDebugPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());
    //sdfDebugPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());
    //sdfDebugPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());
    //sdfDebugPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());
    //sdfDebugPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());
    //sdfDebugPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());

    RenderPassCreateInfo info{};
    info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
    info.depthFormat = device.FindDepthFormat();
    info.frambufferCount = Singleton<MVulkanEngine>::instance().GetSwapchainImageCount();
    info.useSwapchainImages = true;
    info.imageAttachmentFormats = sdfDebugPassFormats;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    info.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

    m_sdfDebugPass = std::make_shared<RenderPass>(device, info);

    std::shared_ptr<SDFTraceShader> sdfDebugShader = std::make_shared<SDFTraceShader>();
    std::vector<std::vector<VkImageView>> seperateImageViews(0);
    //std::vector<std::vector<VkImageView>> seperateImageViews(1);
    //seperateImageViews[0] = std::vector<VkImageView>(1, m_SDFTexture->GetImageView());

    std::vector<StorageBuffer> storageBuffers(1, *m_voxelVisulizeBufferModel);

    //std::vector<std::vector<VkImageView>> storageTextureViews(0);
    std::vector<std::vector<VkImageView>> storageTextureViews(2);
    storageTextureViews[0] = std::vector<VkImageView>(1, m_SDFTexture->GetImageView());
    storageTextureViews[1] = std::vector<VkImageView>(1, m_SDFAlbedoTexture->GetImageView());
    //storageTextureViews[2] = std::vector<VkImageView>(1, m_SDFNormalTexture->GetImageView());

    std::vector<VkSampler> samplers(0);
    //std::vector<VkSampler> samplers(1);
    //samplers[0] = m_linearSampler.GetSampler();

    //auto tlas = m_rayTracing.GetTLAS();
    std::vector<VkAccelerationStructureKHR> accelerationStructures(0);

    Singleton<MVulkanEngine>::instance().CreateRenderPass(
        m_sdfDebugPass, sdfDebugShader, storageBuffers, seperateImageViews, storageTextureViews, samplers, accelerationStructures);
}


void VOXEL::createClearTexturesPass()
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

void VOXEL::createTextures()
{
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
}

void VOXEL::initVoxelVisulizeBuffers()
{
    {
        BufferCreateInfo info{};
        info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        info.arrayLength = 1;
        info.size = sizeof(ModelsBuffer) * m_voxelResolution.x * m_voxelResolution.y * m_voxelResolution.z;

        m_voxelVisulizeBufferModel = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, nullptr);
    }

    {
        BufferCreateInfo info{};
        info.usage = VkBufferUsageFlagBits(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
        info.arrayLength = 1;
        info.size = sizeof(DrawCommand) * m_voxelResolution.x * m_voxelResolution.y * m_voxelResolution.z;

        m_voxelDrawCommandBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, nullptr);
    }
}

void VOXEL::transitionVoxelTextureLayoutToComputeShaderRead()
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

void VOXEL::transitionVoxelTextureLayoutToGeneral()
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

//void VOXEL::transitionSDFTextureLayoutToComputeShaderWrite()
//{
//    std::vector<MVulkanImageMemoryBarrier> barriers;
//    {
//        MVulkanImageMemoryBarrier barrier{};
//        barrier.image = m_SDFTexture->GetImage();
//        barrier.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//        barrier.baseArrayLayer = 0;
//        barrier.layerCount = 1;
//        barrier.levelCount = 1;
//        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
//        barrier.dstAccessMask = 0;
//        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
//        barriers.push_back(barrier);
//
//        //barrier.image = m_volumeProbeDatasDepth->GetImage();
//        //barriers.push_back(barrier);
//    }
//    Singleton<MVulkanEngine>::instance().TransitionImageLayout(barriers, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
//}
//
//void VOXEL::transitionSDFTextureLayoutToFragmentShaderRead()
//{
//    std::vector<MVulkanImageMemoryBarrier> barriers;
//    {
//        MVulkanImageMemoryBarrier barrier{};
//        barrier.image = m_SDFTexture->GetImage();
//        barrier.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//        barrier.baseArrayLayer = 0;
//        barrier.layerCount = 1;
//        barrier.levelCount = 1;
//        barrier.srcAccessMask = 0;
//        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
//        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
//        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//        barriers.push_back(barrier);
//
//        //barrier.image = m_volumeProbeDatasDepth->GetImage();
//        //barriers.push_back(barrier);
//    }
//    Singleton<MVulkanEngine>::instance().TransitionImageLayout(barriers, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
//}

BoundingBox VOXEL::GetSameSizeAABB(std::shared_ptr<Scene> scene)
{
    auto sceneAABB = scene->GetBoundingBox();

    auto aabbCenter = 0.5f * (sceneAABB.pMax + sceneAABB.pMin);
    auto aabbHalfScale = 0.5f * (sceneAABB.pMax - sceneAABB.pMin);
    float maxScale = 1.05f * std::max(std::max(aabbHalfScale.x, aabbHalfScale.y), aabbHalfScale.z);

    return BoundingBox(aabbCenter - glm::vec3(maxScale), aabbCenter + glm::vec3(maxScale));
}
