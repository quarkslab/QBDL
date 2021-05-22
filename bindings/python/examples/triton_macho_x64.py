from argparse import ArgumentParser
from triton import TritonContext, ARCH, CPUSIZE, MemoryAccess, OPCODE, Instruction
import string
import pyqbdl
import lief
import struct

def getMemoryString(ctx, addr):
    s = str()
    index = 0

    while ctx.getConcreteMemoryValue(addr+index):
        c = chr(ctx.getConcreteMemoryValue(addr+index))
        if c not in string.printable: c = ""
        s += c
        index  += 1

    return s

def puts(ctx):
    arg = getMemoryString(ctx, ctx.getConcreteRegisterValue(ctx.registers.rdi))
    print(arg)
    return 0


externalFunctions = [
    ('_puts', puts,  0xdeadc0de),
]

class TritonVM(pyqbdl.TargetMemory):
    def __init__(self, ctx: TritonContext):
        super().__init__()
        self.ctx = ctx

    def mmap(self, ptr, size):
        return ptr

    def mprotect(self, ptr, size, access):
        return True

    def write(self, ptr, data):
        self.ctx.setConcreteMemoryAreaValue(ptr, bytes(data))

    def read(self, ptr, size):
        return self.ctx.getConcreteMemoryAreaValue(ptr, size)

class TritonSystem(pyqbdl.TargetSystem):
    def __init__(self, arch, ctx):
        super().__init__(TritonVM(ctx))
        self.arch = arch
        self.ctx = ctx

    def symlink(self, loader, sym):
        for name, impl, addr in externalFunctions:
            if sym.name == name:
                return addr
        return 0

    def supports(self, bin_):
        return pyqbdl.Arch.from_bin(bin_) == self.arch

    def base_address_hint(self, bin_ba, vsize):
        return bin_ba



def hookingHandler(ctx):
    pc = ctx.getConcreteRegisterValue(ctx.registers.rip)
    for name, impl, addr in externalFunctions:
        if addr != pc:
            continue

        # Emulate the routine and the return value
        ret_value = impl(ctx)
        ctx.setConcreteRegisterValue(ctx.registers.rax, ret_value)

        # Get the return address
        ret_addr = ctx.getConcreteMemoryValue(MemoryAccess(ctx.getConcreteRegisterValue(ctx.registers.rsp), CPUSIZE.QWORD))

        # Hijack RIP to skip the call
        ctx.setConcreteRegisterValue(ctx.registers.rip, ret_addr)

        # Restore RSP (simulate the ret)
        ctx.setConcreteRegisterValue(ctx.registers.rsp, ctx.getConcreteRegisterValue(ctx.registers.rsp)+CPUSIZE.QWORD)
        return
    return

# Emulate the CheckSolution() function.
def emulate(ctx, pc):
    print('[+] Starting emulation.')

    while pc:
        # Fetch opcode
        opcode = ctx.getConcreteMemoryAreaValue(pc, 16)

        # Create the Triton instruction
        instruction = Instruction()
        instruction.setOpcode(opcode)
        instruction.setAddress(pc)

        # Process
        ctx.processing(instruction)
        print(instruction)

        if instruction.getType() == OPCODE.X86.HLT:
            break

        # Simulate routines
        hookingHandler(ctx)

        # Next
        pc = ctx.getConcreteRegisterValue(ctx.registers.rip)

    print('[+] Emulation done.')
    return



parser = ArgumentParser(description="Triton emulation with QBDL")
parser.add_argument("filename", help="x86-64 MachO filename")
args = parser.parse_args()

ctx = TritonContext()
ctx.setArchitecture(ARCH.X86_64)
x86_64_arch = pyqbdl.Arch(lief.ARCHITECTURES.X86, lief.ENDIANNESS.LITTLE, True)

loader = pyqbdl.loaders.MachO.from_file(args.filename, x86_64_arch,
                                        TritonSystem(x86_64_arch, ctx), pyqbdl.Loader.BIND.NOW)

ctx.setConcreteRegisterValue(ctx.registers.rbp, 0x7fffffff)
ctx.setConcreteRegisterValue(ctx.registers.rsp, 0x6fffffff)
emulate(ctx, loader.entrypoint)

