#pragma once
#ifndef MVULKANINSTANCE_H
#define MVULKANINSTANCE_H

#include "vulkan/vulkan_core.h"
#include "vector"

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

class MVulkanInstance {
public:
    MVulkanInstance();

    void Create();
    void SetupDebugMessenger();
    inline VkInstance GetInstance()const { return instance; }

private:
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    std::vector<const char*> getRequiredExtensions();

    bool checkValidationLayerSupport();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
};



#endif