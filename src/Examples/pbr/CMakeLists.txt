cmake_minimum_required(VERSION 3.10)

project(MVulkanEngine_PBR)

# 指定 C++ 标准
set(CMAKE_CXX_STANDARD 20)


add_executable(MVulkanEngine_PBR main.cpp pbr.hpp pbr.cpp "main.cpp")

#message(STATUS "MVulkanEngine_include: ${MVulkanEngine_include}")
# 添加包含目录
target_include_directories(MVulkanEngine_PBR PRIVATE ${MVulkanEngine_include})

# 设置库的路径（假设 Project1 的输出目录是 ../project1/build）
target_link_libraries(MVulkanEngine_PBR PRIVATE MVulkanEngine)

set(GLFW_DLL_PATH "${CMAKE_BINARY_DIR}/3rdParty/glfw/src/$<CONFIG>/$<IF:$<CONFIG:Debug>,glfw3d.dll,glfw3.dll>")
set(ASSIMP_DLL_PATH "${CMAKE_BINARY_DIR}/3rdParty/assimp/bin/$<CONFIG>/$<IF:$<CONFIG:Debug>,assimp-vc143-mtd.dll,assimp-vc143-mt.dll>")

add_custom_command(TARGET MVulkanEngine_PBR POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${GLFW_DLL_PATH} $<TARGET_FILE_DIR:MVulkanEngine_PBR>)

add_custom_command(TARGET MVulkanEngine_PBR POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${ASSIMP_DLL_PATH} $<TARGET_FILE_DIR:MVulkanEngine_PBR>)
