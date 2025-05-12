#include "MRenderApplication.hpp"
#include "MVulkanRHI/MVulkanEngine.hpp"
#include "Managers/InputManager.hpp"
#include "Managers/GlobalConfig.hpp" 
#include "UIRenderer.hpp"

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
        auto start = std::chrono::high_resolution_clock::now();
        
        renderLoop();
        SetCameraMoved(false);

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        m_fpsTime += (float)elapsed.count();
        m_fpsFrameIndex++;

        if (m_fpsFrameIndex == 100) {
            m_fps = 100000.0 / m_fpsTime;

            //spdlog::info("m_fpsTime:{0}", m_fpsTime);

            m_fpsTime = 0.0;
            m_fpsFrameIndex = 0;
        }

        m_frameIndex++;
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

void MRenderApplication::initUIRenderer()
{
    m_uiRenderer = std::make_shared<UIRenderer>();
    //m_uiRenderer->m_app = this;
    //Singleton<MVulkanEngine>::instance().InitUIRenderer(m_uiRenderer);
}

void MRenderApplication::init()
{
    Singleton<MVulkanEngine>::instance().SetWindowRes(WIDTH, HEIGHT);
    Singleton<MVulkanEngine>::instance().Init();

    initUIRenderer();
    Singleton<MVulkanEngine>::instance().InitUIRenderer(m_uiRenderer, this);

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

    Singleton<MVulkanEngine>::instance().RenderUI(m_uiRenderer, imageIndex, m_currentFrame);
    
    std::function<void()> recreateSwapchain = [this]() {
        this->RecreateSwapchainAndRenderPasses();
        };
    Singleton<MVulkanEngine>::instance().Present(m_currentFrame, &imageIndex, recreateSwapchain);

    m_currentFrame = (m_currentFrame + 1) % Singleton<GlobalConfig>::instance().GetMaxFramesInFlight();
}