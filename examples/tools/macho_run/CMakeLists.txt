add_executable(macho_run
  main.cpp
  darwin_aarch64_libc.cpp
)
target_link_libraries(macho_run PRIVATE QBDL)
set_target_properties(macho_run PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)

if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  add_test(NAME macho_run_simple COMMAND macho_run "${QBDL_EXAMPLES_BINARIES_DIR}/macho-x86-64-hello.bin")
endif()
