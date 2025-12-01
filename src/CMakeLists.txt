cmake_minimum_required(VERSION 3.14.0)

project(ad_dcl)
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

add_library(ad_3rdparty_mtar SHARED ${CMAKE_SOURCE_DIR}/3rdparty/microtar/microtar.c)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/
        ${CMAKE_CURRENT_SOURCE_DIR}/navigation_planner
)

add_subdirectory(channel)
add_subdirectory(codec)
add_subdirectory(common)
add_subdirectory(recorder)
add_subdirectory(trigger_engine)
add_subdirectory(uploader)
add_subdirectory(navigation_planner)
add_subdirectory(state_machine)

add_executable(ad_dcl 
               ./main.cpp
               ./data_collection_planner.cpp
)

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