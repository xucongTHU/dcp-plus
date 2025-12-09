# file(GLOB_RECURSE SOURCES 
#     "*.cc" 
#     "*.cpp" 
#     "../3rdparty/microtar/microtar.c"
# )

file(GLOB_RECURSE SOURCES 
    "navigation_planner/*.cc" 
    "navigation_planner/*.cpp"
    "common/*.cc"
    "common/*.cpp"
    "data_collection_planner.cpp"
)
list(FILTER SOURCES EXCLUDE REGEX "main.cpp")
list(FILTER SOURCES EXCLUDE REGEX ".*test.*\\.cpp")
list(FILTER SOURCES EXCLUDE REGEX ".*_test\\.cpp")

# 打印源文件列表用于调试
foreach(f IN LISTS SOURCES)
    message(STATUS "Source file: ${f}")
endforeach()

# 创建共享库
add_library(${PROJECT_NAME} SHARED
    ${SOURCES}
)

# 设置应用程序名称
set(APP "dcl_out")

# 创建可执行文件
add_executable(${APP} main.cpp) 

# 链接库文件
target_link_libraries(${APP} PUBLIC
    ${PROJECT_NAME}
)


# 如果启用了ONNX Runtime，则链接相关库
if(HAVE_ONNXRUNTIME)
    target_compile_definitions(${APP} PRIVATE HAVE_ONNXRUNTIME)
    target_include_directories(${APP} PRIVATE ${ONNXRUNTIME_PATH}/include)
    target_link_libraries(${APP} PRIVATE ${ONNXRUNTIME_PATH}/lib/libonnxruntime.so)
    
    # 复制ONNX Runtime库文件到构建目录以便运行时链接
    file(GLOB ONNXRUNTIME_LIBS "${ONNXRUNTIME_PATH}/lib/*.so")
    file(COPY ${ONNXRUNTIME_LIBS} DESTINATION ${CMAKE_BINARY_DIR})
endif()

# 如果启用了ROS2，则链接相关库
if(ENABLE_ROS2)
    target_link_libraries(${APP} 
        PRIVATE 
        rclcpp::rclcpp
        sensor_msgs::sensor_msgs
        nav_msgs::nav_msgs
        geometry_msgs::geometry_msgs
    )
    
    # 定义宏以启用ROS2代码
    target_compile_definitions(${APP} PRIVATE ENABLE_ROS2)
endif()

# 链接yaml-cpp相关库
target_include_directories(${APP} PRIVATE ${YAMLCPP_PATH}/include)
target_link_libraries(${APP} PRIVATE ${YAMLCPP_PATH}/lib/libyaml-cpp.so)

# 复制yaml-cpp库文件到构建目录以便运行时链接
file(GLOB YAMLCPP_LIBS "${YAMLCPP_PATH}/lib/*.so")
file(COPY ${YAMLCPP_LIBS} DESTINATION ${CMAKE_BINARY_DIR})

# # 包含目录
# target_include_directories(${APP} 
#     PRIVATE 
#     ${CMAKE_CURRENT_SOURCE_DIR}
#     ${CMAKE_CURRENT_SOURCE_DIR}/codec/gpujpeg/include
#     ${CMAKE_CURRENT_SOURCE_DIR}/codec/img_encoder
# )

# target_include_directories(${PROJECT_NAME}
#     PUBLIC
#     ${CMAKE_CURRENT_SOURCE_DIR}
#     ${CMAKE_CURRENT_SOURCE_DIR}/codec/gpujpeg/include
#     ${CMAKE_CURRENT_SOURCE_DIR}/codec/img_encoder
# )

# 安装目标
install(TARGETS ${APP}
    RUNTIME DESTINATION bin
)