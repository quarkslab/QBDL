from argparse import ArgumentParser
##begin_miasm_imports
from miasm.jitter.csts import PAGE_READ, PAGE_WRITE, EXCEPT_SYSCALL
from miasm.analysis.machine import Machine
from miasm.core.locationdb import LocationDB
from miasm.jitter.csts import PAGE_READ, PAGE_WRITE, EXCEPT_SYSCALL
from miasm.core.utils import pck64, upck64
from miasm.os_dep import linux_stdlib
##end_miasm_imports
import pyqbdl
import lief

class ExternFuncs:
    def __init__(self, jitter):
        self.name2addr = {}
        self.curAddr = 0x700000000000
        self.jitter = jitter

##begin_extern_funcs_add
    def symlink(self, loader, sym) -> int:
        return self.add(sym.name)

    def add(self, name):
        ret = self.name2addr.get(name, None)
        if ret is not None:
            return ret
        addr = self.curAddr
        self.curAddr += 8
        self.name2addr[name] = addr

        self.jitter.add_breakpoint(addr, lambda jitter: self.call(jitter, name))
        return addr
##end_extern_funcs_add

##begin_extern_funcs_call
    def call(self, jitter, name):
        print("Calling %s" % name)
        getattr(linux_stdlib, "xxx%s" % name)(jitter)
        return True
##end_extern_funcs_call

##begin_target_memory
class MiasmVM(pyqbdl.TargetMemory):
    def __init__(self, vm):
        super().__init__()
        self.vm = vm

    def mmap(self, ptr, size):
        self.vm.add_memory_page(ptr, PAGE_READ | PAGE_WRITE, b"\x00"*size)
        return ptr

    def mprotect(self, ptr, size, access):
        return True

    def write(self, ptr, data):
        self.vm.set_mem(ptr, bytes(data))

    def read(self, ptr, size):
        return self.vm.get_mem(ptr, size)
##end_target_memory

##begin_target_system
class MiasmSystem(pyqbdl.TargetSystem):
    def __init__(self, jit, arch):
        super().__init__(MiasmVM(jit.vm))
        self.externs = ExternFuncs(jit)
        self.arch = arch

    def symlink(self, loader, sym):
        return self.externs.symlink(loader, sym)

    def supports(self, bin_):
        return pyqbdl.Arch.from_bin(bin_) == self.arch

    def base_address_hint(self, bin_ba, vsize):
        return bin_ba 
##end_target_system

parser = ArgumentParser(description="x86 64 basic Jitter")
parser.add_argument("filename", help="x86 64 MachO filename")
parser.add_argument("-j", "--jitter",
                    help="Jitter engine (default is 'python')",
                    default="python")
parser.add_argument("--verbose", "-v", action="store_true",
                    help="Verbose mode")
args = parser.parse_args()

# Miasm VM initialisation

##begin_miasm_init
loc_db = LocationDB()
myjit = Machine("x86_64").jitter(loc_db, args.jitter)
myjit.init_stack()
##end_miasm_init

# Load the binary

##begin_qbdl_load
x86_64_arch = pyqbdl.Arch(lief.ARCHITECTURES.X86, lief.ENDIANNESS.LITTLE, True)

system = MiasmSystem(myjit, x86_64_arch)

loader = pyqbdl.loaders.MachO.from_file(args.filename, x86_64_arch, system, pyqbdl.Loader.BIND.NOW)
##end_qbdl_load

print(myjit.vm)

# Run the binary

##begin_miasm_run
def code_sentinelle(jitter):
    print("[+] End!")
    return False

myjit.push_uint64_t(0x1337beef)
myjit.add_breakpoint(0x1337beef, code_sentinelle)
myjit.run(loader.entrypoint)
##end_miasm_run
