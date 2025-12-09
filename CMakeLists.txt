# 最低版本要求
cmake_minimum_required(VERSION 3.20)

# 项目名称和版本
project(ad_data_closed_loop VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 7)
        set(CMAKE_CXX_STANDARD 17)
    else()
        set(CMAKE_CXX_STANDARD 17)
    endif()
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译器标志
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3")

# 检查是否启用ROS2支持
option(ENABLE_ROS2 "Enable ROS2 support" OFF)
option(DOWNLOAD_ONNXRUNTIME "Download ONNX Runtime if not found" ON)
option(DOWNLOAD_YAMLCPP "Download yaml-cpp if not found" ON)

# 查找包
find_package(Threads REQUIRED)

# ONNX Runtime支持
set(ONNXRUNTIME_PATH "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/onnxruntime")
set(CMAKE_PREFIX_PATH "${ONNXRUNTIME_PATH};${CMAKE_PREFIX_PATH}")

# yaml-cpp支持
set(YAMLCPP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/yaml-cpp")

# include headers
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/exprtk
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/nlohmann
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/yaml-cpp/include
)

# 检查ONNX Runtime是否可用或需要下载
if(NOT EXISTS "${ONNXRUNTIME_PATH}" AND DOWNLOAD_ONNXRUNTIME)
    message(STATUS "ONNX Runtime not found. Downloading...")
    
    # 确定平台
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
            set(ONNXRUNTIME_URL "https://github.com/microsoft/onnxruntime/releases/download/v1.10.0/onnxruntime-linux-x64-1.10.0.tgz")
            set(ONNXRUNTIME_DIR "onnxruntime-linux-x64-1.10.0")
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
            set(ONNXRUNTIME_URL "https://github.com/microsoft/onnxruntime/releases/download/v1.10.0/onnxruntime-linux-aarch64-1.10.0.tgz")
            set(ONNXRUNTIME_DIR "onnxruntime-linux-aarch64-1.10.0")
        endif()
    endif()
    
    # 下载并解压ONNX Runtime
    if(ONNXRUNTIME_URL)
        include(FetchContent)
        FetchContent_Populate(
            onnxruntime
            URL ${ONNXRUNTIME_URL}
            SOURCE_DIR ${ONNXRUNTIME_PATH}
        )
        message(STATUS "ONNX Runtime downloaded to ${ONNXRUNTIME_PATH}")
    endif()
endif()

# 尝试查找ONNX Runtime
if(EXISTS "${ONNXRUNTIME_PATH}")
    set(HAVE_ONNXRUNTIME TRUE)
    message(STATUS "ONNX Runtime found: ${ONNXRUNTIME_PATH}")
endif()

# 检查yaml-cpp是否可用或需要下载
if(NOT EXISTS "${YAMLCPP_PATH}" AND DOWNLOAD_YAMLCPP)
    message(STATUS "yaml-cpp not found. Downloading...")
    
    # 下载并解压yaml-cpp
    include(FetchContent)
    FetchContent_Populate(
        yamlcpp
        URL https://github.com/jbeder/yaml-cpp/archive/refs/tags/yaml-cpp-0.7.0.tar.gz
        SOURCE_DIR ${YAMLCPP_PATH}
    )
    message(STATUS "yaml-cpp downloaded to ${YAMLCPP_PATH}")
endif()

# 尝试查找yaml-cpp
if(EXISTS "${YAMLCPP_PATH}")
    set(HAVE_YAMLCPP TRUE)
    message(STATUS "yaml-cpp found: ${YAMLCPP_PATH}")
endif()

# 条件性包含ROS2
if(ENABLE_ROS2)
    find_package(rclcpp REQUIRED)
    find_package(sensor_msgs REQUIRED)
    find_package(nav_msgs REQUIRED)
    find_package(geometry_msgs REQUIRED)
endif()

# 获取所有链接目录
get_property(link_dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY LINK_DIRECTORIES)

# 打印所有链接目录
foreach(dir IN LISTS link_dirs)
    message(STATUS "Link directory: ${dir}")
endforeach()

# 添加子目录
add_subdirectory(src)

#############
##  CPack  ##
#############

# 设置CPack变量
set(CPACK_GENERATOR "TGZ")
set(MODULE_VERSION "1.0.0")

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${MODULE_VERSION})
set(CPACK_PACKAGE_CONTACT "xucong@synaptix.ai")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
set(CPACK_SET_DESTDIR ON)
set(CPACK_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_PACKAGE_FILE_NAME "${MODULE_VERSION}")

install(
        FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/resource/tar_extra/release_helper.sh
        DESTINATION /
)

set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
include(CPack)