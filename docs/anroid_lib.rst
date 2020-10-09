Running Android Library on Linux
--------------------------------

The blog post `When SideChannelMarvels meets LIEF`_ describes a technique to load an Android x86-64
library on Linux. Since Linux and Android share almost the same ABI [1]_, the technique described in the blog post consists
in patching ELF dependencies and symbol versions so that, for instance, the ``libc.so`` library on Android is resolved
into ``libc.so.6`` on Linux.
After having **statically** patched the library, the library can be loaded with ``dlopen()``.

This section introduces another way to load the Android library on Linux with QBDL.

Loading the Library with QBDL
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Since the given Android library aims at being running on our native Linux system, we can use the *native*
memory model in which we share the same memory space as the loaded library:

.. code-block:: cpp

   auto mem = std::make_unique<QBDL::Engines::Native::TargetMemory>();

With this memory model, the loader will use the classical ``mmap(), read(), write(), ...`` functions
to load the Android library.

Regarding the targeted system, we share the same native architecture ``x86-64`` so that we can inherit from
:cpp:class:`QBDL::Engines::Native::TargetSystem` and overload the symbol resolution function (``symlink``):

.. code-block:: cpp

  struct FinalTargetSystem: public QBDL::Engines::Native::TargetSystem {
   using QBDL::Engines::Native::TargetSystem::TargetSystem;

   uint64_t symlink(Loader &loader, const LIEF::Symbol &sym) override {
     // Function that resolves library's imported symbols
   }

  };


The library can then be loaded with :cpp:func:`QBDL::Loaders::ELF::from_file()`:

.. code-block:: cpp

   std::unique_ptr<QBDL::ELF::Loader> loader = QBDL::Loaders::ELF::from_file(
     "SECCON2016_whitebox.so",
     *system,
     QBDL::Loader::BIND::NOW
   );

Symbols resolution
~~~~~~~~~~~~~~~~~~

As we loaded the library with the ``BIND::NOW`` option, all the imported symbols are resolved at once
when the library is loaded.

To resolve the symbols, the loader calls our overloaded function ``symlink()`` defined in the structure
``struct FinalTargetSystem``.

The library imports only few symbols (``__cxa_finalize``, ``__cxa_atexit``, ``__stack_chk_fail``) which are all
exported by the ``libc``. Therefore, we could write this (non-generic) resolver:

.. code-block:: cpp

   void* libc_hdl = dlopen("libc.so.6", RTLD_NOW);

   uint64_t symlink(Loader &loader, const LIEF::Symbol &sym) override {
     printf("Resolving %s\n", sym.name().c_str());
     return dlsym(libc_hdl, sym.name().c_str());
   }


Running the Whitebox Function
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Once the library loaded, we get a pointer on the whiteboxed function that performs encryption
using :cpp:func:`QBDL::Loader::get_address()`:

.. code-block:: cpp

   const uintptr_t fcn_addr = loader->get_address("_Z48TfcqPqf1lNhu0DC2qGsAAeML0SEmOBYX4jpYUnyT8qYWIlEqPhS_");
   using wb_fcn_t = uint64_t(*)(unsigned char*, unsigned char*);
   auto aes_128_encrypt = reinterpret_cast<wb_fcn_t>(fcn_addr);

Finally, we can use the function at our convenience:

.. code-block:: cpp

  unsigned char plaintext[16] = {0};
  unsigned char ciphertext[16];

  aes_128_encrypt(plaintext, ciphertext);
  for (unsigned char c : ciphertext) {
    printf("%02x ", c);
  }
  printf("\n");
  return 0;

It results in this kind of output:

.. code-block:: console

  Loading examples/SECCON2016_whitebox.so
  this: 0x55c9513a3db0
  Virtual size: 0x2d000
  mmap(0x0, 0x2d000): 0x7f7e60cd4000
  Mapping LOAD - 0x0
  Mapping LOAD - 0x2bce8
  Resolving __cxa_atexit
  Resolving __cxa_finalize
  Resolving __stack_chk_fail
  8c f2 fb f4 21 75 fe 04 9a d6 3b e8 00 2e 9e 5c


.. _When SideChannelMarvels meets LIEF: https://blog.quarkslab.com/when-sidechannelmarvels-meet-lief.html

.. [1] https://developer.android.com/ndk/guides/abis



