#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include <LIEF/LIEF.hpp>
#include <QBDL/Engine.hpp>
#include <QBDL/engines/Native.hpp>
#include <QBDL/loaders/MachO.hpp>

#ifdef __aarch64__
#include "darwin_aarch64_libc.h"
#define SYM_ARCH(name) darwin_aarch64_##name
#else
#define SYM_ARCH(name) ::name
#endif

static std::unordered_map<std::string, uintptr_t> SYMS{
    {"_puts", reinterpret_cast<uintptr_t>(&SYM_ARCH(puts))},
    {"_printf", reinterpret_cast<uintptr_t>(&SYM_ARCH(printf))}};

using namespace QBDL;

namespace {
struct FinalTargetSystem: public Engines::Native::TargetSystem {
  using Engines::Native::TargetSystem::TargetSystem;

  uint64_t symlink(Loader &loader, const LIEF::Symbol &sym) override {
    const std::string &name = sym.name();
    auto it_sym = SYMS.find(name);
    if (it_sym == std::end(SYMS)) {
      fprintf(stderr, "Symbol %s not resolved!\n", name.c_str());
      return 0;
    }
    return it_sym->second;
  }
};
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <binary> args\n", argv[0]);
    return EXIT_FAILURE;
  }

  auto mem = std::make_unique<Engines::Native::TargetMemory>();
  auto system = std::make_unique<FinalTargetSystem>(*mem);

  const char *path = argv[1];

  std::unique_ptr<Loaders::MachO> loader = Loaders::MachO::from_file(
      path, Engines::Native::arch(), *system, Loader::BIND::NOW);
  if (!loader) {
    fprintf(stderr, "unable to load binary!\n");
    return EXIT_FAILURE;
  }
  auto main = reinterpret_cast<int (*)(int, char **)>(loader->entrypoint());
  return main(argc - 1, &argv[1]);
}
