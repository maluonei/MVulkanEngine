#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "MVulkanRHI/MWindow.hpp"
#include "Managers/InputManager.hpp"
#include "spdlog/spdlog.h"
//#include "Utils/VulkanUtil.hpp"

#include <imgui.h>

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

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    //if (key == GLFW_KEY_E && action == GLFW_PRESS)
    if (action == GLFW_PRESS) {
        Singleton<InputManager>::instance().SetKey(key, true);
    }
    else if (action == GLFW_RELEASE) {
        Singleton<InputManager>::instance().SetKey(key, false);
    }
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{   
    //spdlog::info("xpos:" + std::to_string(xpos) + ", ypos:" + std::to_string(ypos));
    //if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
        Singleton<InputManager>::instance().SetMousePosition(glm::vec2(xpos, ypos));
    //}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_PRESS)
        Singleton<InputManager>::instance().SetMousePressed(button, true);
    else if (action == GLFW_RELEASE)
        Singleton<InputManager>::instance().SetMousePressed(button, false);
}

void cursor_enter_callback(GLFWwindow* window, int entered)
{
    if (entered)
    {
        Singleton<InputManager>::instance().SetCursorEnter(true);
        // The cursor entered the content area of the window
    }
    else
    {
        Singleton<InputManager>::instance().SetCursorLeave(true);
        // The cursor left the content area of the window
    }
}

void MWindow::Init(uint16_t width, uint16_t height)
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_window = glfwCreateWindow(width, height, "MVulkanEngine", nullptr, nullptr);

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
    
    glfwSetKeyCallback(m_window, key_callback);
    glfwSetCursorPosCallback(m_window, cursor_position_callback);
    glfwSetMouseButtonCallback(m_window, mouse_button_callback);
    glfwSetCursorEnterCallback(m_window, cursor_enter_callback);
}

void MWindow::Clean()
{
    glfwDestroyWindow(m_window);

    glfwTerminate();
}

bool MWindow::WindowShouldClose()
{
    return glfwWindowShouldClose(m_window);
}

void MWindow::WindowPollEvents()
{
    glfwPollEvents();
}
