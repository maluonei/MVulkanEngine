cmake_minimum_required(VERSION 3.10)

project(MVulkanEngine_SSR)

# ָ�� C++ ��׼
set(CMAKE_CXX_STANDARD 20)

add_executable(MVulkanEngine_SSR "main.cpp" "ssr.hpp" "ssr.cpp")

#message(STATUS "MVulkanEngine_include: ${MVulkanEngine_include}")
# ���Ӱ���Ŀ¼
target_include_directories(MVulkanEngine_SSR PRIVATE ${MVulkanEngine_include})

# ���ÿ��·�������� Project1 �����Ŀ¼�� ../project1/build��
target_link_libraries(MVulkanEngine_SSR PRIVATE MVulkanEngine)

copy_dlls(MVulkanEngine_SSR)