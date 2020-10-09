#ifndef QBDL_ARCH_H_
#define QBDL_ARCH_H_

#include <LIEF/Abstract/enums.hpp>
#include <QBDL/exports.hpp>

namespace LIEF {
class Binary;
} // namespace LIEF

namespace QBDL {

/** Define a binary architecture
 *
 * It is based on LIEF's description of an architecture. Based on this
 * description, we need three fields to completely target a binary
 * architecture:
 *
 * - the architecture as per LIEF's description
 * - the endianness of the CPU
 * - whether the CPU is 64bit
 *
 * This is a bit unfortunate that LIEF does not have a simple enum that would
 * map every possible architectures.
 *
 * Here are a few common architectures description that are used:
 *
 * \code
 * const auto x86 = Arch{LIEF::ARCHITECTURES::X86, LIEF::ENDIANNESS::LITTLE,
 * false}; const auto x64 = Arch{LIEF::ARCHITECTURES::X86,
 * LIEF::ENDIANNESS::LITTLE, true}; const auto armv8 =
 * Arch{LIEF::ARCHITECTURES::ARM64, LIEF::ENDIANNESS::LITTLE, true}; \endcode
 */
struct Arch {
  /** Architecture as defined by LIEF.
   */
  LIEF::ARCHITECTURES arch;

  /** Endianness of the binary's architecture
   */
  LIEF::ENDIANNESS endianness;

  /** Whether this is a 64bit CPU
   */
  bool is64;

  constexpr Arch(LIEF::ARCHITECTURES arch, LIEF::ENDIANNESS endianness,
                 bool is64)
      : arch(arch), endianness(endianness), is64(is64) {}

  constexpr bool operator==(Arch const &o) const {
    return arch == o.arch && endianness == o.endianness && is64 == o.is64;
  }

  constexpr bool operator!=(Arch const &o) const { return !(*this == o); }

  /** Construct an `Arch` object from a LIEF binary object
   */
  static QBDL_API Arch from_bin(LIEF::Binary const &bin);
};

} // namespace QBDL

#endif
