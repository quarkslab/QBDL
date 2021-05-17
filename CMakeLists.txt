cmake_minimum_required(VERSION 3.10)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
set(THIRD_PARTY_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/third-party/")

project(QBDL VERSION 0.1.0 LANGUAGES CXX)

option(QBDL_PYTHON_BINDING "Build Python bindings" OFF)
option(QBDL_BUILD_DOCS "Build documentation" OFF)
option(QBDL_BUILD_EXAMPLES "Build examples" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
find_package(LIEF REQUIRED)

add_subdirectory(src)
if (QBDL_BUILD_EXAMPLES)
  add_subdirectory(examples/tools)
endif()
if (QBDL_BUILD_DOCS)
  add_subdirectory(docs)
endif()

if(QBDL_PYTHON_BINDING)
  add_subdirectory(bindings/python)
endif()
