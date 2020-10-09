#ifndef QBDL_ENGINE_H_
#define QBDL_ENGINE_H_

#include <QBDL/exports.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>

namespace LIEF {
class Symbol;
class Binary;
} // namespace LIEF

namespace QBDL {
class Loader;
struct Arch;

/** Describe the target memory the binary must be loaded into.
 *
 * This abstraction helps target many different memory system.
 *
 * To describe your target, you need to subclass this and implement the pure
 * virtual functions. See their documentation below.
 */
QBDL_API class TargetMemory {
public:
  virtual ~TargetMemory() = default;

  /** Reserve a region of memory in the targeted memory space.
   *
   * @param[in] hint A hint as to where the address should be. 0 means any
   * address.
   * @param[in] len Size of the memory region to reserve
   * @returns 0 if an error occurred, any value otherwise.
   */
  virtual uint64_t mmap(uint64_t hint, size_t len) = 0;

  /** Change permissions on a region of memory.
   */
  virtual bool mprotect(uint64_t addr, size_t len, int prot) = 0;

  /** Write data to a memory region in the targeted memory space.
   *
   * This always succeeds.
   *
   * @param addr Virtual absolute address to write data into
   * @param buf Pointer to the data to write
   * @param len Length of the data to write
   */
  virtual void write(uint64_t addr, const void *buf, size_t len) = 0;

  /** Read data from a memory region in the targeted memory space.
   *
   * This always succeeds.
   * @param dst A pointer to the buffer that will receive the data
   * @param addr Virtual absolute address to read the data from
   * @param len Length of the data to read
   */
  virtual void read(void *dst, uint64_t addr, size_t len) = 0;

  /** Convenience function that write a pointer value to the targeted memory
   * space, given an architecture.
   *
   * No efforts are made to verify that \p arch matches what
   * ::QBDL::TargetMemory expects.
   *
   * This always succeeds.
   *
   * @param[in] arch Targeted architecture
   * @param[in] addr Virtual absolute address to write the pointer value into
   * @param[in] ptr Value of the address to write
   */
  void write_ptr(Arch const &arch, uint64_t addr, uint64_t ptr);

  /** Convenience function that reads a pointer value from the targeted memory
   * space, given an architecture.
   *
   * No efforts are made to verify that \p arch matches what
   * ::QBDL::TargetMemory expects.
   *
   * This always succeeds.
   *
   * @param[in] arch Targeted architecture
   * @param[in] addr Virtual absolute address to read the pointer value from
   * @returns The read pointer value
   */
  uint64_t read_ptr(Arch const &arch, uint64_t addr);
};

/** Describe the target system the binary must be loaded into.
 *
 * This abstraction helps describe:
 *
 * - how external symbol are resolved
 * - if a system supports a given binary (mainly used to choose a binary in fat
 * MachO files)
 * - how memory is handled (through a `TargetMemory` object)
 * - the base address that must be used when mapping the binary using
 * `TargetMemory::mmap`
 */
QBDL_API class TargetSystem {
public:
  TargetSystem(TargetMemory &mem) : mem_(mem) {}

  virtual ~TargetSystem() = default;

  /** Resolve external functions
   *
   * This function returns the absolute virtual address of an
   * external symbol. This is used by a ::QBDL::Loader object to
   * write the address of external symbol into the targeted
   * memory.
   *
   * @param[in] loader The current loader object that is calling this function
   * @param[in] sym The symbol to resolve
   * @returns The absolute virtual address of \p sym
   */
  virtual uint64_t symlink(Loader &loader, LIEF::Symbol const &sym) = 0;

  /** Verify that the target system supports a binary.
   *
   * This is mainly used by the ::QBDL::Loaders::MachO loader to
   * select a valid binary within a universal MachO.
   *
   * @param[in] bin Binary to verify
   * @returns true iif the target system can handle this binary.
   */
  virtual bool supports(LIEF::Binary const &bin) = 0;

  /** Compute the preferred based address where the binary should
   * be mapped.
   *
   * @param[in] binary_base_address The base address extracted from the binary
   * header
   * @param[in] virtual_size The complete size the binary would take in memory
   * (including all segments contiguously mapped with their relative offsets
   * respected).
   * @returns 0 if the loader can choose any address, otherwise a hint as to
   * what the base address should be.
   */
  virtual uint64_t base_address_hint(uint64_t binary_base_address,
                                     uint64_t virtual_size) = 0;

  TargetMemory &mem() { return mem_; }

private:
  TargetMemory &mem_;
};

} // namespace QBDL

#endif
