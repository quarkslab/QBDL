#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include <LIEF/LIEF.hpp>
#include <QBDL/Engine.hpp>
#include <QBDL/engines/Native.hpp>
#include <QBDL/loaders/PE.hpp>
using namespace QBDL;

namespace {
uintptr_t __acrt_iob_func(int index) {
  printf("In __acrt_iob_func\n");
  return 0;
}

int __stdio_common_vfprintf(uint64_t opt, uintptr_t file,
                                   const char *fmt, uintptr_t locale,
                                   void *valist) {
  printf("In __stdio_common_vfprintf\n");
  return 0;
}

std::unordered_map<std::string, uintptr_t> SYMS{
    {"__acrt_iob_func", reinterpret_cast<uintptr_t>(&__acrt_iob_func)},
    {"__stdio_common_vfprintf",
     reinterpret_cast<uintptr_t>(&__stdio_common_vfprintf)},
};


struct FinalTargetSystem : public Engines::Native::TargetSystem {
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
} // namespace

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <binary> args\n", argv[0]);
    return EXIT_FAILURE;
  }

  auto mem = std::make_unique<Engines::Native::TargetMemory>();
  auto system = std::make_unique<FinalTargetSystem>(*mem);

  const char *path = argv[1];

  std::unique_ptr<Loaders::PE> loader =
      Loaders::PE::from_file(path, *system, Loader::BIND::DEFAULT);

  if (!loader) {
    fprintf(stderr, "unable to load binary!\n");
    return EXIT_FAILURE;
  }
  auto main = reinterpret_cast<int (*)(int, char **)>(loader->entrypoint());
  return main(argc - 1, &argv[1]);
}
