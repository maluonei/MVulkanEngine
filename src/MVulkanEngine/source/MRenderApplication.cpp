#include "MRenderApplication.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"
#include "Managers/InputManager.hpp"
#include "Managers/GlobalConfig.hpp" 

void MRenderApplication::Init()
{
    init();

    SetUp();

    CreateRenderPass();

    PreComputes();
}

void MRenderApplication::Run()
{
    while (!Singleton<MVulkanEngine>::instance().WindowShouldClose()) {
        renderLoop();
    }
}

void MRenderApplication::init()
{
    Singleton<MVulkanEngine>::instance().SetWindowRes(WIDTH, HEIGHT);
    Singleton<MVulkanEngine>::instance().Init();
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
    auto fence = Singleton<MVulkanEngine>::instance().GetInFlightFence(currentFrame);

    fence.WaitForSignal(Singleton<MVulkanEngine>::instance().GetDevice().GetDevice());

    uint32_t imageIndex;
    VkResult result = Singleton<MVulkanEngine>::instance().AcquireNextSwapchainImage(imageIndex, currentFrame);
    //spdlog::info("imageIndex:{0}", imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapchainAndRenderPasses();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    fence.Reset(Singleton<MVulkanEngine>::instance().GetDevice().GetDevice());

    auto graphicsList = Singleton<MVulkanEngine>::instance().GetGraphicsList(currentFrame);
    auto graphicsQueue = Singleton<MVulkanEngine>::instance().GetCommandQueue(MQueueType::GRAPHICS);

    graphicsList.Reset();
    graphicsList.Begin();

    UpdatePerFrame(imageIndex);

    graphicsList.End();

    std::function<void()> recreateSwapchain = [this]() {
        this->RecreateSwapchainAndRenderPasses();
        };

    Singleton<MVulkanEngine>::instance().SubmitCommandsAndPresent(imageIndex, currentFrame, recreateSwapchain);

    currentFrame = (currentFrame + 1) % Singleton<GlobalConfig>::instance().GetMaxFramesInFlight();
}