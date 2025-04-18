cmake_minimum_required(VERSION 3.10)

project(MVulkanEngine_DR)

# ָ�� C++ ��׼
set(CMAKE_CXX_STANDARD 20)


add_executable(MVulkanEngine_DR "main.cpp" "pbr.hpp" "pbr.cpp")

#message(STATUS "MVulkanEngine_include: ${MVulkanEngine_include}")
# ���Ӱ���Ŀ¼
target_include_directories(MVulkanEngine_DR PRIVATE ${MVulkanEngine_include})

# ���ÿ��·�������� Project1 �����Ŀ¼�� ../project1/build��
target_link_libraries(MVulkanEngine_DR PRIVATE MVulkanEngine)

#set(GLFW_DLL_PATH "${CMAKE_BINARY_DIR}/3rdParty/glfw/src/$<CONFIG>/$<IF:$<CONFIG:Debug>,glfw3d.dll,glfw3.dll>")
#set(ASSIMP_DLL_PATH "${CMAKE_BINARY_DIR}/3rdParty/assimp/bin/$<CONFIG>/$<IF:$<CONFIG:Debug>,assimp-vc143-mtd.dll,assimp-vc143-mt.dll>")
#
#add_custom_command(TARGET MVulkanEngine_DR POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_if_different
#    ${GLFW_DLL_PATH} $<TARGET_FILE_DIR:MVulkanEngine_DR>)
#
#add_custom_command(TARGET MVulkanEngine_DR POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_if_different
#    ${ASSIMP_DLL_PATH} $<TARGET_FILE_DIR:MVulkanEngine_DR>)
copy_dlls(MVulkanEngine_DR)
