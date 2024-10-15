#pragma once
#ifndef MWINDOW_H
#define MWINDOW_H

#include "Utils/VulkanUtil.hpp"

#include <cstdint>

struct GLFWwindow;

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
    inline bool GetWindowResized() const { return windowResized; }
    inline void SetWindowResized(bool flag) { windowResized = flag; }
private:
    bool windowResized = false;
    GLFWwindow* window = nullptr;
};


#endif