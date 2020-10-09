find_package(Python3 ${PYTHON_VERSION} EXACT COMPONENTS Development REQUIRED)
set(PYBIND11_VERSION 2.6.2)

Python3_add_library(pyqbdl SHARED
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

target_compile_options(pyqbdl PUBLIC -frtti -fexceptions)
target_link_libraries(pyqbdl PRIVATE QBDL)
