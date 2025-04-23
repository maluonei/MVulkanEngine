#include "pbr.hpp"

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

#include "Shaders/share/Common.h"

void PBR::SetUp()
{
    createSamplers();
    createLight();
    createCamera();
    createTextures();
    loadShaders();

    loadScene();
}

void PBR::ComputeAndDraw(uint32_t imageIndex)
{
    auto& graphicsList = Singleton<MVulkanEngine>::instance().GetGraphicsList(m_currentFrame);
    auto& graphicsQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::GRAPHICS);

    //ImageLayoutToAttachment(imageIndex);
    auto swapchainExtent = Singleton<MVulkanEngine>::instance().GetSwapchainImageExtent();

    //prepare gbufferPass ubo
    {
        MVPBuffer mvpBuffer{};
        mvpBuffer.Model = glm::mat4(1.f);
        mvpBuffer.View = m_camera->GetViewMatrix();
        mvpBuffer.Projection = m_camera->GetProjMatrix();
        Singleton<ShaderResourceManager>::instance().LoadData("mvpBuffer", 0, &mvpBuffer, 0);
        //m_gbufferPass->GetShader()->SetUBO(0, &ubo0);

        TexBuffer texBuffer = m_scene->GenerateTexBuffer();
        Singleton<ShaderResourceManager>::instance().LoadData("texBuffer", 0, &texBuffer, 0);
        //m_gbufferPass->GetShader()->SetUBO(1, &ubo1);
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

        Singleton<ShaderResourceManager>::instance().LoadData("lightBuffer", imageIndex, &lightBuffer, 0);
        Singleton<ShaderResourceManager>::instance().LoadData("cameraBuffer", imageIndex, &cameraBuffer, 0);
        Singleton<ShaderResourceManager>::instance().LoadData("screenBuffer", imageIndex, &screenBuffer, 0);
        //m_lightingPass->GetShader()->SetUBO(0, &ubo0);
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

    graphicsList.Reset();
    graphicsList.Begin();

    //todo:  ÷∂Ø±‰ªªimage layout
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_shadowPass, m_currentFrame, shadowmapRenderInfo, m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), m_scene->GetIndirectBuffer(), m_scene->GetIndirectDrawCommands().size(), std::string("Shadowmap Pass"));
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(0, m_gbufferPass, m_currentFrame, gbufferRenderInfo, m_scene->GetIndirectVertexBuffer(), m_scene->GetIndirectIndexBuffer(), m_scene->GetIndirectBuffer(), m_scene->GetIndirectDrawCommands().size(), std::string("Gbuffer Pass"));
    Singleton<MVulkanEngine>::instance().RecordCommandBuffer(imageIndex, m_lightingPass, m_currentFrame, shadingRenderInfo, m_squad->GetIndirectVertexBuffer(), m_squad->GetIndirectIndexBuffer(), m_squad->GetIndirectBuffer(), m_squad->GetIndirectDrawCommands().size(), std::string("Shading Pass"));

    graphicsList.End();
    Singleton<MVulkanEngine>::instance().SubmitGraphicsCommands(imageIndex, m_currentFrame);
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
    //fs::path modelPath = resourcePath / "San_Miguel" / "san-miguel-low-poly.obj";
    //fs::path modelPath = resourcePath / "shapespark_example_room" / "shapespark_example_room.gltf";

    Singleton<SceneLoader>::instance().Load(arcadePath.string(), m_scene.get());

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
}

void PBR::createLight()
{
    glm::vec3 direction = glm::normalize(glm::vec3(-1.f, -6.f, -1.f));
    //glm::vec3 direction = glm::normalize(glm::vec3(-2.f, -1.f, 1.f));
    glm::vec3 color = glm::vec3(1.f, 1.f, 1.f);
    float intensity = 10.f;
    m_directionalLight = std::make_shared<DirectionalLight>(direction, color, intensity);
}

void PBR::createCamera()
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
    auto shadowMapExtent = VkExtent2D{ 4096, 4096 };

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

        gBuffer0 = std::make_shared<MVulkanTexture>();
        gBuffer1 = std::make_shared<MVulkanTexture>();
        gBufferDepth = std::make_shared<MVulkanTexture>();
        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            gBuffer0, swapchainExtent, gbufferFormat
        );

        Singleton<MVulkanEngine>::instance().CreateColorAttachmentImage(
            gBuffer1, swapchainExtent, gbufferFormat
        );

        Singleton<MVulkanEngine>::instance().CreateDepthAttachmentImage(
            gBufferDepth, shadowMapExtent, depthFormat
        );
    }

}

void PBR::loadShaders() 
{
    Singleton<ShaderManager>::instance().AddShader("GBuffer Shader", { "hlsl/gbuffer.vert.hlsl", "hlsl/gbuffer/gbuffer.frag.hlsl", "main", "main" });
    Singleton<ShaderManager>::instance().AddShader("Shadow Shader", { "glsl/shadow.vert.glsl", "glsl/shadow.frag.glsl" });
    Singleton<ShaderManager>::instance().AddShader("Shading Shader", { "hlsl/lighting_pbr.vert.hlsl", "hlsl/lighting_pbr_packed.frag.hlsl" });

}

void PBR::createGbufferPass()
{
    auto device = Singleton<MVulkanEngine>::instance().GetDevice();

    {
        RenderPassCreateInfo info{};
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_UINT);
        info.pipelineCreateInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32G32B32A32_UINT);
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
                "mvpBuffer", 0, 0));
        resources.push_back(
            PassResources::SetBufferResource(
                "texBuffer", 1, 0));

        resources.push_back(
            PassResources::SetSampledImageResource(
                2, 0, bufferTextures));
        resources.push_back(
            PassResources::SetSamplerResource(
                3, 0, m_linearSampler.GetSampler()));

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

            resources.push_back(
                PassResources::SetSamplerResource(
                    6, 0, m_linearSampler.GetSampler()));

            resources.push_back(
                PassResources::SetSampledImageResource(
                    5, 0, shadowViews));


            m_lightingPass->UpdateDescriptorSetWrite(i, resources);
        }
    }
}

//void PBR::ImageLayoutToShaderRead(int currentFrame)
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
//void PBR::ImageLayoutToAttachment(int imageIndex, int currentFrame)
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
//void PBR::ImageLayoutToPresent(int imageIndex, int currentFrame)
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