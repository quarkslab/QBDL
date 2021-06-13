#ifndef QBDL_LOADER_H_
#define QBDL_LOADER_H_

#include <QBDL/arch.hpp>
#include <QBDL/exports.hpp>
#include <QBDL/macros.hpp>

#include <string>

namespace QBDL {
class TargetSystem;

/** Base class for a Loader
 */
class QBDL_API Loader {
public:
  enum class BIND { NOT_BIND, NOW, LAZY };
  static constexpr inline BIND BIND_DEFAULT = BIND::NOW;

public:
  /** Get the resolved absolute virtual address of a symbol.
   */
  virtual uint64_t get_address(const std::string &sym) const = 0;

  /** Compute the absolute virtual address of on offset. This is basically
   * `base_address + offset`.
   */
  virtual uint64_t get_address(uint64_t offset) const = 0;

  /** Get the absolute virtual address of the entrypoint.
   */
  virtual uint64_t entrypoint() const = 0;

  /** Get the absolute virtual address of the base address.
   */
  virtual uint64_t base_address() const = 0;

  virtual ~Loader();

  /** Get the architecture targeted by the loaded binary.
   */
  virtual Arch arch() const = 0;

protected:
  Loader();
  Loader(TargetSystem &engine);
  TargetSystem *engine_{nullptr};

private:
  DISALLOW_COPY_AND_ASSIGN(Loader);
};
} // namespace QBDL

#endif
