add_executable(macho_run
  main.cpp
  darwin_aarch64_libc.cpp
)
target_link_libraries(macho_run PRIVATE QBDL)
set_target_properties(macho_run PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)
