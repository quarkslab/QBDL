#ifndef QBDL_LOADER_PE_H_
#define QBDL_LOADER_PE_H_
#include <memory>
#include <unordered_map>
#include <vector>

#include <QBDL/Loader.hpp>
#include <QBDL/exports.hpp>

namespace LIEF::PE {
class Binary;
class Symbol;
} // namespace LIEF::PE

namespace QBDL {
struct Arch;
} // namespace QBDL

namespace QBDL::Loaders {

class QBDL_API PE : public Loader {
public:
  /** Loads an PE file directly from a LIEF object.
   *
   * This function also loads the binary into \p engine, and return an :PE
   * object with associated information.
   *
   * @param[in] bin a valid `LIEF::PE::Binary` object. The returned object
   * takes ownership.
   * @param[in] engine Reference to a ::QBDL::TargetSystem object. The returned
   * PE object does *not* own this reference. It is the responsibility of the
   * user to ensure this object lives as long as the returned PE object lives.
   * @param[in] binding Binding mode. Note that the current implementation only
   * supports BIND::DEFAULT and BIND::NOW.
   * @returns A ::QBDL::Loaders::PE object, or nullptr if loading failed.
   */
  static std::unique_ptr<PE> from_binary(std::unique_ptr<LIEF::PE::Binary> bin,
                                         TargetSystem &engine,
                                         BIND binding = BIND::DEFAULT);

  /** Loads an PE file directly from disk.
   *
   * This function also loads the binary into \p engine, and return an :PE
   * object with associated information.
   *
   * @param[in] path Path to the PE file to load
   * @param[in] engine Reference to a ::QBDL::TargetSystem object. The returned
   * PE object does *not* own this reference. It is the responsibility of the
   * user to ensure this object lives as long as the returned PE object lives.
   * @param[in] binding Binding mode. Note that the current implementation only
   * supports BIND::DEFAULT and BIND::NOW.
   * @returns An ::QBDL::Loaders::PE object, or nullptr if loading failed.
   */
  static std::unique_ptr<PE> from_file(const char *path, TargetSystem &engine,
                                       BIND binding = BIND::DEFAULT);

  operator bool() const { return this->is_valid(); }

  inline bool is_valid() const { return this->bin_ != nullptr; }

  uint64_t get_address(const std::string &sym) const override;
  uint64_t get_address(uint64_t offset) const override;
  uint64_t entrypoint() const override;
  Arch arch() const override;

  LIEF::PE::Binary &get_binary() { return *bin_; }
  const LIEF::PE::Binary &get_binary() const { return *bin_; }

  ~PE() override;

private:
  uint64_t get_rva(const LIEF::PE::Binary &bin, uint64_t addr) const;
  void load(BIND binding);
  uintptr_t resolve(const LIEF::PE::Symbol &sym);

  PE(std::unique_ptr<LIEF::PE::Binary> bin, TargetSystem &engines);

  std::unique_ptr<LIEF::PE::Binary> bin_;
  uint64_t base_address_{0};
};
} // namespace QBDL::Loaders

#endif
