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
