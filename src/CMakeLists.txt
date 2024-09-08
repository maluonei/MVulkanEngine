set(target_name MVulkanEngine)

set(cmakeconfig_dir ${PROJECT_SOURCE_DIR}/cmakeconfig/)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${cmakeconfig_dir}")
#include(${cmakeconfig_dir}/configure_optix.cmake)

#mark_as_advanced(CUDA_SDK_ROOT_DIR)

#add_subdirectory("mylib")

file(GLOB_RECURSE MVULKANENGINE_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
file(GLOB_RECURSE MVULKANENGINE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${MVULKANENGINE_HEADERS} ${MVULKANENGINE_SOURCES})

#include_directories("${PROJECT_SOURCE_DIR}/mylib/include")
include_directories("${PROJECT_SOURCE_DIR}/include")

#cuda_compile_and_embed(embedded_ptx_code "source/TestDevicePrograms.cu")

# 添加可执行文件
add_executable(MVulkanEngine ${MVULKANENGINE_HEADERS} ${MVULKANENGINE_SOURCES})

#add_executable(OptixRayTracer src/main.cpp OPTIXRAYTRACER_SOURCES)
#target_link_libraries(OptixRayTracer PRIVATE mylib)
#target_link_libraries(OptixRayTracer PRIVATE ${OptiX_LIBRARIES})
#target_link_libraries(OptixRayTracer PRIVATE ${CUDA_LIBRARIES} ${CUDA_CUDA_LIBRARY})

# 添加包含目录
target_include_directories(MVulkanEngine PRIVATE "include")
target_include_directories(MVulkanEngine PRIVATE ${glm_include})
target_include_directories(MVulkanEngine PRIVATE ${glfw_include})
target_include_directories(MVulkanEngine PRIVATE ${assimp_include})
#target_include_directories(OptixRayTracer PRIVATE "mylib/include")
#target_include_directories(OptixRayTracer PRIVATE ${CUDA_INCLUDE})
#target_include_directories(OptixRayTracer PRIVATE ${OptiX_INCLUDE})
target_include_directories(MVulkanEngine PRIVATE ${stbimage_include})

#install (TARGETS OptixRayTracer DESTINATION bin)
#install (FILES "${PROJECT_BINARY_DIR}/include/test.h"
#         DESTINATION include)
