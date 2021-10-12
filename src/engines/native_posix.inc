#include <sys/mman.h>

namespace QBDL::Engines::Native {

uint64_t TargetMemory::mmap(uint64_t addr, size_t size) {
  void *ret = ::mmap(reinterpret_cast<void *>(addr), size,
                     PROT_READ | PROT_WRITE | PROT_EXEC,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (ret == MAP_FAILED) {
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

} // namespace QBDL::Engines::Native
