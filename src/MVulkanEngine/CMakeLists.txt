set(target_name MVulkanEngine)

set(cmakeconfig_dir ${PROJECT_SOURCE_DIR}/cmakeconfig/)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${cmakeconfig_dir}")


file(GLOB_RECURSE MVULKANENGINE_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
file(GLOB_RECURSE MVULKANENGINE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${MVULKANENGINE_HEADERS} ${MVULKANENGINE_SOURCES})

# ���ӿ�ִ���ļ�
#add_executable(MVulkanEngine 
#	${MVULKANENGINE_HEADERS} 
#	${MVULKANENGINE_SOURCES}
#	${stbimage_src}
#	"source/Utils/MImage.cpp")
add_library(
	MVulkanEngine STATIC 
	${MVULKANENGINE_HEADERS} 
	${MVULKANENGINE_SOURCES}
	${stbimage_src}
	"source/Utils/MImage.cpp"
)


set(RESOURCE_PATH "${CMAKE_SOURCE_DIR}/resources")
message(${CMAKE_SOURCE_DIR})
target_compile_options(MVulkanEngine PRIVATE -DRESOURCE_PATH="${RESOURCE_PATH}")

target_compile_definitions(MVulkanEngine PUBLIC GLFW_STATIC)
target_link_libraries(MVulkanEngine PUBLIC glfw ${vulkan_lib} assimp spirv-cross-core spirv-cross-glsl spirv-cross-hlsl spirv-cross-reflect meshoptimizer embree imgui)

if (MSVC)
    add_compile_options(/MP)  # 启用 MSVC 多线程编译
endif()

# ���Ӱ���Ŀ¼
target_include_directories(MVulkanEngine PUBLIC 
										"include" 
										${vulkan_include}
										${glm_include}
										${glfw_include}
										${assimp_include}
										${stbimage_include}
										${spdlog_include}
										${spirv_cross_include}
										${json_include}
										${meshoptimizer_include}
										${tinyobjloader_include}
										${embree_include}
										${imgui_include}
)

set(GLFW_DLL_PATH "${CMAKE_BINARY_DIR}/3rdParty/glfw/src/$<CONFIG>/$<IF:$<CONFIG:Debug>,glfw3d.dll,glfw3.dll>")
set(ASSIMP_DLL_PATH "${CMAKE_BINARY_DIR}/3rdParty/assimp/bin/$<CONFIG>/$<IF:$<CONFIG:Debug>,assimp-vc143-mtd.dll,assimp-vc143-mt.dll>")
set(EMBREE4_DLL_PATH "${CMAKE_BINARY_DIR}/3rdParty/embree/$<CONFIG>/$<IF:$<CONFIG:Debug>,embree4d.dll,embree4.dll>")
message(${CMAKE_BINARY_DIR})

add_custom_command(TARGET MVulkanEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${GLFW_DLL_PATH} $<TARGET_FILE_DIR:MVulkanEngine>)

add_custom_command(TARGET MVulkanEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${ASSIMP_DLL_PATH} $<TARGET_FILE_DIR:MVulkanEngine>)

add_custom_command(TARGET MVulkanEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${EMBREE4_DLL_PATH} $<TARGET_FILE_DIR:MVulkanEngine>)
