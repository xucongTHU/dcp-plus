cmake_minimum_required(VERSION 3.14.0)

project(ad_dcl)
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

add_library(ad_3rdparty_mtar SHARED ${CMAKE_SOURCE_DIR}/3rdparty/microtar/microtar.c)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/
        ${CMAKE_CURRENT_SOURCE_DIR}/data_collection
        ${CMAKE_CURRENT_SOURCE_DIR}/navigation_planner
)

add_subdirectory(data_collection)
add_subdirectory(navigation_planner)

add_executable(ad_dcl 
               ./main.cpp
               ./data_collection_planner.cpp)

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
