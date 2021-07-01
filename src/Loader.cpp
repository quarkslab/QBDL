#include <QBDL/Loader.hpp>

namespace QBDL {

Loader::Loader() = default;
Loader::Loader(TargetSystem &engine) : engine_{&engine} {}
Loader::~Loader() = default;

bool Loader::contains_address(uint64_t ptr) const {
  const uint64_t BA = base_address();
  return (ptr >= BA) && (ptr < (BA + mem_size()));
}

} // namespace QBDL
