add_subdirectory(MVulkanEngine)

set(MVulkanEngine_include "${CMAKE_CURRENT_SOURCE_DIR}/MVulkanEngine/include")
#set(MVulkanEngine_lib )

function(copy_dlls target)
    set(GLFW_DLL_PATH "${CMAKE_BINARY_DIR}/3rdParty/glfw/src/$<CONFIG>/$<IF:$<CONFIG:Debug>,glfw3d.dll,glfw3.dll>")
    set(ASSIMP_DLL_PATH "${CMAKE_BINARY_DIR}/3rdParty/assimp/bin/$<CONFIG>/$<IF:$<CONFIG:Debug>,assimp-vc143-mtd.dll,assimp-vc143-mt.dll>")
    set(EMBREE4_DLL_PATH "${CMAKE_BINARY_DIR}/3rdParty/embree/$<CONFIG>/$<IF:$<CONFIG:Debug>,embree4d.dll,embree4.dll>")
    set(TBB_DLL_PATH "${CMAKE_BINARY_DIR}/3rdParty/embree/$<CONFIG>/$<IF:$<CONFIG:Debug>,tbb12_debug.dll,tbb12.dll>")
    
    
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${GLFW_DLL_PATH} $<TARGET_FILE_DIR:${target}>)
    
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${ASSIMP_DLL_PATH} $<TARGET_FILE_DIR:${target}>)
    
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${EMBREE4_DLL_PATH} $<TARGET_FILE_DIR:${target}>)
    
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${TBB_DLL_PATH} $<TARGET_FILE_DIR:${target}>)

endfunction()

add_subdirectory(Examples)