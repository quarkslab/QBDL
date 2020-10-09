# QuarkslaB Dynamic Loader

The QuarkslaB Dynamic Linker (QBDL) library aims at providing a modular and
portable way to dynamically load and link binaries.

## Quick example

Here is a quick example using the Python API to run simple MachO binaries under
Linux:

```python
import pyqbdl
import lief
import sys
import ctypes

class TargetSystem(pyqbdl.engines.Native.TargetSystem):
    def __init__(self):
        super().__init__(pyqbdl.engines.Native.memory())
        self.libc = ctypes.CDLL("libc.so.6")

    def symlink(self, loader: pyqbdl.Loader, sym: lief.Symbol) -> int:
        if not sym.name.startswith("_"):
            return 0
        ptr = getattr(self.libc, sym.name[1:], 0)
        if ptr != 0:
            ptr = ctypes.cast(ptr, ctypes.c_void_p).value
        return ptr

loader = pyqbdl.loaders.MachO.from_file(sys.argv[1], pyqbdl.engines.Native.arch(), TargetSystem(),
                                        pyqbdl.Loader.BIND.NOW)
main_type = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_int, ctypes.c_voidp)
main_ptr = main_type(loader.entrypoint)
main_ptr(0, ctypes.c_void_p(0))
```

For more detailed use cases, please refer to [the documentation](docs/use_cases.rst).

## Quick usage

A dockerfile is provided to quickly test the library. From the root of the
project, running:

```
docker build -t quarkslab/qbdl --target runner -f ./docker/qbdl.dockerfile .
```

will create a ``quarkslab/qbdl`` docker image containing a compiled version of
LIEF, QBDL and some examples in the `/qbdl` directory:

```
# docker run --rm -it quarkslab/qbdl /bin/bash
root@...:/qbdl# python3 ./miasm_macho_x64.py macho-o-x86-64-hello.bin -j python
Loading macho-o-x86-64-hello.bin
this: 0x1e28d50
Virtual size: 0x3000
Mapping __PAGEZERO - 0x0
Mapping __TEXT - 0x0
Mapping __DATA - 0x1000
Mapping __LINKEDIT - 0x2000
Symbol dyld_stub_binder resolves to address 0x700000000000, stored at address 0x100001000
Symbol _puts resolves to address 0x700000000008, stored at address 0x100001010
Addr               Size               Access Comment
0x1230000          0x10000            RW_    Stack
0x100000000        0x3000             RW_    

Calling _puts
coucou
[+] End!
```
