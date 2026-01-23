cmake_minimum_required(VERSION 3.22)

project(aurora_edge)

# 编译器标志
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fext-numeric-literals -pipe -O2 -Wall -Wextra -fopenmp -fPIC -pthread")
add_compile_options(-w)

option(ENABLE_RSCL "Enable RSCL support" OFF)
option(ENABLE_ROS2 "Enable ROS2 support" ON)
option(DOWNLOAD_ONNXRUNTIME "Download ONNX Runtime if not found" ON)

find_package(Boost REQUIRED COMPONENTS filesystem system iostreams thread regex)
include_directories(${Boost_INCLUDE_DIRS})

include(cmake/Find3rdparty.cmake)

# ONNX Runtime支持
set(ONNXRUNTIME_PATH "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/onnxruntime")
set(CMAKE_PREFIX_PATH "${ONNXRUNTIME_PATH};${CMAKE_PREFIX_PATH}")

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
)

# 检查ONNX Runtime是否可用或需要下载
if(NOT EXISTS "${ONNXRUNTIME_PATH}" AND DOWNLOAD_ONNXRUNTIME)
    message(STATUS "ONNX Runtime not found. Downloading...")

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
    message(STATUS "Found ONNX Runtime: ${ONNXRUNTIME_PATH}")
endif()

if(ENABLE_ROS2)
    include_directories(
        /opt/ros/humble/include
        /opt/ros/humble/include/rclcpp
        /opt/ros/humble/include/rosbag2_cpp
        /opt/ros/humble/include/rosbag2_storage
        /opt/ros/humble/include/rosidl_typesupport_introspection_cpp
        /opt/ros/humble/include/libstatistics_collector
        /opt/ros/humble/include/rcl
        /opt/ros/humble/include/rcutils
        /opt/ros/humble/include/rcl_yaml_param_parser
        /opt/ros/humble/include/rosidl_runtime_c
        /opt/ros/humble/include/rosidl_typesupport_interface
        /opt/ros/humble/include/rcpputils
        /opt/ros/humble/include/builtin_interfaces
        /opt/ros/humble/include/rmw
        /opt/ros/humble/include/rosidl_runtime_cpp
        /opt/ros/humble/include/tracetools
        /opt/ros/humble/include/rcl_interfaces
        /opt/ros/humble/include/statistics_msgs
        /opt/ros/humble/include/std_msgs
    )

    link_directories(/opt/ros/humble/lib/)

    include_directories(./install/data_collection/include/data_collection/)
    link_directories(./install/data_collection/lib/)

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

set(CPACK_GENERATOR "TGZ")

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