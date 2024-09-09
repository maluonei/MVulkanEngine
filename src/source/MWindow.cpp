#include "MWindow.h"

MWindow::MWindow()
{
}

MWindow::~MWindow()
{

}

void MWindow::Init(uint16_t width, uint16_t height)
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
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
