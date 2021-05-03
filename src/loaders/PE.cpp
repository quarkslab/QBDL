#include "logging.hpp"
#include <LIEF/PE.hpp>
#include <QBDL/Engine.hpp>
#include <QBDL/arch.hpp>
#include <QBDL/loaders/PE.hpp>
#include <QBDL/utils.hpp>

using namespace LIEF::PE;

namespace QBDL::Loaders {

std::unique_ptr<PE> PE::from_file(const char *path, TargetSystem &engines,
                                  BIND binding) {
  Logger::info("Loading {}", path);
  if (not is_pe(path)) {
    Logger::err("{} is not an PE file", path);
    return {};
  }
  std::unique_ptr<Binary> bin = Parser::parse(path);
  if (bin == nullptr) {
    Logger::err("Can't parse {}", path);
    return {};
  }
  return from_binary(std::move(bin), engines, binding);
}

std::unique_ptr<PE> PE::from_binary(std::unique_ptr<Binary> bin,
                                    TargetSystem &engines, BIND binding) {
  if (!engines.supports(*bin)) {
    return {};
  }
  std::unique_ptr<PE> loader(new PE{std::move(bin), engines});
  loader->load(binding);
  return loader;
}

PE::PE(std::unique_ptr<Binary> bin, TargetSystem &engines)
    : Loader::Loader(engines), bin_{std::move(bin)} {}

uint64_t PE::get_address(const std::string &sym) const {
  const Binary &binary = get_binary();
  const LIEF::Symbol *symbol = nullptr;
  if (binary.has_symbol(sym)) {
    symbol = &binary.get_symbol(sym);
  }
  if (symbol == nullptr) {
    return 0;
  }
  return base_address_ + get_rva(binary, symbol->value());
}

uint64_t PE::get_address(uint64_t offset) const {
  return base_address_ + offset;
}

uint64_t PE::entrypoint() const {
  const Binary &binary = get_binary();
  return base_address_ +
         (binary.entrypoint() - binary.optional_header().imagebase());
}

void PE::load(BIND binding) {
  Logger::info("this: 0x{:x}", reinterpret_cast<uintptr_t>(this));
  Binary &binary = get_binary();
  const uint64_t imagebase = binary.optional_header().imagebase();

  uint64_t virtual_size = binary.virtual_size();
  virtual_size = page_align(virtual_size);

  Logger::debug("Virtual size: 0x{:x}", virtual_size);

  const uint64_t base_address_hint =
      engine_->base_address_hint(imagebase, virtual_size);
  const uint64_t base_address =
      engine_->mem().mmap(base_address_hint, virtual_size);
  if (base_address == 0 or base_address == -1ull) {
    Logger::err("mmap() failed! Abort.");
    return;
  }
  base_address_ = base_address;

  // Map sections
  // =======================================================
  for (const Section &section : binary.sections()) {
    QBDL_DEBUG("Mapping: {:<10}: (0x{:06x} - 0x{:06x})", section.name(),
               section.virtual_address(),
               section.virtual_address() + section.virtual_size());
    const uint64_t rva = section.virtual_address();
    const std::vector<uint8_t> &content = section.content();
    if (!content.empty()) {
      engine_->mem().write(base_address_ + rva, content.data(), content.size());
    }
  }

  // Perform relocations
  // =======================================================
  if (binary.has_relocations()) {
    const Arch binarch = arch();
    const uint64_t fixup = base_address_ - imagebase;
    for (const Relocation &relocation : binary.relocations()) {
      const uint64_t rva = relocation.virtual_address();
      for (const RelocationEntry &entry : relocation.entries()) {
        switch (entry.type()) {
        case RELOCATIONS_BASE_TYPES::IMAGE_REL_BASED_DIR64: {
          const uint64_t relocation_addr =
              base_address_ + rva + entry.position();
          const uint64_t value =
              engine_->mem().read_ptr(binarch, relocation_addr);
          engine_->mem().write_ptr(binarch, relocation_addr, value + fixup);
          break;
        }

        default: {
          QBDL_ERROR("PE relocation {} is not supported!",
                     to_string(entry.type()));
          break;
        }
        }
      }
    }
  }

  // Perform symbol resolution
  // =======================================================
  // TODO(romain): Find a mechanism to support import by ordinal
  if (binary.has_imports()) {
    const Arch binarch = arch();
    for (const Import &imp : binary.imports()) {
      for (const ImportEntry &entry : imp.entries()) {
        const uint64_t iat_addr = entry.iat_address();
        QBDL_DEBUG("Resolving: {}:{} (0x{:x})", imp.name(), entry.name(),
                   iat_addr);
        LIEF::Symbol sym{entry.name()};
        const uintptr_t sym_addr = engine_->symlink(*this, sym);
        // Write the value in the IAT:
        engine_->mem().write_ptr(binarch, base_address_ + iat_addr, sym_addr);
      }
    }
  }
}

Arch PE::arch() const { return Arch::from_bin(get_binary()); }

uint64_t PE::get_rva(const Binary &bin, uint64_t addr) const {
  const uint64_t imagebase = bin.optional_header().imagebase();
  if (addr >= imagebase) {
    return addr - imagebase;
  }
  return addr;
}

PE::~PE() = default;

} // namespace QBDL::Loaders
