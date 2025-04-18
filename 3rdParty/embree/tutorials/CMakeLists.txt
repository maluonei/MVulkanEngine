## Copyright 2009-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

include(CMakeDependentOption)

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/README.md.in" "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README.md" DESTINATION "src" COMPONENT examples)

ADD_SUBDIRECTORY(common/image)
CMAKE_DEPENDENT_OPTION(EMBREE_TUTORIALS_GLFW "Enables GLFW usage in tutorials. When disabled tutorial can only render to disk." ON "EMBREE_TUTORIALS" OFF)

IF (EMBREE_TUTORIALS)

#IF (EMBREE_TUTORIALS_GLFW)
#
#  # 修改1: 直接查找本地安装的GLFW
#  FIND_PACKAGE(glfw3 REQUIRED)
#  add_subdirectory
#  
#  IF(NOT glfw3_FOUND)
#    MESSAGE(FATAL_ERROR "GLFW not found! Please install GLFW locally or disable EMBREE_TUTORIALS_GLFW.")
#  ELSE()
#    MESSAGE(STATUS "Found locally installed GLFW: ${glfw3_DIR}")
#  ENDIF()
#
#  FIND_PACKAGE(OpenGL REQUIRED)
#  
#  ADD_DEFINITIONS(-DUSE_GLFW)
#
#  # 修改2: 直接使用找到的GLFW库
#  SET(GUI_LIBRARIES imgui glfw ${OPENGL_LIBRARIES})
#ENDIF()

ADD_SUBDIRECTORY(common)
ADD_SUBDIRECTORY(embree_info)

# 注释掉所有在线获取GLFW的代码
# 原代码中的FetchContent部分已全部移除

ENDIF()