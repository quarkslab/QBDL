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

uintptr_t MachO::__dyld_stub_binder(void *arg0, void *arg1, void *arg2,
                                    void *arg3, void *arg4, void *arg5) {
  // See:
  // https://github.com/opensource-apple/dyld/blob/3f928f32597888c5eac6003b9199d972d49857b5/src/dyld_stub_binder.s#L118-L121
  // sp[1] = address of ImageLoader cache
  // sp[2] = lazy binding info offset
  // sp[3] = ret address
  //
  // 1. Address of ImageLoader cache
  // 2. Lazy bind offset is the offset into the bindings opcodes associated with
  //    the symbol to resolve.
  // 3. Return address is the return address once the symbol has been resolved
  //
  // We changed the value of sp[1] so that it contains a pointer to the current
  // loader
  //
  // TODO(romain): Abstract sp access
  auto sp = reinterpret_cast<uintptr_t *>(__builtin_frame_address(0));
  const uintptr_t scratch_variable = sp[1];
  const uintptr_t lazy_bind_offset = sp[2];
  Logger::info(
      "SP[0] = 0x{:x} | SP[1] = 0x{:x} | SP[2] = 0x{:x} | SP[3] = 0x{:x}",
      sp[0], sp[1], sp[2], sp[3]);
  Logger::info("*sp[1]: 0x{:x}",
               *reinterpret_cast<uintptr_t *>(scratch_variable));
  MachO &loader = **reinterpret_cast<MachO **>(sp[1]);
  LIEF::MachO::Binary &binary = loader.get_binary();
  bool found = false;
  for (const LIEF::MachO::BindingInfo &info : binary.dyld_info().bindings()) {
    if (info.binding_class() != LIEF::MachO::BINDING_CLASS::BIND_CLASS_LAZY) {
      continue;
    }
    if (info.original_offset() != lazy_bind_offset) {
      continue;
    }
    const LIEF::Symbol &sym = info.symbol();
    Logger::info("{}", info.symbol().name());

    uint64_t addr = loader.engine_->symlink(loader, sym);
    if (addr > 0) {
      found = true;
      auto func = reinterpret_cast<decltype(MachO::__dyld_stub_binder) *>(addr);
      sp[1] = sp[3];
      // Call the resolved function
      return func(arg0, arg1, arg2, arg3, arg4, arg5);
    }
  }
  if (not found) {
    Logger::err("Can't find symbol associated with offset {:d}", sp[2]);
    std::exit(1);
  }
  // Restore original ret address
  sp[1] = sp[3];
  return 0;
};

std::unique_ptr<MachO> MachO::from_file(const char *path, Arch const &arch,
                                        TargetSystem &engine, BIND binding) {
  Logger::info("Loading {}", path);
  if (not LIEF::MachO::is_macho(path)) {
    Logger::err("{} is not a Mach-O file", path);
    return {};
  }
  std::unique_ptr<LIEF::MachO::FatBinary> fat =
      LIEF::MachO::Parser::parse(path);
  if (fat == nullptr or fat->size() == 0) {
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
    auto &bin = fatbin[i];
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
  if (not binary.has_symbol(sym)) {
    return 0;
  }
  const LIEF::MachO::Symbol &symbol = binary.get_symbol(sym);
  return base_address_ + get_rva(binary, symbol.value());
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
  Logger::info("this: 0x{:x}", reinterpret_cast<uintptr_t>(this));
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
    const std::vector<uint8_t> &content = segment.content();

    if (content.size() > 0) {
      engine_->mem().write(base_address + rva, content.data(), content.size());
    }
  }

  // Perform relocations
  // =======================================================
  for (const LIEF::MachO::Relocation &relocation : binary.relocations()) {
    if (relocation.origin() ==
        LIEF::MachO::RELOCATION_ORIGINS::ORIGIN_RELOC_TABLE) {
      Logger::warn("Relocation not handled!");
      continue;
    }

    const auto rtype =
        static_cast<LIEF::MachO::REBASE_TYPES>(relocation.type());
    switch (rtype) {
    case LIEF::MachO::REBASE_TYPES::REBASE_TYPE_POINTER: {
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

  case BIND::LAZY: {
    // Note: it should not be available for target that are not "native"
    bind_lazy();
    break;
  }

  case BIND::NOT_BIND:
  case BIND::DEFAULT:
  default: {
    break;
  }
  }

  return true;
}

void MachO::bind_lazy() {
  static const std::string DYLD_SYM_NAME = "dyld_stub_binder";

  const LIEF::MachO::Binary &binary = get_binary();
  const Arch binarch = arch();
  const uintptr_t base_address = base_address_;
  // dyld_stub_binder
  // =======================================================
  if (not binary.has_symbol(DYLD_SYM_NAME)) {
    Logger::warn("Symbol {} not found. Can't bind symbol lazily",
                 DYLD_SYM_NAME);
    return;
  }
  const LIEF::MachO::Symbol &dyld_stub_binder =
      binary.get_symbol(DYLD_SYM_NAME);
  if (not dyld_stub_binder.has_binding_info()) {
    Logger::warn("Can't find binding info associated with symbol {}",
                 DYLD_SYM_NAME);
    return;
  }
  const uintptr_t rva =
      get_rva(binary, dyld_stub_binder.binding_info().address());
  Logger::info("dyld_stub_binder addr 0x{:x}", rva);

  // clang-format off
  // __stub_helper:0000000100000F8C loc_100000F8C:                          ;
  // CODE XREF: __stub_helper:0000000100000FA1↓j
  // __stub_helper:0000000100000F8C lea     r11, off_100001008
  // __stub_helper:0000000100000F93 push    r11
  // __stub_helper:0000000100000F95 jmp     cs:dyld_stub_binder_ptr
  // __stub_helper:0000000100000F95 ;
  // ---------------------------------------------------------------------------
  // __stub_helper:0000000100000F9B align 4
  // __stub_helper:0000000100000F9C push    0             ; Lazy bind offset within the opcodes
  // __stub_helper:0000000100000FA1 jmp     loc_100000F8C
  // ...
  // __nl_symbol_ptr:0000000100001000 __nl_symbol_ptr segment qword public 'DATA' use64
  // __nl_symbol_ptr:0000000100001001                 assume cs:__nl_symbol_ptr
  // __nl_symbol_ptr:0000000100001000                 ;org 100001000h
  // __nl_symbol_ptr:0000000100001000 dyld_stub_binder_ptr dq offset dyld_stub_binder
  // __nl_symbol_ptr:0000000100001000                                         ; DATA XREF: __stub_helper:0000000100000F95↑r
  // __nl_symbol_ptr:0000000100001008 off_100001008   dq 0                    ; DATA XREF: __stub_helper:loc_100000F8C↑o
  // __nl_symbol_ptr:0000000100001008 __nl_symbol_ptr ends
  // ...
  // When calling dyld_stub_binder, it calls dyld_stub_binder_ptr
  // (off_100001000). Since the stub of all imported functions call
  // dyld_stub_binder_ptr, we can replace this pointer with our own
  // implementation.
  //
  // The address of the "cache" (dyld_stub_binder_ptr + 8) in this case
  // is not always located at +uint64_t after
  // the symbol __dyld_stub_binder. The Mach-O format does not provide
  // any clue to get its address. Nevertheless, we know that this address
  // is always pushed right before calling dyld_stub_binder. Therefore,
  // we can do a "dry-call" to dyld_stub_bind to get this address.
  // clang-format on
  //
  engine_->mem().write_ptr(
      binarch, base_address + rva,
      reinterpret_cast<uintptr_t>(&__dyld_stub_binder_dry_call));
  const LIEF::MachO::Section &stub_helper = binary.get_section("__stub_helper");
  const uintptr_t stub_helper_va =
      base_address_ + get_rva(binary, stub_helper.virtual_address());
  auto stub_helper_fcn = reinterpret_cast<uintptr_t (*)()>(stub_helper_va);
  uintptr_t cache_address = stub_helper_fcn();
  Logger::info("Scratch variable addr: 0x{:x}", cache_address);
  engine_->mem().write_ptr(
      binarch, base_address + rva,
      reinterpret_cast<uintptr_t>(&MachO::__dyld_stub_binder));
  engine_->mem().write_ptr(binarch, cache_address,
                           reinterpret_cast<uintptr_t>(this));

  for (const LIEF::MachO::BindingInfo &info : binary.dyld_info().bindings()) {
    // TODO(romain): Add BIND_CLASS_THREADED when moving to LIEF 0.12.0
    if (info.binding_class() == LIEF::MachO::BINDING_CLASS::BIND_CLASS_LAZY) {
      continue;
    }
    if (!info.has_symbol()) {
      Logger::warn("Missing symbol");
      continue;
    }
    const auto &sym = info.symbol();
    if (sym.name() == "dyld_stub_binder") {
      continue;
    }
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
void MachO::bind_now() {
  const LIEF::MachO::Binary &binary = get_binary();
  const Arch binarch = arch();
  for (const LIEF::MachO::BindingInfo &info : binary.dyld_info().bindings()) {
    // TODO(romain): Add BIND_CLASS_THREADED when moving to LIEF 0.12.0
    if (info.binding_class() != LIEF::MachO::BINDING_CLASS::BIND_CLASS_LAZY and
        info.binding_class() !=
            LIEF::MachO::BINDING_CLASS::BIND_CLASS_STANDARD) {
      continue;
    }
    if (!info.has_symbol()) {
      Logger::warn("Lazy bindings isn't linked to a symbol!");
      continue;
    }
    const auto &sym = info.symbol();
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
