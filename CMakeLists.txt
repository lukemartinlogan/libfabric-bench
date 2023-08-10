cmake_minimum_required(VERSION 3.10)
project(FABRIC)

#-----------------------------------------------------------------------------
# Define Options
#-----------------------------------------------------------------------------
option(BUILD_SHARED_LIBS "Build shared libraries (.dll/.so) instead of static ones (.lib/.a)" ON)
option(BUILD_MPI_TESTS "Build tests which depend on MPI" ON)
option(BUILD_OpenMP_TESTS "Build tests which depend on OpenMP" ON)
option(FABRIC_ENABLE_COVERAGE "Check how well tests cover code" OFF)
option(FABRIC_ENABLE_DOXYGEN "Check how well the code is documented" OFF)

#-----------------------------------------------------------------------------
# Compiler Optimization
#-----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("IN DEBUG MODE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
    add_compile_definitions(HERMES_LOG_VERBOSITY=10)
else()
    message("IN RELEASE MODE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O3")
    add_compile_definitions(HERMES_LOG_VERBOSITY=1)
endif()
add_compile_options(-march=native -fomit-frame-pointer)

#-----------------------------------------------------------------------------
# Find Packages
#-----------------------------------------------------------------------------
# HermesShm
find_package(HermesShm CONFIG REQUIRED)
message(STATUS "found hermes_shm.h at ${HermesShm_INCLUDE_DIRS}")

# MPICH
if(BUILD_MPI_TESTS)
    find_package(MPI REQUIRED COMPONENTS C CXX)
    message(STATUS "found mpi.h at ${MPI_CXX_INCLUDE_DIRS}")
endif()

# OpenMP
if(BUILD_OpenMP_TESTS)
    find_package(OpenMP REQUIRED COMPONENTS C CXX)
    message(STATUS "found omp.h at ${OpenMP_CXX_INCLUDE_DIRS}")
endif()

# Pkg-Config
find_package(PkgConfig REQUIRED)
if(PkgConfig)
    message(STATUS "found pkg config")
endif()

# Thallium
find_package(thallium CONFIG REQUIRED)
if(thallium_FOUND)
    message(STATUS "found thallium at ${thallium_DIR}")
endif()

# Libfabric
find_package(PkgConfig REQUIRED)
pkg_search_module(libfabric REQUIRED libfabric)
message("LibFabric found at: ${libfabric_INCLUDE_DIRS} with libs ${libfabric_LIBRARIES} in dir ${libfabric_LIBRARY_DIRS}")
include_directories( ${libfabric_INCLUDE_DIRS})
link_directories( ${libfabric_LIBRARY_DIRS})

# YAML-CPP
find_package(yaml-cpp REQUIRED)
message(STATUS "found yaml-cpp at ${yaml-cpp_DIR}")

#------------------------------------------------------------------------------
# Setup CMake Environment
#------------------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
        ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all Executables.")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
        ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all Libraries")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
        ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all static libraries.")

#------------------------------------------------------------------------------
# Setup install and output Directories
#------------------------------------------------------------------------------
if(NOT FABRIC_INSTALL_BIN_DIR)
    set(FABRIC_INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX}/bin)
endif()
if(NOT FABRIC_INSTALL_LIB_DIR)
    set(FABRIC_INSTALL_LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib)
endif()
if(NOT FABRIC_INSTALL_INCLUDE_DIR)
    set(FABRIC_INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include)
endif()
if(NOT FABRIC_INSTALL_DATA_DIR)
    set(FABRIC_INSTALL_DATA_DIR ${CMAKE_INSTALL_PREFIX}/share)
endif()

#-----------------------------------------------------------------------------
# Build FABRIC Main Packages
#-----------------------------------------------------------------------------
# Main includes
include_directories(${HermesShm_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/include)

add_subdirectory(src)

#-----------------------------------------------------------------------------
# Install FABRIC Headers
#-----------------------------------------------------------------------------
install(DIRECTORY include DESTINATION ${CMAKE_INSTALL_PREFIX})
