set(BUILDTOOLS_TARGET_PLATFORM "thor")
find_package(ADBuildTools REQUIRED)

#######################
#    Project Config
#######################

set(CMAKE_INSTALL_PREFIX "/opt/senseauto/tmp/senseauto_data_closed_loop")
set(CMAKE_INSTALL_RPATH ".${CMAKE_INSTALL_PREFIX}/lib")

# platform&compiler related feature
set(AD_CROSS_COMPILE ON)
set(PLATFORM_NAME "aarch64_thor")
set(CXXSTD_FSLIB "stdc++fs")
set(BUILD_CYBER ON)

# 3rd related feature
set(NV_JETSON_FEATURE OFF)
set(OPENCV_ENABLED OFF)

# python related
set(AD_PYTHON_SUPPORT OFF)
# set(AD_PYTHON_VERSION "aarch64-linux-gnu/python3.6m")
