set(target_name MVulkanEngine)

set(cmakeconfig_dir ${PROJECT_SOURCE_DIR}/cmakeconfig/)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${cmakeconfig_dir}")
#include(${cmakeconfig_dir}/configure_optix.cmake)

#mark_as_advanced(CUDA_SDK_ROOT_DIR)

#add_subdirectory("mylib")

#find_package(Vulkan REQUIRED)
message(STATUS "Vulkan_INCLUDE_DIRS: ${vulkan_include}")
message(STATUS "Vulkan_LIBRARIES: ${vulkan_lib}")

#find_package(glfw REQUIRED)

file(GLOB_RECURSE MVULKANENGINE_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
file(GLOB_RECURSE MVULKANENGINE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
#file(GLOB_RECURSE MVULKANENGINE_SHAEDRS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/*.glsl)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${MVULKANENGINE_HEADERS} ${MVULKANENGINE_SOURCES})
#source_group("resources\\shaders" FILES ${MVULKANENGINE_SHAEDRS})

# 添加可执行文件
add_executable(MVulkanEngine ${MVULKANENGINE_HEADERS} ${MVULKANENGINE_SOURCES})


set(RESOURCE_PATH "${CMAKE_SOURCE_DIR}/resources")
message(${CMAKE_SOURCE_DIR})
target_compile_options(MVulkanEngine PRIVATE -DRESOURCE_PATH="${RESOURCE_PATH}")

target_compile_definitions(MVulkanEngine PRIVATE GLFW_STATIC)
target_link_libraries(MVulkanEngine glfw ${vulkan_lib} assimp spirv-cross-core spirv-cross-glsl spirv-cross-hlsl spirv-cross-reflect)


# 添加包含目录
target_include_directories(MVulkanEngine PRIVATE 
												"include" 
												${vulkan_include}
												${glm_include}
												${glfw_include}
												${assimp_include}
												${stbimage_include}
												${spdlog_include}
												${spirv_cross_include}
)

set(GLFW_DLL_PATH "${CMAKE_BINARY_DIR}/3rdParty/glfw/src/$<CONFIG>/$<IF:$<CONFIG:Debug>,glfw3d.dll,glfw3.dll>")

add_custom_command(TARGET MVulkanEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${GLFW_DLL_PATH} $<TARGET_FILE_DIR:MVulkanEngine>)