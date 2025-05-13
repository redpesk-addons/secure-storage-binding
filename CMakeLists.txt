cmake_minimum_required(VERSION 3.10)

project(secure-storage-binding
    VERSION 2.0.0
    DESCRIPTION "Binding for secure key-value storage using BerkeleyDB"
    LANGUAGES C
)

# Set module path so CMake finds FindBerkeleyDB.cmake
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}")

# Declare install options
set(AFM_APP_DIR ${CMAKE_INSTALL_PREFIX}/redpesk CACHE PATH "Applications directory")
set(APP_DIR ${AFM_APP_DIR}/${PROJECT_NAME})

# Dependencies
include(FindPkgConfig)
pkg_check_modules(deps REQUIRED
    afb-binding>=4.1.0
    afb-helpers4
    json-c
    libsystemd
)

# Find BerkeleyDB (from FindBerkeleyDB.cmake)
find_package(BerkeleyDB REQUIRED)

# Build binding
add_library(secure-storage-binding SHARED
    secstorage/secure-storage-service-binding.c
)

set_target_properties(secure-storage-binding PROPERTIES PREFIX "")
target_include_directories(secure-storage-binding PRIVATE
    ${deps_INCLUDE_DIRS}
    ${BerkeleyDB_INCLUDE_DIR}
)

target_link_libraries(secure-storage-binding
    ${deps_LDFLAGS}
    ${DB_LIBRARY}
)

option(SECSTOREADMIN "Enter Secure Storage Admin mode" ON)
if (SECSTOREADMIN)
  add_definitions(-DSECSTOREADMIN=1)
endif()

option(ALLOW_SECS_GLOBAL "Enable Secure Storage global access mode" ON)
if (ALLOW_SECS_GLOBAL)
  add_definitions(-DALLOW_SECS_GLOBAL=1)
endif()


# Version script for symbol visibility
pkg_get_variable(vscript afb-binding version_script)
if(vscript)
    target_link_options(secure-storage-binding PRIVATE -Wl,--version-script=${vscript})
endif()

# Installation
install(TARGETS secure-storage-binding DESTINATION ${APP_DIR}/lib)
install(FILES manifest.yml DESTINATION ${APP_DIR}/.rpconfig)
install(PROGRAMS ${CMAKE_SOURCE_DIR}/redtest/run-redtest
	DESTINATION /usr/libexec/redtest/${PROJECT_NAME}/)
install(FILES ${CMAKE_SOURCE_DIR}/test/py-test.py
    DESTINATION /usr/libexec/redtest/${PROJECT_NAME}/)

