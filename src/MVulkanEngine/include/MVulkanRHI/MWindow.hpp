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
        return m_window;
    }

    bool WindowShouldClose();
    void WindowPollEvents();
    inline glm::vec2 GetMousePosition() {
        double mousePosX, mousePosY;
        glfwGetCursorPos(m_window, &mousePosX, &mousePosY);
        return glm::vec2(mousePosX, mousePosY);
    }
    inline bool GetWindowResized() const { return m_windowResized; }
    inline void SetWindowResized(bool flag) { m_windowResized = flag; }
private:
    bool        m_windowResized = false;
    GLFWwindow* m_window = nullptr;
};


#endif