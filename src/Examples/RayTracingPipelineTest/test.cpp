#include "test.hpp"
#include <Scene/SceneLoader.hpp>

#include "MVulkanRHI/MVulkanEngine.hpp"

void RayTracingPipelineTest::SetUp()
{
    loadScene();

    rayTracing.Create(Singleton<MVulkanEngine>::instance().GetDevice());
    rayTracing.TestCreateAccelerationStructure(m_scene);
}

void RayTracingPipelineTest::UpdatePerFrame(uint32_t imageIndex)
{
}

void RayTracingPipelineTest::RecreateSwapchainAndRenderPasses()
{
}

void RayTracingPipelineTest::CreateRenderPass()
{
}

void RayTracingPipelineTest::PreComputes()
{
}

void RayTracingPipelineTest::Clean()
{
}

void RayTracingPipelineTest::loadScene()
{
    m_scene = std::make_shared<Scene>();

    fs::path projectRootPath = PROJECT_ROOT;
    fs::path resourcePath = projectRootPath.append("resources").append("models");
    fs::path modelPath = resourcePath / "Sponza" / "glTF" / "Sponza.gltf";

    Singleton<SceneLoader>::instance().Load(modelPath.string(), m_scene.get());

    m_scene->GenerateMeshBuffers();

}
