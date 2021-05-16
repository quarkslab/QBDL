#include "logging.hpp"
#include <QBDL/engines/Native.hpp>

static_assert(
    sizeof(uintptr_t) <= sizeof(uint64_t),
    "native target with pointer integer type > 64 bits are not supported");

namespace QBDL::Engines::Native {

void TargetMemory::write(uint64_t addr, const void *ptr, size_t size) {
  memcpy(reinterpret_cast<void *>(addr), ptr, size);
}

void TargetMemory::read(void *buf, uint64_t addr, size_t size) {
  memcpy(buf, reinterpret_cast<const void *>(addr), size);
}

bool TargetSystem::supports(LIEF::Binary const &bin) {
  return Arch::from_bin(bin) == arch();
}

uint64_t TargetSystem::base_address_hint(uint64_t binary_base_address,
                                         uint64_t virtual_size) {
  // Mean a random base address
  return 0;
}

QBDL_API std::unique_ptr<QBDL::TargetMemory> memory() {
  QBDL::TargetMemory *Ret = new Native::TargetMemory{};
  return std::unique_ptr<QBDL::TargetMemory>{Ret};
}

}

#if defined(__linux__) or defined(__APPLE__)
#include "native_posix.inc"
#elif defined(_WIN32)
#include "native_win.inc"
#else
#error Unsupported OS
#endif

