#include "vbuffer.hpp"

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
    createSamplers();
    createLight();
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
        VBufferShader::CameraProperties cam;
        VBufferShader::SceneProperties sce;

        auto model = glm::mat4(1.0f);
        auto view = m_camera->GetViewMatrix();
        auto proj = m_camera->GetProjMatrix();

        cam.Model = model;
        cam.View = view;
        cam.Projection = proj;
        cam.MVP = proj * view * model;

        //sce.InstanceCount = 16;
        sce.MeshletCount = m_meshLet->m_meshlets.size();

        m_vbufferPass->GetMeshShader()->SetUBO(0, &sce);
        m_vbufferPass->GetMeshShader()->SetUBO(1, &cam);
    }

    {
        VBufferCullingShader::CameraProperties cam;
        VBufferCullingShader::SceneProperties sce;
    
        //auto model = glm::mat4(1.0f);
        auto view = m_camera->GetViewMatrix();
        auto proj = m_camera->GetProjMatrix();
    
        cam.View = view;
        cam.Projection = proj;
        //cam.MVP = proj * view * model;
    
        //sce.InstanceCount = 16;
        sce.MeshletCount = m_meshLet->m_meshlets.size();
    
        auto cameraFrustumData = m_camera->GetFrustumData();
    
        FrustumPlane frLeft = cameraFrustumData.Planes[0];
        FrustumPlane frRight = cameraFrustumData.Planes[1];
        FrustumPlane frTop = cameraFrustumData.Planes[2];
        FrustumPlane frBottom = cameraFrustumData.Planes[3];
        FrustumPlane frNear = cameraFrustumData.Planes[4];
        FrustumPlane frFar = cameraFrustumData.Planes[5];
        //sce.CameraVP = camera.GetViewProjectionMatrix();
        sce.Frustum.Planes[FRUSTUM_PLANE_LEFT] = { frLeft.Normal, 0.0f, frLeft.Position, 0.0f };
        sce.Frustum.Planes[FRUSTUM_PLANE_RIGHT] = { frRight.Normal, 0.0f, frRight.Position, 0.0f };
        sce.Frustum.Planes[FRUSTUM_PLANE_TOP] = { frTop.Normal, 0.0f, frTop.Position, 0.0f };
        sce.Frustum.Planes[FRUSTUM_PLANE_BOTTOM] = { frBottom.Normal, 0.0f, frBottom.Position, 0.0f };
        sce.Frustum.Planes[FRUSTUM_PLANE_NEAR] = { frNear.Normal, 0.0f, frNear.Position, 0.0f };
        sce.Frustum.Planes[FRUSTUM_PLANE_FAR] = { frFar.Normal, 0.0f, frFar.Position, 0.0f };
        sce.Frustum.Sphere = cameraFrustumData.Sphere;
        sce.Frustum.Cone.Tip = cameraFrustumData.Cone.Tip;
        sce.Frustum.Cone.Height = cameraFrustumData.Cone.Height;
        sce.Frustum.Cone.Direction = cameraFrustumData.Cone.Direction;
        sce.Frustum.Cone.Angle = cameraFrustumData.Cone.Angle;
        sce.VisibilityFunc = 1;
    
        m_vbufferPass2->GetMeshShader()->SetUBO(0, &sce);
        m_vbufferPass2->GetMeshShader()->SetUBO(1, &cam);
    }

    {
        auto shader = m_shadingPass->GetShader();
        VBufferShadingShader::UnifomBuffer0 ubo0{};
        VBufferShadingShader::UnifomBuffer1 ubo1{};

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
                ubo1.tex[indirectCommand.firstInstance].diffuseTextureIdx = diffuseTexId;
            }
            else {
                ubo1.tex[indirectCommand.firstInstance].diffuseTextureIdx = -1;
            }

            if (mat->metallicAndRoughnessTexture != "") {
                auto metallicAndRoughnessTexId = Singleton<TextureManager>::instance().GetTextureId(mat->metallicAndRoughnessTexture);
                ubo1.tex[indirectCommand.firstInstance].metallicAndRoughnessTextureIdx = metallicAndRoughnessTexId;
            }
            else {
                ubo1.tex[indirectCommand.firstInstance].metallicAndRoughnessTextureIdx = -1;
            }
        }
        shader->SetUBO(1, &ubo1);

        ubo0.lightNum = 1;
        ubo0.lights[0].direction = m_directionalLightCamera->GetDirection();
        ubo0.lights[0].intensity = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetIntensity();
        ubo0.lights[0].color = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetColor();
        ubo0.lights[0].shadowMapIndex = 0;
        ubo0.lights[0].shadowViewProj = m_directionalLightCamera->GetOrthoMatrix() * m_directionalLightCamera->GetViewMatrix();
        ubo0.lights[0].cameraZnear = m_directionalLightCamera->GetZnear();
        ubo0.lights[0].cameraZfar = m_directionalLightCamera->GetZfar();
        ubo0.cameraPos = m_camera->GetPosition();

        ubo0.ShadowmapResWidth = m_shadowPass->GetFrameBuffer(0).GetExtent2D().width;
        ubo0.ShadowmapResHeight = m_shadowPass->GetFrameBuffer(0).GetExtent2D().height;
        ubo0.WindowResWidth = m_shadingPass->GetFrameBuffer(0).GetExtent2D().width;
        ubo0.WindowResHeight = m_shadingPass->GetFrameBuffer(0).GetExtent2D().height;

        shader->SetUBO(0, &ubo0);
    }

    //prepare shadowPass ubo
    {
        ShadowShader::ShadowUniformBuffer ubo0{};
        ubo0.shadowMVP = m_directionalLightCamera->GetOrthoMatrix() * m_directionalLightCamera->GetViewMatrix();
        m_shadowPass->GetShader()->SetUBO(0, &ubo0);
    }

    graphicsList.Reset();
    graphicsList.Begin();

    
    auto dispatchX = 32;// *32;
    //auto instanceCount = 16;
    auto meshletCount = m_meshLet->m_meshlets.size();

    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(
        0, m_shadowPass, m_currentFrame, 
        m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), 
        m_scene->GetIndirectBuffer(), m_scene->GetIndirectDrawCommands().size(), std::string("Shadowmap Pass"));

    Singleton<MVulkanEngine>::instance().RecordMeshShaderCommandBuffer(
        0, m_vbufferPass, m_currentFrame,
        (meshletCount + (dispatchX - 1)) / dispatchX, 1, 1, std::string("VBuffer Pass"));

    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(
        imageIndex, m_shadingPass, m_currentFrame, 
        m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size(), std::string("Shading Pass"));


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
    //createMeshletPass();
    //createMeshletPass2();
    //createTestPass();
    //createTestPass2();
    //createTestPass3();
    createShadowmapPass();
    createVBufferPass();
    createVBufferPass2();
    createShadingPass();
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
    m_scene = std::make_shared<Scene>();
    //
    fs::path projectRootPath = PROJECT_ROOT;
    fs::path resourcePath = projectRootPath.append("resources").append("models");
    fs::path modelPath = resourcePath / "Sponza" / "glTF" / "Sponza.gltf";
    //fs::path modelPath = resourcePath / "Lantern" / "new" / "Lantern2.gltf";
    //fs::path modelPath = resourcePath / "horse_statue_01_1k_LOD_2_6.obj";
    ////fs::path modelPath = resourcePath / "San_Miguel" / "san-miguel-low-poly.obj";
    ////fs::path modelPath = resourcePath / "shapespark_example_room" / "shapespark_example_room.gltf";
    //
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
    //
    m_squad = std::make_shared<Scene>();
    fs::path squadPath = resourcePath / "squad.obj";
    Singleton<SceneLoader>::instance().Load(squadPath.string(), m_squad.get());
    
    m_squad->GenerateIndirectDataAndBuffers();
    m_scene->GenerateIndirectDataAndBuffers();

    //m_suzanne = std::make_shared<Scene>();
    ////fs::path squadPath = resourcePath / "suzanne.obj";
    //fs::path squadPath = resourcePath / "horse_statue_01_1k_LOD_1.obj";
    //Singleton<SceneLoader>::instance().LoadForMeshlet(squadPath.string(), m_suzanne.get());

    //fs::path squadPath = resourcePath / "cube.obj";
    //Singleton<SceneLoader>::instance().Load(squadPath.string(), m_suzanne.get());

    m_meshLet = std::make_shared<Meshlet>();

    //auto mesh = m_scene->GetMesh(0);
    //auto numMeshes = m_scene->GetNumMeshes();
    ////spdlog::info("numMeshes;{0}", numMeshes);
    //auto vertices = mesh->vertices;
    //auto indices = mesh->indices;
    //
    //m_meshLet->createMeshlets(&vertices, &indices);

    m_meshLet->createMeshlets(m_scene);
}

void MSTestApplication::createLight()
{
    glm::vec3 direction = glm::normalize(glm::vec3(-1.f, -6.f, -1.f));
    //glm::vec3 direction = glm::normalize(glm::vec3(-2.f, -1.f, 1.f));
    glm::vec3 color = glm::vec3(1.f, 1.f, 1.f);
    float intensity = 10.f;
    m_directionalLight = std::make_shared<DirectionalLight>(direction, color, intensity);
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
    glm::vec3 position(-4.9944386, 2.9471996, -5.8589);
    glm::vec3 direction = glm::normalize(glm::vec3(2.f, -1.f, 2.f));


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

void MSTestApplication::createSamplers()
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
        m_linearSampler.Create(Singleton<MVulkanEngine>::instance().GetDevice(), info);
    }
}

void MSTestApplication::createLightCamera()
{
    VkExtent2D extent = m_shadowPass->GetFrameBuffer(0).GetExtent2D();

    glm::vec3 direction = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetDirection();
    glm::vec3 position = -50.f * direction;

    float fov = 60.f;
    float aspect = (float)extent.width / (float)extent.height;
    float zNear = 0.001f;
    float zFar = 60.f;
    m_directionalLightCamera = std::make_shared<Camera>(position, direction, fov, aspect, zNear, zFar);
    m_directionalLightCamera->SetOrth(true);
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
        auto meshletAddon = m_meshLet->m_addons;

        BufferCreateInfo info{};
        info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        info.arrayLength = 1;
        info.size = meshletAddon.size() * sizeof(MeshletAddon);

        m_meshletAddonBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, meshletAddon.data());
    }

    {
        struct Vertex {
            glm::vec3 pos;
            float u;
            glm::vec3 normal;
            float v;
        };

        auto vertices = m_scene->GetTotalVertexs();
        //auto indices = m_scene->GetTotalIndices();

        auto numVertices = vertices.size();
        std::vector<Vertex> vertices2(numVertices);
        for (int i = 0; i < numVertices; i++) {

            vertices2[i] = Vertex{
                .pos = vertices[i].position,
                .u = vertices[i].texcoord.x,
                .normal = vertices[i].normal,
                .v = vertices[i].texcoord.y,
            };
            //vertices2[2 * i + 1] = glm::vec4(vertices[i].normal, 0.f);
        }

        BufferCreateInfo info{};
        info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        info.arrayLength = 1;
        info.size = vertices2.size() * sizeof(Vertex);

        m_verticesBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, vertices2.data());
    }

    {
        auto numPrims = m_scene->GetNumPrimInfos();
        std::vector<glm::mat4> models(numPrims, glm::mat4(1.0f));

        BufferCreateInfo info{};
        info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        info.arrayLength = 1;
        info.size = models.size() * sizeof(glm::mat4);

        m_modelMatrixBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, models.data());
    }

    {
        auto meshletBounds = m_meshLet->m_meshletBounds;

        BufferCreateInfo info{};
        info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        info.arrayLength = 1;
        info.size = meshletBounds.size() * sizeof(glm::vec4);

        m_meshletBoundsBuffer = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, meshletBounds.data());
    }

    //{
    //    std::vector<glm::mat4> models(4 * 4);
    //    int index = 0;
    //    for (int i = 0; i < 4; i++) {
    //        for (int j = 0; j < 4; j++) {
    //            auto rotM = glm::scale(glm::mat4(1.0f), glm::vec3(5.f));
    //            auto tranM = glm::translate(glm::mat4(1.0f), glm::vec3(i * 2.f, 0.f, j * 2.f));
    //
    //            models[index] = tranM * rotM;
    //
    //            index++;
    //        }
    //    }
    //}
}

void MSTestApplication::createVBufferPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();
    {
        std::vector<VkFormat> testPassFormats;
        testPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());
        testPassFormats.push_back(VK_FORMAT_R32G32B32A32_UINT);
        //testPassFormats.push_back(VK_FORMAT_R32G32B32A32_UINT);
        //testPassFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);

        RenderPassCreateInfo info{};
        info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
        info.depthFormat = device.FindDepthFormat();
        info.frambufferCount = 1;
        info.useSwapchainImages = false;
        info.useAttachmentResolve = false;
        info.imageAttachmentFormats = testPassFormats;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalDepthLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        //info.pipelineCreateInfo.cullmode = VK_CULL_MODE_NONE;
        info.pipelineCreateInfo.depthTestEnable = VK_TRUE;
        info.pipelineCreateInfo.depthWriteEnable = VK_TRUE;
        //info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        //info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
        //info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
        //info.depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();

        m_vbufferPass = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<MeshShaderModule> meshShader = std::make_shared<VBufferShader>();

        std::vector<std::vector<VkImageView>> views(0);

        std::vector<std::vector<VkImageView>> storageTextureViews(0);

        std::vector<VkSampler> samplers(0);

        std::vector<VkAccelerationStructureKHR> accelerationStructures(0);

        std::vector<StorageBuffer> storageBuffers(5);
        storageBuffers[0] = *m_verticesBuffer;
        //storageBuffers[0] = *m_testVerticesBuffer2;
        storageBuffers[1] = *m_meshletsBuffer;
        storageBuffers[2] = *m_meshletVerticesBuffer;
        storageBuffers[3] = *m_meshletTrianglesBuffer;
        storageBuffers[4] = *m_meshletAddonBuffer;
        //storageBuffers[4] = *m_modelsBuffer;

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_vbufferPass, meshShader,
            storageBuffers, views, storageTextureViews, samplers, accelerationStructures);
    }

}

void MSTestApplication::createVBufferPass2()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();
    {
        std::vector<VkFormat> testPassFormats;
        testPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());
        testPassFormats.push_back(VK_FORMAT_R32G32B32A32_UINT);
        //testPassFormats.push_back(VK_FORMAT_R32G32B32A32_UINT);
        //testPassFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);

        RenderPassCreateInfo info{};
        info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
        info.depthFormat = device.FindDepthFormat();
        info.frambufferCount = 1;
        info.useSwapchainImages = false;
        info.useAttachmentResolve = false;
        info.imageAttachmentFormats = testPassFormats;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalDepthLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.pipelineCreateInfo.cullmode = VK_CULL_MODE_NONE;
        info.pipelineCreateInfo.depthTestEnable = VK_TRUE;
        info.pipelineCreateInfo.depthWriteEnable = VK_TRUE;
        //info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        //info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
        //info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
        //info.depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();

        m_vbufferPass2 = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<MeshShaderModule> meshShader = std::make_shared<VBufferCullingShader>();

        std::vector<std::vector<VkImageView>> views(0);

        std::vector<std::vector<VkImageView>> storageTextureViews(0);

        std::vector<VkSampler> samplers(0);

        std::vector<VkAccelerationStructureKHR> accelerationStructures(0);

        std::vector<StorageBuffer> storageBuffers(10);
        storageBuffers[0] = *m_meshletBoundsBuffer;
        storageBuffers[1] = *m_modelMatrixBuffer;
        storageBuffers[2] = *m_meshletsBuffer;
        storageBuffers[3] = *m_meshletAddonBuffer;
        storageBuffers[4] = *m_modelMatrixBuffer;
        storageBuffers[5] = *m_meshletsBuffer;
        storageBuffers[6] = *m_meshletAddonBuffer;
        storageBuffers[7] = *m_verticesBuffer;
        storageBuffers[8] = *m_meshletVerticesBuffer;
        storageBuffers[9] = *m_meshletTrianglesBuffer;
        //storageBuffers[4] = *m_modelsBuffer;

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_vbufferPass2, meshShader,
            storageBuffers, views, storageTextureViews, samplers, accelerationStructures);
    }
}

void MSTestApplication::createShadingPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        std::vector<VkFormat> shadingFormats;
        shadingFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());

        RenderPassCreateInfo info{};
        info.extent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
        info.depthFormat = device.FindDepthFormat();
        info.frambufferCount = Singleton<MVulkanEngine>::instance().GetSwapchainImageCount();
        info.useSwapchainImages = true;
        info.imageAttachmentFormats = shadingFormats;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        info.initialDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.finalDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        //info.pipelineCreateInfo.cullmode = VK_CULL_MODE_NONE;

        m_shadingPass = std::make_shared<RenderPass>(device, info);

        std::shared_ptr<ShaderModule> shadingShader = std::make_shared<VBufferShadingShader>();
        
        std::vector<std::vector<VkImageView>> bufferTextureViews(3);
        auto wholeTextures = Singleton<TextureManager>::instance().GenerateTextureVector();
        auto wholeTextureSize = wholeTextures.size();

        if (wholeTextureSize == 0) {
            bufferTextureViews[1].resize(1);
            bufferTextureViews[1][0] = Singleton<MVulkanEngine>::instance().GetPlaceHolderTexture().GetImageView();
        }
        else {
            bufferTextureViews[1].resize(wholeTextureSize);
            for (auto j = 0; j < wholeTextureSize; j++) {
                bufferTextureViews[1][j] = wholeTextures[j]->GetImageView();
            }
        }

        bufferTextureViews[0] = std::vector<VkImageView>(1, m_vbufferPass->GetFrameBuffer(0).GetImageView(1));
        //bufferTextureViews[1] = std::vector<VkImageView>(1, m_vbufferPass->GetFrameBuffer(0).GetImageView(2));
        bufferTextureViews[2] = std::vector<VkImageView>(2, m_shadowPass->GetFrameBuffer(0).GetDepthImageView());

        std::vector<std::vector<VkImageView>> storageTextureViews(0);
        //storageTextureViews[0].resize(1, m_volumeProbeDatasRadiance->GetImageView());
        //storageTextureViews[1].resize(1, m_volumeProbeDatasDepth->GetImageView());

        std::vector<VkSampler> samplers(1);
        samplers[0] = m_linearSampler .GetSampler();

        //auto tlas = m_rayTracing.GetTLAS();
        std::vector<VkAccelerationStructureKHR> accelerationStructures(0);

        std::vector<StorageBuffer> storageBuffers(4);
        storageBuffers[0] = *m_verticesBuffer;
        storageBuffers[1] = *m_meshletsBuffer;
        storageBuffers[2] = *m_meshletVerticesBuffer;
        storageBuffers[3] = *m_meshletTrianglesBuffer;

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_shadingPass, shadingShader,
            storageBuffers, bufferTextureViews, storageTextureViews, samplers, accelerationStructures);

    }
}

void MSTestApplication::createShadowmapPass()
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
        std::vector<std::vector<VkImageView>> shadowShaderTextures(1);
        shadowShaderTextures[0].resize(0);

        std::vector<VkSampler> samplers(0);

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_shadowPass, shadowShader, shadowShaderTextures, samplers);
    }

    createLightCamera();
}
