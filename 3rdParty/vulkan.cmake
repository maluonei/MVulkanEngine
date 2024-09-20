# #Vulkan SDK
if(WIN32)
    set(vulkan_lib "${third_party_folder}/VulkanSDK/lib/Windows/vulkan-1.lib" PARENT_SCOPE)
    message("A")
elseif(LINUX)
    set(vulkan_lib "${third_party_folder}/VulkanSDK/lib/Linux/libvulkan.so.1" PARENT_SCOPE)
    add_compile_definitions("VK_LAYER_PATH=${third_party_folder}/VulkanSDK/binary/Linux")
    #https://chromium.googlesource.com/external/github.com/KhronosGroup/Vulkan-Loader/+/HEAD/loader/LoaderAndLayerInterface.md#icd-discovery
    add_compile_definitions("VK_ICD_FILENAMES=${third_party_folder}/VulkanSDK/binary/Linux/vulkan_icd.json")
    message("B")
elseif(APPLE)
    set(vulkan_lib "${third_party_folder}/VulkanSDK/lib/MacOS/libvulkan.1.dylib" PARENT_SCOPE)
    add_compile_definitions("VK_LAYER_PATH=${third_party_folder}/VulkanSDK/binary/MacOS")
    #https://chromium.googlesource.com/external/github.com/KhronosGroup/Vulkan-Loader/+/HEAD/loader/LoaderAndLayerInterface.md#icd-discovery
    add_compile_definitions("VK_ICD_FILENAMES=${third_party_folder}/VulkanSDK/binary/MacOS/MoltenVK_icd.json")
    message("C")
else()
endif()
