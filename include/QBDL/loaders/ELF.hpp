#ifndef QBDL_LOADER_ELF_H_
#define QBDL_LOADER_ELF_H_
#include <memory>
#include <unordered_map>
#include <vector>

#include <QBDL/Loader.hpp>
#include <QBDL/exports.hpp>

extern "C" void _dl_resolve_internal();

namespace LIEF::ELF {
class Binary;
class Relocation;
class Symbol;
} // namespace LIEF::ELF

namespace QBDL {
struct Arch;
} // namespace QBDL

namespace QBDL::Loaders {

class QBDL_API ELF : public Loader {
public:
  /** Loads an ELF file directly from a LIEF object.
   *
   * This function also loads the binary into \p engine, and return an :ELF
   * object with associated information.
   *
   * @param[in] bin a valid `LIEF::ELF::Binary` object. The returned object
   * takes ownership.
   * @param[in] engine Reference to a ::QBDL::TargetSystem object. The returned
   * ELF object does *not* own this reference. It is the responsibility of the
   * user to ensure this object lives as long as the returned ELF object lives.
   * @param[in] binding Binding mode. Only BIND::NOW is supported for now.
   * @returns A ::QBDL::Loaders::ELF object, or nullptr if loading failed.
   */
  static std::unique_ptr<ELF>
  from_binary(std::unique_ptr<LIEF::ELF::Binary> bin, TargetSystem &engine,
              BIND binding = BIND::NOW);

  /** Loads an ELF file directly from disk.
   *
   * This function also loads the binary into \p engine, and return an :ELF
   * object with associated information.
   *
   * @param[in] path Path to the ELF file to load
   * @param[in] engine Reference to a ::QBDL::TargetSystem object. The returned
   * ELF object does *not* own this reference. It is the responsibility of the
   * user to ensure this object lives as long as the returned ELF object lives.
   * @param[in] binding Binding mode. Note that BIND::LAZY is only supported
   * with a native engine.
   * @returns An ::QBDL::Loaders::ELF object, or nullptr if loading failed.
   */
  static std::unique_ptr<ELF> from_file(const char *path, TargetSystem &engine,
                                        BIND binding = BIND_DEFAULT);

  operator bool() const { return this->is_valid(); }

  inline bool is_valid() const { return this->bin_ != nullptr; }

  uint64_t get_address(const std::string &sym) const override;
  uint64_t get_address(uint64_t offset) const override;
  uint64_t entrypoint() const override;
  uint64_t base_address() const override { return base_address_; }
  uint64_t mem_size() const override { return mem_size_; }
  Arch arch() const override;

  LIEF::ELF::Binary &get_binary() { return *bin_; }
  const LIEF::ELF::Binary &get_binary() const { return *bin_; }

  ~ELF() override;

private:
  static uintptr_t dl_resolve(void *loader, uintptr_t symidx);
  using relocator_t = void (ELF::*)(const LIEF::ELF::Relocation &);
  void reloc_x86_64(const LIEF::ELF::Relocation &reloc);
  void reloc_aarch64(const LIEF::ELF::Relocation &reloc);
  void bind_lazy(relocator_t relocator);
  void bind_now(relocator_t relocator);
  uint64_t get_rva(const LIEF::ELF::Binary &bin, uint64_t addr) const;
  void load(BIND binding);
  uintptr_t resolve(const LIEF::ELF::Symbol &sym);
  uintptr_t resolve_or_symlink(const LIEF::ELF::Symbol &sym);

  ELF(std::unique_ptr<LIEF::ELF::Binary> bin, TargetSystem &engines);

  std::unique_ptr<LIEF::ELF::Binary> bin_;
  uint64_t base_address_{0};
  uint64_t mem_size_{0};
  std::unordered_map<std::string, LIEF::ELF::Symbol *>
      sym_exp_; // Cache to speed-up symbol resolution
};
} // namespace QBDL::Loaders

#endif
