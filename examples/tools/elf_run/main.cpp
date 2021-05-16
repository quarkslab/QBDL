#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <unistd.h>
#include <stdarg.h>

#include <LIEF/LIEF.hpp>
#include <QBDL/Engine.hpp>
#include <QBDL/engines/Native.hpp>
#include <QBDL/loaders/ELF.hpp>

using namespace QBDL;

std::unordered_map<std::string, void *> dlmap;
int __android_log_print(int, const char *tag, const char *msg, ...);

static std::unordered_map<std::string, uintptr_t> SYMS{
    {"getpid", reinterpret_cast<uintptr_t>(&::getpid)},
    {"printf", reinterpret_cast<uintptr_t>(&::printf)},
    {"__android_log_print", reinterpret_cast<uintptr_t>(&__android_log_print)}};

int __android_log_print(int, const char *tag, const char *msg, ...) {
  va_list args;
  va_start(args, msg);
  vprintf(msg, args);
  va_end(args);
  return 0;
}


namespace {

struct FinalTargetSystem: public Engines::Native::TargetSystem {
  using Engines::Native::TargetSystem::TargetSystem;

  uint64_t symlink(Loader &loader, const LIEF::Symbol &sym) override {
    auto &elfLoader = reinterpret_cast<Loaders::ELF &>(loader);
    const std::string &name = sym.name();
    auto it_sym = SYMS.find(name);
    if (it_sym != std::end(SYMS)) {
      return it_sym->second;
    }

    for (const std::string &lib :
        elfLoader.get_binary().imported_libraries()) {
      auto res = dlmap.insert({lib, dlopen(lib.c_str(), RTLD_NOW)});
      void *hdl = res.first->second;
      if (hdl == nullptr) {
        fprintf(stderr, "Can't load %s\n", dlerror());
        return 0;
      }
      void *symAddr = dlsym(hdl, name.c_str());
      if (symAddr == nullptr) {
        continue;
      }
      return reinterpret_cast<uintptr_t>(symAddr);
    }

    fprintf(stderr, "Can't resolve %s\n", name.c_str());
    return 0;
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

  std::unique_ptr<Loaders::ELF> loader = Loaders::ELF::from_file(
      path, *system, Loader::BIND::NOW);
  if (!loader) {
    fprintf(stderr, "unable to load binary!\n");
    return EXIT_FAILURE;
  }
  auto main =
      reinterpret_cast<int (*)(int, char **)>(loader->get_address("main"));
  if (main == nullptr) {
    fprintf(stderr, "Can't find symbol 'find'\n");
    return 1;
  }
  return main(argc - 1, &argv[1]);
}
