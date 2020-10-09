Introduction
============

The QuarkslaB Dynamic Linker (QBDL) library aims at providing a modular and
portable way to dynamically load and link binaries.

The goals of the library are:

* provide a simple-to-use abstraction to load dynamically linked binaries
* modular enough to be able to load binaries in foreign systems or lightweight sandboxes (e.g. `Miasm`_'s sandbox, or `Triton`_'s engine)
* support loading and dynamic linking of PE/ELF/MachO binaries
* cross-platform and portable library

Notable non goals of the library:

* provide full operating system (re)implementations, like `Wine`_ or
  `Darling`_.
* get the best performance out of all dynamic linkers. Said differently,
  performance can be scarified for better modularity, in order to make the
  library usable in various reverse engineering scenarios.
* supports architectures where pointer values are bigger than 64 bits.


.. _Miasm: https://miasm.re/
.. _Triton: https://triton.quarkslab.com/
.. _Wine: https://www.winehq.org/
.. _Darling: https://darlinghq.org/
