#include "mdf.hpp"

#include "MVulkanRHI/MVulkanEngine.hpp"
#include "Scene/SceneLoader.hpp"
#include "Scene/Scene.hpp"
#include "Managers/TextureManager.hpp"
#include "Managers/ShaderManager.hpp"
#include "RenderPass.hpp"
#include "Camera.hpp"
#include "Scene/Light/DirectionalLight.hpp"
#include "Shaders/ShaderModule.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/component_wise.hpp>

#include "Shaders/share/Common.h"
#include "Shaders/share/MeshDistanceField.h"
#include "Scene/EmbreeScene.hpp"

#include "MultiThread/ParallelFor.hpp"

#define targetScene m_sphere

void MDF::SetUp()
{
    createSamplers();
    createLight();
    createCamera();
    createTextures();
    loadShaders();

    loadScene();

    //testEmbreeScene();
    createMDF();
    createStorageBuffers();
}

void MDF::ComputeAndDraw(uint32_t imageIndex)
{
    auto& graphicsList = Singleton<MVulkanEngine>::instance().GetGraphicsList(m_currentFrame);
    auto& graphicsQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::GRAPHICS);

    //ImageLayoutToAttachment(imageIndex);
    auto swapchainExtent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();

    //prepare gbufferPass ubo
    //{
    //    MVPBuffer mvpBuffer{};
    //    mvpBuffer.Model = glm::mat4(1.f);
    //    mvpBuffer.View = m_camera->GetViewMatrix();
    //    mvpBuffer.Projection = m_camera->GetProjMatrix();
    //    Singleton<ShaderResourceManager>::instance().LoadData("mvpBuffer", 0, &mvpBuffer, 0);
    //    //m_gbufferPass->GetShader()->SetUBO(0, &ubo0);
    //
    //    TexBuffer texBuffer = m_scene->GenerateTexBuffer();
    //    Singleton<ShaderResourceManager>::instance().LoadData("texBuffer", 0, &texBuffer, 0);
    //    //m_gbufferPass->GetShader()->SetUBO(1, &ubo1);
    //}
    //
    ////prepare shadowPass ubo
    //{
    //    glm::mat4 shadowMVP{};
    //    shadowMVP = m_directionalLightCamera->GetOrthoMatrix() * m_directionalLightCamera->GetViewMatrix();
    //    Singleton<ShaderResourceManager>::instance().LoadData("ShadowMVPBuffer", 0, &shadowMVP, 0);
    //    //m_shadowPass->GetShader()->SetUBO(0, &ubo0);
    //}

    //prepare lightingPass ubo
    {

        //LightBuffer lightBuffer{};
        //lightBuffer.lightNum = 1;
        //lightBuffer.lights[0].direction = m_directionalLightCamera->GetDirection();
        //lightBuffer.lights[0].intensity = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetIntensity();
        //lightBuffer.lights[0].color = std::static_pointer_cast<DirectionalLight>(m_directionalLight)->GetColor();
        //lightBuffer.lights[0].shadowMapIndex = 0;
        //lightBuffer.lights[0].shadowViewProj = m_directionalLightCamera->GetOrthoMatrix() * m_directionalLightCamera->GetViewMatrix();
        //lightBuffer.lights[0].shadowCameraZnear = m_directionalLightCamera->GetZnear();
        //lightBuffer.lights[0].shadowCameraZfar = m_directionalLightCamera->GetZfar();
        //lightBuffer.lights[0].shadowmapResolution = int2(shadowmapExtent.width, shadowmapExtent.height);
        //MeshDistanceFieldInput mdfBuffer = m_mdf.GetMDFBuffer(); 

        MCameraBuffer cameraBuffer{};
        cameraBuffer.cameraPos = m_camera->GetPosition();
        cameraBuffer.cameraDir = m_camera->GetDirection();
        cameraBuffer.zNear = m_camera->GetZnear();
        cameraBuffer.zFar = m_camera->GetZfar();
        cameraBuffer.fovY = glm::radians(m_camera->GetFov());

        MScreenBuffer screenBuffer{};
        screenBuffer.WindowRes = int2(swapchainExtent.width, swapchainExtent.height);

        MSceneBuffer sceneBuffer{};
        sceneBuffer.numInstances = targetScene->GetNumPrimInfos();
        //sceneBuffer.numInstances = 1;

        MDFGlobalBuffer mdfGlobalBuffer{};
        mdfGlobalBuffer.mdfAtlasDim = MeshDistanceField::MdfAtlasDims;

        Singleton<ShaderResourceManager>::instance().LoadData("sceneBuffer", m_currentFrame, &sceneBuffer, 0);
        Singleton<ShaderResourceManager>::instance().LoadData("cameraBuffer", m_currentFrame, &cameraBuffer, 0);
        Singleton<ShaderResourceManager>::instance().LoadData("screenBuffer", m_currentFrame, &screenBuffer, 0);
        Singleton<ShaderResourceManager>::instance().LoadData("mdfGlobalBuffer", m_currentFrame, &mdfGlobalBuffer, 0);
        //m_lightingPass->GetShader()->SetUBO(0, &ubo0);
    }

    //RenderingInfo gbufferRenderInfo, shadowmapRenderInfo, shadingRenderInfo; 
    //{
    //    gbufferRenderInfo.colorAttachments.push_back(
    //        RenderingAttachment{
    //            .texture = gBuffer0,
    //            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    //        }
    //    );
    //    gbufferRenderInfo.colorAttachments.push_back(
    //        RenderingAttachment{
    //            .texture = gBuffer1,
    //            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    //        }
    //    );
    //    gbufferRenderInfo.depthAttachment = RenderingAttachment{
    //            .texture = gBufferDepth,
    //            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    //    };
    //}
    //
    //{
    //    shadowmapRenderInfo.colorAttachments.push_back(
    //        RenderingAttachment{
    //            .texture = shadowMap,
    //            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    //        }
    //    );
    //    shadowmapRenderInfo.depthAttachment = RenderingAttachment{
    //            .texture = shadowMapDepth,
    //            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    //    };
    //}

    RenderingInfo shadingRenderInfo;
    {
        //auto swapChainImages = Singleton<MVulkanEngine>::instance().GetSwa
        auto swapChain = Singleton<MVulkanEngine>::instance().GetSwapchain();

        shadingRenderInfo.colorAttachments.push_back(
            RenderingAttachment{
                .texture = nullptr,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .view = swapChain.GetImageView(m_currentFrame),
            }
        );

        shadingRenderInfo.colorAttachments.push_back(
            RenderingAttachment{
                .texture = swapchainDebugViews0[m_currentFrame],
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                //.view = swapChain.GetImageView(m_currentFrame),
            }
            );

        shadingRenderInfo.depthAttachment = RenderingAttachment{
                .texture = swapchainDepthViews[m_currentFrame],
                .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                //.view = swapchainDepthViews[imageIndex]->GetImageView(),
        };
    }
    //gbufferRenderInfo.offset = { 0, 0 };
    //gbufferRenderInfo.extent = swapchainExtent;
    //
    //shadowmapRenderInfo.offset = { 0, 0 };
    //shadowmapRenderInfo.extent = shadowmapExtent;

    shadingRenderInfo.offset = { 0, 0 };
    shadingRenderInfo.extent = swapchainExtent;

    graphicsList.Reset();
    graphicsList.Begin();

    //todo: �ֶ��任image layout
    //Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_shadowPass, m_currentFrame, shadowmapRenderInfo, m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), m_scene->GetIndirectBuffer(), m_scene->GetIndirectDrawCommands().size(), std::string("Shadowmap Pass"));
    //Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_gbufferPass, m_currentFrame, gbufferRenderInfo, m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), m_scene->GetIndirectBuffer(), m_scene->GetIndirectDrawCommands().size(), std::string("Gbuffer Pass"));
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(imageIndex, m_shadingPass, m_currentFrame, shadingRenderInfo, m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size(), std::string("Shading Pass"));

    graphicsList.End();
    Singleton<MVulkanEngine>::instance().SubmitGraphicsCommands(imageIndex, m_currentFrame);
}

void MDF::RecreateSwapchainAndRenderPasses()
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

void MDF::CreateRenderPass()
{
    //createGbufferPass();
    //createShadowPass();
    createShadingPass();

    createLightCamera();
}

void MDF::PreComputes()
{

}

void MDF::Clean()
{
    //m_gbufferPass->Clean();
    //m_lightingPass->Clean();
    //m_shadowPass->Clean();

    m_linearSampler.Clean();

    m_squad->Clean();
    m_scene->Clean();

    MRenderApplication::Clean();
}

void MDF::loadScene()
{
    m_scene = std::make_shared<Scene>();
    //
    fs::path projectRootPath = PROJECT_ROOT;
    fs::path resourcePath = projectRootPath.append("resources").append("models");
    fs::path modelPath = resourcePath / "Sponza" / "glTF" / "Sponza.gltf";
    fs::path arcadePath = resourcePath / "Arcade" / "Arcade.gltf";
    ////fs::path modelPath = resourcePath / "San_Miguel" / "san-miguel-low-poly.obj";
    ////fs::path modelPath = resourcePath / "shapespark_example_room" / "shapespark_example_room.gltf";
    //
    //Singleton<SceneLoader>::instance().Load(modelPath.string(), m_scene.get());
    ////
    //////split Image
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

    m_squad = std::make_shared<Scene>();
    fs::path squadPath = resourcePath / "squad.obj";
    Singleton<SceneLoader>::instance().Load(squadPath.string(), m_squad.get());

    m_sphere = std::make_shared<Scene>();
    //fs::path cubePath = resourcePath / "sphere.obj";
    //fs::path cubePath = resourcePath / "mdf_test3.obj";
    fs::path cubePath = resourcePath / "mdf_test4_3.obj";
    Singleton<SceneLoader>::instance().Load(cubePath.string(), m_sphere.get());
    //Singleton<SceneLoader>::instance().Load(cubePath.string(), m_sphere.get());


    m_squad->GenerateIndirectDataAndBuffers();
    //m_scene->GenerateIndirectDataAndBuffers();
    m_sphere->GenerateIndirectDataAndBuffers();
}

void MDF::createLight()
{
    glm::vec3 direction = glm::normalize(glm::vec3(-1.f, -6.f, -1.f));
    //glm::vec3 direction = glm::normalize(glm::vec3(-2.f, -1.f, 1.f));
    glm::vec3 color = glm::vec3(1.f, 1.f, 1.f);
    float intensity = 10.f;
    m_directionalLight = std::make_shared<DirectionalLight>(direction, color, intensity);
}

void MDF::createCamera()
{
    //glm::vec3 position(-1.f, 0.f, 4.f);
    //glm::vec3 center(0.f);
    //glm::vec3 direction = glm::normalize(center - position);

    //-4.9944386, 2.9471996, -5.8589
    //glm::vec3 position(-4.9944386, 2.9471996, -5.8589);
    //glm::vec3 direction = glm::normalize(glm::vec3(2.f, -1.f, 2.f));

    glm::vec3 position(0.f, 0.f, 8.f);
    glm::vec3 direction = glm::normalize(glm::vec3(0.f, 0.f, -1.f));

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

void MDF::createSamplers()
{
    {
        MVulkanSamplerCreateInfo info{};
        info.minFilter = VK_FILTER_LINEAR;
        info.magFilter = VK_FILTER_LINEAR;
        info.mipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        m_linearSampler.Create(Singleton<MVulkanEngine>::instance().GetDevice(), info);
    }
}

void MDF::createLightCamera()
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

void MDF::createTextures()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();
    auto swapchainExtent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();
    auto shadowMapExtent = VkExtent2D{ 4096, 4096 };

    auto depthFormat = device.FindDepthFormat();
    auto shadowMapFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

    //{
    //    shadowMap = std::make_shared<MVulkanTexture>();
    //    shadowMapDepth = std::make_shared<MVulkanTexture>();
    //
    //    Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
    //        shadowMap, shadowMapExtent, shadowMapFormat
    //    );
    //
    //    Singleton<MVulkanEngine>::instance().CreateDepthAttachmentImage(
    //        shadowMapDepth, shadowMapExtent, depthFormat
    //    );
    //}

    {
        swapchainDepthViews.resize(Singleton<MVulkanEngine>::instance().GetSwapchainImageCount());
        swapchainDebugViews0.resize(Singleton<MVulkanEngine>::instance().GetSwapchainImageCount());

        for (auto i = 0; i < swapchainDepthViews.size(); i++) {
            //Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            //    shadowMap, shadowMapExtent, shadowMapFormat
            //);
            swapchainDepthViews[i] = std::make_shared<MVulkanTexture>();

            Singleton<MVulkanEngine>::instance().CreateDepthAttachmentImage(
                swapchainDepthViews[i], swapchainExtent, depthFormat
            );

            swapchainDebugViews0[i] = std::make_shared<MVulkanTexture>();
            Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
                swapchainDebugViews0[i], swapchainExtent, Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat()
            );
            //swapchainDebugViews0
        }
    }

    //{
    //    auto gbufferFormat = VK_FORMAT_R32G32B32A32_UINT;
    //
    //    gBuffer0 = std::make_shared<MVulkanTexture>();
    //    gBuffer1 = std::make_shared<MVulkanTexture>();
    //    gBufferDepth = std::make_shared<MVulkanTexture>();
    //    Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
    //        gBuffer0, swapchainExtent, gbufferFormat
    //    );
    //
    //    Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
    //        gBuffer1, swapchainExtent, gbufferFormat
    //    );
    //
    //    Singleton<MVulkanEngine>::instance().CreateDepthAttachmentImage(
    //        gBufferDepth, shadowMapExtent, depthFormat
    //    );
    //}

}

void MDF::loadShaders() 
{
    //Singleton<ShaderManager>::instance().AddShader("GBuffer Shader", { "hlsl/gbuffer.vert.hlsl", "hlsl/gbuffer/gbuffer.frag.hlsl", "main", "main" });
    //Singleton<ShaderManager>::instance().AddShader("Shadow Shader", { "glsl/shadow.vert.glsl", "glsl/shadow.frag.glsl" });
    //Singleton<ShaderManager>::instance().AddShader("Shading Shader", { "hlsl/lighting_pbr.vert.hlsl", "hlsl/mdf/mdf.frag.hlsl" }, true);
    Singleton<ShaderManager>::instance().AddShader("Shading Shader", { "hlsl/lighting_pbr.vert.hlsl", "hlsl/mdf/mdfVisulize.frag.hlsl" }, true);

}

//void MDF::testEmbreeScene()
//{
//    auto mesh = m_sphere->GetMesh(0);
//    
//    EmbreeScene embreeScene;
//    embreeScene.Build(mesh);
//    
//    glm::vec3 origin = glm::vec3(0.f, 3.f, 0.f);
//    glm::vec3 direction = glm::normalize(glm::vec3(0.f, -1.f, 0.f));
//    
//    HitResult result = embreeScene.RayCast(origin, direction);
//    //embreeScene.buildBVH(mesh);
//    return;
//}

void MDF::createMDF()
{
    m_mdfAtlas.Init();

    auto numPrims = targetScene->GetNumPrimInfos();
    auto drawIndexedIndirectCommands = targetScene->GetIndirectDrawCommands();

    spdlog::info("building mdf");

    std::vector<AsyncDistanceFieldTask> tasks;
    for (auto i = 0; i < numPrims; i++) {
        //spdlog::info("building mesh:{0}, total:{1}", i, numPrims);
        auto mesh = targetScene->GetMesh(targetScene->m_primInfos[i].mesh_id);
        //spdlog::info("mesh:{0}",)
        //SparseMeshDistanceField mdf;

        auto scale = mesh->m_box.GetExtent();
        auto maxLength = std::max(scale.x, std::max(scale.y, scale.z));
        maxLength = std::min(maxLength, 3.f);

        tasks.push_back(
            AsyncDistanceFieldTask(mesh, 24 * maxLength)
            //AsyncDistanceFieldTask(mesh, 32)
        );
    }

    ParallelFor(tasks.begin(), tasks.end(), [this](AsyncDistanceFieldTask& task)
        {
            task.DoWork();
        },
        6
    );

    for (auto i = 0; i < numPrims; i++) {
        m_mdfAtlas.LoadData(tasks[i].m_mdf);
    }

    return;
}

void MDF::createStorageBuffers()
{
    //m_mdfBuffers = std::make_shared<StorageBuffer>();

    {
        auto mdfAssets = m_mdfAtlas.m_smdfs;

        std::vector<MeshDistanceFieldInput> mdfInputs(mdfAssets.size());
        for (int i = 0; i < mdfAssets.size(); i++) {
            //auto volumeBounds = mdf.;
            auto& mdf = mdfAssets[i];
            
            mdfInputs[i].distanceFieldToVolumeScaleBias = mdf.DistanceFieldToVolumeScaleBias;
            mdfInputs[i].volumeCenter = mdf.volumeBounds.GetCenter();
            mdfInputs[i].volumeOffset = mdf.volumeBounds.GetExtent();
           

            //glm::mat4 m(1.0f);
            glm::mat4 trans = glm::translate(glm::mat4(1.f), mdf.worldSpaceMeshBounds.GetCenter());
            //glm::mat4 scale = glm::scale(glm::mat4(1.0f), mdf.worldSpaceMeshBounds.GetExtent());
            //glm::mat4 trans = glm::translate(glm::mat4(1.f), mdf.volumeBounds.GetCenter());
            float _scale = glm::compMax(mdf.volumeBounds.GetExtent());
            mdfInputs[i].volumeToWorldScale = _scale;

            glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(_scale));
            mdfInputs[i].VolumeToWorld = trans * scale;
            mdfInputs[i].WorldToVolume = glm::inverse(mdfInputs[i].VolumeToWorld);

            mdfInputs[i].dimensions = mdf.BrickDimensions;
            mdfInputs[i].mdfTextureResolusion = mdf.TextureResolution;

            mdfInputs[i].firstBrickIndex = mdf.firstBrickIndex;

            const glm::vec3 volumeSpacePositionExtent = mdf.localSpaceMeshBounds.GetExtent() * mdf.localToVolumnScale;
            const glm::vec3 _expandSurfaceDistance = MeshDistanceField::GMeshSDFSurfaceBiasExpand * volumeSpacePositionExtent / glm::vec3(mdf.IndirectionDimensions * MeshDistanceField::UniqueDataBrickSize);
            float expandSurfaceDistance = glm::length(_expandSurfaceDistance);
            mdfInputs[i].expandSurfaceDistance = expandSurfaceDistance;
        }

        BufferCreateInfo info{};
        info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        info.arrayLength = 1;
        info.size = sizeof(MeshDistanceFieldInput) * mdfInputs.size();

        m_mdfBuffers = Singleton<MVulkanEngine>::instance().CreateStorageBuffer(info, mdfInputs.data());
    }
}

//void MDF::createGbufferPass()
//{
//    auto device = Singleton<MVulkanEngine>::instance().GetDevice();
//
//    {
//        RenderPassCreateInfo info{};
//        info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_UINT);
//        info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_UINT);
//        info.pipelineCreateInfo.depthAttachmentFormats = device.FindDepthFormat();
//        info.frambufferCount = 1;
//        info.dynamicRender = 1;
//
//        m_gbufferPass = std::make_shared<RenderPass>(device, info);
//        auto shader = Singleton<ShaderManager>::instance().GetShader<ShaderModule>("GBuffer Shader");
//
//        Singleton<MVulkanEngine>::instance().CreateRenderPass(
//            m_gbufferPass, shader);
//
//        //std::vector<VkImageView> bufferTextureViews = Singleton<TextureManager>::instance().GenerateTextureViews();
//        std::vector<std::shared_ptr<MVulkanTexture>> bufferTextures = Singleton<TextureManager>::instance().GenerateTextures();
//
//        std::vector<PassResources> resources;
//        resources.push_back(
//            PassResources::SetBufferResource(
//                "mvpBuffer", 0, 0));
//        resources.push_back(
//            PassResources::SetBufferResource(
//                "texBuffer", 1, 0));
//
//        resources.push_back(
//            PassResources::SetSampledImageResource(
//                2, 0, bufferTextures));
//        resources.push_back(
//            PassResources::SetSamplerResource(
//                3, 0, m_linearSampler.GetSampler()));
//
//        m_gbufferPass->UpdateDescriptorSetWrite(0, resources);
//    }
//}
//
//void MDF::createShadowPass()
//{
//    auto device = Singleton<MVulkanEngine>::instance().GetDevice();
//
//    {
//        RenderPassCreateInfo info{};
//        info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
//        info.pipelineCreateInfo.depthAttachmentFormats = device.FindDepthFormat();
//        info.frambufferCount = 1;
//        info.dynamicRender = 1;
//
//        m_shadowPass = std::make_shared<RenderPass>(device, info);
//
//        auto shadowShader = Singleton<ShaderManager>::instance().GetShader<ShaderModule>("Shadow Shader");
//
//        Singleton<MVulkanEngine>::instance().CreateRenderPass(
//            m_shadowPass, shadowShader);
//
//        std::vector<PassResources> resources;
//        resources.push_back(
//            PassResources::SetBufferResource(
//                "ShadowMVPBuffer", 0, 0));
//
//        m_shadowPass->UpdateDescriptorSetWrite(0, resources);
//    }
//}

void MDF::createShadingPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        //std::vector<VkFormat> lightingPassFormats;
        //lightingPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());
        //lightingPassFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());

        //RenderPassCreateInfo info{};
        RenderPassCreateInfo info{};
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(Singleton<MVulkanEngine>::instance().GetSwapchainImageFormat());
        info.pipelineCreateInfo.depthAttachmentFormats = device.FindDepthFormat();
        //info.numFrameBuffers = 1;
        info.frambufferCount = Singleton<MVulkanEngine>::instance().GetSwapchainImageCount();
        info.pipelineCreateInfo.depthTestEnable = VK_FALSE;
        info.pipelineCreateInfo.depthWriteEnable = VK_FALSE;
        info.dynamicRender = 1;
        //info.depthView = m_gbufferPass->GetFrameBuffer(0).GetDepthImageView();

        m_shadingPass = std::make_shared<RenderPass>(device, info);

        auto shader = Singleton<ShaderManager>::instance().GetShader<ShaderModule>("Shading Shader");

        Singleton<MVulkanEngine>::instance().CreateRenderPass(
            m_shadingPass, shader);


        //std::vector<std::shared_ptr<MVulkanTexture>> shadowViews{
        //    m_shadowPass->GetFrameBuffer(0).GetDepthTexture(),
        //    m_shadowPass->GetFrameBuffer(0).GetDepthTexture()
        //};
        //std::vector<std::shared_ptr<MVulkanTexture>> shadowViews{
        //    shadowMapDepth,
        //    shadowMapDepth
        //};
        std::vector<std::shared_ptr<MVulkanTexture>> mdfTexture{
            //m_mdf.mdfTexture
            m_mdfAtlas.m_mdfAtlas
        };

        std::vector<std::shared_ptr<MVulkanTexture>> mdfTexture2{
            //m_mdf.mdfTexture
            m_mdfAtlas.m_mdfAtlas_origin
        };

        for (int i = 0; i < info.frambufferCount; i++) {
            std::vector<PassResources> resources;

            resources.push_back(
                PassResources::SetBufferResource(
                    "sceneBuffer", 0, 0, i));
            resources.push_back(
                PassResources::SetBufferResource(
                    "cameraBuffer", 1, 0, i));
            resources.push_back(
                PassResources::SetBufferResource(
                    "screenBuffer", 2, 0, i));
            resources.push_back(
                PassResources::SetBufferResource(
                    "mdfGlobalBuffer", 6, 0, i));
            resources.push_back(
                PassResources::SetBufferResource(
                    3, 0, m_mdfBuffers));
            resources.push_back(
                PassResources::SetSampledImageResource(
                    4, 0, mdfTexture));
            //resources.push_back(
            //    PassResources::SetSampledImageResource(
            //        7, 0, mdfTexture2));

            resources.push_back(
                PassResources::SetSamplerResource(
                    5, 0, m_linearSampler.GetSampler()));


            m_shadingPass->UpdateDescriptorSetWrite(i, resources);
        }
    }
}

//void MDF::ImageLayoutToShaderRead(int currentFrame)
//{
//    std::vector<MVulkanImageMemoryBarrier> barriers;
//    {
//        MVulkanImageMemoryBarrier barrier{};
//        barrier.image = gBuffer0->GetImage();
//        barrier.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//        barrier.baseArrayLayer = 0;
//        barrier.layerCount = 1;
//        barrier.levelCount = 1;
//        barrier.srcAccessMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
//        barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//        barriers.push_back(barrier);
//
//        barrier.image = gBuffer1->GetImage();
//        barriers.push_back(barrier);
//
//        barrier.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
//        barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//        barrier.image = shadowMapDepth->GetImage();
//        barriers.push_back(barrier);
//    }
//    Singleton<MVulkanEngine>::instance().TransitionImageLayout2(currentFrame, barriers, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
//}
//
//void MDF::ImageLayoutToAttachment(int imageIndex, int currentFrame)
//{
//    std::vector<MVulkanImageMemoryBarrier> barriers;
//    {
//        MVulkanImageMemoryBarrier barrier{};
//        barrier.image = gBuffer0->GetImage();
//        barrier.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//        barrier.baseArrayLayer = 0;
//        barrier.layerCount = 1;
//        barrier.levelCount = 1;
//        barrier.srcAccessMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//        barrier.dstAccessMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//        barrier.newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
//        barriers.push_back(barrier);
//
//        barrier.image = gBuffer1->GetImage();
//        barriers.push_back(barrier);
//
//        barrier.image = shadowMap->GetImage();
//        barriers.push_back(barrier);
//
//        barrier.image = Singleton<MVulkanEngine>::instance().GetSwapchain().GetImage(imageIndex);
//        barriers.push_back(barrier);
//
//        barrier.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
//        barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//        barrier.image = shadowMapDepth->GetImage();
//        barrier.srcAccessMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
//        barrier.dstAccessMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
//        //barrier.dst
//        barriers.push_back(barrier);
//    }
//    Singleton<MVulkanEngine>::instance().TransitionImageLayout2(currentFrame, barriers, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
//}
//
//void MDF::ImageLayoutToPresent(int imageIndex, int currentFrame)
//{
//    std::vector<MVulkanImageMemoryBarrier> barriers;
//    {
//        MVulkanImageMemoryBarrier barrier{};
//        barrier.image = Singleton<MVulkanEngine>::instance().GetSwapchain().GetImage(imageIndex);
//        barrier.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//        barrier.baseArrayLayer = 0;
//        barrier.layerCount = 1;
//        barrier.levelCount = 1;
//        barrier.srcAccessMask = 0;
//        barrier.dstAccessMask = 0;
//        barrier.oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
//        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
//        barriers.push_back(barrier);
//    }
//    Singleton<MVulkanEngine>::instance().TransitionImageLayout2(currentFrame, barriers, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_NONE);
//}
//