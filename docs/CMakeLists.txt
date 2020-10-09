if (NOT QBDL_PYTHON_BINDING)
  message(FATAL_ERROR "Python bindings must be built to generate the documentation.")
endif()

add_subdirectory(doxygen)

find_program(SPHINX_BUILD_BIN NAMES sphinx-build)

set(QBDL_DOC_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/_build")
set(SPHINX_CACHE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_doctrees")
set(QBDL_DOC_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/qbdl-doc")

file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/qbdl_include.py"
  CONTENT "import sys,os\nsys.path.insert(0, os.path.abspath(\"$<TARGET_FILE_DIR:pyqbdl>\"))\n")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in"
  "${QBDL_DOC_BUILD_DIR}/conf.py"
  @ONLY)

add_custom_target(doc ALL
  COMMAND
    "${SPHINX_BUILD_BIN}"
    -c "${QBDL_DOC_BUILD_DIR}"
    -d "${SPHINX_CACHE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${QBDL_DOC_OUT_DIR}"
  COMMENT "Building documentation with Sphinx..."
)

add_dependencies(doc doc_doxygen QBDL pyqbdl)
