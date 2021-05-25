find_package(Python3 COMPONENTS Development.Module REQUIRED)
set(PYBIND11_VERSION 2.6.2)

Python3_add_library(pyqbdl MODULE
  pyQBDL.cpp
)

set_target_properties(pyqbdl PROPERTIES PREFIX "")
if (UNIX)
  set_target_properties(pyqbdl PROPERTIES SUFFIX ".so")
elseif(WIN32)
  set_target_properties(pyqbdl PROPERTIES SUFFIX ".pyd")
endif()

target_include_directories(pyqbdl
  PRIVATE
    ${THIRD_PARTY_DIRECTORY}/pybind11-${PYBIND11_VERSION}/include)

target_link_libraries(pyqbdl PRIVATE QBDL)
set_property(TARGET pyqbdl PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
