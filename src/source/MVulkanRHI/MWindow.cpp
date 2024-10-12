#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "MVulkanRHI/MWindow.hpp"
//#include "Utils/VulkanUtil.hpp"

MWindow::MWindow()
{
}

MWindow::~MWindow()
{

}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<MWindow*>(glfwGetWindowUserPointer(window));
    app->SetWindowResized(true);
    spdlog::info("change window size");
}

void MWindow::Init(uint16_t width, uint16_t height)
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void MWindow::Clean()
{
    glfwDestroyWindow(window);

    glfwTerminate();
}

bool MWindow::WindowShouldClose()
{
    return glfwWindowShouldClose(window);
}

void MWindow::WindowPollEvents()
{
    glfwPollEvents();
}
