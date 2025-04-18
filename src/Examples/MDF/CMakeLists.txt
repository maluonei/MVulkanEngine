cmake_minimum_required(VERSION 3.10)

project(MVulkanEngine_MDF)

# ָ�� C++ ��׼
set(CMAKE_CXX_STANDARD 20)


add_executable(MVulkanEngine_MDF "main.cpp" "mdf.hpp" "mdf.cpp")

#message(STATUS "MVulkanEngine_include: ${MVulkanEngine_include}")
# ���Ӱ���Ŀ¼
target_include_directories(MVulkanEngine_MDF PRIVATE ${MVulkanEngine_include})

# ���ÿ��·�������� Project1 �����Ŀ¼�� ../project1/build��
target_link_libraries(MVulkanEngine_MDF PRIVATE MVulkanEngine)

#set(GLFW_DLL_PATH "${CMAKE_BINARY_DIR}/3rdParty/glfw/src/$<CONFIG>/$<IF:$<CONFIG:Debug>,glfw3d.dll,glfw3.dll>")
#set(ASSIMP_DLL_PATH "${CMAKE_BINARY_DIR}/3rdParty/assimp/bin/$<CONFIG>/$<IF:$<CONFIG:Debug>,assimp-vc143-mtd.dll,assimp-vc143-mt.dll>")
#set(EMBREE4_DLL_PATH "${CMAKE_BINARY_DIR}/3rdParty/embree/$<CONFIG>/$<IF:$<CONFIG:Debug>,embree4d.dll,embree4.dll>")
#set(TBB_DLL_PATH "${CMAKE_BINARY_DIR}/3rdParty/embree/$<CONFIG>/$<IF:$<CONFIG:Debug>,tbb12_debug.dll,tbb12.dll>")
#
#
#add_custom_command(TARGET MVulkanEngine_MDF POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_if_different
#    ${GLFW_DLL_PATH} $<TARGET_FILE_DIR:MVulkanEngine_MDF>)
#
#add_custom_command(TARGET MVulkanEngine_MDF POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_if_different
#    ${ASSIMP_DLL_PATH} $<TARGET_FILE_DIR:MVulkanEngine_MDF>)
#
#add_custom_command(TARGET MVulkanEngine_MDF POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_if_different
#    ${EMBREE4_DLL_PATH} $<TARGET_FILE_DIR:MVulkanEngine_MDF>)
#
#add_custom_command(TARGET MVulkanEngine_MDF POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_if_different
#    ${TBB_DLL_PATH} $<TARGET_FILE_DIR:MVulkanEngine_MDF>)




copy_dlls(MVulkanEngine_MDF)

#function(copy_dlls target)
#    get_target_property(deps ${target} LINK_LIBRARIES)
#    foreach(lib ${deps})
#        if(TARGET ${lib})
#            get_target_property(dll_path ${lib} IMPORTED_LOCATION_RELEASE)
#            if(dll_path AND EXISTS "${dll_path}")
#                add_custom_command(TARGET ${target} POST_BUILD
#                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
#                        "${dll_path}"
#                        $<TARGET_FILE_DIR:${target}>
#                )
#            endif()
#        endif()
#    endforeach()
#endfunction()
#
#copy_dlls(MVulkanEngine_MDF)

#find_package(TBB REQUIRED COMPONENTS tbb tbbmalloc)

## 检查调试版
#if(TARGET TBB::tbb_debug)
#    message(STATUS "Found TBB debug version")
#    target_link_libraries(YourTarget PRIVATE 
#        $<$<CONFIG:Debug>:TBB::tbb_debug>
#        $<$<CONFIG:Release>:TBB::tbb>
#    )
#else()
#    target_link_libraries(YourTarget PRIVATE TBB::tbb)
#endif()
#