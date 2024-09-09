#pragma once

#include <cstdint>
#ifndef MWINDOW_H
#define MWINDOW_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class MWindow {
public:
    MWindow();
    ~MWindow();

    void Init(uint16_t width, uint16_t height);
    void Clean();

    bool WindowShouldClose();
    void WindowPollEvents();
private:

    GLFWwindow* window = nullptr;
};


#endif