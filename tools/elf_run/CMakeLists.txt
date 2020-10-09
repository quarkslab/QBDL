add_executable(elf_run
  main.cpp
)
target_link_libraries(elf_run PRIVATE QBDL dl)
set_target_properties(elf_run PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)
