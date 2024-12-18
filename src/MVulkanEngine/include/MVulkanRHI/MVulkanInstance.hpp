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
    void Clean();
    void SetupDebugMessenger();
    inline VkInstance GetInstance()const { return m_instance; }

private:
    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debugMessenger;

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    std::vector<const char*> getRequiredExtensions();

    bool checkValidationLayerSupport();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
};



#endif