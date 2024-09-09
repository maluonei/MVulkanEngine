cmake_minimum_required(VERSION 3.10)
project(MVulkanEngine LANGUAGES CXX)

set(third_party_dir "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty")
set(assimp_include "${third_party_dir}/assimp/include")
set(glm_include "${third_party_dir}/glm")
set(glfw_include "${third_party_dir}/glfw/include")
set(glad_include "${third_party_dir}/glad/include")
set(spdlog_include "${third_party_dir}/spdlog/include")
set(stbimage_include "${third_party_dir}/stb_image")

# 设置编译选项
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 设置子文件夹
add_subdirectory("3rdParty")
add_subdirectory("src")

#set(cmakeconfig_dir ${PROJECT_SOURCE_DIR}/cmakeconfig/)
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${cmakeconfig_dir}")
#include(${cmakeconfig_dir}/configure_optix.cmake)
#
#mark_as_advanced(CUDA_SDK_ROOT_DIR)

