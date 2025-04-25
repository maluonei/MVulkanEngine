cmake_minimum_required(VERSION 3.10)
project(MVulkanEngine LANGUAGES CXX)

# 设置编译选项
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_definitions(-DPROJECT_ROOT="${PROJECT_SOURCE_DIR}")

# 设置子文件夹
add_subdirectory("3rdParty")

set(third_party_dir "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty")
set(assimp_include "${third_party_dir}/assimp/include")
set(glm_include "${third_party_dir}/glm")
set(glfw_include "${third_party_dir}/glfw/include")
set(glad_include "${third_party_dir}/glad/include")
set(spdlog_include "${third_party_dir}/spdlog/include")
set(stbimage_include "${third_party_dir}/stb_image")
set(vulkan_include "${third_party_dir}/VulkanSDK/include")
set(spirv_cross_include "${third_party_dir}/SPIRV-CROSS/include")
set(json_include "${third_party_dir}/json")
set(meshoptimizer_include "${third_party_dir}/meshoptimizer/src")
set(tinyobjloader_include "${third_party_dir}/tinyobjloader")
set(embree_include "${third_party_dir}/embree/include/embree4")
set(imgui_include "${third_party_dir}/imgui")
set(imgui_backends_include "${third_party_dir}/imgui/backends")

set(imgui_src 
    "${third_party_dir}/imgui/imgui.h"
    "${third_party_dir}/imgui/imgui_internal.h"
    "${third_party_dir}/imgui/imgui.cpp"
    "${third_party_dir}/imgui/imgui_demo.cpp"
    "${third_party_dir}/imgui/imgui_draw.cpp"
    "${third_party_dir}/imgui/imgui_tables.cpp"
    "${third_party_dir}/imgui/imgui_widgets.cpp"
)

set(imgui_backends
    "${third_party_dir}/imgui/backends/imgui_impl_glfw.cpp"
    "${third_party_dir}/imgui/backends/imgui_impl_vulkan.cpp"
)

set(stbimage_src "${third_party_dir}/stb_image/stb_image.h" "${third_party_dir}/stb_image/stb_image.cpp")
#set(shaderc_include "${shaderc_SOURCE_DIR}/libshaderc/include"
#					"${shaderc_SOURCE_DIR}/third_party/glslang"
#					"${shaderc_SOURCE_DIR}/third_party/spirc-tools/include")


add_subdirectory("src")

#file(COPY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/3rdParty/VulkanSDK/binary/Windows/glslangValidator.exe DESTINATION ${CMAKE_BINARY_DIR}/ShaderCompilers)
file(COPY ${CMAKE_SOURCE_DIR}/3rdParty/dxc/dxc.exe DESTINATION ${CMAKE_BINARY_DIR}/ShaderCompilers)
file(COPY ${CMAKE_SOURCE_DIR}/3rdParty/dxc/dxcompiler.dll DESTINATION ${CMAKE_BINARY_DIR}/ShaderCompilers)
file(COPY ${CMAKE_SOURCE_DIR}/3rdParty/dxc/dxil.dll DESTINATION ${CMAKE_BINARY_DIR}/ShaderCompilers)
#set(cmakeconfig_dir ${PROJECT_SOURCE_DIR}/cmakeconfig/)
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${cmakeconfig_dir}")
#include(${cmakeconfig_dir}/configure_optix.cmake)
#
#mark_as_advanced(CUDA_SDK_ROOT_DIR)

