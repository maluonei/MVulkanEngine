set(third_party_folder "${CMAKE_CURRENT_SOURCE_DIR}")

include(vulkan.cmake)

#add_subdirectory(glslang)
add_subdirectory(SPIRV-Cross)
add_subdirectory(assimp)
add_subdirectory(glm)
add_subdirectory(glfw)
add_subdirectory(spdlog)