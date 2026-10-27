cmake_minimum_required(VERSION 3.18)
project(heat1d LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE
        Release
        CACHE STRING "Choose a type of build." FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MISO の場所: cmake -DMISO_DIR=/path/to/MISO
set(MISO_DIR
    ""
    CACHE PATH "Path to the MISO repository")
if(NOT EXISTS "${MISO_DIR}/miso/CMakeLists.txt")
    message(FATAL_ERROR "Set -DMISO_DIR=/path/to/MISO")
endif()

# GPU 版 (03) を CUDA でビルドするときは -DMISO_USE_CUDA=ON
option(MISO_USE_CUDA "Enable CUDA support" OFF)
if(MISO_USE_CUDA)
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
        set(CMAKE_CUDA_ARCHITECTURES
            native
            CACHE STRING "CUDA architectures")
    endif()
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
endif()

# MISO のユニットテストはビルドしない
set(ENABLE_TESTING
    OFF
    CACHE BOOL "Build MISO unit tests")
add_subdirectory(${MISO_DIR}/miso ${CMAKE_BINARY_DIR}/miso)

add_executable(heat1d_serial 01_serial.cpp)
target_link_libraries(heat1d_serial PRIVATE MISO::MISO)

add_executable(heat1d_mpi 02_mpi.cpp)
target_link_libraries(heat1d_mpi PRIVATE MISO::MISO)

add_executable(heat1d_gpu 03_gpu.cpp)
target_link_libraries(heat1d_gpu PRIVATE MISO::MISO)
if(MISO_USE_CUDA)
    set_source_files_properties(03_gpu.cpp PROPERTIES LANGUAGE CUDA)
endif()
