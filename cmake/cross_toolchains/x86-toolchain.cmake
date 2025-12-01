set(BUILDTOOLS_TARGET_PLATFORM "x86_64")
find_package(ADBuildTools REQUIRED)

#######################
#    Project Config
#######################

set(CMAKE_INSTALL_PREFIX "/opt/senseauto/tmp/senseauto_data_closed_loop/")
set(CMAKE_INSTALL_RPATH ".${CMAKE_INSTALL_PREFIX}/lib")

set(PLATFORM_NAME "x86_64_ub16")

#######################
#    Install Config
#######################
message("-- Building x86 deb...")
