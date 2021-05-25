set(SPDLOG_VERSION 1.8.2)
set(QBDL_MAIN_SRC
  "Loader.cpp"
  "logging.cpp"
  "arch.cpp"
  "Engine.cpp"
)

set(QBDL_MAIN_INC
  "logging.hpp"
)

add_library(QBDL
  ${QBDL_MAIN_SRC}
)
target_link_libraries(QBDL PUBLIC
  LIEF::LIEF
)

target_include_directories(QBDL
  PRIVATE ${THIRD_PARTY_DIRECTORY}/spdlog-${SPDLOG_VERSION}/include)

get_target_property(QBDL_TYPE QBDL TYPE)
if (QBDL_TYPE STREQUAL "STATIC_LIBRARY")
  target_compile_definitions(QBDL PUBLIC QBDL_STATIC)
endif()

set_target_properties(QBDL PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  C_VISIBILITY_PRESET       hidden
  CXX_VISIBILITY_PRESET     hidden
  VISIBILITY_INLINES_HIDDEN ON
)
if (NOT MSVC)
  # AG: LINK_WHAT_YOU_USE try to use ldd under Windows!
  set_target_properties(QBDL PROPERTIES
    LINK_WHAT_YOU_USE         ON
  )
endif()

if (NOT BUILD_SHARED_LIBS)
  set_property(TARGET QBDL PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Multiple things:
# * remove RTTI and exceptions
# * activate some warnings
if(MSVC)
  # GR-: remove RTTI
  # GX-: remove exceptions
  # Gy: equivalent of -ffunction-sections
  set(CXX_FLAGS /Gy)
  target_compile_definitions(QBDL PRIVATE _CRT_SECURE_NO_WARNINGS _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
  # LIEF public headers need this include if we are using MSVC...
  target_compile_options(QBDL PRIVATE /FIiso646.h)
else (MSVC)
  set(CXX_FLAGS
    -Wall
    -Wno-undefined-var-template)
endif(MSVC)
target_compile_options(QBDL PRIVATE ${CXX_FLAGS})
target_compile_definitions(QBDL PRIVATE SPDLOG_NO_EXCEPTIONS)

target_include_directories(QBDL
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(QBDL PUBLIC
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

include("${CMAKE_CURRENT_LIST_DIR}/loaders/CMakeLists.txt")
include("${CMAKE_CURRENT_LIST_DIR}/engines/CMakeLists.txt")
