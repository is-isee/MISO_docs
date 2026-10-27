# practice/execution の解答例。第 1 章の「自分のプログラムから MISO を使う最小の CMake 設定」と同じ形。
#   CPU : cmake -S . -B build_host -DMISO_DIR=/path/to/MISO
#   CUDA: cmake -S . -B build_cuda -DMISO_DIR=/path/to/MISO -DUSE_CUDA=ON -DCMAKE_CUDA_ARCHITECTURES=89
cmake_minimum_required(VERSION 3.18)
project(execution_solutions LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MISO_DIR "" CACHE PATH "MISO を clone したフォルダ")
if(NOT EXISTS "${MISO_DIR}/miso/CMakeLists.txt")
  message(FATAL_ERROR "-DMISO_DIR=/path/to/MISO を指定してください")
endif()

option(USE_CUDA "CUDA でビルドする（第 9 章の後）" OFF)
if(USE_CUDA)
  if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES native CACHE STRING "CUDA architectures")
  endif()
  enable_language(CUDA)
  set(CMAKE_CUDA_STANDARD 17)
  set(CMAKE_CUDA_STANDARD_REQUIRED ON)
  set(MISO_USE_CUDA ON CACHE BOOL "Enable CUDA support in MISO")
endif()

# MISO のユニットテストは作らない
set(ENABLE_TESTING OFF CACHE BOOL "Disable tests in MISO")
add_subdirectory(${MISO_DIR}/miso ${CMAKE_BINARY_DIR}/miso)

foreach(name 01_vector_add 02_reduction 03_advection_1d)
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} PRIVATE MISO::MISO)
  if(USE_CUDA)
    set_source_files_properties(${name}.cpp PROPERTIES LANGUAGE CUDA)
  endif()
endforeach()
