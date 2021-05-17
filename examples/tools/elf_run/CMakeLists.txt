add_executable(elf_run
  main.cpp
)
target_link_libraries(elf_run PRIVATE QBDL)
if (UNIX) 
  target_link_libraries(elf_run PRIVATE dl)
endif()
set_target_properties(elf_run PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)

if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  add_test(NAME elf_run_simple COMMAND elf_run "${QBDL_EXAMPLES_BINARIES_DIR}/elf-linux-x86-64-hello.bin")
endif()
