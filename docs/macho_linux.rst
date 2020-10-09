Run simple MachO binaries under Linux
-------------------------------------

QBDL ships the ``run_macho`` tool that can run "simple" MachO binaries under
Linux. It tries to map calls to OSX's ``libSystem.dylib`` to Linux's ``glibc``.

Full source code of the tool is in the ``tools/macho_run`` directory.

QBDL usage
~~~~~~~~~~

We use the native engine, using the classes
:cpp:class:`QBDL::Engines::Native::TargetMemory` and
:cpp:class:`QBDL::Engines::Native::TargetSystem`.

The only thing that we have to define is the
:cpp:func:`QBDL::TargetSystem::symlink` function. For this simple use case,
let's define a map of the symbols we support, and their effective address
(resolved at static link time):

.. code-block:: cpp

  static std::unordered_map<std::string, uintptr_t> SYMS{
      {"_puts", reinterpret_cast<uintptr_t>(&puts)},
      {"_printf", reinterpret_cast<uintptr_t>(&printf)}};

The ``symlink`` is a lookup in this map:

.. code-block:: cpp

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

The only thing that remains is glue all of this together and run the binary:

.. code-block:: cpp

  auto mem = std::make_unique<Engines::Native::TargetMemory>();
  auto system = std::make_unique<FinalTargetSystem>(*mem);

  std::unique_ptr<Loaders::MachO> loader = Loaders::MachO::from_file(
      path, Engines::Native::arch(), *system, Loader::BIND::NOW);
  if (!loader) {
    fprintf(stderr, "unable to load binary!\n");
    return EXIT_FAILURE;
  }
  auto main = reinterpret_cast<int (*)(int, char **)>(loader->entrypoint());
  return main(argc - 1, &argv[1]);


ABI issues
~~~~~~~~~~

This tool can also be used to run ARM64 MachO binaries. Emulation for Intel
systems can be achieved by using the `QEMU user mode
<https://qemu-project.gitlab.io/qemu/user/index.html>`_.

Unfortunately, while OSX and Linux x64 ABIs are pretty close, there are `non
negligeable differences between Linux and OSX ARM64 ABIs
<https://developer.apple.com/documentation/xcode/writing_arm64_code_for_apple_platforms>`_.
So, we can't just "forward" calls to ``libSystem.dylib`` functions to Linux's
``glibc`` implementation.

In order to fix this issue, an approach has been to implement a
``__attribute__((darwin_abi))`` attribute in Clang, similar in what
``__attribute__((ms_abi))`` does today. This allows us to generate "ABI
wrappers", like this one for ``printf``:

.. code-block:: c

  __attribute__((darwin_abi)) int darwin_aarch64_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    const int ret = vprintf(format, args);
    va_end(args);
    return ret;
  }

This feature isn't merged in Clang/LLVM yet. The patch is `available here
<https://reviews.llvm.org/D89490>`_.


Going further
~~~~~~~~~~~~~

As seen, this sample tool will run very simple binary. It could be extended
thought to support more "libc" functions.

That being said, if you are looking for a project that run more complex OSX
binaries, `Darling`_ would be the way to go. It is interesting to note that
`Darling`_ used to have a similar approach than here, but moved to a
completely different model. If you are curious about this, `this very
interesting blog post
<http://blog.darlinghq.org/2017/02/the-mach-o-transition-darling-in-past-5.html>`_
explains the reasons why.

.. _Darling: https://darlinghq.org/
