set(QBDL_LOADERS_SRC
  "${CMAKE_CURRENT_LIST_DIR}/MachO.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/ELF.cpp"
)
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  list(APPEND QBDL_LOADERS_SRC
    ${CMAKE_CURRENT_LIST_DIR}/arch/x86-64/dyld_stub_binder_dry.s
    ${CMAKE_CURRENT_LIST_DIR}/arch/x86-64/dl_trampoline.s)
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
  list(APPEND QBDL_LOADERS_SRC
    ${CMAKE_CURRENT_LIST_DIR}/arch/aarch64/dyld_stub_binder_dry.s
    ${CMAKE_CURRENT_LIST_DIR}/arch/aarch64/dl_trampoline.s)
else()
  message(FATAL_ERROR "Missing stub for ${CMAKE_SYSTEM_PROCESSOR}")
endif()

set(QBDL_LOADERS_INC )

target_sources(QBDL PRIVATE
  ${QBDL_LOADERS_SRC}
  ${QBDL_LOADERS_INC}
)

