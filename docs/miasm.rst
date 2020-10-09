Load binaries in the Miasm VM
-----------------------------

`Miasm`_ is a reverse engineering framework which, among many things, provides a
full emulation engine for many architectures. Miasm already provides loaders
for PE and ELF binaries for this engine, that loads the binaries into his own
VM (virtual machine). In this section, we'll see how QBDL can be used to load
MachO binaries into the Miasm VM, and then use the emulation engine to actually
run the binary.

The full example can be downloaded :download:`here <../bindings/python/examples/miasm_macho_x64.py>`.

Instantiate the Miasm emulation engine
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This step is mainly based on `an example in Miasm's repository <https://github.com/cea-sec/miasm/blob/381d370b73b980190e9a24753a595b043193bec1/example/jitter/x86_64.py>`_:

.. literalinclude:: ../bindings/python/examples/miasm_macho_x64.py
   :start-after: ##begin_miasm_imports
   :end-before: ##end_miasm_imports

.. literalinclude:: ../bindings/python/examples/miasm_macho_x64.py
   :start-after: ##begin_miasm_init
   :end-before: ##end_miasm_init

What's happening here is the initialisation of an x64 Miasm emulator. We also
allocate a simple stack. We can then call ``print(myjit.vm)`` to see the current
memory mapping of the VM:

.. code-block::

   Addr               Size               Access Comment
   0x1230000          0x10000            RW_    Stack


Define the target machine
~~~~~~~~~~~~~~~~~~~~~~~~~

We will now define a QBDL target machine to inject the loaded binary into the Miasm VM.

First, we need to define how memory is handled, through the
creation of a :py:class:`~pyqbdl.TargetMemory`-based class:

.. literalinclude:: ../bindings/python/examples/miasm_macho_x64.py
   :start-after: ##begin_target_memory
   :end-before: ##end_target_memory

This is what allows us to load the binary inside Miasm's VM, and not in the running process.

We now need to define the final target machine, with the resolution of external
symbols, through the creation of a :py:class:`~pyqbdl.TargetSystem`-based class:

.. literalinclude:: ../bindings/python/examples/miasm_macho_x64.py
   :start-after: ##begin_target_system
   :end-before: ##end_target_system

``ExternFuncs`` is a class that handles the calls to external functions. What we
do is adding a breakpoint (on an arbitrary address of your choice) in Miasm's
VM for each external symbol. Each of this breakpoint will call a Python
function that mimic the behavior of the external function. Miasm already
provides such implementation for a part of C library. This is far from complete
but will be sufficient for our example.

Here is the code that handles this part:

.. literalinclude:: ../bindings/python/examples/miasm_macho_x64.py
   :start-after: ##begin_extern_funcs_add
   :end-before: ##end_extern_funcs_add

When a breakpoint is hit, it will execute the ``ExternFuncs.call`` function,
which simply does:

.. literalinclude:: ../bindings/python/examples/miasm_macho_x64.py
   :start-after: ##begin_extern_funcs_call
   :end-before: ##end_extern_funcs_call


Load the actual binary
~~~~~~~~~~~~~~~~~~~~~~

This is now time to put all the pieces together and use QBDL to load the binary
inside our initialized Miasm VM, using a ``MiasmSystem`` object:

.. literalinclude:: ../bindings/python/examples/miasm_macho_x64.py
   :start-after: ##begin_extern_funcs_call
   :end-before: ##end_extern_funcs_call

After this initialization, we can see that a new mapping appeared in the Miasm
VM, in ``0x100000000`` (which is the base address in our MachO example binary):

.. code-block::

  Addr               Size               Access Comment
  0x1230000          0x10000            RW_    Stack
  0x100000000        0x3000             RW_    


Last but not least, run the code!
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The last thing we need to do is to tell Miasm to run the code at ``entrypoint``:

.. literalinclude:: ../bindings/python/examples/miasm_macho_x64.py
   :start-after: ##begin_miasm_run
   :end-before: ##end_miasm_run

Using :download:`this example MachO binary <../examples/macho-o-x86-64-hello.bin>`, which
is supposed to print the ``coucou`` string, here is the output:

.. code-block::

   Addr               Size               Access Comment
   0x1230000          0x10000            RW_    Stack
    
   Loading examples/macho-o-x86-64-hello.bin
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


.. _Miasm: https://miasm.re/
