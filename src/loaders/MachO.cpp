#include "logging.hpp"
#include <LIEF/MachO.hpp>
#include <QBDL/Engine.hpp>
#include <QBDL/arch.hpp>
#include <QBDL/loaders/MachO.hpp>
#include <QBDL/utils.hpp>

// Return this address of the ImageCache
// See: dyld_stub_binder_dry.s for the implementation
extern "C" uintptr_t __dyld_stub_binder_dry_call();

namespace QBDL::Loaders {

std::unique_ptr<MachO> MachO::from_file(const char *path, Arch const &arch,
                                        TargetSystem &engine, BIND binding) {
  Logger::info("Loading {}", path);
  if (!LIEF::MachO::is_macho(path)) {
    Logger::err("{} is not a Mach-O file", path);
    return {};
  }
  std::unique_ptr<LIEF::MachO::FatBinary> fat =
      LIEF::MachO::Parser::parse(path);
  if (fat == nullptr || fat->size() == 0) {
    Logger::err("Can't parse {}", path);
    return {};
  }
  auto bin = take_arch_binary(*fat, arch);
  if (!bin) {
    Logger::err("Unable to find a binary that match given architecture");
    return {};
  }
  return from_binary(std::move(bin), engine, binding);
}

std::unique_ptr<MachO>
MachO::from_binary(std::unique_ptr<LIEF::MachO::Binary> bin,
                   TargetSystem &engine, BIND binding) {
  if (!engine.supports(*bin)) {
    Logger::err("Engine does not support binary!");
    return {};
  }
  std::unique_ptr<MachO> loader(new MachO{std::move(bin), engine});
  loader->load(binding);
  return loader;
}

std::unique_ptr<LIEF::MachO::Binary>
MachO::take_arch_binary(LIEF::MachO::FatBinary &fatbin, Arch const &arch) {
  for (size_t i = 0; i < fatbin.size(); ++i) {
    auto &bin = *fatbin[i];
    if (Arch::from_bin(bin) == arch) {
      return fatbin.take(i);
    }
  }
  return {};
}

MachO::MachO(std::unique_ptr<LIEF::MachO::Binary> bin, TargetSystem &engine)
    : Loader::Loader(engine), bin_{std::move(bin)} {}

uint64_t MachO::get_address(const std::string &sym) const {
  const LIEF::MachO::Binary &binary = get_binary();
  const LIEF::MachO::Symbol *symbol = binary.get_symbol(sym);
  if (!symbol) {
    return 0;
  }
  return base_address_ + get_rva(binary, symbol->value());
}

uint64_t MachO::get_address(uint64_t offset) const {
  return base_address_ + offset;
}

Arch MachO::arch() const {
  assert(is_valid());
  return Arch::from_bin(get_binary());
}

uint64_t MachO::entrypoint() const {
  const LIEF::MachO::Binary &binary = get_binary();
  return base_address_ + (binary.entrypoint() - binary.imagebase());
}

bool MachO::load(BIND binding) {
  LIEF::MachO::Binary &binary = get_binary();
  const Arch binarch = arch();

  // TODO(romain): Could be moved in LIEF
  uint64_t virtual_size = 0;
  for (const LIEF::MachO::SegmentCommand &segment : binary.segments()) {
    virtual_size = std::max(virtual_size,
                            segment.virtual_address() + segment.virtual_size());
  }
  virtual_size -= binary.imagebase();
  virtual_size = page_align(virtual_size);
  mem_size_ = virtual_size;

  Logger::debug("Virtual size: 0x{:x}", virtual_size);

  const uint64_t base_address_hint =
      engine_->base_address_hint(binary.imagebase(), virtual_size);
  const uint64_t base_address =
      engine_->mem().mmap(base_address_hint, virtual_size);
  base_address_ = base_address;

  // Map segments
  // =======================================================
  for (const LIEF::MachO::SegmentCommand &segment : binary.segments()) {
    if (segment.size() == 0) { // __PAGEZERO for instance
      continue;
    }
    const uint64_t rva = get_rva(binary, segment.virtual_address());

    Logger::debug("Mapping {} - 0x{:x}", segment.name(), rva);
    const auto &content = segment.content();

    if (content.size() > 0) {
      engine_->mem().write(base_address + rva, content.data(), content.size());
    }
  }

  // Perform relocations
  // =======================================================
  for (const LIEF::MachO::Relocation &relocation : binary.relocations()) {
    if (relocation.origin() ==
        LIEF::MachO::Relocation::ORIGIN::RELOC_TABLE) {
      Logger::warn("Relocation not handled!");
      continue;
    }

    const auto rtype =
        static_cast<LIEF::MachO::DyldInfo::REBASE_TYPE>(relocation.type());
    switch (rtype) {
    case LIEF::MachO::DyldInfo::REBASE_TYPE::POINTER: {
      const uint64_t rva = get_rva(binary, relocation.address());
      const uint64_t rel_ptr = base_address + rva;
      uint64_t rel_ptr_val = engine_->mem().read_ptr(binarch, rel_ptr);
      if (rel_ptr_val >= binary.imagebase()) {
        rel_ptr_val -= binary.imagebase();
      }
      rel_ptr_val += base_address;
      engine_->mem().write_ptr(binarch, rel_ptr, rel_ptr_val);
      break;
    }

    default: {
      Logger::warn("Relocation {} not supported yet",
                   LIEF::MachO::to_string(rtype));
    }
    }
  }

  // Bind symbols
  switch (binding) {
  case BIND::NOW: {
    bind_now();
    break;
  }

  case BIND::NOT_BIND:
  default:
    break;
  }

  return true;
}

void MachO::bind_now() {
  const LIEF::MachO::Binary &binary = get_binary();
  const Arch binarch = arch();
  for (const LIEF::MachO::DyldBindingInfo &info : binary.dyld_info()->bindings()) {
    if (info.binding_class() != LIEF::MachO::DyldBindingInfo::CLASS::LAZY &&
        info.binding_class() != LIEF::MachO::DyldBindingInfo::CLASS::STANDARD &&
        info.binding_class() != LIEF::MachO::DyldBindingInfo::CLASS::THREADED) {
      continue;
    }
    if (!info.has_symbol()) {
      Logger::warn("Lazy bindings isn't linked to a symbol!");
      continue;
    }
    const auto &sym = *info.symbol();
    const uint64_t ptrRVA = get_rva(binary, info.address());
    const uint64_t ptrAddr = base_address_ + ptrRVA;
    const uint64_t symAddr = engine_->symlink(*this, sym);
    Logger::info(
        "Symbol {} resolves to address 0x{:x}, stored at address 0x{:x}",
        sym.name(), symAddr, ptrAddr);
    // Store the address of the resolved symbol into ptrAddr
    engine_->mem().write_ptr(binarch, ptrAddr, symAddr);
  }
}

uint64_t MachO::get_rva(const LIEF::MachO::Binary &bin, uint64_t addr) const {
  if (addr >= bin.imagebase()) {
    return addr - bin.imagebase();
  }
  return addr;
}

MachO::~MachO() = default;

} // namespace QBDL::Loaders
