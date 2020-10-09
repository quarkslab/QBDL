#ifndef QBDL_UTILS_H_
#define QBDL_UTILS_H_

#include <cstdint>

static inline constexpr uintptr_t page_mask(uintptr_t page_size) {
  return ~(page_size - 1);
}

static inline constexpr uintptr_t page_start(uintptr_t address,
                                             uintptr_t page_size = 0x1000) {
  return address & page_mask(page_size);
}

static inline constexpr uintptr_t page_align(uintptr_t address,
                                             uintptr_t page_size = 0x1000) {
  return (address + (page_size - 1)) & page_mask(page_size);
}

static inline constexpr uintptr_t page_offset(uintptr_t address,
                                              uintptr_t page_size = 0x1000) {
  return (address & (page_size - 1));
}

#endif
