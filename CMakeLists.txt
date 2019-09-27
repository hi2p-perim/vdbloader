#
#   vdbloader - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

cmake_minimum_required(VERSION 3.10)

# Enable policy to manage VERSION in project() command
cmake_policy(SET CMP0048 NEW)
project(vdbloader VERSION 1.0.0)

# Check if the directory is added via add_subdirectory
set(VDBLOADER_MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(VDBLOADER_MASTER_PROJECT ON)
endif()

# Build options
option(VDBLOADER_INSTALL "Enable install" ${VDBLOADER_MASTER_PROJECT})

# For CMAKE_INSTALL_*
include(GNUInstallDirs)

# -------------------------------------------------------------------------------------------------

# If CONDA_PREFIX is not given from the command line,
# try to find it from the environment variable.
if (NOT CONDA_PREFIX)
    # Normalize the path separator
    file(TO_CMAKE_PATH "$ENV{CONDA_PREFIX}" CONDA_PREFIX)
endif()

# Conda prefix directory
if (MSVC)
    set(_CONDA_LIB_PREFIX "${CONDA_PREFIX}/Library")
else()
    set(_CONDA_LIB_PREFIX "${CONDA_PREFIX}")
endif()

# Set module directory for find_package (incl. hints for transitive dependencies)
list(APPEND CMAKE_MODULE_PATH "${_CONDA_LIB_PREFIX}/lib/cmake/OpenVDB")
set(ILMBASE_ROOT "${_CONDA_LIB_PREFIX}")
set(OPENEXR_ROOT "${_CONDA_LIB_PREFIX}")
set(BLOSC_ROOT "${_CONDA_LIB_PREFIX}")

# External dependencies
find_package(OpenVDB REQUIRED)
find_package(IlmBase REQUIRED)
find_package(OpenEXR REQUIRED)

# -------------------------------------------------------------------------------------------------

# Create project
set(_PROJECT_NAME vdbloader)
add_library(${_PROJECT_NAME} SHARED "vdbloader.h" "vdbloader.cpp")
add_library(vdbloader::vdbloader ALIAS ${_PROJECT_NAME})
target_link_libraries(${_PROJECT_NAME} PRIVATE 
    OpenVDB::openvdb
    OpenEXR::IlmImf
    IlmBase::IlmThread
    IlmBase::Iex)
target_include_directories(${_PROJECT_NAME} PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:include/vdbloader>")
target_compile_definitions(${_PROJECT_NAME} PRIVATE -DVDBLOADER_EXPORTS)
target_compile_definitions(${_PROJECT_NAME} PRIVATE -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
target_compile_definitions(${_PROJECT_NAME} PRIVATE -D_CRT_SECURE_NO_WARNINGS)
target_compile_features(${_PROJECT_NAME} PRIVATE cxx_std_17)
target_compile_options(${_PROJECT_NAME} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /EHa>)
set_target_properties(${_PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${_PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "-debug")
set_target_properties(${_PROJECT_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if (VDBLOADER_INSTALL)
    install(
        TARGETS ${_PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# -------------------------------------------------------------------------------------------------

# Install
if (VDBLOADER_INSTALL)
    # Install include directory
    install(
        FILES "vdbloader.h"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/vdbloader"
    )

    # Package configuration directory
    set(_PACKAGE_CONFIG_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

    # Make package configuration files
    # Use configure_package_config_file to make the package relocatable
    include(CMakePackageConfigHelpers)
    # Generate *Config.cmake file
    configure_package_config_file(
        "${CMAKE_CURRENT_LIST_DIR}/${PROJECT_NAME}Config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        INSTALL_DESTINATION ${_PACKAGE_CONFIG_DIR}
    )
    # Generate *ConfigVersion.cmake file
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
    )
    # Install generated configuration files
    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION ${_PACKAGE_CONFIG_DIR}
    )

    # Export targets
    # Make export target group
    install(
        EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${_PACKAGE_CONFIG_DIR}
    )
    # Generate *Target.cmake file
    export(
        EXPORT ${PROJECT_NAME}Targets
        FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake
        NAMESPACE ${PROJECT_NAME}::
    )
endif()