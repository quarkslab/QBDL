#include "logging.hpp"
#include <QBDL/engines/Native.hpp>
#include <sys/mman.h>

namespace QBDL::Engines::Native {

static_assert(
    sizeof(uintptr_t) <= sizeof(uint64_t),
    "native target with pointer integer type > 64 bits are not supported");

uint64_t TargetMemory::mmap(uint64_t addr, size_t size) {
  void *ret = ::mmap(reinterpret_cast<void *>(addr), size,
                     PROT_READ | PROT_WRITE | PROT_EXEC,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (ret == nullptr) {
    Logger::err("Error while trying mmap: {}", strerror(errno));
    return 0;
  }

  Logger::debug("mmap(0x{:x}, 0x{:x}): 0x{:x}", addr, size,
                reinterpret_cast<uintptr_t>(ret));
  return reinterpret_cast<uint64_t>(ret);
}

bool TargetMemory::mprotect(uint64_t addr, size_t size, int prot) {
  Logger::warn("mprotect not implemented!");
  return false;
}

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

} // namespace QBDL::Engines::Native
