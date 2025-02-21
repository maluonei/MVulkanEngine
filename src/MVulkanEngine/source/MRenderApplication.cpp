#include "MRenderApplication.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"
#include "Managers/InputManager.hpp"
#include "Managers/GlobalConfig.hpp" 

void MRenderApplication::Init()
{
    init();
    //Singleton<InputManager>::instance().RegisterApplication(std::make_shared<MRenderApplication>(this));

    SetUp();

    CreateRenderPass();

    PreComputes();
}

void MRenderApplication::Run()
{
    while (!Singleton<MVulkanEngine>::instance().WindowShouldClose()) {
        renderLoop();
        SetCameraMoved(false);
    }
}

void MRenderApplication::SetUp()
{
}

void MRenderApplication::ComputeAndDraw(uint32_t imageInde)
{
}

void MRenderApplication::RecreateSwapchainAndRenderPasses()
{
}

void MRenderApplication::CreateRenderPass()
{
}

void MRenderApplication::PreComputes()
{
}

void MRenderApplication::Clean()
{
    Singleton<MVulkanEngine>::instance().Clean();
}

void MRenderApplication::init()
{
    Singleton<MVulkanEngine>::instance().SetWindowRes(WIDTH, HEIGHT);
    Singleton<MVulkanEngine>::instance().Init();

    //MRenderApplication::SetCameraMove setCameraMovePtr = &MRenderApplication::SetCameraMoved;
    //Singleton<InputManager>::instance().RegisterSetCameraMoveFunc(setCameraMovePtr);
    //Singleton<InputManager>::instance().RegisterApplication(this);
    Singleton<InputManager>::instance().RegisterApplication(this);
}

void MRenderApplication::renderLoop()
{
    Singleton<MVulkanEngine>::instance().PollWindowEvents();
    Singleton<InputManager>::instance().DealInputs();

    drawFrame();
}

void MRenderApplication::drawFrame()
{
    //spdlog::info("currentFrame:{0}", currentFrame);
    auto fence = Singleton<MVulkanEngine>::instance().GetInFlightFence(m_currentFrame);

    fence.WaitForSignal();

    uint32_t imageIndex;
    VkResult result = Singleton<MVulkanEngine>::instance().AcquireNextSwapchainImage(imageIndex, m_currentFrame);
    //spdlog::info("imageIndex:{0}", imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapchainAndRenderPasses();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    fence.Reset();

    ComputeAndDraw(imageIndex);
    //auto graphicsList = Singleton<MVulkanEngine>::instance().GetGraphicsList(m_currentFrame);
    //auto graphicsQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::GRAPHICS);
    //
    //graphicsList.Reset();
    //graphicsList.Begin();
    //
    //UpdatePerFrame(imageIndex);
    //
    //graphicsList.End();

    //Singleton<MVulkanEngine>::instance().SubmitCommands(imageIndex, m_currentFrame);

    std::function<void()> recreateSwapchain = [this]() {
        this->RecreateSwapchainAndRenderPasses();
        };
    Singleton<MVulkanEngine>::instance().Present(m_currentFrame, &imageIndex, recreateSwapchain);

    m_currentFrame = (m_currentFrame + 1) % Singleton<GlobalConfig>::instance().GetMaxFramesInFlight();
}