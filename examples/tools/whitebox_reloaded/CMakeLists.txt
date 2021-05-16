add_executable(whitebox_reloaded
  main.cpp
)
target_link_libraries(whitebox_reloaded PRIVATE QBDL dl)
set_target_properties(whitebox_reloaded PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)
