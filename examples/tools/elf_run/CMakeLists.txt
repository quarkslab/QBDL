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
