#ifndef QBDL_LOADER_MACHO_H_
#define QBDL_LOADER_MACHO_H_
#include <memory>

#include <QBDL/Loader.hpp>
#include <QBDL/exports.hpp>

namespace LIEF::MachO {
class Binary;
class FatBinary;
} // namespace LIEF::MachO

namespace QBDL {
struct Arch;
} // namespace QBDL

namespace QBDL::Loaders {
class QBDL_API MachO : public Loader {
public:
  /** Loads a MachO file directly from a LIEF object.
   *
   * This function also loads the binary into \p engine, and return a :MachO
   * object with associated information.
   *
   * @param[in] bin a valid `LIEF::MachO::Binary` object. The returned object
   * takes ownership.
   * @param[in] engine Reference to a ::QBDL::TargetSystem object. The returned
   * ELF object does *not* own this reference. It is the responsibility of the
   * user to ensure this object lives as long as the returned ELF object lives.
   * @param[in] binding Binding mode. Only BIND::NOW is supported for now.
   * @returns A ::QBDL::Loaders::MachO object, or nullptr if loading failed.
   */
  static std::unique_ptr<MachO>
  from_binary(std::unique_ptr<LIEF::MachO::Binary> bin, TargetSystem &engine,
              BIND binding = BIND_DEFAULT);

  /** Select a binary from a universal MachO that matches a given architecture.
   *
   *
   * @param[in] fatbin Reference to a `LIEF::MachO::FatBinary` object that
   * represents the universal MachO binary
   * @param[in] arch Architecture to extract
   * @returns A `LIEF::MachO::Binary` object if \p arch is found successful,
   * nullptr otherwise. The caller gets the ownership of the returned object,
   * and it has been removed from \p fatbin.
   * Returns nullptr if nothing matches. */
  static std::unique_ptr<LIEF::MachO::Binary>
  take_arch_binary(LIEF::MachO::FatBinary &fatbin, Arch const &arch);

  /** Loads a (potentially universal) MachO file directly from disk.
   *
   * If \p path is a universal MachO, this function will select the first
   * architecture that matches with \p arch. If no such architecture is found,
   * nullptr is returned. If successful, this function also loads the binary
   * into \p engine.
   *
   * @param[in] path Path to the MachO file to load
   * @param[in] arch In case of a universal MachO, specify the architecture to
   * extract
   * @param[in] engine Reference to a ::QBDL::TargetSystem object. The returned
   * MachO object does *not* own this reference. It is the responsibility of the
   * user to ensure this object lives as long as the returned MachO object
   * lives.
   * @param[in] binding Binding mode. Note that BIND::LAZY is only supported
   * with a native engine.
   * @returns An ::QBDL::Loaders::MachO object, or nullptr if loading failed.
   */
  static std::unique_ptr<MachO> from_file(const char *path, Arch const &arch,
                                          TargetSystem &engine,
                                          BIND binding = BIND_DEFAULT);

  operator bool() const { return this->is_valid(); }

  inline bool is_valid() const { return this->bin_ != nullptr; }

  uint64_t get_address(const std::string &sym) const override;
  uint64_t get_address(uint64_t offset) const override;
  uint64_t entrypoint() const override;
  uint64_t base_address() const override { return base_address_; }
  uint64_t mem_size() const override { return mem_size_; }
  Arch arch() const override;

  ~MachO() override;

private:
  void bind_now();
  uint64_t get_rva(const LIEF::MachO::Binary &bin, uint64_t addr) const;
  LIEF::MachO::Binary &get_binary() { return *bin_; }
  const LIEF::MachO::Binary &get_binary() const { return *bin_; }
  bool load(BIND binding);

  MachO(std::unique_ptr<LIEF::MachO::Binary> bin, TargetSystem &engine);

  std::unique_ptr<LIEF::MachO::Binary> bin_;
  uint64_t base_address_{0};
  uint64_t mem_size_{0};
};
} // namespace QBDL::Loaders

#endif
