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

namespace {

struct FinalTargetSystem: public Engines::Native::TargetSystem {
  using Engines::Native::TargetSystem::TargetSystem;
  inline static void* libc_hdl = dlopen("libc.so.6", RTLD_NOW);

  uint64_t symlink(Loader &loader, const LIEF::Symbol &sym) override {
    printf("Resolving %s\n", sym.name().c_str());
    return reinterpret_cast<uint64_t>(dlsym(libc_hdl, sym.name().c_str()));
  }
};

}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s whitebox.so\n", argv[0]);
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
  using wb_fcn_t = uint64_t(*)(unsigned char*, unsigned char*);
  auto aes_128_encrypt =
      reinterpret_cast<wb_fcn_t>(loader->get_address("_Z48TfcqPqf1lNhu0DC2qGsAAeML0SEmOBYX4jpYUnyT8qYWIlEqPhS_"));
  if (aes_128_encrypt == nullptr) {
    fprintf(stderr, "Can't find symbol 'find'\n");
    return 1;
  }
  unsigned char plaintext[16] = {0};
  unsigned char ciphertext[16];

  aes_128_encrypt(plaintext, ciphertext);
  for (unsigned char c : ciphertext) {
    printf("%02x ", c);
  }
  printf("\n");
  return 0;
}
