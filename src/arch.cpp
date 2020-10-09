#include <LIEF/Abstract/Binary.hpp>
#include <QBDL/arch.hpp>

namespace QBDL {

Arch Arch::from_bin(LIEF::Binary const &bin) {
  const auto &header = bin.header();
  return {header.architecture(), header.endianness(), header.is_64()};
}

} // namespace QBDL
