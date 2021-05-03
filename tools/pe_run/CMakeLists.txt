add_executable(pe_run
  main.cpp
)
target_link_libraries(pe_run PRIVATE QBDL)
set_target_properties(pe_run PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)
