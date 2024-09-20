#pragma once
#ifndef MWINDOW_H
#define MWINDOW_H

#include "Utils/VulkanUtil.h"

#include <cstdint>

class GLFWwindow;

class MWindow {
public:
    MWindow();
    ~MWindow();

    void Init(uint16_t width, uint16_t height);
    void Clean();
    inline GLFWwindow* GetWindow()const {
        return window;
    }

    bool WindowShouldClose();
    void WindowPollEvents();
private:

    GLFWwindow* window = nullptr;
};


#endif