#include "ms.hpp"

#include "MVulkanRHI/MVulkanEngine.hpp"
#include "Scene/SceneLoader.hpp"
#include "Scene/Scene.hpp"
#include "Managers/TextureManager.hpp"
#include "RenderPass.hpp"
#include "Camera.hpp"
#include "Scene/Light/DirectionalLight.hpp"
#include "Shaders/ShaderModule.hpp"
#include "Shaders/meshShader.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Meshlet/meshlet.hpp"
#include "Managers/RandomGenerator.hpp"


void MSTestApplication::SetUp()
{
    //createSamplers();
    //createLight();
    createCamera();
    //
    loadScene();

    createStorageBuffers();
}

void MSTestApplication::ComputeAndDraw(uint32_t imageIndex)
{
    auto graphicsList = Singleton<MVulkanEngine>::instance().GetGraphicsList(m_currentFrame);
    auto graphicsQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::GRAPHICS);

    {
        MeshletShader::CameraProperties cam;
    
        auto model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(3.f));
        auto view = m_camera->GetViewMatrix();
        auto proj = m_camera->GetProjMatrix();
    
        cam.Model = model;
        cam.View = view;
        cam.Projection = proj;
        cam.MVP = proj * view * model;

        //cam.View = model;
        //cam.Projection = model;
        //cam.MVP = model;
        //cam.MVP = model;
        m_meshletPass->GetMeshShader()->SetUBO(0, &cam);
        m_meshShaderTestPass2->GetMeshShader()->SetUBO(0, &cam);
    }

    {
        TestMeshShader3::CameraProperties cam;
    
        auto model = glm::mat4(1.0f);
        auto view = m_camera->GetViewMatrix();
        auto proj = m_camera->GetProjMatrix();
        auto vertices = m_suzanne->GetTotalVertexs();
        auto indices = m_suzanne->GetTotalIndices();
    
        cam.Model = model;
        cam.View = view;
        cam.Projection = proj;
        cam.MVP = proj * view * model;
        cam.numVertices = vertices.size();
        cam.numIndices = indices.size();
    
        //cam.View = model;
        //cam.Projection = model;
        //cam.MVP = model;
        //cam.MVP = model;
        m_meshShaderTestPass3->GetMeshShader()->SetUBO(0, &cam);
    }

    {
        MeshletShader2::CameraProperties cam;
        MeshletShader2::SceneProperties sce;

        auto model = glm::mat4(1.0f);
        auto view = m_camera->GetViewMatrix();
        auto proj = m_camera->GetProjMatrix();
        auto vertices = m_suzanne->GetTotalVertexs();
        auto indices = m_suzanne->GetTotalIndices();

        cam.Model = model;
        cam.View = view;
        cam.Projection = proj;
        cam.MVP = proj * view * model;
        //cam.numVertices = vertices.size();
        //cam.numIndices = indices.size();

        sce.InstanceCount = 16;
        sce.MeshletCount = m_meshLet->m_meshlets.size();

        //cam.View = model;
        //cam.Projection = model;
        //cam.MVP = model;
        //cam.MVP = model;
        m_meshletPass2->GetMeshShader()->SetUBO(0, &sce);
        m_meshletPass2->GetMeshShader()->SetUBO(1, &cam);
    }

    graphicsList.Reset();
    graphicsList.Begin();

    //Singleton<MVulkanEngine>::instance().RecordMeshShaderCommandBuffer(
    //    imageIndex, m_meshShaderTestPass, m_currentFrame,
    //    1, 1, 1, "Test Mesh Shader Pass");
    //Singleton<MVulkanEngine>::instance().RecordMeshShaderCommandBuffer(
    //    imageIndex, m_meshShaderTestPass2, m_currentFrame,
    //    1, 1, 1, "Test Mesh Shader Pass2");
    //Singleton<MVulkanEngine>::instance().RecordMeshShaderCommandBuffer(
    //    imageIndex, m_meshletPass, m_currentFrame,
    //    m_meshLet->meshlets.size(), 1, 1, "Test Mesh Shader Pass");
    auto dispatchX = 32;// *32;
    auto instanceCount = 16;
    auto meshletCount = m_meshLet->m_meshlets.size();
    //Singleton<MVulkanEngine>::instance().RecordMeshShaderCommandBuffer(
    //    imageIndex, m_meshletPass, m_currentFrame,
    //    (m_meshLet->meshlets.size() + (dispatchX - 1)) / dispatchX, 1, 1, "Meshlet Shader Pass");
    Singleton<MVulkanEngine>::instance().RecordMeshShaderCommandBuffer(
        imageIndex, m_meshletPass2, m_currentFrame,
        (instanceCount * meshletCount + (dispatchX - 1)) / dispatchX, 1, 1, "Meshlet Shader Pass2");
    //Singleton<MVulkanEngine>::instance().RecordMeshShaderCommandBuffer(
    //    imageIndex, m_meshShaderTestPass3, m_currentFrame,
    //    1, 1, 1, "Test Mesh Shader Pass");
    //Singleton<MVulkanEngine>::instance().RecordMeshShaderCommandBuffer(
    //    imageIndex, m_meshletPass, m_currentFrame,
    //    m_meshLet->meshlets.size(), 1, 1, "Test Mesh Shader Pass");
    ////prepare gbufferPass ubo
    //{
    //    GbufferShader::UniformBufferObject0 ubo0{};
    //    ubo0.Model = glm::mat4(1.f);
    //    ubo0.View = m_camera->GetViewMatrix();
    //    ubo0.Projection = m_camera->GetProjMatrix();
    //    m_gbufferPass->GetShader()->SetUBO(0, &ubo0);
    //
    //    GbufferShader::UniformBufferObject1 ubo1 = GbufferShader::GetFromScene(m_scene);
    //    m_gbufferPass->GetShader()->SetUBO(1, &ubo1);
    //}
    //
    ////prepare shadowPass ubo
    //{
    //    ShadowShader::ShadowUniformBuffer ubo0{};
    //    ubo0.shadowMVP = m_directionalLightCamera->GetOrthoMatrix() * m_directionalLightCamera->GetViewMatrix();
    //    m_shadowPass->GetShader()->SetUBO(0, &ubo0);
    //}
    //
    ////prepare lightingPass ubo
    //{
    //    LightingPbrShader::UniformBuffer0 ubo0{};
    //    ubo0.lightNum = 1;
    //    ubo0.lights[0].direction = m_directionalLightCamera->GetDirection();
    //    ubo0.lights[0].intensity = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetIntensity();
    //    ubo0.lights[0].color = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetColor();
    //    ubo0.lights[0].shadowMapIndex = 0;
    //    ubo0.lights[0].shadowViewProj = m_directionalLightCamera->GetOrthoMatrix() * m_directionalLightCamera->GetViewMatrix();
    //    ubo0.lights[0].cameraZnear = m_directionalLightCamera->GetZnear();
    //    ubo0.lights[0].cameraZfar = m_directionalLightCamera->GetZfar();
    //    ubo0.cameraPos = m_camera->GetPosition();
    //
    //    ubo0.ResolusionWidth = m_shadowPass->GetFrameBuffer(0).GetExtent2D().width;
    //    ubo0.ResolusionHeight = m_shadowPass->GetFrameBuffer(0).GetExtent2D().height;
    //
    //    m_lightingPass->GetShader()->SetUBO(0, &ubo0);
    //}
    //
    //Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_shadowPass, m_currentFrame, m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), m_scene->GetIndirectBuffer(), m_scene->GetIndirectDrawCommands().size());
    //Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_gbufferPass, m_currentFrame, m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), m_scene->GetIndirectBuffer(), m_scene->GetIndirectDrawCommands().size());
    //Singleton<MVulkanEngine>::instance().RecordCommandBuffer(imageIndex, m_lightingPass, m_currentFrame, m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size());
    //
    graphicsList.End();
    Singleton<MVulkanEngine>::instance().SubmitGraphicsCommands(imageIndex, m_currentFrame);
    //
    //spdlog::info("camera.pos: {0}, {1}, {2}", m_camera->GetPosition().x, m_camera->GetPosition().y, m_camera->GetPosition().z);
}

void MSTestApplication::RecreateSwapchainAndRenderPasses()
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

void MSTestApplication::CreateRenderPass()
{
    createMeshletPass();
    createMeshletPass2();
    createTestPass();
    createTestPass2();
    createTestPass3();
}

void MSTestApplication::PreComputes()
{

}

void MSTestApplication::Clean()
{
    //m_gbufferPass->Clean();
    //m_lightingPass->Clean();
    //m_shadowPass->Clean();
    //
    //m_linearSampler.Clean();
    //
    //m_squad->Clean();
    //m_scene->Clean();

    MRenderApplication::Clean();
}

void MSTestApplication::loadScene()
{
    //m_scene = std::make_shared<Scene>();
    //
    fs::path projectRootPath = PROJECT_ROOT;
    fs::path resourcePath = projectRootPath.append("resources").append("models");
    //fs::path modelPath = resourcePath / "Sponza" / "glTF" / "Sponza.gltf";
    ////fs::path modelPath = resourcePath / "San_Miguel" / "san-miguel-low-poly.obj";
    ////fs::path modelPath = resourcePath / "shapespark_example_room" / "shapespark_example_room.gltf";
    //
    //Singleton<SceneLoader>::instance().Load(modelPath.string(), m_scene.get());
    //
    ////split Image
    //{
    //    auto wholeTextures = Singleton<TextureManager>::instance().GenerateTextureVector();
    //
    //    auto& transferList = Singleton<MVulkanEngine>::instance().GetCommandList(MQueueType::TRANSFER);
    //
    //    transferList.Reset();
    //    transferList.Begin();
    //    std::vector<MVulkanImageMemoryBarrier> barriers(wholeTextures.size());
    //    for (auto i = 0; i < wholeTextures.size(); i++) {
    //        MVulkanImageMemoryBarrier barrier{};
    //        barrier.image = wholeTextures[i]->GetImage();
    //
    //        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    //        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    //        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    //        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    //        barrier.levelCount = wholeTextures[i]->GetImageInfo().mipLevels;
    //
    //        barriers[i] = barrier;
    //    }
    //
    //    transferList.TransitionImageLayout(barriers, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    //
    //    transferList.End();
    //
    //    VkSubmitInfo submitInfo{};
    //    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    //    submitInfo.commandBufferCount = 1;
    //    submitInfo.pCommandBuffers = &transferList.GetBuffer();
    //
    //    auto& transferQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::TRANSFER);
    //
    //    transferQueue.SubmitCommands(1, &submitInfo, VK_NULL_HANDLE);
    //    transferQueue.WaitForQueueComplete();
    //
    //    for (auto item : wholeTextures) {
    //        auto texture = item;
    //        Singleton<MVulkanEngine>::instance().GenerateMipMap(*texture);
    //    }
    //}
    //
    //m_squad = std::make_shared<Scene>();
    //fs::path squadPath = resourcePath / "squad.obj";
    //Singleton<SceneLoader>::instance().Load(squadPath.string(), m_squad.get());
    //
    //m_squad->GenerateIndirectDataAndBuffers();
    //m_scene->GenerateIndirectDataAndBuffers();

    m_suzanne = std::make_shared<Scene>();
    //fs::path squadPath = resourcePath / "suzanne.obj";
    fs::path squadPath = resourcePath / "horse_statue_01_1k_LOD_1.obj";
    //Singleton<SceneLoader>::instance().LoadForMeshlet(squadPath.string(), m_suzanne.get());

    //fs::path squadPath = resourcePath / "cube.obj";
    Singleton<SceneLoader>::instance().Load(squadPath.string(), m_suzanne.get());

    m_meshLet = std::make_shared<Meshlet>();

    auto mesh = m_suzanne->GetMesh(0);
    auto numMeshes = m_suzanne->GetNumMeshes();
    //spdlog::info("numMeshes;{0}", numMeshes);
    auto vertices = mesh->vertices;
    auto indices = mesh->indices;

    //spdlog::info("numVertices;{0}", vertices.size());

    auto numVertices = vertices.size();
    std::vector<glm::vec3> positions(numVertices);
    for (int i = 0; i < numVertices; i++) {
        positions[i] = vertices[i].position;
    }


    m_meshLet->createMeshlets(&vertices, &indices);
}
//
//void MSTestApplication::createLight()
//{
//    glm::vec3 direction = glm::normalize(glm::vec3(-1.f, -6.f, -1.f));
//    //glm::vec3 direction = glm::normalize(glm::vec3(-2.f, -1.f, 1.f));
//    glm::vec3 color = glm::vec3(1.f, 1.f, 1.f);
//    float intensity = 10.f;
//    m_directionalLight = std::make_shared<DirectionalLight>(direction, color, intensity);
//}
//
void MSTestApplication::createCamera()
{
    //glm::vec3 position(-1.f, 0.f, 4.f);
    //glm::vec3 center(0.f);
    //glm::vec3 direction = glm::normalize(center - position);

    //-4.9944386, 2.9471996, -5.8589
    glm::vec3 position(0, 1.f, 4.f);
    glm::vec3 direction = glm::normalize(glm::vec3(0.f, -0.2f, -1.f));

    //volumn pmin : -6.512552, 0.31353754, -6.434822
    // volumn pmax : 2.139489, 3.020252, 6.2549577
    //
    float fov = 60.f;
    float aspectRatio = (float)WIDTH / (float)HEIGHT;
    float zNear = 0.01f;
    float zFar = 1000.f;

    m_camera = std::make_shared<Camera>(position, direction, fov, aspectRatio, zNear, zFar);
    Singleton<MVulkanEngine>::instance().SetCamera(m_camera);
}

void MSTestApplication::createStorageBuffers()
{
    {
        auto meshlets = m_meshLet->m_meshlets;

        BufferCreateInfo info{};
        info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        info.arrayLength = 1;
        info.size = meshlets.size() * sizeof(meshopt_Meshlet);

        m_meshletsBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, meshlets.data());
    }

    {
        auto meshletVertices = m_meshLet->m_meshletVertices;

        BufferCreateInfo info{};
        info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        info.arrayLength = 1;
        info.size = meshletVertices.size() * sizeof(uint32_t);

        m_meshletVerticesBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, meshletVertices.data());
    }

    {
        auto meshletTrianglesU32 = m_meshLet->m_meshletTrianglesU32;

        BufferCreateInfo info{};
        info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        info.arrayLength = 1;
        info.size = meshletTrianglesU32.size() * sizeof(uint32_t);

        m_meshletTrianglesBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, meshletTrianglesU32.data());
    }

    {
        auto vertices = m_suzanne->GetTotalVertexs();
        auto indices = m_suzanne->GetTotalIndices();

        auto numVertices = vertices.size();
        std::vector<glm::vec4> positions(numVertices);
        std::vector<glm::vec4> positions2(2*numVertices);
        std::vector<glm::vec4> vertices2(2 * numVertices);
        for (int i = 0; i < numVertices; i++) {
            positions2[2*i] = glm::vec4(vertices[i].position, 0.f);
            positions2[2*i+1] = glm::vec4(Singleton<RandomGenerator>::instance().GetRandomFloat(), Singleton<RandomGenerator>::instance().GetRandomFloat(), Singleton<RandomGenerator>::instance().GetRandomFloat(), 0.f);
            positions[i] = glm::vec4(vertices[i].position, 0.f);

            vertices2[2 * i] = glm::vec4(vertices[i].position, 0.f);
            vertices2[2 * i + 1] = glm::vec4(vertices[i].normal, 0.f);
            //positions[i] = glm::vec3(vertices[i].position);
        }

        BufferCreateInfo info{};
        info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        info.arrayLength = 1;
        info.size = positions.size() * sizeof(glm::vec4);
        spdlog::info("sizeof(glm::vec3):{0}", sizeof(glm::vec3));
        spdlog::info("sizeof(glm::vec4):{0}", sizeof(glm::vec4));

        m_verticesBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, positions.data());
    
        info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        info.arrayLength = 1;
        info.size = positions2.size() * sizeof(glm::vec4);
        m_testVerticesBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, positions2.data());

        info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        info.arrayLength = 1;
        info.size = vertices2.size() * sizeof(glm::vec4);
        m_testVerticesBuffer2 = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, vertices2.data());
        
        info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        info.arrayLength = 1;
        info.size = indices.size() * sizeof(uint32_t);
        m_testIndexBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, indices.data());
    }

    {
        std::vector<glm::mat4> models(4 * 4);
        int index = 0;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                auto rotM = glm::scale(glm::mat4(1.0f), glm::vec3(5.f));
                auto tranM = glm::translate(glm::mat4(1.0f), glm::vec3(i * 2.f, 0.f, j * 2.f));

                models[index] = tranM * rotM;

                index++;
            }
        }

        BufferCreateInfo info{};
        info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        info.arrayLength = 1;
        info.size = models.size() * sizeof(glm::mat4);
        m_modelsBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, models.data());
    }
}
void MSTestApplication::createTestPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();
    {
        std::vector<VkFormat> testPassFormats;
        testPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());

        RenderPassCreateInfo info{};
        info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
        info.depthFormat = device.FindDepthFormat();
        info.frambufferCount = Singleton<MVulkanEngine>::instance().GetSwapchainImageCount();
        info.useSwapchainImages = true;
        info.imageAttachmentFormats = testPassFormats;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        info.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        info.pipelineCreateInfo.cullmode = VK_CULL_MODE_NONE;
        //info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        //info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
        //info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
        //info.depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();

        m_meshShaderTestPass = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<MeshShaderModule> meshShader = std::make_shared<TestMeshShader>();

        std::vector<std::vector<VkImageView>> views(0);

        std::vector<std::vector<VkImageView>> storageTextureViews(0);

        std::vector<VkSampler> samplers(0);

        std::vector<VkAccelerationStructureKHR> accelerationStructures(0);

        std::vector<StorageBuffer> storageBuffers(0);
        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_meshShaderTestPass, meshShader,
            storageBuffers, views, storageTextureViews, samplers, accelerationStructures);
    }
}

void MSTestApplication::createTestPass2()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();
    {
        std::vector<VkFormat> testPassFormats;
        testPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());

        RenderPassCreateInfo info{};
        info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
        info.depthFormat = device.FindDepthFormat();
        info.frambufferCount = Singleton<MVulkanEngine>::instance().GetSwapchainImageCount();
        info.useSwapchainImages = true;
        info.imageAttachmentFormats = testPassFormats;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        info.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        info.pipelineCreateInfo.cullmode = VK_CULL_MODE_NONE;
        //info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        //info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
        //info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
        //info.depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();

        m_meshShaderTestPass2 = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<MeshShaderModule> meshShader = std::make_shared<TestMeshShader2>();

        std::vector<std::vector<VkImageView>> views(0);

        std::vector<std::vector<VkImageView>> storageTextureViews(0);

        std::vector<VkSampler> samplers(0);

        std::vector<VkAccelerationStructureKHR> accelerationStructures(0);

        std::vector<StorageBuffer> storageBuffers(0);
        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_meshShaderTestPass2, meshShader,
            storageBuffers, views, storageTextureViews, samplers, accelerationStructures);
    }
}

void MSTestApplication::createTestPass3()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();
    {
        std::vector<VkFormat> testPassFormats;
        testPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());

        RenderPassCreateInfo info{};
        info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
        info.depthFormat = device.FindDepthFormat();
        info.frambufferCount = Singleton<MVulkanEngine>::instance().GetSwapchainImageCount();
        info.useSwapchainImages = true;
        info.imageAttachmentFormats = testPassFormats;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        info.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        info.pipelineCreateInfo.cullmode = VK_CULL_MODE_NONE;
        //info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        //info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
        //info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
        //info.depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();

        m_meshShaderTestPass3 = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<MeshShaderModule> meshShader = std::make_shared<TestMeshShader3>();

        std::vector<std::vector<VkImageView>> views(0);

        std::vector<std::vector<VkImageView>> storageTextureViews(0);

        std::vector<VkSampler> samplers(0);

        std::vector<VkAccelerationStructureKHR> accelerationStructures(0);

        std::vector<StorageBuffer> storageBuffers(2);
        storageBuffers[0] = *m_testVerticesBuffer;
        storageBuffers[1] = *m_testIndexBuffer;

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_meshShaderTestPass3, meshShader,
            storageBuffers, views, storageTextureViews, samplers, accelerationStructures);
    }
}

void MSTestApplication::createMeshletPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();
    {
        std::vector<VkFormat> testPassFormats;
        testPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());

        RenderPassCreateInfo info{};
        info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
        info.depthFormat = device.FindDepthFormat();
        info.frambufferCount = Singleton<MVulkanEngine>::instance().GetSwapchainImageCount();
        info.useSwapchainImages = true;
        info.imageAttachmentFormats = testPassFormats;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        info.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        info.pipelineCreateInfo.cullmode = VK_CULL_MODE_NONE;
        //info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        //info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
        //info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
        //info.depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();

        m_meshletPass = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<MeshShaderModule> meshShader = std::make_shared<MeshletShader>();

        std::vector<std::vector<VkImageView>> views(0);

        std::vector<std::vector<VkImageView>> storageTextureViews(0);

        std::vector<VkSampler> samplers(0);

        std::vector<VkAccelerationStructureKHR> accelerationStructures(0);

        std::vector<StorageBuffer> storageBuffers(4);
        storageBuffers[0] = *m_verticesBuffer;
        storageBuffers[1] = *m_meshletsBuffer;
        storageBuffers[2] = *m_meshletVerticesBuffer;
        storageBuffers[3] = *m_meshletTrianglesBuffer;

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_meshletPass, meshShader,
            storageBuffers, views, storageTextureViews, samplers, accelerationStructures);
    }
}
void MSTestApplication::createMeshletPass2()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();
    {
        std::vector<VkFormat> testPassFormats;
        testPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());

        RenderPassCreateInfo info{};
        info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
        info.depthFormat = device.FindDepthFormat();
        info.frambufferCount = Singleton<MVulkanEngine>::instance().GetSwapchainImageCount();
        info.useSwapchainImages = true;
        info.imageAttachmentFormats = testPassFormats;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        info.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        info.pipelineCreateInfo.cullmode = VK_CULL_MODE_NONE;
        //info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        //info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
        //info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
        //info.depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();

        m_meshletPass2 = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<MeshShaderModule> meshShader = std::make_shared<MeshletShader2>();

        std::vector<std::vector<VkImageView>> views(0);

        std::vector<std::vector<VkImageView>> storageTextureViews(0);

        std::vector<VkSampler> samplers(0);

        std::vector<VkAccelerationStructureKHR> accelerationStructures(0);

        std::vector<StorageBuffer> storageBuffers(5);
        //storageBuffers[0] = *m_verticesBuffer;
        storageBuffers[0] = *m_testVerticesBuffer2;
        storageBuffers[1] = *m_meshletsBuffer;
        storageBuffers[2] = *m_meshletVerticesBuffer;
        storageBuffers[3] = *m_meshletTrianglesBuffer;
        storageBuffers[4] = *m_modelsBuffer;

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_meshletPass2, meshShader,
            storageBuffers, views, storageTextureViews, samplers, accelerationStructures);
    }

}
//
//void MSTestApplication::createSamplers()
//{
//    {
//        MVulkanSamplerCreateInfo info{};
//        info.minFilter = VK_FILTER_LINEAR;
//        info.magFilter = VK_FILTER_LINEAR;
//        info.mipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
//        m_linearSampler.Create(Singleton<MVulkanEngine>::instance().GetDevice(), info);
//    }
//}
//
//void MSTestApplication::createLightCamera()
//{
//    VkExtent2D extent = m_shadowPass->GetFrameBuffer(0).GetExtent2D();
//
//    glm::vec3 direction = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetDirection();
//    glm::vec3 position = -50.f * direction;
//
//    float fov = 60.f;
//    float aspect = (float)extent.width / (float)extent.height;
//    float zNear = 0.001f;
//    float zFar = 60.f;
//    m_directionalLightCamera = std::make_shared<Camera>(position, direction, fov, aspect, zNear, zFar);
//    m_directionalLightCamera->SetOrth(true);
//}