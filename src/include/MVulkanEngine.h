#pragma once
#include <cstdint>
#include <vulkan\vulkan_core.h>
#include "MVulkanInstance.h"
#include "MVulkanDevice.h"
#ifndef MVULKANENGINE_H
#define MVULKANENGINE_H

class MWindow;

class MVulkanEngine {
public:
    MVulkanEngine();
    ~MVulkanEngine();

    void Init();
    void Clean();

    void Run();
    void SetWindowRes(uint16_t _windowWidth, uint16_t _windowHeight);
private:
    void initVulkan();
    void renderLoop();

    void createInstance();
    void createDevice();

    uint16_t windowWidth = 800, windowHeight = 600;

    MWindow* window = nullptr;

    bool enableValidationLayer;
    
    MVulkanInstance instance;
    MVulkanDevice device;
    //VkInstance instance;
};

#endif