#include <Windows.h>
#include <Memoryapi.h>

namespace QBDL::Engines::Native {

uint64_t TargetMemory::mmap(uint64_t addr, size_t size) {
  void *ret = VirtualAlloc((void*)addr, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

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

} // namespace QBDL::Engines::Native
