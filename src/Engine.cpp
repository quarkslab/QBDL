#include "intmem.hpp"
#include <QBDL/Engine.hpp>
#include <QBDL/arch.hpp>

namespace QBDL {

namespace {
template <class T> struct Tag { using type = T; };

template <class Func>
static constexpr auto archPtrType(Arch const &arch, Func F) {
  // TODO: more precise tests
  if (arch.is64) {
    return F(Tag<uint64_t>{});
  }
  return F(Tag<uint32_t>{});
}

template <class T>
void write_ptr_impl(TargetMemory &mem, uint64_t addr, uint64_t ptr,
                    LIEF::ENDIANNESS endian) {
  T data;
  const T ptr_ = static_cast<T>(ptr);
  if (endian == LIEF::ENDIANNESS::ENDIAN_LITTLE) {
    intmem::store_le<T>(&data, ptr_);
  } else {
    intmem::store_be<T>(&data, ptr_);
  }
  mem.write(addr, &data, sizeof(data));
}
template <class T>
uint64_t read_ptr_impl(TargetMemory &mem, uint64_t addr,
                       LIEF::ENDIANNESS endian) {
  T ret;
  mem.read(&ret, addr, sizeof(ret));
  if (endian == LIEF::ENDIANNESS::ENDIAN_LITTLE) {
    return intmem::bswap_le(ret);
  }
  return intmem::bswap_be(ret);
}

} // namespace

void TargetMemory::write_ptr(Arch const &arch, uint64_t addr, uint64_t ptr) {
  archPtrType(arch, [&](auto tag) {
    using T = typename decltype(tag)::type;
    write_ptr_impl<T>(*this, addr, ptr, arch.endianness);
  });
}

uint64_t TargetMemory::read_ptr(Arch const &arch, uint64_t addr) {
  return archPtrType(arch, [&](auto tag) {
    using T = typename decltype(tag)::type;
    return read_ptr_impl<T>(*this, addr, arch.endianness);
  });
}

} // namespace QBDL
