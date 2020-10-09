#ifndef QBDL_ENGINE_NATIVE_H_
#define QBDL_ENGINE_NATIVE_H_

#include <QBDL/Engine.hpp>
#include <QBDL/arch.hpp>
#include <QBDL/exports.hpp>

#include <optional>

namespace LIEF {
class Binary;
}

namespace QBDL::Engines::Native {

/** Native ::QBDL::TargetMemory class that matches the OS QBDL is
 * running onto.
 *
 * It only works for Linux-based system for now.
 */
QBDL_API class TargetMemory : public QBDL::TargetMemory {
public:
  uint64_t mmap(uint64_t hint, size_t len) override;
  bool mprotect(uint64_t addr, size_t len, int prot) override;
  void write(uint64_t addr, const void *buf, size_t len) override;
  void read(void *dst, uint64_t addr, size_t len) override;
};

/** Allocates and returns a ::QBDL::Engines::Native::TargetMemory object.
 */
QBDL_API std::unique_ptr<QBDL::TargetMemory> memory();

/** Native ::QBDL::TargetMemory class that matches the OS on which QBDL is
 * running on.
 */
QBDL_API class TargetSystem : public QBDL::TargetSystem {
public:
  using QBDL::TargetSystem::TargetSystem;

  bool supports(LIEF::Binary const &bin) override;
  uint64_t base_address_hint(uint64_t binary_base_address,
                             uint64_t virtual_size) override;
};

namespace details {
static inline constexpr LIEF::ARCHITECTURES LIEFArch() {
#if defined(__arm__)
  return LIEF::ARCH_ARM;
#elif defined(__aarch64__)
  return LIEF::ARCH_ARM64;
#elif defined(__i386__) || defined(__x86_64__)
  return LIEF::ARCH_X86;
#elif defined(__mips__)
  return LIEF::ARCH_MIPS;
#else
  return LIEF::ARCH_NONE;
#endif
}

static inline constexpr LIEF::ENDIANNESS LIEFEndianess() {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  return LIEF::ENDIAN_LITTLE;
#else
  return LIEF::ENDIAN_BIG;
#endif
}

static inline constexpr bool Is64Bit() {
#ifdef __LP64__
  return true;
#else
  return false;
#endif
}
} // namespace details

/** Returns a ::QBDL::Arch object that represents the native architecture QBDL
 * is running onto.
 */
static inline constexpr Arch arch() {
  return Arch{details::LIEFArch(), details::LIEFEndianess(),
              details::Is64Bit()};
}

} // namespace QBDL::Engines::Native

#endif
