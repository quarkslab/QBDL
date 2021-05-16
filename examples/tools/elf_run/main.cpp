#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdarg.h>

#ifndef _WIN32
#include <dlfcn.h>
#include <unistd.h>
#endif

#include <LIEF/LIEF.hpp>
#include <QBDL/Engine.hpp>
#include <QBDL/engines/Native.hpp>
#include <QBDL/loaders/ELF.hpp>

using namespace QBDL;

extern "C" {
#ifndef _WIN32
static int __android_log_print(int, const char *tag, const char *msg, ...) {
  va_list args;
  va_start(args, msg);
  vprintf(msg, args);
  va_end(args);
  return 0;
}
#endif

#ifdef _WIN32
__attribute__((sysv_abi)) extern "C" int sysv_puts(const char* str) {
  return puts(str);
}
#endif
}

static std::unordered_map<std::string, uintptr_t> SYMS{
#ifndef _WIN32
    {"__android_log_print", reinterpret_cast<uintptr_t>(&__android_log_print)},
#endif
#ifdef _WIN32
    {"puts", reinterpret_cast<uintptr_t>(&sysv_puts)}
#endif
};

namespace {

struct FinalTargetSystem: public Engines::Native::TargetSystem {
  using Engines::Native::TargetSystem::TargetSystem;

  uint64_t symlink(Loader &, const LIEF::Symbol &sym) override {
    const std::string &name = sym.name();
    auto it_sym = SYMS.find(name);
    if (it_sym != std::end(SYMS)) {
      return it_sym->second;
    }

#ifndef _WIN32
    void *symAddr = dlsym(RTLD_DEFAULT, name.c_str());
    return reinterpret_cast<uint64_t>(symAddr);
#else
    void* symAddr = nullptr;
#endif
    if (symAddr == nullptr) {
      fprintf(stderr, "Can't resolve %s\n", name.c_str());
    }
    return reinterpret_cast<uint64_t>(symAddr);
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

  auto bin = LIEF::ELF::Parser::parse(path);
  if (!bin) {
    fprintf(stderr, "Unable to parse binary!\n");
    return 1;
  }
#ifndef _WIN32
  // dlopen every imported libraries
  for (const std::string &lib:
      bin->imported_libraries()) {
    const char* libname = lib.c_str();
    void* hdl = dlopen(lib.c_str(), RTLD_NOW);
    if (hdl == nullptr) {
      fprintf(stderr, "Warning: can't load library %s: %s\n", libname, dlerror());
    }
    else {
      fprintf(stderr, "Loaded imported library %s...\n", libname);
    }
  }
#endif

  std::unique_ptr<Loaders::ELF> loader = Loaders::ELF::from_binary(std::move(bin),
      *system, Loader::BIND::NOW);
  if (!loader) {
    fprintf(stderr, "unable to load binary!\n");
    return EXIT_FAILURE;
  }
  auto main =
      reinterpret_cast<int (*)(int, char **)>(loader->get_address("main"));
  if (main == nullptr) {
    fprintf(stderr, "Can't find symbol 'main'\n");
    return 1;
  }
  return main(argc - 1, &argv[1]);
}
