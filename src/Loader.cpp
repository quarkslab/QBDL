#include <QBDL/Loader.hpp>

namespace QBDL {

Loader::Loader() = default;
Loader::Loader(TargetSystem &engine) : engine_{&engine} {}
Loader::~Loader() = default;

} // namespace QBDL
